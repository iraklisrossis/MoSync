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

#include "config_platform.h"
#include <helpers/helpers.h>
#include "bb10err.h"

#include "ThreadPoolImpl.h"
using namespace MoSyncError;

#include <errno.h>
#include <unistd.h>

//*****************************************************************************
//MoSyncThread
//*****************************************************************************

MoSyncThread::MoSyncThread() : mThread(0) {
}

typedef void* (*VPFunc)(void*);

void MoSyncThread::start(int (*func)(void*), void* arg) {
	int res = pthread_create(&mThread, NULL, (VPFunc)func, arg);
	DEBUG_ASSERT(res == EOK);
	DEBUG_ASSERT(mThread != 0);
}

int MoSyncThread::join() {
	if(mThread != 0) {
		void* rp;
		int res = pthread_join(mThread, &rp);
		DEBUG_ASSERT(res == EOK);
		return (int)rp;
	} else {
		LOG("Joining broken thread!\n");
		return 0;
	}
}

void MoSyncThread::sleep(unsigned int ms) {
  delay(ms);
}

bool MoSyncThread::isCurrent() {
	if(mThread) {
		if(pthread_self() == mThread)
			return true;
	}
	return false;
}

//*****************************************************************************
//MoSyncSemaphore
//*****************************************************************************

MoSyncSemaphore::MoSyncSemaphore() {
	ERRNO(sem_init(&mSem, 0, 0));
}

MoSyncSemaphore::~MoSyncSemaphore() {
	ERRNO(sem_destroy(&mSem));
}

void MoSyncSemaphore::wait() {
	ERRNO(sem_wait(&mSem));
}

void MoSyncSemaphore::post() {
	ERRNO(sem_post(&mSem));
}
