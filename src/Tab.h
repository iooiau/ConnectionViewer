/******************************************************************************
*                                                                             *
*    Tab.h                                  Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_TAB_H
#define CV_TAB_H


#include "Widget.h"


namespace CV
{

class Tab : public Widget
{
public:
	Tab();
	~Tab();
	bool Create(HWND Parent, int ID = 0) override;
	bool InsertItem(int Index, LPCTSTR pText, int Image = -1);
	bool AddItem(LPCTSTR pText, int Image = -1) { return InsertItem(-1, pText, Image); }
	bool DeleteItem(int Index);
	void SetImageList(HIMAGELIST ImageList);
	bool SetImage(int Index, int Image);
	int GetTabHeight() const;
	void SetSel(int Index);
	int GetSel() const;
	bool SetFixedWidth(bool Fixed);

private:
	HFONT m_Font;
	HIMAGELIST m_ImageList;
};

}	// namespace CV


#endif	// ndef CV_TAB_H
