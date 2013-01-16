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
#include "helpers/fifo.h"

// libc includes
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

// bb10 includes
#include <screen/screen.h>
#include <input/event_types.h>
#include <input/screen_helpers.h>
#include <bps/button.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/vibration.h>
#include <img/img.h>
#include <ft2build.h>
#include <freetype/freetype.h>

// using
using namespace Base;

// macros
#define LOG_ERRNO LOG("Errno at %s:%i: %i(%s)\n", __FILE__, __LINE__, errno, strerror(errno))

#define DO_ERRNO do { LOG_ERRNO; MoSyncErrorExit(errno); } while(0)

#define ERRNO(func) do { int _res = (func); if(_res < 0) DO_ERRNO; } while(0)

#define IMGERR(func) do { int _res = (func); if(_res != IMG_ERR_OK) {\
	LOG("IMGERR at %s:%i: %i\n", __FILE__, __LINE__, _res);\
	MoSyncErrorExit(_res); } } while(0)

#define BPSERR(func) do { int _res = (func);\
	if(_res == BPS_FAILURE) DO_ERRNO; else if(_res != BPS_SUCCESS) {DEBIG_PHAT_ERROR;} } while(0)

#define FTERR(func) do { int _res = (func); if(_res != 0) {DEBIG_PHAT_ERROR;} } while(0)


// static variables
static screen_context_t sScreen;
static screen_window_t sWindow;
static Image* sCurrentDrawSurface = NULL;
static MAHandle sCurrentDrawHandle = HANDLE_SCREEN;
static Image* sBackBuffer = NULL;
static int sCurrentColor = 0;
static screen_buffer_t sScreenBuffer;
static int sScreenRect[4] = { 0,0 };
static int sScreenFormat;

static size_t sCodecCount = 0;
static img_codec_t* sCodecs = NULL;

static FT_Library sFtLib;
static FT_Face sFontFace;
static bool sFontLoaded = false;

static CircularFifo<MAEventNative, EVENT_BUFFER_SIZE> sEventFifo;

// operators

#if 0
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
#endif

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
		ERRNO(screen_get_window_property_iv(sWindow, SCREEN_PROPERTY_FORMAT, &sScreenFormat));
		LOG("Screen format: %i\n", sScreenFormat);

		Image::PixelFormat format;
		switch(sScreenFormat) {
		case SCREEN_FORMAT_RGBX5551: format = Image::PIXELFORMAT_RGB555; break;
		case SCREEN_FORMAT_RGBX4444: format = Image::PIXELFORMAT_RGB444; break;
		case SCREEN_FORMAT_RGB565: format = Image::PIXELFORMAT_RGB565; break;
		case SCREEN_FORMAT_RGB888: format = Image::PIXELFORMAT_RGB888; break;
		case SCREEN_FORMAT_RGBX8888: format = Image::PIXELFORMAT_ARGB8888; break;
		case SCREEN_FORMAT_RGBA8888: format = Image::PIXELFORMAT_ARGB8888; break;
		default:
			LOG("Unsupported screen format %i\n", sScreenFormat);
			DEBIG_PHAT_ERROR;
		}

		// obtain stride
		ERRNO(screen_get_buffer_property_iv(sScreenBuffer, SCREEN_PROPERTY_STRIDE, &param));
		LOG("Screen stride: %i\n", param);

		// obtain framebuffer pointer
		byte* framebuffer;
		ERRNO(screen_get_buffer_property_pv(sScreenBuffer, SCREEN_PROPERTY_POINTER, (void **)&framebuffer));

		// initialize Image system
		initMulTable();
		initRecipLut();

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
		BPSERR(navigator_request_events(0));
		BPSERR(button_request_events(0));
		BPSERR(screen_request_events(sScreen));

		// initialize image decoder
		{
			img_lib_t ilib = NULL;
			IMGERR(img_lib_attach(&ilib));
			sCodecCount = img_codec_list(ilib, NULL, 0, NULL, 0);
			LOG("%i codecs.\n", sCodecCount);
			sCodecs = new img_codec_t[sCodecCount];
			img_codec_list(ilib, sCodecs, sCodecCount, NULL, 0);
			for(size_t i=0; i<sCodecCount; i++) {
				const char* mime;
				const char* ext;
				img_codec_get_criteria(sCodecs[i], &ext, &mime);
				LOG("codec %d: ext = %s: mime = %s\n", i, ext, mime);
			}
		}
	}

	Image* Syscall::loadImage(MemStream& s)
	{
		LOG("loadImage...\n");
		io_stream_t* io;
		int length;
		DEBUG_ASSERT(s.length(length));
		DUMPINT(length);
		io = io_open(IO_MEM, IO_READ, length, s.ptrc());
		if(!io)
			DO_ERRNO;

		unsigned index;
		IMGERR(img_decode_validate(sCodecs, sCodecCount, io, &index));
		DUMPINT(index);

		uintptr_t decode_data;
		IMGERR(img_decode_begin(sCodecs[index], io, &decode_data));

		img_t img;
		img.flags = IMG_FORMAT;
		switch(sBackBuffer->pixelFormat) {
			case Image::PIXELFORMAT_RGB555: img.format = IMG_FMT_PKHE_ARGB1555; break;
			case Image::PIXELFORMAT_RGB444: DEBIG_PHAT_ERROR; break;
			case Image::PIXELFORMAT_RGB565: img.format = IMG_FMT_PKHE_RGB565; break;
			case Image::PIXELFORMAT_RGB888: img.format = IMG_FMT_RGB888; break;
			case Image::PIXELFORMAT_ARGB8888: img.format = IMG_FMT_PKHE_ARGB8888; break;
			default: DEBIG_PHAT_ERROR;
		}
		IMGERR(img_decode_frame(sCodecs[index], io, NULL, &img, &decode_data));

		IMGERR(img_decode_finish(sCodecs[index], io, &decode_data));
		io_close(io);

		static const int flags = IMG_DIRECT | IMG_FORMAT | IMG_W | IMG_H;
		if((img.flags & flags) != flags) {
			LOG("Bad flags: %X (expected %X)\n", img.flags, flags);
			return NULL;
		}

		LOG("loadImage complete!\n");

		return new Image(img.access.direct.data, NULL, img.w, img.h,
			img.access.direct.stride, sBackBuffer->pixelFormat, false, false);
	}

	Image* Syscall::loadSprite(void* surface, ushort left, ushort top, ushort width, ushort height, ushort cx, ushort cy)
	{
		return 0;
	}

	void Syscall::platformDestruct()
	{
	}
}

static void sigalrmHandler(int code) {
	LOG("sigalrmHandler(%i)\n", code);
	MoSyncErrorExit(code);
}

// event handler.
// handles all events, then returns immediately, if timeout == 0.
static void bpsWait(int timeout) {
	bps_event_t* event_bps = NULL;
	//LOG("bps_get_event(%i)...\n", timeout);
	BPSERR(bps_get_event(&event_bps, timeout));
	if(event_bps == NULL) {
		//LOG("bps_get_event timeout\n");
		return;
	}

	do {
		int event_domain = bps_event_get_domain(event_bps);
		int event_id = bps_event_get_code(event_bps);
		//LOG("Event domain %i id %i\n", event_domain, event_id);

		MAEventNative event;
		event.type = 0;
		if(event_domain == navigator_get_domain()) {
			switch(event_id) {
			case NAVIGATOR_EXIT:	// user has requested we exit the application
				event.type = EVENT_TYPE_CLOSE;
				ERRNO(alarm(EVENT_CLOSE_TIMEOUT / 1000));
				LOG("%i second alarm set.\n", EVENT_CLOSE_TIMEOUT / 1000);
				if(SIG_ERR == std::signal(SIGALRM, sigalrmHandler)) {
					LOG("sigalrmHandler could not be set. %s\n", strerror(errno));
				}
				break;
			case NAVIGATOR_WINDOW_ACTIVE:
				event.type = EVENT_TYPE_FOCUS_GAINED;
				break;
			case NAVIGATOR_WINDOW_INACTIVE:
				event.type = EVENT_TYPE_FOCUS_LOST;
				break;
			}
		} else if(event_domain == screen_get_domain()) {
			screen_event_t screen_event = screen_event_get_event(event_bps);
			mtouch_event_t mtouch_event;
			int screen_val;
			ERRNO(screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val));
			//LOG("screen event %i\n", screen_val);
			switch(screen_val) {
			case SCREEN_EVENT_MTOUCH_TOUCH: event.type = EVENT_TYPE_POINTER_PRESSED; break;
			case SCREEN_EVENT_MTOUCH_MOVE: event.type = EVENT_TYPE_POINTER_DRAGGED; break;
			case SCREEN_EVENT_MTOUCH_RELEASE: event.type = EVENT_TYPE_POINTER_RELEASED; break;
			case SCREEN_EVENT_POINTER:
				{
					static int oldPressed = -1;
					int buttons;
					int pair[2];
					ERRNO(screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS, &buttons));
					ERRNO(screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_SOURCE_POSITION, pair));

					bool pressed = ((buttons & SCREEN_LEFT_MOUSE_BUTTON) != 0);
					if(oldPressed >= 0) {
						if(pressed && !oldPressed) {
							event.type = EVENT_TYPE_POINTER_PRESSED;
						} else if(!pressed && oldPressed) {
							event.type = EVENT_TYPE_POINTER_RELEASED;
						} else if(pressed) {
							event.type = EVENT_TYPE_POINTER_DRAGGED;
						}
					}
					oldPressed = pressed;

					if(event.type) {
						event.touchId = 0;
						event.point.x = pair[0];
						event.point.y = pair[1];
						//LOG("pointer %i %i\n", event.point.x, event.point.y);
					}
				}
				break;
			}
			if(event.type && screen_val != SCREEN_EVENT_POINTER) {
				ERRNO(screen_get_mtouch_event(screen_event, &mtouch_event, 0));
				event.touchId = mtouch_event.contact_id;
				event.point.x = mtouch_event.x;
				event.point.y = mtouch_event.y;
				LOG("touch %i %i %i\n", event.touchId, event.point.x, event.point.y);
			}
		} else if(event_domain == button_get_domain()) {
			LOG("Button event %i %i\n", event_id, button_event_get_button(event_bps));
			switch(event_id) {
			case BUTTON_UP:
			case BUTTON_DOWN:
				event.type = (event_id == BUTTON_UP) ? EVENT_TYPE_KEY_RELEASED : EVENT_TYPE_KEY_PRESSED;
				int res = button_event_get_button(event_bps);
				if(res == BPS_FAILURE)
					DO_ERRNO;
				event.nativeKey = res;
				switch(res) {
					case BUTTON_POWER: event.key = MAK_ESCAPE; break;
					case BUTTON_PLAYPAUSE: event.key = MAK_FIRE; break;
					case BUTTON_PLUS: event.key = MAK_PLUS; break;
					case BUTTON_MINUS: event.key = MAK_MINUS; break;
					default: event.key = MAK_UNKNOWN;
				}
				break;
			}
		}
		if(event.type != 0)
			sEventFifo.put(event);

		// fetch another event, if the queue isn't empty.
		BPSERR(bps_get_event(&event_bps, 0));
	} while(event_bps != NULL);
}

SYSCALL(int, maGetKeys())
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maSetClipRect(int left, int top, int width, int height))
{
	sCurrentDrawSurface->clipRect.x = left;
	sCurrentDrawSurface->clipRect.y = top;
	sCurrentDrawSurface->clipRect.width = width;
	sCurrentDrawSurface->clipRect.height = height;
}

SYSCALL(void, maGetClipRect(MARect* rect))
{
	gSyscall->ValidateMemRange(rect, sizeof(MARect));
	rect->left = sCurrentDrawSurface->clipRect.x;
	rect->top = sCurrentDrawSurface->clipRect.y;
	rect->width = sCurrentDrawSurface->clipRect.width;
	rect->height = sCurrentDrawSurface->clipRect.height;
}

SYSCALL(int, maSetColor(int argb)) {
	int oldColor = sCurrentColor;
	sCurrentColor = argb | 0xFF000000;
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

static void loadFont() {
	if(sFontLoaded)
		return;
	//LOG("loading font...\n");
	FTERR(FT_Init_FreeType(&sFtLib));
	FTERR(FT_New_Face(sFtLib, "/usr/fonts/font_repository/monotype/arial.ttf", 0, &sFontFace));

	static const int point_size = 12;
	static const int dpi = 170;
	FTERR(FT_Set_Char_Size(sFontFace, point_size * 64, point_size * 64, dpi, dpi));

	sFontLoaded = true;
	//LOG("font loaded.\n");
}

template<class Tchar>
MAExtent maGetTextSizeT(const Tchar* str)
{
	loadFont();
	FT_GlyphSlot slot = sFontFace->glyph;
	const Tchar* ptr = str;
	uint w = 0;
	while(*ptr) {
		// todo: find a faster way to get width.
		FTERR(FT_Load_Char(sFontFace, *ptr, FT_LOAD_RENDER | FT_LOAD_MONOCHROME));
		DEBUG_ASSERT(slot->format == FT_GLYPH_FORMAT_BITMAP);
		const FT_Bitmap& b(slot->bitmap);
		DEBUG_ASSERT(b.pixel_mode == FT_PIXEL_MODE_MONO);

		w += slot->advance.x / 64;
		ptr++;
	}
	//LOG("maGetTextSizeT %i %li\n", w, sFontFace->size->metrics.height / 64);
	return EXTENT(w, sFontFace->height / 64);
}

SYSCALL(MAExtent, maGetTextSize(const char* str))
{
	return maGetTextSizeT<char>(str);
}

SYSCALL(MAExtent, maGetTextSizeW(const wchar* str))
{
	return maGetTextSizeT<wchar>(str);
}

template<class Tchar>
void maDrawTextT(int left, int top, const Tchar* str)
{
	loadFont();
	FT_GlyphSlot slot = sFontFace->glyph;
	const Tchar* ptr = str;

	// probably not perfect, but it will work.
	top += (sFontFace->bbox.yMax + sFontFace->bbox.yMin) >> 6;

	while(*ptr) {
		//LOG("FT_Load_Char(%c)\n", *ptr);
		FTERR(FT_Load_Char(sFontFace, *ptr, FT_LOAD_RENDER | FT_LOAD_MONOCHROME));
		DEBUG_ASSERT(slot->format == FT_GLYPH_FORMAT_BITMAP);
		const FT_Bitmap& b(slot->bitmap);
		DEBUG_ASSERT(b.pixel_mode == FT_PIXEL_MODE_MONO);

#if 0
		LOG("drawing bitmap of %c size %i %i at %i %i\n", *ptr, b.width, b.rows, left, top);
#if 0
		for(int i=0; i<b.rows; i++) {
			for(int j=0; j<b.pitch; j++) {
				LOG("%02X", b.buffer[i*b.pitch + j]);
			}
			LOG("\n");
		}
#endif
#endif
		sCurrentDrawSurface->drawBitmap(left + slot->bitmap_left, top - slot->bitmap_top,
			b.buffer, b.width, b.rows, b.pitch, sCurrentColor);

		left += slot->advance.x / 64;
		ptr++;
	}
}

SYSCALL(void, maDrawText(int left, int top, const char* str))
{
	//LOG("maDrawText(%s)\n", str);
	maDrawTextT<char>(left, top, str);
}

SYSCALL(void, maDrawTextW(int left, int top, const wchar* str))
{
#if 0
	wchar_t buf[2048];
	const wchar* src = str;
	wchar_t* dst = buf;
	while(*src) {
		*dst++ = *src++;
	}
	*dst = 0;
	LOG("maDrawTextW(%S)\n", buf);
#endif
	maDrawTextT<wchar>(left, top, str);
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
	return EXTENT(sBackBuffer->width, sBackBuffer->height);
}

SYSCALL(void, maDrawImage(MAHandle image, int left, int top))
{
	Image* img = gSyscall->resources.get_RT_IMAGE(image);
	sCurrentDrawSurface->drawImage(left, top, img);
}

SYSCALL(void, maDrawRGB(const MAPoint2d* dstPoint, const void* src,
	const MARect* srcRect, int scanlength))
{
	Image srcImg((byte*)src, NULL, scanlength, srcRect->top + srcRect->height, scanlength*4, Image::PIXELFORMAT_ARGB8888, false, false);
	ClipRect cr = {srcRect->left, srcRect->top, srcRect->width, srcRect->height};
	sCurrentDrawSurface->drawImageRegion(dstPoint->x, dstPoint->y, &cr, &srcImg, TRANS_NONE);
}

SYSCALL(void, maDrawImageRegion(MAHandle image, const MARect* src, const MAPoint2d* dstTopLeft, int transformMode))
{
	gSyscall->ValidateMemRange(dstTopLeft, sizeof(MAPoint2d));
#if 0
	maDrawImage(image, dstTopLeft->x, dstTopLeft->y);
#else
	gSyscall->ValidateMemRange(src, sizeof(MARect));
	Image* img = gSyscall->resources.get_RT_IMAGE(image);
	ClipRect srcRect = {src->left, src->top, src->width, src->height};
	sCurrentDrawSurface->drawImageRegion(dstTopLeft->x, dstTopLeft->y, &srcRect, img, transformMode);
#endif
}

SYSCALL(MAExtent, maGetImageSize(MAHandle image))
{
	Image* img = gSyscall->resources.get_RT_IMAGE(image);
	return EXTENT(img->width, img->height);
}

SYSCALL(void, maGetImageData(MAHandle image, void* dst, const MARect* src, int scanlength))
{
	gSyscall->ValidateMemRange(src, sizeof(MARect));
	gSyscall->ValidateMemRange(dst, src->height*scanlength);
	Image* img = gSyscall->resources.get_RT_IMAGE(image);

	int x = src->left;
	int y = src->top;
	int width = src->width;
	int height = src->height;

	if(width < 0 || height < 0 || x < 0 || y < 0) { DEBIG_PHAT_ERROR; }
	if(x > img->width) { DEBIG_PHAT_ERROR; }
	if(y > img->height) { DEBIG_PHAT_ERROR; }
	if(x + width > img->width) { DEBIG_PHAT_ERROR; }
	if(y+height > img->height) { DEBIG_PHAT_ERROR; }
	if(scanlength < 0) { DEBIG_PHAT_ERROR; }

	if(width > scanlength)
		width = scanlength;

#if 0	// doesn't copy alpha
	Image dstImg((byte*)dst, NULL, scanlength, height, scanlength*4, Image::PIXELFORMAT_ARGB8888, false, false);
	ClipRect srcRect = {src->left, src->top, src->width, src->height};
	dstImg.drawImageRegion(0, 0, &srcRect, img, TRANS_NONE);
#else
	for(int i=0; i<height; i++) {
		DEBUG_ASSERT(img->pixelFormat == Image::PIXELFORMAT_ARGB8888);
		memcpy((byte*)dst + i*scanlength*4, img->data + (i+y) * img->pitch + x*4, width*4);
	}
#endif
}

SYSCALL(MAHandle, maSetDrawTarget(MAHandle handle))
{
	MAHandle old = sCurrentDrawHandle;
	if(sCurrentDrawHandle != HANDLE_SCREEN) {
		// restore image
		SYSCALL_THIS->resources.extract_RT_FLUX(sCurrentDrawHandle);
		ROOM(SYSCALL_THIS->resources.add_RT_IMAGE(sCurrentDrawHandle, sCurrentDrawSurface));
	}
	if(handle == HANDLE_SCREEN) {
		sCurrentDrawSurface = sBackBuffer;
	} else {
		// put image into flux
		Image* img = SYSCALL_THIS->resources.extract_RT_IMAGE(handle);
		sCurrentDrawSurface = img;
		ROOM(SYSCALL_THIS->resources.add_RT_FLUX(handle, NULL));
	}
	sCurrentDrawHandle = handle;
	return old;
}

SYSCALL(int, maCreateImageFromData(MAHandle placeholder, MAHandle src, int offset, int size))
{
	MYASSERT(size>0, ERR_DATA_OOB);
	Stream* stream = gSyscall->resources.get_RT_BINARY(src);
	Image* image = 0;

	if(!stream->ptrc()) {
		// is not a memstream, create a memstream and load it.
		MYASSERT(stream->seek(Seek::Start, offset), ERR_DATA_OOB);
		MemStream b(size);
		MYASSERT(stream->readFully(b), ERR_DATA_ACCESS_FAILED);
		image = gSyscall->loadImage(b);
	} else {
		image = gSyscall->loadImage(*(MemStream*)stream);
	}
	if(image == NULL)
		return RES_OUT_OF_MEMORY;

	return gSyscall->resources.add_RT_IMAGE(placeholder, image);
}

SYSCALL(int, maCreateImageRaw(MAHandle placeholder, const void* src, MAExtent size, int alpha))
{
	int width = EXTENT_X(size), height = EXTENT_Y(size);
	Image* image = new Image((byte*)src, NULL, width, height,
		width*4, Image::PIXELFORMAT_ARGB8888, true);
	if(image == NULL)
		return RES_OUT_OF_MEMORY;
	return SYSCALL_THIS->resources.add_RT_IMAGE(placeholder, image);
}

SYSCALL(int, maCreateDrawableImage(MAHandle placeholder, int width, int height))
{
	Image::PixelFormat f = sBackBuffer->pixelFormat;
	MYASSERT(width > 0 && height > 0, ERR_IMAGE_SIZE_INVALID);
	Image *i = new Image(width, height, width * sBackBuffer->bytesPerPixel, f);
	if(i==NULL) return RES_OUT_OF_MEMORY;
	if(!(i->hasData())) { delete i; return RES_OUT_OF_MEMORY; }

	return gSyscall->resources.add_RT_IMAGE(placeholder, i);
}

SYSCALL(int, maGetEvent(MAEvent* dst))
{
	bpsWait(0);
	if(sEventFifo.empty())
		return 0;
	memcpy(dst, &sEventFifo.get(), sizeof(MAEvent));
	return 1;
}

SYSCALL(void, maWait(int timeout))
{
	if(!sEventFifo.empty())
		return;
	if(timeout == 0)
		timeout = -1;
	bpsWait(timeout);
}

SYSCALL(longlong, maTime())
{
	return time(NULL);
}

SYSCALL(longlong, maLocalTime())
{
	time_t t = time(NULL);
	tm* lt = localtime(&t);
	return t + lt->tm_gmtoff;
}

SYSCALL(int, maGetMilliSecondCount())
{
	timespec t;
	ERRNO(clock_gettime(CLOCK_MONOTONIC, &t));
	int ms = t.tv_sec * 1000 + t.tv_nsec / 1000000;
	return ms;
}

SYSCALL(int, maFreeObjectMemory())
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maTotalObjectMemory())
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maVibrate(int duration))
{
	int res = vibration_request(VIBRATION_INTENSITY_MEDIUM, duration);
	if(res == BPS_SUCCESS)
		return 1;
	DEBUG_ASSERT(res == BPS_FAILURE);
	LOG_ERRNO;
	return 0;
}

SYSCALL(int, maSoundPlay(MAHandle sound_res, int offset, int size))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maSoundStop())
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maSoundIsPlaying())
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maSoundSetVolume(int vol))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maSoundGetVolume())
{
	DEBIG_PHAT_ERROR;
}

#if 0
SYSCALL(int, maFrameBufferGetInfo(MAFrameBufferInfo *info))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maFrameBufferInit(void *data))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maFrameBufferClose())
{
	DEBIG_PHAT_ERROR;
}
#endif

SYSCALL(MAHandle, maConnect(const char* url))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maConnClose(MAHandle conn))
{
	DEBIG_PHAT_ERROR;
}


SYSCALL(int, maConnGetAddr(MAHandle conn, MAConnAddr* addr))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maConnRead(MAHandle conn, void* dst, int size))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maConnWrite(MAHandle conn, const void* src, int size))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maConnReadToData(MAHandle conn, MAHandle data, int offset, int size))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maConnWriteFromData(MAHandle conn, MAHandle data, int offset, int size))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(MAHandle, maHttpCreate(const char* url, int method))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maHttpSetRequestHeader(MAHandle conn, const char* key, const char* value))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(int, maHttpGetResponseHeader(MAHandle conn, const char* key, char* buffer, int bufSize))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maHttpFinish(MAHandle conn))
{
	DEBIG_PHAT_ERROR;
}

SYSCALL(void, maPanic(int result, const char* message))
{
	LOG("maPanic(%i, %s)\n", result, message);
	exit(result);
}

SYSCALL(longlong, maExtensionFunctionInvoke(int, int, int, int, ...))
{
	DEBIG_PHAT_ERROR;
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
