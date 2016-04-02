/******************************************************************************
*                                                                             *
*    MiscDialog.cpp                         Copyright(c) 2010-2016 itow,y.    *
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
#include "MiscDialog.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

void ErrorDialog(HWND Owner, HINSTANCE hinst, int MessageID)
{
	MSGBOXPARAMS mbp;

	::ZeroMemory(&mbp, sizeof(mbp));
	mbp.cbSize = sizeof(mbp);
	mbp.hwndOwner = Owner;
	mbp.hInstance = hinst;
	mbp.lpszText = MAKEINTRESOURCE(MessageID);
	mbp.lpszCaption = MAKEINTRESOURCE(IDS_ERROR_CAPTION);
	mbp.dwStyle = MB_OK | MB_ICONEXCLAMATION;
	::MessageBoxIndirect(&mbp);
}

void SystemErrorDialog(HWND Owner, HINSTANCE hinst, DWORD ErrorCode, int HeaderID, int TitleID)
{
	TCHAR szMessage[1024];

	if (GetSystemErrorMessage(ErrorCode, szMessage, cvLengthOf(szMessage)) <= 0)
		FormatString(szMessage, cvLengthOf(szMessage), TEXT("(Error code %x)"), ErrorCode);
	::TaskDialog(Owner, hinst,
				 MAKEINTRESOURCE(TitleID),
				 MAKEINTRESOURCE(HeaderID),
				 szMessage,
				 TDCBF_CLOSE_BUTTON,
				 TD_WARNING_ICON,
				 nullptr);
}

bool FileOpenDialog(HWND Owner, LPCTSTR pFilter, int *pFilterIndex,
					LPTSTR pFileName, int MaxFileName,
					LPCTSTR pInitialDir)
{
	OPENFILENAME ofn;

	::ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = Owner;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = pFilter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nFilterIndex = *pFilterIndex + 1;
	ofn.lpstrFile = pFileName;
	ofn.nMaxFile = MaxFileName;
	ofn.lpstrFileTitle = nullptr;
	ofn.lpstrInitialDir = pInitialDir;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;
	bool Result = ::GetOpenFileName(&ofn) != FALSE;
	*pFilterIndex = ofn.nFilterIndex - 1;
	return Result;
}

bool FileSaveDialog(HWND Owner, LPCTSTR pFilter, int *pFilterIndex,
					LPTSTR pFileName, int MaxFileName,
					LPCTSTR pInitialDir)
{
	OPENFILENAME ofn;

	::ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = Owner;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = pFilter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nFilterIndex = *pFilterIndex + 1;
	ofn.lpstrFile = pFileName;
	ofn.nMaxFile = MaxFileName;
	ofn.lpstrFileTitle = nullptr;
	ofn.lpstrInitialDir = pInitialDir;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;
	bool Result = ::GetSaveFileName(&ofn) != FALSE;
	*pFilterIndex = ofn.nFilterIndex - 1;
	return Result;
}

bool ChooseColorDialog(HWND Owner, COLORREF *pColor)
{
	static COLORREF CustomColors[16] = {
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF)
	};
	static bool FullOpen = true;
	CHOOSECOLOR cc;

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = Owner;
	cc.hInstance = nullptr;
	cc.rgbResult = *pColor;
	cc.lpCustColors = CustomColors;
	cc.Flags = CC_RGBINIT;
	if (FullOpen)
		cc.Flags |= CC_FULLOPEN;
	if (!::ChooseColor(&cc))
		return false;
	*pColor = cc.rgbResult;
	FullOpen = (cc.Flags & CC_FULLOPEN) != 0;
	return true;
}

bool ChooseFontDialog(HWND Owner, LOGFONT *pFont)
{
	CHOOSEFONT cf;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = Owner;
	cf.lpLogFont = pFont;
	cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	return ::ChooseFont(&cf) != FALSE;
}


bool AboutDialog::Show(HINSTANCE hinst, HWND Owner)
{
	return ::DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), Owner, DlgProc) >= 0;
}

INT_PTR CALLBACK AboutDialog::DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hfontLink;

	switch (Msg) {
	case WM_INITDIALOG:
		{
			LOGFONT lf;

			::GetObject((HFONT)::SendMessage(hDlg, WM_GETFONT, 0, 0), sizeof(LOGFONT), &lf);
			lf.lfUnderline = 1;
			hfontLink = ::CreateFontIndirect(&lf);
			::SendDlgItemMessage(hDlg, IDC_ABOUT_WEB, WM_SETFONT, (WPARAM)hfontLink, FALSE);

			MoveWindowToCenter(hDlg);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == ::GetDlgItem(hDlg, IDC_ABOUT_WEB)) {
			::SetTextColor((HDC)wParam, RGB(0, 0, 255));
			::SetBkMode((HDC)wParam, TRANSPARENT);
			return (INT_PTR)::GetStockObject(NULL_BRUSH);
		}
		break;

	case WM_SETCURSOR:
		if ((HWND)wParam == ::GetDlgItem(hDlg, IDC_ABOUT_WEB)) {
			::SetCursor(::LoadCursor(nullptr, IDC_HAND));
			::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ABOUT_WEB:
			if (HIWORD(wParam) == STN_CLICKED) {
				TCHAR szURL[256];

				::GetDlgItemText(hDlg, LOWORD(wParam), szURL, cvLengthOf(szURL));
				::ShellExecute(nullptr, TEXT("open"), szURL,
							   nullptr, nullptr, SW_SHOWNORMAL);
			}
			return TRUE;
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
		}
		return TRUE;

	case WM_DESTROY:
		::DeleteObject(hfontLink);
		return TRUE;
	}

	return FALSE;
}

}	// namespace CV
