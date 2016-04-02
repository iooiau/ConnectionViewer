/******************************************************************************
*                                                                             *
*    PacketFilter.h                         Copyright(c) 2010-2016 itow,y.    *
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

#ifndef CV_PACKET_FILTER_H
#define CV_PACKET_FILTER_H


#include <map>


namespace CV
{

class PacketFilter
{
public:
	struct FilterCondition
	{
		enum MatchType
		{
			MATCH_EQUAL,
			MATCH_RANGE,
			MATCH_TRAILER
		};

		MatchType Match;
		IPAddress Address;
		struct
		{
			IPAddress Low;
			IPAddress High;
		} AddressRange;

		bool operator<(const FilterCondition &Right) const;
	};

	PacketFilter();
	~PacketFilter();
	bool Open();
	void Close();
	bool IsOpen() const;
	bool BeginFiltering();
	bool EndFiltering();
	bool IsFiltering() const;
	bool AddFilter(const FilterCondition &Condition);
	bool RemoveFilter(const FilterCondition &Condition);
	bool HasFilter(const FilterCondition &Condition) const;
	bool RemoveAllFilters();

private:
	struct FilterInfo
	{
		UINT64 ConnectFilterID;
		UINT64 InFilterID;
		UINT64 OutFilterID;

		FilterInfo();
	};

	typedef std::map<FilterCondition, FilterInfo> FilterList;

	enum class BlockLayer
	{
		INBOUND,
		OUTBOUND,
		CONNECT
	};

	DWORD AddBlockFilter(const FilterCondition &Condition,
						 BlockLayer Layer,
						 UINT64 *pFilterID);
	DWORD RemoveBlockFilter(UINT64 *pFilterID);

	HANDLE m_hEngine;
	GUID m_SubLayerGUID;
	FilterList m_FilterList;
};

}	// namespace CV


#endif	// ndef CV_PACKET_FILTER_H
