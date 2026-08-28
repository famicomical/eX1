/*
	malloc.h compatibility shim for macOS / BSD.
	<malloc.h> is a Linux/Win header; the standard allocation
	declarations live in <stdlib.h> on macOS.
*/
#ifndef _CSP_COMPAT_MALLOC_H_
#define _CSP_COMPAT_MALLOC_H_
#include <stdlib.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif
#endif
