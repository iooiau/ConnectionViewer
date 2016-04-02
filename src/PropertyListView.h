/******************************************************************************
*                                                                             *
*    PropertyListView.h                     Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_PROPERTY_LIST_VIEW_H
#define CV_PROPERTY_LIST_VIEW_H


#include <vector>
#include "ListView.h"
#include "ProgramCore.h"


namespace CV
{

class PropertyListView : public ListView
{
public:
	enum
	{
		COLUMN_INDEX,
		COLUMN_NAME,
		COLUMN_VALUE,
		COLUMN_TRAILER
	};
	enum { NUM_COLUMN_TYPES = COLUMN_TRAILER };

	PropertyListView(const ProgramCore &Core);
	~PropertyListView();
	int NumItems() const override;
	bool GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	LPCTSTR GetColumnIDName(int ID) const override;
	void SetItemList(const LPCTSTR *ppItemList, int NumItems);
	bool SetItemValue(int Index, LPCTSTR pValue);

private:
	struct ItemInfo
	{
		int Index;
		TCHAR szName[64];
		TCHAR szValue[MAX_ITEM_TEXT];
	};

	bool SortItems() override;

	friend class PropertyListItemCompare;

	const ProgramCore &m_Core;
	std::vector<ItemInfo> m_ItemList;
};

}	// namespace CV


#endif	// ndef CV_PROPERTY_LIST_VIEW_H
