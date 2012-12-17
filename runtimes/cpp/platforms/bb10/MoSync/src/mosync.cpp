
#include "config_platform.h"

#include <core/Core.h>
#include <base/Syscall.h>
#include <string.h>
#include <stdio.h>
#include <helpers/log.h>

using namespace std;

static MAHandle gReloadHandle = 0;
bool gRunning = false;

int main() {
	InitLog("logs/log.txt");
	LOG("Log initialized.\n");

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
