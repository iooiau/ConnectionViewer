/******************************************************************************
*                                                                             *
*    BlockListView.cpp                      Copyright(c) 2010-2016 itow,y.    *
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
#include "BlockListView.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

BlockListView::BlockListView(const ProgramCore &Core)
	: m_Core(Core)
{
	static const struct {
		int ID;
		ColumnAlign Align;
		bool Visible;
		int Width;
	} DefaultColumnList[] = {
		{COLUMN_STATUS,					COLUMN_ALIGN_LEFT,		true,	3},
		{COLUMN_REMOTE_ADDRESS,			COLUMN_ALIGN_CENTER,	true,	8},
		{COLUMN_REMOTE_HOST,			COLUMN_ALIGN_LEFT,		true,	14},
		{COLUMN_COUNTRY,				COLUMN_ALIGN_LEFT,		true,	2},
		{COLUMN_CITY,					COLUMN_ALIGN_LEFT,		true,	8},
		{COLUMN_LOCATION,				COLUMN_ALIGN_LEFT,		false,	8},
		{COLUMN_ADDED_TIME,				COLUMN_ALIGN_LEFT,		true,	10},
		{COLUMN_COMMENT,				COLUMN_ALIGN_LEFT,		true,	20},
	};

	cvStaticAssert(cvLengthOf(DefaultColumnList) == NUM_COLUMN_TYPES);

	LOGFONT lf;
	GetDefaultFont(&lf);
	const int FontHeight = max(abs(lf.lfHeight), 12);
	const int ItemMargin = m_ItemMargin.left + m_ItemMargin.right;

	m_ColumnList.reserve(cvLengthOf(DefaultColumnList));
	for (int i = 0; i < cvLengthOf(DefaultColumnList); i++) {
		ColumnInfo Column;

		Column.ID = DefaultColumnList[i].ID;
		m_Core.LoadText(IDS_BLOCKLIST_COLUMN_FIRST + Column.ID,
						Column.szText, cvLengthOf(Column.szText));
		Column.Align = DefaultColumnList[i].Align;
		Column.Visible = DefaultColumnList[i].Visible;
		Column.Width = DefaultColumnList[i].Width * FontHeight + ItemMargin;
		m_ColumnList.push_back(Column);
	}

	m_SortOrder.reserve(NUM_COLUMN_TYPES);
	m_SortOrder.push_back(COLUMN_REMOTE_ADDRESS);
	for (int i = 1; i < NUM_COLUMN_TYPES; i++) {
		if (DefaultColumnList[i].ID != COLUMN_REMOTE_ADDRESS)
			m_SortOrder.push_back(DefaultColumnList[i].ID);
	}
}

BlockListView::~BlockListView()
{
}

void BlockListView::OnListUpdated()
{
	const FilterManager &Manager = m_Core.GetFilterManager();
	const int NumFilters = Manager.NumFilters();

	std::vector<ItemInfo> NewList;
	NewList.reserve(NumFilters);

	for (int i = 0; i < NumFilters; i++) {
		ItemInfo NewItem;

		NewItem.Index = i;
		NewItem.Selected = false;
		Manager.GetFilter(i, &NewItem.Info);

		bool Exist = false;
		for (size_t j = 0; j < m_ItemList.size(); j++) {
			ItemInfo &OldItem = m_ItemList[j];

			if (OldItem.Info.ID == NewItem.Info.ID) {
				OldItem.Index = NewItem.Index;
				OldItem.Info = NewItem.Info;
				NewList.push_back(OldItem);
				Exist = true;
				break;
			}
		}

		if (!Exist) {
			NewItem.EnableCityInfo =
				m_Core.GetGeoIPCityInfo(NewItem.Info.Address, &NewItem.CityInfo);

			NewList.push_back(NewItem);
		}
	}

	m_ItemList.swap(NewList);
	SortItems();

	SetScrollBar();
	AdjustScrollPos(false);
	Redraw();
}

int BlockListView::NumItems() const
{
	return (int)m_ItemList.size();
}

bool BlockListView::GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = '\0';

	if (Row < 0 || Row >= NumItems()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ItemInfo &Item = m_ItemList[Row];

	switch (Column) {
	case COLUMN_STATUS:
		m_Core.LoadText(Item.Info.Enable ? IDS_BLOCK_STATE_ENABLED : IDS_BLOCK_STATE_DISABLED,
						pText, MaxTextLength);
		break;

	case COLUMN_REMOTE_ADDRESS:
		if (Item.Info.Match == FilterInfo::MATCH_EQUAL) {
			FormatIPAddress(Item.Info.Address, pText, MaxTextLength);
		} else if (Item.Info.Match == FilterInfo::MATCH_RANGE) {
			TCHAR szAddressLow[64], szAddressHigh[64];

			FormatIPAddress(Item.Info.AddressRange.Low, szAddressLow, cvLengthOf(szAddressLow));
			FormatIPAddress(Item.Info.AddressRange.High, szAddressHigh, cvLengthOf(szAddressHigh));
			FormatString(pText, MaxTextLength, TEXT("%s - %s"),
						 szAddressLow, szAddressHigh);
		}
		break;

	case COLUMN_REMOTE_HOST:
		::lstrcpyn(pText, Item.Info.HostName, MaxTextLength);
		break;

	case COLUMN_COUNTRY:
		if (Item.EnableCityInfo)
			::lstrcpyn(pText, Item.CityInfo.Country.Code2, MaxTextLength);
		break;

	case COLUMN_CITY:
		if (Item.EnableCityInfo)
			//FormatString(pText,MaxTextLength,TEXT("%s %s"),
			//			 Item.CityInfo.Region,Item.CityInfo.City);
			::lstrcpyn(pText, Item.CityInfo.City, MaxTextLength);
		break;

	case COLUMN_LOCATION:
		if (Item.EnableCityInfo && Item.CityInfo.EnableLocation)
			FormatString(pText, MaxTextLength, TEXT("%.3f %.3f"),
						 Item.CityInfo.Latitude,
						 Item.CityInfo.Longitude);
		break;

	case COLUMN_ADDED_TIME:
		{
			SYSTEMTIME stUTC, stLocal;

			if (::FileTimeToSystemTime(&Item.Info.AddedTime, &stUTC)
					&& ::SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal))
				FormatSystemTime(stLocal, SYSTEMTIME_FORMAT_TIME,
								 pText, MaxTextLength);
		}
		break;

	case COLUMN_COMMENT:
		::lstrcpyn(pText, Item.Info.Comment, MaxTextLength);
		break;

	default:
		cvDebugBreak();
		return false;
	}

	return true;
}

bool BlockListView::GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = _T('\0');

	if (Row < 0 || Row >= NumItems()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ItemInfo &Item = m_ItemList[Row];

	switch (Column) {
	case COLUMN_COUNTRY:
		if (Item.EnableCityInfo)
			FormatString(pText, MaxTextLength, TEXT("%s (%s)"),
						 Item.CityInfo.Country.Code2,
						 Item.CityInfo.Country.Name);
		break;

	case COLUMN_LOCATION:
		if (Item.EnableCityInfo && Item.CityInfo.EnableLocation)
			FormatString(pText, MaxTextLength, TEXT("%.5f %.5f"),
						 Item.CityInfo.Latitude,
						 Item.CityInfo.Longitude);
		break;

	default:
		return GetItemText(Row, Column, pText, MaxTextLength);
	}

	return true;
}

LPCTSTR BlockListView::GetColumnIDName(int ID) const
{
	static const LPCTSTR ColumnNameList[] = {
		TEXT("Status"),
		TEXT("RemoteAddress"),
		TEXT("RemoteHost"),
		TEXT("Country"),
		TEXT("City"),
		TEXT("Location"),
		TEXT("AddedTime"),
		TEXT("Comment"),
	};

	cvStaticAssert(cvLengthOf(ColumnNameList) == NUM_COLUMN_TYPES);

	if (ID < 0 || ID >= cvLengthOf(ColumnNameList))
		return nullptr;
	return ColumnNameList[ID];
}

int BlockListView::GetItemFilterIndex(int Item) const
{
	if (Item < 0 || (size_t)Item >= m_ItemList.size())
		return -1;
	return m_ItemList[Item].Index;
}

int BlockListView::FilterIDToIndex(UINT ID) const
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i].Info.ID == ID)
			return (int)i;
	}
	return -1;
}

void BlockListView::DrawItem(HDC hdc, int Row, const ListView::ColumnInfo &Column,
							 const RECT &rcBound, const RECT &rcItem)
{
	RECT rcDraw = rcItem;
	TCHAR szText[MAX_ITEM_TEXT];

	GetItemText(Row, Column.ID, szText, cvLengthOf(szText));
	if (szText[0] != _T('\0')) {
		::DrawText(hdc, szText, -1, &rcDraw,
				   GetDrawTextAlignFlag(Column.Align)
				   | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}

bool BlockListView::OnSelChange(int OldSel, int NewSel)
{
	if (OldSel >= 0)
		m_ItemList[OldSel].Selected = false;
	if (NewSel >= 0)
		m_ItemList[NewSel].Selected = true;
	return true;
}

template<typename T> int CompareValue(T Value1, T Value2)
{
	return Value1 < Value2 ? -1 : Value1 > Value2 ? 1 : 0;
}

class BlockItemCompare
{
	const std::vector<int> &m_SortOrder;
	const bool m_Ascending;

public:
	BlockItemCompare(const std::vector<int> &SortOrder, bool Ascending)
		: m_SortOrder(SortOrder)
		, m_Ascending(Ascending)
	{
	}

	bool operator()(const BlockListView::ItemInfo &Item1,
					const BlockListView::ItemInfo &Item2) const
	{
		for (size_t i = 0; i < m_SortOrder.size(); i++) {
			int Cmp = 0;

			switch (m_SortOrder[i]) {
			case BlockListView::COLUMN_STATUS:
				if (Item1.Info.Enable) {
					if (!Item2.Info.Enable)
						Cmp = -1;
				} else if (Item2.Info.Enable)
					Cmp = 1;
				break;

			case BlockListView::COLUMN_REMOTE_ADDRESS:
				if (Item1.Info.Match == Item2.Info.Match) {
					if (Item1.Info.Match == FilterInfo::MATCH_EQUAL) {
						Cmp = CompareValue(Item1.Info.Address, Item2.Info.Address);
					} else if (Item1.Info.Match == FilterInfo::MATCH_RANGE) {
						Cmp = CompareValue(Item1.Info.AddressRange.Low,
										   Item2.Info.AddressRange.Low);
						if (Cmp == 0)
							Cmp = CompareValue(Item1.Info.AddressRange.High,
											   Item2.Info.AddressRange.High);
					}
				} else {
					const IPAddress *pAddress1, *pAddress2;

					if (Item1.Info.Match == FilterInfo::MATCH_EQUAL)
						pAddress1 = &Item1.Info.Address;
					else if (Item1.Info.Match == FilterInfo::MATCH_RANGE)
						pAddress1 = &Item1.Info.AddressRange.Low;
					else
						pAddress1 = nullptr;
					if (Item2.Info.Match == FilterInfo::MATCH_EQUAL)
						pAddress2 = &Item2.Info.Address;
					else if (Item2.Info.Match == FilterInfo::MATCH_RANGE)
						pAddress2 = &Item2.Info.AddressRange.Low;
					else
						pAddress2 = nullptr;
					if (pAddress1 != nullptr && pAddress2 != nullptr)
						Cmp = CompareValue(*pAddress1, *pAddress2);
					if (Cmp == 0)
						Cmp = CompareValue(Item1.Info.Match, Item2.Info.Match);
				}
				break;

			case BlockListView::COLUMN_REMOTE_HOST:
				if (Item1.Info.HostName[0] != _T('\0')) {
					if (Item2.Info.HostName[0] != _T('\0'))
						Cmp = ::lstrcmpi(Item1.Info.HostName, Item2.Info.HostName);
					else
						Cmp = -1;
				} else if (Item2.Info.HostName[0] != _T('\0'))
					Cmp = 1;
				break;

			case BlockListView::COLUMN_COUNTRY:
				if (Item1.EnableCityInfo) {
					if (Item2.EnableCityInfo)
						Cmp = ::lstrcmpi(Item1.CityInfo.Country.Code2,
										 Item2.CityInfo.Country.Code2);
					else
						Cmp = -1;
				} else if (Item2.EnableCityInfo)
					Cmp = 1;
				break;

			case BlockListView::COLUMN_CITY:
				if (Item1.EnableCityInfo && Item1.CityInfo.City[0] != _T('\0')) {
					if (Item2.EnableCityInfo && Item2.CityInfo.City[0] != _T('\0')) {
						/*
						Cmp= ::lstrcmpi(Item1.CityInfo.Region,
										Item2.CityInfo.Region);
						if (Cmp==0)
						*/
						Cmp = ::lstrcmpi(Item1.CityInfo.City,
										 Item2.CityInfo.City);
					} else
						Cmp = -1;
				} else if (Item2.EnableCityInfo && Item2.CityInfo.City[0] != _T('\0'))
					Cmp = 1;
				break;

			case BlockListView::COLUMN_LOCATION:
				if (Item1.EnableCityInfo && Item1.CityInfo.EnableLocation) {
					if (Item2.EnableCityInfo && Item2.CityInfo.EnableLocation) {
						Cmp = CompareValue(Item1.CityInfo.Latitude,
										   Item2.CityInfo.Latitude);
						if (Cmp == 0)
							Cmp = CompareValue(Item1.CityInfo.Longitude,
											   Item2.CityInfo.Longitude);
					} else
						Cmp = -1;
				} else if (Item2.EnableCityInfo && Item2.CityInfo.EnableLocation)
					Cmp = 1;
				break;

			case BlockListView::COLUMN_ADDED_TIME:
				Cmp = CompareValue(Item1.Info.AddedTime.dwHighDateTime,
								   Item2.Info.AddedTime.dwHighDateTime);
				if (Cmp == 0)
					Cmp = CompareValue(Item1.Info.AddedTime.dwLowDateTime,
									   Item2.Info.AddedTime.dwLowDateTime);
				break;

			case BlockListView::COLUMN_COMMENT:
				if (Item1.Info.Comment[0] != _T('\0')) {
					if (Item2.Info.Comment[0] != _T('\0'))
						Cmp = ::lstrcmpi(Item1.Info.Comment, Item2.Info.Comment);
					else
						Cmp = -1;
				} else if (Item2.Info.Comment[0] != _T('\0'))
					Cmp = 1;
				break;
			}

			if (Cmp != 0)
				return m_Ascending ? Cmp<0: Cmp>0;
		}
		return false;
	}
};

bool BlockListView::SortItems()
{
	std::sort(m_ItemList.begin(), m_ItemList.end(),
			  BlockItemCompare(m_SortOrder, m_SortAscending));

	m_SelectedItem = -1;
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i].Selected) {
			m_SelectedItem = (int)i;
			break;
		}
	}

	return true;
}

}	// namespace CV
