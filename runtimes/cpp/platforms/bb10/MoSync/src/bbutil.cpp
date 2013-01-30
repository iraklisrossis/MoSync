#include "config_platform.h"
#include "helpers/helpers.h"
#include "bb10err.h"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <stdlib.h>

#include "bbutil.h"

static EGLDisplay egl_disp;
static EGLSurface egl_surf;
static EGLConfig egl_conf;
static EGLContext egl_ctx;
static int initialized = 0;
static screen_display_t screen_disp;

static void bbutil_egl_perror(const char *msg) {
	static const char *errmsg[] = {
		"function succeeded",
		"EGL is not initialized, or could not be initialized, for the specified display",
		"cannot access a requested resource",
		"failed to allocate resources for the requested operation",
		"an unrecognized attribute or attribute value was passed in an attribute list",
		"an EGLConfig argument does not name a valid EGLConfig",
		"an EGLContext argument does not name a valid EGLContext",
		"the current surface of the calling thread is no longer valid",
		"an EGLDisplay argument does not name a valid EGLDisplay",
		"arguments are inconsistent",
		"an EGLNativePixmapType argument does not refer to a valid native pixmap",
		"an EGLNativeWindowType argument does not refer to a valid native window",
		"one or more argument values are invalid",
		"an EGLSurface argument does not name a valid surface configured for rendering",
		"a power management event has occurred",
		"unknown error code"
	};

	int message_index = eglGetError() - EGL_SUCCESS;

	if (message_index < 0 || message_index > 14)
		message_index = 15;

	LOG("%s: %s\n", msg, errmsg[message_index]);
}

#define EGL_FAIL LOG("FAIL at %s:%i\n", __FILE__, __LINE__); return EXIT_FAILURE

int bbutil_init_egl(screen_window_t screen_win, int gl2) {
	LOG("bbutil_init_egl(%i)\n", gl2);
	int usage;
	//int format = SCREEN_FORMAT_RGBX8888;
	EGLint interval = 1;
	int rc, num_configs;
	EGLint* attributes;

	EGLint attrib_list[]= {
		EGL_RED_SIZE,        8,
		EGL_GREEN_SIZE,      8,
		EGL_BLUE_SIZE,       8,
		EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, 0,
		EGL_DEPTH_SIZE, 8,
		EGL_NONE};

	if(gl2) {
		static EGLint a[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
		attributes = a;
		usage = SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_ROTATION;
		attrib_list[9] = EGL_OPENGL_ES2_BIT;
	} else {
		usage = SCREEN_USAGE_OPENGL_ES1 | SCREEN_USAGE_ROTATION;
		attrib_list[9] = EGL_OPENGL_ES_BIT;
	}

	//Simple egl initialization
	//screen_ctx = ctx;

	egl_disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (egl_disp == EGL_NO_DISPLAY) {
		bbutil_egl_perror("eglGetDisplay");
		bbutil_terminate();
		EGL_FAIL;
	}

	rc = eglInitialize(egl_disp, NULL, NULL);
	if (rc != EGL_TRUE) {
		bbutil_egl_perror("eglInitialize");
		bbutil_terminate();
		EGL_FAIL;
	}

	rc = eglBindAPI(EGL_OPENGL_ES_API);

	if (rc != EGL_TRUE) {
		bbutil_egl_perror("eglBindApi");
		bbutil_terminate();
		EGL_FAIL;
	}

	if(!eglChooseConfig(egl_disp, attrib_list, &egl_conf, 1, &num_configs)) {
		bbutil_egl_perror("eglChooseConfig");
		bbutil_terminate();
		EGL_FAIL;
	}

	if(gl2)
		egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, attributes);
	else
		egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, NULL);


	if (egl_ctx == EGL_NO_CONTEXT) {
		bbutil_egl_perror("eglCreateContext");
		bbutil_terminate();
		EGL_FAIL;
	}

#if 0
	ERRNO(screen_create_window(&screen_win, screen_ctx));
	ERRNO(screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &format));
#endif

	ERRNO(screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage));

#if 1
	ERRNO(screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_DISPLAY, (void **)&screen_disp));

	{
		const char *env = getenv("WIDTH");

	if (0 == env) {
		LOG("failed getenv for WIDTH\n");
		bbutil_terminate();
		EGL_FAIL;
	}

	{
	int width = atoi(env);

	env = getenv("HEIGHT");

	if (0 == env) {
		LOG("failed getenv for HEIGHT\n");
		bbutil_terminate();
		EGL_FAIL;
	}

	{
	int height = atoi(env);
	int size[2] = { width, height };

	ERRNO(screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE, size));
}
}
}

{
static int nbuffers = 2;
	ERRNO(screen_create_window_buffers(screen_win, nbuffers));
}
#endif

	egl_surf = eglCreateWindowSurface(egl_disp, egl_conf, screen_win, NULL);
	if (egl_surf == EGL_NO_SURFACE) {
		bbutil_egl_perror("eglCreateWindowSurface");
		bbutil_terminate();
		EGL_FAIL;
	}

	rc = eglMakeCurrent(egl_disp, egl_surf, egl_surf, egl_ctx);
	if (rc != EGL_TRUE) {
		bbutil_egl_perror("eglMakeCurrent");
		bbutil_terminate();
		EGL_FAIL;
	}

	rc = eglSwapInterval(egl_disp, interval);
	if (rc != EGL_TRUE) {
		bbutil_egl_perror("eglSwapInterval");
		bbutil_terminate();
		EGL_FAIL;
	}

	initialized = 1;

	LOG("success.\n");
	return EXIT_SUCCESS;
}

void bbutil_terminate(void) {
	//Typical EGL cleanup
	if (egl_disp != EGL_NO_DISPLAY) {
		eglMakeCurrent(egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (egl_surf != EGL_NO_SURFACE) {
			eglDestroySurface(egl_disp, egl_surf);
			egl_surf = EGL_NO_SURFACE;
		}
		if (egl_ctx != EGL_NO_CONTEXT) {
			eglDestroyContext(egl_disp, egl_ctx);
			egl_ctx = EGL_NO_CONTEXT;
		}
#if 0
		if (screen_win != NULL) {
			ERRNO(screen_destroy_window(screen_win));
			screen_win = NULL;
		}
#endif
		eglTerminate(egl_disp);
		egl_disp = EGL_NO_DISPLAY;
	}
	eglReleaseThread();

	initialized = 0;
}

void bbutil_swap(void) {
	//LOG("bbutil_swap\n");
	int rc = eglSwapBuffers(egl_disp, egl_surf);
	if (rc != EGL_TRUE) {
		bbutil_egl_perror("eglSwapBuffers");
	}
}
