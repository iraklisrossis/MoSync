#include "config_platform.h"

#define INSIDE_MOSYNC_CORE
#include "Core.h"
#include "mainLoop.h"

#ifdef FAKE_CALL_STACK
#include "sld.h"
#endif

#include <base/Syscall.h>

static int gReloadHandle = 0;

struct ReloadException { ReloadException() {} };

void mainLoop(Base::Syscall* syscall, const char* programFile, const char* resourceFile) {
	Base::Stream* reloadedStream = NULL;
	while(1) {
		try {
			Core::Run2(gCore);

			if(gReloadHandle > 0) {
				LOG("gReloadHandle 0x%x\n", gReloadHandle);
#ifdef FAKE_CALL_STACK
				clearSLD();
#endif
#ifdef REPORT
				reportIp(0, "LoadProgram");
				report(REPORT_LOAD_PROGRAM);
#endif
#if 0
				if(reloadedStream != NULL) {
					LOG("Deleting old stream...\n");
					delete reloadedStream;
				}
#endif
				reloadedStream = syscall->resources.extract_RT_BINARY(gReloadHandle);
				delete gCore;
				syscall->close();
				LOG("Core deleted.\n");
				gCore = Core::CreateCore(*syscall);
				LOG("Core created.\n");
				bool res = Core::LoadVMApp(gCore, *reloadedStream);
				LOG("Core loaded.\n");
				// must leave the stream alive for resource loading to work.
				gReloadHandle = 0;
				if(!res) {
					BIG_PHAT_ERROR(ERR_PROGRAM_LOAD_FAILED);
				}	//if
			}	//if
		}	catch(ReloadException) {
			LOG("Caught ReloadException.\n");
			delete gCore;
			if(reloadedStream != NULL)
				delete reloadedStream;
			gCore = Core::CreateCore(*syscall);
			if(!Core::LoadVMApp(gCore, programFile, resourceFile)) {
				BIG_PHAT_ERROR(ERR_PROGRAM_LOAD_FAILED);
			}
		}
	}
}

namespace Base {
	bool gReload = false;

	void reloadProgram() {
	#ifdef REPORT
		report(REPORT_RELOAD);
	#endif
		gReload = false;
		LOG("Throwing ReloadException.\n");
		throw ReloadException();
	}
}

SYSCALL(void, maLoadProgram(MAHandle data, int reload)) {
	Base::gSyscall->VM_Yield();
	gReloadHandle = data;
	Base::gReload |= (reload != 0);
}
