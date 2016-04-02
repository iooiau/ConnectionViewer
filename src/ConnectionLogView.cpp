/******************************************************************************
*                                                                             *
*    ConnectionLogView.cpp                  Copyright(c) 2010-2016 itow,y.    *
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
#include "ConnectionLogView.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

LPCTSTR ConnectionLogView::GetColumnName(int Column)
{
	static const LPCTSTR ColumnNameList[] = {
		TEXT("CreateTime"),
		TEXT("UpdateTime"),
		TEXT("Process"),
		TEXT("Path"),
		TEXT("PID"),
		TEXT("Protocol"),
		TEXT("LocalAddress"),
		TEXT("LocalPort"),
		TEXT("RemoteAddress"),
		TEXT("RemotePort"),
		TEXT("RemoteHost"),
		TEXT("Country"),
		TEXT("City"),
		TEXT("Location"),
		TEXT("State"),
		TEXT("Duration"),
		TEXT("InBytes"),
		TEXT("OutBytes"),
		TEXT("InBandwidth"),
		TEXT("OutBandwidth"),
		TEXT("MaxInBandwidth"),
		TEXT("MaxOutBandwidth"),
	};

	cvStaticAssert(cvLengthOf(ColumnNameList) == NUM_COLUMN_TYPES);

	if (Column < 0 || Column >= cvLengthOf(ColumnNameList))
		return nullptr;
	return ColumnNameList[Column];
}

ConnectionLogView::ConnectionLogView(const ProgramCore &Core, const ConnectionLog &Log)
	: m_Core(Core)
	, m_Log(Log)
	, m_NewItemBackColor(RGB(255, 255, 224))
{
	m_ScrollBeyondBottom = true;

	static const struct {
		int ID;
		ColumnAlign Align;
		bool Visible;
		int Width;
	} DefaultColumnList[] = {
		{COLUMN_CREATED_TIME,			COLUMN_ALIGN_LEFT,		true,	10},
		{COLUMN_UPDATED_TIME,			COLUMN_ALIGN_LEFT,		true,	10},
		{COLUMN_PROCESS_NAME,			COLUMN_ALIGN_LEFT,		true,	10},
		{COLUMN_PROCESS_ID,				COLUMN_ALIGN_RIGHT,		true,	3},
		{COLUMN_PROTOCOL,				COLUMN_ALIGN_LEFT,		true,	4},
		{COLUMN_LOCAL_ADDRESS,			COLUMN_ALIGN_CENTER,	true,	8},
		{COLUMN_LOCAL_PORT,				COLUMN_ALIGN_RIGHT,		true,	3},
		{COLUMN_REMOTE_ADDRESS,			COLUMN_ALIGN_CENTER,	true,	8},
		{COLUMN_REMOTE_PORT,			COLUMN_ALIGN_RIGHT,		true,	3},
		{COLUMN_REMOTE_HOST,			COLUMN_ALIGN_LEFT,		true,	14},
		{COLUMN_COUNTRY,				COLUMN_ALIGN_LEFT,		true,	2},
		{COLUMN_CITY,					COLUMN_ALIGN_LEFT,		true,	8},
		{COLUMN_LOCATION,				COLUMN_ALIGN_LEFT,		false,	8},
		{COLUMN_STATE,					COLUMN_ALIGN_LEFT,		true,	8},
		{COLUMN_DURATION,				COLUMN_ALIGN_LEFT,		true,	5},
		{COLUMN_IN_BANDWIDTH,			COLUMN_ALIGN_RIGHT,		true,	5},
		{COLUMN_MAX_IN_BANDWIDTH,		COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_BANDWIDTH,			COLUMN_ALIGN_RIGHT,		true,	5},
		{COLUMN_MAX_OUT_BANDWIDTH,		COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_BYTES,				COLUMN_ALIGN_RIGHT,		true,	7},
		{COLUMN_OUT_BYTES,				COLUMN_ALIGN_RIGHT,		true,	7},
		{COLUMN_PROCESS_PATH,			COLUMN_ALIGN_LEFT,		true,	12},
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
		m_Core.LoadText(IDS_CONNECTIONLOG_COLUMN_FIRST + Column.ID,
						Column.szText, cvLengthOf(Column.szText));
		Column.Align = DefaultColumnList[i].Align;
		Column.Visible = DefaultColumnList[i].Visible;
		Column.Width = DefaultColumnList[i].Width * FontHeight + ItemMargin;
		m_ColumnList.push_back(Column);
	}

	m_SortOrder.reserve(NUM_COLUMN_TYPES);
	m_SortOrder.push_back(COLUMN_UPDATED_TIME);
	for (int i = 0; i < NUM_COLUMN_TYPES; i++) {
		if (DefaultColumnList[i].ID != COLUMN_UPDATED_TIME)
			m_SortOrder.push_back(DefaultColumnList[i].ID);
	}
	m_SortAscending = false;
}

ConnectionLogView::~ConnectionLogView()
{
	Destroy();
}

void ConnectionLogView::OnListUpdated()
{
	ULONGLONG SelectedID = 0;
	if (m_SelectedItem >= 0) {
		for (size_t i = 0; i < m_ItemList.size(); i++) {
			if ((m_ItemList[i].Flags & ItemInfo::FLAG_SELECTED) != 0) {
				SelectedID = m_ItemList[i].ID;
				break;
			}
		}
	}

	const ULONGLONG UpdatedTime = m_Log.GetUpdatedTickCount();
	const bool NewFlag = m_ItemList.size() > 0;
	m_ItemList.clear();
	m_ItemList.reserve(m_Log.NumItems());
	const ConnectionLog::ItemList &List = m_Log.GetItemList();
	for (ConnectionLog::ItemList::const_iterator itr = List.begin(); itr != List.end(); itr++) {
		ItemInfo Item;

		Item.Flags = 0;
		Item.ID = itr->ID;
		if (itr->CreatedTime.Tick == UpdatedTime) {
			if (NewFlag)
				Item.Flags |= ItemInfo::FLAG_NEW;
		} else {
			if (Item.ID == SelectedID)
				Item.Flags |= ItemInfo::FLAG_SELECTED;
		}
		Item.Iterator = itr;
		m_ItemList.push_back(Item);
	}

	SortItems();

	SetScrollBar();
	AdjustScrollPos(false);
	Redraw();
}

int ConnectionLogView::NumItems() const
{
	return (int)m_ItemList.size();
}

static BYTE ClampByte(int Value)
{
	return Value < 0 ? 0 : Value > 255 ? 255 : (BYTE)Value;
}

bool ConnectionLogView::DrawItemBackground(HDC hdc, int Row, const RECT &rcBound)
{
	const ItemInfo &Item = m_ItemList[Row];

	if ((Item.Flags & ItemInfo::FLAG_NEW) != 0) {
		COLORREF Color = m_NewItemBackColor;
		if (Row % 2 != 0) {
			Color = RGB(
				ClampByte(GetRValue(Color) +
						  (GetRValue(m_ItemBackColor[1]) - GetRValue(m_ItemBackColor[0]))),
				ClampByte(GetGValue(Color) +
						  (GetGValue(m_ItemBackColor[1]) - GetGValue(m_ItemBackColor[0]))),
				ClampByte(GetBValue(Color) +
						  (GetBValue(m_ItemBackColor[1]) - GetBValue(m_ItemBackColor[0]))));
		}
		HBRUSH Brush = ::CreateSolidBrush(Color);
		::FillRect(hdc, &rcBound, Brush);
		::DeleteObject(Brush);
		return true;
	}
	return false;
}

bool ConnectionLogView::GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = '\0';

	if (Row < 0 || (size_t)Row >= m_ItemList.size()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ConnectionLog::ItemInfo &Item = *m_ItemList[Row].Iterator;

	switch (Column) {
	case COLUMN_CREATED_TIME:
		{
			SYSTEMTIME stUTC, stLocal;

			if (::FileTimeToSystemTime(&Item.CreatedTime.Time, &stUTC)
					&& ::SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal))
				FormatSystemTime(stLocal, SYSTEMTIME_FORMAT_TIME | SYSTEMTIME_FORMAT_SECONDS,
								 pText, MaxTextLength);
		}
		break;

	case COLUMN_UPDATED_TIME:
		{
			SYSTEMTIME stUTC, stLocal;

			if (::FileTimeToSystemTime(&Item.UpdatedTime.Time, &stUTC)
					&& ::SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal))
				FormatSystemTime(stLocal, SYSTEMTIME_FORMAT_TIME | SYSTEMTIME_FORMAT_SECONDS,
								 pText, MaxTextLength);
		}
		break;

	case COLUMN_PROCESS_NAME:
		if (Item.pProcessName != nullptr)
			::lstrcpyn(pText, Item.pProcessName, MaxTextLength);
		break;

	case COLUMN_PROCESS_PATH:
		if (Item.pProcessPath != nullptr)
			::lstrcpyn(pText, Item.pProcessPath, MaxTextLength);
		break;

	case COLUMN_PROCESS_ID:
		UIntToStr(Item.Info.PID, pText, MaxTextLength);
		break;

	case COLUMN_PROTOCOL:
		::lstrcpyn(pText, GetProtocolText(Item.Info.Protocol), MaxTextLength);
		break;

	case COLUMN_LOCAL_ADDRESS:
		FormatIPAddress(Item.Info.LocalAddress, pText, MaxTextLength);
		break;

	case COLUMN_LOCAL_PORT:
		UIntToStr(Item.Info.LocalPort, pText, MaxTextLength);
		break;

	case COLUMN_REMOTE_ADDRESS:
		FormatIPAddress(Item.Info.RemoteAddress, pText, MaxTextLength);
		break;

	case COLUMN_REMOTE_PORT:
		UIntToStr(Item.Info.RemotePort, pText, MaxTextLength);
		break;

	case COLUMN_REMOTE_HOST:
		if (Item.pRemoteHostName != nullptr)
			::lstrcpyn(pText, Item.pRemoteHostName, MaxTextLength);
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

	case COLUMN_STATE:
		::lstrcpyn(pText, GetConnectionStateText(Item.Info.State), MaxTextLength);
		break;

	case COLUMN_DURATION:
		if (Item.Info.CreateTimestamp > 0
				&& (LONGLONG)Item.CreatedTime.Tick >= Item.ConnectionCreateTickCount) {
			const UINT ConnectionSec =
				(UINT)((Item.UpdatedTime.Tick - Item.ConnectionCreateTickCount) / 1000);
			FormatString(pText, MaxTextLength, TEXT("%02u:%02u:%02u"),
						 ConnectionSec / (60 * 60), (ConnectionSec / 60) % 60, ConnectionSec % 60);
		} else {
			const UINT ConnectionSec = (UINT)((Item.UpdatedTime.Tick - Item.CreatedTime.Tick) / 1000);
			FormatString(pText, MaxTextLength, TEXT("%02u:%02u:%02u+"),
						 ConnectionSec / (60 * 60), (ConnectionSec / 60) % 60, ConnectionSec % 60);
		}
		break;

	case COLUMN_IN_BYTES:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
			FormatUInt64(Item.Statistics.InBytes, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BYTES:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
			FormatUInt64(Item.Statistics.OutBytes, pText, MaxTextLength);
		break;

	case COLUMN_IN_BANDWIDTH:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
			FormatUInt64(Item.Statistics.InBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BANDWIDTH:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
			FormatUInt64(Item.Statistics.OutBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_MAX_IN_BANDWIDTH:
		if (Item.MaxInBitsPerSecond >= 0)
			FormatInt64(Item.MaxInBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_MAX_OUT_BANDWIDTH:
		if (Item.MaxOutBitsPerSecond >= 0)
			FormatInt64(Item.MaxOutBitsPerSecond / 8, pText, MaxTextLength);
		break;
	}

	return true;
}

bool ConnectionLogView::GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = '\0';

	if (Row < 0 || (size_t)Row >= m_ItemList.size()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ConnectionLog::ItemInfo &Item = *m_ItemList[Row].Iterator;

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

	case COLUMN_IN_BYTES:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
			FormatBytesLong(Item.Statistics.InBytes, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BYTES:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
			FormatBytesLong(Item.Statistics.OutBytes, pText, MaxTextLength);
		break;

	case COLUMN_IN_BANDWIDTH:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
			FormatBandwidthLong(Item.Statistics.InBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BANDWIDTH:
		if (Item.EnableStatistics
				&& (Item.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
			FormatBandwidthLong(Item.Statistics.OutBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_MAX_IN_BANDWIDTH:
		if (Item.MaxInBitsPerSecond >= 0)
			FormatBandwidthLong(Item.MaxInBitsPerSecond / 8, pText, MaxTextLength);
		break;

	case COLUMN_MAX_OUT_BANDWIDTH:
		if (Item.MaxOutBitsPerSecond >= 0)
			FormatBandwidthLong(Item.MaxOutBitsPerSecond / 8, pText, MaxTextLength);
		break;

	default:
		return GetItemText(Row, Column, pText, MaxTextLength);
	}

	return true;
}

LPCTSTR ConnectionLogView::GetColumnIDName(int ID) const
{
	return GetColumnName(ID);
}

void ConnectionLogView::SetSpecialColors(COLORREF NewItemBackColor)
{
	m_NewItemBackColor = NewItemBackColor;
	if (m_Handle != nullptr)
		Redraw();
}

bool ConnectionLogView::GetItemConnectionInfo(int Item, ConnectionInfo *pInfo) const
{
	if (Item < 0 || (size_t)Item >= m_ItemList.size())
		return false;
	*pInfo = m_ItemList[Item].Iterator->Info;
	return true;
}

bool ConnectionLogView::OnSelChange(int OldSel, int NewSel)
{
	if (OldSel >= 0)
		m_ItemList[OldSel].Flags &= ~ItemInfo::FLAG_SELECTED;
	if (NewSel >= 0)
		m_ItemList[NewSel].Flags |= ItemInfo::FLAG_SELECTED;
	return true;
}

template<typename T> int CompareValue(T Value1, T Value2)
{
	return Value1 < Value2 ? -1 : Value1 > Value2 ? 1 : 0;
}

class LogItemCompare
{
	const std::vector<int> &m_SortOrder;
	bool m_Ascending;

public:
	LogItemCompare(const std::vector<int> &SortOrder, bool Ascending)
		: m_SortOrder(SortOrder)
		, m_Ascending(Ascending)
	{
	}

	bool operator()(const ConnectionLogView::ItemInfo &ViewItem1,
					const ConnectionLogView::ItemInfo &ViewItem2) const
	{
		const ConnectionLog::ItemInfo &Item1 = *ViewItem1.Iterator;
		const ConnectionLog::ItemInfo &Item2 = *ViewItem2.Iterator;

		for (size_t i = 0; i < m_SortOrder.size(); i++) {
			int Cmp = 0;

			switch (m_SortOrder[i]) {
			case ConnectionLogView::COLUMN_CREATED_TIME:
				Cmp = CompareValue(Item1.CreatedTime.Tick, Item2.CreatedTime.Tick);
				break;

			case ConnectionLogView::COLUMN_UPDATED_TIME:
				Cmp = CompareValue(Item1.UpdatedTime.Tick, Item2.UpdatedTime.Tick);
				break;

			case ConnectionLogView::COLUMN_PROCESS_NAME:
				if (Item1.pProcessName != nullptr) {
					if (Item2.pProcessName != nullptr)
						Cmp = ::lstrcmpi(Item1.pProcessName, Item2.pProcessName);
					else
						Cmp = -1;
				} else if (Item2.pProcessName != nullptr)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_PROCESS_PATH:
				if (Item1.pProcessPath != nullptr) {
					if (Item2.pProcessPath != nullptr)
						Cmp = ::lstrcmpi(Item1.pProcessPath, Item2.pProcessPath);
					else
						Cmp = -1;
				} else if (Item2.pProcessPath != nullptr)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_PROCESS_ID:
				Cmp = CompareValue(Item1.Info.PID, Item2.Info.PID);
				break;

			case ConnectionLogView::COLUMN_PROTOCOL:
				Cmp = CompareValue(Item1.Info.Protocol, Item2.Info.Protocol);
				break;

			case ConnectionLogView::COLUMN_LOCAL_ADDRESS:
				if (Item1.Info.LocalAddress < Item2.Info.LocalAddress)
					Cmp = -1;
				else if (Item1.Info.LocalAddress > Item2.Info.LocalAddress)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_LOCAL_PORT:
				Cmp = CompareValue(Item1.Info.LocalPort, Item2.Info.LocalPort);
				break;

			case ConnectionLogView::COLUMN_REMOTE_ADDRESS:
				if (Item1.Info.RemoteAddress < Item2.Info.RemoteAddress)
					Cmp = -1;
				else if (Item1.Info.RemoteAddress > Item2.Info.RemoteAddress)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_REMOTE_PORT:
				Cmp = CompareValue(Item1.Info.RemotePort, Item2.Info.RemotePort);
				break;

			case ConnectionLogView::COLUMN_REMOTE_HOST:
				if (Item1.pRemoteHostName != nullptr) {
					if (Item2.pRemoteHostName != nullptr)
						Cmp = ::lstrcmpi(Item1.pRemoteHostName, Item2.pRemoteHostName);
					else
						Cmp = -1;
				} else if (Item2.pRemoteHostName != nullptr)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_COUNTRY:
				if (Item1.EnableCityInfo) {
					if (Item2.EnableCityInfo)
						Cmp = ::lstrcmpi(Item1.CityInfo.Country.Code2,
										Item2.CityInfo.Country.Code2);
					else
						Cmp = -1;
				} else if (Item2.EnableCityInfo)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_CITY:
				if (Item1.EnableCityInfo && Item1.CityInfo.City[0] != '\0') {
					if (Item2.EnableCityInfo && Item2.CityInfo.City[0] != '\0') {
						/*
						Cmp= ::lstrcmpi(Item1.CityInfo.Region,
									   Item2.CityInfo.Region);
						if (Cmp==0)
						*/
						Cmp = ::lstrcmpi(Item1.CityInfo.City,
										Item2.CityInfo.City);
					} else
						Cmp = -1;
				} else if (Item2.EnableCityInfo && Item2.CityInfo.City[0] != '\0')
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_LOCATION:
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

			case ConnectionLogView::COLUMN_STATE:
				if (Item1.Info.State != ConnectionState::UNDEFINED) {
					if (Item2.Info.State != ConnectionState::UNDEFINED)
						Cmp = CompareValue(Item1.Info.State, Item2.Info.State);
					else
						Cmp = -1;
				} else if (Item2.Info.State != ConnectionState::UNDEFINED)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_DURATION:
				{
					ULONGLONG Duration1, Duration2;

					Duration1 = Item1.UpdatedTime.Tick - Item1.CreatedTime.Tick;
					if (Item1.Info.CreateTimestamp > 0
							&& (LONGLONG)Item1.CreatedTime.Tick >= Item1.ConnectionCreateTickCount)
						Duration1 = Item1.UpdatedTime.Tick - Item1.ConnectionCreateTickCount;
					Duration2 = Item2.UpdatedTime.Tick - Item2.CreatedTime.Tick;
					if (Item2.Info.CreateTimestamp > 0
							&& (LONGLONG)Item2.CreatedTime.Tick >= Item2.ConnectionCreateTickCount)
						Duration2 = Item2.UpdatedTime.Tick - Item2.ConnectionCreateTickCount;
					Cmp = CompareValue(Duration1, Duration2);
				}
				break;

			case ConnectionLogView::COLUMN_IN_BYTES:
				if (Item1.EnableStatistics
						&& (Item1.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0) {
					if (Item2.EnableStatistics
							&& (Item2.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
						Cmp = CompareValue(Item1.Statistics.InBytes,
										   Item2.Statistics.InBytes);
					else
						Cmp = -1;
				} else if (Item2.EnableStatistics
						   && (Item2.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_OUT_BYTES:
				if (Item1.EnableStatistics
						&& (Item1.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0) {
					if (Item2.EnableStatistics
							&& (Item2.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
						Cmp = CompareValue(Item1.Statistics.OutBytes,
										   Item2.Statistics.OutBytes);
					else
						Cmp = -1;
				} else if (Item2.EnableStatistics
						   && (Item2.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_IN_BANDWIDTH:
				if (Item1.EnableStatistics
						&& (Item1.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0) {
					if (Item2.EnableStatistics
							&& (Item2.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
						Cmp = CompareValue(Item1.Statistics.InBitsPerSecond,
										   Item2.Statistics.InBitsPerSecond);
					else
						Cmp = -1;
				} else if (Item2.EnableStatistics
						   && (Item2.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_OUT_BANDWIDTH:
				if (Item1.EnableStatistics
						&& (Item1.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0) {
					if (Item2.EnableStatistics
							&& (Item2.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
						Cmp = CompareValue(Item1.Statistics.OutBitsPerSecond,
										   Item2.Statistics.OutBitsPerSecond);
					else
						Cmp = -1;
				} else if (Item2.EnableStatistics
						   && (Item2.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_MAX_IN_BANDWIDTH:
				if (Item1.MaxInBitsPerSecond >= 0) {
					if (Item2.MaxInBitsPerSecond >= 0)
						Cmp = CompareValue(Item1.MaxInBitsPerSecond,
										   Item2.MaxInBitsPerSecond);
					else
						Cmp = -1;
				} else if (Item2.MaxInBitsPerSecond >= 0)
					Cmp = 1;
				break;

			case ConnectionLogView::COLUMN_MAX_OUT_BANDWIDTH:
				if (Item1.MaxOutBitsPerSecond >= 0) {
					if (Item2.MaxOutBitsPerSecond >= 0)
						Cmp = CompareValue(Item1.MaxOutBitsPerSecond,
										   Item2.MaxOutBitsPerSecond);
					else
						Cmp = -1;
				} else if (Item2.MaxOutBitsPerSecond >= 0)
					Cmp = 1;
				break;
			}

			if (Cmp != 0)
				return m_Ascending ? Cmp<0: Cmp>0;
		}

		return false;
	}
};

bool ConnectionLogView::SortItems()
{
	std::sort(m_ItemList.begin(), m_ItemList.end(),
			  LogItemCompare(m_SortOrder, m_SortAscending));

	m_SelectedItem = -1;
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if ((m_ItemList[i].Flags & ItemInfo::FLAG_SELECTED) != 0) {
			m_SelectedItem = (int)i;
			break;
		}
	}

	return true;
}

}	// namespace CV
