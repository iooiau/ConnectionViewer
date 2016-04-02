/******************************************************************************
*                                                                             *
*    FilterManager.cpp                      Copyright(c) 2010-2016 itow,y.    *
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
#include "FilterManager.h"


namespace CV
{

static bool FilterInfoToCondition(const FilterInfo &Info,
								  PacketFilter::FilterCondition *pCondition)
{
	if (Info.Type != FilterInfo::TYPE_BLOCK_ADDRESS)
		return false;
	if (Info.Match == FilterInfo::MATCH_EQUAL) {
		pCondition->Match = PacketFilter::FilterCondition::MATCH_EQUAL;
		pCondition->Address = Info.Address;
	} else if (Info.Match == FilterInfo::MATCH_RANGE) {
		pCondition->Match = PacketFilter::FilterCondition::MATCH_RANGE;
		pCondition->AddressRange.Low = Info.AddressRange.Low;
		pCondition->AddressRange.High = Info.AddressRange.High;
	} else {
		return false;
	}
	return true;
}


FilterManager::FilterManager()
	: m_LastError(ERROR_SUCCESS)
{
}

FilterManager::~FilterManager()
{
}

void FilterManager::Clear()
{
	m_PacketFilter.RemoveAllFilters();
	m_FilterList.Clear();
}

int FilterManager::NumFilters() const
{
	return m_FilterList.NumFilters();
}

bool FilterManager::AddFilter(const FilterInfo &Filter)
{
	for (int i = 0; i < m_FilterList.NumFilters(); i++) {
		const FilterInfo &Info = m_FilterList[i];

		if (Info.IsEqualCondition(Filter)) {
			m_LastError = ERROR_ALREADY_EXISTS;
			return false;
		}
	}

	if (Filter.Enable) {
		if (!ActivateFilter(Filter)) {
			return false;
		}
	}

	m_FilterList.AddFilter(Filter);

	m_LastError = ERROR_SUCCESS;

	return true;
}

bool FilterManager::RemoveFilter(int Index)
{
	if (Index < 0 || Index >= m_FilterList.NumFilters()) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	FilterInfo Filter;
	m_FilterList.GetFilter(Index, &Filter);

	if (Filter.Enable) {
		if (!DeactivateFilter(Filter))
			return false;
	}

	m_FilterList.RemoveFilter(Index);

	m_LastError = ERROR_SUCCESS;

	return true;
}

bool FilterManager::GetFilter(int Index, FilterInfo *pFilter) const
{
	return m_FilterList.GetFilter(Index, pFilter);
}

bool FilterManager::GetFilterByID(UINT ID, FilterInfo *pFilter) const
{
	return m_FilterList.GetFilterByID(ID, pFilter);
}

bool FilterManager::SetFilter(int Index, const FilterInfo &Filter)
{
	if (Index < 0 || Index >= m_FilterList.NumFilters()) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	for (int i = 0; i < m_FilterList.NumFilters(); i++) {
		const FilterInfo &Info = m_FilterList[i];

		if (Info.IsEqualCondition(Filter) && i != Index) {
			m_LastError = ERROR_ALREADY_EXISTS;
			return false;
		}
	}

	FilterInfo &CurFilter = m_FilterList[Index];

	if (!CurFilter.IsEqualCondition(Filter)) {
		if (CurFilter.Enable) {
			if (!DeactivateFilter(CurFilter))
				return false;
			CurFilter.Enable = false;
		}
	}
	if (CurFilter.Enable != Filter.Enable) {
		bool Result;
		if (Filter.Enable)
			Result = ActivateFilter(Filter);
		else
			Result = DeactivateFilter(Filter);
		if (!Result)
			return false;
	}

	CurFilter = Filter;

	m_LastError = ERROR_SUCCESS;

	return true;
}

bool FilterManager::EnableFilter(int Index, bool Enable)
{
	if (Index < 0 || Index >= m_FilterList.NumFilters()) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	FilterInfo &Filter = m_FilterList[Index];
	if (Filter.Enable != Enable) {
		bool Result;
		if (Enable)
			Result = ActivateFilter(Filter);
		else
			Result = DeactivateFilter(Filter);
		if (!Result)
			return false;
		Filter.Enable = Enable;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}

bool FilterManager::IsFilterActive(int Index) const
{
	if (Index < 0 || Index >= m_FilterList.NumFilters())
		return false;

	PacketFilter::FilterCondition Condition;

	if (!FilterInfoToCondition(m_FilterList[Index], &Condition))
		return false;

	return m_PacketFilter.HasFilter(Condition);
}

bool FilterManager::SetActive(bool Active)
{
	bool Result;

	if (Active) {
		if (!m_PacketFilter.IsOpen()) {
			if (!m_PacketFilter.Open()) {
				m_LastError = ::GetLastError();
				return false;
			}
		}
		Result = m_PacketFilter.BeginFiltering();
	} else {
		Result = m_PacketFilter.EndFiltering();
	}

	m_LastError = ::GetLastError();

	return Result;
}

bool FilterManager::IsActive() const
{
	return m_PacketFilter.IsFiltering();
}

bool FilterManager::LoadFilterList(LPCTSTR pFileName)
{
	Clear();

	FilterList List;
	if (!List.LoadFromFile(pFileName))
		return false;

	for (int i = 0; i < List.NumFilters(); i++)
		AddFilter(List[i]);

	return true;
}

bool FilterManager::SaveFilterList(LPCTSTR pFileName) const
{
	return m_FilterList.SaveToFile(pFileName);
}

DWORD FilterManager::GetLastError() const
{
	return m_LastError;
}

bool FilterManager::ActivateFilter(const FilterInfo &Filter)
{
	PacketFilter::FilterCondition Condition;

	if (!FilterInfoToCondition(Filter, &Condition)) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}
	if (!m_PacketFilter.AddFilter(Condition)) {
		m_LastError = ::GetLastError();
		return false;
	}
	return true;
}

bool FilterManager::DeactivateFilter(const FilterInfo &Filter)
{
	PacketFilter::FilterCondition Condition;

	if (!FilterInfoToCondition(Filter, &Condition)) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}
	if (!m_PacketFilter.RemoveFilter(Condition)) {
		m_LastError = ::GetLastError();
		return false;
	}
	return true;
}

}	// namespace CV
