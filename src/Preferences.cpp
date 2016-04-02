/******************************************************************************
*                                                                             *
*    Preferences.cpp                        Copyright(c) 2010-2016 itow,y.    *
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
#include "Preferences.h"
#include "Utility.h"
#include "MiscDialog.h"
#include "resource.h"


namespace CV
{

CorePreferences::CorePreferences()
{
	SetDefault();
}

void CorePreferences::SetDefault()
{
	GeoIPDatabaseFileName[0] = '\0';
}


ListPreferences::ListPreferences()
{
	SetDefault();
}

void ListPreferences::SetDefault()
{
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &Font);
	ShowGrid = true;
	TextColor = RGB(0, 0, 0);
	GridColor = RGB(224, 224, 224);
	BackColor1 = RGB(255, 255, 255);
	BackColor2 = RGB(240, 240, 240);
	SelTextColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	SelBackColor = ::GetSysColor(COLOR_HIGHLIGHT);
	NewBackColor = RGB(255, 255, 224);
}


LogPreferences::LogPreferences()
{
	SetDefault();
}

void LogPreferences::SetDefault()
{
	MaxLog = 1000;
}


GraphPreferences::GraphPreferences()
{
	SetDefault();
}

void GraphPreferences::SetDefault()
{
	BackColor = RGB(0, 0, 0);
	GridColor = RGB(0, 64, 64);
	CaptionColor = RGB(255, 255, 255);
	InBandwidth.Type = GraphInfo::TYPE_AREA;
	InBandwidth.Color = RGB(0, 255, 128);
	InBandwidth.Scale = 10 * 1024 * 1024;
	OutBandwidth.Type = GraphInfo::TYPE_AREA;
	OutBandwidth.Color = RGB(0, 128, 255);
	OutBandwidth.Scale = 10 * 1024 * 1024;
	Connections.Type = GraphInfo::TYPE_LINE;
	Connections.Color = RGB(255, 255, 128);
	Connections.Scale = 100;
}


void AllPreferences::SetDefault()
{
	List.SetDefault();
	Log.SetDefault();
	Graph.SetDefault();
}


int PreferencesDialog::m_StartPage = 0;

PreferencesDialog::PreferencesDialog(AllPreferences &Pref, EventHandler *pHandler)
	: m_CurPref(Pref)
	, m_pEventHandler(pHandler)
{
}

PreferencesDialog::~PreferencesDialog()
{
}

bool PreferencesDialog::Show(HINSTANCE hinst, HWND Owner)
{
	PROPSHEETPAGE psp[2];
	for (int i = 0; i < cvLengthOf(psp); i++) {
		psp[i].dwSize = sizeof(PROPSHEETPAGE);
		psp[i].dwFlags = PSP_DEFAULT;
		psp[i].hInstance = hinst;
		psp[i].lParam = reinterpret_cast<LPARAM>(this);
	}
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PREF_GENERAL);
	psp[0].pfnDlgProc = GeneralDlgProc;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PREF_GRAPH);
	psp[1].pfnDlgProc = GraphDlgProc;

	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_PROPSHEETPAGE;
	psh.hwndParent = Owner;
	psh.hInstance = hinst;
	psh.pszCaption = MAKEINTRESOURCE(IDS_PREF_CAPTION);
	psh.nPages = cvLengthOf(psp);
	psh.nStartPage = m_StartPage;
	psh.ppsp = psp;

	m_TempPref = m_CurPref;

	if (::PropertySheet(&psh) <= 0)
		return false;

	if (m_pEventHandler != nullptr)
		m_pEventHandler->ApplyPreferences(m_TempPref);
	m_CurPref = m_TempPref;

	return true;
}

const ListPreferences &PreferencesDialog::GetListPreferences() const
{
	return m_CurPref.List;
}

static void SetFontInfo(HWND hDlg, int ID, const LOGFONT &Font)
{
	HDC hdc = ::GetDC(hDlg);
	TCHAR szText[LF_FACESIZE + 16];

	FormatString(szText, cvLengthOf(szText), TEXT("%s, %dpt"),
				 Font.lfFaceName, CalcFontPointSize(hdc, Font));
	::ReleaseDC(hDlg, hdc);
	::SetDlgItemText(hDlg, ID, szText);
}

static void RedrawDlgItem(HWND hDlg, int ID)
{
	::RedrawWindow(::GetDlgItem(hDlg, ID), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

static void DrawColorButton(HDC hdc, const RECT &Rect, COLORREF Color)
{
	RECT rc = Rect;

	::InflateRect(&rc, -8, -6);
	HGDIOBJ OldPen = ::SelectObject(hdc, ::GetStockObject(WHITE_PEN));
	HBRUSH Brush = ::CreateSolidBrush(Color);
	HGDIOBJ OldBrush = ::SelectObject(hdc, Brush);
	::Rectangle(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1);
	::SelectObject(hdc, ::GetStockObject(BLACK_PEN));
	::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
	::InflateRect(&rc, 1, 1);
	::Rectangle(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1);
	::SelectObject(hdc, OldBrush);
	::SelectObject(hdc, OldPen);
	::DeleteObject(Brush);
}

INT_PTR CALLBACK PreferencesDialog::GeneralDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_INITDIALOG:
		{
			PROPSHEETPAGE *pPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
			PreferencesDialog *pThis = reinterpret_cast<PreferencesDialog*>(pPage->lParam);

			::SetProp(hDlg, TEXT("This"), pThis);

			SetFontInfo(hDlg, IDC_PREF_FONT_INFO, pThis->m_TempPref.List.Font);
			::CheckDlgButton(hDlg, IDC_PREF_SHOWGRID,
							 pThis->m_TempPref.List.ShowGrid ? BST_CHECKED : BST_UNCHECKED);

			::SetDlgItemInt(hDlg, IDC_PREF_MAXLOG, pThis->m_TempPref.Log.MaxLog, FALSE);
			::SendDlgItemMessage(hDlg, IDC_PREF_MAXLOG_SPIN, UDM_SETRANGE32, 0, 0x7FFFFFFF);

			::SendDlgItemMessage(hDlg, IDC_PREF_GEOIP_DATABASE, EM_LIMITTEXT,
								 cvLengthOf(pThis->m_TempPref.Core.GeoIPDatabaseFileName) - 1, 0);
			::SetDlgItemText(hDlg, IDC_PREF_GEOIP_DATABASE,
							 pThis->m_TempPref.Core.GeoIPDatabaseFileName);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case NM_CUSTOMDRAW:
			{
				LPNMCUSTOMDRAW pnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

				if ((pnmcd->hdr.idFrom == IDC_PREF_COLOR_TEXT
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_GRID
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_BACK1
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_BACK2
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_NEW_BACK
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_SEL_TEXT
						|| pnmcd->hdr.idFrom == IDC_PREF_COLOR_SEL_BACK)
						&& pnmcd->dwDrawStage == CDDS_PREPAINT) {
					PreferencesDialog *pThis = GetThis(hDlg);
					COLORREF Color;

					switch (pnmcd->hdr.idFrom) {
					case IDC_PREF_COLOR_TEXT:
						Color = pThis->m_TempPref.List.TextColor;
						break;
					case IDC_PREF_COLOR_GRID:
						Color = pThis->m_TempPref.List.GridColor;
						break;
					case IDC_PREF_COLOR_BACK1:
						Color = pThis->m_TempPref.List.BackColor1;
						break;
					case IDC_PREF_COLOR_BACK2:
						Color = pThis->m_TempPref.List.BackColor2;
						break;
					case IDC_PREF_COLOR_NEW_BACK:
						Color = pThis->m_TempPref.List.NewBackColor;
						break;
					case IDC_PREF_COLOR_SEL_TEXT:
						Color = pThis->m_TempPref.List.SelTextColor;
						break;
					case IDC_PREF_COLOR_SEL_BACK:
						Color = pThis->m_TempPref.List.SelBackColor;
						break;
					}
					DrawColorButton(pnmcd->hdc, pnmcd->rc, Color);

					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
					return TRUE;
				}
			}
			break;

		case PSN_APPLY:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				pThis->m_TempPref.List.ShowGrid =
					::IsDlgButtonChecked(hDlg, IDC_PREF_SHOWGRID) == BST_CHECKED;
				pThis->m_TempPref.Log.MaxLog =
					::GetDlgItemInt(hDlg, IDC_PREF_MAXLOG, nullptr, FALSE);
				::GetDlgItemText(hDlg, IDC_PREF_GEOIP_DATABASE,
								 pThis->m_TempPref.Core.GeoIPDatabaseFileName,
								 cvLengthOf(pThis->m_TempPref.Core.GeoIPDatabaseFileName));

				::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
			}
			return TRUE;

		case PSN_SETACTIVE:
			{
				LPPSHNOTIFY ppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);

				m_StartPage = PropSheet_HwndToIndex(ppsn->hdr.hwndFrom, hDlg);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PREF_FONT_CHOOSE:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseFontDialog(hDlg, &pThis->m_TempPref.List.Font))
					SetFontInfo(hDlg, IDC_PREF_FONT_INFO, pThis->m_TempPref.List.Font);
			}
			return TRUE;

		case IDC_PREF_COLOR_TEXT:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.TextColor))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_TEXT);
			}
			return TRUE;

		case IDC_PREF_COLOR_GRID:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.GridColor))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_GRID);
			}
			return TRUE;

		case IDC_PREF_COLOR_BACK1:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.BackColor1))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_BACK1);
			}
			return TRUE;

		case IDC_PREF_COLOR_BACK2:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.BackColor2))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_BACK2);
			}
			return TRUE;

		case IDC_PREF_COLOR_NEW_BACK:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.NewBackColor))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_NEW_BACK);
			}
			return TRUE;

		case IDC_PREF_COLOR_SEL_TEXT:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.SelTextColor))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_SEL_TEXT);
			}
			return TRUE;

		case IDC_PREF_COLOR_SEL_BACK:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.List.SelBackColor))
					RedrawDlgItem(hDlg, IDC_PREF_COLOR_SEL_BACK);
			}
			return TRUE;

		case IDC_PREF_GEOIP_DATABASE_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH], szFileName[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_PREF_GEOIP_DATABASE,
								 szDirectory, cvLengthOf(szDirectory));
				if (szDirectory[0] != '\0') {
					::lstrcpy(szFileName, ::PathFindFileName(szDirectory));
					::PathRemoveFileSpec(szDirectory);
				} else {
					szFileName[0] = '\0';
				}
				HINSTANCE hinst = (HINSTANCE)::GetWindowLongPtr(hDlg, GWLP_HINSTANCE);
				TCHAR Filter[256];
				::LoadString(hinst, IDS_GEOIP_DATABASE_FILTERS, Filter, cvLengthOf(Filter));
				ReplaceChar(Filter, _T('|'), _T('\0'));
				int FilterIndex = 0;
				if (FileOpenDialog(hDlg, Filter, &FilterIndex,
								   szFileName, cvLengthOf(szFileName),
								   szDirectory)) {
					::SetDlgItemText(hDlg, IDC_PREF_GEOIP_DATABASE, szFileName);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg, TEXT("This"));
		return TRUE;
	}

	return FALSE;
}

static void SetComboBoxList(HWND hDlg, int ID, LPCTSTR pList)
{
	for (LPCTSTR p = pList; *p != '\0'; p += ::lstrlen(p) + 1)
		::SendDlgItemMessage(hDlg, ID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p));
}

INT_PTR CALLBACK PreferencesDialog::GraphDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static const int BandwidthUnits[] = {1, 1024, 1024 * 1024};

	switch (Msg) {
	case WM_INITDIALOG:
		{
			PROPSHEETPAGE *pPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
			PreferencesDialog *pThis = reinterpret_cast<PreferencesDialog*>(pPage->lParam);

			::SetProp(hDlg, TEXT("This"), pThis);

			HINSTANCE hinst = (HINSTANCE)::GetWindowLongPtr(hDlg, GWLP_HINSTANCE);

			TCHAR GraphTypeList[256];
			::LoadString(hinst, IDS_GRAPH_TYPE_LIST,
						 GraphTypeList, cvLengthOf(GraphTypeList));
			ReplaceChar(GraphTypeList, _T('\n'), _T('\0'));

			static const LPCTSTR pBandwidthUnitList = TEXT("B/s\0KB/s\0MB/s\0");

			SetComboBoxList(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_GRAPH_TYPE, GraphTypeList);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_GRAPH_TYPE,
								 CB_SETCURSEL, (WPARAM)pThis->m_TempPref.Graph.InBandwidth.Type, 0);
			int Scale = pThis->m_TempPref.Graph.InBandwidth.Scale;
			int i;
			for (i = cvLengthOf(BandwidthUnits) - 1; i > 0; i--) {
				if (Scale % BandwidthUnits[i] == 0)
					break;
			}
			::SetDlgItemInt(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE, Scale / BandwidthUnits[i], TRUE);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE_SPIN,
								 UDM_SETRANGE32, 0, 0x7FFFFFFF);
			SetComboBoxList(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE_UNIT, pBandwidthUnitList);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE_UNIT,
								 CB_SETCURSEL, i, 0);

			SetComboBoxList(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_GRAPH_TYPE, GraphTypeList);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_GRAPH_TYPE,
								 CB_SETCURSEL, (WPARAM)pThis->m_TempPref.Graph.OutBandwidth.Type, 0);
			Scale = pThis->m_TempPref.Graph.OutBandwidth.Scale;
			for (i = cvLengthOf(BandwidthUnits) - 1; i > 0; i--) {
				if (Scale % BandwidthUnits[i] == 0)
					break;
			}
			::SetDlgItemInt(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE, Scale / BandwidthUnits[i], TRUE);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE_SPIN,
								 UDM_SETRANGE32, 0, 0x7FFFFFFF);
			SetComboBoxList(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE_UNIT, pBandwidthUnitList);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE_UNIT,
								 CB_SETCURSEL, i, 0);

			SetComboBoxList(hDlg, IDC_PREF_GRAPH_CONNECTIONS_GRAPH_TYPE, GraphTypeList);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_CONNECTIONS_GRAPH_TYPE,
								 CB_SETCURSEL, (WPARAM)pThis->m_TempPref.Graph.Connections.Type, 0);
			::SetDlgItemInt(hDlg, IDC_PREF_GRAPH_CONNECTIONS_SCALE,
							pThis->m_TempPref.Graph.Connections.Scale, TRUE);
			::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_CONNECTIONS_SCALE_SPIN,
								 UDM_SETRANGE32, 0, 0x7FFFFFFF);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case NM_CUSTOMDRAW:
			{
				LPNMCUSTOMDRAW pnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

				if ((pnmcd->hdr.idFrom == IDC_PREF_GRAPH_COLOR_BACK
						|| pnmcd->hdr.idFrom == IDC_PREF_GRAPH_COLOR_GRID
						|| pnmcd->hdr.idFrom == IDC_PREF_GRAPH_COLOR_CAPTION
						|| pnmcd->hdr.idFrom == IDC_PREF_GRAPH_IN_BANDWIDTH_COLOR
						|| pnmcd->hdr.idFrom == IDC_PREF_GRAPH_OUT_BANDWIDTH_COLOR
						|| pnmcd->hdr.idFrom == IDC_PREF_GRAPH_CONNECTIONS_COLOR)
						&& pnmcd->dwDrawStage == CDDS_PREPAINT) {
					PreferencesDialog *pThis = GetThis(hDlg);
					COLORREF Color;

					switch (pnmcd->hdr.idFrom) {
					case IDC_PREF_GRAPH_COLOR_BACK:
						Color = pThis->m_TempPref.Graph.BackColor;
						break;
					case IDC_PREF_GRAPH_COLOR_GRID:
						Color = pThis->m_TempPref.Graph.GridColor;
						break;
					case IDC_PREF_GRAPH_COLOR_CAPTION:
						Color = pThis->m_TempPref.Graph.CaptionColor;
						break;
					case IDC_PREF_GRAPH_IN_BANDWIDTH_COLOR:
						Color = pThis->m_TempPref.Graph.InBandwidth.Color;
						break;
					case IDC_PREF_GRAPH_OUT_BANDWIDTH_COLOR:
						Color = pThis->m_TempPref.Graph.OutBandwidth.Color;
						break;
					case IDC_PREF_GRAPH_CONNECTIONS_COLOR:
						Color = pThis->m_TempPref.Graph.Connections.Color;
						break;
					}
					DrawColorButton(pnmcd->hdc, pnmcd->rc, Color);

					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
					return TRUE;
				}
			}
			break;

		case PSN_APPLY:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				pThis->m_TempPref.Graph.InBandwidth.Type = (GraphPreferences::GraphInfo::GraphType)
						::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_GRAPH_TYPE,
											 CB_GETCURSEL, 0, 0);
				pThis->m_TempPref.Graph.InBandwidth.Scale =
					::GetDlgItemInt(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE, nullptr, TRUE);
				int Unit = (int)::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_SCALE_UNIT,
													 CB_GETCURSEL, 0, 0);
				if (Unit >= 0)
					pThis->m_TempPref.Graph.InBandwidth.Scale *= BandwidthUnits[Unit];

				pThis->m_TempPref.Graph.OutBandwidth.Type = (GraphPreferences::GraphInfo::GraphType)
						::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_GRAPH_TYPE,
											 CB_GETCURSEL, 0, 0);
				pThis->m_TempPref.Graph.OutBandwidth.Scale =
					::GetDlgItemInt(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE, nullptr, TRUE);
				Unit = (int)::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_SCALE_UNIT,
												 CB_GETCURSEL, 0, 0);
				if (Unit >= 0)
					pThis->m_TempPref.Graph.OutBandwidth.Scale *= BandwidthUnits[Unit];

				pThis->m_TempPref.Graph.Connections.Type = (GraphPreferences::GraphInfo::GraphType)
						::SendDlgItemMessage(hDlg, IDC_PREF_GRAPH_CONNECTIONS_GRAPH_TYPE,
											 CB_GETCURSEL, 0, 0);
				pThis->m_TempPref.Graph.Connections.Scale =
					::GetDlgItemInt(hDlg, IDC_PREF_GRAPH_CONNECTIONS_SCALE, nullptr, TRUE);

				::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
			}
			return TRUE;

		case PSN_SETACTIVE:
			{
				LPPSHNOTIFY ppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);

				m_StartPage = PropSheet_HwndToIndex(ppsn->hdr.hwndFrom, hDlg);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PREF_GRAPH_COLOR_BACK:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.BackColor))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_COLOR_BACK);
			}
			return TRUE;

		case IDC_PREF_GRAPH_COLOR_GRID:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.GridColor))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_COLOR_GRID);
			}
			return TRUE;

		case IDC_PREF_GRAPH_COLOR_CAPTION:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.CaptionColor))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_COLOR_CAPTION);
			}
			return TRUE;

		case IDC_PREF_GRAPH_IN_BANDWIDTH_COLOR:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.InBandwidth.Color))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_IN_BANDWIDTH_COLOR);
			}
			return TRUE;

		case IDC_PREF_GRAPH_OUT_BANDWIDTH_COLOR:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.OutBandwidth.Color))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_OUT_BANDWIDTH_COLOR);
			}
			return TRUE;

		case IDC_PREF_GRAPH_CONNECTIONS_COLOR:
			{
				PreferencesDialog *pThis = GetThis(hDlg);

				if (ChooseColorDialog(hDlg, &pThis->m_TempPref.Graph.Connections.Color))
					RedrawDlgItem(hDlg, IDC_PREF_GRAPH_CONNECTIONS_COLOR);
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg, TEXT("This"));
		return TRUE;
	}

	return FALSE;
}

PreferencesDialog *PreferencesDialog::GetThis(HWND hDlg)
{
	return static_cast<PreferencesDialog*>(::GetProp(hDlg, TEXT("This")));
}

}	// namespace CV
