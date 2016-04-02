/******************************************************************************
*                                                                             *
*    PropertyListView.cpp                   Copyright(c) 2010-2016 itow,y.    *
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
#include <algorithm>
#include "PropertyListView.h"
#include "resource.h"


namespace CV
{

PropertyListView::PropertyListView(const ProgramCore &Core)
	: m_Core(Core)
{
	m_Position.Height = 120;

	static const struct {
		int ID;
		ColumnAlign Align;
		bool Visible;
		int Width;
	} ColumnList[] = {
		{COLUMN_INDEX,	COLUMN_ALIGN_RIGHT,	true,	2},
		{COLUMN_NAME,	COLUMN_ALIGN_LEFT,	true,	12},
		{COLUMN_VALUE,	COLUMN_ALIGN_LEFT,	true,	30},
	};

	cvStaticAssert(cvLengthOf(ColumnList) == NUM_COLUMN_TYPES);

	LOGFONT lf;
	GetDefaultFont(&lf);
	const int FontHeight = max(abs(lf.lfHeight), 12);
	const int ItemMargin = m_ItemMargin.left + m_ItemMargin.right;

	m_ColumnList.reserve(cvLengthOf(ColumnList));
	for (int i = 0; i < cvLengthOf(ColumnList); i++) {
		ColumnInfo Column;

		Column.ID = ColumnList[i].ID;
		m_Core.LoadText(IDS_PROPERTYLIST_COLUMN_FIRST + Column.ID,
						Column.szText, cvLengthOf(Column.szText));
		Column.Align = ColumnList[i].Align;
		Column.Visible = ColumnList[i].Visible;
		Column.Width = ColumnList[i].Width * FontHeight + ItemMargin;
		m_ColumnList.push_back(Column);
	}

	m_SortOrder.resize(NUM_COLUMN_TYPES);
	for (int i = 0; i < NUM_COLUMN_TYPES; i++)
		m_SortOrder[i] = i;
}

PropertyListView::~PropertyListView()
{
	Destroy();
}

int PropertyListView::NumItems() const
{
	return (int)m_ItemList.size();
}

bool PropertyListView::GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = _T('\0');

	if (Row < 0 || (size_t)Row >= m_ItemList.size()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ItemInfo &Item = m_ItemList[Row];

	switch (Column) {
	case COLUMN_INDEX:
		FormatInt(Item.Index + 1, pText, MaxTextLength);
		break;

	case COLUMN_NAME:
		::lstrcpyn(pText, Item.szName, MaxTextLength);
		break;

	case COLUMN_VALUE:
		::lstrcpyn(pText, Item.szValue, MaxTextLength);
		break;
	}

	return true;
}

LPCTSTR PropertyListView::GetColumnIDName(int ID) const
{
	static const LPCTSTR ColumnNameList[] = {
		TEXT("Index"),
		TEXT("Name"),
		TEXT("Value"),
	};

	cvStaticAssert(cvLengthOf(ColumnNameList) == NUM_COLUMN_TYPES);

	if (ID < 0 || ID >= cvLengthOf(ColumnNameList))
		return nullptr;
	return ColumnNameList[ID];
}

void PropertyListView::SetItemList(const LPCTSTR *ppItemList, int NumItems)
{
	m_ItemList.resize(NumItems);
	for (int i = 0; i < NumItems; i++) {
		ItemInfo Item;

		Item.Index = i;
		::lstrcpyn(Item.szName, ppItemList[i], cvLengthOf(Item.szName));
		Item.szValue[0] = _T('\0');
		m_ItemList[i] = Item;
	}
	if (m_Handle != nullptr) {
		m_SelectedItem = -1;
		m_ScrollTop = 0;
		SetScrollBar();
		Redraw();
	}
}

bool PropertyListView::SetItemValue(int Index, LPCTSTR pValue)
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		ItemInfo &Item = m_ItemList[i];

		if (Item.Index == Index) {
			::lstrcpyn(Item.szValue, pValue, cvLengthOf(Item.szValue));
			if (m_Handle != nullptr) {
				RECT rc;
				GetItemRect((int)Index, &rc);
				::InvalidateRect(m_Handle, &rc, FALSE);
			}
			return true;
		}
	}
	return false;
}

class PropertyListItemCompare
{
	const std::vector<int> &m_SortOrder;
	bool m_Ascending;

public:
	PropertyListItemCompare(const std::vector<int> &SortOrder, bool Ascending)
		: m_SortOrder(SortOrder)
		, m_Ascending(Ascending)
	{
	}

	bool operator()(const PropertyListView::ItemInfo &Item1,
					const PropertyListView::ItemInfo &Item2) const
	{
		for (size_t i = 0; i < m_SortOrder.size(); i++) {
			int Cmp = 0;

			switch (m_SortOrder[i]) {
			case PropertyListView::COLUMN_INDEX:
				Cmp = Item1.Index - Item2.Index;
				break;

			case PropertyListView::COLUMN_NAME:
				Cmp = ::lstrcmp(Item1.szName, Item2.szName);
				break;

			case PropertyListView::COLUMN_VALUE:
				Cmp = ::lstrcmp(Item1.szValue, Item2.szValue);
				break;
			}

			if (Cmp != 0)
				return m_Ascending ? Cmp<0: Cmp>0;
		}
		return false;
	}
};

bool PropertyListView::SortItems()
{
	std::sort(m_ItemList.begin(), m_ItemList.end(),
			  PropertyListItemCompare(m_SortOrder, m_SortAscending));
	return true;
}

}	// namespace CV
