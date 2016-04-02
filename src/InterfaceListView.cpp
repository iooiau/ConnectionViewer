/******************************************************************************
*                                                                             *
*    InterfaceListView.cpp                  Copyright(c) 2010-2016 itow,y.    *
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
#include <algorithm>
#include "InterfaceListView.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

LPCTSTR InterfaceListView::GetColumnName(int Column)
{
	static const LPCTSTR ColumnNameList[] = {
		TEXT("InterfaceLUID"),
		TEXT("InterfaceIndex"),
		TEXT("InterfaceGUID"),
		TEXT("Alias"),
		TEXT("Description"),
		TEXT("PhysicalAddress"),
		TEXT("PermanentPhysicalAddress"),
		TEXT("MTU"),
		TEXT("Type"),
		TEXT("TunnelType"),
		TEXT("MediaType"),
		TEXT("PhysicalMediumType"),
		TEXT("AccessType"),
		TEXT("DirectionType"),
		TEXT("OperationalStatus"),
		TEXT("AdministrativeStatus"),
		TEXT("MediaConnectionState"),
		TEXT("NetworkGUID"),
		TEXT("ConnectionType"),
		TEXT("TransmitLinkSpeed"),
		TEXT("ReceiveLinkSpeed"),
		TEXT("InOctets"),
		TEXT("InBandwidth"),
		TEXT("MaxInBandwidth"),
		TEXT("InUnicastPackets"),
		TEXT("InNonUnicastPackets"),
		TEXT("InDiscardedPackets"),
		TEXT("InErrorPackets"),
		TEXT("InUnknownProtocolPackets"),
		TEXT("InUnicastOctets"),
		TEXT("InMulticastOctets"),
		TEXT("InBroadcastOctets"),
		TEXT("OutOctets"),
		TEXT("OutBandwidth"),
		TEXT("MaxOutBandwidth"),
		TEXT("OutUnicastPackets"),
		TEXT("OutNonUnicastPackets"),
		TEXT("OutDiscardedPackets"),
		TEXT("OutErrorPackets"),
		TEXT("OutUnicastOctets"),
		TEXT("OutMulticastOctets"),
		TEXT("OutBroadcastOctets"),
	};

	cvStaticAssert(cvLengthOf(ColumnNameList) == NUM_COLUMN_TYPES);

	if (Column < 0 || Column >= cvLengthOf(ColumnNameList))
		return nullptr;
	return ColumnNameList[Column];
}

InterfaceListView::InterfaceListView(const ProgramCore &Core)
	: m_Core(Core)
	, m_UpdatedTickCount(0)
{
	static const struct {
		int ID;
		ColumnAlign Align;
		bool Visible;
		int Width;
	} ColumnList[] = {
		{COLUMN_INTERFACE_INDEX,				COLUMN_ALIGN_RIGHT,		true,	2},
		{COLUMN_DESCRIPTION,					COLUMN_ALIGN_LEFT,		true,	15},
		{COLUMN_ALIAS,							COLUMN_ALIGN_LEFT,		false,	12},
		{COLUMN_INTERFACE_LUID,					COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_INTERFACE_GUID,					COLUMN_ALIGN_LEFT,		false,	10},
		{COLUMN_PHYSICAL_ADDRESS,				COLUMN_ALIGN_CENTER,	true,	10},
		{COLUMN_PERMANENT_PHYSICAL_ADDRESS,		COLUMN_ALIGN_CENTER,	false,	10},
		{COLUMN_MTU,							COLUMN_ALIGN_RIGHT,		false,	3},
		{COLUMN_TYPE,							COLUMN_ALIGN_LEFT,		true,	10},
		{COLUMN_TUNNEL_TYPE,					COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_MEDIA_TYPE,						COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_PHYSICAL_MEDIUM_TYPE,			COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_ACCESS_TYPE,					COLUMN_ALIGN_LEFT,		false,	8},
		{COLUMN_DIRECTION_TYPE,					COLUMN_ALIGN_LEFT,		false,	8},
		{COLUMN_OPERATIONAL_STATUS,				COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_ADMINISTRATIVE_STATUS,			COLUMN_ALIGN_LEFT,		false,	4},
		{COLUMN_MEDIA_CONNECTION_STATE,			COLUMN_ALIGN_LEFT,		true,	8},
		{COLUMN_NETWORK_GUID,					COLUMN_ALIGN_LEFT,		false,	10},
		{COLUMN_CONNECTION_TYPE,				COLUMN_ALIGN_LEFT,		false,	6},
		{COLUMN_TRANSMIT_LINK_SPEED,			COLUMN_ALIGN_RIGHT,		false,	8},
		{COLUMN_RECEIVE_LINK_SPEED,				COLUMN_ALIGN_RIGHT,		false,	8},
		{COLUMN_IN_OCTETS,						COLUMN_ALIGN_RIGHT,		true,	7},
		{COLUMN_IN_BANDWIDTH,					COLUMN_ALIGN_RIGHT,		true,	5},
		{COLUMN_MAX_IN_BANDWIDTH,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_UNICAST_PACKETS,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_NON_UNICAST_PACKETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_DISCARDED_PACKETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_ERROR_PACKETS,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_UNKNOWN_PROTOCOL_PACKETS,	COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_UNICAST_OCTETS,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_MULTICAST_OCTETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_IN_BROADCAST_OCTETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_OCTETS,						COLUMN_ALIGN_RIGHT,		true,	7},
		{COLUMN_OUT_BANDWIDTH,					COLUMN_ALIGN_RIGHT,		true,	5},
		{COLUMN_MAX_OUT_BANDWIDTH,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_UNICAST_PACKETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_NON_UNICAST_PACKETS,		COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_DISCARDED_PACKETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_ERROR_PACKETS,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_UNICAST_OCTETS,				COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_MULTICAST_OCTETS,			COLUMN_ALIGN_RIGHT,		false,	5},
		{COLUMN_OUT_BROADCAST_OCTESTS,			COLUMN_ALIGN_RIGHT,		false,	5},
	};

	cvStaticAssert(cvLengthOf(ColumnList) == NUM_COLUMN_TYPES);

	LOGFONT lf;
	GetDefaultFont(&lf);
	const int FontHeight = max(abs(lf.lfHeight), 12);
	const int ItemMargin = m_ItemMargin.left + m_ItemMargin.right;

	m_ColumnList.reserve(cvLengthOf(ColumnList));
	for (int i = 0; i < cvLengthOf(ColumnList); i++) {
		ColumnInfo Column;

		Column.ID = ColumnList[i].ID;
		m_Core.LoadText(IDS_INTERFACELIST_COLUMN_FIRST + Column.ID,
						Column.szText, cvLengthOf(Column.szText));
		Column.Align = ColumnList[i].Align;
		Column.Visible = ColumnList[i].Visible;
		Column.Width = ColumnList[i].Width * FontHeight + ItemMargin;
		m_ColumnList.push_back(Column);
	}

	m_SortOrder.reserve(NUM_COLUMN_TYPES);
	m_SortOrder.push_back(COLUMN_INTERFACE_INDEX);
	for (int i = 0; i < NUM_COLUMN_TYPES; i++) {
		if (i != COLUMN_INTERFACE_INDEX)
			m_SortOrder.push_back(i);
	}
}

InterfaceListView::~InterfaceListView()
{
	Destroy();
}

void InterfaceListView::OnListUpdated()
{
	const ULONGLONG CurTickCount = m_Core.GetUpdatedTickCount();
	const ULONGLONG TimeSpan = CurTickCount - m_UpdatedTickCount;
	const int NumInterfaces = m_Core.NumNetworkInterfaces();
	std::vector<ItemInfo> NewItemList;

	NewItemList.resize(NumInterfaces);
	for (int i = 0; i < NumInterfaces; i++) {
		ItemInfo &NewItem = NewItemList[i];

		NewItem.Selected = false;
		NewItem.IfRow = *m_Core.GetNetworkInterfaceInfo(i);
		NewItem.InBytesPerSecond = 0;
		NewItem.OutBytesPerSecond = 0;
		NewItem.MaxInBytesPerSecond = 0;
		NewItem.MaxOutBytesPerSecond = 0;
		for (size_t j = 0; j < m_ItemList.size(); j++) {
			const ItemInfo &OldItem = m_ItemList[j];

			if (OldItem.IfRow.InterfaceGuid == NewItem.IfRow.InterfaceGuid) {
				if (OldItem.Selected)
					NewItem.Selected = true;
				NewItem.MaxInBytesPerSecond = OldItem.MaxInBytesPerSecond;
				NewItem.MaxOutBytesPerSecond = OldItem.MaxOutBytesPerSecond;
				if (OldItem.IfRow.InOctets < NewItem.IfRow.InOctets) {
					NewItem.InBytesPerSecond =
						(UINT)((NewItem.IfRow.InOctets - OldItem.IfRow.InOctets) * 1000 / TimeSpan);
					if (NewItem.MaxInBytesPerSecond < NewItem.InBytesPerSecond)
						NewItem.MaxInBytesPerSecond = NewItem.InBytesPerSecond;
				}
				if (OldItem.IfRow.OutOctets < NewItem.IfRow.OutOctets) {
					NewItem.OutBytesPerSecond =
						(UINT)((NewItem.IfRow.OutOctets - OldItem.IfRow.OutOctets) * 1000 / TimeSpan);
					if (NewItem.MaxOutBytesPerSecond < NewItem.OutBytesPerSecond)
						NewItem.MaxOutBytesPerSecond = NewItem.OutBytesPerSecond;
				}
				break;
			}
		}
	}

	m_ItemList.swap(NewItemList);
	m_UpdatedTickCount = CurTickCount;

	SortItems();

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE;
	::GetScrollInfo(m_Handle, SB_VERT, &si);
	if ((int)m_ItemList.size() > si.nMax + 1)
		SetScrollBar();
	else if ((int)m_ItemList.size() <= si.nMax - si.nPage)
		AdjustScrollPos(false);
	Redraw();
}

int InterfaceListView::NumItems() const
{
	return (int)m_ItemList.size();
}

static void FormatGUID(const GUID &guid, LPTSTR pText, int MaxTextLength)
{
	FormatString(pText, MaxTextLength,
				 TEXT("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"),
				 guid.Data1,
				 guid.Data2,
				 guid.Data3,
				 guid.Data4[0],
				 guid.Data4[1],
				 guid.Data4[2],
				 guid.Data4[3],
				 guid.Data4[4],
				 guid.Data4[5],
				 guid.Data4[6],
				 guid.Data4[7]);
}

static void FormatPhysicalAddress(const BYTE *pAddress, int AddressLength,
								  LPTSTR pText, int MaxTextLength)
{
	if (AddressLength <= 0) {
		pText[0] = _T('\0');
		return;
	}

	static const LPCTSTR Hex = TEXT("0123456789ABCDEF");
	int i;

	for (i = 0; i < AddressLength && (i + 1) * 3 <= MaxTextLength; i++) {
		pText[i * 3 + 0] = Hex[pAddress[i] >> 4];
		pText[i * 3 + 1] = Hex[pAddress[i] & 0x0F];
		if (i + 1 < AddressLength)
			pText[i * 3 + 2] = _T('-');
	}
	pText[i * 3 - 1] = _T('\0');
}

template<typename L, typename T> void FormatType(
	const L *pList, size_t ListLength, T Type, LPTSTR pText, int MaxTextLength)
{
	size_t i;

	for (i = 0; i < ListLength; i++) {
		if (pList[i].Type == Type)
			break;
	}
	if (i < ListLength)
		::lstrcpyn(pText, pList[i].pText, MaxTextLength);
	else
		UIntToStr((unsigned int)Type, pText, MaxTextLength);
}

bool InterfaceListView::GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = _T('\0');

	if (Row < 0 || (size_t)Row >= m_ItemList.size()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ItemInfo &Item = m_ItemList[Row];

	switch (Column) {
	case COLUMN_INTERFACE_LUID:
		FormatString(pText, MaxTextLength, TEXT("%03x-%03x-%02x"),
					 (int)Item.IfRow.InterfaceLuid.Info.Reserved,
					 (int)Item.IfRow.InterfaceLuid.Info.NetLuidIndex,
					 (int)Item.IfRow.InterfaceLuid.Info.IfType);
		break;

	case COLUMN_INTERFACE_INDEX:
		UIntToStr(Item.IfRow.InterfaceIndex, pText, MaxTextLength);
		break;

	case COLUMN_INTERFACE_GUID:
		//::StringFromGUID(Item.IfRow.InterfaceGuid,pText,MaxTextLength);
		FormatGUID(Item.IfRow.InterfaceGuid, pText, MaxTextLength);
		break;

	case COLUMN_ALIAS:
		::lstrcpyn(pText, Item.IfRow.Alias, MaxTextLength);
		break;

	case COLUMN_DESCRIPTION:
		::lstrcpyn(pText, Item.IfRow.Description, MaxTextLength);
		break;

	case COLUMN_PHYSICAL_ADDRESS:
		FormatPhysicalAddress(Item.IfRow.PhysicalAddress, Item.IfRow.PhysicalAddressLength,
							  pText, MaxTextLength);
		break;

	case COLUMN_PERMANENT_PHYSICAL_ADDRESS:
		FormatPhysicalAddress(Item.IfRow.PermanentPhysicalAddress, Item.IfRow.PhysicalAddressLength,
							  pText, MaxTextLength);
		break;

	case COLUMN_MTU:
		UIntToStr(Item.IfRow.Mtu, pText, MaxTextLength);
		break;

	case COLUMN_TYPE:
		{
			static const struct {
				IFTYPE Type;
				LPCTSTR pText;
			} TypeList[] = {
				{IF_TYPE_OTHER,					TEXT("Other")},
				{IF_TYPE_ETHERNET_CSMACD,		TEXT("Ethernet")},
				{IF_TYPE_ISO88025_TOKENRING,	TEXT("Token ring")},
				{IF_TYPE_PPP,					TEXT("PPP")},
				{IF_TYPE_SOFTWARE_LOOPBACK,		TEXT("Software loopback")},
				{IF_TYPE_ATM,					TEXT("ATM")},
				{IF_TYPE_IEEE80211,				TEXT("IEEE 802.11")},
				{IF_TYPE_TUNNEL,				TEXT("Tunnel")},
				{IF_TYPE_IEEE1394,				TEXT("IEEE 1394")},
			};

			FormatType(TypeList, cvLengthOf(TypeList),
					   Item.IfRow.Type, pText, MaxTextLength);
		}
		break;

	case COLUMN_TUNNEL_TYPE:
		{
			static const struct {
				TUNNEL_TYPE Type;
				LPCTSTR pText;
			} TunnelTypeList[] = {
				{TUNNEL_TYPE_NONE,		TEXT("None")},
				{TUNNEL_TYPE_DIRECT,	TEXT("Direct")},
				{TUNNEL_TYPE_6TO4,		TEXT("6to4")},
				{TUNNEL_TYPE_ISATAP,	TEXT("ISATAP")},
				{TUNNEL_TYPE_TEREDO,	TEXT("Teredo")},
			};

			FormatType(TunnelTypeList, cvLengthOf(TunnelTypeList),
					   Item.IfRow.TunnelType, pText, MaxTextLength);
		}
		break;

	case COLUMN_MEDIA_TYPE:
		{
			static const struct {
				NDIS_MEDIUM Type;
				LPCTSTR pText;
			} MediaTypeList[] = {
				{NdisMedium802_3,			TEXT("Ethernet")},
				{NdisMedium802_5,			TEXT("Token ring")},
				{NdisMediumFddi,			TEXT("FDDI")},
				{NdisMediumWan,				TEXT("WAN")},
				{NdisMediumLocalTalk,		TEXT("LocalTalk")},
				{NdisMediumDix,				TEXT("DIX")},
				{NdisMediumArcnetRaw,		TEXT("ARCNET")},
				{NdisMediumArcnet878_2,		TEXT("ARCNET(878.2)")},
				{NdisMediumAtm,				TEXT("ATM")},
				{NdisMediumWirelessWan,		TEXT("WAN")},
				{NdisMediumIrda,			TEXT("IrDA")},
				{NdisMediumBpc,				TEXT("Broadcast PC")},
				{NdisMediumCoWan,			TEXT("Co WAN")},
				{NdisMedium1394,			TEXT("IEEE 1394")},
				{NdisMediumInfiniBand,		TEXT("InfiniBand")},
				{NdisMediumTunnel,			TEXT("Tunnel")},
				{NdisMediumNative802_11,	TEXT("IEE 802.11")},
				{NdisMediumLoopback,		TEXT("Loopback")},
			};

			FormatType(MediaTypeList, cvLengthOf(MediaTypeList),
					   Item.IfRow.MediaType, pText, MaxTextLength);
		}
		break;

	case COLUMN_PHYSICAL_MEDIUM_TYPE:
		{
			static const struct {
				NDIS_PHYSICAL_MEDIUM Type;
				LPCTSTR pText;
			} PhysicalMediumTypeList[] = {
				{NdisPhysicalMediumUnspecified,		TEXT("Unspecified")},
				{NdisPhysicalMediumWirelessLan,		TEXT("Wireless LAN")},
				{NdisPhysicalMediumCableModem,		TEXT("Cable modem")},
				{NdisPhysicalMediumPhoneLine,		TEXT("Phone line")},
				{NdisPhysicalMediumPowerLine,		TEXT("Power line")},
				{NdisPhysicalMediumDSL,				TEXT("DSL")},
				{NdisPhysicalMediumFibreChannel,	TEXT("Fibre Channel")},
				{NdisPhysicalMedium1394,			TEXT("IEEE 1394")},
				{NdisPhysicalMediumWirelessWan,		TEXT("Wireless WAN")},
				{NdisPhysicalMediumNative802_11,	TEXT("Native 802.11")},
				{NdisPhysicalMediumBluetooth,		TEXT("Bluetooth")},
				{NdisPhysicalMediumInfiniband,		TEXT("InfiniBand")},
				{NdisPhysicalMediumWiMax,			TEXT("WiMax")},
				{NdisPhysicalMediumUWB,				TEXT("UWB")},
				{NdisPhysicalMedium802_3,			TEXT("Ethernet")},
				{NdisPhysicalMedium802_5,			TEXT("Token ring")},
				{NdisPhysicalMediumIrda,			TEXT("IrDA")},
				{NdisPhysicalMediumWiredWAN,		TEXT("Wired WAN")},
				{NdisPhysicalMediumWiredCoWan,		TEXT("Co WAN")},
				{NdisPhysicalMediumOther,			TEXT("Other")},
			};

			FormatType(PhysicalMediumTypeList, cvLengthOf(PhysicalMediumTypeList),
					   Item.IfRow.PhysicalMediumType, pText, MaxTextLength);
		}
		break;

	case COLUMN_ACCESS_TYPE:
		{
			static const struct {
				NET_IF_ACCESS_TYPE Type;
				LPCTSTR pText;
			} AccessTypeList[] = {
				{NET_IF_ACCESS_LOOPBACK,				TEXT("Loopback")},
				{NET_IF_ACCESS_BROADCAST,				TEXT("Broadcast")},
				{NET_IF_ACCESS_POINT_TO_POINT,			TEXT("Point-to-point")},
				{NET_IF_ACCESS_POINT_TO_MULTI_POINT,	TEXT("Point-to-multipoint")},
			};

			FormatType(AccessTypeList, cvLengthOf(AccessTypeList),
					   Item.IfRow.AccessType, pText, MaxTextLength);
		}
		break;

	case COLUMN_DIRECTION_TYPE:
		{
			static const struct {
				NET_IF_DIRECTION_TYPE Type;
				LPCTSTR pText;
			} DirectionTypeList[] = {
				{NET_IF_DIRECTION_SENDRECEIVE,	TEXT("Send/Receive")},
				{NET_IF_DIRECTION_SENDONLY,		TEXT("Send")},
				{NET_IF_DIRECTION_RECEIVEONLY,	TEXT("Receive")},
			};

			FormatType(DirectionTypeList, cvLengthOf(DirectionTypeList),
					   Item.IfRow.DirectionType, pText, MaxTextLength);
		}
		break;

	case COLUMN_OPERATIONAL_STATUS:
		{
			static const struct {
				IF_OPER_STATUS Type;
				LPCTSTR pText;
			} OperationalStatusList[] = {
				{IfOperStatusUp,				TEXT("Up")},
				{IfOperStatusDown,				TEXT("Down")},
				{IfOperStatusTesting,			TEXT("Testing")},
				{IfOperStatusUnknown,			TEXT("Unknown")},
				{IfOperStatusDormant,			TEXT("Dormant")},
				{IfOperStatusNotPresent,		TEXT("Not present")},
				{IfOperStatusLowerLayerDown,	TEXT("Lower layer down")},
			};

			FormatType(OperationalStatusList, cvLengthOf(OperationalStatusList),
					   Item.IfRow.OperStatus, pText, MaxTextLength);
		}
		break;

	case COLUMN_ADMINISTRATIVE_STATUS:
		{
			static const struct {
				NET_IF_ADMIN_STATUS Type;
				LPCTSTR pText;
			} AdminStatusList[] = {
				{NET_IF_ADMIN_STATUS_UP,		TEXT("Up")},
				{NET_IF_ADMIN_STATUS_DOWN,		TEXT("Down")},
				{NET_IF_ADMIN_STATUS_TESTING,	TEXT("Testing")},
			};

			FormatType(AdminStatusList, cvLengthOf(AdminStatusList),
					   Item.IfRow.AdminStatus, pText, MaxTextLength);
		}
		break;

	case COLUMN_MEDIA_CONNECTION_STATE:
		{
			static const struct {
				NET_IF_MEDIA_CONNECT_STATE Type;
				LPCTSTR pText;
			} MediaConnectStateList[] = {
				{MediaConnectStateUnknown,		TEXT("Unknown")},
				{MediaConnectStateConnected,	TEXT("Connected")},
				{MediaConnectStateDisconnected,	TEXT("Disconnected")},
			};

			FormatType(MediaConnectStateList, cvLengthOf(MediaConnectStateList),
					   Item.IfRow.MediaConnectState, pText, MaxTextLength);
		}
		break;

	case COLUMN_NETWORK_GUID:
		FormatGUID(Item.IfRow.NetworkGuid, pText, MaxTextLength);
		break;

	case COLUMN_CONNECTION_TYPE:
		{
			static const struct {
				NET_IF_CONNECTION_TYPE Type;
				LPCTSTR pText;
			} ConnectionTypeList[] = {
				{NET_IF_CONNECTION_DEDICATED,	TEXT("Dedicated")},
				{NET_IF_CONNECTION_PASSIVE,		TEXT("Passive")},
				{NET_IF_CONNECTION_DEMAND,		TEXT("Demand")},
			};

			FormatType(ConnectionTypeList, cvLengthOf(ConnectionTypeList),
					   Item.IfRow.ConnectionType, pText, MaxTextLength);
		}
		break;

	case COLUMN_TRANSMIT_LINK_SPEED:
		FormatUInt64(Item.IfRow.TransmitLinkSpeed, pText, MaxTextLength);
		break;

	case COLUMN_RECEIVE_LINK_SPEED:
		FormatUInt64(Item.IfRow.ReceiveLinkSpeed, pText, MaxTextLength);
		break;

	case COLUMN_IN_OCTETS:
		FormatUInt64(Item.IfRow.InOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_BANDWIDTH:
		FormatUInt(Item.InBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_MAX_IN_BANDWIDTH:
		FormatUInt(Item.MaxInBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_IN_UNICAST_PACKETS:
		FormatUInt64(Item.IfRow.InUcastPkts, pText, MaxTextLength);
		break;

	case COLUMN_IN_NON_UNICAST_PACKETS:
		FormatUInt64(Item.IfRow.InNUcastPkts, pText, MaxTextLength);
		break;

	case COLUMN_IN_DISCARDED_PACKETS:
		FormatUInt64(Item.IfRow.InDiscards, pText, MaxTextLength);
		break;

	case COLUMN_IN_ERROR_PACKETS:
		FormatUInt64(Item.IfRow.InErrors, pText, MaxTextLength);
		break;

	case COLUMN_IN_UNKNOWN_PROTOCOL_PACKETS:
		FormatUInt64(Item.IfRow.InUnknownProtos, pText, MaxTextLength);
		break;

	case COLUMN_IN_UNICAST_OCTETS:
		FormatUInt64(Item.IfRow.InUcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_MULTICAST_OCTETS:
		FormatUInt64(Item.IfRow.InMulticastOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_BROADCAST_OCTETS:
		FormatUInt64(Item.IfRow.InBroadcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_OCTETS:
		FormatUInt64(Item.IfRow.OutOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BANDWIDTH:
		FormatUInt(Item.OutBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_MAX_OUT_BANDWIDTH:
		FormatUInt(Item.MaxOutBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_OUT_UNICAST_PACKETS:
		FormatUInt64(Item.IfRow.OutUcastPkts, pText, MaxTextLength);
		break;

	case COLUMN_OUT_NON_UNICAST_PACKETS:
		FormatUInt64(Item.IfRow.OutNUcastPkts, pText, MaxTextLength);
		break;

	case COLUMN_OUT_DISCARDED_PACKETS:
		FormatUInt64(Item.IfRow.OutDiscards, pText, MaxTextLength);
		break;

	case COLUMN_OUT_ERROR_PACKETS:
		FormatUInt64(Item.IfRow.OutErrors, pText, MaxTextLength);
		break;

	case COLUMN_OUT_UNICAST_OCTETS:
		FormatUInt64(Item.IfRow.OutUcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_MULTICAST_OCTETS:
		FormatUInt64(Item.IfRow.OutMulticastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BROADCAST_OCTESTS:
		FormatUInt64(Item.IfRow.OutBroadcastOctets, pText, MaxTextLength);
		break;

	default:
		cvDebugBreak();
		return false;
	}

	return true;
}

bool InterfaceListView::GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	pText[0] = _T('\0');

	if (Row < 0 || (size_t)Row >= m_ItemList.size()
			|| Column < 0 || Column >= NUM_COLUMN_TYPES)
		return false;

	const ItemInfo &Item = m_ItemList[Row];

	switch (Column) {
	case COLUMN_IN_OCTETS:
		FormatBytesLong(Item.IfRow.InOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_BANDWIDTH:
		FormatBandwidthLong(Item.InBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_MAX_IN_BANDWIDTH:
		FormatBandwidthLong(Item.MaxInBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_IN_UNICAST_OCTETS:
		FormatBytesLong(Item.IfRow.InUcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_MULTICAST_OCTETS:
		FormatBytesLong(Item.IfRow.InMulticastOctets, pText, MaxTextLength);
		break;

	case COLUMN_IN_BROADCAST_OCTETS:
		FormatBytesLong(Item.IfRow.InBroadcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_OCTETS:
		FormatBytesLong(Item.IfRow.OutOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BANDWIDTH:
		FormatBandwidthLong(Item.OutBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_MAX_OUT_BANDWIDTH:
		FormatBandwidthLong(Item.MaxOutBytesPerSecond, pText, MaxTextLength);
		break;

	case COLUMN_OUT_UNICAST_OCTETS:
		FormatBytesLong(Item.IfRow.OutUcastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_MULTICAST_OCTETS:
		FormatBytesLong(Item.IfRow.OutMulticastOctets, pText, MaxTextLength);
		break;

	case COLUMN_OUT_BROADCAST_OCTESTS:
		FormatBytesLong(Item.IfRow.OutBroadcastOctets, pText, MaxTextLength);
		break;

	default:
		return GetItemText(Row, Column, pText, MaxTextLength);
	}

	return true;
}

LPCTSTR InterfaceListView::GetColumnIDName(int ID) const
{
	return GetColumnName(ID);
}

const InterfaceListView::ItemInfo *InterfaceListView::GetItemInfo(const GUID &Guid) const
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i].IfRow.InterfaceGuid == Guid)
			return &m_ItemList[i];
	}
	return nullptr;
}

bool InterfaceListView::OnSelChange(int OldSel, int NewSel)
{
	if (OldSel >= 0)
		m_ItemList[OldSel].Selected = false;
	if (NewSel >= 0)
		m_ItemList[NewSel].Selected = true;
	return true;
}

template<typename T> int CompareValue(T Value1, T Value2)
{
	return Value1 < Value2 ? -1 : Value1 > Value2 ? 1 : 0;
}

static int CompareGUID(const GUID &Guid1, const GUID &Guid2)
{
	int Cmp;

	Cmp = CompareValue(Guid1.Data1, Guid2.Data1);
	if (Cmp == 0) {
		Cmp = CompareValue(Guid1.Data2, Guid2.Data2);
		if (Cmp == 0) {
			Cmp = CompareValue(Guid1.Data3, Guid2.Data3);
			if (Cmp == 0)
				Cmp = ::memcmp(Guid1.Data4, Guid2.Data4, 8);
		}
	}
	return Cmp;
}

class IfListItemCompare
{
	const std::vector<int> &m_SortOrder;
	bool m_Ascending;

public:
	IfListItemCompare(const std::vector<int> &SortOrder, bool Ascending)
		: m_SortOrder(SortOrder)
		, m_Ascending(Ascending)
	{
	}

	bool operator()(const InterfaceListView::ItemInfo &Item1,
					const InterfaceListView::ItemInfo &Item2) const
	{
		for (size_t i = 0; i < m_SortOrder.size(); i++) {
			int Cmp = 0;

			switch (m_SortOrder[i]) {
			case InterfaceListView::COLUMN_INTERFACE_LUID:
				Cmp = CompareValue(Item1.IfRow.InterfaceLuid.Value, Item2.IfRow.InterfaceLuid.Value);
				break;

			case InterfaceListView::COLUMN_INTERFACE_INDEX:
				Cmp = CompareValue(Item1.IfRow.InterfaceIndex, Item2.IfRow.InterfaceIndex);
				break;

			case InterfaceListView::COLUMN_INTERFACE_GUID:
				Cmp = CompareGUID(Item1.IfRow.InterfaceGuid, Item2.IfRow.InterfaceGuid);
				break;

			case InterfaceListView::COLUMN_ALIAS:
				Cmp = ::lstrcmp(Item1.IfRow.Alias, Item2.IfRow.Alias);
				break;

			case InterfaceListView::COLUMN_DESCRIPTION:
				Cmp = ::lstrcmp(Item1.IfRow.Description, Item2.IfRow.Description);
				break;

			case InterfaceListView::COLUMN_PHYSICAL_ADDRESS:
				if (Item1.IfRow.PhysicalAddressLength > 0) {
					if (Item2.IfRow.PhysicalAddressLength > 0) {
						Cmp = CompareValue(Item1.IfRow.PhysicalAddressLength,
										   Item2.IfRow.PhysicalAddressLength);
						if (Cmp == 0)
							Cmp = ::memcmp(Item1.IfRow.PhysicalAddress,
										  Item2.IfRow.PhysicalAddress,
										  Item1.IfRow.PhysicalAddressLength);
					} else
						Cmp = -1;
				} else if (Item2.IfRow.PhysicalAddressLength > 0)
					Cmp = 1;
				break;

			case InterfaceListView::COLUMN_PERMANENT_PHYSICAL_ADDRESS:
				if (Item1.IfRow.PhysicalAddressLength > 0) {
					if (Item2.IfRow.PhysicalAddressLength > 0) {
						Cmp = CompareValue(Item1.IfRow.PhysicalAddressLength,
										   Item2.IfRow.PhysicalAddressLength);
						if (Cmp == 0)
							Cmp = ::memcmp(Item1.IfRow.PermanentPhysicalAddress,
										  Item2.IfRow.PermanentPhysicalAddress,
										  Item1.IfRow.PhysicalAddressLength);
					} else
						Cmp = -1;
				} else if (Item2.IfRow.PhysicalAddressLength > 0)
					Cmp = 1;
				break;

			case InterfaceListView::COLUMN_MTU:
				Cmp = CompareValue(Item1.IfRow.Mtu, Item2.IfRow.Mtu);
				break;

			case InterfaceListView::COLUMN_TYPE:
				Cmp = CompareValue(Item1.IfRow.Type, Item2.IfRow.Type);
				break;

			case InterfaceListView::COLUMN_TUNNEL_TYPE:
				Cmp = CompareValue(Item1.IfRow.TunnelType, Item2.IfRow.TunnelType);
				break;

			case InterfaceListView::COLUMN_MEDIA_TYPE:
				Cmp = CompareValue(Item1.IfRow.MediaType, Item2.IfRow.MediaType);
				break;

			case InterfaceListView::COLUMN_PHYSICAL_MEDIUM_TYPE:
				Cmp = CompareValue(Item1.IfRow.PhysicalMediumType, Item2.IfRow.PhysicalMediumType);
				break;

			case InterfaceListView::COLUMN_ACCESS_TYPE:
				Cmp = CompareValue(Item1.IfRow.AccessType, Item2.IfRow.AccessType);
				break;

			case InterfaceListView::COLUMN_DIRECTION_TYPE:
				Cmp = CompareValue(Item1.IfRow.DirectionType, Item2.IfRow.DirectionType);
				break;

			case InterfaceListView::COLUMN_OPERATIONAL_STATUS:
				Cmp = CompareValue(Item1.IfRow.OperStatus, Item2.IfRow.OperStatus);
				break;

			case InterfaceListView::COLUMN_ADMINISTRATIVE_STATUS:
				Cmp = CompareValue(Item1.IfRow.AdminStatus, Item2.IfRow.AdminStatus);
				break;

			case InterfaceListView::COLUMN_MEDIA_CONNECTION_STATE:
				Cmp = CompareValue(Item1.IfRow.MediaConnectState, Item2.IfRow.MediaConnectState);
				break;

			case InterfaceListView::COLUMN_NETWORK_GUID:
				Cmp = CompareGUID(Item1.IfRow.NetworkGuid, Item2.IfRow.NetworkGuid);
				break;

			case InterfaceListView::COLUMN_CONNECTION_TYPE:
				Cmp = CompareValue(Item1.IfRow.ConnectionType, Item2.IfRow.ConnectionType);
				break;

			case InterfaceListView::COLUMN_TRANSMIT_LINK_SPEED:
				Cmp = CompareValue(Item1.IfRow.TransmitLinkSpeed, Item2.IfRow.TransmitLinkSpeed);
				break;

			case InterfaceListView::COLUMN_RECEIVE_LINK_SPEED:
				Cmp = CompareValue(Item1.IfRow.ReceiveLinkSpeed, Item2.IfRow.ReceiveLinkSpeed);
				break;

			case InterfaceListView::COLUMN_IN_OCTETS:
				Cmp = CompareValue(Item1.IfRow.InOctets, Item2.IfRow.InOctets);
				break;

			case InterfaceListView::COLUMN_IN_BANDWIDTH:
				Cmp = CompareValue(Item1.InBytesPerSecond, Item2.InBytesPerSecond);
				break;

			case InterfaceListView::COLUMN_MAX_IN_BANDWIDTH:
				Cmp = CompareValue(Item1.MaxInBytesPerSecond, Item2.MaxInBytesPerSecond);
				break;

			case InterfaceListView::COLUMN_IN_UNICAST_PACKETS:
				Cmp = CompareValue(Item1.IfRow.InUcastPkts, Item2.IfRow.InUcastPkts);
				break;

			case InterfaceListView::COLUMN_IN_NON_UNICAST_PACKETS:
				Cmp = CompareValue(Item1.IfRow.InNUcastPkts, Item2.IfRow.InNUcastPkts);
				break;

			case InterfaceListView::COLUMN_IN_DISCARDED_PACKETS:
				Cmp = CompareValue(Item1.IfRow.InDiscards, Item2.IfRow.InDiscards);
				break;

			case InterfaceListView::COLUMN_IN_ERROR_PACKETS:
				Cmp = CompareValue(Item1.IfRow.InErrors, Item2.IfRow.InErrors);
				break;

			case InterfaceListView::COLUMN_IN_UNKNOWN_PROTOCOL_PACKETS:
				Cmp = CompareValue(Item1.IfRow.InUnknownProtos, Item2.IfRow.InUnknownProtos);
				break;

			case InterfaceListView::COLUMN_IN_UNICAST_OCTETS:
				Cmp = CompareValue(Item1.IfRow.InUcastOctets, Item2.IfRow.InUcastOctets);
				break;

			case InterfaceListView::COLUMN_IN_MULTICAST_OCTETS:
				Cmp = CompareValue(Item1.IfRow.InMulticastOctets, Item2.IfRow.InMulticastOctets);
				break;

			case InterfaceListView::COLUMN_IN_BROADCAST_OCTETS:
				Cmp = CompareValue(Item1.IfRow.InBroadcastOctets, Item2.IfRow.InBroadcastOctets);
				break;

			case InterfaceListView::COLUMN_OUT_OCTETS:
				Cmp = CompareValue(Item1.IfRow.OutOctets, Item2.IfRow.OutOctets);
				break;

			case InterfaceListView::COLUMN_OUT_BANDWIDTH:
				Cmp = CompareValue(Item1.OutBytesPerSecond, Item2.OutBytesPerSecond);
				break;

			case InterfaceListView::COLUMN_MAX_OUT_BANDWIDTH:
				Cmp = CompareValue(Item1.MaxOutBytesPerSecond, Item2.MaxOutBytesPerSecond);
				break;

			case InterfaceListView::COLUMN_OUT_UNICAST_PACKETS:
				Cmp = CompareValue(Item1.IfRow.OutUcastPkts, Item2.IfRow.OutUcastPkts);
				break;

			case InterfaceListView::COLUMN_OUT_NON_UNICAST_PACKETS:
				Cmp = CompareValue(Item1.IfRow.OutNUcastPkts, Item2.IfRow.OutNUcastPkts);
				break;

			case InterfaceListView::COLUMN_OUT_DISCARDED_PACKETS:
				Cmp = CompareValue(Item1.IfRow.OutDiscards, Item2.IfRow.OutDiscards);
				break;

			case InterfaceListView::COLUMN_OUT_ERROR_PACKETS:
				Cmp = CompareValue(Item1.IfRow.OutErrors, Item2.IfRow.OutErrors);
				break;

			case InterfaceListView::COLUMN_OUT_UNICAST_OCTETS:
				Cmp = CompareValue(Item1.IfRow.OutUcastOctets, Item2.IfRow.OutUcastOctets);
				break;

			case InterfaceListView::COLUMN_OUT_MULTICAST_OCTETS:
				Cmp = CompareValue(Item1.IfRow.OutMulticastOctets, Item2.IfRow.OutMulticastOctets);
				break;

			case InterfaceListView::COLUMN_OUT_BROADCAST_OCTESTS:
				Cmp = CompareValue(Item1.IfRow.OutBroadcastOctets, Item2.IfRow.OutBroadcastOctets);
				break;
			}

			if (Cmp != 0)
				return m_Ascending ? Cmp<0: Cmp>0;
		}

		return false;
	}
};

bool InterfaceListView::SortItems()
{
	std::sort(m_ItemList.begin(), m_ItemList.end(),
			  IfListItemCompare(m_SortOrder, m_SortAscending));

	m_SelectedItem = -1;
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i].Selected) {
			m_SelectedItem = (int)i;
			break;
		}
	}

	return true;
}

}	// namespace CV
