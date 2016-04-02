/******************************************************************************
*                                                                             *
*    ToolTip.cpp                            Copyright(c) 2010-2016 itow,y.    *
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
#include "ToolTip.h"


namespace CV
{

InPlaceToolTip::InPlaceToolTip()
{
}

InPlaceToolTip::~InPlaceToolTip()
{
	Destroy();
}

bool InPlaceToolTip::Create(HWND Parent, int ID)
{
	Destroy();

	m_Handle = ::CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		TOOLTIPS_CLASS, nullptr,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		0, 0, 0, 0,
		Parent, nullptr, ::GetModuleHandle(nullptr), nullptr);
	if (m_Handle == nullptr)
		return false;

	m_hwndTool = Parent;

	TOOLINFO ti;

	ti.cbSize = sizeof(ti);
	ti.uFlags = /*TTF_IDISHWND | */TTF_TRACK | TTF_ABSOLUTE;
	ti.hwnd = Parent;
	//ti.uId = reinterpret_cast<UINT_PTR>(Parent);
	ti.uId = 1;
	::SetRectEmpty(&ti.rect);
	ti.hinst = nullptr;
	ti.lpszText = TEXT("");
	::SendMessage(m_Handle, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

	return true;
}

bool InPlaceToolTip::Show(const RECT &Rect, LPCTSTR pText)
{
	if (m_Handle == nullptr)
		return false;

	TOOLINFO ti;
	ti.cbSize = sizeof(ti);
	//ti.uFlags = TTF_IDISHWND;
	ti.uFlags = 0;
	ti.hwnd = m_hwndTool;
	//ti.uId = reinterpret_cast<UINT_PTR>(ti.hwnd);
	ti.uId = 1;
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(pText);
	::SendMessage(m_Handle, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));

	RECT rc = Rect;
	::MapWindowPoints(ti.hwnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
	::SendMessage(m_Handle, TTM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&rc));
	::SendMessage(m_Handle, TTM_TRACKPOSITION, 0, MAKELONG(rc.left, rc.top));
	::SendMessage(m_Handle, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));

	return true;
}

void InPlaceToolTip::Hide()
{
	if (m_Handle != nullptr) {
		TOOLINFO ti;

		ti.cbSize = sizeof(ti);
		//ti.uFlags = TTF_IDISHWND;
		ti.uFlags = 0;
		ti.hwnd = m_hwndTool;
		//ti.uId = reinterpret_cast<UINT_PTR>(ti.hwnd);
		ti.uId = 1;
		::SendMessage(m_Handle, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&ti));
	}
}

void InPlaceToolTip::SetFont(HFONT hfont)
{
	if (m_Handle != nullptr)
		::SendMessage(m_Handle, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), FALSE);
}

}	// namespace CV
