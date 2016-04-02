/******************************************************************************
*                                                                             *
*    Settings.h                             Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_SETTINGS_H
#define CV_SETTINGS_H


namespace CV
{

class Settings
{
public:
	enum
	{
		OPEN_READ  = 0x00000001,
		OPEN_WRITE = 0x00000002
	};

	Settings();
	~Settings();
	bool Open(LPCTSTR pszFileName, LPCTSTR pszSection, unsigned int Flags);
	void Close();
	bool Clear();
	bool Flush();
	bool Read(LPCTSTR pszValueName, int *pData);
	bool Write(LPCTSTR pszValueName, int Data);
	bool Read(LPCTSTR pszValueName, unsigned int *pData);
	bool Write(LPCTSTR pszValueName, unsigned int Data);
	bool Read(LPCTSTR pszValueName, LPTSTR pszData, unsigned int Max);
	bool Write(LPCTSTR pszValueName, LPCTSTR pszData);
	bool Read(LPCTSTR pszValueName, bool *pfData);
	bool Write(LPCTSTR pszValueName, bool fData);
	bool ReadColor(LPCTSTR pszValueName, COLORREF *pcrData);
	bool WriteColor(LPCTSTR pszValueName, COLORREF crData);

private:
	TCHAR m_szFileName[MAX_PATH];
	TCHAR m_szSection[64];
	unsigned int m_OpenFlags;
};

}	// namespace CV


#endif	// ndef CV_SETTINGS_H
