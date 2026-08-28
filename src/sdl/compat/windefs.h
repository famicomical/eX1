/*
	Win32 constant compatibility for the SDL OSD build.

	CSP uses Windows virtual-key codes as its internal key-code space
	(the VM keyboard tables are indexed by VK_*). On macOS/POSIX these
	are undefined, so we provide the standard numeric values here.
	Included from sdl/osd.h so every TU that pulls emu.h sees them.
*/
#ifndef _CSP_COMPAT_WINDEFS_H_
#define _CSP_COMPAT_WINDEFS_H_

#ifndef _WIN32

// ---- virtual key codes (subset CSP actually uses, full common range) ----
#define VK_LBUTTON     0x01
#define VK_RBUTTON     0x02
#define VK_CANCEL      0x03
#define VK_MBUTTON     0x04
#define VK_BACK        0x08
#define VK_TAB         0x09
#define VK_CLEAR       0x0C
#define VK_RETURN      0x0D
#define VK_SHIFT       0x10
#define VK_CONTROL     0x11
#define VK_MENU        0x12
#define VK_PAUSE       0x13
#define VK_CAPITAL     0x14
#define VK_KANA        0x15
#define VK_KANJI       0x19
#define VK_ESCAPE      0x1B
#define VK_SPACE       0x20
#define VK_PRIOR       0x21
#define VK_NEXT        0x22
#define VK_END         0x23
#define VK_HOME        0x24
#define VK_LEFT        0x25
#define VK_UP          0x26
#define VK_RIGHT       0x27
#define VK_DOWN        0x28
#define VK_SELECT      0x29
#define VK_PRINT       0x2A
#define VK_EXECUTE     0x2B
#define VK_SNAPSHOT    0x2C
#define VK_INSERT      0x2D
#define VK_DELETE      0x2E
#define VK_HELP        0x2F
// 0x30-0x39 : '0'-'9'  /  0x41-0x5A : 'A'-'Z' (ASCII)
#define VK_LWIN        0x5B
#define VK_RWIN        0x5C
#define VK_APPS        0x5D
#define VK_NUMPAD0     0x60
#define VK_NUMPAD1     0x61
#define VK_NUMPAD2     0x62
#define VK_NUMPAD3     0x63
#define VK_NUMPAD4     0x64
#define VK_NUMPAD5     0x65
#define VK_NUMPAD6     0x66
#define VK_NUMPAD7     0x67
#define VK_NUMPAD8     0x68
#define VK_NUMPAD9     0x69
#define VK_MULTIPLY    0x6A
#define VK_ADD         0x6B
#define VK_SEPARATOR   0x6C
#define VK_SUBTRACT    0x6D
#define VK_DECIMAL     0x6E
#define VK_DIVIDE      0x6F
#define VK_F1          0x70
#define VK_F2          0x71
#define VK_F3          0x72
#define VK_F4          0x73
#define VK_F5          0x74
#define VK_F6          0x75
#define VK_F7          0x76
#define VK_F8          0x77
#define VK_F9          0x78
#define VK_F10         0x79
#define VK_F11         0x7A
#define VK_F12         0x7B
#define VK_F13         0x7C
#define VK_F14         0x7D
#define VK_F15         0x7E
#define VK_NUMLOCK     0x90
#define VK_SCROLL      0x91
#define VK_LSHIFT      0xA0
#define VK_RSHIFT      0xA1
#define VK_LCONTROL    0xA2
#define VK_RCONTROL    0xA3
#define VK_LMENU       0xA4
#define VK_RMENU       0xA5
#define VK_OEM_1       0xBA  // ';:'
#define VK_OEM_PLUS    0xBB  // '+'
#define VK_OEM_COMMA   0xBC  // ','
#define VK_OEM_MINUS   0xBD  // '-'
#define VK_OEM_PERIOD  0xBE  // '.'
#define VK_OEM_2       0xBF  // '/?'
#define VK_OEM_3       0xC0  // '`~'
#define VK_OEM_4       0xDB  // '[{'
#define VK_OEM_5       0xDC  // '\|'
#define VK_OEM_6       0xDD  // ']}'
#define VK_OEM_7       0xDE  // ''"'
#define VK_OEM_8       0xDF
#define VK_OEM_102     0xE2  // '<>' or '\|' on 102-key
#define VK_OEM_AUTO    0xF3
#define VK_OEM_COPY    0xF2
#define VK_OEM_ENLW    0xF4

// ---- pointer-sized integer types ----
#include <stdint.h>
#ifndef LONG_PTR
typedef intptr_t  LONG_PTR;
#endif
#ifndef ULONG_PTR
typedef uintptr_t ULONG_PTR;
#endif
#ifndef INT_PTR
typedef intptr_t  INT_PTR;
#endif
#ifndef UINT_PTR
typedef uintptr_t UINT_PTR;
#endif
#ifndef LONG
typedef int32_t   LONG;
#endif
#ifndef ULONG
typedef uint32_t  ULONG;
#endif

// console text-attribute flags used by debugger console
#ifndef FOREGROUND_BLUE
#define FOREGROUND_BLUE      0x0001
#define FOREGROUND_GREEN     0x0002
#define FOREGROUND_RED       0x0004
#define FOREGROUND_INTENSITY 0x0008
#endif

#endif // !_WIN32

#endif // _CSP_COMPAT_WINDEFS_H_
