/******************************************************************************
*                                                                             *
*    StatusBar.cpp                          Copyright(c) 2010-2016 itow,y.    *
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
#include "StatusBar.h"


namespace CV
{

StatusBar::StatusBar()
{
}

StatusBar::~StatusBar()
{
	Destroy();
}

bool StatusBar::Create(HWND Parent, int ID)
{
	Destroy();

	m_Handle = ::CreateWindowEx(
		0, STATUSCLASSNAME, nullptr,
		WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		Parent,
		reinterpret_cast<HMENU>(ID),
		::GetModuleHandle(nullptr), nullptr);
	return m_Handle != nullptr;
}

bool StatusBar::SetParts(const int *pWidths, int NumParts)
{
	if (m_Handle == nullptr || pWidths == nullptr || NumParts <= 0)
		return false;

	int *pList = new int[NumParts + 1];
	int Width = 0;
	for (int i = 0; i < NumParts; i++) {
		Width += pWidths[i];
		pList[i] = Width;
	}
	pList[NumParts] = -1;
	::SendMessage(m_Handle, SB_SETPARTS, NumParts, reinterpret_cast<LPARAM>(pList));
	::SendMessage(m_Handle, SB_SETTEXT, NumParts | SBT_NOBORDERS, reinterpret_cast<LPARAM>(TEXT("")));
	delete [] pList;
	return true;
}

bool StatusBar::SetPartText(int Part, LPCTSTR pText)
{
	if (m_Handle == nullptr)
		return false;
	return ::SendMessage(m_Handle, SB_SETTEXT, Part, reinterpret_cast<LPARAM>(pText)) != FALSE;
}

}	// namespace CV
