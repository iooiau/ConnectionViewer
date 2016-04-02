/******************************************************************************
*                                                                             *
*    Process.h                              Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_PROCESS_H
#define CV_PROCESS_H


#include <map>


namespace CV
{

bool GetExeFileNameByPID(DWORD PID, LPTSTR pFileName, int MaxFileName);

class ProcessList
{
public:
	struct ProcessInfoP
	{
		LPCTSTR pFileName;
		LPCTSTR pFilePath;
		HICON hIcon;
	};

	ProcessList();
	~ProcessList();
	void BeginUpdate();
	void EndUpdate();
	bool UpdateProcessInfo(DWORD PID);
	bool GetProcessFileName(DWORD PID, LPTSTR pFileName, int MaxFileName) const;
	bool GetProcessFilePath(DWORD PID, LPTSTR pFilePath, int MaxFilePath) const;
	HICON GetProcessIcon(DWORD PID) const;
	bool GetProcessInfo(DWORD PID, ProcessInfoP *pInfo) const;

private:
	struct ProcessInfo
	{
		bool Updated;
		bool GetIconFailed;
		TCHAR szFileName[MAX_PATH];
		TCHAR szFilePath[MAX_PATH];
		HICON hIcon;

		ProcessInfo();
		ProcessInfo(const ProcessInfo &Src);
		~ProcessInfo();
		ProcessInfo &operator=(const ProcessInfo &Src);
		void SetFileName(LPCTSTR pFileName);
		void SetPath(LPCTSTR pFilePath);
		HICON GetIcon();
		void DestroyIcon();
	};

	typedef std::map<DWORD, ProcessInfo> ProcessMap;
	ProcessMap m_ProcessMap;
};

}	// namespace CV


#endif	// ndef CV_PROCESS_H
