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

#include "ma.h"
#include "maheap.h"
#include "mastack.h"
#include "maassert.h"

#ifdef MOSYNCDEBUG
#include "mavsprintf.h"
#endif

#if defined(MOSYNCDEBUG) && !defined(__arm__)
#define HAVE_STACK_DUMP 1
#else
#define HAVE_STACK_DUMP 0
#endif

malloc_handler gMallocHandler = default_malloc_handler;
malloc_hook gMallocHook = NULL;
free_hook gFreeHook = NULL;
realloc_hook gReallocHook = NULL;
block_size_hook gBlockSizeHook = NULL;

#ifdef MAPIP
static void* sHeapBase;
static int sHeapLength;
#endif

#ifdef MOSYNCDEBUG
//#define MEMORY_PROTECTION
int gUsedMem = 0;
int gWastedMem = 0;
int gNumMallocs = 0, gNumFrees = 0;
#endif

#if HAVE_STACK_DUMP
// The dumpStack system has been moved to a separate file in order to avoid
// linking it if it is not in use.
// This was done because _getStackTop is not valid in Rebuild mode.
// The variable gDumpStack must be defined here; if it is defined in mastackdump.c,
// that entire file gets linked, again causing the original problem.
void (*gDumpStack)(int req, int block, void* address) = NULL;
#define dumpStack if(gDumpStack) gDumpStack
#endif

void default_malloc_handler(int size) {
#ifdef MOSYNCDEBUG
	lprintfln("um %i", gUsedMem);
	lprintfln("wm %i", gWastedMem);
	lprintfln("nm %i, nf %i", gNumMallocs, gNumFrees);
#if HAVE_STACK_DUMP
	dumpStack(size, 0, 0);
#endif
#endif
	maPanic(size, "Malloc failed. You most likely ran out of heap memory. Try to increase the heap size.");
}

malloc_handler set_malloc_handler(malloc_handler new) {
	malloc_handler temp = gMallocHandler;
	gMallocHandler = new;
	return temp;
}

malloc_hook set_malloc_hook(malloc_hook new) {
	malloc_hook temp = gMallocHook;
	gMallocHook = new;
	return temp;
}

realloc_hook set_realloc_hook(realloc_hook new) {
	realloc_hook temp = gReallocHook;
	gReallocHook = new;
	return temp;
}

free_hook set_free_hook(free_hook new) {
	free_hook temp = gFreeHook;
	gFreeHook = new;
	return temp;
}

block_size_hook set_block_size_hook(block_size_hook new) {
	block_size_hook temp = gBlockSizeHook;
	gBlockSizeHook = new;
	return temp;
}

#ifdef MAPIP

#include "tlsf.h"

//#define MASTD_HEAP_LOGGING

#ifdef MASTD_HEAP_LOGGING
#include "conprint.h"
#define MASTD_HEAP_LOG lprintfln
#else
#define MASTD_HEAP_LOG(...)
#endif

//****************************************
//				NewPtr
//****************************************

void override_heap_init_crt0(char* start, int length) __attribute__ ((weak, alias ("ansi_heap_init_crt0")));

void ansi_heap_init_crt0(char *start, int length)
{
	int res;
	if(maCheckInterfaceVersion(MAIDL_HASH) != (int)MAIDL_HASH) {
		maPanic(1, "Interface version mismatch!");
	}

	MASTD_HEAP_LOG("heap: start 0x%p len 0x%x", start, length);

	sHeapBase = start;
	sHeapLength = length;

	if(length <= 0)
		return;
	res = init_memory_pool(length, start);
	if(res < 0) {
		maPanic(1, "init_memory_pool failed!");
	}
	set_malloc_hook((malloc_hook)tlsf_malloc);
	set_free_hook(tlsf_free);
	set_realloc_hook((realloc_hook)tlsf_realloc);
	set_block_size_hook((block_size_hook)tlsf_block_size);

	MASTD_HEAP_LOG("TLSF initialized!");
}

size_t heapTotalMemory(void) {
	return sHeapLength;
}
size_t heapFreeMemory(void) {
	return heapTotalMemory() - get_used_size(sHeapBase);
}

//****************************************
// _crt_tor_chain
//****************************************
typedef void (*VoidFunc)(void);

extern VoidFunc* __CTOR_LIST__;
extern VoidFunc* __DTOR_LIST__;

void crt_ctor_chain(void);
void crt_ctor_chain(void) {
	VoidFunc* ctor = __CTOR_LIST__;
	if(!ctor)
		return;
	// run all ctors.
	while(*ctor) {
		(*ctor)();
		ctor++;
	}
}

void crt_dtor_chain(void);
void crt_dtor_chain(void) {
	VoidFunc* dtor = __DTOR_LIST__;
	if(!dtor)
		return;
	// run all dtors.
	while(*dtor) {
		(*dtor)();
		dtor++;
	}
}

void exit(int code) {
	static int exiting = 0;
	MAASSERT(!exiting);
	exiting = 1;
	crt_dtor_chain();
	maExit(code);
}

//****************************************
//				malloc
//****************************************

void * malloc(size_t size)
{
	void *result;

#ifdef MEMORY_PROTECTION
	int wasMemoryProtected = maGetMemoryProtection();
	if(wasMemoryProtected < 0) {
		maPanic(0, "maGetMemoryProtection fail");
	}
	maSetMemoryProtection(FALSE);
#endif
	MASTD_HEAP_LOG("malloc(%d)\n", size);

	result = gMallocHook(size);

	if(result == 0)
	{
		if (!gMallocHandler)
			return 0;
		gMallocHandler(size);
		result = gMallocHook(size);
	}
#ifdef MOSYNCDEBUG
	if(result != NULL) {
		gNumMallocs++;
		gUsedMem += gBlockSizeHook(result);
		gWastedMem += gBlockSizeHook(result) - size;
#if HAVE_STACK_DUMP
		dumpStack(size, gBlockSizeHook(result), result);
#endif
		}
#endif

#ifdef MEMORY_PROTECTION
	maUnprotectMemory((byte*)result, size);
	maSetMemoryProtection(wasMemoryProtected);
#endif
	MASTD_HEAP_LOG("malloc(%i) pointer (0x%p)\n", size,result);

	return result;
}

//****************************************
//				calloc
//****************************************

void * calloc(size_t num_elem, size_t size_elem)
{
	size_t size = num_elem * size_elem;
	void *mem;

	MASTD_HEAP_LOG("calloc(%i, %i)\n", num_elem, size_elem);
	mem = (void *) malloc(size);

	if (mem == 0)
		return mem;

	memset(mem, 0, size);			// Do initial clear
	return mem;
}

//****************************************
//				free
//****************************************

void free(void *mem)
{
#ifdef MEMORY_PROTECTION
	int wasMemoryProtected;
#endif
	MASTD_HEAP_LOG("free(0x%08X)\n", (int)mem);

	if (!mem)
		return;

#ifdef MEMORY_PROTECTION
	wasMemoryProtected = maGetMemoryProtection();
	maSetMemoryProtection(FALSE);
	if(gBlockSizeHook)
		maProtectMemory(mem, gBlockSizeHook(mem));
#endif

#ifdef MOSYNCDEBUG
	gNumFrees++;
	gUsedMem -= gBlockSizeHook(mem);
#if HAVE_STACK_DUMP
	dumpStack(-1, gBlockSizeHook(mem), mem);
#endif
#endif

	gFreeHook(mem);

#ifdef MEMORY_PROTECTION
	maSetMemoryProtection(wasMemoryProtected);
#endif

}

//****************************************
//				nice realloc
//****************************************
void* realloc(void* old, size_t size) {
	void* result;
	MASTD_HEAP_LOG("realloc(0x%08X, %i)\n", (int)old, size);

#ifdef MOSYNCDEBUG
	// we'll count it as a free + malloc
	if (old != NULL) {
		gNumFrees++;
		gUsedMem -= gBlockSizeHook(old);
#if HAVE_STACK_DUMP
		dumpStack(-1, gBlockSizeHook(old), old);
#endif
	}
#endif

	result = gReallocHook(old, size);
	if(result == 0)
	{
		if (!gMallocHandler)
			return 0;
		gMallocHandler(size);
		result = gReallocHook(old, size);
	}

#ifdef MOSYNCDEBUG
	if(result != NULL) {
		gNumMallocs++;
		gUsedMem += gBlockSizeHook(result);
		gWastedMem += gBlockSizeHook(result) - size;
#if HAVE_STACK_DUMP
		dumpStack(size, gBlockSizeHook(result), result);
#endif
	}
#endif

	return result;
}

#endif /* MAPIP */
