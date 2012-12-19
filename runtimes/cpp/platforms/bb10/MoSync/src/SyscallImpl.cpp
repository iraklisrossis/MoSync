/*
 * SyscallImpl.cpp
 *
 *  Created on: Nov 23, 2012
 *      Author: andersmalm
 */
#include "config_platform.h"

// local includes
#include "Syscall.h"
#include "Image.h"
#include "helpers/helpers.h"

// libc includes
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

// bb10 includes
#include <screen/screen.h>
#include <bps/event.h>
#include <bps/navigator.h>

// using
using namespace Base;

// macros
#define DO_ERRNO do { LOG("Errno at %s:%i: %i(%s)\n", __FILE__, __LINE__, errno, strerror(errno));\
	MoSyncErrorExit(errno); } while(0)

#define ERRNO(func) do { int _res = (func); if(_res < 0) DO_ERRNO; } while(0)

// static variables
static screen_context_t sScreen;
static screen_window_t sWindow;
static Image *sCurrentDrawSurface = NULL;
static Image *sBackBuffer = NULL;
static int sCurrentColor = 0;
static screen_buffer_t sScreenBuffer;
static int sScreenRect[4] = { 0,0 };

// operators

static timespec operator-(const timespec& a, const timespec& b) {
	static const int NANO = 1000000000;
	timespec t = { a.tv_sec - b.tv_sec, a.tv_nsec - b.tv_nsec };
	if(t.tv_nsec > NANO) {
		int sec = t.tv_nsec / NANO;
		int nsec = t.tv_nsec % NANO;
		t.tv_sec += sec;
		t.tv_nsec = nsec;
	} else if(t.tv_nsec < 0) {
		int sec = t.tv_nsec / NANO;
		int nsec = (t.tv_nsec % NANO) + NANO * (1-sec);
		t.tv_sec += sec;
		t.tv_nsec = nsec;
	}
	return t;
}


namespace Base
{
	Syscall* gSyscall = NULL;

	Syscall::Syscall() {
		LOG("Screen init...\n");
		assert(gSyscall == NULL);
		gSyscall = this;

		ERRNO(screen_create_context(&sScreen, SCREEN_APPLICATION_CONTEXT));
		ERRNO(screen_create_window(&sWindow, sScreen));

		// define the screen usage
		int param = SCREEN_USAGE_WRITE | SCREEN_USAGE_NATIVE;
		ERRNO(screen_set_window_property_iv(sWindow, SCREEN_PROPERTY_USAGE, &param));

		// create a the buffer required for rendering the window
		ERRNO(screen_create_window_buffers(sWindow, 1));

		// obtain the framebuffer context
		ERRNO(screen_get_window_property_pv(sWindow, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&sScreenBuffer));

		// optain window size
		ERRNO(screen_get_window_property_iv(sWindow, SCREEN_PROPERTY_BUFFER_SIZE, sScreenRect + 2));
		LOG("Screen size: %ix%i\n", sScreenRect[2], sScreenRect[3]);

		// obtain window format
		ERRNO(screen_get_window_property_iv(sWindow, SCREEN_PROPERTY_FORMAT, &param));
		LOG("Screen format: %i\n", param);

		Image::PixelFormat format;
		switch(param) {
		case SCREEN_FORMAT_RGBX5551: format = Image::PIXELFORMAT_RGB555; break;
		case SCREEN_FORMAT_RGBX4444: format = Image::PIXELFORMAT_RGB444; break;
		case SCREEN_FORMAT_RGB565: format = Image::PIXELFORMAT_RGB565; break;
		case SCREEN_FORMAT_RGB888: format = Image::PIXELFORMAT_RGB888; break;
		case SCREEN_FORMAT_RGBX8888: format = Image::PIXELFORMAT_ARGB8888; break;
		default:
			LOG("Unsupported screen format %i\n", param);
			DEBIG_PHAT_ERROR;
		}

		// obtain stride
		ERRNO(screen_get_buffer_property_iv(sScreenBuffer, SCREEN_PROPERTY_STRIDE, &param));
		LOG("Screen stride: %i\n", param);

		// obtain framebuffer pointer
		byte* framebuffer;
		ERRNO(screen_get_buffer_property_pv(sScreenBuffer, SCREEN_PROPERTY_POINTER, (void **)&framebuffer));

		// initialize internal backbuffer
		sBackBuffer = new Image(framebuffer, NULL, sScreenRect[2], sScreenRect[3], param, format, false, false);
		sCurrentDrawSurface = sBackBuffer;
		assert(sBackBuffer != NULL);

		// fill with black
		memset(framebuffer, 0, param * sScreenRect[3]);

		// request the window be displayed
		maUpdateScreen();
		LOG("Screen init complete.\n");

		// initialize events
		ERRNO(navigator_request_events(0));
	}

	Image* Syscall::loadImage(MemStream& s)
	{
		return 0;
	}

	Image* Syscall::loadSprite(void* surface, ushort left, ushort top, ushort width, ushort height, ushort cx, ushort cy)
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

SYSCALL(int, maSetColor(int argb)) {
	int oldColor = sCurrentColor;
	sCurrentColor = argb;
	return oldColor;
}

SYSCALL(void, maPlot(int posX, int posY)) {
	sCurrentDrawSurface->drawPoint(posX, posY, sCurrentColor);
}

SYSCALL(void, maLine(int x0, int y0, int x1, int y1)) {
	sCurrentDrawSurface->drawLine(x0, y0, x1, y1, sCurrentColor);
}

SYSCALL(void, maFillRect(int left, int top, int width, int height)) {
	sCurrentDrawSurface->drawFilledRect(left, top, width, height, sCurrentColor);
}

SYSCALL(void, maFillTriangleStrip(const MAPoint2d *points, int count)) {
	SYSCALL_THIS->ValidateMemRange(points, sizeof(MAPoint2d) * count);
	CHECK_INT_ALIGNMENT(points);
	MYASSERT(count >= 3, ERR_POLYGON_TOO_FEW_POINTS);
	for(int i = 2; i < count; i++) {
		sCurrentDrawSurface->drawTriangle(
			points[i-2].x,
			points[i-2].y,
			points[i-1].x,
			points[i-1].y,
			points[i].x,
			points[i].y,
			sCurrentColor);
	}
}

SYSCALL(void, maFillTriangleFan(const MAPoint2d *points, int count)) {
	SYSCALL_THIS->ValidateMemRange(points, sizeof(MAPoint2d) * count);
	CHECK_INT_ALIGNMENT(points);
	MYASSERT(count >= 3, ERR_POLYGON_TOO_FEW_POINTS);
	for(int i = 2; i < count; i++) {
		sCurrentDrawSurface->drawTriangle(
			points[0].x,
			points[0].y,
			points[i-1].x,
			points[i-1].y,
			points[i].x,
			points[i].y,
			sCurrentColor);
	}
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
	ERRNO(screen_post_window(sWindow, sScreenBuffer, 1, sScreenRect, 0));
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
	if(timeout == 0)
		timeout = -1;
	timespec endTime;
	if(timeout > 0) {
		ERRNO(clock_gettime(CLOCK_MONOTONIC, &endTime));
		int sec = timeout / 1000;
		int ms = timeout % 1000;
		endTime.tv_sec += sec;
		endTime.tv_nsec += ms * 1000000;
	}
	while(true) {
		bps_event_t* event_bps = NULL;
		LOG("bps_get_event(%i)...\n", timeout);
		{
			int res = bps_get_event(&event_bps, timeout);
			if(res == BPS_FAILURE)
				DO_ERRNO;
			else if(res != BPS_SUCCESS)
				DEBIG_PHAT_ERROR;
		}
		if(event_bps == NULL) {
			LOG("bps_get_event timeout\n");
			return;
		}

		// process the event accordingly
		int event_domain = bps_event_get_domain(event_bps);
		LOG("Event domain %i\n", event_domain);

		// did we receive a navigator event?
		if (event_domain == navigator_get_domain())
		{
			int event_id = bps_event_get_code(event_bps);
			switch(event_id) {
			case NAVIGATOR_EXIT:	// user has requested we exit the application
				LOG("NAVIGATOR_EXIT\n");
				MoSyncExit(0);
				break;
			}
		}

		if(timeout > 0) {
			timespec t;
			ERRNO(clock_gettime(CLOCK_MONOTONIC, &t));
			t = endTime - t;
			int ms = t.tv_sec * 1000 + t.tv_nsec / 1000000;
			if(ms > 0)
				timeout = ms;
			else
				timeout = 0;
		}
	}
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

SYSCALL(void, maPanic(int result, const char* message))
{
	LOG("maPanic(%i, %s)\n", result, message);
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
	LOG("MoSyncExit(%i)\n", exitCode);
	exit(exitCode);
}

void MoSyncErrorExit(int errorCode)
{
	LOG("MoSyncErrorExit(%i)\n", errorCode);
	exit(errorCode);
}
