/******************************************************************************
*                                                                             *
*    FilterSettingDialog.cpp                Copyright(c) 2010-2016 itow,y.    *
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
#include "FilterSettingDialog.h"
#include "MiscDialog.h"
#include "Utility.h"
#include "resource.h"


namespace CV
{

bool FilterSettingDialog::Show(HINSTANCE hinst, HWND Owner, FilterInfo *pFilter,
							   EventHandler *pEventHandler)
{
	m_hinst = hinst;
	m_pFilter = pFilter;
	m_pEventHandler = pEventHandler;
	return ::DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_FILTER),
							Owner, DlgProc,
							reinterpret_cast<LPARAM>(this)) == IDOK;
}

INT_PTR CALLBACK FilterSettingDialog::DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_INITDIALOG:
		{
			FilterSettingDialog *pThis = reinterpret_cast<FilterSettingDialog*>(lParam);

			::SetProp(hDlg, TEXT("This"), pThis);

			FilterInfo *pFilter = pThis->m_pFilter;

			TCHAR szAddress[64];
			if (pFilter->Match == FilterInfo::MATCH_EQUAL) {
				FormatIPAddress(pFilter->Address, szAddress, cvLengthOf(szAddress));
				::SetDlgItemText(hDlg, IDC_FILTER_ADDRESS_LOW, szAddress);
				//::SetDlgItemText(hDlg,IDC_FILTER_ADDRESS_HIGH,szAddress);
			} else if (pFilter->Match == FilterInfo::MATCH_RANGE) {
				FormatIPAddress(pFilter->AddressRange.Low, szAddress, cvLengthOf(szAddress));
				::SetDlgItemText(hDlg, IDC_FILTER_ADDRESS_LOW, szAddress);
				FormatIPAddress(pFilter->AddressRange.High, szAddress, cvLengthOf(szAddress));
				::SetDlgItemText(hDlg, IDC_FILTER_ADDRESS_HIGH, szAddress);
			}

			::SendDlgItemMessage(hDlg, IDC_FILTER_COMMENT, EM_LIMITTEXT,
								 FilterInfo::MAX_COMMENT - 1, 0);
			::SetDlgItemText(hDlg, IDC_FILTER_COMMENT, pFilter->Comment);

			MoveWindowToCenter(hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				FilterSettingDialog *pThis =
					static_cast<FilterSettingDialog*>(::GetProp(hDlg, TEXT("This")));

				IPAddress AddressLow, AddressHigh;
				if (!pThis->GetAddress(hDlg, IDC_FILTER_ADDRESS_LOW, &AddressLow))
					return TRUE;
				if (::GetWindowTextLength(::GetDlgItem(hDlg, IDC_FILTER_ADDRESS_HIGH)) > 0) {
					if (!pThis->GetAddress(hDlg, IDC_FILTER_ADDRESS_HIGH, &AddressHigh))
						return TRUE;
					if (AddressLow.Type != AddressHigh.Type) {
						ErrorDialog(hDlg, pThis->m_hinst, IDS_ERROR_FILTER_ADDRESS_MISMATCH);
						return TRUE;
					}
					if (AddressLow > AddressHigh) {
						ErrorDialog(hDlg, pThis->m_hinst, IDS_ERROR_FILTER_ADDRESS_RANGE);
						return TRUE;
					}
				} else {
					AddressHigh = AddressLow;
				}

				FilterInfo Filter;
				Filter.Type = FilterInfo::TYPE_BLOCK_ADDRESS;
				if (AddressLow == AddressHigh) {
					Filter.Match = FilterInfo::MATCH_EQUAL;
					Filter.Address = AddressLow;
				} else {
					Filter.Match = FilterInfo::MATCH_RANGE;
					Filter.AddressRange.Low = AddressLow;
					Filter.AddressRange.High = AddressHigh;
				}
				::lstrcpy(Filter.HostName, pThis->m_pFilter->HostName);
				Filter.Enable = pThis->m_pFilter->Enable;
				Filter.AddedTime = pThis->m_pFilter->AddedTime;
				::GetDlgItemText(hDlg, IDC_FILTER_COMMENT,
								 Filter.Comment, cvLengthOf(Filter.Comment));

				if (pThis->m_pEventHandler != nullptr) {
					if (!pThis->m_pEventHandler->OnOK(Filter))
						return TRUE;
				}

				*pThis->m_pFilter = Filter;
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

bool FilterSettingDialog::GetAddress(HWND hDlg, int ID, IPAddress *pAddress)
{
	const HWND hwndEdit = ::GetDlgItem(hDlg, ID);
	TCHAR szAddress[64];

	::GetWindowText(hwndEdit, szAddress, cvLengthOf(szAddress));
	if (!pAddress->Parse(szAddress)) {
		::SetFocus(hwndEdit);
		::SendMessage(hwndEdit, EM_SETSEL, 0, -1);
		::TaskDialog(hDlg, m_hinst,
					 MAKEINTRESOURCE(IDS_ERROR_FILTER_ADDRESS_TITLE),
					 MAKEINTRESOURCE(IDS_ERROR_FILTER_ADDRESS_HEADER),
					 MAKEINTRESOURCE(IDS_ERROR_FILTER_ADDRESS_MESSAGE),
					 TDCBF_OK_BUTTON, nullptr, nullptr);
		return false;
	}
	return true;
}


FilterSettingDialog::EventHandler::~EventHandler()
{
}

}	// namespace CV
