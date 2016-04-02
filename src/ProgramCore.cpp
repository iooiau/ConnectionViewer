/******************************************************************************
*                                                                             *
*    ProgramCore.cpp                        Copyright(c) 2010-2016 itow,y.    *
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

#pragma warning(disable : 4355)

ProgramCore::ProgramCore()
	: m_ConnectionLog(*this)
	, m_hinstLanguage(::GetModuleHandle(nullptr))
{
}

ProgramCore::~ProgramCore()
{
}

void ProgramCore::UpdateConnectionStatus()
{
	m_ConnectionStatus.Update();
	m_InterfaceStatus.Update();

	m_UpdatedTime.SetCurrent();

	m_ProcessList.BeginUpdate();
	const int NumConnections = m_ConnectionStatus.NumConnections();
	for (int i = 0; i < NumConnections; i++) {
		m_ProcessList.UpdateProcessInfo(m_ConnectionStatus.GetConnectionPID(i));
	}
	m_ProcessList.EndUpdate();

	m_ConnectionLog.OnListUpdated();
}

ULONGLONG ProgramCore::GetUpdatedTickCount() const
{
	return m_UpdatedTime.Tick;
}

const TimeAndTick &ProgramCore::GetUpdatedTime() const
{
	return m_UpdatedTime;
}

int ProgramCore::NumConnections() const
{
	return m_ConnectionStatus.NumConnections();
}

int ProgramCore::NumTCPConnections() const
{
	return m_ConnectionStatus.NumTCPConnections();
}

int ProgramCore::NumUDPConnections() const
{
	return m_ConnectionStatus.NumUDPConnections();
}

bool ProgramCore::GetConnectionInfo(int Index, ConnectionInfo *pInfo) const
{
	return m_ConnectionStatus.GetConnectionInfo(Index, pInfo);
}

bool ProgramCore::GetConnectionStatistics(int Index, ConnectionStatistics *pStatistics) const
{
	return m_ConnectionStatus.GetConnectionStatistics(Index, pStatistics);
}

const ConnectionLog &ProgramCore::GetConnectionLog() const
{
	return m_ConnectionLog;
}

void ProgramCore::SetConnectionLogMax(size_t Max)
{
	m_ConnectionLog.SetMaxLog(Max);
}

void ProgramCore::ClearConnectionLog()
{
	m_ConnectionLog.Clear();
}

bool ProgramCore::OnHostNameFound(const IPAddress &Address)
{
	return m_ConnectionLog.OnHostNameFound(Address);
}

int ProgramCore::NumNetworkInterfaces() const
{
	return m_InterfaceStatus.NumInterfaces();
}

const MIB_IF_ROW2 *ProgramCore::GetNetworkInterfaceInfo(int Index) const
{
	return m_InterfaceStatus.GetInterfaceInfo(Index);
}

const MIB_IF_ROW2 *ProgramCore::GetNetworkInterfaceInfo(const GUID &Guid) const
{
	return m_InterfaceStatus.GetInterfaceInfo(Guid);
}

bool ProgramCore::GetNetworkInterfaceStatistics(int Index, NetworkInterfaceStatistics *pStatistics) const
{
	return m_InterfaceStatus.GetInterfaceStatistics(Index, pStatistics);
}

bool ProgramCore::GetNetworkInterfaceStatistics(const GUID &Guid, NetworkInterfaceStatistics *pStatistics) const
{
	return m_InterfaceStatus.GetInterfaceStatistics(Guid, pStatistics);
}

bool ProgramCore::GetNetworkInterfaceTotalStatistics(NetworkInterfaceStatistics *pStatistics) const
{
	return m_InterfaceStatus.GetTotalStatistics(pStatistics);
}

bool ProgramCore::GetProcessFileName(DWORD PID, LPTSTR pFileName, int MaxFileName) const
{
	return m_ProcessList.GetProcessFileName(PID, pFileName, MaxFileName);
}

bool ProgramCore::GetProcessInfo(DWORD PID, ProcessList::ProcessInfoP *pInfo) const
{
	return m_ProcessList.GetProcessInfo(PID, pInfo);
}

bool ProgramCore::GetHostName(HostManager::Request *pRequest)
{
	return m_HostManager.GetHostName(pRequest);
}

bool ProgramCore::GetHostName(const IPAddress &Address, LPTSTR pHostName, int MaxLength) const
{
	return m_HostManager.GetHostName(Address, pHostName, MaxLength);
}

void ProgramCore::EndHostManager()
{
	m_HostManager.EndThread();
}

bool ProgramCore::OpenGeoIP(LPCTSTR pFileName)
{
	if (::PathIsRelative(pFileName)) {
		TCHAR szTemp[MAX_PATH], szFileName[MAX_PATH];

		::GetModuleFileName(nullptr, szTemp, cvLengthOf(szTemp));
		::PathRemoveFileSpec(szTemp);
		if (::lstrlen(szTemp) + 1 +::lstrlen(pFileName) >= MAX_PATH)
			return false;
		::PathAppend(szTemp, pFileName);
		::PathCanonicalize(szFileName, szTemp);
		return m_GeoIPManager.Open(szFileName);
	}
	return m_GeoIPManager.Open(pFileName);
}

bool ProgramCore::GetGeoIPCountryInfo(const IPAddress &Address, GeoIPManager::CountryInfo *pInfo) const
{
	return m_GeoIPManager.GetCountryInfo(Address, pInfo);
}

bool ProgramCore::GetGeoIPCityInfo(const IPAddress &Address, GeoIPManager::CityInfo *pInfo) const
{
	return m_GeoIPManager.GetCityInfo(Address, pInfo);
}

const GeoIPManager &ProgramCore::GetGeoIPManager() const
{
	return m_GeoIPManager;
}

FilterManager &ProgramCore::GetFilterManager()
{
	return m_FilterManager;
}

const FilterManager &ProgramCore::GetFilterManager() const
{
	return m_FilterManager;
}

HINSTANCE ProgramCore::GetLanguageInstance() const
{
	return m_hinstLanguage;
}

int ProgramCore::LoadText(UINT ID, LPTSTR pText, int MaxLength) const
{
	return ::LoadString(m_hinstLanguage, ID, pText, MaxLength);
}

HMENU ProgramCore::LoadMenu(UINT ID) const
{
	return ::LoadMenu(m_hinstLanguage, MAKEINTRESOURCE(ID));
}

bool ProgramCore::LoadPreferences(Settings *pSettings)
{
	pSettings->Read(TEXT("GeoIP.Database"),
					m_Preferences.Core.GeoIPDatabaseFileName,
					cvLengthOf(m_Preferences.Core.GeoIPDatabaseFileName));

	TCHAR szFontName[LF_FACESIZE];
	if (pSettings->Read(TEXT("List.FontName"), szFontName, cvLengthOf(szFontName))
			&& szFontName[0] != _T('\0')) {
		int Height, Style;

		m_Preferences.List.Font.lfWidth = 0;
		m_Preferences.List.Font.lfEscapement = 0;
		m_Preferences.List.Font.lfOrientation = 0;
		m_Preferences.List.Font.lfItalic = 0;
		m_Preferences.List.Font.lfUnderline = 0;
		m_Preferences.List.Font.lfStrikeOut = 0;
		m_Preferences.List.Font.lfCharSet = DEFAULT_CHARSET;
		m_Preferences.List.Font.lfOutPrecision = OUT_DEFAULT_PRECIS;
		m_Preferences.List.Font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		m_Preferences.List.Font.lfQuality = DEFAULT_QUALITY;
		m_Preferences.List.Font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		::lstrcpy(m_Preferences.List.Font.lfFaceName, szFontName);
		if (pSettings->Read(TEXT("List.FontHeight"), &Height))
			m_Preferences.List.Font.lfHeight = Height;
		if (pSettings->Read(TEXT("List.FontStyle"), &Style)) {
			m_Preferences.List.Font.lfWeight = (Style & 1) != 0 ? FW_BOLD : FW_NORMAL;
			m_Preferences.List.Font.lfItalic = (Style & 2) != 0;
		}
	}
	pSettings->Read(TEXT("List.ShowGrid"), &m_Preferences.List.ShowGrid);
	pSettings->ReadColor(TEXT("List.TextColor"), &m_Preferences.List.TextColor);
	pSettings->ReadColor(TEXT("List.GridColor"), &m_Preferences.List.GridColor);
	pSettings->ReadColor(TEXT("List.BackColor"), &m_Preferences.List.BackColor1);
	pSettings->ReadColor(TEXT("List.BackColor2"), &m_Preferences.List.BackColor2);
	pSettings->ReadColor(TEXT("List.New.BackColor"), &m_Preferences.List.NewBackColor);

	unsigned int MaxLog;
	if (pSettings->Read(TEXT("Log.Max"), &MaxLog))
		m_Preferences.Log.MaxLog = MaxLog;

	pSettings->ReadColor(TEXT("Graph.BackColor"), &m_Preferences.Graph.BackColor);
	pSettings->ReadColor(TEXT("Graph.GridColor"), &m_Preferences.Graph.GridColor);
	pSettings->ReadColor(TEXT("Graph.CaptionColor"), &m_Preferences.Graph.CaptionColor);

	int GraphType;
	if (pSettings->Read(TEXT("Graph.InBandwidth.Type"), &GraphType))
		m_Preferences.Graph.InBandwidth.Type = (GraphPreferences::GraphInfo::GraphType)GraphType;
	pSettings->ReadColor(TEXT("Graph.InBandwidth.Color"), &m_Preferences.Graph.InBandwidth.Color);
	pSettings->Read(TEXT("Graph.InBandwidth.Scale"), &m_Preferences.Graph.InBandwidth.Scale);

	if (pSettings->Read(TEXT("Graph.OutBandwidth.Type"), &GraphType))
		m_Preferences.Graph.OutBandwidth.Type = (GraphPreferences::GraphInfo::GraphType)GraphType;
	pSettings->ReadColor(TEXT("Graph.OutBandwidth.Color"), &m_Preferences.Graph.OutBandwidth.Color);
	pSettings->Read(TEXT("Graph.OutBandwidth.Scale"), &m_Preferences.Graph.OutBandwidth.Scale);

	if (pSettings->Read(TEXT("Graph.Connections.Type"), &GraphType))
		m_Preferences.Graph.Connections.Type = (GraphPreferences::GraphInfo::GraphType)GraphType;
	pSettings->ReadColor(TEXT("Graph.Connections.Color"), &m_Preferences.Graph.Connections.Color);
	pSettings->Read(TEXT("Graph.Connections.Scale"), &m_Preferences.Graph.Connections.Scale);

	return true;
}

bool ProgramCore::SavePreferences(Settings *pSettings) const
{
	pSettings->Write(TEXT("GeoIP.Database"),
					 m_Preferences.Core.GeoIPDatabaseFileName);

	pSettings->Write(TEXT("List.FontName"), m_Preferences.List.Font.lfFaceName);
	pSettings->Write(TEXT("List.FontHeight"), m_Preferences.List.Font.lfHeight);
	pSettings->Write(TEXT("List.FontStyle"),
					 (m_Preferences.List.Font.lfWeight >= FW_BOLD ? 1 : 0) |
					 (m_Preferences.List.Font.lfItalic ? 2 : 0));
	pSettings->Write(TEXT("List.ShowGrid"), m_Preferences.List.ShowGrid);
	pSettings->WriteColor(TEXT("List.TextColor"), m_Preferences.List.TextColor);
	pSettings->WriteColor(TEXT("List.GridColor"), m_Preferences.List.GridColor);
	pSettings->WriteColor(TEXT("List.BackColor"), m_Preferences.List.BackColor1);
	pSettings->WriteColor(TEXT("List.BackColor2"), m_Preferences.List.BackColor2);
	pSettings->WriteColor(TEXT("List.New.BackColor"), m_Preferences.List.NewBackColor);

	pSettings->Write(TEXT("Log.Max"), (unsigned int)m_Preferences.Log.MaxLog);

	pSettings->WriteColor(TEXT("Graph.BackColor"), m_Preferences.Graph.BackColor);
	pSettings->WriteColor(TEXT("Graph.GridColor"), m_Preferences.Graph.GridColor);
	pSettings->WriteColor(TEXT("Graph.CaptionColor"), m_Preferences.Graph.CaptionColor);

	pSettings->Write(TEXT("Graph.InBandwidth.Type"), (int)m_Preferences.Graph.InBandwidth.Type);
	pSettings->WriteColor(TEXT("Graph.InBandwidth.Color"), m_Preferences.Graph.InBandwidth.Color);
	pSettings->Write(TEXT("Graph.InBandwidth.Scale"), m_Preferences.Graph.InBandwidth.Scale);

	pSettings->Write(TEXT("Graph.OutBandwidth.Type"), (int)m_Preferences.Graph.OutBandwidth.Type);
	pSettings->WriteColor(TEXT("Graph.OutBandwidth.Color"), m_Preferences.Graph.OutBandwidth.Color);
	pSettings->Write(TEXT("Graph.OutBandwidth.Scale"), m_Preferences.Graph.OutBandwidth.Scale);

	pSettings->Write(TEXT("Graph.Connections.Type"), (int)m_Preferences.Graph.Connections.Type);
	pSettings->WriteColor(TEXT("Graph.Connections.Color"), m_Preferences.Graph.Connections.Color);
	pSettings->Write(TEXT("Graph.Connections.Scale"), m_Preferences.Graph.Connections.Scale);

	return true;
}

const AllPreferences &ProgramCore::GetPreferences() const
{
	return m_Preferences;
}

AllPreferences &ProgramCore::GetPreferences()
{
	return m_Preferences;
}

}	// namespace CV
