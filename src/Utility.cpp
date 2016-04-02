/******************************************************************************
*                                                                             *
*    Utility.cpp                            Copyright(c) 2010-2016 itow,y.    *
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
#include "Utility.h"
#include <cwchar>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")


namespace CV
{

LPSTR DuplicateString(LPCSTR pString)
{
	if (pString == nullptr)
		return nullptr;

	int Length = ::lstrlenA(pString);
	LPSTR pNewString = new char[Length + 1];
	std::memcpy(pNewString, pString, Length + 1);
	return pNewString;
}

LPWSTR DuplicateString(LPCWSTR pString)
{
	if (pString == nullptr)
		return nullptr;

	int Length = ::lstrlenW(pString);
	LPWSTR pNewString = new WCHAR[Length + 1];
	std::memcpy(pNewString, pString, (Length + 1) * sizeof(WCHAR));
	return pNewString;
}

void ReplaceChar(LPTSTR pString, TCHAR From, TCHAR To)
{
	if (pString == nullptr)
		return;

	for (LPTSTR p = pString; *p != _T('\0'); p++) {
		if (*p == From)
			*p = To;
#ifndef UNICODE
		else if (::IsDBCSLeadByteEx(CP_ACP, *p) && *(p + 1) != '\0')
			p++;
#endif
	}
}

int StrToInt(LPCTSTR pszValue, int Base)
{
	return std::_tcstol(pszValue, nullptr, Base);
}

unsigned int StrToUInt(LPCTSTR pszValue, int Base)
{
	return std::_tcstoul(pszValue, nullptr, Base);
}

INT64 StrToInt64(LPCTSTR pszValue, int Base)
{
	return std::_tcstoll(pszValue, nullptr, Base);
}

UINT64 StrToUInt64(LPCTSTR pszValue, int Base)
{
	return std::_tcstoull(pszValue, nullptr, Base);
}

int IntToStr(int Value, LPTSTR pszString, int MaxLength)
{
	return FormatString(pszString, MaxLength, TEXT("%d"), Value);
}

int UIntToStr(unsigned int Value, LPTSTR pszString, int MaxLength)
{
	return FormatString(pszString, MaxLength, TEXT("%u"), Value);
}

int Int64ToStr(LONGLONG Value, LPTSTR pszString, int MaxLength)
{
	return FormatString(pszString, MaxLength, TEXT("%lld"), Value);
}

int UInt64ToStr(ULONGLONG Value, LPTSTR pszString, int MaxLength)
{
	return FormatString(pszString, MaxLength, TEXT("%llu"), Value);
}

static int FormatNumberString(LPCTSTR pszValue, LPTSTR pszNumber, int MaxLength)
{
	TCHAR szDecimal[4];
	LPTSTR p;

	if (::GetNumberFormat(LOCALE_USER_DEFAULT, 0, pszValue, nullptr, pszNumber, MaxLength) <= 0) {
		pszNumber[0] = _T('\0');
		return 0;
	}
	::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimal, cvLengthOf(szDecimal));
	for (p = pszNumber; *p != _T('\0'); p++) {
		if (*p == szDecimal[0]) {
			*p = _T('\0');
			break;
		}
	}
	return (int)(p - pszNumber);
}

int FormatInt(int Value, LPTSTR pText, int MaxLength)
{
	TCHAR szValue[24];

	IntToStr(Value, szValue, cvLengthOf(szValue));
	return FormatNumberString(szValue, pText, MaxLength);
}

int FormatUInt(unsigned int Value, LPTSTR pText, int MaxLength)
{
	TCHAR szValue[24];

	UIntToStr(Value, szValue, cvLengthOf(szValue));
	return FormatNumberString(szValue, pText, MaxLength);
}

int FormatInt64(LONGLONG Value, LPTSTR pText, int MaxLength)
{
	TCHAR szValue[32];

	Int64ToStr(Value, szValue, cvLengthOf(szValue));
	return FormatNumberString(szValue, pText, MaxLength);
}

int FormatUInt64(ULONGLONG Value, LPTSTR pText, int MaxLength)
{
	TCHAR szValue[32];

	UInt64ToStr(Value, szValue, cvLengthOf(szValue));
	return FormatNumberString(szValue, pText, MaxLength);
}

int FormatDecimalInt(int Value, int Denom, int Digits, LPTSTR pszNumber, int MaxLength)
{
	int Decimal;
	LONGLONG Value64;

	Decimal = 1;
	for (int i = Digits; i > 0; i--)
		Decimal *= 10;
	Value64 = ((LONGLONG)Value * Decimal + Denom / 2) / Denom;
	int Length = FormatInt((int)(Value64 / Decimal), pszNumber, MaxLength);
	if (Digits > 0) {
		TCHAR szText[16];

		if (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szText, cvLengthOf(szText)) <= 0)
			::lstrcpy(szText, TEXT("."));
		Length += FormatString(pszNumber + Length, MaxLength - Length,
							   TEXT("%s%0*d"),
							   szText, Digits, (int)(Value64 % Decimal));
	}
	return Length;
}

int FormatDecimalInt64(LONGLONG Value, LONGLONG Denom, int Digits, LPTSTR pszNumber, int MaxLength)
{
	int Decimal;
	LONGLONG Value64;

	Decimal = 1;
	for (int i = Digits; i > 0; i--)
		Decimal *= 10;
	Value64 = (Value * Decimal + Denom / 2) / Denom;
	int Length = FormatInt64(Value64 / Decimal, pszNumber, MaxLength);
	if (Digits > 0) {
		TCHAR szText[16];

		if (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szText, cvLengthOf(szText)) <= 0)
			::lstrcpy(szText, TEXT("."));
		Length += FormatString(pszNumber + Length, MaxLength - Length,
							   TEXT("%s%0*d"),
							   szText, Digits, (int)(Value64 % Decimal));
	}
	return Length;
}

int FormatString(LPTSTR pDest, int DestLength, LPCTSTR pFormat, ...)
{
	if (DestLength <= 0)
		return 0;

	va_list Args;

	va_start(Args, pFormat);
	LPTSTR pEnd = pDest;
	StringCchVPrintfEx(pDest, DestLength, &pEnd, nullptr, STRSAFE_IGNORE_NULLS, pFormat, Args);
	va_end(Args);
	return (int)(pEnd - pDest);
}

int FormatStringV(LPTSTR pDest, int DestLength, LPCTSTR pFormat, va_list Args)
{
	if (DestLength <= 0)
		return 0;

	LPTSTR pEnd = pDest;
	StringCchVPrintfEx(pDest, DestLength, &pEnd, nullptr, STRSAFE_IGNORE_NULLS, pFormat, Args);
	return (int)(pEnd - pDest);
}

void CopyString(LPTSTR pDest, int DestLength, LPCTSTR pSrc)
{
	if (DestLength <= 0)
		return;

	StringCchCopyEx(pDest, DestLength, pSrc, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
}

int FormatSystemTime(const SYSTEMTIME &Time, unsigned int Flags, LPTSTR pText, int MaxLength)
{
	if (pText == nullptr || MaxLength <= 0)
		return 0;

	int Length = ::GetDateFormat(
		LOCALE_USER_DEFAULT,
		(Flags & SYSTEMTIME_FORMAT_LONG) != 0 ? DATE_LONGDATE : DATE_SHORTDATE,
		&Time, nullptr, pText, MaxLength);
	if (Length > 0)
		Length--;
	if ((Flags & SYSTEMTIME_FORMAT_TIME) != 0) {
		if (Length < MaxLength - 2) {
			pText[Length++] = _T(' ');
			Length += ::GetTimeFormat(
				LOCALE_USER_DEFAULT,
				TIME_FORCE24HOURFORMAT |
					((Flags & SYSTEMTIME_FORMAT_SECONDS) != 0 ? 0 : TIME_NOSECONDS),
				&Time, nullptr, pText + Length, MaxLength - Length);
		}
	}
	pText[Length] = _T('\0');
	return Length;
}

int FormatBytesLong(ULONGLONG Bytes, LPTSTR pText, int MaxLength)
{
	int Length = FormatUInt64(Bytes, pText, MaxLength);
	if (Bytes >= 1024) {
		TCHAR szTemp[32];
		FormatBytes(Bytes, szTemp, cvLengthOf(szTemp));
		Length += FormatString(pText + Length, MaxLength - Length, TEXT(" (%s)"), szTemp);
	}
	return Length;
}

int FormatBandwidthLong(ULONGLONG BytesPerSecond, LPTSTR pText, int MaxLength)
{
	int Length = FormatUInt64(BytesPerSecond, pText, MaxLength);
	if (BytesPerSecond >= 1024) {
		TCHAR szBps[32];
		FormatBandwidth((int)BytesPerSecond, szBps, cvLengthOf(szBps));
		Length += FormatString(pText + Length, MaxLength - Length, TEXT(" (%s)"), szBps);
	}
	return Length;
}

int GetSystemErrorMessage(DWORD Code, LPTSTR pText, int MaxLength)
{
	return ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		Code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		pText, MaxLength, nullptr);
}

bool IsAdministratorsMember()
{
	bool Admin = false;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;

	if (::AllocateAndInitializeSid(&NtAuthority, 2,
								   SECURITY_BUILTIN_DOMAIN_RID,
								   DOMAIN_ALIAS_RID_ADMINS,
								   0, 0, 0, 0, 0, 0,
								   &AdministratorsGroup)) {
		BOOL b;
		if (::CheckTokenMembership(nullptr, AdministratorsGroup, &b) && b)
			Admin = true;
		::FreeSid(AdministratorsGroup);
	}
	return Admin;
}

char *ISO2022JPToShiftJIS(const char *pString)
{
	if (pString == nullptr)
		return nullptr;

	int Length = ::lstrlenA(pString);
	char *pBuffer = new char[Length * 2 + 1];

	int i = 0, j = 0;
	bool Kanji = false;
	while (i < Length) {
		if (pString[i] == '\x1b') {
			if (i + 2 >= Length)
				break;
			if (pString[i + 1] == '$'
					&& (pString[i + 2] == '@' || pString[i + 2] == 'B')) {
				Kanji = true;
				i += 3;
			} else if (pString[i + 1] == '('
					   && (pString[i + 2] == 'B' || pString[i + 2] == 'J')) {
				Kanji = false;
				i += 3;
			} else {
				i++;
			}
		} else {
			if (Kanji) {
				if (i + 1 >= Length)
					break;
				pBuffer[j] = ((pString[i] - 0x21) >> 1) + 0x81;
				if ((BYTE)pBuffer[j] > 0x9F)
					pBuffer[j] += 0x40;
				pBuffer[j + 1] = pString[i + 1] + ((pString[i] & 1) == 0 ? 0x7D : 0x1F);
				if ((BYTE)pBuffer[j + 1] >= 0x7F)
					pBuffer[j + 1]++;
				i += 2;
				j += 2;
			} else {
				if (pString[i] == '\r' || pString[i] == '\n') {
					pBuffer[j++] = '\r';
					pBuffer[j++] = '\n';
					if (pString[i] == '\r' && pString[i + 1] == '\n')
						i++;
					i++;
				} else {
					pBuffer[j++] = pString[i++];
				}
			}
		}
	}
	pBuffer[j] = '\0';
	return pBuffer;
}

int PixelsToPoints(HDC hdc, int Size)
{
	int LogPixels = ::GetDeviceCaps(hdc, LOGPIXELSY);

	if (LogPixels == 0)
		return 0;
	return (Size * 72 + LogPixels / 2) / LogPixels;
}

int CalcFontPointSize(HDC hdc, const LOGFONT &Font)
{
	HFONT hfont, hfontOld;
	TEXTMETRIC tm;
	int Size;

	if (hdc == nullptr)
		return 0;
	hfont = ::CreateFontIndirect(&Font);
	if (hfont == nullptr)
		return 0;
	hfontOld = static_cast<HFONT>(::SelectObject(hdc, hfont));
	::GetTextMetrics(hdc, &tm);
	Size = PixelsToPoints(hdc, tm.tmHeight - tm.tmInternalLeading);
	::SelectObject(hdc, hfontOld);
	::DeleteObject(hfont);
	return Size;
}

void MoveWindowToCenter(HWND hwnd)
{
	HWND hwndParent = ::GetParent(hwnd);
	if (hwndParent == nullptr)
		hwndParent = ::GetDesktopWindow();
	RECT rcWindow, rcParent;

	::GetWindowRect(hwnd, &rcWindow);
	::GetWindowRect(hwndParent, &rcParent);
	::OffsetRect(&rcWindow,
				 (rcParent.left + ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2) -
				 rcWindow.left,
				 (rcParent.top + ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2) -
				 rcWindow.top);

	HMONITOR hMonitor = ::MonitorFromWindow(hwndParent, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	::GetMonitorInfo(hMonitor, &mi);
	if (rcWindow.left < mi.rcWork.left)
		::OffsetRect(&rcWindow, mi.rcWork.left - rcWindow.left, 0);
	else if (rcWindow.right > mi.rcWork.right)
		::OffsetRect(&rcWindow, mi.rcWork.right - rcWindow.right, 0);
	if (rcWindow.top < mi.rcWork.top)
		::OffsetRect(&rcWindow, 0, mi.rcWork.top - rcWindow.top);
	else if (rcWindow.bottom > mi.rcWork.bottom)
		::OffsetRect(&rcWindow, 0, mi.rcWork.bottom - rcWindow.bottom);

	::SetWindowPos(hwnd, nullptr, rcWindow.left, rcWindow.top, 0, 0,
				   SWP_NOSIZE | SWP_NOZORDER);
}

}	// namespace CV
