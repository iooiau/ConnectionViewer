/******************************************************************************
*                                                                             *
*    StringPool.cpp                         Copyright(c) 2010-2016 itow,y.    *
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
#include "StringPool.h"
#include "Utility.h"


namespace CV
{

StringPool::StringPool()
{
}

StringPool::~StringPool()
{
	Clear();
}

void StringPool::Clear()
{
	for (std::set<String>::iterator i = m_Set.begin(); i != m_Set.end(); i++) {
		delete [] i->m_pString;
	}
	m_Set.clear();
}

LPCTSTR StringPool::Set(LPCTSTR pString)
{
	if (pString == nullptr)
		return nullptr;

	std::pair<std::set<String>::iterator, bool> Result =
		m_Set.insert(String(const_cast<LPTSTR>(pString)));
	if (Result.second)
		Result.first->m_pString = DuplicateString(pString);
	return Result.first->Get();
}


StringPool::String::String(LPTSTR pString)
	: m_pString(pString)
{
}

int StringPool::String::Compare(LPCTSTR pString) const
{
	LPCTSTR p1, p2;

	p1 = m_pString;
	p2 = pString;
	while (*p1 == *p2) {
		if (*p1 == _T('\0'))
			return 0;
		p1++;
		p2++;
	}
	return (int) * p1 - (int) * p2;
}

}	// namespace CV
