
#include "config_platform.h"

#if NATIVE_PROGRAM
extern "C" int MAMain();
extern "C" int resource_selector();
#else
#define INSIDE_MOSYNC_CORE
#include <core/Core.h>
#endif

#include <base/Syscall.h>
#include <string.h>
#include <stdio.h>
#include <helpers/log.h>
#include <signal.h>
#include <sys/resource.h>
#include <bps/bps.h>
#include "Cascade.h"
#include "bb10err.h"

using namespace std;

bool gRunning = false;

#if !NATIVE_PROGRAM
static MAHandle gReloadHandle = 0;
#endif

static int bpsThreadFunc(void*);

static void sigquit_handler(int) {
	LOG("SIGQUIT received. Application will now shut down.\n");
	abort();
}

int main(int argc, char** argv) {
	LogOverrideFile(stderr);
	LOG("Log initialized.\n");

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		act.sa_handler = sigquit_handler;
		sigaction(SIGQUIT, &act, NULL);
		LOG("SIGQUIT handler installed.\n");
	}

	{
		rlimit r;
		ERRNO(getrlimit(RLIMIT_DATA, &r));
		LOG("RLIMIT_DATA: cur %lu, max %lu\n", r.rlim_cur, r.rlim_max);
		ERRNO(getrlimit(RLIMIT_STACK, &r));
		LOG("RLIMIT_STACK: cur %lu, max %lu\n", r.rlim_cur, r.rlim_max);
		ERRNO(getrlimit(RLIMIT_AS, &r));
		LOG("RLIMIT_AS: cur %lu, max %lu\n", r.rlim_cur, r.rlim_max);
	}

	return runCascadeApp(argc, argv, bpsThreadFunc);
}

static int bpsThreadFunc(void*) {
	bps_initialize();
	bps_set_verbosity(2);

	Base::Syscall* syscall = new Base::Syscall();
	int res;

#if NATIVE_PROGRAM
	Base::FileStream rf("app/native/resources");
	MYASSERT(syscall->loadResources(rf, "app/native/resources"), ERR_PROGRAM_LOAD_FAILED);
	LOG("resources loaded.\n");
	res = resource_selector();
	LOG("resource_selector: %i\n", res);
	res = MAMain();
	LOG("MAMain: %i\n", res);
#else
	gCore = Core::CreateCore(*syscall);
	MYASSERT(Core::LoadVMApp(gCore, "app/native/program", "app/native/resources"), ERR_PROGRAM_LOAD_FAILED);
	gRunning = true;

	Base::Stream* reloadedStream = NULL;
	while(1) {
		Core::Run2(gCore);

		if(gReloadHandle > 0) {
			LOG("Reloading from handle 0x%x...\n", gReloadHandle);
			if(reloadedStream != NULL)
				delete reloadedStream;
			reloadedStream = Base::gSyscall->resources.extract_RT_BINARY(gReloadHandle);
			res = Core::LoadVMApp(gCore, *reloadedStream);
			gReloadHandle = 0;
			if(!res) {
				BIG_PHAT_ERROR(ERR_PROGRAM_LOAD_FAILED);
			}
			LOG("Reload complete.\n");
		}
	}
	res = 0;
#endif

	bps_shutdown();
	LOG("main return %i\n", res);
	return res;
}

#if !NATIVE_PROGRAM
SYSCALL(void, maLoadProgram(MAHandle data, int reload)) {
	Base::gSyscall->VM_Yield();
	gReloadHandle = data;
	//gReload |= (reload != 0);
}
#endif
