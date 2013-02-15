#include "config_platform.h"

#include <helpers/helpers.h>
#include <helpers/attribute.h>
#include <helpers/log.h>
#include "../Syscall.h"
#include "../FileStream.h"
#include <stdlib.h>

// warn: not gonna work in 64-bit systems.
int Base::Syscall::TranslateNativePointerToMoSyncPointer(void* n) {
	DEBUG_ASSERT(sizeof(void*) == sizeof(int));
	return (int)(size_t)n;
}
void* Base::Syscall::GetCustomEventPointer() {
	static char buffer[1024];
	return buffer;
}

void* Base::Syscall::GetValidatedMemRange(int address, int size) {
	return (void*)address;
}
void Base::Syscall::ValidateMemRange(const void* ptr, int size) {
}
int Base::Syscall::ValidatedStrLen(const char* ptr) {
	return strlen(ptr);
}
int Base::Syscall::GetValidatedStackValue(int offset VSV_ARGPTR_DECL) {
	if(offset < INT_MAX)	//hack
		BIG_PHAT_ERROR(ERR_FUNCTION_UNSUPPORTED);
	return 0;
}
const char* Base::Syscall::GetValidatedStr(int address) {
	return (const char*)address;
}

void Base::Syscall::VM_Yield() {
}

extern "C" void GCCATTRIB(noreturn) maLoadProgram(MAHandle data, int reload) {
	BIG_PHAT_ERROR(ERR_FUNCTION_UNSUPPORTED);
}

void MoSyncError::addRuntimeSpecificPanicInfo(char* ptr, bool newLines) {
}

#ifdef MEMORY_PROTECTION
void Base::Syscall::protectMemory(int start, int length) {
}

void Base::Syscall::unprotectMemory(int start, int length) {
}

void Base::Syscall::setMemoryProtection(int enable) {
}

int Base::Syscall::getMemoryProtection() {
	return 0;
}
#endif
