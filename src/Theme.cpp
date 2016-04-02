/******************************************************************************
*                                                                             *
*    Theme.cpp                              Copyright(c) 2010-2016 itow,y.    *
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
#include "Theme.h"

#pragma comment(lib, "uxtheme.lib")


namespace CV
{

ThemePainter::ThemePainter()
	: m_Theme(nullptr)
{
}

ThemePainter::~ThemePainter()
{
	Close();
}

bool ThemePainter::Open(HWND hwnd, LPCWSTR pClassList)
{
	Close();

	if (!::IsThemeActive() || !::IsAppThemed())
		return false;
	m_Theme = ::OpenThemeData(hwnd, pClassList);
	if (m_Theme == nullptr)
		return false;
	return true;
}

void ThemePainter::Close()
{
	if (m_Theme != nullptr) {
		::CloseThemeData(m_Theme);
		m_Theme = nullptr;
	}
}

bool ThemePainter::DrawBackground(HDC hdc, int PartID, int StateID, const RECT &Rect)
{
	if (m_Theme == nullptr)
		return false;

	if (::IsThemeBackgroundPartiallyTransparent(m_Theme, PartID, StateID))
		::DrawThemeParentBackground(::WindowFromDC(hdc), hdc, &Rect);
	::DrawThemeBackground(m_Theme, hdc, PartID, StateID, &Rect, nullptr);
	return true;
}

bool ThemePainter::GetColor(int PartID, int StateID, int PropID, COLORREF *pColor)
{
	if (m_Theme == nullptr)
		return false;
	return ::GetThemeColor(m_Theme, PartID, StateID, PropID, pColor) == S_OK;
}

COLORREF ThemePainter::GetSysColor(int ColorID)
{
	return ::GetThemeSysColor(m_Theme, ColorID);
}

bool ThemePainter::GetPartSize(HDC hdc, int PartID, int StateID, THEMESIZE ThemeSize, SIZE *pSize)
{
	if (m_Theme == nullptr)
		return false;

	return ::GetThemePartSize(m_Theme, hdc, PartID, StateID, nullptr, ThemeSize, pSize) == S_OK;
}

}	// namespace CV
