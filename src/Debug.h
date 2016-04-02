/******************************************************************************
*                                                                             *
*    Debug.h                                Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_DEBUG_H
#define CV_DEBUG_H


namespace CV
{

namespace Debug
{

void Trace(LPCTSTR pFormat, ...);
void Assert(bool Value, LPCTSTR pFile, int Line);
void Break(LPCTSTR pFile, int Line);

}	// namespace Debug

#ifdef _DEBUG
#define CV_TEXT_(text)		TEXT(text)
#define cvDebugTrace		Debug::Trace
#define cvDebugAssert(e)	Debug::Assert(e, CV_TEXT_(__FILE__), __LINE__)
#define cvDebugBreak()		Debug::Break(CV_TEXT_(__FILE__), __LINE__)
#else
#define cvDebugTrace		__noop
#define cvDebugAssert(e)
#define cvDebugBreak()
#endif
#define cvStaticAssert(e)	static_assert(e, "")

}	// namespace CV


#endif	// ndef CV_DEBUG_H
