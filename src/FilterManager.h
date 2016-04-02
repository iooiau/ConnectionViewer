/******************************************************************************
*                                                                             *
*    FilterManager.h                        Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_FILTER_MANAGER_H
#define CV_FILTER_MANAGER_H


#include "PacketFilter.h"
#include "FilterList.h"


namespace CV
{

class FilterManager
{
public:
	FilterManager();
	~FilterManager();
	void Clear();
	int NumFilters() const;
	bool AddFilter(const FilterInfo &Filter);
	bool RemoveFilter(int Index);
	bool GetFilter(int Index, FilterInfo *pFilter) const;
	bool GetFilterByID(UINT ID, FilterInfo *pFilter) const;
	bool SetFilter(int Index, const FilterInfo &Filter);
	bool EnableFilter(int Index, bool Enable);
	bool IsFilterActive(int Index) const;
	bool SetActive(bool Active);
	bool IsActive() const;
	bool LoadFilterList(LPCTSTR pFileName);
	bool SaveFilterList(LPCTSTR pFileName) const;
	DWORD GetLastError() const;

private:
	bool ActivateFilter(const FilterInfo &Filter);
	bool DeactivateFilter(const FilterInfo &Filter);

	PacketFilter m_PacketFilter;
	FilterList m_FilterList;
	DWORD m_LastError;
};

}	// namespace CV


#endif	// ndef CV_FILTER_MANAGER_H
