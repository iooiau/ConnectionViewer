/******************************************************************************
*                                                                             *
*    HostManager.h                          Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_HOST_MANAGER_H
#define CV_HOST_MANAGER_H


#include <map>
#include <deque>
#include "Utility.h"


namespace CV
{

class HostManager
{
public:
	cvAbstractClass(Request)
	{
public:
		Request(const IPAddress & Address, WORD Port)
			: m_Address(Address)
			, m_Port(Port)
		{
		}
		virtual ~Request() {}
		bool operator==(const Request & RVal) const
		{
			return m_Address == RVal.m_Address && m_Port == RVal.m_Port;
		}
		bool operator!=(const Request & RVal) const
		{
			return !(*this == RVal);
		}
		virtual void OnHostFound(LPCTSTR pHostName) {};
		const IPAddress &GetAddress() const { return m_Address; }
		const WORD GetPort() const { return m_Port; }

protected:
		IPAddress m_Address;
		WORD m_Port;
	};

	HostManager();
	~HostManager();
	void Clear();
	void EndThread();
	bool GetHostName(Request *pRequest);
	bool GetHostName(const IPAddress &Address, LPTSTR pHostName, int MaxLength) const;

private:
	static DWORD WINAPI ThreadProc(LPVOID pParameter);

	class HostInfo
	{
	public:
		HostInfo(LPCTSTR pHostName);
		HostInfo(const HostInfo &Src);
		~HostInfo();
		HostInfo &operator=(const HostInfo &Src);
		LPCTSTR GetHostName() const { return m_pHostName; }

	private:
		LPTSTR m_pHostName;
	};

	typedef std::map<IPAddress, HostInfo> HostMap;
	typedef std::deque<Request*> GetHostQueue;

	HANDLE m_Thread;
	HANDLE m_Event;
	volatile bool m_Abort;
	mutable LocalLock m_Lock;
	HostMap m_HostMap;
	GetHostQueue m_GetHostQueue;
};

}	// namespace CV


#endif	// ndef CV_HOST_MANAGER_H
