/******************************************************************************
*                                                                             *
*    StringPool.h                           Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_STRING_POOL_H
#define CV_STRING_POOL_H


#include <set>


namespace CV
{

class StringPool
{
public:
	StringPool();
	~StringPool();
	void Clear();
	LPCTSTR Set(LPCTSTR pString);

private:
	class String
	{
		mutable LPTSTR m_pString;

	public:
		String(LPTSTR pString);
		LPCTSTR Get() const { return m_pString; }
		int Compare(LPCTSTR pString) const;
		bool operator<(const String &RVal) const
		{
			return Compare(RVal.m_pString) < 0;
		}

		friend class StringPool;
	};

	std::set<String> m_Set;
};

}	// namespace CV


#endif	// ndef CV_STRING_POOL_H
