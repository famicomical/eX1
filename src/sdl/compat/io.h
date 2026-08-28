/*
	io.h compatibility shim for macOS / POSIX (SDL OSD build)

	common.h includes <io.h> unconditionally; that header is Win32-only.
	This shim satisfies the include and provides the few _-prefixed
	low-level I/O aliases CSP code may reference. Expand as needed.
*/
#ifndef _CSP_COMPAT_IO_H_
#define _CSP_COMPAT_IO_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <wchar.h>

#ifndef _WIN32

// low-level file descriptor I/O aliases
#ifndef _open
#define _open   open
#endif
#ifndef _close
#define _close  close
#endif
#ifndef _read
#define _read   read
#endif
#ifndef _write
#define _write  write
#endif
#ifndef _lseek
#define _lseek  lseek
#endif
#ifndef _access
#define _access access
#endif
#ifndef _unlink
#define _unlink unlink
#endif
#ifndef _fileno
#define _fileno fileno
#endif

// access() mode flags (Win names)
#ifndef F_OK
#define F_OK 0
#endif

#endif // !_WIN32

#endif // _CSP_COMPAT_IO_H_
