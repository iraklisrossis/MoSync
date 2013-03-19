#include "config_platform.h"
#include "nfc.h"
#include "event.h"
#include "bb10err.h"
#include "helpers/hash_map.h"

#include <nfc/nfc.h>
#include <nfc/nfc_bps.h>
#include <nfc/nfc_ndef.h>
#include <nfc/nfc_types.h>

#include <queue>

using namespace std;

#define NFCERR(func) do { int _res = (func); if(_res != NFC_RESULT_SUCCESS) {\
	LOG("NFCERR at %s:%i: %i\n", __FILE__, __LINE__, _res);\
	MoSyncErrorExit(_res); } } while(0)

struct NfcTarget {
	nfc_target_t* t;
	bool read;	// if not read, they cannot be destroyed.
};

static hash_map<int, NfcTarget> sTargets;
static queue<int> sTargetQueue;
static int sNextHandle = 1;

// simplified std::map insert function.
template<class Map, class Key, class Value>
std::pair<typename Map::iterator, bool> insert(Map& map, const Key& k, const Value& v) {
	return map.insert(std::pair<Key, Value>(k,v));
}

int maNFCStart(void) {
	BPSERR(nfc_request_events());
	NFCERR(nfc_register_tag_readerwriter(TAG_TYPE_ALL));
	NFCERR(nfc_register_field_events());
	return 0;
}

void maNFCStop(void) {
	BPSERR(nfc_stop_events());
}

void handleNfcEvent(bps_event_t* e, int event_id) {
	nfc_event_t* nfcEvent;
	LOG("BPS-NFC event %i\n", event_id);
	BPSERR(nfc_get_nfc_event(e, &nfcEvent));
	switch(event_id) {
	case NFC_TAG_READWRITE_EVENT:
	{
		nfc_target_t* target;
		uint count;
		NFCERR(nfc_get_target(nfcEvent, &target));
		NFCERR(nfc_get_ndef_message_count(target, &count));
		LOG("NFC_TAG_READWRITE_EVENT: %u messages.\n", count);

		char name[256];
		size_t len;
		NFCERR(nfc_get_tag_name(target, name, sizeof(name), &len));
		LOG("name: %s\n", name);

		uchar_t id[256];
		NFCERR(nfc_get_tag_id(target, id, sizeof(id), &len));
		LOG("id: %i bytes:", len);
		for(size_t i=0; i<len; i++) {
			LOG(" %02x", id[i]);
		}
		LOG("\n");

#if 0
NFC_API nfc_result_t nfc_get_tag_variant(const nfc_target_t *target,
                                         tag_variant_type_t *variant);
NFC_API nfc_result_t nfc_tag_is_writable(const nfc_target_t *tag,
                                         bool *is_writable);

NFC_API nfc_result_t nfc_tag_is_virtual(const nfc_target_t *tag,
                                        bool *is_virtual);
NFC_API nfc_result_t nfc_is_tag_locked(nfc_target_t *tag, bool *locked);
#endif

		insert(sTargets, sNextHandle, NfcTarget{ target, false });
		sTargetQueue.push(sNextHandle);
		sNextHandle++;

		MAEventNative en;
		en.type = EVENT_TYPE_NFC_TAG_RECEIVED;
		putEvent(en);
		break;
	}
	}
}

static NfcTarget& getTarget(MAHandle h) {
	auto itr = sTargets.find(h);
	DEBUG_ASSERT(itr != sTargets.end());
	return itr->second;
}

MAHandle maNFCReadTag(MAHandle nfcContext) {
	if(sTargetQueue.empty())
		return 0;
	MAHandle h = sTargetQueue.front();
	NfcTarget& t(getTarget(h));
	t.read = true;
	sTargetQueue.pop();
	return h;
}

void maNFCDestroyTag(MAHandle tagHandle) {
	auto itr = sTargets.find(tagHandle);
	DEBUG_ASSERT(itr != sTargets.end());
	NfcTarget& t(itr->second);
	DEBUG_ASSERT(t.read);
	NFCERR(nfc_destroy_target(t.t));
	sTargets.erase(itr);
}

int maNFCIsType(MAHandle tagHandle, int type) {
	NfcTarget& t(getTarget(tagHandle));
	bool isSupported;
	nfc_tag_type_t nativeType;
	switch(type) {
		case MA_NFC_TAG_TYPE_NDEF: nativeType = TAG_TYPE_NDEF; break;
		case MA_NFC_TAG_TYPE_ISO_DEP: nativeType = TAG_TYPE_ISO_14443_4; break;
		case MA_NFC_TAG_TYPE_NFC_A:
		case MA_NFC_TAG_TYPE_NFC_B:
			nativeType = TAG_TYPE_ISO_14443_3;
			break;
		default: return MA_NFC_INVALID_TAG_TYPE;
	}
	NFCERR(nfc_tag_supports_tag_type(t.t, nativeType, &isSupported));
	return isSupported;
}

MAHandle maNFCGetTypedTag(MAHandle tagHandle, int type) {
	return tagHandle;
}

int maNFCGetSize(MAHandle tag) {
	return -1;
}

int maNFCGetId(MAHandle tag, void* dst, int len) {
	return -1;
}
