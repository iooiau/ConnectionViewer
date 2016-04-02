/******************************************************************************
*                                                                             *
*    ConnectionLog.cpp                      Copyright(c) 2010-2016 itow,y.    *
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
#include "ProgramCore.h"


namespace CV
{

ConnectionLog::ConnectionLog(const ProgramCore &Core)
	: m_Core(Core)
	, m_MaxLog(1000)
	, m_IDCount(0)
	, m_NumCurrentConnections(0)
{
}

ConnectionLog::~ConnectionLog()
{
}

void ConnectionLog::Clear()
{
	m_ItemList.clear();
	m_StringPool.Clear();
	m_NumCurrentConnections = 0;
}

void ConnectionLog::SetMaxLog(size_t Max)
{
	if (m_MaxLog != Max) {
		m_MaxLog = Max;
		if (Max < m_NumCurrentConnections)
			Max = m_NumCurrentConnections;
		while (m_ItemList.size() > Max)
			m_ItemList.pop_back();
	}
}

size_t ConnectionLog::GetMaxLog() const
{
	return m_MaxLog;
}

size_t ConnectionLog::NumItems() const
{
	return m_ItemList.size();
}

size_t ConnectionLog::NumCurrentConnections() const
{
	return m_NumCurrentConnections;
}

const ConnectionLog::ItemList &ConnectionLog::GetItemList() const
{
	return m_ItemList;
}

const ConnectionLog::ItemInfo &ConnectionLog::GetItemInfo(size_t Index) const
{
	return m_ItemList[Index];
}

void ConnectionLog::OnListUpdated()
{
	if (m_MaxLog == 0)
		return;

	const TimeAndTick CurTime = m_Core.GetUpdatedTime();
	const int NumConnections = m_Core.NumConnections();

	for (int i = 0; i < NumConnections; i++) {
		ItemInfo NewItem;

		m_Core.GetConnectionInfo(i, &NewItem.Info);
		NewItem.EnableStatistics = m_Core.GetConnectionStatistics(i, &NewItem.Statistics);
		bool Exists = false;
		ItemList::iterator j = m_ItemList.begin();
		for (j += i; j != m_ItemList.end(); j++) {
			ItemInfo &Item = *j;

			if (Item.UpdatedTime.Tick != m_UpdatedTime.Tick)
				break;
			if (Item.Info.PID == NewItem.Info.PID
					&& Item.Info.RemotePort == NewItem.Info.RemotePort
					&& Item.Info.LocalPort == NewItem.Info.LocalPort
					&& Item.Info.Protocol == NewItem.Info.Protocol
					&& Item.Info.RemoteAddress == NewItem.Info.RemoteAddress
					&& Item.Info.LocalAddress == NewItem.Info.LocalAddress
					&& Item.Info.CreateTimestamp == NewItem.Info.CreateTimestamp
					&& ((Item.Statistics.Mask & ConnectionStatistics::MASK_BYTES) == 0
						|| (NewItem.Statistics.Mask & ConnectionStatistics::MASK_BYTES) == 0
						|| (NewItem.Statistics.OutBytes >= Item.Statistics.OutBytes
							&& NewItem.Statistics.InBytes >= Item.Statistics.InBytes))) {
				Item.Info.State = NewItem.Info.State;
				if (NewItem.EnableStatistics
						&& (NewItem.Statistics.Mask & ConnectionStatistics::MASK_BYTES) != 0) {
					Item.Statistics.OutBytes = NewItem.Statistics.OutBytes;
					Item.Statistics.InBytes = NewItem.Statistics.InBytes;
				}
				if (NewItem.EnableStatistics
						&& (NewItem.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0) {
					if ((Item.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0) {
						ULONGLONG Diff1 = Item.UpdatedTime.Tick - Item.CreatedTime.Tick;
						ULONGLONG Diff2 = CurTime.Tick - Item.UpdatedTime.Tick;

						if (Diff1 == 0) {
							if (Diff2 == 0)
								Diff1 = Diff2 = 1;
							else
								Diff1 = Diff2;
						}
						Item.Statistics.OutBitsPerSecond =
							((Item.Statistics.OutBitsPerSecond * Diff1) +
							 (NewItem.Statistics.OutBitsPerSecond * Diff2)) / (Diff1 + Diff2);
						Item.Statistics.InBitsPerSecond =
							((Item.Statistics.InBitsPerSecond * Diff1) +
							 (NewItem.Statistics.InBitsPerSecond * Diff2)) / (Diff1 + Diff2);
					} else {
						Item.Statistics.OutBitsPerSecond = NewItem.Statistics.OutBitsPerSecond;
						Item.Statistics.InBitsPerSecond = NewItem.Statistics.InBitsPerSecond;
					}
					if (Item.MaxInBitsPerSecond < (LONGLONG)NewItem.Statistics.InBitsPerSecond)
						Item.MaxInBitsPerSecond = (LONGLONG)NewItem.Statistics.InBitsPerSecond;
					if (Item.MaxOutBitsPerSecond < (LONGLONG)NewItem.Statistics.OutBitsPerSecond)
						Item.MaxOutBitsPerSecond = (LONGLONG)NewItem.Statistics.OutBitsPerSecond;
				}
				Item.UpdatedTime = CurTime;
				if (j != m_ItemList.begin()) {
					NewItem = Item;
					j = m_ItemList.erase(j);
					m_ItemList.push_front(NewItem);
				}
				Exists = true;
				break;
			}
		}

		if (!Exists) {
			NewItem.ID = ++m_IDCount;
			NewItem.CreatedTime = CurTime;
			NewItem.UpdatedTime = CurTime;
			if (NewItem.Info.CreateTimestamp > 0)
				NewItem.ConnectionCreateTickCount = (LONGLONG)CurTime.Tick -
													((LONGLONG)FileTimeToUInt64(CurTime.Time) - NewItem.Info.CreateTimestamp) / 10000;

			if ((NewItem.Statistics.Mask & ConnectionStatistics::MASK_BANDWIDTH) != 0) {
				NewItem.MaxInBitsPerSecond = (LONGLONG)NewItem.Statistics.InBitsPerSecond;
				NewItem.MaxOutBitsPerSecond = (LONGLONG)NewItem.Statistics.OutBitsPerSecond;
			} else {
				NewItem.MaxInBitsPerSecond = -1;
				NewItem.MaxOutBitsPerSecond = -1;
			}

			ProcessList::ProcessInfoP ProcessInfo;
			if (m_Core.GetProcessInfo(NewItem.Info.PID, &ProcessInfo)) {
				NewItem.pProcessName = m_StringPool.Set(ProcessInfo.pFileName);
				NewItem.pProcessPath = m_StringPool.Set(ProcessInfo.pFilePath);
				NewItem.hProcessIcon = ProcessInfo.hIcon;
			} else {
				NewItem.pProcessName = nullptr;
				NewItem.pProcessPath = nullptr;
				NewItem.hProcessIcon = nullptr;
			}
			NewItem.pRemoteHostName = nullptr;

			NewItem.EnableCityInfo =
				NewItem.Info.Protocol == ConnectionProtocol::TCP
				&& m_Core.GetGeoIPCityInfo(NewItem.Info.RemoteAddress, &NewItem.CityInfo);

			m_ItemList.push_front(NewItem);
		}
	}

	const size_t MaxLog = max(m_MaxLog, (size_t)NumConnections);
	while (m_ItemList.size() > MaxLog)
		m_ItemList.pop_back();

	for (ItemList::iterator i = m_ItemList.begin(); i != m_ItemList.end(); i++) {
		if (CurTime.Tick - i->UpdatedTime.Tick >= 30 * 1000)
			break;
		if (i->pRemoteHostName == nullptr
				&& CurTime.Tick - i->CreatedTime.Tick < 30 * 1000) {
			TCHAR szHostName[256];

			if (m_Core.GetHostName(i->Info.RemoteAddress,
								   szHostName, cvLengthOf(szHostName)))
				i->pRemoteHostName = m_StringPool.Set(szHostName);
		}
	}

	m_NumCurrentConnections = NumConnections;
	m_UpdatedTime = CurTime;
}

ULONGLONG ConnectionLog::GetUpdatedTickCount() const
{
	return m_UpdatedTime.Tick;
}

const TimeAndTick &ConnectionLog::GetUpdatedTime() const
{
	return m_UpdatedTime;
}

bool ConnectionLog::OnHostNameFound(const IPAddress &Address)
{
	TCHAR szHostName[256];

	if (!m_Core.GetHostName(Address, szHostName, cvLengthOf(szHostName)))
		return false;

	LPCTSTR pHostName = nullptr;
	for (ItemList::iterator i = m_ItemList.begin(); i != m_ItemList.end(); i++) {
		ItemInfo &Item = *i;

		if (Item.UpdatedTime.Tick != m_UpdatedTime.Tick)
			break;
		if (Item.Info.RemoteAddress == Address) {
			if (pHostName == nullptr)
				pHostName = m_StringPool.Set(szHostName);
			Item.pRemoteHostName = pHostName;
		}
	}

	return pHostName != nullptr;
}

}	// namespace CV
