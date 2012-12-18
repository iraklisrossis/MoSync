
#include "config_platform.h"

#include <core/Core.h>
#include <base/Syscall.h>
#include <string.h>
#include <stdio.h>
#include <helpers/log.h>
#include <signal.h>
#include <bps/bps.h>

using namespace std;

static MAHandle gReloadHandle = 0;
bool gRunning = false;

static void sigquit_handler(int) {
	LOG("SIGQUIT received. Application will now shut down.\n");
}

int main() {
	LogOverrideFile(stderr);
	bps_set_verbosity(2);
	LOG("Log initialized.\n");

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		act.sa_handler = sigquit_handler;
		sigaction(SIGQUIT, &act, NULL);
		LOG("SIGQUIT handler installed.\n");
	}

	Base::Syscall *syscall = 0;
	syscall = new Base::Syscall();

	gCore = Core::CreateCore(*syscall);
	MYASSERT(Core::LoadVMApp(gCore, "app/native/program", "app/native/resources"), ERR_PROGRAM_LOAD_FAILED);
	gRunning = true;

	while(1) {
		Core::Run2(gCore);

		if(gReloadHandle > 0) {
			Base::Stream* stream = Base::gSyscall->resources.extract_RT_BINARY(gReloadHandle);
			bool res = Core::LoadVMApp(gCore, *stream);
			delete stream;
			gReloadHandle = 0;
			if(!res) {
				BIG_PHAT_ERROR(ERR_PROGRAM_LOAD_FAILED);
			}
		}
	}

	LOG("main return 0\n");
	return 0;
}

SYSCALL(void, maLoadProgram(MAHandle data, int reload)) {
	Base::gSyscall->VM_Yield();
	gReloadHandle = data;
	//gReload |= (reload != 0);
}
