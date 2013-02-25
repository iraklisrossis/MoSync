#include <errno.h>

#define LOG_ERRNO_(e) LOG("Errno at %s:%i: %i(%s)\n", __FILE__, __LINE__, e, strerror(e))
#define LOG_ERRNO LOG_ERRNO_(errno)

#define DO_ERRNO_(e) do { LOG_ERRNO_(e); MoSyncErrorExit(e); } while(0)
#define DO_ERRNO DO_ERRNO_(errno)

#define ERRNO(func) do { int _res = (func); if(_res < 0) DO_ERRNO; } while(0)

#define ERRNO_RET(func) do { int _res = (func); if(_res != EOK) DO_ERRNO_(_res); } while(0)

#define NULL_ERRNO(func) do { void* _res = (func); if(_res < NULL) DO_ERRNO; } while(0)

#define IMGERR(func) do { int _res = (func); if(_res != IMG_ERR_OK) {\
	LOG("IMGERR at %s:%i: %i\n", __FILE__, __LINE__, _res);\
	MoSyncErrorExit(_res); } } while(0)

#define DUAL_ERRNO(fail, success, func) do { int _res = (func);\
	if(_res == fail) DO_ERRNO; else if(_res != success) {DEBIG_PHAT_ERROR;} } while(0)

#define BPSERR(func) DUAL_ERRNO(BPS_FAILURE,  BPS_SUCCESS, func)

#define FTERR(func) do { int _res = (func); if(_res != 0) {DEBIG_PHAT_ERROR;} } while(0)

void logMmrError();

#define MMR(func) do { int _res = (func); if(_res < 0) {logMmrError(); DEBIG_PHAT_ERROR;} } while(0)
