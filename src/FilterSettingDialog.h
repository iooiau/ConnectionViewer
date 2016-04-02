/******************************************************************************
*                                                                             *
*    FilterSettingDialog.h                  Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_FILTER_SETTING_DIALOG_H
#define CV_FILTER_SETTING_DIALOG_H


#include "FilterList.h"


namespace CV
{

class FilterSettingDialog
{
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler() = 0;
		virtual bool OnOK(FilterInfo &Filter) { return true; }
	};

	bool Show(HINSTANCE hinst, HWND Owner, FilterInfo *pFilter,
			  EventHandler *pEventHandler = nullptr);

private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

	bool GetAddress(HWND hDlg, int ID, IPAddress *pAddress);

	HINSTANCE m_hinst;
	FilterInfo *m_pFilter;
	EventHandler *m_pEventHandler;
};

}	// namespace CV


#endif	// ndef CV_FILTER_SETTING_DIALOG
