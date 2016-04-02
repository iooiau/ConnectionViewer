/******************************************************************************
*                                                                             *
*    Base.cpp                               Copyright(c) 2010-2016 itow,y.    *
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
#include "Utility.h"


namespace CV
{

bool IPAddress::IsLocal() const
{
	if (Type == IP_ADDRESS_V4) {
		if (V4.Address == CV_IP_ADDRESS_V4(127, 0, 0, 1))
			return true;
	} else if (Type == IP_ADDRESS_V6) {
		for (int i = 0; i < 15; i++) {
			if (V6.Bytes[i] != 0)
				return false;
		}
		if (V6.Bytes[15] == 1)
			return true;
	}
	return false;
}

bool IPAddress::Parse(LPCTSTR pText)
{
	LPCTSTR p = pText;

	if (::StrChr(pText, _T(':')) != nullptr) {
		BYTE Address[16];

		for (int i = 0; i < 8; i++) {
			if (i == 6 && ::StrChr(p, _T('.')) != nullptr) {
				IPAddress V4Address;

				if (!V4Address.Parse(p) || V4Address.Type != IP_ADDRESS_V4)
					return false;
				::CopyMemory(Address + 12, V4Address.V4.Bytes, 4);
				break;
			}

			unsigned int Value = 0;
			while (true) {
				if (*p >= _T('0') && *p <= _T('9'))
					Value = (Value << 4) | (*p - _T('0'));
				else if (*p >= _T('a') && *p <= _T('f'))
					Value = (Value << 4) | (*p - _T('a'));
				else if (*p >= _T('A') && *p <= _T('F'))
					Value = (Value << 4) | (*p - _T('A'));
				else
					return false;
				p++;
			}
			if (Value > 0xFFFF)
				return false;
			Address[i * 2 + 0] = (BYTE)(Value >> 8);
			Address[i * 2 + 1] = (BYTE)(Value & 0xFF);
			if (i < 7) {
				if (*p != _T(':'))
					return false;
				p++;
			}
		}
		Type = IP_ADDRESS_V6;
		::CopyMemory(V6.Bytes, Address, 16);
	} else {
		BYTE Address[4];

		for (int i = 0; i < 4; i++) {
			unsigned int Value = 0;
			while (*p >= _T('0') && *p <= _T('9')) {
				Value = Value * 10 + (*p - _T('0'));
				p++;
			}
			if (Value > 255)
				return false;
			Address[i] = (BYTE)Value;
			if (i < 3) {
				if (*p != _T('.'))
					return false;
				p++;
			}
		}
		Type = IP_ADDRESS_V4;
		::CopyMemory(V4.Bytes, Address, 4);
	}

	return true;
}


int FormatIPv4Address(DWORD Address, LPTSTR pText, int MaxLength)
{
	return FormatString(pText, MaxLength, TEXT("%u.%u.%u.%u"),
						Address & 0xFF,
						(Address >> 8) & 0xFF,
						(Address >> 16) & 0xFF,
						(Address >> 24) & 0xFF);
}

bool IsIPv4CompatibleAddress(const BYTE *pAddress)
{
	return pAddress[0] == 0 && pAddress[1] == 0
		&& pAddress[2] == 0 && pAddress[3] == 0
		&& pAddress[4] == 0 && pAddress[5] == 0
		&& pAddress[6] == 0 && pAddress[7] == 0
		&& pAddress[8] == 0 && pAddress[9] == 0
		&& ((pAddress[10] == 0xFF && pAddress[11] == 0xFF)
			|| ((pAddress[10] == 0 && pAddress[11] == 0)
				&& (pAddress[12] != 0 || pAddress[13] != 0 || pAddress[14] != 0
					|| (pAddress[15] != 0 && pAddress[15] != 1))));
}

int FormatIPv6Address(const BYTE *pAddress, LPTSTR pText, int MaxLength)
{
	if (IsIPv4CompatibleAddress(pAddress))
		return FormatString(pText, MaxLength, TEXT("0:0:0:0:0:%x:%u.%u.%u.%u"),
							(pAddress[10] << 8) | pAddress[11],
							pAddress[12], pAddress[13], pAddress[14], pAddress[15]);

	return FormatString(pText, MaxLength, TEXT("%x:%x:%x:%x:%x:%x:%x:%x"),
						(pAddress[ 0] << 8) | pAddress[ 1],
						(pAddress[ 2] << 8) | pAddress[ 3],
						(pAddress[ 4] << 8) | pAddress[ 5],
						(pAddress[ 6] << 8) | pAddress[ 7],
						(pAddress[ 8] << 8) | pAddress[ 9],
						(pAddress[10] << 8) | pAddress[11],
						(pAddress[12] << 8) | pAddress[13],
						(pAddress[14] << 8) | pAddress[15]);
}

int FormatIPAddress(const IPAddress &Address, LPTSTR pText, int MaxLength)
{
	if (Address.Type == IP_ADDRESS_V4)
		return FormatIPv4Address(Address.V4.Address, pText, MaxLength);
	if (Address.Type == IP_ADDRESS_V6)
		return FormatIPv6Address(Address.V6.Bytes, pText, MaxLength);
	if (MaxLength > 0)
		pText[0] = _T('\0');
	return 0;
}

void FormatBytes(ULONGLONG Bytes, LPTSTR pText, int MaxLength)
{
	int Length;

	if (Bytes < 1024) {
		Length = FormatUInt64(Bytes, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" Bytes"));
	} else if (Bytes < 1024 * 1024) {
		Length = FormatDecimalInt64(Bytes, 1024, 2, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" KB"));
	} else if (Bytes < 1024 * 1024 * 1024) {
		Length = FormatDecimalInt64(Bytes, 1024 * 1024, 3, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" MB"));
	} else {
		Length = FormatDecimalInt64(Bytes, 1024 * 1024 * 1024, 3, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" GB"));
	}
}

void FormatBandwidth(int Value, LPTSTR pText, int MaxLength)
{
	int Length;

	if (Value < 1024) {
		Length = FormatInt(Value, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" B/s"));
	} else if (Value < 1024 * 1024) {
		Length = FormatDecimalInt(Value, 1024, 2, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" KB/s"));
	} else {
		Length = FormatDecimalInt(Value, 1024 * 1024, 3, pText, MaxLength);
		if (Length + 1 < MaxLength)
			CopyString(pText + Length, MaxLength - Length, TEXT(" MB/s"));
	}
}

}	// namespace CV
