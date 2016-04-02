/******************************************************************************
*                                                                             *
*    BlockListView.h                        Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_BLOCK_LIST_VIEW_H
#define CV_BLOCK_LIST_VIEW_H


#include "ListView.h"
#include "ProgramCore.h"


namespace CV
{

class BlockListView : public ListView
{
public:
	enum
	{
		COLUMN_STATUS,
		COLUMN_REMOTE_ADDRESS,
		COLUMN_REMOTE_HOST,
		COLUMN_COUNTRY,
		COLUMN_CITY,
		COLUMN_LOCATION,
		COLUMN_ADDED_TIME,
		COLUMN_COMMENT,
		COLUMN_TRAILER
	};
	enum { NUM_COLUMN_TYPES = COLUMN_TRAILER };

	BlockListView(const ProgramCore &Core);
	~BlockListView();
	void OnListUpdated();
	int NumItems() const override;
	bool GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	bool GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	LPCTSTR GetColumnIDName(int ID) const override;
	int GetItemFilterIndex(int Item) const;
	int FilterIDToIndex(UINT ID) const;

private:
	struct ItemInfo
	{
		int Index;
		bool Selected;
		FilterInfo Info;
		bool EnableCityInfo;
		GeoIPManager::CityInfo CityInfo;
	};

	void DrawItem(HDC hdc, int Row, const ListView::ColumnInfo &Column,
				  const RECT &rcBound, const RECT &rcItem) override;
	bool OnSelChange(int OldSel, int NewSel) override;
	bool SortItems() override;

	friend class BlockItemCompare;

	const ProgramCore &m_Core;
	std::vector<ItemInfo> m_ItemList;
};

}	// namespace CV


#endif	// ndef CV_BLOCK_LIST_VIEW_H
