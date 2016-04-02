/******************************************************************************
*                                                                             *
*    FilterList.cpp                         Copyright(c) 2010-2016 itow,y.    *
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
#include "FilterList.h"
#include "Settings.h"
#include "Utility.h"


namespace CV
{

bool FilterInfo::IsEqualCondition(const FilterInfo &Op) const
{
	if (Type != Op.Type || Match != Op.Match)
		return false;
	if (Match == MATCH_EQUAL)
		return Address == Op.Address;
	return AddressRange.Low == Op.AddressRange.Low
		&& AddressRange.High == Op.AddressRange.High;
}


FilterList::FilterList()
	: m_IDCount(0)
{
}

FilterList::~FilterList()
{
}

int FilterList::NumFilters() const
{
	return (int)m_FilterList.size();
}

void FilterList::Clear()
{
	m_FilterList.clear();
}

bool FilterList::AddFilter(const FilterInfo &Filter)
{
	FilterInfo NewFilter(Filter);

	NewFilter.ID = ++m_IDCount;
	m_FilterList.push_back(NewFilter);
	return true;
}

bool FilterList::RemoveFilter(int Index)
{
	if (Index < 0 || (size_t)Index >= m_FilterList.size())
		return false;

	FilterInfoList::iterator i = m_FilterList.begin();
	std::advance(i, Index);
	m_FilterList.erase(i);

	return true;
}

bool FilterList::GetFilter(int Index, FilterInfo *pFilter) const
{
	if (Index < 0 || (size_t)Index >= m_FilterList.size())
		return false;
	*pFilter = m_FilterList[Index];
	return true;
}

bool FilterList::GetFilterByID(UINT ID, FilterInfo *pFilter) const
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return false;
	*pFilter = m_FilterList[Index];
	return true;
}

FilterInfo &FilterList::operator[](size_t Index)
{
	cvDebugAssert(Index < m_FilterList.size());
	return m_FilterList[Index];
}

const FilterInfo &FilterList::operator[](size_t Index) const
{
	cvDebugAssert(Index < m_FilterList.size());
	return m_FilterList[Index];
}

int FilterList::IDToIndex(UINT ID) const
{
	for (size_t i = 0; i < m_FilterList.size(); i++) {
		if (m_FilterList[i].ID == ID)
			return (int)i;
	}
	return -1;
}

bool FilterList::LoadFromFile(LPCTSTR pFileName)
{
	Clear();

	Settings Setting;

	if (!Setting.Open(pFileName, TEXT("Filters"), Settings::OPEN_READ))
		return false;

	for (int i = 0;; i++) {
		TCHAR szKeyName[64];
		int KeyPrefixLength = FormatString(szKeyName, cvLengthOf(szKeyName), TEXT("Filter%d."), i);
		LPTSTR pszKeyProp = szKeyName + KeyPrefixLength;
		int MaxKeyProp = cvLengthOf(szKeyName) - KeyPrefixLength;
		FilterInfo Filter;
		int Type, Match;
		TCHAR szBuffer[64];

		CopyString(pszKeyProp, MaxKeyProp, TEXT("Type"));
		if (!Setting.Read(szKeyName, &Type))
			break;
		if (Type < 0 || Type >= FilterInfo::TYPE_TRAILER)
			continue;
		Filter.Type = (FilterInfo::FilterType)Type;

		CopyString(pszKeyProp, MaxKeyProp, TEXT("Match"));
		if (!Setting.Read(szKeyName, &Match)
				|| Match < 0 || Match >= FilterInfo::MATCH_TRAILER)
			continue;
		Filter.Match = (FilterInfo::MatchType)Match;

		if (Filter.Match == FilterInfo::MATCH_EQUAL) {
			CopyString(pszKeyProp, MaxKeyProp, TEXT("Address"));
			if (!Setting.Read(szKeyName, szBuffer, cvLengthOf(szBuffer))
					|| !Filter.Address.Parse(szBuffer))
				continue;
		} else if (Filter.Match == FilterInfo::MATCH_RANGE) {
			CopyString(pszKeyProp, MaxKeyProp, TEXT("AddressLow"));
			if (!Setting.Read(szKeyName, szBuffer, cvLengthOf(szBuffer))
					|| !Filter.AddressRange.Low.Parse(szBuffer))
				continue;
			CopyString(pszKeyProp, MaxKeyProp, TEXT("AddressHigh"));
			if (!Setting.Read(szKeyName, szBuffer, cvLengthOf(szBuffer))
					|| !Filter.AddressRange.High.Parse(szBuffer))
				continue;
		} else {
			cvDebugTrace(TEXT("Invalid filter match type %d\n"), Filter.Match);
			continue;
		}

		Filter.HostName[0] = _T('\0');
		CopyString(pszKeyProp, MaxKeyProp, TEXT("HostName"));
		Setting.Read(szKeyName, Filter.HostName, cvLengthOf(Filter.HostName));

		Filter.Enable = true;
		CopyString(pszKeyProp, MaxKeyProp, TEXT("Enable"));
		Setting.Read(szKeyName, &Filter.Enable);

		::GetSystemTimeAsFileTime(&Filter.AddedTime);
		CopyString(pszKeyProp, MaxKeyProp, TEXT("AddedTime"));
		if (Setting.Read(szKeyName, szBuffer, cvLengthOf(szBuffer))) {
			ULONGLONG Time = StrToUInt64(szBuffer);
			SYSTEMTIME st;
			FILETIME ft;

			st.wYear  = (WORD)(Time / 10000000000000);
			st.wMonth = (WORD)((Time / 100000000000) % 100);
			st.wDay   = (WORD)((Time / 1000000000) % 100);
			st.wHour  = (WORD)((Time / 10000000) % 100);
			st.wMinute = (WORD)((Time / 100000) % 100);
			st.wSecond = (WORD)((Time / 1000) % 100);
			st.wMilliseconds = (WORD)(Time % 1000);
			if (::SystemTimeToFileTime(&st, &ft))
				Filter.AddedTime = ft;
		}

		Filter.Comment[0] = _T('\0');
		CopyString(pszKeyProp, MaxKeyProp, TEXT("Comment"));
		Setting.Read(szKeyName, Filter.Comment, cvLengthOf(Filter.Comment));

		Filter.ID = ++m_IDCount;

		AddFilter(Filter);
	}

	Setting.Close();

	return true;
}

bool FilterList::SaveToFile(LPCTSTR pFileName) const
{
	Settings Setting;

	if (!Setting.Open(pFileName, TEXT("Filters"), Settings::OPEN_WRITE))
		return false;

	Setting.Clear();

	for (int i = 0; i < (int)m_FilterList.size(); i++) {
		const FilterInfo &Filter = m_FilterList[i];
		TCHAR szKeyName[64];
		int KeyPrefixLength = FormatString(szKeyName, cvLengthOf(szKeyName), TEXT("Filter%d."), i);
		LPTSTR pszKeyProp = szKeyName + KeyPrefixLength;
		int MaxKeyProp = cvLengthOf(szKeyName) - KeyPrefixLength;
		TCHAR szBuffer[64];

		CopyString(pszKeyProp, MaxKeyProp, TEXT("Type"));
		Setting.Write(szKeyName, (int)Filter.Type);

		CopyString(pszKeyProp, MaxKeyProp, TEXT("Match"));
		Setting.Write(szKeyName, (int)Filter.Match);

		if (Filter.Match == FilterInfo::MATCH_EQUAL) {
			FormatIPAddress(Filter.Address, szBuffer, cvLengthOf(szBuffer));
			CopyString(pszKeyProp, MaxKeyProp, TEXT("Address"));
			Setting.Write(szKeyName, szBuffer);
		} else if (Filter.Match == FilterInfo::MATCH_RANGE) {
			FormatIPAddress(Filter.AddressRange.Low, szBuffer, cvLengthOf(szBuffer));
			CopyString(pszKeyProp, MaxKeyProp, TEXT("AddressLow"));
			Setting.Write(szKeyName, szBuffer);
			FormatIPAddress(Filter.AddressRange.High, szBuffer, cvLengthOf(szBuffer));
			CopyString(pszKeyProp, MaxKeyProp, TEXT("AddressHigh"));
			Setting.Write(szKeyName, szBuffer);
		} else {
			cvDebugTrace(TEXT("Invalid filter match type %d\n"), Filter.Match);
		}

		if (Filter.HostName[0] != _T('\0')) {
			CopyString(pszKeyProp, MaxKeyProp, TEXT("HostName"));
			Setting.Write(szKeyName, Filter.HostName);
		}

		CopyString(pszKeyProp, MaxKeyProp, TEXT("Enable"));
		Setting.Write(szKeyName, Filter.Enable);

		SYSTEMTIME st;
		::FileTimeToSystemTime(&Filter.AddedTime, &st);
		FormatString(szBuffer, cvLengthOf(szBuffer),
					 TEXT("%d%02d%02d%02d%02d%02d%03d"),
					 st.wYear, st.wMonth, st.wDay,
					 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		CopyString(pszKeyProp, MaxKeyProp, TEXT("AddedTime"));
		Setting.Write(szKeyName, szBuffer);

		if (Filter.Comment[0] != _T('\0')) {
			CopyString(pszKeyProp, MaxKeyProp, TEXT("Comment"));
			Setting.Write(szKeyName, Filter.Comment);
		}
	}

	Setting.Close();

	return true;
}

}	// namespace CV
