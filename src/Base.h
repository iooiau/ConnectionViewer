/******************************************************************************
*                                                                             *
*    Base.h                                 Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_BASE_H
#define CV_BASE_H


namespace CV
{

#define CV_IP_ADDRESS_V4(a,b,c,d) \
	((DWORD)(a) | ((DWORD)(b)<<8) | ((DWORD)(c)<<16) | ((DWORD)(d)<<24))

struct IPv4Address
{
	union {
		BYTE Bytes[4];
		DWORD Address;
	};

	IPv4Address() : Address(0) {}
	IPv4Address(DWORD Addr) : Address(Addr) {}

	DWORD NetworkValue() const
	{
		return Address;
	}

	DWORD HostValue() const
	{
		return ((DWORD)Bytes[0] << 24) | ((DWORD)Bytes[1] << 16) |
			   ((DWORD)Bytes[2] << 8) | (DWORD)Bytes[3];
	}

	bool operator==(const IPv4Address &RVal) const
	{
		return Address == RVal.Address;
	}

	bool operator!=(const IPv4Address &RVal) const
	{
		return Address != RVal.Address;
	}

	bool operator<(const IPv4Address &RVal) const
	{
		return HostValue() < RVal.HostValue();
	}

	bool operator>(const IPv4Address &RVal) const
	{
		return HostValue() > RVal.HostValue();
	}

	bool operator<=(const IPv4Address &RVal) const
	{
		return HostValue() <= RVal.HostValue();
	}

	bool operator>=(const IPv4Address &RVal) const
	{
		return HostValue() >= RVal.HostValue();
	}
};

struct IPv6Address
{
	union {
		BYTE Bytes[16];
		WORD Words[8];
		DWORD DWords[4];
#ifdef _WIN64
		ULONGLONG QWords[2];
#endif
	};
	DWORD ScopeID;

	IPv6Address &operator=(const IPv6Address &RVal)
	{
#ifdef _WIN64
		QWords[0] = RVal.QWords[0];
		QWords[1] = RVal.QWords[1];
#else
		DWords[0] = RVal.DWords[0];
		DWords[1] = RVal.DWords[1];
		DWords[2] = RVal.DWords[2];
		DWords[3] = RVal.DWords[3];
#endif
		ScopeID = RVal.ScopeID;
		return *this;
	}

	bool operator==(const IPv6Address &RVal) const
	{
		return std::memcmp(Bytes, RVal.Bytes, 16) == 0
			&& ScopeID == RVal.ScopeID;
	}

	bool operator!=(const IPv6Address &RVal) const
	{
		return !(*this == RVal);
	}

	bool operator<(const IPv6Address &RVal) const
	{
		int Cmp = std::memcmp(Bytes, RVal.Bytes, 16);
		return Cmp < 0 || (Cmp == 0 && ScopeID < RVal.ScopeID);
	}

	bool operator>(const IPv6Address &RVal) const
	{
		int Cmp = std::memcmp(Bytes, RVal.Bytes, 16);
		return Cmp > 0 || (Cmp == 0 && ScopeID > RVal.ScopeID);
	}

	bool operator<=(const IPv6Address &RVal) const
	{
		return !(*this > RVal);
	}

	bool operator>=(const IPv6Address &RVal) const
	{
		return !(*this < RVal);
	}

	void SetAddress(const BYTE pAddress[16])
	{
		if (pAddress != nullptr)
			std::memcpy(Bytes, pAddress, 16);
		else
			std::memset(Bytes, 0, 16);
	}

	bool IsUnspecified() const
	{
#ifdef _WIN64
		return QWords[0] == 0 && QWords[1] == 0;
#else
		return DWords[0] == 0 && DWords[1] == 0 && DWords[2] == 0 && DWords[3] == 0;
#endif
	}
};

enum IPAddressType
{
	IP_ADDRESS_V4,
	IP_ADDRESS_V6
};

struct IPAddress
{
	IPAddressType Type;
	IPv4Address V4;
	IPv6Address V6;

	IPAddress() {}

	IPAddress(const IPv4Address &Src)
		: Type(IP_ADDRESS_V4)
	{
		V4 = Src;
	}

	IPAddress(const IPv6Address &Src)
		: Type(IP_ADDRESS_V6)
	{
		V6 = Src;
	}

	void SetV4Address(DWORD Address)
	{
		Type = IP_ADDRESS_V4;
		V4.Address = Address;
	}

	void SetV6Address(const BYTE pAddress[16], DWORD ScopeID = 0)
	{
		Type = IP_ADDRESS_V6;
		V6.SetAddress(pAddress);
		V6.ScopeID = ScopeID;
	}

	bool IsZero() const
	{
		return (Type == IP_ADDRESS_V4 && V4.Address == 0)
			|| (Type == IP_ADDRESS_V6 && V6.IsUnspecified());
	}

	bool IsLocal() const;

	bool Parse(LPCTSTR pText);

	bool operator==(const IPAddress &RVal) const
	{
		if (Type == RVal.Type) {
			if (Type == IP_ADDRESS_V4)
				return V4 == RVal.V4;
			return V6 == RVal.V6;
		}
		return false;
	}

	bool operator!=(const IPAddress &RVal) const
	{
		return !(*this == RVal);
	}

	bool operator<(const IPAddress &RVal) const
	{
		if (Type != RVal.Type)
			return Type < RVal.Type;
		if (Type == IP_ADDRESS_V4)
			return V4 < RVal.V4;
		return V6 < RVal.V6;
	}

	bool operator>(const IPAddress &RVal) const
	{
		if (Type != RVal.Type)
			return Type > RVal.Type;
		if (Type == IP_ADDRESS_V4)
			return V4 > RVal.V4;
		return V6 > RVal.V6;
	}

	bool operator>=(const IPAddress &RVal) const
	{
		return !(*this < RVal);
	}

	bool operator<=(const IPAddress &RVal) const
	{
		return !(*this > RVal);
	}
};

int FormatIPv4Address(DWORD Address, LPTSTR pText, int MaxLength);
int FormatIPv6Address(const BYTE *pAddress, LPTSTR pText, int MaxLength);
int FormatIPAddress(const IPAddress &Address, LPTSTR pText, int MaxLength);

void FormatBytes(ULONGLONG Bytes, LPTSTR pText, int MaxLength);
void FormatBandwidth(int Value, LPTSTR pText, int MaxLength);

struct TimeAndTick
{
	FILETIME Time;
	ULONGLONG Tick;

	TimeAndTick()
	{
		Reset();
	}

	void Reset()
	{
		Time.dwLowDateTime = 0;
		Time.dwHighDateTime = 0;
		Tick = 0;
	}

	void SetCurrent()
	{
		::GetSystemTimeAsFileTime(&Time);
		Tick = ::GetTickCount64();
	}
};

}	// namespace CV


#endif	// ndef CV_BASE_H
