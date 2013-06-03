/* Copyright (C) 2009 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <mastring.h>
#include <MAUtil/util.h>
#include <MAUtil/Connection.h>
#include <conprint.h>
#include "common.h"
#include "MAHeaders.h"
#include "conn_common.h"

//HTTP: use all known headers. add in a bunch of random ones too.
//check for filtering

//all conn types:
//read x bytes
//compare

//write x bytes
//server will compare, reply success or failure

//torture test:
//recv and write different sizes (1-16, powers of 2 up to perhaps 2^13, random values in between).
//this should find any hidden requirements of buffer size.

//create maximum number of connections (sockets, bluetooth, http) individually and mixed.
//try to write/read from all of them at once.

#define BT_HOST "0080984474c8"	//TDK @ MS-FREDRIK
#define IP_HOST "modev.mine.nu" //"192.168.0.173"	//"localhost"	//

#define SOCKET_URL(port) ("socket://" IP_HOST ":" + integerToString(port)).c_str()
#define UDP_URL(port) ("datagram://" IP_HOST ":" + integerToString(port)).c_str()
#define HTTP_GET_URL(port) ("http://" IP_HOST ":" +integerToString(port)+ "/server_data.bin").c_str()
#define HTTP_POST_URL ("http://" IP_HOST ":5004/post")
#define HTTP_CANCEL_GET_URL ("http://" IP_HOST ":5006/cancel_get")
#define HTTP_CANCEL_POST_URL SOCKET_URL(SOCKET_CANCEL_PORT)
#define BT_URL(port) ("btspp://" BT_HOST ":" +integerToString(port)).c_str()

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

class SocketSizeCase : public TestCase, public ConnectionListener {
private:
	//1, 2, 3, 4,  6, 7, 8,  14, 15, 16, ...32,64,128,156,512,1024,2048,4096
	//sizes <= 0 should result in a panic. we'll test that elsewhere.
#define NSIZES (1 + 2 + 9*3)
	int mSizes[NSIZES];
	char mServerData[DATA_SIZE];

	int mCurrentSizeIndex;
	Connection mConn;
	char mReadBuffer[DATA_SIZE];
public:
	SocketSizeCase() : TestCase("socketSize"), mConn(this) {
		mSizes[0] = 1;
		mSizes[1] = 2048;
		mSizes[2] = 4096;
		for(int i=1; i<=9; i++) {
			mSizes[i*3+0] = (2 << i) - 2;
			mSizes[i*3+1] = (2 << i) - 1;
			mSizes[i*3+2] = (2 << i) - 0;
		}

		maReadData(SERVER_DATA, mServerData, 0, DATA_SIZE);
	}

	//TestCase
	void start() {
		printf("Socket size test\n");
		mCurrentSizeIndex = -1;
		int res = mConn.connect(SOCKET_URL(SOCKET_SIZE_PORT));
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
		}
	}
	void close() {
		mConn.close();
	}

	//ConnectionListener
	virtual void connectFinished(Connection* conn, int result) {
		printf("Connected %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		next();
	}
	virtual void connReadFinished(Connection* conn, int result) {
		printf("Read %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		if(memcmp(mReadBuffer, mServerData, mSizes[mCurrentSizeIndex]) != 0) {
			printf("Data diff!\n");
			fail();
			return;
		}
		next();
	}

	void next() {
		if(mCurrentSizeIndex < NSIZES) {
			mCurrentSizeIndex++;
			mConn.read(mReadBuffer, mSizes[mCurrentSizeIndex]);
		} else {
			succeed();
		}
	}

	void fail() {
		assert(name, false);
		suite->runNextCase();
	}

	void succeed() {
		assert(name, true);
		suite->runNextCase();
	}
};

struct PAIR {
	const char* key, *value;
};
static const PAIR sHeaders[] = {	//I suppose we should test all known header keys here
	{"foo", "bar"},
	{"w00t", "b4k4"},
	{"Cookie", "WAAAGH!"}
};
static const int snHeaders = sizeof(sHeaders) / sizeof(PAIR);

class SingleHttpPostCase : public TestCase, public HttpConnectionListener {
private:
	HttpConnection mHttp;
	char* mReadBuffer;
	char* mClientData;
	const int mMultiple;
public:
	SingleHttpPostCase(int multiple) : TestCase("singlePost"), mHttp(this),
		mReadBuffer(NULL), mClientData(NULL), mMultiple(multiple)
	{
	}

	void fail() {
		assert(name, false);
		suite->runNextCase();
	}

	//TestCase
	void start() {
		printf("Single HTTP test (%d)\n", mMultiple);
		int res = mHttp.create(HTTP_POST_URL, HTTP_POST);
		printf("res:%d\n", res);
		if(res <= 0) {
			printf("create %i\n", res);
			fail();
			return;
		}
		//allocate source data
		mClientData = new char[mMultiple * DATA_SIZE];
		for(int i=0; i<mMultiple; i++) {
			maReadData(CLIENT_DATA, mClientData + (i * DATA_SIZE), 0, DATA_SIZE);
		}

		//set a bunch of headers
		for(int i=0; i<snHeaders; i++) {
			mHttp.setRequestHeader(sHeaders[i].key, sHeaders[i].value);
		}

		char buffer[64];
		sprintf(buffer, "%i", DATA_SIZE * mMultiple);
		printf("cl:%d\n", DATA_SIZE*mMultiple);
		mHttp.setRequestHeader("Content-Length", buffer);
		//write some data
		mHttp.write(mClientData, DATA_SIZE * mMultiple);

	}
	void close() {
		mHttp.close();
		delete mClientData;
		mClientData = NULL;
		delete mReadBuffer;
		mReadBuffer = NULL;
	}

	//HttpConnectionListener
	virtual void connWriteFinished(Connection*, int result) {
		printf("Write %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		//our POST is complete. ask for the response.
		mHttp.finish();
	}
	virtual void httpFinished(HttpConnection*, int result) {
		printf("Finish %i\n", result);
		if(result < 200 || result >= 300) {
			fail();
			return;
		}
		//check the response headers
		for(int i=0; i<snHeaders; i++) {
			String str;
			result = mHttp.getResponseHeader(sHeaders[i].key, &str);
			if(result < 0) {
				printf("Header missing: %s\n", sHeaders[i].key);
				fail();
				return;
			}
			if(strcmp(str.c_str(), sHeaders[i].value) != 0) {
				printf("Header mismatch: %s:%s != %s\n", sHeaders[i].key, sHeaders[i].value, str.c_str());
				fail();
				return;
			}
		}

		mReadBuffer = new char[DATA_SIZE * mMultiple];

		//read the response body
		mHttp.read(mReadBuffer, DATA_SIZE * mMultiple);
	}
	virtual void connReadFinished(Connection*, int result) {
		printf("Read %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		assert(name, memcmp(mReadBuffer, mClientData, DATA_SIZE * mMultiple) == 0);
		suite->runNextCase();
	}
};

class SingleSocketCase : public TestCase, public ConnectionListener {
private:
	Connection mConn;
	char mReadBuffer[DATA_SIZE];
	char mClientData[DATA_SIZE];
	char mServerData[DATA_SIZE];
	bool mReadFinished;
public:
	SingleSocketCase() : TestCase("singleSocket"), mConn(this) {
		maReadData(CLIENT_DATA, mClientData, 0, DATA_SIZE);
		maReadData(SERVER_DATA, mServerData, 0, DATA_SIZE);
	}

	void fail() {
		assert(name, false);
		suite->runNextCase();
	}

	//TestCase
	void start() {
		printf("Single Socket test\n");
		memset(mReadBuffer, 0, DATA_SIZE);
		mReadFinished = false;
		int res = mConn.connect(SOCKET_URL(SINGLE_SOCKET_PORT));
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
		}
	}
	void close() {
		mConn.close();
	}

	virtual void connectFinished(Connection* conn, int result) {
		printf("Connected %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		MAConnAddr addr;
		int res = mConn.getAddr(&addr);
		if(res >= 0) {
			union {
				char c[4];
				int i;
			} u;
			u.i = addr.inet4.addr;
			printf("Remote: %i.%i.%i.%i:%i\n",
				u.c[0], u.c[1], u.c[2], u.c[3], addr.inet4.port);
		} else {
			printf("getAddr error: %i\n", res);
		}
		assert("TCP getAddr", res >= 0);
		conn->read(mReadBuffer, DATA_SIZE);
	}
	virtual void connReadFinished(Connection* conn, int result) {
		printf("Read %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		if(!mReadFinished) {
			if(memcmp(mReadBuffer, mServerData, DATA_SIZE) != 0) {
				printf("Data diff!\n");
				fail();
				return;
			}
			mReadFinished = true;
			conn->write(mClientData, DATA_SIZE);
		} else {	//reply to write
			//success?
			assert(name, mReadBuffer[0] == 1);
			suite->runNextCase();
		}
	}
	virtual void connWriteFinished(Connection* conn, int result) {
		printf("Wrote %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		conn->read(mReadBuffer, 1);
	}
};


class SingleHttpGetCase : public TestCase, public HttpConnectionListener {
private:
	HttpConnection mHttp;
	char mReadBuffer[DATA_SIZE];
	char mServerData[DATA_SIZE];
public:
	SingleHttpGetCase() : TestCase("singleHttp"), mHttp(this) {
		maReadData(SERVER_DATA, mServerData, 0, DATA_SIZE);
	}

	void fail() {
		assert(name, false);
		suite->runNextCase();
	}

	//TestCase
	void start() {
		printf("Single HTTP get test\n");
		memset(mReadBuffer, 0, DATA_SIZE);
		int res = mHttp.create(HTTP_GET_URL(HTTP_PORT), HTTP_GET);
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
			return;
		}
		mHttp.finish();
	}
	void close() {
		mHttp.close();
	}

	//HttpConnectionListener
	virtual void httpFinished(HttpConnection* http, int result) {
		printf("Finish %i\n", result);
		if(result < 200 || result >= 300) {
			fail();
			return;
		}
		http->read(mReadBuffer, DATA_SIZE);
	}
	virtual void connReadFinished(Connection* conn, int result) {
		printf("Read %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		assert(name, memcmp(mReadBuffer, mServerData, DATA_SIZE) == 0);
		suite->runNextCase();
	}
};

class BaseCancelCase : public TestCase, protected Connection, protected ConnectionListener, protected TimerListener {
public:
	BaseCancelCase(const char* name) : TestCase(name), Connection(this) {
	}

	void fail() {
		Environment::getEnvironment().removeTimer(this);
		assert(name, false);
		suite->runNextCase();
	}

	//TestCase
	void close() {
		Connection::close();
	}

	void runTimerEvent() {
		// call maConnClose directly, so we get CONNERR_CANCELED events.
		maConnClose(mConn);
	}
};

class BaseSocketCancelCase : public BaseCancelCase {
protected:
	char mBuffer[DATA_SIZE];
	String mUrl;

	virtual bool connectTestFinished() = 0;
	virtual void readTestFinished() = 0;
public:
	BaseSocketCancelCase(const char* name, const String& url) : BaseCancelCase(name), mUrl(url) {
	}

	//TestCase
	void start() {
		printf("%s test\n", name.c_str());
		int res = Connection::connect(mUrl.c_str());
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
		}
	}

	virtual void connectFinished(Connection* conn, int result) {
		printf("Connected %i\n", result);
		if(result <= 0) {
			fail();
			return;
		}
		if(connectTestFinished()) {
			return;
		}
		conn->recv(mBuffer, DATA_SIZE);
		Environment::getEnvironment().addTimer(this, 1000, 1);
	}
	virtual void connRecvFinished(Connection* conn, int result) {
		if(result <= 0)
			printf("Recv %i\n", result);
		if(result == CONNERR_CANCELED) {
			Environment::getEnvironment().removeConnListener(mConn);
			mConn = -1;

			readTestFinished();
			return;
		}
		if(result <= 0) {
			fail();
			return;
		}
		conn->recv(mBuffer, DATA_SIZE);
		Environment::getEnvironment().addTimer(this, 1000, 1);
	}
};

class SocketCancelCase : public BaseSocketCancelCase {
private:
	bool mReadFinished;
	int mWriteCount;
public:
	SocketCancelCase(const char* name = "socketCancel")
		: BaseSocketCancelCase(name, SOCKET_URL(SOCKET_CANCEL_PORT))
	{
		mReadFinished = false;
		mWriteCount = 0;
	}

	virtual bool connectTestFinished() {
		if(mReadFinished) {
			write();
			return true;
		}
		return false;
	}
	virtual void readTestFinished() {
		mReadFinished = true;
		int res = Connection::connect(mUrl.c_str());
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
		}
	}
	// keep writing until it no longer returns instantly.
	// addTimer() resets the timer.
	void write() {
		mWriteCount++;
		Connection::write(mBuffer, DATA_SIZE);
		Environment::getEnvironment().addTimer(this, 1000, 1);
	}
	virtual void connWriteFinished(Connection* conn, int result) {
		if(result <= 0) {
			printf("Wrote %i\n", result);
		}
		if(result == CONNERR_CANCELED) {
			printf("count: %i\n", mWriteCount);
			// remember to de-register the now-closed connection.
			Environment::getEnvironment().removeConnListener(mConn);
			mConn = -1;
			assert(name, true);
			suite->runNextCase();
			return;
		}
		if(result <= 0) {
			fail();
			return;
		}
		write();
	}
};


class UdpCancelCase : public BaseSocketCancelCase {
public:
	UdpCancelCase() : BaseSocketCancelCase("udpCancel", UDP_URL(CONNECT_CANCEL_PORT)) {
	}

	virtual bool connectTestFinished() {
		return false;
	}
	virtual void readTestFinished() {
		assert(name, true);
		suite->runNextCase();
	}
};


class ConnectCancelCase : public BaseCancelCase {
protected:
	const String mUrl;
public:
	ConnectCancelCase(const char* name, const String& url) : BaseCancelCase(name), mUrl(url) {
	}

	//TestCase
	void start() {
		printf("%s test\n", name.c_str());
		int res = Connection::connect(mUrl.c_str());
		if(res <= 0) {
			printf("connect %i\n", res);
			fail();
			return;
		}
		Environment::getEnvironment().addTimer(this, 500, 1);
	}
	void close() {
		Connection::close();
	}

	virtual void connectFinished(Connection* conn, int result) {
		printf("Connected %i\n", result);
		if(result == CONNERR_CANCELED) {
			Environment::getEnvironment().removeConnListener(mConn);
			mConn = -1;
			assert(name, true);
			suite->runNextCase();
			return;
		}
		// If maConnClose was called, but CONNERR_CANCELED did not happen,
		// another call to maConnClose (like from Connection::close) will
		// cause a panic.
		fail();
	}
	void runTimerEvent() {
		// call maConnClose directly, so we get CONNERR_CANCELED events.
		maConnClose(mConn);
	}
};

class HttpCancelCase : public SocketCancelCase {
public:
	HttpCancelCase() : SocketCancelCase("httpCancel") {
		mUrl = HTTP_CANCEL_GET_URL;
	}

	virtual void readTestFinished() {
		mUrl = HTTP_CANCEL_POST_URL;
		SocketCancelCase::readTestFinished();
	}
};

void addConnTests(TestSuite* suite);
void addConnTests(TestSuite* suite) {
#if 1
	suite->addTestCase(new SingleSocketCase);
	suite->addTestCase(new SingleHttpGetCase);
	suite->addTestCase(new SocketCancelCase);
	suite->addTestCase(new UdpCancelCase);
	suite->addTestCase(new ConnectCancelCase("connectCancel", SOCKET_URL(CONNECT_CANCEL_PORT)));
	// cancel at TCP-connect stage.
	suite->addTestCase(new ConnectCancelCase("httpConnectCancel", HTTP_GET_URL(CONNECT_CANCEL_PORT)));
	// cancel while reading 16:17 2013-05-29headers.
	suite->addTestCase(new ConnectCancelCase("httpHeadersCancel", HTTP_GET_URL(SOCKET_CANCEL_PORT)));
#endif
	// cancel while reading response data, and while writing POST data.
	suite->addTestCase(new HttpCancelCase);
#if 1
	for(int i=0; i<5; i++) {
		suite->addTestCase(new SingleHttpPostCase(1 << i));
	}
#endif
}
