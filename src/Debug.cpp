/******************************************************************************
*                                                                             *
*    Debug.cpp                              Copyright(c) 2010-2016 itow,y.    *
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


#include "ConnectionViewer.h"
#include "Utility.h"


namespace CV
{

namespace Debug
{

void Trace(LPCTSTR pFormat, ...)
{
	TCHAR szText[1024];
	va_list Args;

	va_start(Args, pFormat);
	FormatStringV(szText, cvLengthOf(szText), pFormat, Args);
	va_end(Args);
	::OutputDebugString(szText);
}

void Assert(bool Value, LPCTSTR pFile, int Line)
{
	if (!Value) {
		Trace(TEXT("%s (%d) : Assertion failed\n"), pFile, Line);
		::DebugBreak();
	}
}

void Break(LPCTSTR pFile, int Line)
{
	Trace(TEXT("%s (%d) : Debug break\n"), pFile, Line);
	::DebugBreak();
}

}	// namespace Debug

}	// namespace CV
