/******************************************************************************
*                                                                             *
*    FilterList.h                           Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_FILTER_LIST_H
#define CV_FILTER_LIST_H


#include <vector>


namespace CV
{
struct FilterInfo
{
	enum FilterType
	{
		TYPE_BLOCK_ADDRESS,
		TYPE_TRAILER
	};

	enum MatchType
	{
		MATCH_EQUAL,
		MATCH_RANGE,
		MATCH_TRAILER
	};

	enum
	{
		MAX_HOST_NAME	= 256,
		MAX_COMMENT		= 256
	};

	UINT ID;
	FilterType Type;
	MatchType Match;
	IPAddress Address;
	struct
	{
		IPAddress Low;
		IPAddress High;
	} AddressRange;
	TCHAR HostName[MAX_HOST_NAME];
	bool Enable;
	FILETIME AddedTime;
	TCHAR Comment[MAX_COMMENT];

	bool IsEqualCondition(const FilterInfo &Op) const;
};

class FilterList
{
public:
	FilterList();
	~FilterList();
	int NumFilters() const;
	void Clear();
	bool AddFilter(const FilterInfo &Filter);
	bool RemoveFilter(int Index);
	bool GetFilter(int Index, FilterInfo *pFilter) const;
	bool GetFilterByID(UINT ID, FilterInfo *pFilter) const;
	FilterInfo &operator[](size_t Index);
	const FilterInfo &operator[](size_t Index) const;
	int IDToIndex(UINT ID) const;
	bool LoadFromFile(LPCTSTR pFileName);
	bool SaveToFile(LPCTSTR pFileName) const;

private:
	typedef std::vector<FilterInfo> FilterInfoList;

	UINT m_IDCount;
	FilterInfoList m_FilterList;
};

}	// namespace CV


#endif	// ndef CV_FILTER_LIST_H
