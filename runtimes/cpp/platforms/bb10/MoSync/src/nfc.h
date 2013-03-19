#include "helpers/cpp_defs.h"
#include <bps/event.h>

int maNFCStart(void);
void maNFCStop(void);
MAHandle maNFCReadTag(MAHandle nfcContext);
void maNFCDestroyTag(MAHandle tagHandle);
void maNFCConnectTag(MAHandle tagHandle);
void maNFCCloseTag(MAHandle tagHandle);
int maNFCIsType(MAHandle tagHandle, int type);
MAHandle maNFCGetTypedTag(MAHandle tagHandle, int type);
int maNFCBatchStart(MAHandle tagHandle);
void maNFCBatchCommit(MAHandle tagHandle);
void maNFCBatchRollback(MAHandle tagHandle);
int maNFCTransceive(MAHandle tag, const void* src, int srcLen, void* dst, int* dstLen);
int maNFCSetReadOnly(MAHandle tag);
int maNFCIsReadOnly(MAHandle tag);
int maNFCGetSize(MAHandle tag);
int maNFCGetId(MAHandle tag, void* dst, int len);

int maNFCReadNDEFMessage(MAHandle tag);
int maNFCWriteNDEFMessage(MAHandle tag, MAHandle ndefMessage);
MAHandle maNFCCreateNDEFMessage(int recordCount);
int maNFCGetNDEFMessage(MAHandle tag);
MAHandle maNFCGetNDEFRecord(MAHandle ndefMessage, int ix);
int maNFCGetNDEFRecordCount(MAHandle ndefMessage);
int maNFCGetNDEFId(MAHandle ndefRecord, void* dst, int len);
int maNFCGetNDEFPayload(MAHandle ndefRecord, void* dst, int len);
int maNFCGetNDEFTnf(MAHandle ndefRecord);
int maNFCGetNDEFType(MAHandle ndefRecord, void* dst, int len);

void handleNfcEvent(bps_event_t*, int event_id);
extern "C" int nfc_get_domain();
