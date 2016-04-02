/******************************************************************************
*                                                                             *
*    ToolBar.cpp                            Copyright(c) 2010-2016 itow,y.    *
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
#include "ToolBar.h"


namespace CV
{

ToolBar::ToolBar()
	: m_ImageList(nullptr)
{
}

ToolBar::~ToolBar()
{
	Destroy();

	if (m_ImageList != nullptr)
		ImageList_Destroy(m_ImageList);
}

bool ToolBar::Create(HWND Parent, int ID)
{
	Destroy();

	m_Handle = ::CreateWindowEx(
		0, TOOLBARCLASSNAME, nullptr,
		WS_CHILD | WS_CLIPSIBLINGS
			| CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN
			| TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT,
		0, 0, 0, 0,
		Parent,
		reinterpret_cast<HMENU>(ID),
		::GetModuleHandle(nullptr), nullptr);
	if (m_Handle == nullptr)
		return false;

	::SendMessage(m_Handle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	::SendMessage(m_Handle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

	if (m_ImageList != nullptr)
		::SendMessage(m_Handle, TB_SETIMAGELIST, 0,
					  reinterpret_cast<LPARAM>(m_ImageList));

	return true;
}

bool ToolBar::SetImageList(HIMAGELIST ImageList)
{
	if (ImageList == nullptr)
		return false;

	if (m_Handle != nullptr)
		::SendMessage(m_Handle, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ImageList));
	if (m_ImageList != nullptr)
		::ImageList_Destroy(m_ImageList);
	m_ImageList = ImageList;

	return true;
}

int ToolBar::NumItems() const
{
	if (m_Handle == nullptr)
		return 0;
	return (int)::SendMessage(m_Handle, TB_BUTTONCOUNT, 0, 0);
}

bool ToolBar::InsertItem(int Index, int Command, int Image, LPCTSTR pText, BYTE Style)
{
	if (m_Handle == nullptr)
		return false;

	TBBUTTON tbb;

	tbb.iBitmap = Image;
	tbb.idCommand = Command;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = Style;
	tbb.iString = reinterpret_cast<INT_PTR>(pText);
	return ::SendMessage(m_Handle, TB_INSERTBUTTON, Index, reinterpret_cast<LPARAM>(&tbb)) != FALSE;
}

bool ToolBar::AddItem(int Command, int Image, LPCTSTR pText, BYTE Style)
{
	return InsertItem(NumItems(), Command, Image, pText, Style);
}

bool ToolBar::InsertSeparator(int Index)
{
	return InsertItem(Index, 0, 0, nullptr, BTNS_SEP);
}

bool ToolBar::AddSeparator()
{
	return AddItem(0, 0, nullptr, BTNS_SEP);
}

bool ToolBar::CheckItem(int Command, bool Check)
{
	if (m_Handle == nullptr)
		return false;
	return ::SendMessage(m_Handle, TB_CHECKBUTTON, Command, Check) != FALSE;
}

bool ToolBar::EnableItem(int Command, bool Enable)
{
	if (m_Handle == nullptr)
		return false;
	return ::SendMessage(m_Handle, TB_ENABLEBUTTON, Command, Enable) != FALSE;
}

bool ToolBar::GetMinSize(SIZE *pSize) const
{
	pSize->cx = 0;
	pSize->cy = 0;
	if (m_Handle == nullptr)
		return false;
	int LastItem = NumItems() - 1;
	if (LastItem > 0) {
		RECT rc;
		if (!::SendMessage(m_Handle, TB_GETITEMRECT, LastItem, reinterpret_cast<LPARAM>(&rc)))
			return false;
		pSize->cx = rc.right;
		pSize->cy = rc.bottom;
	}
	return true;
}

}	// namespace CV
