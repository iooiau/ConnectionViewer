/******************************************************************************
*                                                                             *
*    HostManager.cpp                        Copyright(c) 2010-2016 itow,y.    *
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
#include "HostManager.h"


namespace CV
{

HostManager::HostManager()
	: m_Thread(nullptr)
	, m_Event(nullptr)
{
}

HostManager::~HostManager()
{
	EndThread();
}

void HostManager::Clear()
{
	BlockLock Lock(m_Lock);

	while (!m_GetHostQueue.empty()) {
		delete m_GetHostQueue.front();
		m_GetHostQueue.pop_front();
	}
}

void HostManager::EndThread()
{
	if (m_Thread != nullptr) {
		m_Abort = true;
		::SetEvent(m_Event);
		if (::WaitForSingleObject(m_Thread, 10000) == WAIT_TIMEOUT)
			::TerminateThread(m_Thread, -1);
		::CloseHandle(m_Thread);
		m_Thread = nullptr;
	}
	if (m_Event != nullptr) {
		::CloseHandle(m_Event);
		m_Event = nullptr;
	}
	Clear();
}

bool HostManager::GetHostName(Request *pRequest)
{
	BlockLock Lock(m_Lock);

	HostMap::iterator i = m_HostMap.find(pRequest->GetAddress());
	if (i != m_HostMap.end()) {
		pRequest->OnHostFound(i->second.GetHostName());
		delete pRequest;
		return true;
	}

	for (GetHostQueue::iterator i = m_GetHostQueue.begin();
			i != m_GetHostQueue.end(); i++) {
		if (*(*i) == *pRequest) {
			delete pRequest;
			return true;
		}
	}

	if (m_Thread == nullptr) {
		m_Abort = false;
		if (m_Event == nullptr)
			m_Event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		else
			::ResetEvent(m_Event);
		m_Thread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
		if (m_Thread == nullptr) {
			delete pRequest;
			return false;
		}
	}

	m_GetHostQueue.push_back(pRequest);
	::SetEvent(m_Event);

	return true;
}

bool HostManager::GetHostName(const IPAddress &Address, LPTSTR pHostName, int MaxLength) const
{
	if (Address.Type == IP_ADDRESS_V4) {
		if (Address.V4.Address == 0 || Address.V4.Address == 0xFFFFFFFF)
			return false;
	} else if (Address.Type == IP_ADDRESS_V6) {
		if (Address.V6.IsUnspecified())
			return false;
	}

	BlockLock Lock(m_Lock);

	HostMap::const_iterator i = m_HostMap.find(Address);
	if (i == m_HostMap.end())
		return false;
	if (pHostName != nullptr)
		::lstrcpyn(pHostName, i->second.GetHostName(), MaxLength);
	return true;
}

DWORD WINAPI HostManager::ThreadProc(LPVOID pParameter)
{
	HostManager *pThis = static_cast<HostManager*>(pParameter);

	while (true) {
		::WaitForSingleObject(pThis->m_Event, INFINITE);

		while (true) {
			if (pThis->m_Abort)
				goto Exit;

			pThis->m_Lock.Lock();
			if (pThis->m_GetHostQueue.empty()) {
				pThis->m_Lock.Unlock();
				break;
			}
			Request *pRequest = pThis->m_GetHostQueue.front();
			pThis->m_GetHostQueue.pop_front();
			pThis->m_Lock.Unlock();

			sockaddr_in Addr4;
			sockaddr_in6 Addr6;
			SOCKADDR *pSockAddr;
			socklen_t SockAddrSize;
			const IPAddress &Address = pRequest->GetAddress();
			if (Address.Type == IP_ADDRESS_V4) {
				Addr4.sin_family = AF_INET;
				Addr4.sin_port = ::htons(pRequest->GetPort());
				Addr4.sin_addr.s_addr = Address.V4.Address;
				pSockAddr = reinterpret_cast<SOCKADDR*>(&Addr4);
				SockAddrSize = sizeof(Addr4);
			} else if (Address.Type == IP_ADDRESS_V6) {
				Addr6.sin6_family = AF_INET6;
				Addr6.sin6_port = ::htons(pRequest->GetPort());
				Addr6.sin6_flowinfo = 0;
				::memcpy(Addr6.sin6_addr.u.Byte, Address.V6.Bytes, 16);
				Addr6.sin6_scope_id = Address.V6.ScopeID;
				pSockAddr = reinterpret_cast<SOCKADDR*>(&Addr6);
				SockAddrSize = sizeof(Addr6);
			} else {
#ifdef _DEBUG
				::DebugBreak();
#endif
				delete pRequest;
				continue;
			}
			TCHAR szHostName[NI_MAXHOST];
			if (::GetNameInfo(pSockAddr, SockAddrSize,
							  szHostName, cvLengthOf(szHostName),
							  nullptr, 0, 0) == 0) {
				pThis->m_Lock.Lock();
				pThis->m_HostMap.insert(
					std::pair<IPAddress, HostInfo>(pRequest->GetAddress(), HostInfo(szHostName)));
				pThis->m_Lock.Unlock();
				pRequest->OnHostFound(szHostName);
			}
			delete pRequest;
		}
	}

Exit:

	return 0;
}


HostManager::HostInfo::HostInfo(LPCTSTR pHostName)
	: m_pHostName(DuplicateString(pHostName))
{
}

HostManager::HostInfo::HostInfo(const HostInfo &Src)
	: m_pHostName(DuplicateString(Src.m_pHostName))
{
}

HostManager::HostInfo::~HostInfo()
{
	delete [] m_pHostName;
}

HostManager::HostInfo &HostManager::HostInfo::operator=(const HostInfo &Src)
{
	if (this != &Src) {
		delete [] m_pHostName;
		m_pHostName = DuplicateString(Src.m_pHostName);
	}
	return *this;
}

}	// namespace CV
