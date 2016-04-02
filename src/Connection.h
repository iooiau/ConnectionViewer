/******************************************************************************
*                                                                             *
*    Connection.h                           Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_CONNECTION_H
#define CV_CONNECTION_H


#include <iphlpapi.h>
#include <vector>


namespace CV
{

enum class ConnectionProtocol
{
	TCP,
	UDP,
	TCP_V6,
	UDP_V6
};

enum class ConnectionState
{
	UNDEFINED,
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_RECEIVED,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT,
	DELETE_TCB
};

struct ConnectionInfo
{
	ConnectionProtocol Protocol;
	ConnectionState State;
	IPAddress LocalAddress;
	WORD LocalPort;
	IPAddress RemoteAddress;
	WORD RemotePort;
	DWORD PID;
	LONGLONG CreateTimestamp;
};

struct ConnectionStatistics
{
	enum {
		MASK_BYTES		= 0x0001,
		MASK_BANDWIDTH	= 0x0002
	};

	UINT Mask;
	ULONGLONG OutBytes;
	ULONGLONG InBytes;
	ULONGLONG OutBitsPerSecond;
	ULONGLONG InBitsPerSecond;
};

class ConnectionStatus
{
public:
	ConnectionStatus();
	~ConnectionStatus();
	bool Update();
	int NumConnections() const;
	int NumTCPConnections() const;
	int NumUDPConnections() const;
	bool GetConnectionInfo(int Index, ConnectionInfo *pInfo) const;
	bool GetConnectionStatistics(int Index, ConnectionStatistics *pStatistics) const;
	DWORD GetConnectionPID(int Index) const;

private:
	struct ConnectionInfoAndStatistics
	{
		ConnectionInfo Info;
		ConnectionStatistics Statistics;
	};

	bool AllocateBuffer(size_t Size);
	void ReserveList(size_t Size);
	bool GetStatistics(ConnectionInfoAndStatistics *pInfoAndStat);

	std::vector<ConnectionInfoAndStatistics> m_ConnectionList;
	int m_NumTCPConnections;
	int m_NumUDPConnections;
	BYTE *m_pBuffer;
	size_t m_BufferSize;
};

struct NetworkInterfaceStatistics
{
	ULONGLONG InBytes;
	ULONGLONG OutBytes;
};

class NetworkInterfaceStatus
{
public:
	NetworkInterfaceStatus();
	~NetworkInterfaceStatus();
	bool Update();
	void Clear();
	int NumInterfaces() const;
	const MIB_IF_ROW2 *GetInterfaceInfo(int Index) const;
	const MIB_IF_ROW2 *GetInterfaceInfo(const GUID &Guid) const;
	bool GetInterfaceStatistics(int Index, NetworkInterfaceStatistics *pStatistics) const;
	bool GetInterfaceStatistics(const GUID &Guid, NetworkInterfaceStatistics *pStatistics) const;
	bool GetTotalStatistics(NetworkInterfaceStatistics *pStatistics) const;

private:
	MIB_IF_TABLE2 *m_pTable;
};

LPCTSTR GetProtocolText(ConnectionProtocol Protocol);
LPCTSTR GetConnectionStateText(ConnectionState State);

}	// namespace CV


#endif	// CV_CONNECTION_H
