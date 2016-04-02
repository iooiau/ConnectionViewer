/******************************************************************************
*                                                                             *
*    ProgramCore.h                          Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_PROGRAM_CORE_H
#define CV_PROGRAM_CORE_H


#include "Connection.h"
#include "Process.h"
#include "HostManager.h"
#include "ConnectionLog.h"
#include "GeoIPManager.h"
#include "FilterManager.h"
#include "Preferences.h"
#include "Settings.h"


namespace CV
{

class ProgramCore
{
public:
	ProgramCore();
	~ProgramCore();
	void UpdateConnectionStatus();
	ULONGLONG GetUpdatedTickCount() const;
	const TimeAndTick &GetUpdatedTime() const;

	int NumConnections() const;
	int NumTCPConnections() const;
	int NumUDPConnections() const;
	bool GetConnectionInfo(int Index, ConnectionInfo *pInfo) const;
	bool GetConnectionStatistics(int Index, ConnectionStatistics *pStatistics) const;

	const ConnectionLog &GetConnectionLog() const;
	void SetConnectionLogMax(size_t Max);
	void ClearConnectionLog();
	bool OnHostNameFound(const IPAddress &Address);

	int NumNetworkInterfaces() const;
	const MIB_IF_ROW2 *GetNetworkInterfaceInfo(int Index) const;
	const MIB_IF_ROW2 *GetNetworkInterfaceInfo(const GUID &Guid) const;
	bool GetNetworkInterfaceStatistics(int Index, NetworkInterfaceStatistics *pStatistics) const;
	bool GetNetworkInterfaceStatistics(const GUID &Guid, NetworkInterfaceStatistics *pStatistics) const;
	bool GetNetworkInterfaceTotalStatistics(NetworkInterfaceStatistics *pStatistics) const;

	bool GetProcessFileName(DWORD PID, LPTSTR pFileName, int MaxFileName) const;
	bool GetProcessInfo(DWORD PID, ProcessList::ProcessInfoP *pInfo) const;

	bool GetHostName(HostManager::Request *pRequest);
	bool GetHostName(const IPAddress &Address, LPTSTR pHostName, int MaxLength) const;
	void EndHostManager();

	bool OpenGeoIP(LPCTSTR pFileName);
	bool GetGeoIPCountryInfo(const IPAddress &Address, GeoIPManager::CountryInfo *pInfo) const;
	bool GetGeoIPCityInfo(const IPAddress &Address, GeoIPManager::CityInfo *pInfo) const;
	const GeoIPManager &GetGeoIPManager() const;

	FilterManager &GetFilterManager();
	const FilterManager &GetFilterManager() const;

	HINSTANCE GetLanguageInstance() const;
	int LoadText(UINT ID, LPTSTR pText, int MaxLength) const;
	HMENU LoadMenu(UINT ID) const;
	bool LoadPreferences(Settings *pSettings);
	bool SavePreferences(Settings *pSettings) const;
	const AllPreferences &GetPreferences() const;
	AllPreferences &GetPreferences();

private:
	ConnectionStatus m_ConnectionStatus;
	NetworkInterfaceStatus m_InterfaceStatus;
	ProcessList m_ProcessList;
	HostManager m_HostManager;
	ConnectionLog m_ConnectionLog;
	GeoIPManager m_GeoIPManager;
	FilterManager m_FilterManager;
	TimeAndTick m_UpdatedTime;
	ULONGLONG m_UpdatedTickCount;
	HINSTANCE m_hinstLanguage;
	AllPreferences m_Preferences;
};

}	// namespace CV


#endif	// ndef CV_PROGRAM_CORE_H
