/******************************************************************************
*                                                                             *
*    ConnectionLog.h                        Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_CONNECTION_LOG_H
#define CV_CONNECTION_LOG_H


#include <deque>
#include "StringPool.h"
#include "GeoIPManager.h"


namespace CV
{
class ProgramCore;

class ConnectionLog
{
public:
	struct ItemInfo
	{
		ULONGLONG ID;
		TimeAndTick CreatedTime;
		TimeAndTick UpdatedTime;
		ConnectionInfo Info;
		LONGLONG ConnectionCreateTickCount;
		bool EnableStatistics;
		ConnectionStatistics Statistics;
		LONGLONG MaxInBitsPerSecond;
		LONGLONG MaxOutBitsPerSecond;
		LPCTSTR pProcessName;
		LPCTSTR pProcessPath;
		HICON hProcessIcon;
		LPCTSTR pRemoteHostName;
		bool EnableCityInfo;
		GeoIPManager::CityInfo CityInfo;
	};

	typedef std::deque<ItemInfo> ItemList;

	ConnectionLog(const ProgramCore &Core);
	~ConnectionLog();
	void Clear();
	void SetMaxLog(size_t Max);
	size_t GetMaxLog() const;
	size_t NumItems() const;
	size_t NumCurrentConnections() const;
	const ItemList &GetItemList() const;
	const ItemInfo &GetItemInfo(size_t Index) const;
	void OnListUpdated();
	ULONGLONG GetUpdatedTickCount() const;
	const TimeAndTick &GetUpdatedTime() const;
	bool OnHostNameFound(const IPAddress &Address);

private:
	const ProgramCore &m_Core;
	size_t m_MaxLog;
	ULONGLONG m_IDCount;
	ItemList m_ItemList;
	size_t m_NumCurrentConnections;
	TimeAndTick m_UpdatedTime;
	StringPool m_StringPool;
};

}	// namespace CV


#endif	// ndef CV_CONNECTION_LOG_H
