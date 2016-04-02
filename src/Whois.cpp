/******************************************************************************
*                                                                             *
*    Whois.cpp                              Copyright(c) 2010-2016 itow,y.    *
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
#include "Whois.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

WhoisQuery::WhoisQuery()
{
}

WhoisQuery::~WhoisQuery()
{
}

WhoisQuery::ErrorCode WhoisQuery::Query(LPCSTR pServer, LPCSTR pAddress, LPSTR *ppResult)
{
	*ppResult = nullptr;

	IPAddress Address;
	bool fOK = false;

	if (::IsCharAlphaA(pServer[0])) {
		struct addrinfo hints, *addr;

		::ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		if (::getaddrinfo(pServer, nullptr, &hints, &addr) == 0) {
			if (addr->ai_family == AF_INET) {
				Address.SetV4Address(
					reinterpret_cast<sockaddr_in*>(addr->ai_addr)->sin_addr.s_addr);
				fOK = true;
			} else if (addr->ai_family == AF_INET6) {
				Address.SetV6Address(
					reinterpret_cast<sockaddr_in6*>(addr->ai_addr)->sin6_addr.u.Byte);
				fOK = true;
			}
			::freeaddrinfo(addr);
		}
	} else {
		struct in_addr addr4;

		int Result = ::inet_pton(AF_INET, pServer, &addr4);
		if (Result == 1) {
			Address.SetV4Address(addr4.s_addr);
			fOK = true;
		} else if (Result == 0) {
			struct in6_addr addr6;
			Result = ::inet_pton(AF_INET6, pServer, &addr6);
			if (Result == 1) {
				Address.SetV6Address(addr6.u.Byte);
				fOK = true;
			}
		}
	}

	if (!fOK)
		return ERR_GET_HOST;

	sockaddr_in Addr4;
	sockaddr_in6 Addr6;
	sockaddr *pAddr;
	int AddrSize;
	if (Address.Type == IP_ADDRESS_V4) {
		Addr4.sin_family = AF_INET;
		Addr4.sin_port = ::htons(IPPORT_WHOIS);
		Addr4.sin_addr.s_addr = Address.V4.Address;
		pAddr = reinterpret_cast<sockaddr*>(&Addr4);
		AddrSize = sizeof(Addr4);
	} else if (Address.Type == IP_ADDRESS_V6) {
		Addr6.sin6_family = AF_INET6;
		Addr6.sin6_port = ::htons(IPPORT_WHOIS);
		Addr6.sin6_flowinfo = 0;
		::CopyMemory(Addr6.sin6_addr.u.Byte, Address.V6.Bytes, 16);
		Addr6.sin6_scope_id = 0;
		pAddr = reinterpret_cast<sockaddr*>(&Addr6);
		AddrSize = sizeof(Addr6);
	} else {
		return ERR_UNSUPPORTED_PROTOCOL;
	}

	SOCKET sock = ::socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		return ERR_SOCKET_OPEN;

	ErrorCode Error;

	if (::connect(sock, pAddr, AddrSize) != 0) {
		Error = ERR_CONNECT;
	} else {
		int Length;
		char szQuery[256];

		Length = ::wnsprintfA(szQuery, sizeof(szQuery), "%s\r\n", pAddress);
		if (::send(sock, szQuery, Length, 0) == SOCKET_ERROR) {
			Error = ERR_SEND;
		} else {
			LPSTR pReceived = nullptr;
			int ReceivedBufferLength = 0;
			Length = 0;
			while (true) {
				char Buffer[256];

				::ZeroMemory(Buffer, sizeof(Buffer));
				int Result = ::recv(sock, Buffer, sizeof(Buffer) - 1, 0);
				if (Result == SOCKET_ERROR)
					break;
				if (Result <= 0)
					break;
				Result = ::lstrlenA(Buffer);
				if (Length + Result >= ReceivedBufferLength) {
					ReceivedBufferLength = Length + Result + 2048;
					LPSTR pNewBuffer = new char[ReceivedBufferLength];
					if (pReceived != nullptr) {
						::lstrcpyA(pNewBuffer, pReceived);
						delete [] pReceived;
					}
					pReceived = pNewBuffer;
				}
				::CopyMemory(pReceived + Length, Buffer, Result + 1);
				Length += Result;
			}
			Error = ERR_SUCCESS;
			*ppResult = pReceived;
		}
	}

	::closesocket(sock);

	return Error;
}


WhoisServerList::WhoisServerList()
{
}

WhoisServerList::~WhoisServerList()
{
	Clear();
}

void WhoisServerList::Clear()
{
	for (size_t i = 0; i < m_List.size(); i++)
		delete [] m_List[i];
	m_List.clear();
}

bool WhoisServerList::Add(const char *pServer)
{
	if (pServer == nullptr || pServer[0] == '\0')
		return false;
	for (size_t i = 0; i < m_List.size(); i++) {
		if (::lstrcmpA(m_List[i], pServer) == 0)
			return true;
	}
	m_List.push_back(DuplicateString(pServer));
	return true;
}

int WhoisServerList::NumServers() const
{
	return (int)m_List.size();
}

const char *WhoisServerList::GetServer(int Index) const
{
	if (Index < 0 || (size_t)Index >= m_List.size())
		return nullptr;
	return m_List[Index];
}


WhoisDialog::WhoisDialog(WhoisServerList &ServerList)
	: m_ServerList(ServerList)
{
	m_szDefaultServer[0] = '\0';
}

WhoisDialog::~WhoisDialog()
{
}

void WhoisDialog::SetDefaultServer(LPCTSTR pServer)
{
	if (pServer != nullptr)
		::lstrcpyn(m_szDefaultServer, pServer, cvLengthOf(m_szDefaultServer));
	else
		m_szDefaultServer[0] = '\0';
}

LPCTSTR WhoisDialog::GetDefaultServer() const
{
	return m_szDefaultServer;
}

bool WhoisDialog::Show(HINSTANCE hinst, HWND Owner, const IPAddress &Address, bool Immediate)
{
	if (m_Handle == nullptr) {
		m_hinst = hinst;
		m_Handle = ::CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_WHOIS), Owner, DlgProc,
									   reinterpret_cast<LPARAM>(this));
		if (m_Handle == nullptr)
			return false;
	}

	TCHAR szText[64];
	FormatIPAddress(Address, szText, cvLengthOf(szText));
	::SetDlgItemText(m_Handle, IDC_WHOIS_ADDRESS, szText);

	if (Immediate) {
		::UpdateWindow(m_Handle);
		::SendMessage(m_Handle, WM_COMMAND, IDC_WHOIS_QUERY, 0);
	}

	return true;
}

INT_PTR CALLBACK WhoisDialog::DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_INITDIALOG:
		{
			WhoisDialog *pThis = reinterpret_cast<WhoisDialog*>(lParam);

			::SetProp(hDlg, TEXT("This"), pThis);

			LOGFONT lf;
			::GetObject(reinterpret_cast<HFONT>(
							::SendDlgItemMessage(hDlg, IDC_WHOIS_RESULT, WM_GETFONT, 0, 0)),
						sizeof(LOGFONT), &lf);
			::LoadString(pThis->m_hinst, IDS_DEFAULT_FIXED_FONT,
						 lf.lfFaceName, cvLengthOf(lf.lfFaceName));
			lf.lfWidth = 0;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
			pThis->m_Font = ::CreateFontIndirect(&lf);
			if (pThis->m_Font != nullptr)
				::SendDlgItemMessage(hDlg, IDC_WHOIS_RESULT, WM_SETFONT,
									 reinterpret_cast<WPARAM>(pThis->m_Font), FALSE);

			::SendDlgItemMessage(hDlg, IDC_WHOIS_ADDRESS, EM_LIMITTEXT, MAX_ADDRESS_LENGTH - 1, 0);

			::SendDlgItemMessage(hDlg, IDC_WHOIS_SERVER, CB_LIMITTEXT, MAX_SERVER_LENGTH - 1, 0);
			if (pThis->m_ServerList.NumServers() > 0) {
				for (int i = 0; i < pThis->m_ServerList.NumServers(); i++) {
					const char *pServer = pThis->m_ServerList.GetServer(i);

#ifdef UNICODE
					TCHAR szServer[256];
					::MultiByteToWideChar(CP_ACP, 0, pServer, -1, szServer, cvLengthOf(szServer));
					::SendDlgItemMessage(hDlg, IDC_WHOIS_SERVER, CB_ADDSTRING, 0,
										 reinterpret_cast<LPARAM>(szServer));
#else
					::SendDlgItemMessage(hDlg, IDC_WHOIS_SERVER, CB_ADDSTRING, 0,
										 reinterpret_cast<LPARAM>(pServer));
#endif
				}
				if (pThis->m_szDefaultServer[0] == '\0')
					::SendDlgItemMessage(hDlg, IDC_WHOIS_SERVER, CB_SETCURSEL, 0, 0);
			}
			if (pThis->m_szDefaultServer[0] != '\0')
				::SetDlgItemText(hDlg, IDC_WHOIS_SERVER, pThis->m_szDefaultServer);

			if (pThis->m_Position.Width == 0) {
				MoveWindowToCenter(hDlg);
			} else {
				::SetWindowPos(hDlg, nullptr,
							   pThis->m_Position.Left, pThis->m_Position.Top,
							   //pThis->m_Position.Width,pThis->m_Position.Height,
							   0, 0,
							   SWP_NOZORDER | SWP_NOSIZE);
				HMONITOR hMonitor = ::MonitorFromWindow(hDlg, MONITOR_DEFAULTTONULL);
				if (hMonitor == nullptr)
					MoveWindowToCenter(hDlg);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_WHOIS_QUERY:
			{
				char szAddress[MAX_ADDRESS_LENGTH], szServer[MAX_SERVER_LENGTH];

				::GetDlgItemTextA(hDlg, IDC_WHOIS_ADDRESS, szAddress, cvLengthOf(szAddress));
				::GetDlgItemTextA(hDlg, IDC_WHOIS_SERVER, szServer, cvLengthOf(szServer));
				if (szAddress[0] == '\0' || szServer[0] == '\0')
					return TRUE;
				::EnableWindow(::GetDlgItem(hDlg, IDC_WHOIS_QUERY), FALSE);
				HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
				WhoisQuery Whois;
				LPSTR pResult;
				WhoisQuery::ErrorCode Err = Whois.Query(szServer, szAddress, &pResult);
				if (pResult != nullptr) {
					char *pText = ISO2022JPToShiftJIS(pResult);
					if (pText != nullptr) {
						::SetDlgItemTextA(hDlg, IDC_WHOIS_RESULT, pText);
						delete [] pText;
					}
					delete [] pResult;
				} else {
					TCHAR szMessage[256];

					if (Err != WhoisQuery::ERR_SUCCESS)
						::LoadString(GetThis(hDlg)->m_hinst,
									 IDS_WHOIS_ERROR_FIRST + (int)Err,
									 szMessage, cvLengthOf(szMessage));
					else
						szMessage[0] = '\0';
					::SetDlgItemText(hDlg, IDC_WHOIS_RESULT, szMessage);
				}
				::SetCursor(hcurOld);
				::EnableWindow(::GetDlgItem(hDlg, IDC_WHOIS_QUERY), TRUE);
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		{
			WhoisDialog *pThis = GetThis(hDlg);

			::GetDlgItemText(hDlg, IDC_WHOIS_SERVER,
							 pThis->m_szDefaultServer, cvLengthOf(pThis->m_szDefaultServer));
			pThis->GetPosition(&pThis->m_Position);
		}
		return TRUE;

	case WM_NCDESTROY:
		{
			WhoisDialog *pThis = GetThis(hDlg);

			if (pThis != nullptr) {
				::RemoveProp(hDlg, TEXT("This"));
				::DeleteObject(pThis->m_Font);
				pThis->m_Handle = nullptr;
			}
		}
		return TRUE;
	}

	return FALSE;
}

WhoisDialog *WhoisDialog::GetThis(HWND hDlg)
{
	return static_cast<WhoisDialog*>(::GetProp(hDlg, TEXT("This")));
}

}	// namespace CV
