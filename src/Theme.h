/******************************************************************************
*                                                                             *
*    Theme.h                                Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_THEME_H
#define CV_THEME_H


#include <uxtheme.h>
#include <vssym32.h>


namespace CV
{

class ThemePainter
{
public:
	ThemePainter();
	~ThemePainter();
	bool Open(HWND hwnd, LPCWSTR pClassList);
	void Close();
	bool IsOpen() const { return m_Theme != nullptr; }
	bool DrawBackground(HDC hdc, int PartID, int StateID, const RECT &Rect);
	bool GetColor(int PartID, int StateID, int PropID, COLORREF *pColor);
	COLORREF GetSysColor(int ColorID);
	bool GetPartSize(HDC hdc, int PartID, int StateID, THEMESIZE ThemeSize, SIZE *pSize);

private:
	HTHEME m_Theme;
};

}	// namespace CV


#endif	// CV_THEME_H
