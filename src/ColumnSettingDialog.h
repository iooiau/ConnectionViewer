/******************************************************************************
*                                                                             *
*    ColumnSettingDialog.h                  Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_COLUMN_SETTING_DIALOG_H
#define CV_COLUMN_SETTING_DIALOG_H


#include "ListView.h"


namespace CV
{

class ColumnSettingDialog
{
public:
	ColumnSettingDialog();
	~ColumnSettingDialog();
	bool Show(HINSTANCE hinst, HWND Owner, ListView *pList);

private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

	ListView *m_pListView;
};

}	// namespace CV


#endif	// ndef CV_COLUMN_SETTING_DIALOG_H
