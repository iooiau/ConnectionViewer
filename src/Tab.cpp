/******************************************************************************
*                                                                             *
*    Tab.cpp                                Copyright(c) 2010-2016 itow,y.    *
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
#include "Tab.h"


namespace CV
{

Tab::Tab()
	: m_Font(nullptr)
	, m_ImageList(nullptr)
{
}

Tab::~Tab()
{
	Destroy();

	if (m_Font != nullptr)
		::DeleteObject(m_Font);

	if (m_ImageList != nullptr)
		::ImageList_Destroy(m_ImageList);
}

bool Tab::Create(HWND Parent, int ID)
{
	Destroy();

	m_Handle = ::CreateWindowEx(
		0, WC_TABCONTROL, nullptr,
		WS_CHILD | WS_CLIPSIBLINGS | TCS_HOTTRACK,
		0, 0, 0, 0,
		Parent,
		reinterpret_cast<HMENU>(ID),
		::GetModuleHandle(nullptr), nullptr);
	if (m_Handle == nullptr)
		return false;

	if (m_Font == nullptr) {
		NONCLIENTMETRICS ncm;

		ncm.cbSize = sizeof(ncm);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		ncm.lfCaptionFont.lfWeight = FW_NORMAL;
		m_Font = ::CreateFontIndirect(&ncm.lfCaptionFont);
	}
	::SendMessage(m_Handle, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), FALSE);

	if (m_ImageList != nullptr)
		TabCtrl_SetImageList(m_Handle, m_ImageList);

	return true;
}

bool Tab::InsertItem(int Index, LPCTSTR pText, int Image)
{
	if (m_Handle == nullptr)
		return false;

	if (Index < 0)
		Index = TabCtrl_GetItemCount(m_Handle);

	TCITEM tci;

	tci.mask = TCIF_TEXT | TCIF_IMAGE;
	tci.pszText = const_cast<LPTSTR>(pText);
	tci.iImage = Image;
	return TabCtrl_InsertItem(m_Handle, Index, &tci) >= 0;
}

bool Tab::DeleteItem(int Index)
{
	if (m_Handle == nullptr)
		return false;
	return TabCtrl_DeleteItem(m_Handle, Index) != FALSE;
}

void Tab::SetImageList(HIMAGELIST ImageList)
{
	if (m_Handle != nullptr)
		TabCtrl_SetImageList(m_Handle, ImageList);
	if (m_ImageList != nullptr)
		::ImageList_Destroy(m_ImageList);
	m_ImageList = ImageList;
}

bool Tab::SetImage(int Index, int Image)
{
	if (m_Handle == nullptr)
		return false;
	TCITEM tci;
	tci.mask = TCIF_IMAGE;
	tci.iImage = Image;
	return TabCtrl_SetItem(m_Handle, Index, &tci) != FALSE;
}

int Tab::GetTabHeight() const
{
	if (m_Handle == nullptr)
		return 0;
#if 0
	RECT rc = {0, 0, 0, 0};
	TabCtrl_AdjustRect(m_Handle, FALSE, &rc);
	return rc.top;
#else
	RECT rc;
	if (!TabCtrl_GetItemRect(m_Handle, 0, &rc))
		return 0;
	return rc.bottom;
#endif
}

void Tab::SetSel(int Index)
{
	if (m_Handle == nullptr)
		return;
	TabCtrl_SetCurSel(m_Handle, Index);
}

int Tab::GetSel() const
{
	if (m_Handle == nullptr)
		return -1;
	return TabCtrl_GetCurSel(m_Handle);
}

bool Tab::SetFixedWidth(bool Fixed)
{
	if (m_Handle == nullptr)
		return false;

	DWORD Style = GetStyle();

	if (Fixed) {
		if ((Style & TCS_FIXEDWIDTH) == 0) {
			const int ItemCount = TabCtrl_GetItemCount(m_Handle);

			if (ItemCount > 0) {
				int MaxWidth = 0, MaxHeight = 0;

				for (int i = 0; i < ItemCount; i++) {
					RECT rc;

					TabCtrl_GetItemRect(m_Handle, i, &rc);
					if (rc.right - rc.left > MaxWidth)
						MaxWidth = rc.right - rc.left;
					if (rc.bottom - rc.top > MaxHeight)
						MaxHeight = rc.bottom - rc.top;
				}
				SetStyle(Style | TCS_FIXEDWIDTH);
				TabCtrl_SetItemSize(m_Handle, MaxWidth, MaxHeight);
			} else {
				SetStyle(Style | TCS_FIXEDWIDTH);
			}
		}
	} else {
		if ((Style & TCS_FIXEDWIDTH) != 0)
			SetStyle(Style ^ TCS_FIXEDWIDTH);
	}
	return true;
}

}	// namespace CV
