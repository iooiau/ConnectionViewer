/******************************************************************************
*                                                                             *
*    ToolBar.h                              Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_TOOLBAR_H
#define CV_TOOLBAR_H


#include "Widget.h"


namespace CV
{

class ToolBar : public Widget
{
public:
	ToolBar();
	~ToolBar();
	bool Create(HWND Parent, int ID = 0) override;
	bool SetImageList(HIMAGELIST ImageList);
	int NumItems() const;
	bool InsertItem(int Index, int Command, int Image, LPCTSTR pText, BYTE Style = BTNS_BUTTON);
	bool AddItem(int Command, int Image, LPCTSTR pText, BYTE Style = BTNS_BUTTON);
	bool InsertSeparator(int Index);
	bool AddSeparator();
	bool CheckItem(int Command, bool Check);
	bool EnableItem(int Command, bool Enable);
	bool GetMinSize(SIZE *pSize) const;

private:
	HIMAGELIST m_ImageList;
};

}	// namespace CV


#endif	// ndef CV_TOOLBAR_H
