/*
 * SyscallImpl.cpp
 *
 *  Created on: Nov 23, 2012
 *      Author: andersmalm
 */
#include "config_platform.h"

#include "Syscall.h"

namespace Base
{
	Syscall* gSyscall = NULL;

	void* Syscall::loadImage(MemStream& s)
	{
		return 0;
	}

	void* Syscall::loadSprite(void* surface, ushort left, ushort top, ushort width, ushort height, ushort cx, ushort cy)
	{
		return 0;
	}

	void Syscall::platformDestruct()
	{
	}


}

void MoSyncExit(int exitCode)
{

	exit(exitCode);
}

void MoSyncErrorExit(int errorCode)
{
	exit(errorCode);
}
