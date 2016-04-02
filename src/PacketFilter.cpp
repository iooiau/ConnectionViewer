/******************************************************************************
*                                                                             *
*    PacketFilter.cpp                       Copyright(c) 2010-2016 itow,y.    *
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
#include <fwpmu.h>
#include "PacketFilter.h"

#pragma comment(lib, "fwpuclnt.lib")
#pragma comment(lib, "rpcrt4.lib")


namespace CV
{

PacketFilter::PacketFilter()
	: m_hEngine(nullptr)
	, m_SubLayerGUID(GUID_NULL)
{
}

PacketFilter::~PacketFilter()
{
	Close();
}

bool PacketFilter::Open()
{
	Close();

	cvDebugTrace(TEXT("Open filtering engine\n"));

	DWORD Result = ::FwpmEngineOpen0(
		nullptr,
		RPC_C_AUTHN_WINNT,
		nullptr,
		nullptr,
		&m_hEngine);
	if (Result != ERROR_SUCCESS) {
		cvDebugTrace(TEXT("Failed to open engine (%x)\n"), Result);
		m_hEngine = nullptr;
		::SetLastError(Result);
		return false;
	}

	::SetLastError(ERROR_SUCCESS);

	return true;
}

void PacketFilter::Close()
{
	if (m_hEngine != nullptr) {
		EndFiltering();

		cvDebugTrace(TEXT("Close filtering engine\n"));

		::FwpmEngineClose0(m_hEngine);
		m_hEngine = nullptr;
	}
}

bool PacketFilter::IsOpen() const
{
	return m_hEngine != nullptr;
}

bool PacketFilter::BeginFiltering()
{
	if (m_hEngine == nullptr) {
		::SetLastError(ERROR_INVALID_FUNCTION);
		return false;
	}

	if (m_SubLayerGUID == GUID_NULL) {
		cvDebugTrace(TEXT("Begin packet filtering\n"));

		FWPM_SUBLAYER0 SubLayer;
		::ZeroMemory(&SubLayer, sizeof(SubLayer));
		RPC_STATUS Status = ::UuidCreate(&SubLayer.subLayerKey);
		if (Status != RPC_S_OK && Status != RPC_S_UUID_LOCAL_ONLY) {
			cvDebugTrace(TEXT("Failed to create UUID (%x)\n"), Status);
			::SetLastError(Status);
			return false;
		}
		SubLayer.displayData.name = L"Connection Viewer";
		SubLayer.displayData.description = L"Connection Viewer";
		SubLayer.flags = 0;
		SubLayer.weight = 0x100;
		DWORD Result = ::FwpmSubLayerAdd0(m_hEngine, &SubLayer, nullptr);
		if (Result != ERROR_SUCCESS) {
			cvDebugTrace(TEXT("Failed to add sublayer (%x)\n"), Result);
			::SetLastError(Result);
			return false;
		}
		m_SubLayerGUID = SubLayer.subLayerKey;

		for (FilterList::iterator i = m_FilterList.begin(); i != m_FilterList.end(); i++) {
			AddBlockFilter(i->first, BlockLayer::CONNECT, &i->second.ConnectFilterID);
			AddBlockFilter(i->first, BlockLayer::INBOUND, &i->second.InFilterID);
			AddBlockFilter(i->first, BlockLayer::OUTBOUND, &i->second.OutFilterID);
		}
	}

	::SetLastError(ERROR_SUCCESS);

	return true;
}

bool PacketFilter::EndFiltering()
{
	if (m_SubLayerGUID != GUID_NULL) {
		for (FilterList::iterator i = m_FilterList.begin(); i != m_FilterList.end(); i++) {
			RemoveBlockFilter(&i->second.ConnectFilterID);
			RemoveBlockFilter(&i->second.InFilterID);
			RemoveBlockFilter(&i->second.OutFilterID);
		}

		cvDebugTrace(TEXT("End packet filtering\n"));

		::FwpmSubLayerDeleteByKey0(m_hEngine, &m_SubLayerGUID);
		m_SubLayerGUID = GUID_NULL;
	}

	return true;
}

bool PacketFilter::IsFiltering() const
{
	return (m_SubLayerGUID != GUID_NULL) != FALSE;
}

bool PacketFilter::AddFilter(const FilterCondition &Condition)
{
	switch (Condition.Match) {
	case FilterCondition::MATCH_EQUAL:
		break;

	case FilterCondition::MATCH_RANGE:
		if (Condition.AddressRange.Low.Type != Condition.AddressRange.High.Type
				|| Condition.AddressRange.Low > Condition.AddressRange.High) {
			::SetLastError(ERROR_INVALID_PARAMETER);
			return false;
		}
		break;

	default:
		::SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	if (m_FilterList.find(Condition) != m_FilterList.end()) {
		::SetLastError(ERROR_ALREADY_EXISTS);
		return false;
	}

	FilterInfo Info;

	if (m_SubLayerGUID != GUID_NULL) {
		DWORD Result = AddBlockFilter(Condition, BlockLayer::CONNECT, &Info.ConnectFilterID);
		if (Result != ERROR_SUCCESS) {
			cvDebugTrace(TEXT("Failed to add connection block filter (%x)\n"), Result);
			::SetLastError(Result);
			return false;
		}
		Result = AddBlockFilter(Condition, BlockLayer::INBOUND, &Info.InFilterID);
		if (Result != ERROR_SUCCESS) {
			cvDebugTrace(TEXT("Failed to add inbound block filter (%x)\n"), Result);
			RemoveBlockFilter(&Info.ConnectFilterID);
			::SetLastError(Result);
			return false;
		}
		Result = AddBlockFilter(Condition, BlockLayer::OUTBOUND, &Info.OutFilterID);
		if (Result != ERROR_SUCCESS) {
			cvDebugTrace(TEXT("Failed to add outbound block filter (%x)\n"), Result);
			RemoveBlockFilter(&Info.ConnectFilterID);
			RemoveBlockFilter(&Info.InFilterID);
			::SetLastError(Result);
			return false;
		}
	}

	m_FilterList.insert(std::pair<FilterCondition, FilterInfo>(Condition, Info));

	::SetLastError(ERROR_SUCCESS);

	return true;
}

bool PacketFilter::RemoveFilter(const FilterCondition &Condition)
{
	FilterList::iterator i = m_FilterList.find(Condition);
	if (i == m_FilterList.end()) {
		::SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	if (m_SubLayerGUID != GUID_NULL) {
#ifdef _DEBUG
		if (Condition.Match == FilterCondition::MATCH_EQUAL) {
			TCHAR szAddress[64];
			FormatIPAddress(Condition.Address, szAddress, cvLengthOf(szAddress));
			cvDebugTrace(TEXT("Remove block filter %s\n"), szAddress);
		} else if (Condition.Match == FilterCondition::MATCH_RANGE) {
			TCHAR szAddressLow[64], szAddressHigh[64];
			FormatIPAddress(Condition.AddressRange.Low, szAddressLow, cvLengthOf(szAddressLow));
			FormatIPAddress(Condition.AddressRange.High, szAddressHigh, cvLengthOf(szAddressHigh));
			cvDebugTrace(TEXT("Remove block filter %s - %s\n"), szAddressLow, szAddressHigh);
		}
#endif

		DWORD Result = RemoveBlockFilter(&i->second.ConnectFilterID);
		if (Result != ERROR_SUCCESS) {
			::SetLastError(Result);
			return false;
		}
		Result = RemoveBlockFilter(&i->second.InFilterID);
		if (Result != ERROR_SUCCESS) {
			::SetLastError(Result);
			return false;
		}
		Result = RemoveBlockFilter(&i->second.OutFilterID);
		if (Result != ERROR_SUCCESS) {
			::SetLastError(Result);
			return false;
		}
	}

	m_FilterList.erase(i);

	::SetLastError(ERROR_SUCCESS);

	return true;
}

bool PacketFilter::HasFilter(const FilterCondition &Condition) const
{
	FilterList::const_iterator i = m_FilterList.find(Condition);
	if (i == m_FilterList.end())
		return false;
	return i->second.InFilterID != 0 || i->second.OutFilterID != 0;
}

bool PacketFilter::RemoveAllFilters()
{
	if (m_SubLayerGUID != GUID_NULL) {
		for (FilterList::iterator i = m_FilterList.begin(); i != m_FilterList.end(); i++) {
			RemoveBlockFilter(&i->second.ConnectFilterID);
			RemoveBlockFilter(&i->second.InFilterID);
			RemoveBlockFilter(&i->second.OutFilterID);
		}
	}
	m_FilterList.clear();
	return true;
}

DWORD PacketFilter::AddBlockFilter(const FilterCondition &Condition,
								   BlockLayer Layer,
								   UINT64 *pFilterID)
{
	FWPM_FILTER_CONDITION0 FwpmCondition;
	FWP_V4_ADDR_AND_MASK V4AddrMask;
	FWP_V6_ADDR_AND_MASK V6AddrMask;
	FWP_RANGE0 Range;
	FWP_BYTE_ARRAY16 ByteArray16Low, ByteArray16High;
	bool fIPv4;

	FwpmCondition.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
	if (Condition.Match == FilterCondition::MATCH_EQUAL) {
		fIPv4 = Condition.Address.Type == IP_ADDRESS_V4;
		FwpmCondition.matchType = FWP_MATCH_EQUAL;
		if (fIPv4) {
			V4AddrMask.addr = Condition.Address.V4.HostValue();
			V4AddrMask.mask = 0xFFFFFFFF;
			FwpmCondition.conditionValue.type = FWP_V4_ADDR_MASK;
			FwpmCondition.conditionValue.v4AddrMask = &V4AddrMask;
		} else {
			::CopyMemory(V6AddrMask.addr, Condition.Address.V6.Bytes, 16);
			V6AddrMask.prefixLength = 128;
			FwpmCondition.conditionValue.type = FWP_V6_ADDR_MASK;
			FwpmCondition.conditionValue.v6AddrMask = &V6AddrMask;
		}
	} else {
		fIPv4 = Condition.AddressRange.Low.Type == IP_ADDRESS_V4;
		FwpmCondition.matchType = FWP_MATCH_RANGE; //FWP_MATCH_EQUAL;
		FwpmCondition.conditionValue.type = FWP_RANGE_TYPE;
		FwpmCondition.conditionValue.rangeValue = &Range;
		if (fIPv4) {
			Range.valueLow.type = FWP_UINT32;
			Range.valueLow.uint32 = Condition.AddressRange.Low.V4.HostValue();
			Range.valueHigh.type = FWP_UINT32;
			Range.valueHigh.uint32 = Condition.AddressRange.High.V4.HostValue();
		} else {
			Range.valueLow.type = FWP_BYTE_ARRAY16_TYPE;
			Range.valueLow.byteArray16 = &ByteArray16Low;
			::CopyMemory(ByteArray16Low.byteArray16, Condition.AddressRange.Low.V6.Bytes, 16);
			Range.valueHigh.type = FWP_BYTE_ARRAY16_TYPE;
			Range.valueHigh.byteArray16 = &ByteArray16High;
			::CopyMemory(ByteArray16High.byteArray16, Condition.AddressRange.High.V6.Bytes, 16);
		}
	}

	FWPM_FILTER0 Filter;
	::ZeroMemory(&Filter, sizeof(Filter));
	Filter.displayData.name = L"Connection Viewer block filter";
	Filter.flags = FWPM_FILTER_FLAG_NONE;
	Filter.subLayerKey = m_SubLayerGUID;
	Filter.weight.type = FWP_EMPTY;
	Filter.numFilterConditions = 1;
	Filter.filterCondition = &FwpmCondition;
	Filter.action.type = FWP_ACTION_BLOCK;
	switch (Layer) {
	case BlockLayer::INBOUND:
		/*
		if (fIPv4)
			Filter.layerKey=FWPM_LAYER_INBOUND_TRANSPORT_V4;
		else
			Filter.layerKey=FWPM_LAYER_INBOUND_TRANSPORT_V6;
		*/
		if (fIPv4)
			Filter.layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;
		else
			Filter.layerKey = FWPM_LAYER_INBOUND_IPPACKET_V6;
		break;
	case BlockLayer::OUTBOUND:
		/*
		if (fIPv4)
			Filter.layerKey=FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
		else
			Filter.layerKey=FWPM_LAYER_OUTBOUND_TRANSPORT_V6;
		*/
		if (fIPv4)
			Filter.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
		else
			Filter.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V6;
		break;
	case BlockLayer::CONNECT:
		if (fIPv4)
			Filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		else
			Filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		break;
	default:
		return ERROR_INVALID_PARAMETER;
	}

#ifdef _DEBUG
	TCHAR szAddress[128];
	if (Condition.Match == FilterCondition::MATCH_EQUAL) {
		FormatIPAddress(Condition.Address, szAddress, cvLengthOf(szAddress));
	} else {
		FormatIPAddress(Condition.AddressRange.Low, szAddress, cvLengthOf(szAddress));
		::lstrcat(szAddress, TEXT(" - "));
		int Length = ::lstrlen(szAddress);
		FormatIPAddress(Condition.AddressRange.High,
						szAddress + Length, cvLengthOf(szAddress) - Length);
	}
	cvDebugTrace(TEXT("Add block filter %s (%s)\n"),
				 szAddress,
				 Layer == BlockLayer::INBOUND ? TEXT("In") :
				 Layer == BlockLayer::OUTBOUND ? TEXT("Out") : TEXT("Connect"));
#endif

	DWORD Result = ::FwpmFilterAdd0(m_hEngine, &Filter, nullptr, pFilterID);
#ifdef _DEBUG
	if (Result == ERROR_SUCCESS)
		cvDebugTrace(TEXT("FwpmFilterAdd0 (ID %llu)\n"), *pFilterID);
	else
		cvDebugTrace(TEXT("FwpmFilterAdd0() failed (%x)\n"), Result);
#endif
	return Result;
}

DWORD PacketFilter::RemoveBlockFilter(UINT64 *pFilterID)
{
	if (*pFilterID != 0) {
		cvDebugTrace(TEXT("FwpmFilterDeleteById0 (ID %llu)\n"), *pFilterID);
		DWORD Result = ::FwpmFilterDeleteById0(m_hEngine, *pFilterID);
		if (Result != ERROR_SUCCESS) {
			cvDebugTrace(TEXT("FwpmFilterDeleteById0() failed (%x)\n"), Result);
			return Result;
		}
		*pFilterID = 0;
	}
	return ERROR_SUCCESS;
}


template<typename T> int CompareValue(T Value1, T Value2)
{
	return Value1 < Value2 ? -1 : Value1 > Value2 ? 1 : 0;
}

static int CompareAddress(const IPAddress &Address1, const IPAddress &Address2)
{
	if (Address1.Type != Address2.Type)
		return CompareValue(Address1.Type, Address2.Type);
	if (Address1.Type == IP_ADDRESS_V4)
		return CompareValue(Address1.V4.Address, Address2.V4.Address);
	return ::memcmp(Address1.V6.Bytes, Address2.V6.Bytes, 16);
}

bool PacketFilter::FilterCondition::operator<(const FilterCondition &Right) const
{
	if (Match != Right.Match)
		return Match < Right.Match;
	if (Match == MATCH_EQUAL)
		return CompareAddress(Address, Right.Address) < 0;
	int Cmp = CompareAddress(AddressRange.Low, Right.AddressRange.Low);
	if (Cmp != 0)
		return Cmp < 0;
	return CompareAddress(AddressRange.High, Right.AddressRange.High) < 0;
}


PacketFilter::FilterInfo::FilterInfo()
	: ConnectFilterID(0)
	, InFilterID(0)
	, OutFilterID(0)
{
}

}	// namespace CV
