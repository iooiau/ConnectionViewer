/******************************************************************************
*                                                                             *
*    Connection.cpp                         Copyright(c) 2010-2016 itow,y.    *
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
#include "Connection.h"

#pragma comment(lib, "iphlpapi.lib")


namespace CV
{

ConnectionStatus::ConnectionStatus()
	: m_NumTCPConnections(0)
	, m_NumUDPConnections(0)
	, m_pBuffer(nullptr)
	, m_BufferSize(0)
{
}

ConnectionStatus::~ConnectionStatus()
{
	delete [] m_pBuffer;
}

bool ConnectionStatus::Update()
{
	m_ConnectionList.clear();
	m_NumTCPConnections = 0;
	m_NumUDPConnections = 0;

	ConnectionInfoAndStatistics InfoAndStat;
	DWORD Size;

	Size = 0;
	if (::GetExtendedTcpTable(nullptr, &Size, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == ERROR_INSUFFICIENT_BUFFER
			&& Size > 0) {
		AllocateBuffer(Size);
		if (::GetExtendedTcpTable(m_pBuffer, &Size, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR) {
			const MIB_TCPTABLE_OWNER_MODULE *pTable =
				reinterpret_cast<const MIB_TCPTABLE_OWNER_MODULE*>(m_pBuffer);

			ReserveList(pTable->dwNumEntries);
			for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
				const MIB_TCPROW_OWNER_MODULE &Module = pTable->table[i];

				InfoAndStat.Info.Protocol = ConnectionProtocol::TCP;
				InfoAndStat.Info.State = (ConnectionState)Module.dwState;
				InfoAndStat.Info.LocalAddress.SetV4Address(Module.dwLocalAddr);
				InfoAndStat.Info.LocalPort = ::ntohs((u_short)Module.dwLocalPort);
				InfoAndStat.Info.RemoteAddress.SetV4Address(Module.dwRemoteAddr);
				if (InfoAndStat.Info.RemoteAddress.V4.Address != 0)
					InfoAndStat.Info.RemotePort = ::ntohs((u_short)Module.dwRemotePort);
				else
					InfoAndStat.Info.RemotePort = 0;
				InfoAndStat.Info.PID = Module.dwOwningPid;
				InfoAndStat.Info.CreateTimestamp = Module.liCreateTimestamp.QuadPart;
				GetStatistics(&InfoAndStat);
				m_ConnectionList.push_back(InfoAndStat);
			}
			m_NumTCPConnections += pTable->dwNumEntries;
		}
	}

	Size = 0;
	if (::GetExtendedTcpTable(nullptr, &Size, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == ERROR_INSUFFICIENT_BUFFER
			&& Size > 0) {
		AllocateBuffer(Size);
		if (::GetExtendedTcpTable(m_pBuffer, &Size, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR) {
			const MIB_TCP6TABLE_OWNER_MODULE *pTable =
				reinterpret_cast<const MIB_TCP6TABLE_OWNER_MODULE*>(m_pBuffer);

			ReserveList(pTable->dwNumEntries);
			for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
				const MIB_TCP6ROW_OWNER_MODULE &Module = pTable->table[i];

				InfoAndStat.Info.Protocol = ConnectionProtocol::TCP_V6;
				InfoAndStat.Info.State = (ConnectionState)Module.dwState;
				InfoAndStat.Info.LocalAddress.SetV6Address(Module.ucLocalAddr, Module.dwLocalScopeId);
				InfoAndStat.Info.LocalPort = ::ntohs((u_short)Module.dwLocalPort);
				InfoAndStat.Info.RemoteAddress.SetV6Address(Module.ucRemoteAddr, Module.dwRemoteScopeId);
				if (!InfoAndStat.Info.RemoteAddress.V6.IsUnspecified())
					InfoAndStat.Info.RemotePort = ::ntohs((u_short)Module.dwRemotePort);
				else
					InfoAndStat.Info.RemotePort = 0;
				InfoAndStat.Info.PID = Module.dwOwningPid;
				InfoAndStat.Info.CreateTimestamp = Module.liCreateTimestamp.QuadPart;
				GetStatistics(&InfoAndStat);
				m_ConnectionList.push_back(InfoAndStat);
			}
			m_NumTCPConnections += pTable->dwNumEntries;
		}
	}

	Size = 0;
	if (::GetExtendedUdpTable(nullptr, &Size, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0) == ERROR_INSUFFICIENT_BUFFER
			&& Size > 0) {
		AllocateBuffer(Size);
		if (::GetExtendedUdpTable(m_pBuffer, &Size, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR) {
			const MIB_UDPTABLE_OWNER_MODULE *pTable =
				reinterpret_cast<const MIB_UDPTABLE_OWNER_MODULE*>(m_pBuffer);

			ReserveList(pTable->dwNumEntries);
			for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
				const MIB_UDPROW_OWNER_MODULE &Module = pTable->table[i];

				InfoAndStat.Info.Protocol = ConnectionProtocol::UDP;
				InfoAndStat.Info.State = ConnectionState::UNDEFINED;
				InfoAndStat.Info.LocalAddress.SetV4Address(Module.dwLocalAddr);
				InfoAndStat.Info.LocalPort = ::ntohs((u_short)Module.dwLocalPort);
				InfoAndStat.Info.RemoteAddress.SetV4Address(0);
				InfoAndStat.Info.RemotePort = 0;
				InfoAndStat.Info.PID = Module.dwOwningPid;
				InfoAndStat.Info.CreateTimestamp = -1;
				InfoAndStat.Statistics.Mask = 0;
				m_ConnectionList.push_back(InfoAndStat);
			}
			m_NumUDPConnections += pTable->dwNumEntries;
		}
	}

	Size = 0;
	if (::GetExtendedUdpTable(nullptr, &Size, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0) == ERROR_INSUFFICIENT_BUFFER
			&& Size > 0) {
		AllocateBuffer(Size);
		if (::GetExtendedUdpTable(m_pBuffer, &Size, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR) {
			const MIB_UDP6TABLE_OWNER_MODULE *pTable =
				reinterpret_cast<const MIB_UDP6TABLE_OWNER_MODULE*>(m_pBuffer);

			ReserveList(pTable->dwNumEntries);
			for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
				const MIB_UDP6ROW_OWNER_MODULE &Module = pTable->table[i];

				InfoAndStat.Info.Protocol = ConnectionProtocol::UDP_V6;
				InfoAndStat.Info.State = ConnectionState::UNDEFINED;
				InfoAndStat.Info.LocalAddress.SetV6Address(Module.ucLocalAddr, Module.dwLocalScopeId);
				InfoAndStat.Info.LocalPort = ::ntohs((u_short)Module.dwLocalPort);
				InfoAndStat.Info.RemoteAddress.SetV6Address(nullptr);
				InfoAndStat.Info.RemotePort = 0;
				InfoAndStat.Info.PID = Module.dwOwningPid;
				InfoAndStat.Info.CreateTimestamp = -1;
				InfoAndStat.Statistics.Mask = 0;
				m_ConnectionList.push_back(InfoAndStat);
			}
			m_NumUDPConnections += pTable->dwNumEntries;
		}
	}

	return true;
}

int ConnectionStatus::NumConnections() const
{
	return (int)m_ConnectionList.size();
}

int ConnectionStatus::NumTCPConnections() const
{
	return m_NumTCPConnections;
}

int ConnectionStatus::NumUDPConnections() const
{
	return m_NumUDPConnections;
}

bool ConnectionStatus::GetConnectionInfo(int Index, ConnectionInfo *pInfo) const
{
	if (Index < 0 || (size_t)Index >= m_ConnectionList.size())
		return false;

	*pInfo = m_ConnectionList[Index].Info;

	return true;
}

bool ConnectionStatus::GetConnectionStatistics(int Index, ConnectionStatistics *pStatistics) const
{
	if (Index < 0 || (size_t)Index >= m_ConnectionList.size())
		return false;

	*pStatistics = m_ConnectionList[Index].Statistics;

	return true;
}

DWORD ConnectionStatus::GetConnectionPID(int Index) const
{
	if (Index < 0 || (size_t)Index >= m_ConnectionList.size())
		return 0;

	return m_ConnectionList[Index].Info.PID;
}

bool ConnectionStatus::AllocateBuffer(size_t Size)
{
	if (Size > m_BufferSize) {
		const size_t AllocateSize = (Size + 1023) / 1024 * 1024;

		delete [] m_pBuffer;
		m_pBuffer = new BYTE[AllocateSize];
		m_BufferSize = AllocateSize;
	}
	return true;
}

void ConnectionStatus::ReserveList(size_t Size)
{
	if (Size > 0) {
		const size_t CurSize = m_ConnectionList.size();
		const size_t NewSize = CurSize + Size;

		if (NewSize > m_ConnectionList.capacity())
			m_ConnectionList.reserve(NewSize);
	}
}

bool ConnectionStatus::GetStatistics(ConnectionInfoAndStatistics *pInfoAndStat)
{
	const ConnectionInfo &Info = pInfoAndStat->Info;
	ConnectionStatistics &Statistics = pInfoAndStat->Statistics;
	UINT Mask = 0;

	if (Info.Protocol == ConnectionProtocol::TCP) {
		if (Info.LocalAddress.Type != IP_ADDRESS_V4
				|| Info.RemoteAddress.Type != IP_ADDRESS_V4)
			return false;

		MIB_TCPROW Row;
		Row.dwState = (DWORD)Info.State;
		Row.dwLocalAddr = Info.LocalAddress.V4.Address;
		Row.dwLocalPort = ::htons(Info.LocalPort);
		Row.dwRemoteAddr = Info.RemoteAddress.V4.Address;
		Row.dwRemotePort = ::htons(Info.RemotePort);

		TCP_ESTATS_DATA_RW_v0 DataRW;
		DataRW.EnableCollection = TRUE;
		::SetPerTcpConnectionEStats(&Row, TcpConnectionEstatsData,
									(PUCHAR)&DataRW, 0, sizeof(DataRW), 0);
		TCP_ESTATS_DATA_ROD_v0 Data;
		if (::GetPerTcpConnectionEStats(&Row, TcpConnectionEstatsData,
										(PUCHAR)&DataRW, 0, sizeof(DataRW),
										nullptr, 0, 0,
										(PUCHAR)&Data, 0, sizeof(Data)) == NO_ERROR
				&& DataRW.EnableCollection) {
			Statistics.OutBytes = Data.DataBytesOut;
			Statistics.InBytes = Data.DataBytesIn;
			Mask |= ConnectionStatistics::MASK_BYTES;
		}

		TCP_ESTATS_BANDWIDTH_RW_v0 BandwidthRW;
		BandwidthRW.EnableCollectionOutbound = TcpBoolOptEnabled;
		BandwidthRW.EnableCollectionInbound = TcpBoolOptEnabled;
		::SetPerTcpConnectionEStats(&Row, TcpConnectionEstatsBandwidth,
									(PUCHAR)&BandwidthRW, 0, sizeof(BandwidthRW), 0);
		TCP_ESTATS_BANDWIDTH_ROD_v0 Bandwidth;
		if (::GetPerTcpConnectionEStats(&Row, TcpConnectionEstatsBandwidth,
										(PUCHAR)&BandwidthRW, 0, sizeof(BandwidthRW),
										nullptr, 0, 0,
										(PUCHAR)&Bandwidth, 0, sizeof(Bandwidth)) == NO_ERROR
				&& BandwidthRW.EnableCollectionOutbound == TcpBoolOptEnabled
				&& BandwidthRW.EnableCollectionInbound == TcpBoolOptEnabled) {
			//Statistics.OutBitsPerSecond=Bandwidth.OutboundBandwidth;
			//Statistics.InBitsPerSecond=Bandwidth.InboundBandwidth;
			Statistics.OutBitsPerSecond = Bandwidth.OutboundInstability;
			Statistics.InBitsPerSecond = Bandwidth.InboundInstability;
			Mask |= ConnectionStatistics::MASK_BANDWIDTH;
		}
	} else if (Info.Protocol == ConnectionProtocol::TCP) {
		if (Info.LocalAddress.Type != IP_ADDRESS_V6
				|| Info.RemoteAddress.Type != IP_ADDRESS_V6)
			return false;

		MIB_TCP6ROW Row;
		Row.State = (MIB_TCP_STATE)Info.State;
		::memcpy(Row.LocalAddr.u.Byte, Info.LocalAddress.V6.Bytes, 16);
		Row.dwLocalScopeId = Info.LocalAddress.V6.ScopeID;
		Row.dwLocalPort = ::htons(Info.LocalPort);
		::memcpy(Row.RemoteAddr.u.Byte, Info.RemoteAddress.V6.Bytes, 16);
		Row.dwRemoteScopeId = Info.RemoteAddress.V6.ScopeID;
		Row.dwRemotePort = ::htons(Info.RemotePort);

		TCP_ESTATS_DATA_RW_v0 DataRW;
		DataRW.EnableCollection = TRUE;
		::SetPerTcp6ConnectionEStats(&Row, TcpConnectionEstatsData,
									 (PUCHAR)&DataRW, 0, sizeof(DataRW), 0);
		TCP_ESTATS_DATA_ROD_v0 Data;
		if (::GetPerTcp6ConnectionEStats(&Row, TcpConnectionEstatsData,
										 (PUCHAR)&DataRW, 0, sizeof(DataRW),
										 nullptr, 0, 0,
										 (PUCHAR)&Data, 0, sizeof(Data)) == NO_ERROR
				&& DataRW.EnableCollection) {
			Statistics.OutBytes = Data.DataBytesOut;
			Statistics.InBytes = Data.DataBytesIn;
			Mask |= ConnectionStatistics::MASK_BYTES;
		}

		TCP_ESTATS_BANDWIDTH_RW_v0 BandwidthRW;
		BandwidthRW.EnableCollectionOutbound = TcpBoolOptEnabled;
		BandwidthRW.EnableCollectionInbound = TcpBoolOptEnabled;
		::SetPerTcp6ConnectionEStats(&Row, TcpConnectionEstatsBandwidth,
									 (PUCHAR)&BandwidthRW, 0, sizeof(BandwidthRW), 0);
		TCP_ESTATS_BANDWIDTH_ROD_v0 Bandwidth;
		if (::GetPerTcp6ConnectionEStats(&Row, TcpConnectionEstatsBandwidth,
										 (PUCHAR)&BandwidthRW, 0, sizeof(BandwidthRW),
										 nullptr, 0, 0,
										 (PUCHAR)&Bandwidth, 0, sizeof(Bandwidth)) == NO_ERROR
				&& BandwidthRW.EnableCollectionOutbound == TcpBoolOptEnabled
				&& BandwidthRW.EnableCollectionInbound == TcpBoolOptEnabled) {
			//Statistics.OutBitsPerSecond=Bandwidth.OutboundBandwidth;
			//Statistics.InBitsPerSecond=Bandwidth.InboundBandwidth;
			Statistics.OutBitsPerSecond = Bandwidth.OutboundInstability;
			Statistics.InBitsPerSecond = Bandwidth.InboundInstability;
			Mask |= ConnectionStatistics::MASK_BANDWIDTH;
		}
	} else {
		return false;
	}
	Statistics.Mask = Mask;
	return true;
}


NetworkInterfaceStatus::NetworkInterfaceStatus()
	: m_pTable(nullptr)
{
}

NetworkInterfaceStatus::~NetworkInterfaceStatus()
{
	Clear();
}

bool NetworkInterfaceStatus::Update()
{
	Clear();

	if (::GetIfTable2(&m_pTable) != NO_ERROR) {
		m_pTable = nullptr;
		return false;
	}

	return true;
}

void NetworkInterfaceStatus::Clear()
{
	if (m_pTable != nullptr) {
		::FreeMibTable(m_pTable);
		m_pTable = nullptr;
	}
}

int NetworkInterfaceStatus::NumInterfaces() const
{
	if (m_pTable == nullptr)
		return 0;
	return (int)m_pTable->NumEntries;
}

const MIB_IF_ROW2 *NetworkInterfaceStatus::GetInterfaceInfo(int Index) const
{
	if (m_pTable == nullptr  || Index < 0 || (ULONG)Index >= m_pTable->NumEntries)
		return false;
	return &m_pTable->Table[Index];
}

const MIB_IF_ROW2 *NetworkInterfaceStatus::GetInterfaceInfo(const GUID &Guid) const
{
	if (m_pTable != nullptr) {
		for (ULONG i = 0; i < m_pTable->NumEntries; i++) {
			const MIB_IF_ROW2 &Row = m_pTable->Table[i];

			if (Row.InterfaceGuid == Guid)
				return &Row;
		}
	}
	return nullptr;
}

bool NetworkInterfaceStatus::GetInterfaceStatistics(int Index, NetworkInterfaceStatistics *pStatistics) const
{
	if (m_pTable == nullptr || Index < 0 || (ULONG)Index >= m_pTable->NumEntries)
		return false;
	const MIB_IF_ROW2 &Row = m_pTable->Table[Index];
	pStatistics->InBytes = Row.InOctets;
	pStatistics->OutBytes = Row.OutOctets;
	return true;
}

bool NetworkInterfaceStatus::GetInterfaceStatistics(const GUID &Guid, NetworkInterfaceStatistics *pStatistics) const
{
	if (m_pTable != nullptr) {
		for (ULONG i = 0; i < m_pTable->NumEntries; i++) {
			const MIB_IF_ROW2 &Row = m_pTable->Table[i];

			if (Row.InterfaceGuid == Guid) {
				pStatistics->InBytes = Row.InOctets;
				pStatistics->OutBytes = Row.OutOctets;
				return true;
			}
		}
	}
	return false;
}

bool NetworkInterfaceStatus::GetTotalStatistics(NetworkInterfaceStatistics *pStatistics) const
{
	if (m_pTable == nullptr)
		return false;

	NetworkInterfaceStatistics Stats;
	Stats.InBytes = 0;
	Stats.OutBytes = 0;

	for (ULONG i = 0; i < m_pTable->NumEntries; i++) {
		const MIB_IF_ROW2 &Row = m_pTable->Table[i];

		if (Row.Type != IF_TYPE_SOFTWARE_LOOPBACK
				&& Row.InterfaceAndOperStatusFlags.HardwareInterface) {
			Stats.InBytes += Row.InOctets;
			Stats.OutBytes += Row.OutOctets;
		}
	}

	*pStatistics = Stats;

	return true;
}


LPCTSTR GetProtocolText(ConnectionProtocol Protocol)
{
	switch (Protocol) {
	case ConnectionProtocol::TCP:		return TEXT("TCP");
	case ConnectionProtocol::UDP:		return TEXT("UDP");
	case ConnectionProtocol::TCP_V6:	return TEXT("TCP(v6)");
	case ConnectionProtocol::UDP_V6:	return TEXT("UDP(v6)");
	}
	return TEXT("");
}

LPCTSTR GetConnectionStateText(ConnectionState State)
{
	static const LPCTSTR StateList[] = {
		TEXT(""),
		TEXT("CLOSED"),
		TEXT("LISTEN"),
		TEXT("SYN-SENT"),
		TEXT("SYN-RECEIVED"),
		TEXT("ESTABLISHED"),
		TEXT("FIN-WAIT-1"),
		TEXT("FIN-WAIT-2"),
		TEXT("CLOSE-WAIT"),
		TEXT("CLOSING"),
		TEXT("LAST-ACK"),
		TEXT("TIME-WAIT"),
		TEXT("DELETE TCB"),
	};

	if (State < ConnectionState::UNDEFINED)
		return TEXT("");
	if ((int)State >= cvLengthOf(StateList))
		return TEXT("?");
	return StateList[(int)State];
}


}	// namespace CV
