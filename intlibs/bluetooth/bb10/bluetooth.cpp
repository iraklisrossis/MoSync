#include "../config_bluetooth.h"

#include <helpers/helpers.h>
#include <helpers/log.h>
#include <helpers/CriticalSection.h>

#include "../discovery.h"
#include "../discInternal.h"
#include "../server.h"
#include "../btinit.h"
#include "bb10err.h"

#include "../bt_errors.h"
using namespace MoSyncError;

#include <btapi/btdevice.h>
#include <btapi/btspp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace Bluetooth;

static Bt gBt;
static bool sCanceled;

#define TEST(truth) if(!(truth)) { IN_FILE_ON_LINE; LOG("%i: %c(0x%02x)\n", i, buf[i], buf[i]); return CONNERR_INTERNAL; }
#define TCHARtoCHAR memcpy
#define ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

static void* doDiscovery(void*);
//static void* doSearch(void*);


static void printBtAddr(char* buf, const MABtAddr& addr) {
	const byte* a = addr.a;
	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
		a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int parseBtAddr(const char* buf, MABtAddr& addr) {
	int i = 17, j;
	TEST(buf[i] == 0);
	for(i=0, j=0; i<17; ) {
		const char* ptr = buf+i;
		TEST(isxdigit(buf[i])); i++;
		TEST(isxdigit(buf[i])); i++;
		if(i < 15) {
			TEST(buf[i] == ':'); i++;
		}
		addr.a[j++] = (byte)strtoul(ptr, NULL, 16);
	}
	return 1;
}

int Bluetooth::getLocalAddress(MABtAddr& addr) {
	char buf[18];
	ERRNO(bt_ldev_get_address(buf));
	return parseBtAddr(buf, addr);
}

class BB10BtSppConnection : public BtSppConnection {
public:
	BB10BtSppConnection(const MABtAddr&, const char* uuid);
	~BB10BtSppConnection();

	int connect();
	bool isConnected();
	int read(void* dst, int max);
	int write(const void* src, int len);
	void close();
	int getAddr(MAConnAddr& addr);
private:
	int mSock;
	const MABtAddr mAddr;
	char mUuid[37];

	BB10BtSppConnection(int sock);
	friend class BtSppServer;
};

BtSppConnection* createBtSppConnection(const MABtAddr& addr, const char* uuid) {
	return new BB10BtSppConnection(addr, uuid);
}

BB10BtSppConnection::BB10BtSppConnection(const MABtAddr& addr, const char* uuid)
: mSock(-1), mAddr(addr)
{
	DEBUG_ASSERT(strlen(uuid) == 32);
	//#define SPP_SERVICE_UUID "00001101-0000-1000-8000-00805F9B34FB"
	static const uint uuidLens[] = { 8, 4, 4, 4, 12 };
	static const uint nUuidParts = ARRAY_LEN(uuidLens);
	char* dst = mUuid;
	const char* src = uuid;
	for(uint i=0; i<nUuidParts; i++) {
		uint len = uuidLens[i];
		memcpy(dst, src, len);
		src += len;
		dst += len;
		if(i != nUuidParts-1) {
			*dst++ = '-';
		}
	}
	*dst++ = 0;
	DEBUG_ASSERT((dst - mUuid) == sizeof(mUuid));
}
BB10BtSppConnection::~BB10BtSppConnection() {
	close();
}

BB10BtSppConnection::BB10BtSppConnection(int fd) : mSock(fd), mAddr(MABtAddr()) {
	DEBUG_ASSERT(fd > 0);
}

int BB10BtSppConnection::connect() {
	char addrBuf[18];
	printBtAddr(addrBuf, mAddr);
	//LOG("%s %s\n", addrBuf, mUuid);
	mSock = bt_spp_open(addrBuf, mUuid, false);
	if(mSock < 0) {
		LOG_ERRNO;
		return CONNERR_GENERIC;
	}
	return 1;
}

bool BB10BtSppConnection::isConnected() {
	return mSock > 0;
}

int BB10BtSppConnection::read(void* dst, int max) {
	int bytesRecv = ::read(mSock, dst, max);
	if(bytesRecv < 0) {
		LOG_ERRNO;
		return CONNERR_GENERIC;
	} else if (bytesRecv == 0) {
		return CONNERR_CLOSED;
	} else {
		return bytesRecv;
	}
}

int BB10BtSppConnection::write(const void* src, int len) {
	int bytesSent = ::write(mSock, src, len);
	if(bytesSent != len) {
		DUMPINT(bytesSent);
		LOG_ERRNO;
		return CONNERR_GENERIC;
	} else {
		return 1;
	}
}


void BB10BtSppConnection::close() {
	if(isConnected()) {
		ERRNO(bt_spp_close(mSock));
		mSock = -1;
	}
}

int BB10BtSppConnection::getAddr(MAConnAddr&) {
	return IOCTL_UNAVAILABLE;
}

static void btServerCallback(long param, int fd) {
	BtSppServer* s = (BtSppServer*)param;
	LOG("btServerCallback %i\n", fd);
	s->callback(fd);
}
void BtSppServer::callback(int fd) {
	if(!mIsAccepting)
		return;
	mFd = fd;
	mErrno = errno;
	mSem.post();
}

int BtSppServer::open(MAUUID const& uuid, char const* name, int port) {
	mIsOpen = false;
	mIsAccepting = false;
	DEBUG_ASSERT(port == ANY_PORT);
	const uint* u = (uint*)uuid.i;
	//#define SPP_SERVICE_UUID "00001101-0000-1000-8000-00805F9B34FB"
	sprintf(mUuid, "%08X-%04X-%04X-%04X-%04X%08X",
		u[0], u[1] >> 16, u[1] & 0xFFFF, u[2] >> 16, u[2] & 0xFFFF, u[3]);
	LOG("%s\n", mUuid);
	int res = bt_spp_open_server((char*)name, mUuid, false, btServerCallback, (long)this);
	if(res < 0) {
		LOG_ERRNO;
		return CONNERR_GENERIC;
	}
	mIsOpen = true;
	return 1;
}
int BtSppServer::getAddr(MAConnAddr& addr) {
	addr.bt.port = -1;
	addr.family = CONN_FAMILY_BT;
	return getLocalAddress(addr.bt.addr);
}
int BtSppServer::accept(BtSppConnection*& connP) {
	DEBUG_ASSERT(mIsOpen);
	DEBUG_ASSERT(!mIsAccepting);
	mIsAccepting = true;
	mSem.wait();
	if(mFd < 0) {
		if(mErrno == ECANCELED)
			return CONNERR_CANCELED;
		else
			return CONNERR_GENERIC;
	}
	connP = new BB10BtSppConnection(mFd);
	mIsAccepting = false;
	return 1;
}
void BtSppServer::close() {
	if(mIsOpen) {
		if(mIsAccepting) {
			mIsAccepting = false;
			mFd = -1;
			mErrno = ECANCELED;
			mSem.post();
		}
		ERRNO(bt_spp_close_server(mUuid));
		mIsOpen = false;
	}
}


static void btCallback(const int event, const char* bt_addr, const char* event_data) {
	LOG("btCallback %i '%s' '%s'\n", event, bt_addr, event_data);
	switch(event) {
	case BT_EVT_DEVICE_ADDED:
		if(gBt.discoveryState == 0 && gBt.deviceCallback != NULL) {
			CriticalSectionHandler csh(&gBt.critSec);
			BtDevice d;
			DEBUG_ASSERT(parseBtAddr(bt_addr, d.address) > 0);
			if(gBt.names) {
				char name[256];
				bt_remote_device_t* r = bt_rdev_get_device(bt_addr);
				ERRNO(bt_rdev_get_friendly_name(r, name, sizeof(name)));
				d.name = name;
				bt_rdev_free(r);
			}
			gBt.devices.push_back(d);
			gBt.deviceCallback();
		}
		break;
	}
}

void Bluetooth::MABtInit() {
	gBt.discoveryState = CONNERR_UNINITIALIZED;
	gBt.nextDevice = 0;
	gBt.deviceCallback = NULL;

	InitializeCriticalSection(&gBt.critSec);

	ERRNO(bt_device_init(btCallback));
	ERRNO(bt_spp_init());
}

void Bluetooth::MABtClose() {
	DeleteCriticalSection(&gBt.critSec);
	ERRNO(bt_spp_deinit());
	bt_device_deinit();
}

// <0 CONNERR on failure. 0 while still working. (number of devices/services found) + 1 on success.
int Bluetooth::maBtDiscoveryState() {
	return gBt.discoveryState;
}

int Bluetooth::maBtStartDeviceDiscovery(MABtCallback cb, bool names) {
	DEBUG_ASSERT(cb != NULL);
	MYASSERT(gBt.discoveryState != 0, BTERR_DISCOVERY_IN_PROGRESS);

	//check for active bluetooth radios in the system.
	//if we don't have one, tell the user.
	if(!bt_ldev_get_power()) {
		return -1;
	}

	CriticalSectionHandler csh(&gBt.critSec);
	gBt.discoveryState = 0;
	gBt.nextDevice = 0;
	gBt.devices.clear();
	gBt.deviceCallback = cb;
	gBt.names = names;
	sCanceled = false;

	// use thread? posix timers seem FAR too complicated.
	// yeah.

	ERRNO_RET(pthread_create(NULL, NULL, doDiscovery, NULL));

	return 0;
}

static void* doDiscovery(void*) {
	LOGT("doDiscovery start");
	// call bt_disc_start_inquiry, then
	ERRNO(bt_disc_start_inquiry(BT_INQUIRY_GIAC));

#if 1
	#if 0
	LOGT("sleep(10) start");
	sleep(10);
	LOGT("sleep(10) done");
	#endif
#else
	// wait one second to allow BT_EVT_DEVICE_DELETED events to arrive, then
	sleep(1);

	// call bt_disc_retrieve_devices(BT_DISCOVERY_CACHED).
	bool has = false;
	{
		CriticalSectionHandler csh(&gBt.critSec);
		bt_remote_device_t** devices = bt_disc_retrieve_devices(BT_DISCOVERY_CACHED, NULL);
		NULL_ERRNO(devices);
		for(int i=0; devices[i] != NULL; i++) {
			// populate BtDevice from bt_remote_device_t.
			bt_remote_device_t* r(devices[i]);
			BtDevice d;
			char buf[18];
			ERRNO(bt_rdev_get_addr(r, buf));
			DEBUG_ASSERT(parseBtAddr(buf, d.address) > 0);
			if(gBt.names) {
				char name[256];
				ERRNO(bt_rdev_get_friendly_name(r, name, sizeof(name)));
				d.name = name;
			}
			gBt.devices.push_back(d);
			has = true;
		}
		bt_rdev_free_array(devices);
	}
	if(has)
		gBt.deviceCallback();
	// sleep 9 more seconds, then
	sleep(8);
#endif

	// finish up.
	gBt.discoveryState = gBt.devices.size() + 1;
	gBt.deviceCallback();
	LOGT("doDiscovery end");
	return NULL;
}

int Bluetooth::maBtCancelDiscovery() {
	CriticalSectionHandler csh(&gBt.critSec);
	if(gBt.discoveryState != 0)
		return 0;

	sCanceled = true;
	return 1;
}

int Bluetooth::maBtGetNewDevice(MABtDeviceNative* dst) {
	CriticalSectionHandler csh(&gBt.critSec);
	LOGBT("maBtGetNewDevice(). next %i, size %i\n", gBt.nextDevice, gBt.devices.size());
	if(gBt.nextDevice == gBt.devices.size()) {
		return 0;
	}
	const BtDevice& dev = gBt.devices[gBt.nextDevice++];
	memcpy(dst->address.a, dev.address.a, BTADDR_LEN);
	if(gBt.names) {
		uint strLen = MIN(dev.name.size(), uint(dst->nameBufSize - 1));
		TCHARtoCHAR(dst->name, dev.name.c_str(), strLen);
		dst->name[strLen] = 0;
	}
	return 1;
}

int Bluetooth::maBtStartServiceDiscovery(const MABtAddr* address, const MAUUID* uuid,
	MABtCallback cb)
{
#if 1
	// we can't get port numbers, so service discovery is not practical.
	// we could do the special case of "all SPP services",
	// but with Android's API being what it is (pre-level-15),
	// most people don't care.
	return IOCTL_UNAVAILABLE;
}
int Bluetooth::maBtGetNewService(MABtServiceNative* dst) {
	return IOCTL_UNAVAILABLE;
}
int Bluetooth::maBtGetNextServiceSize(MABtServiceSize* dst) {
	return IOCTL_UNAVAILABLE;
}

#else
	DEBUG_ASSERT(cb != NULL);
	MYASSERT(gBt.discoveryState != 0, BTERR_DISCOVERY_IN_PROGRESS);
	if(!bt_ldev_get_power()) {
		return -1;
	}

	CriticalSectionHandler csh(&gBt.critSec);
	sCanceled = false;
	BtServiceSearch& s(gBt.serviceSearch);
	s.cb = cb;
	s.services.clear();
	s.nextService = 0;
	gBt.discoveryState = 0;
	s.uuid = *uuid;
	s.address = *address;
	ERRNO_RET(pthread_create(NULL, NULL, doSearch, NULL));
	return 0;
}

static void* doSearch(void*) {
	BtServiceSearch& s(gBt.serviceSearch);
	char buf[18];
	printBtAddr(buf, s.address);
	LOGT("bt_rdev_get_device start\n");
	bt_remote_device_t* r = bt_rdev_get_device(buf);
	LOGT("bt_rdev_get_device done\n");
	LOGT("bt_rdev_get_services start\n");
	char** services = bt_rdev_get_services(r);
	LOGT("bt_rdev_get_services done\n");
	DEBUG_ASSERT(services);
	for(int i=0; services[i] != NULL; i++) {
		const char* u = services[i];
		LOG("%i: %s\n", i, u);
		DEBUG_ASSERT(u[0] == '0');
		DEBUG_ASSERT(u[1] == 'x');
		for(int j=2; j<6; j++) {
			DEBUG_ASSERT(isxdigit(u[j]));
		}
		DEBUG_ASSERT(u[6] == 0);
		uint32_t uuid16 = strtoul(u+2, NULL, 16);
		MAUUID uuid = {{ uuid16, 0x00001000, (int)0x80000080, 0x5F9B34FB }};

		BtService serv;
		serv.port = ???
		s.services.push_back(serv);
	}

	DEBIG_PHAT_ERROR;
}

int Bluetooth::maBtGetNewService(MABtServiceNative* dst) {
	CriticalSectionHandler csh(&gBt.critSec);
	BtServiceSearch& s(gBt.serviceSearch);
	if(s.nextService == s.services.size()) {
		return 0;
	}
	MYASSERT(dst->nameBufSize >= 0, BTERR_NEGATIVE_BUFFER_SIZE);
	const BtService& serv = s.services[s.nextService++];
	dst->port = serv.port;
	int servNameLen = MIN(dst->nameBufSize - 1, (int)serv.name.size());
	TCHARtoCHAR(dst->name, serv.name.c_str(), servNameLen);
	dst->name[servNameLen] = 0;
	for(size_t i=0; i<serv.uuids.size(); i++) {
		dst->uuids[i] = serv.uuids[i];
	}
	return 1;
}

int Bluetooth::maBtGetNextServiceSize(MABtServiceSize* dst) {
	CriticalSectionHandler csh(&gBt.critSec);
	BtServiceSearch& s(gBt.serviceSearch);
	if(s.nextService == s.services.size()) {
		return 0;
	}
	const BtService& serv = s.services[s.nextService];
	dst->nameBufSize = serv.name.size() + 1;
	dst->nUuids = serv.uuids.size();
	return 1;
}
#endif
