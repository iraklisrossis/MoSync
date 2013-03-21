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
#include "bb10err.h"
#include "helpers/CPP_IX_OPENGL_ES.h"
#include "helpers/CPP_IX_GL1.h"
#include "helpers/CPP_IX_GL2.h"
#include "bbutil.h"
#include "NativeUI.h"
#include "event.h"
#include "nfc.h"

#define NETWORKING_H
#include "networking.h"
#include "bluetooth/discovery.h"

// libc includes
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// bb10 includes
#include <screen/screen.h>
#include <input/event_types.h>
#include <input/screen_helpers.h>
#include <bps/battery.h>
#include <bps/button.h>
#include <bps/event.h>
#include <bps/geolocation.h>
#include <bps/locale.h>
#include <bps/navigator.h>
#include <bps/navigator_invoke.h>
#include <bps/screen.h>
#include <bps/sensor.h>
#include <bps/vibration.h>
#include <img/img.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <mm/renderer.h>

// OpenGL ES includes
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include "base/GLFixes.h"
#include "generated/gl.h.cpp"
#include <EGL/egl.h>

#define HAVE_SCREEN 1

// using
using namespace Base;

// macros

// enums
enum EventCode {
	EVENT_CODE_MA,
	EVENT_CODE_DEFLUX,
	EVENT_CODE_FUNCTOR,
};

// functions
static void bpsWait(int timeout);

// static variables
#if HAVE_SCREEN
static int sDisplayCount;
static screen_context_t sScreen;
static int sScreenFormat;
#endif
static screen_display_t* sDisplays;
static screen_window_t sWindow;
static Image* sCurrentDrawSurface = NULL;
static MAHandle sCurrentDrawHandle = HANDLE_SCREEN;
static Image* sBackBuffer = NULL;
static int sCurrentColor = 0;
static screen_buffer_t sScreenBuffer;
static int sScreenRect[4] = { 0,0 };
static bool sOpenGLActive = false;

static char sCwd[PATH_MAX];

static size_t sCodecCount = 0;
static img_codec_t* sCodecs = NULL;

static FT_Library sFtLib;
static FT_Face sFontFace;
static bool sFontLoaded = false;

static bool sLocationActive = false;

static CircularFifo<MAEventNative, EVENT_BUFFER_SIZE> sEventFifo;

static int sMyEventDomain;
static int sMainEventChannel;

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

		NULL_ERRNO(getcwd(sCwd, PATH_MAX));

		Syscall::init();
		MANetworkInit();
#if HAVE_SCREEN
		ERRNO(screen_create_context(&sScreen, SCREEN_APPLICATION_CONTEXT));
		ERRNO(screen_create_window(&sWindow, sScreen));

		ERRNO(screen_get_context_property_iv(sScreen, SCREEN_PROPERTY_DISPLAY_COUNT, &sDisplayCount));
		sDisplays = new screen_display_t[sDisplayCount];
		ERRNO(screen_get_context_property_pv(sScreen, SCREEN_PROPERTY_DISPLAYS, (void**)sDisplays));

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
#endif
		// initialize events
		BPSERR(navigator_request_events(0));
		BPSERR(button_request_events(0));
		//BPSERR(screen_request_events(sScreen));

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

		Bluetooth::MABtInit();

		ERRNO(sMyEventDomain = bps_register_domain());
		sMainEventChannel = bps_channel_get_active();
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

	bool MAProcessEvents() {
		bpsWait(0);
		return true;
	}
}

static void sigalrmHandler(int code) {
	LOG("sigalrmHandler(%i)\n", code);
	MoSyncErrorExit(code);
}

void ConnWaitEvent() {
	bpsWait(-1);
}
void ConnPushEvent(MAEventNative* ep) {
	LOG("ConnPushEvent %p\n", ep);
	bps_event_t* be;
	bps_event_payload_t payload = { (uintptr_t)ep, 0, 0 };
	BPSERR(bps_event_create(&be, sMyEventDomain, EVENT_CODE_MA, &payload, NULL));
	BPSERR(bps_channel_push_event(sMainEventChannel, be));
}
void DefluxBinPushEvent(MAHandle handle, Stream& s) {
	bps_event_t* be;
	bps_event_payload_t payload = { handle, (uintptr_t)&s, 0 };
	BPSERR(bps_event_create(&be, sMyEventDomain, EVENT_CODE_DEFLUX, &payload, NULL));
	BPSERR(bps_channel_push_event(sMainEventChannel, be));
}

void putEvent(const MAEventNative& e) {
	sEventFifo.put(e);
}

void executeInCoreThread(Functor* f) {
	bps_event_t* be;
	bps_event_payload_t payload = { (uintptr_t)f, 0, 0 };
	BPSERR(bps_event_create(&be, sMyEventDomain, EVENT_CODE_FUNCTOR, &payload, NULL));
	BPSERR(bps_channel_push_event(sMainEventChannel, be));
}


// event handler.
// waits up to timeout milliseconds, then handles all events.
// negative timeout means waiting forever.
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
		} else if(event_domain == geolocation_get_domain()) {
			switch(event_id) {
			case GEOLOCATION_CANCEL:
				LOG("GEOLOCATION_CANCEL\n");
				event.type = EVENT_TYPE_LOCATION_PROVIDER;
				event.state = MA_LPS_OUT_OF_SERVICE;
				break;
			case GEOLOCATION_ERROR:
				LOG("GEOLOCATION_ERROR %s\n", geolocation_event_get_error_message(event_bps));
				event.type = EVENT_TYPE_LOCATION_PROVIDER;
				event.state = MA_LPS_OUT_OF_SERVICE;
				break;
			case GEOLOCATION_INFO:
				{
					event.type = EVENT_TYPE_LOCATION;
					MALocation* loc = new MALocation;
					loc->state = MA_LOC_QUALIFIED;
					loc->alt = (float)geolocation_event_get_altitude(event_bps);
					loc->lat = geolocation_event_get_latitude(event_bps);
					loc->lon = geolocation_event_get_longitude(event_bps);
					loc->horzAcc = geolocation_event_get_accuracy(event_bps);
					loc->vertAcc = geolocation_event_get_altitude_accuracy(event_bps);
					event.data = loc;
				}
				break;
			default:
				LOG("GEOLOCATION unknown event %i\n", event_id);
			}
		} else if(event_domain == sensor_get_domain()) {
			//LOG("SENSOR event %i. voff: %i\n", event_id, (char*)event.sensor.values - (char*)&event);
			event.type = EVENT_TYPE_SENSOR;
			float* v = event.sensor.values;
			switch(event_id) {
			case SENSOR_ACCELEROMETER_READING:
				event.sensor.type = MA_SENSOR_TYPE_ACCELEROMETER;
				BPSERR(sensor_event_get_xyz(event_bps, v+0, v+1, v+2));
				break;
			case SENSOR_MAGNETOMETER_READING:
				event.sensor.type = MA_SENSOR_TYPE_MAGNETIC_FIELD;
				BPSERR(sensor_event_get_xyz(event_bps, v+0, v+1, v+2));
				break;
			case SENSOR_GYROSCOPE_READING:
				event.sensor.type = MA_SENSOR_TYPE_GYROSCOPE;
				BPSERR(sensor_event_get_xyz(event_bps, v+0, v+1, v+2));
				break;
			case SENSOR_PROXIMITY_READING:
				event.sensor.type = MA_SENSOR_TYPE_PROXIMITY;
				v[0] = sensor_event_get_proximity(event_bps);
				if(v[0] == 0)
					v[0] = MA_SENSOR_PROXIMITY_VALUE_NEAR;
				else if(v[0] == 1)
					v[0] = MA_SENSOR_PROXIMITY_VALUE_FAR;
				break;
			default:
				event.type = 0;
				LOG("SENSOR unknown event %i\n", event_id);
			}
		} else if(event_domain == nfc_get_domain()) {
			handleNfcEvent(event_bps, event_id);
		} else if(event_domain == sMyEventDomain) {
			//LOG("MyEvent %i\n", event_id);
			const bps_event_payload_t* payload = bps_event_get_payload(event_bps);
			switch(event_id) {
			case EVENT_CODE_MA:
				event = *(MAEventNative*)payload->data1;
				//LOG("event %p type %i\n", (void*)payload->data1, event.type);
				delete (MAEventNative*)payload->data1;
				DEBUG_ASSERT(event.type < 100);
				break;
			case EVENT_CODE_DEFLUX:
				SYSCALL_THIS->resources.extract_RT_FLUX(payload->data1);
				ROOM(SYSCALL_THIS->resources.add_RT_BINARY(payload->data1,
					(Stream*)payload->data2));
				break;
			case EVENT_CODE_FUNCTOR:
				{
					Functor* f = (Functor*)payload->data1;
					f->func(*f);
					free(f);
				}
				break;
			default:
				DEBIG_PHAT_ERROR;
			}
		} else {
			LOG("Unknown event: domain %i, code %i\n", event_domain, event_id);
			//DEBIG_PHAT_ERROR;
		}
		if(event.type != 0) {
			if(event.type == EVENT_TYPE_WIDGET) {
				LOG("sEventFifo.put(EVENT_TYPE_WIDGET)\n");
				if(((MAWidgetEventData*)event.data)->eventType == MAW_EVENT_GL_VIEW_READY) {
					LOG("sEventFifo.put(MAW_EVENT_GL_VIEW_READY)\n");
				}
			}
			sEventFifo.put(event);
		}

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
	if(sOpenGLActive)
		bbutil_swap();
	else {
		ERRNO(screen_wait_vsync(sDisplays[0]));
		ERRNO(screen_post_window(sWindow, sScreenBuffer, 1, sScreenRect, 0));
	}
}

SYSCALL(void, maResetBacklight())
{
}

SYSCALL(MAExtent, maGetScrSize())
{
#if HAVE_SCREEN
	return EXTENT(sBackBuffer->width, sBackBuffer->height);
#else
	return getScrSize();
#endif
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

	//memcpy(dst, &sEventFifo.get(), sizeof(MAEvent));

	const MAEventNative& e(sEventFifo.get());
	dst->type = e.type;
	// copy data separately, because there can be padding in MAEventNative on 64-bit systems.
	// compare offsetof(MAEventNative, data) and offsetof(MAEvent, data)
	void* p = (char*)dst + offsetof(MAEventNative, data);
	memcpy(p, &e.data, size_t(sizeof(MAEvent) - sizeof(dst->type)));

#if 0	//debug log
	if(e.type == EVENT_TYPE_WIDGET) {
		LOG("maGetEvent(EVENT_TYPE_WIDGET)\n");
		if(((MAWidgetEventData*)e.data)->eventType == MAW_EVENT_GL_VIEW_READY) {
			LOG("maGetEvent(MAW_EVENT_GL_VIEW_READY)\n");
		}
	}
#endif

	void* cep = SYSCALL_THIS->GetCustomEventPointer();

#define HANDLE_CUSTOM_EVENT(eventType, dataType) if(dst->type == eventType) {\
	memcpy(cep, e.data, sizeof(dataType));\
	delete (dataType*)e.data;\
	dst->data = SYSCALL_THIS->TranslateNativePointerToMoSyncPointer(cep); }

	CUSTOM_EVENTS(HANDLE_CUSTOM_EVENT);

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

#define USE_ASOUND 0

#if USE_ASOUND



#else	// use MMR

#define USE_FIFO 0

static mmr_connection_t* sMmrConnection;
static mmr_context_t* sMmrContext;
static char sAudioUrl[PATH_MAX];
static const char* const AUDIO_PATH = "tmp/maSound.mp3";

extern ThreadPool* gpThreadPool;
#define gThreadPool (*gpThreadPool)

static class AudioFifoWriter : public Runnable {
public:
	int fifo;
	int end;
	Stream* src;

	void run() {
		DEBUG_ASSERT(end > 0);
		DEBUG_ASSERT(fifo < 0);
#if USE_FIFO
		LOGT("open(fifo)");
		ERRNO(fifo = open(AUDIO_PATH, O_WRONLY));
		LOGT("open(fifo) compleat");
#else
		ERRNO(fifo = open(AUDIO_PATH, O_CREAT | O_WRONLY, 0644));
#endif
		int pos;
		DEBUG_ASSERT(src->tell(pos));
		while(pos < end) {
			int res;
			const void* ptrc = src->ptrc();
			if(ptrc) {
				res = write(fifo, (byte*)ptrc + pos, end - pos);
			} else {
				char buf[4*1024];
				int todo = MIN(sizeof(buf), end - pos);
				// inefficient if write returns less than todo.
				DEBUG_ASSERT(src->seek(Seek::Start, pos));
				DEBUG_ASSERT(src->read(buf, todo));
				res = write(fifo, buf, todo);
			}
			LOG("write: %i\n", res);
			if(res < 0 && errno == EPIPE) {
				LOGT("broken pipe");
				break;
			}
			ERRNO(res);
			pos += res;
		}
	}
	AudioFifoWriter() : fifo(-1) {
	}
	virtual ~AudioFifoWriter() {
	}
} sAudioFifoWriter;

static void soundInit() {
	if(sMmrConnection)
		return;
	LOG("soundInit...\n");
	NULL_ERRNO(sMmrConnection = mmr_connect(NULL));
	NULL_ERRNO(sMmrContext = mmr_context_create(sMmrConnection, "maSound", 0, S_IRUSR | S_IXUSR));
	snprintf(sAudioUrl, PATH_MAX, "file://%s/%s", sCwd, AUDIO_PATH);

	// remove the fifo file, just in case.
	int res = unlink(AUDIO_PATH);
	if(res != 0 && errno != ENOENT) {
		DO_ERRNO;
	} else if(res >= 0) {
		LOG("%s removed.\n", sAudioUrl);
	}

#if USE_FIFO
	ERRNO(mkfifo(AUDIO_PATH, 0644));
#endif
	LOG("soundInit complete.\n");
}

void logMmrError() {
	const mmr_error_info_t* e = mmr_error_info(sMmrContext);
	DEBUG_ASSERT(e != NULL);
	LOG("MMR: %i %s %lli %s\n", e->error_code, e->extra_type, e->extra_value, e->extra_text);
}

SYSCALL(int, maSoundPlay(MAHandle sound_res, int offset, int size))
{
	soundInit();
	maSoundStop();

	// open the resource.
	LOG("Opening sound resource...\n");
	sAudioFifoWriter.src = gSyscall->resources.get_RT_BINARY(sound_res);
	MYASSERT(sAudioFifoWriter.src->seek(Seek::Start, offset), ERR_DATA_ACCESS_FAILED);
	sAudioFifoWriter.end = offset + size;
	int length;
	MYASSERT(sAudioFifoWriter.src->length(length), ERR_DATA_ACCESS_FAILED);
	MYASSERT(sAudioFifoWriter.end <= length, ERR_DATA_ACCESS_FAILED);

	//read the MIME type
	char mime[1024];
	size_t i=0;
	byte b;
	do {
		if(!sAudioFifoWriter.src->readByte(b) || i >= sizeof(mime)) {
			BIG_PHAT_ERROR(ERR_MIME_READ_FAILED);
		}
		mime[i++] = b;
	} while(b);
	LOG("MIME: %s\n", mime);

	// start writing from sound_res to the fifo.
	LOG("starting sAudioFifoWriter...\n");
#if USE_FIFO
	gThreadPool.execute(&sAudioFifoWriter);
#else
	// it's just a regular file now.
	sAudioFifoWriter.run();
	ERRNO(close(sAudioFifoWriter.fifo));
#endif

	// start the renderer.
	int audio_oid;
	LOG("mmr_output_attach...\n");
	MMR(audio_oid = mmr_output_attach(sMmrContext, "audio:default", "audio"));
	//LOG("mmr_output_parameters...\n");
	//MMR(mmr_output_parameters(sMmrContext, audio_oid, NULL));
	LOG("mmr_input_attach...\n");
	MMR(mmr_input_attach(sMmrContext, sAudioUrl, "track"));
	LOG("mmr_play...\n");
	MMR(mmr_play(sMmrContext));
	LOG("mmr_play done.\n");

	return 1;
}

SYSCALL(void, maSoundStop())
{
	LOGT("maSoundStop");
	MMR(mmr_stop(sMmrContext));
	MMR(mmr_input_detach(sMmrContext));
#if USE_FIFO
	if(sAudioFifoWriter.fifo >= 0) {
		ERRNO(close(sAudioFifoWriter.fifo));	// this should stop the writing thread.
		sAudioFifoWriter.fifo = -1;
	}
#endif
/*
	mmr_context_destroy(ctxt);
	mmr_disconnect(connection);
*/
}

#endif	//USE_ASOUND

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

#define USING_GLVIEW 0
#if USING_GLVIEW
static void my_frame_callback(void *) {
}

static int maOpenGLInitFullscreen(int glApi) {
	glview_api_t glvApi;
	switch(glApi) {
	case MA_GL_API_GL1: glvApi = GLVIEW_API_OPENGLES_11; break;
	case MA_GL_API_GL2: glvApi = GLVIEW_API_OPENGLES_20; break;
	default: return MA_GL_INIT_RES_UNAVAILABLE_API;
	}
	DUAL_ERRNO(GLVIEW_FAILURE, GLVIEW_SUCCESS, glview_initialize(glvApi, my_frame_callback));
	return 0;
}
#else	// use eGL.
static int maOpenGLInitFullscreen(int glApi) {
	int gl2;
	if (glApi == MA_GL_API_GL1) {
		gl2 = 0;
	} else if (glApi == MA_GL_API_GL2) {
		gl2 = 1;
	} else {
		return MA_GL_INIT_RES_UNAVAILABLE_API;
	}
	DEBUG_ASSERT(bbutil_init_egl(sWindow, gl2) == EXIT_SUCCESS);
	sOpenGLActive = true;

	MAEventNative event;
	event.type = EVENT_TYPE_WIDGET;
	MAWidgetEventData *eventData = new MAWidgetEventData;
	eventData->eventType = MAW_EVENT_GL_VIEW_READY;
	eventData->widgetHandle = 0;
	event.data = eventData;
	sEventFifo.put(event);
	LOG("PushEvent(EVENT_TYPE_WIDGET)\n");

	return MA_GL_INIT_RES_OK;
}
#endif

static int maOpenGLCloseFullscreen() {
	sOpenGLActive = false;
	return 0;
}


static int gles2ioctl(int function, int a, int b, int c, va_list argptr) {
	switch(function) {
		maIOCtl_IX_GL2_caselist;
	default:
		return IOCTL_UNAVAILABLE;
	}
};


static int maOpenGLTexImage2D(MAHandle image) {
	Image* img = gSyscall->resources.get_RT_IMAGE(image);
	DEBUG_ASSERT(img->pixelFormat == Image::PIXELFORMAT_ARGB8888);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, img->data);
	return MA_GL_TEX_IMAGE_2D_OK;
}

static int maOpenGLTexSubImage2D(MAHandle image) {
	Image* img = gSyscall->resources.get_RT_IMAGE(image);
	DEBUG_ASSERT(img->pixelFormat == Image::PIXELFORMAT_ARGB8888);
	glTexSubImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, img->data);
	return MA_GL_TEX_IMAGE_2D_OK;
}

static int maGetBatteryCharge() {
	battery_info_t* info;
	BPSERR(battery_get_info(&info));
	return battery_info_get_state_of_charge(info);
}

static int maLocationStart() {
	if(sLocationActive)
		return 0;
	BPSERR(geolocation_request_events(0));
	geolocation_set_period(1);
	sLocationActive = true;
	return MA_LPS_AVAILABLE;
}

static int maLocationStop() {
	if(sLocationActive) {
		BPSERR(geolocation_stop_events(0));
		sLocationActive = false;
	}
	return 0;
}

#define SENSOR_RATE_FASTEST_IOS 50
#define SENSOR_RATE_GAME_IOS 80
#define SENSOR_RATE_NORMAL_IOS 140
#define SENSOR_RATE_UI_IOS 160

#define SENSORS(m)\
	m(ACCELEROMETER, ACCELEROMETER)\
	m(MAGNETIC_FIELD, MAGNETOMETER)\
	m(GYROSCOPE, GYROSCOPE)\
	m(PROXIMITY, PROXIMITY)\

#define DECLARE_ACTIVITY(maName, bbName) static bool sSensorActive##bbName = false;

SENSORS(DECLARE_ACTIVITY);

#define CASE_SENSOR(maName, bbName)\
	case MA_SENSOR_TYPE_##maName:\
		type = SENSOR_TYPE_##bbName;\
		if(sSensorActive##bbName == activityCheck)\
			return activityResult;\
		break;

#define SET_SENSOR_TYPE \
	sensor_type_t type;\
	switch(sensor) {\
	SENSORS(CASE_SENSOR)\
	default:\
		return MA_SENSOR_ERROR_NOT_AVAILABLE;\
	}\
	if(!sensor_is_supported(type))\
		return MA_SENSOR_ERROR_NOT_AVAILABLE;\

static int maSensorStart(int sensor, int interval) {
	static const bool activityCheck = true;
	static const int activityResult = MA_SENSOR_ERROR_ALREADY_ENABLED;
	SET_SENSOR_TYPE;

	if(interval <= 0) switch(interval) {
		case MA_SENSOR_RATE_FASTEST: interval = SENSOR_RATE_FASTEST_IOS; break;
		case MA_SENSOR_RATE_GAME: interval = SENSOR_RATE_GAME_IOS; break;
		case MA_SENSOR_RATE_NORMAL: interval = SENSOR_RATE_NORMAL_IOS; break;
		case MA_SENSOR_RATE_UI: interval = SENSOR_RATE_UI_IOS; break;
		default: BIG_PHAT_ERROR(ERR_INVALID_SENSOR_RATE);
	}
	// Convert milliseconds to microseconds.
	interval *= 1000;

	BPSERR(sensor_set_rate(type, interval));
	BPSERR(sensor_set_skip_duplicates(type, true));
	BPSERR(sensor_request_events(type));
	return MA_SENSOR_ERROR_NONE;
}

static int maSensorStop(int sensor) {
	static const bool activityCheck = false;
	static const int activityResult = MA_SENSOR_ERROR_NOT_ENABLED;
	SET_SENSOR_TYPE;
	BPSERR(sensor_stop_events(type));
	return MA_SENSOR_ERROR_NONE;
}

static void BtWaitTrigger() {
	LOGD("BtWaitTrigger\n");
	MAEventNative* e = new MAEventNative;
	e->type = EVENT_TYPE_BT;
	e->state = Bluetooth::maBtDiscoveryState();
	ConnPushEvent(e);
}

static int maPlatformRequest(const char* uri) {
	LOG("maPlatformRequest(%s)\n", uri);

	// create handler invocation
	navigator_invoke_invocation_t *invoke = NULL;
	BPSERR(navigator_invoke_invocation_create(&invoke));

	//BPSERR(navigator_invoke_invocation_set_target(invoke, "sys.browser"));
	//BPSERR(navigator_invoke_invocation_set_type(invoke, "text/html"));
	//BPSERR(navigator_invoke_invocation_set_action(invoke, "bb.action.OPEN"));

	//BPSERR(navigator_invoke_invocation_set_type(invoke, "application/vnd.blackberry.phone.startcall"));
	//BPSERR(navigator_invoke_invocation_set_action(invoke, "bb.action.DIAL"));

	// pass URI pointing the the data (in this example, the image to view)
	BPSERR(navigator_invoke_invocation_set_uri(invoke, uri));

	// invoke the target
	// This function returns SUCCESS even though no app is invoked.
	BPSERR(navigator_invoke_invocation_send(invoke));

	// clean up resources
	BPSERR(navigator_invoke_invocation_destroy(invoke));

	return IOCTL_UNAVAILABLE;	// above code does not work.
}

static int maGetSystemProperty(const char* key, char* buf, int size) {
	size_t len;
	if(strcmp(key, "mosync.iso-639-1") == 0) {
		char* language, * country;
		BPSERR(locale_get(&language, &country));
		LOG("language: %s. country: %s\n", language, country);
		len = strlen(language);
		strncpy(buf, language, size);
		bps_free(language);
		bps_free(country);
	} else if(strcmp(key, "mosync.path.local") == 0) {
		char cwd[PATH_MAX+8];
		NULL_ERRNO(getcwd(cwd, PATH_MAX));
		len = strlen(cwd);
		strcpy(cwd + len, "/data/");
		strncpy(buf, cwd, size);
		len += 6;
	} else {
		return -2;
	}
	return len + 1;
}

static int maFileSetProperty(const char* path, int property, int value) {
	// keep it quiet.
	return MA_FERR_NO_SUCH_PROPERTY;
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
	switch(function) {

#ifdef FAKE_CALL_STACK
	case maIOCtl_maReportCallStack:
		reportCallStack();
		return 0;

		maIOCtl_case(maDumpCallStackEx);
#endif

#ifdef MEMORY_PROTECTION
	case maIOCtl_maProtectMemory:
		SYSCALL_THIS->protectMemory(a, b);
		return 0;
	case maIOCtl_maUnprotectMemory:
		SYSCALL_THIS->unprotectMemory(a, b);
		return 0;
	case maIOCtl_maSetMemoryProtection:
		SYSCALL_THIS->setMemoryProtection(a);
		return 0;
	case maIOCtl_maGetMemoryProtection:
		return SYSCALL_THIS->getMemoryProtection();
#endif

#ifdef LOGGING_ENABLED
	case maIOCtl_maWriteLog:
		{
			const char* ptr = (const char*)gSyscall->GetValidatedMemRange(a, b);
			LogBin(ptr, b);
			if(ptr[b-1] == '\n')	//hack to get rid of EOL
				b--;
		}
		return 0;
#endif	//LOGGING_ENABLED

#ifdef SUPPORT_OPENGL_ES
#define glGetPointerv maGlGetPointerv
#define GL2_CASE(i) case maIOCtl_##i:
	maIOCtl_IX_OPENGL_ES_caselist;
	maIOCtl_IX_GL1_caselist;
	maIOCtl_IX_GL2_m_caselist(GL2_CASE)
		return gles2ioctl(function, a, b, c, argptr);
	//maIOCtl_IX_GL_OES_FRAMEBUFFER_OBJECT_caselist;
#undef glGetPointerv
#endif	//SUPPORT_OPENGL_ES

	maIOCtl_IX_WIDGET_caselist;

	maIOCtl_case(maGetBatteryCharge);

	maIOCtl_case(maLocationStart);
	maIOCtl_case(maLocationStop);

	maIOCtl_case(maSensorStart);
	maIOCtl_case(maSensorStop);

	maIOCtl_case(maAccept);

	case maIOCtl_maBtStartDeviceDiscovery:
		return BLUETOOTH(maBtStartDeviceDiscovery)(BtWaitTrigger, a != 0);

	case maIOCtl_maBtStartServiceDiscovery:
		return BLUETOOTH(maBtStartServiceDiscovery)(GVMRA(MABtAddr), GVMR(b, MAUUID), BtWaitTrigger);

	maIOCtl_syscall_case(maBtGetNewDevice);
	maIOCtl_syscall_case(maBtGetNewService);
	maIOCtl_maBtGetNextServiceSize_case(BLUETOOTH(maBtGetNextServiceSize));
	maIOCtl_maBtCancelDiscovery_case(BLUETOOTH(maBtCancelDiscovery));

	maIOCtl_case(maPlatformRequest);
	maIOCtl_case(maGetSystemProperty);

#if 0
		maIOCtl_case(maSendTextSMS);

		//maIOCtl_case(maStartVideoStream);

		maIOCtl_case(maSendToBackground);
		maIOCtl_case(maBringToForeground);

		maIOCtl_case(maFrameBufferGetInfo);
	case maIOCtl_maFrameBufferInit:
		return maFrameBufferInit(SYSCALL_THIS->GetValidatedMemRange(a,
			gBackBuffer->pitch*gBackBuffer->h));
		maIOCtl_case(maFrameBufferClose);

		maIOCtl_case(maAudioBufferInit);
		maIOCtl_case(maAudioBufferReady);
		maIOCtl_case(maAudioBufferClose);

#ifdef MA_PROF_SUPPORT_VIDEO_STREAMING
		maIOCtl_case(maStreamVideoStart);
		maIOCtl_case(maStreamClose);
		maIOCtl_case(maStreamPause);
		maIOCtl_case(maStreamResume);
		maIOCtl_case(maStreamVideoSize);
		maIOCtl_case(maStreamVideoSetFrame);
		maIOCtl_case(maStreamLength);
		maIOCtl_case(maStreamPos);
		maIOCtl_case(maStreamSetPos);
#endif	//MA_PROF_SUPPORT_VIDEO_STREAMING
#endif	//0

		maIOCtl_syscall_case(maFileOpen);
		maIOCtl_syscall_case(maFileExists);
		maIOCtl_syscall_case(maFileClose);
		maIOCtl_syscall_case(maFileCreate);
		maIOCtl_syscall_case(maFileDelete);
		maIOCtl_syscall_case(maFileSize);
		maIOCtl_syscall_case(maFileAvailableSpace);
		maIOCtl_syscall_case(maFileTotalSpace);
		maIOCtl_syscall_case(maFileDate);
		maIOCtl_syscall_case(maFileRename);
		maIOCtl_syscall_case(maFileTruncate);

	case maIOCtl_maFileWrite:
		return SYSCALL_THIS->maFileWrite(a, SYSCALL_THIS->GetValidatedMemRange(b, c), c);
	case maIOCtl_maFileRead:
		return SYSCALL_THIS->maFileRead(a, SYSCALL_THIS->GetValidatedMemRange(b, c), c);

		maIOCtl_syscall_case(maFileWriteFromData);
		maIOCtl_syscall_case(maFileReadToData);

		maIOCtl_syscall_case(maFileTell);
		maIOCtl_syscall_case(maFileSeek);

		maIOCtl_syscall_case(maFileListStart);
	case maIOCtl_maFileListNext:
		return SYSCALL_THIS->maFileListNext(a, (char*)SYSCALL_THIS->GetValidatedMemRange(b, c), c);
		maIOCtl_syscall_case(maFileListClose);

		maIOCtl_case(maFileSetProperty);

#if 0
		maIOCtl_case(maCameraFormatNumber);
		maIOCtl_case(maCameraFormat);
		maIOCtl_case(maCameraStart);
		maIOCtl_case(maCameraStop);
		maIOCtl_case(maCameraSnapshot);

		maIOCtl_case(maDBOpen);
		maIOCtl_case(maDBClose);
		maIOCtl_case(maDBExecSQL);
		maIOCtl_case(maDBExecSQLParams);
		maIOCtl_case(maDBCursorDestroy);
		maIOCtl_case(maDBCursorNext);
		maIOCtl_case(maDBCursorGetColumnData);
		maIOCtl_case(maDBCursorGetColumnText);
		maIOCtl_case(maDBCursorGetColumnInt);
		maIOCtl_case(maDBCursorGetColumnDouble);
#ifdef EMULATOR
	maIOCtl_syscall_case(maPimListOpen);
	maIOCtl_syscall_case(maPimListNext);
	maIOCtl_syscall_case(maPimListClose);
	maIOCtl_syscall_case(maPimItemCount);
	maIOCtl_syscall_case(maPimItemGetField);
	maIOCtl_syscall_case(maPimItemFieldCount);
	maIOCtl_syscall_case(maPimItemGetAttributes);
	maIOCtl_syscall_case(maPimFieldType);
	maIOCtl_syscall_case(maPimItemGetValue);
	maIOCtl_syscall_case(maPimItemSetValue);
	maIOCtl_syscall_case(maPimItemAddValue);
	maIOCtl_syscall_case(maPimItemRemoveValue);
	maIOCtl_syscall_case(maPimItemClose);
	maIOCtl_syscall_case(maPimItemCreate);
	maIOCtl_syscall_case(maPimItemRemove);
#endif	//EMULATOR

#endif	//0

	maIOCtl_case(maNFCStart);
	maIOCtl_case(maNFCStop);
	maIOCtl_case(maNFCReadTag);
	maIOCtl_case(maNFCDestroyTag);
	//maIOCtl_case(maNFCConnectTag);
	//maIOCtl_case(maNFCCloseTag);
	maIOCtl_case(maNFCIsType);
	maIOCtl_case(maNFCGetTypedTag);
	//maIOCtl_case(maNFCBatchStart);
	//maIOCtl_case(maNFCBatchCommit);
	//maIOCtl_case(maNFCBatchRollback);
	//maIOCtl_case(maNFCTransceive);
	//maIOCtl_case(maNFCSetReadOnly);
	//maIOCtl_case(maNFCIsReadOnly);
	maIOCtl_case(maNFCGetSize);
	maIOCtl_case(maNFCGetId);
	maIOCtl_case(maNFCGetNDEFMessage);
	maIOCtl_case(maNFCGetNDEFRecordCount);
	maIOCtl_case(maNFCGetNDEFRecord);
	maIOCtl_case(maNFCGetNDEFPayload);
	maIOCtl_case(maNFCGetNDEFType);
	maIOCtl_case(maNFCGetNDEFId);
	maIOCtl_case(maNFCGetNDEFTnf);

	default:
		LOG("maIOCtl(%i) unimplemented.\n", function);
		return IOCTL_UNAVAILABLE;
	}
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
