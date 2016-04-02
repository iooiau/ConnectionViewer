/******************************************************************************
*                                                                             *
*    ConnectionViewer.h                     Copyright(c) 2010-2016 itow,y.    *
*                                                                             *
******************************************************************************/

/*
  Connection Viewer
  Copyright(c) 2010-2016 itow,y.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef CONNECTION_VIEWER_H
#define CONNECTION_VIEWER_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _DEBUG
#define _SECURE_SCL 0
#endif

#include <cstdlib>
#include <cstring>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <tchar.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#define cvAbstractClass(name) class __declspec(novtable) name abstract

#define cvLengthOf(a) (sizeof(a) / sizeof(a[0]))


#include "Debug.h"
#include "Base.h"


#endif	// ndef CONNECTION_VIEWER_H
