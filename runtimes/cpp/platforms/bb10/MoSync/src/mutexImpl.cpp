/* Copyright (C) 2009 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#undef LOGGING_ENABLED	//debugger
#include "config_platform.h"

#include <helpers/helpers.h>

#define NETWORKING_H
#include "networking.h"

//***************************************************************************
//MoSyncMutex
//***************************************************************************

MoSyncMutex::MoSyncMutex() : mInited(false) {}

void MoSyncMutex::init() {
	DEBUG_ASSERT(!mInited);
	int res = pthread_mutex_init(&mMutex, NULL);
	DEBUG_ASSERT(res == EOK);
	mInited = true;
}

MoSyncMutex::~MoSyncMutex() {
	DEBUG_ASSERT(!mInited);	//make sure it's closed
}

void MoSyncMutex::close() {
	if(mInited) {
		int res = pthread_mutex_destroy(&mMutex);
		DEBUG_ASSERT(res == EOK);
		mInited = false;
	}
}

void MoSyncMutex::lock() {
	int res = pthread_mutex_lock(&mMutex);
	DEBUG_ASSERT(res == EOK);
}

void MoSyncMutex::unlock() {
	int res = pthread_mutex_unlock(&mMutex);
	DEBUG_ASSERT(res == EOK);
}
