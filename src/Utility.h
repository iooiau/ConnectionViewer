/******************************************************************************
*                                                                             *
*    Utility.h                              Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_UTILITY_H
#define CV_UTILITY_H


namespace CV
{

inline UINT64 PackUInt64(UINT32 Low, UINT32 High)
{
	return ((UINT64)High << 32) | (UINT64)Low;
}

LPSTR DuplicateString(LPCSTR pString);
LPWSTR DuplicateString(LPCWSTR pString);
void ReplaceChar(LPTSTR pString, TCHAR From, TCHAR To);

int StrToInt(LPCTSTR pszValue, int Base = 10);
unsigned int StrToUInt(LPCTSTR pszValue, int Base = 10);
INT64 StrToInt64(LPCTSTR pszValue, int Base = 10);
UINT64 StrToUInt64(LPCTSTR pszValue, int Base = 10);

int IntToStr(int Value, LPTSTR pszString, int MaxLength);
int UIntToStr(unsigned int Value, LPTSTR pszString, int MaxLength);
int Int64ToStr(LONGLONG Value, LPTSTR pszString, int MaxLength);
int UInt64ToStr(ULONGLONG Value, LPTSTR pszString, int MaxLength);
int FormatInt(int Value, LPTSTR pText, int MaxLength);
int FormatUInt(unsigned int Value, LPTSTR pText, int MaxLength);
int FormatInt64(LONGLONG Value, LPTSTR pText, int MaxLength);
int FormatUInt64(ULONGLONG Value, LPTSTR pText, int MaxLength);
int FormatDecimalInt(int Value, int Denom, int Digits, LPTSTR pszNumber, int MaxLength);
int FormatDecimalInt64(LONGLONG Value, LONGLONG Denom, int Digits, LPTSTR pszNumber, int MaxLength);

int FormatString(LPTSTR pDest, int DestLength, LPCTSTR pFormat, ...);
int FormatStringV(LPTSTR pDest, int DestLength, LPCTSTR pFormat, va_list Args);

void CopyString(LPTSTR pDest, int DestLength, LPCTSTR pSrc);

enum
{
	SYSTEMTIME_FORMAT_LONG		= 0x0001,
	SYSTEMTIME_FORMAT_TIME		= 0x0002,
	SYSTEMTIME_FORMAT_SECONDS	= 0x0004
};
int FormatSystemTime(const SYSTEMTIME &Time, unsigned int Flags, LPTSTR pText, int MaxLength);

int FormatBytesLong(ULONGLONG Bytes, LPTSTR pText, int MaxLength);
int FormatBandwidthLong(ULONGLONG BytesPerSecond, LPTSTR pText, int MaxLength);

int GetSystemErrorMessage(DWORD Code, LPTSTR pText, int MaxLength);

bool IsAdministratorsMember();

char *ISO2022JPToShiftJIS(const char *pString);

inline UINT64 FileTimeToUInt64(const FILETIME &Time)
{
	return PackUInt64(Time.dwLowDateTime, Time.dwHighDateTime);
}

int PixelsToPoints(HDC hdc, int Size);
int CalcFontPointSize(HDC hdc, const LOGFONT &Font);

void MoveWindowToCenter(HWND hwnd);

class LocalLock
{
public:
	LocalLock()
	{
		::InitializeCriticalSection(&m_CriticalSection);
	}

	~LocalLock()
	{
		::DeleteCriticalSection(&m_CriticalSection);
	}

	void Lock()
	{
		::EnterCriticalSection(&m_CriticalSection);
	}

	void Unlock()
	{
		::LeaveCriticalSection(&m_CriticalSection);
	}

private:
	CRITICAL_SECTION m_CriticalSection;
};

class BlockLock
{
public:
	BlockLock(LocalLock &LockObj)
		: m_Lock(LockObj)
	{
		LockObj.Lock();
	}

	~BlockLock()
	{
		m_Lock.Unlock();
	}

private:
	LocalLock &m_Lock;
};

}	// namespace CV


#endif	// ndef CV_UTILITY_H
