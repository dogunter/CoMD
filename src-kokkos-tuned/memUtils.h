/// \file
/// Wrappers for memory allocation.

#ifndef _MEMUTILS_H_
#define _MEMUTILS_H_

#include <stdlib.h>

#ifdef __INTEL_COMPILER
	#define ASSUME_ALIGN
	#define _ALIGN_INT 64
#endif

#define freeMe(s,element) {if(s->element) comdFree(s->element);  s->element = NULL;}

static void* comdMalloc(size_t iSize)
{
#ifdef ASSUME_ALIGN
	return _mm_malloc(iSize,_ALIGN_INT);
#else
	return malloc(iSize);
#endif
}

static void* comdCalloc(size_t num, size_t iSize)
{
   return calloc(num, iSize);
}

static void* comdRealloc(void* ptr, size_t iSize)
{
   return realloc(ptr, iSize);
}

static void comdFree(void *ptr)
{
#ifdef ASSUME_ALIGN
	_mm_free(ptr);
#else
	free(ptr);
#endif
}

#endif


