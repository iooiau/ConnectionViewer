/******************************************************************************
*                                                                             *
*    ConnectionLogView.h                    Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_CONNECTION_LOG_VIEW_H
#define CV_CONNECTION_LOG_VIEW_H


#include "ListView.h"
#include "ProgramCore.h"


namespace CV
{

class ConnectionLogView : public ListView
{
public:
	enum
	{
		COLUMN_CREATED_TIME,
		COLUMN_UPDATED_TIME,
		COLUMN_PROCESS_NAME,
		COLUMN_PROCESS_PATH,
		COLUMN_PROCESS_ID,
		COLUMN_PROTOCOL,
		COLUMN_LOCAL_ADDRESS,
		COLUMN_LOCAL_PORT,
		COLUMN_REMOTE_ADDRESS,
		COLUMN_REMOTE_PORT,
		COLUMN_REMOTE_HOST,
		COLUMN_COUNTRY,
		COLUMN_CITY,
		COLUMN_LOCATION,
		COLUMN_STATE,
		COLUMN_DURATION,
		COLUMN_IN_BYTES,
		COLUMN_OUT_BYTES,
		COLUMN_IN_BANDWIDTH,
		COLUMN_OUT_BANDWIDTH,
		COLUMN_MAX_IN_BANDWIDTH,
		COLUMN_MAX_OUT_BANDWIDTH,
		COLUMN_TRAILER
	};
	enum { NUM_COLUMN_TYPES = COLUMN_TRAILER };

	static LPCTSTR GetColumnName(int Column);

	ConnectionLogView(const ProgramCore &Core, const ConnectionLog &Log);
	~ConnectionLogView();
	void OnListUpdated();
	int NumItems() const override;
	bool GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	bool GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const override;
	LPCTSTR GetColumnIDName(int ID) const override;
	void SetSpecialColors(COLORREF NewItemBackColor);
	bool GetItemConnectionInfo(int Item, ConnectionInfo *pInfo) const;

private:
	bool DrawItemBackground(HDC hdc, int Row, const RECT &rcBound) override;
	bool OnSelChange(int OldSel, int NewSel) override;
	bool SortItems() override;

	struct ItemInfo
	{
		enum
		{
			FLAG_NEW		= 0x0001,
			FLAG_SELECTED	= 0x0002
		};

		unsigned int Flags;
		ULONGLONG ID;
		ConnectionLog::ItemList::const_iterator Iterator;
	};

	friend class LogItemCompare;

	const ProgramCore &m_Core;
	const ConnectionLog &m_Log;
	std::vector<ItemInfo> m_ItemList;
	COLORREF m_NewItemBackColor;
};

}	// namespace CV


#endif	// ndef CV_CONNECTION_LOG_VIEW_H
