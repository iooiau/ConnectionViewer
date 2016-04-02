/******************************************************************************
*                                                                             *
*    Whois.h                                Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_WHOIS_H
#define CV_WHOIS_H


#include <vector>
#include "Widget.h"


namespace CV
{

class WhoisQuery
{
public:
	enum ErrorCode
	{
		ERR_SUCCESS,
		ERR_GET_HOST,
		ERR_UNSUPPORTED_PROTOCOL,
		ERR_SOCKET_OPEN,
		ERR_CONNECT,
		ERR_SEND,
	};

	WhoisQuery();
	~WhoisQuery();
	ErrorCode Query(LPCSTR pServer, LPCSTR pAddress, LPSTR *ppResult);
};

class WhoisServerList
{
public:
	WhoisServerList();
	~WhoisServerList();
	void Clear();
	bool Add(const char *pServer);
	int NumServers() const;
	const char *GetServer(int Index) const;

private:
	std::vector<char*> m_List;
};

class WhoisDialog : public Widget
{
public:
	WhoisDialog(WhoisServerList &ServerList);
	~WhoisDialog();
	void SetDefaultServer(LPCTSTR pServer);
	LPCTSTR GetDefaultServer() const;
	bool Show(HINSTANCE hinst, HWND Owner, const IPAddress &Address, bool Immediate = true);

private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static WhoisDialog *GetThis(HWND hDlg);

	enum {
		MAX_ADDRESS_LENGTH	= 256,
		MAX_SERVER_LENGTH	= 256
	};

	WhoisServerList &m_ServerList;
	HINSTANCE m_hinst;
	HFONT m_Font;
	TCHAR m_szDefaultServer[MAX_SERVER_LENGTH];
};

}	// namespace CV


#endif	// ndef CV_WHOIS_H
