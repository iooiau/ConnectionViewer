/******************************************************************************
*                                                                             *
*    ColumnSettingDialog.cpp                Copyright(c) 2010-2016 itow,y.    *
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
#include <uxtheme.h>
#include "ColumnSettingDialog.h"
#include "Utility.h"
#include "resource.h"

#pragma comment(lib, "uxtheme.lib")


namespace CV
{

ColumnSettingDialog::ColumnSettingDialog()
	: m_pListView(nullptr)
{
}

ColumnSettingDialog::~ColumnSettingDialog()
{
}

bool ColumnSettingDialog::Show(HINSTANCE hinst, HWND Owner, ListView *pList)
{
	m_pListView = pList;
	return ::DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_COLUMNS),
							Owner, DlgProc,
							reinterpret_cast<LPARAM>(this)) == IDOK;
}

static void SetDialogItemState(HWND hDlg)
{
	const HWND hwndList = ::GetDlgItem(hDlg, IDC_COLUMNS_LIST);
	const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	const int ItemCount = ListView_GetItemCount(hwndList);

	::EnableWindow(::GetDlgItem(hDlg, IDC_COLUMNS_UP), Sel > 0);
	::EnableWindow(::GetDlgItem(hDlg, IDC_COLUMNS_DOWN), Sel >= 0 && Sel + 1 < ItemCount);
}

INT_PTR CALLBACK ColumnSettingDialog::DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_INITDIALOG:
		{
			ColumnSettingDialog *pThis = reinterpret_cast<ColumnSettingDialog*>(lParam);
	
			::SetProp(hDlg, TEXT("This"), pThis);
	
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_COLUMNS_LIST);
	
			::SetWindowTheme(hwndList, L"Explorer", nullptr);
	
			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	
			RECT rc;
			::GetClientRect(hwndList, &rc);
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = rc.right -::GetSystemMetrics(SM_CXVSCROLL);
			lvc.pszText = TEXT("");
			ListView_InsertColumn(hwndList, 0, &lvc);
	
			const int NumColumns = pThis->m_pListView->NumColumns();
			ListView_SetItemCount(hwndList, NumColumns);
			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem = 0;
			for (int i = 0; i < NumColumns; i++) {
				ListView::ColumnInfo Column;
	
				pThis->m_pListView->GetColumnInfo(i, &Column);
				lvi.iItem = i;
				lvi.pszText = Column.szText;
				lvi.lParam = Column.ID;
				ListView_InsertItem(hwndList, &lvi);
				ListView_SetCheckState(hwndList, i, Column.Visible);
			}
	
			SetDialogItemState(hDlg);
	
			MoveWindowToCenter(hDlg);
		}
		return TRUE;

	case WM_NOTIFY:
		{
			LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
	
			switch (pnmh->code) {
			case LVN_ITEMCHANGED:
				SetDialogItemState(hDlg);
				return TRUE;
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COLUMNS_UP:
		case IDC_COLUMNS_DOWN:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_COLUMNS_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	
				if (Sel >= 0) {
					int To;
	
					if (LOWORD(wParam) == IDC_COLUMNS_UP) {
						if (Sel < 1)
							break;
						To = Sel - 1;
					} else {
						if (Sel + 1 > ListView_GetItemCount(hwndList))
							break;
						To = Sel + 1;
					}
	
					LVITEM lvi1, lvi2;
					TCHAR szText1[ListView::MAX_COLUMN_TEXT];
					TCHAR szText2[ListView::MAX_COLUMN_TEXT];
	
					lvi1.mask = LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi1.iItem = Sel;
					lvi1.iSubItem = 0;
					lvi1.stateMask = (UINT) - 1;
					lvi1.pszText = szText1;
					lvi1.cchTextMax = cvLengthOf(szText1);
					lvi2 = lvi1;
					ListView_GetItem(hwndList, &lvi1);
					lvi2.iItem = To;
					lvi2.pszText = szText2;
					ListView_GetItem(hwndList, &lvi2);
					BOOL Check1 = ListView_GetCheckState(hwndList, Sel);
					BOOL Check2 = ListView_GetCheckState(hwndList, To);
					lvi2.iItem = Sel;
					ListView_SetItem(hwndList, &lvi2);
					ListView_SetCheckState(hwndList, Sel, Check2);
					lvi1.iItem = To;
					ListView_SetItem(hwndList, &lvi1);
					ListView_SetCheckState(hwndList, To, Check1);
	
					SetDialogItemState(hDlg);
				}
			}
			return TRUE;

		case IDOK:
			{
				ColumnSettingDialog *pThis =
					static_cast<ColumnSettingDialog*>(::GetProp(hDlg, TEXT("This")));
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_COLUMNS_LIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				std::vector<int> ColumnOrder;
				ColumnOrder.reserve(ItemCount);
	
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iSubItem = 0;
				for (int i = 0; i < ItemCount; i++) {
					lvi.iItem = i;
					ListView_GetItem(hwndList, &lvi);
					ColumnOrder.push_back((int)lvi.lParam);
				}
				pThis->m_pListView->SetColumnOrder(ColumnOrder);
				// スクロール位置を維持するために、表示->非表示の順に設定しています
				for (int i = 0; i < ItemCount; i++) {
					if (ListView_GetCheckState(hwndList, i))
						pThis->m_pListView->SetColumnVisible(ColumnOrder[i], true);
				}
				for (int i = 0; i < ItemCount; i++) {
					if (!ListView_GetCheckState(hwndList, i))
						pThis->m_pListView->SetColumnVisible(ColumnOrder[i], false);
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg, TEXT("This"));
		return TRUE;
	}

	return FALSE;
}

}	// namespace CV
