/******************************************************************************
*                                                                             *
*    Settings.cpp                           Copyright(c) 2010-2016 itow,y.    *
*                                                                             *
******************************************************************************/

/*
  Connection Viewer
  Copyright(c) 2010-2016 itow,y.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
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
#include "Settings.h"
#include "Utility.h"


namespace CV
{

Settings::Settings()
	: m_OpenFlags(0)
{
}

Settings::~Settings()
{
}

bool Settings::Open(LPCTSTR pszFileName, LPCTSTR pszSection, unsigned int Flags)
{
	if (pszFileName == nullptr || pszSection == nullptr)
		return false;
	if (Flags == OPEN_READ && !::PathFileExists(pszFileName))
		return false;
	::lstrcpy(m_szFileName, pszFileName);
	::lstrcpyn(m_szSection, pszSection, cvLengthOf(m_szSection));
	m_OpenFlags = Flags;
	return true;
}

void Settings::Close()
{
	m_OpenFlags = 0;
}

bool Settings::Clear()
{
	if ((m_OpenFlags & OPEN_WRITE) == 0)
		return false;
	return ::WritePrivateProfileString(m_szSection, nullptr, nullptr, m_szFileName) != FALSE;
}

bool Settings::Flush()
{
	if ((m_OpenFlags & OPEN_WRITE) == 0)
		return false;
	return ::WritePrivateProfileString(nullptr, nullptr, nullptr, m_szFileName) != FALSE;
}

bool Settings::Read(LPCTSTR pszValueName, int *pData)
{
	TCHAR szValue[16];

	if (!Read(pszValueName, szValue, 16) || szValue[0] == _T('\0'))
		return false;
	*pData = StrToInt(szValue);
	return true;
}

bool Settings::Write(LPCTSTR pszValueName, int Data)
{
	TCHAR szValue[16];

	IntToStr(Data, szValue, cvLengthOf(szValue));
	return Write(pszValueName, szValue);
}

bool Settings::Read(LPCTSTR pszValueName, unsigned int *pData)
{
	TCHAR szValue[16];

	if (!Read(pszValueName, szValue, 16) || szValue[0] == _T('\0'))
		return false;
	*pData = StrToUInt(szValue);
	return true;
}

bool Settings::Write(LPCTSTR pszValueName, unsigned int Data)
{
	TCHAR szValue[16];

	UIntToStr(Data, szValue, cvLengthOf(szValue));
	return Write(pszValueName, szValue);
}

bool Settings::Read(LPCTSTR pszValueName, LPTSTR pszData, unsigned int Max)
{
	if (pszValueName == nullptr || pszData == nullptr)
		return false;
	if ((m_OpenFlags & OPEN_READ) == 0)
		return false;

	TCHAR cBack[2];

	cBack[0] = pszData[0];
	if (Max > 1)
		cBack[1] = pszData[1];
	::GetPrivateProfileString(m_szSection, pszValueName, TEXT("\x1A"), pszData, Max, m_szFileName);
	if (pszData[0] == _T('\x1A')) {
		pszData[0] = cBack[0];
		if (Max > 1)
			pszData[1] = cBack[1];
		return false;
	}
	return true;
}

bool Settings::Write(LPCTSTR pszValueName, LPCTSTR pszData)
{
	if (pszValueName == nullptr || pszData == nullptr)
		return false;
	if ((m_OpenFlags & OPEN_WRITE) == 0)
		return false;

	if (pszData[0] == _T('"') || pszData[0] == _T('\'')) {
		LPCTSTR p;
		TCHAR Quote;

		p = pszData;
		Quote = *p++;
		while (*p != _T('\0')) {
			if (*p == Quote && *(p + 1) == _T('\0'))
				break;
#ifndef UNICODE
			if (::IsDBCSLeadByteEx(CP_ACP, *p) && *(p + 1) != '\0')
				p++;
#endif
			p++;
		}
		if (*p == Quote) {
			int Length = (int)(p - pszData + 1);
			LPTSTR pszBuff = new TCHAR[Length + 3];

			pszBuff[0] = _T('"');
			::lstrcpy(pszBuff + 1, pszData);
			pszBuff[Length + 1] = _T('"');
			pszBuff[Length + 2] = _T('\0');
			BOOL Result = ::WritePrivateProfileString(m_szSection, pszValueName, pszBuff, m_szFileName);
			delete [] pszBuff;
			return Result != FALSE;
		}
	}
	return ::WritePrivateProfileString(m_szSection, pszValueName, pszData, m_szFileName) != FALSE;
}

bool Settings::Read(LPCTSTR pszValueName, bool *pfData)
{
	TCHAR szData[8];

	if (!Read(pszValueName, szData, cvLengthOf(szData)))
		return false;
	if (::lstrcmpi(szData, TEXT("true")) == 0
			|| ::lstrcmpi(szData, TEXT("yes")) == 0)
		*pfData = true;
	else if (::lstrcmpi(szData, TEXT("false")) == 0
			 || ::lstrcmpi(szData, TEXT("no")) == 0)
		*pfData = false;
	else
		return false;
	return true;
}

bool Settings::Write(LPCTSTR pszValueName, bool fData)
{
	return Write(pszValueName, fData ? TEXT("true") : TEXT("false"));
}

static int HexToNum(TCHAR cCode)
{
	if (cCode >= _T('0') && cCode <= _T('9'))
		return cCode - _T('0');
	if (cCode >= _T('A') && cCode <= _T('F'))
		return cCode - _T('A') + 10;
	if (cCode >= _T('a') && cCode <= _T('f'))
		return cCode - _T('a') + 10;
	return 0;
}

bool Settings::ReadColor(LPCTSTR pszValueName, COLORREF *pcrData)
{
	TCHAR szText[8];

	if (!Read(pszValueName, szText, 8)
			|| szText[0] != _T('#') || ::lstrlen(szText) != 7)
		return false;
	*pcrData = RGB((HexToNum(szText[1]) << 4) | HexToNum(szText[2]),
				   (HexToNum(szText[3]) << 4) | HexToNum(szText[4]),
				   (HexToNum(szText[5]) << 4) | HexToNum(szText[6]));
	return true;
}

bool Settings::WriteColor(LPCTSTR pszValueName, COLORREF crData)
{
	TCHAR szText[8];

	FormatString(szText, cvLengthOf(szText), TEXT("#%02X%02X%02X"),
				 GetRValue(crData), GetGValue(crData), GetBValue(crData));
	return Write(pszValueName, szText);
}

}	// namespace CV
