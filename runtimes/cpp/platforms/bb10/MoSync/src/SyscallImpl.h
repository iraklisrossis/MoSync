/*
 * SyscallImpl.h
 *
 *  Created on: Nov 23, 2012
 *      Author: andersmalm
 */

#ifndef SYSCALLIMPL_H_
#define SYSCALLIMPL_H_

#if !NATIVE_PROGRAM
extern bool gReload;
void reloadProgram() GCCATTRIB(noreturn);
#endif

class Syscall {
public:
//SDL_Surface* loadImage(MemStream& s);
//SDL_Surface* loadSprite(SDL_Surface* surface, ushort left, ushort top,
//	ushort width, ushort height, ushort cx, ushort cy);

	Image* loadImage(MemStream& s);
	Image* loadSprite(void* surface, ushort left, ushort top, ushort width, ushort height, ushort cx, ushort cy);

public:
	/*
		struct STARTUP_SETTINGS {
			STARTUP_SETTINGS ( ) {
				showScreen = true;
				haveSkin   = true;
				id         = NULL;
				iconPath   = NULL;
				resmem     = ((uint)-1);
			}

			bool showScreen;
			const char* id;
			const char *iconPath;
			uint resmem;
			MoRE::DeviceProfile profile;
			bool haveSkin;
		};
	*/

	Syscall();

#endif /* SYSCALLIMPL_H_ */
