/******************************************************************************
*                                                                             *
*    MiscDialog.h                           Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_MISC_DIALOG_H
#define CV_MISC_DIALOG_H


namespace CV
{

void ErrorDialog(HWND Owner, HINSTANCE hinst, int MessageID);
void SystemErrorDialog(HWND Owner, HINSTANCE hinst, DWORD ErrorCode, int HeaderID = 0, int TitleID = 0);

bool FileOpenDialog(HWND Owner, LPCTSTR pFilter, int *pFilterIndex,
					LPTSTR pFileName, int MaxFileName,
					LPCTSTR pInitialDir = nullptr);
bool FileSaveDialog(HWND Owner, LPCTSTR pFilter, int *pFilterIndex,
					LPTSTR pFileName, int MaxFileName,
					LPCTSTR pInitialDir = nullptr);

bool ChooseColorDialog(HWND Owner, COLORREF *pColor);
bool ChooseFontDialog(HWND Owner, LOGFONT *pFont);

class AboutDialog
{
public:
	bool Show(HINSTANCE hinst, HWND Owner);

private:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
};

}	// namespace CV


#endif	// ndef CV_MISC_DIALOG_H
