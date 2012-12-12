/*
 * SyscallImpl.cpp
 *
 *  Created on: Nov 23, 2012
 *      Author: andersmalm
 */
#include "config_platform.h"

#include "Syscall.h"
#include <math.h>

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

#if 0
SYSCALL(double, sin(double x))
{
	return ::sin(x);
}

SYSCALL(double, cos(double x))
{
	return ::cos(x);
}

SYSCALL(double, tan(double x))
{
	return ::tan(x);
}

SYSCALL(double, sqrt(double x))
{
	return ::sqrt(x);
}
#endif

SYSCALL(int, maGetKeys())
{
		return -1;
}

SYSCALL(void, maSetClipRect(int left, int top, int width, int height))
{
}

SYSCALL(void, maGetClipRect(MARect *rect))
{
}

SYSCALL(int, maSetColor(int argb))
{
	return -1;
}

SYSCALL(void, maPlot(int posX, int posY))
{
}

SYSCALL(void, maLine(int startX, int startY, int endX, int endY))
{
}

SYSCALL(void, maFillRect(int left, int top, int width, int height))
{
}

SYSCALL(void, maFillTriangleStrip(const MAPoint2d* points, int count))
{
}

SYSCALL(void, maFillTriangleFan(const MAPoint2d* points, int count))
{
}

SYSCALL(MAExtent, maGetTextSize(const char* str))
{
	return -1;
}

SYSCALL(MAExtent, maGetTextSizeW(const wchar* str))
{
	return -1;
}

SYSCALL(void, maDrawText(int left, int top, const char* str))
{
}

SYSCALL(void, maDrawTextW(int left, int top, const wchar* str))
{
}

SYSCALL(void, maUpdateScreen())
{
}

SYSCALL(void, maResetBacklight())
{
}

SYSCALL(MAExtent, maGetScrSize())
{
	return -1;
}

SYSCALL(void, maDrawImage(MAHandle image, int left, int top))
{
}

SYSCALL(void, maDrawRGB(const MAPoint2d* dstPoint, const void* src,
		const MARect* srcRect, int scanlength))
{
}

SYSCALL(void, maDrawImageRegion(MAHandle image, const MARect* src, const MAPoint2d* dstTopLeft, int transformMode))
{
}

SYSCALL(MAExtent, maGetImageSize(MAHandle image))
{
	return -1;
}

SYSCALL(void, maGetImageData(MAHandle image, void* dst, const MARect* src, int scanlength))
{
}

SYSCALL(MAHandle, maSetDrawTarget(MAHandle handle))
{
	return -1;
}

SYSCALL(int, maCreateImageFromData(MAHandle placeholder, MAHandle resource, int offset, int size))
{
	return -1;
}

SYSCALL(int, maCreateImageRaw(MAHandle placeholder, const void* src, MAExtent size, int alpha))
{
	return -1;
}

SYSCALL(int, maCreateDrawableImage(MAHandle placeholder, int width, int height))
{
	return -1;
}

SYSCALL(int, maGetEvent(MAEvent* dst))
{
	return -1;
}

SYSCALL(void, maWait(int timeout))
{
}

SYSCALL(longlong, maTime())
{
	return -1;
}

SYSCALL(longlong, maLocalTime())
{
	return -1;
}

SYSCALL(int, maGetMilliSecondCount())
{
	return -1;
}

SYSCALL(int, maFreeObjectMemory())
{
	return -1;
}

SYSCALL(int, maTotalObjectMemory())
{
	return -1;
}

SYSCALL(int, maVibrate(int))
{
	return -1;
}

SYSCALL(int, maSoundPlay(MAHandle sound_res, int offset, int size))
{
	return -1;
}

SYSCALL(void, maSoundStop())
{
}

SYSCALL(int, maSoundIsPlaying())
{
	return -1;
}

SYSCALL(void, maSoundSetVolume(int vol))
{
}

SYSCALL(int, maSoundGetVolume())
{
	return -1;
}

#if 0
SYSCALL(int, maFrameBufferGetInfo(MAFrameBufferInfo *info))
{
	return -1;
}

SYSCALL(int, maFrameBufferInit(void *data))
{
	return -1;
}

SYSCALL(int, maFrameBufferClose())
{
	return -1;
}
#endif
/*
SYSCALL(int, maAudioBufferInit(const MAAudioBufferInfo *ainfo))
{
	return -1;
}

SYSCALL(int, maAudioBufferReady())
{
	return -1;
}

SYSCALL(int, maAudioBufferClose())
{
	return -1;
}
*/
SYSCALL(MAHandle, maConnect(const char* url))
{
	return -1;
}

SYSCALL(void, maConnClose(MAHandle conn))
{
}


SYSCALL(int, maConnGetAddr(MAHandle conn, MAConnAddr* addr))
{
	return -1;
}

SYSCALL(void, maConnRead(MAHandle conn, void* dst, int size))
{
}

SYSCALL(void, maConnWrite(MAHandle conn, const void* src, int size))
{
}

SYSCALL(void, maConnReadToData(MAHandle conn, MAHandle data, int offset, int size))
{
}

SYSCALL(void, maConnWriteFromData(MAHandle conn, MAHandle data, int offset, int size))
{
}

SYSCALL(MAHandle, maHttpCreate(const char* url, int method))
{
	return -1;
}

SYSCALL(void, maHttpSetRequestHeader(MAHandle conn, const char* key, const char* value))
{
}

SYSCALL(int, maHttpGetResponseHeader(MAHandle conn, const char* key, char* buffer, int bufSize))
{
	return -1;
}

SYSCALL(void, maHttpFinish(MAHandle conn))
{
}

SYSCALL(void,  maLoadProgram(MAHandle data, int reload))
{

}

SYSCALL(void, maPanic(int result, const char* message))
{
	exit(result);
}

SYSCALL(longlong, maExtensionFunctionInvoke(int, int, int, int, ...))
{
	return -1;
}

SYSCALL(longlong, maIOCtl(int function, int a, int b, int c, ...))
{
	va_list argptr;
	va_start(argptr, c);

	return -1;
}

void MoSyncExit(int exitCode)
{
	exit(exitCode);
}

void MoSyncErrorExit(int errorCode)
{
	exit(errorCode);
}
