/******************************************************************************
*                                                                             *
*    InterfaceListView.h                    Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_INTERFACE_LIST_VIEW_H
#define CV_INTERFACE_LIST_VIEW_H


#include <vector>
#include "ProgramCore.h"
#include "ListView.h"


namespace CV
{

class InterfaceListView : public ListView
{
public:
	enum
	{
		COLUMN_INTERFACE_LUID,
		COLUMN_INTERFACE_INDEX,
		COLUMN_INTERFACE_GUID,
		COLUMN_ALIAS,
		COLUMN_DESCRIPTION,
		COLUMN_PHYSICAL_ADDRESS,
		COLUMN_PERMANENT_PHYSICAL_ADDRESS,
		COLUMN_MTU,
		COLUMN_TYPE,
		COLUMN_TUNNEL_TYPE,
		COLUMN_MEDIA_TYPE,
		COLUMN_PHYSICAL_MEDIUM_TYPE,
		COLUMN_ACCESS_TYPE,
		COLUMN_DIRECTION_TYPE,
		COLUMN_OPERATIONAL_STATUS,
		COLUMN_ADMINISTRATIVE_STATUS,
		COLUMN_MEDIA_CONNECTION_STATE,
		COLUMN_NETWORK_GUID,
		COLUMN_CONNECTION_TYPE,
		COLUMN_TRANSMIT_LINK_SPEED,
		COLUMN_RECEIVE_LINK_SPEED,
		COLUMN_IN_OCTETS,
		COLUMN_IN_BANDWIDTH,
		COLUMN_MAX_IN_BANDWIDTH,
		COLUMN_IN_UNICAST_PACKETS,
		COLUMN_IN_NON_UNICAST_PACKETS,
		COLUMN_IN_DISCARDED_PACKETS,
		COLUMN_IN_ERROR_PACKETS,
		COLUMN_IN_UNKNOWN_PROTOCOL_PACKETS,
		COLUMN_IN_UNICAST_OCTETS,
		COLUMN_IN_MULTICAST_OCTETS,
		COLUMN_IN_BROADCAST_OCTETS,
		COLUMN_OUT_OCTETS,
		COLUMN_OUT_BANDWIDTH,
		COLUMN_MAX_OUT_BANDWIDTH,
		COLUMN_OUT_UNICAST_PACKETS,
		COLUMN_OUT_NON_UNICAST_PACKETS,
		COLUMN_OUT_DISCARDED_PACKETS,
		COLUMN_OUT_ERROR_PACKETS,
		COLUMN_OUT_UNICAST_OCTETS,
		COLUMN_OUT_MULTICAST_OCTETS,
		COLUMN_OUT_BROADCAST_OCTESTS,
		COLUMN_TRAILER
	};
	enum { NUM_COLUMN_TYPES = COLUMN_TRAILER };

	struct ItemInfo
	{
		bool Selected;
		MIB_IF_ROW2 IfRow;
		UINT InBytesPerSecond;
		UINT OutBytesPerSecond;
		UINT MaxInBytesPerSecond;
		UINT MaxOutBytesPerSecond;
	};

	static LPCTSTR GetColumnName(int Column);

	InterfaceListView(const ProgramCore &Core);
	~InterfaceListView();
	void OnListUpdated();
	int NumItems() const override;
	bool GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	bool GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	LPCTSTR GetColumnIDName(int ID) const override;

	const ItemInfo *GetItemInfo(const GUID &Guid) const;

private:
	bool OnSelChange(int OldSel, int NewSel) override;
	bool SortItems() override;

	friend class IfListItemCompare;

	const ProgramCore &m_Core;
	std::vector<ItemInfo> m_ItemList;
	ULONGLONG m_UpdatedTickCount;
};

}	// namespace CV


#endif	// ndef CV_INTERFACE_LIST_VIEW_H
