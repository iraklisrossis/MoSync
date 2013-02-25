/*
 * Platform.h
 *
 *  Created on: Nov 23, 2012
 *      Author: andersmalm
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <string>

#include "config_platform.h"

#include <bluetooth/discovery.h>

//#include "pim.h"
//#include "helpers/CPP_IX_PIM.h"

#define FILESYSTEM_CHROOT 0

#define SUPPORT_OPENGL_ES
#define MA_PROF_SUPPORT_WIDGETAPI	// required by OpenGL.


namespace Core {
	class VMCore;
}

#define VSV_ARGPTR_DECL , va_list argptr
#define VSV_ARGPTR_USE , argptr



#endif /* PLATFORM_H_ */
