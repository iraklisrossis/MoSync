#include <errno.h>

#define LOG_ERRNO LOG("Errno at %s:%i: %i(%s)\n", __FILE__, __LINE__, errno, strerror(errno))

#define DO_ERRNO do { LOG_ERRNO; MoSyncErrorExit(errno); } while(0)

#define ERRNO(func) do { int _res = (func); if(_res < 0) DO_ERRNO; } while(0)

#define NULL_ERRNO(func) do { void* _res = (func); if(_res < NULL) DO_ERRNO; } while(0)

#define IMGERR(func) do { int _res = (func); if(_res != IMG_ERR_OK) {\
	LOG("IMGERR at %s:%i: %i\n", __FILE__, __LINE__, _res);\
	MoSyncErrorExit(_res); } } while(0)

#define BPSERR(func) do { int _res = (func);\
	if(_res == BPS_FAILURE) DO_ERRNO; else if(_res != BPS_SUCCESS) {DEBIG_PHAT_ERROR;} } while(0)

#define FTERR(func) do { int _res = (func); if(_res != 0) {DEBIG_PHAT_ERROR;} } while(0)

void logMmrError();

#define MMR(func) do { int _res = (func); if(_res < 0) {logMmrError(); DEBIG_PHAT_ERROR;} } while(0)
