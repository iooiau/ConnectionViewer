/******************************************************************************
*                                                                             *
*    Process.cpp                            Copyright(c) 2010-2016 itow,y.    *
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
#include <psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include "Process.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")


namespace CV
{

bool GetExeFileNameByPID(DWORD PID, LPTSTR pFileName, int MaxFileName)
{
	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);

	if (hProcess == nullptr)
		return false;
	//bool OK= ::GetProcessImageFileName(hProcess,pFileName,MaxFileName)>0;
	DWORD Size = MaxFileName;
	bool OK = ::QueryFullProcessImageName(hProcess, 0, pFileName, &Size) != FALSE;
	::CloseHandle(hProcess);
	if (OK)
		::GetLongPathName(pFileName, pFileName, MaxFileName);
	return OK;
}


ProcessList::ProcessList()
{
}

ProcessList::~ProcessList()
{
}

void ProcessList::BeginUpdate()
{
	for (ProcessMap::iterator i = m_ProcessMap.begin(); i != m_ProcessMap.end(); i++) {
		i->second.Updated = false;
	}

	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe;

		pe.dwSize = sizeof(pe);
		if (::Process32First(hSnapshot, &pe)) {
			do {
				ProcessMap::iterator i = m_ProcessMap.find(pe.th32ProcessID);

				if (i != m_ProcessMap.end()) {
					i->second.SetFileName(pe.szExeFile);
				} else {
					ProcessInfo Info;

					::lstrcpy(Info.szFileName, pe.szExeFile);
					m_ProcessMap.insert(std::pair<DWORD, ProcessInfo>(pe.th32ProcessID, Info));
				}
			} while (::Process32Next(hSnapshot, &pe));
		}
		::CloseHandle(hSnapshot);
	}
}

void ProcessList::EndUpdate()
{
	for (ProcessMap::iterator i = m_ProcessMap.begin(); i != m_ProcessMap.end();) {
		if (!i->second.Updated)
			m_ProcessMap.erase(i++);
		else
			i++;
	}
}

bool ProcessList::UpdateProcessInfo(DWORD PID)
{
	ProcessMap::iterator i = m_ProcessMap.find(PID);

	if (i != m_ProcessMap.end()) {
		if (!i->second.Updated) {
			TCHAR szFileName[MAX_PATH];

			if (GetExeFileNameByPID(PID, szFileName, cvLengthOf(szFileName)))
				i->second.SetPath(szFileName);
			i->second.Updated = true;
		}
	} else {
		TCHAR szFilePath[MAX_PATH];
		ProcessInfo Info;

		if (!GetExeFileNameByPID(PID, szFilePath, cvLengthOf(szFilePath)))
			return false;
		Info.SetPath(szFilePath);
		m_ProcessMap.insert(std::pair<DWORD, ProcessInfo>(PID, Info));
	}
	return true;
}

bool ProcessList::GetProcessFileName(DWORD PID, LPTSTR pFileName, int MaxFileName) const
{
	ProcessMap::const_iterator i = m_ProcessMap.find(PID);

	if (i == m_ProcessMap.end())
		return false;
	::lstrcpyn(pFileName, i->second.szFileName, MaxFileName);
	return true;
}

bool ProcessList::GetProcessFilePath(DWORD PID, LPTSTR pFilePath, int MaxFilePath) const
{
	ProcessMap::const_iterator i = m_ProcessMap.find(PID);

	if (i == m_ProcessMap.end() || i->second.szFilePath[0] == _T('\0'))
		return false;
	::lstrcpyn(pFilePath, i->second.szFilePath, MaxFilePath);
	return true;
}

HICON ProcessList::GetProcessIcon(DWORD PID) const
{
	ProcessMap::const_iterator i = m_ProcessMap.find(PID);

	if (i == m_ProcessMap.end())
		return nullptr;
	return i->second.hIcon;
}

bool ProcessList::GetProcessInfo(DWORD PID, ProcessInfoP *pInfo) const
{
	ProcessMap::const_iterator i = m_ProcessMap.find(PID);

	if (i == m_ProcessMap.end())
		return false;
	pInfo->pFileName = i->second.szFileName;
	pInfo->pFilePath = i->second.szFilePath;
	pInfo->hIcon = i->second.hIcon;
	return true;
}


ProcessList::ProcessInfo::ProcessInfo()
	: Updated(false)
	, GetIconFailed(false)
	, hIcon(nullptr)
{
	szFileName[0] = _T('\0');
	szFilePath[0] = _T('\0');
}

ProcessList::ProcessInfo::ProcessInfo(const ProcessInfo &Src)
	: Updated(Src.Updated)
	, GetIconFailed(Src.GetIconFailed)
	, hIcon(nullptr)
{
	if (Src.hIcon != nullptr)
		hIcon = ::CopyIcon(Src.hIcon);
	::lstrcpy(szFileName, Src.szFileName);
	::lstrcpy(szFilePath, Src.szFilePath);
}

ProcessList::ProcessInfo::~ProcessInfo()
{
	DestroyIcon();
}

ProcessList::ProcessInfo &ProcessList::ProcessInfo::operator=(const ProcessInfo &Src)
{
	if (&Src != this) {
		Updated = Src.Updated;
		GetIconFailed = Src.GetIconFailed;
		::lstrcpy(szFileName, Src.szFileName);
		::lstrcpy(szFilePath, Src.szFilePath);
		DestroyIcon();
		if (Src.hIcon != nullptr)
			hIcon = ::CopyIcon(Src.hIcon);
	}
	return *this;
}

void ProcessList::ProcessInfo::SetFileName(LPCTSTR pFileName)
{
	if (::lstrcmpi(szFileName, pFileName) != 0) {
		::lstrcpy(szFileName, pFileName);
		szFilePath[0] = _T('\0');
		DestroyIcon();
	} else {
		Updated = true;
	}
}

void ProcessList::ProcessInfo::SetPath(LPCTSTR pFilePath)
{
	if (::lstrcmpi(szFilePath, pFilePath) != 0) {
		::lstrcpy(szFilePath, pFilePath);
		if (szFileName[0] == _T('\0'))
			::lstrcpy(szFileName, ::PathFindFileName(pFilePath));
		DestroyIcon();
		GetIcon();
	}
	Updated = true;
}

HICON ProcessList::ProcessInfo::GetIcon()
{
	if (hIcon == nullptr && !GetIconFailed && szFilePath[0] != _T('\0')) {
		SHFILEINFO shfi;

		if (::SHGetFileInfo(szFilePath, 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON)) {
			hIcon = shfi.hIcon;
		} else {
			GetIconFailed = true;
		}
	}
	return hIcon;
}

void ProcessList::ProcessInfo::DestroyIcon()
{
	if (hIcon != nullptr) {
		::DestroyIcon(hIcon);
		hIcon = nullptr;
	}
	GetIconFailed = false;
}

}	// namespace CV
