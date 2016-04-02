/******************************************************************************
*                                                                             *
*    MainForm.cpp                           Copyright(c) 2010-2016 itow,y.    *
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
#include "MainForm.h"
#include "MiscDialog.h"
#include "ColumnSettingDialog.h"
#include "FilterSettingDialog.h"
#include "Utility.h"
#include "resource.h"


#define IDC_MAIN_CONNECTION_LIST	1000
#define IDC_MAIN_CONNECTION_LOG		1001
#define IDC_MAIN_GRAPH				1002
#define IDC_MAIN_INTERFACE_LIST		1003
#define IDC_MAIN_BLOCK_LIST			1004
#define IDC_MAIN_PROPERTY_LIST		1010
#define IDC_MAIN_TAB				1011
#define IDC_MAIN_TOOLBAR			1012
#define IDC_MAIN_STATUSBAR			1013

#define TIMER_ID_UPDATE	1

#define WM_APP_HOSTFOUND	WM_APP

#define MAX_GRAPH_HISTORY	10000

#define MENU_POS_VIEW	2
#define MENU_POS_VIEW_CONNECTION_COLUMNS	3
#define MENU_POS_VIEW_LOG_COLUMNS			4
#define MENU_POS_VIEW_INTERFACE_COLUMNS		5
#define MENU_POS_VIEW_BLOCK_COLUMNS			6


namespace CV
{

static bool TextCopyToClipboard(HWND hwnd, LPCTSTR pText)
{
	if (pText == nullptr || pText[0] == _T('\0'))
		return false;

	SIZE_T DataSize = (::lstrlen(pText) + 1) * sizeof(TCHAR);
	HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE, DataSize);
	if (hData == nullptr)
		return false;
	void *pData = ::GlobalLock(hData);
	::CopyMemory(pData, pText, DataSize);
	::GlobalUnlock(hData);

	if (!::OpenClipboard(hwnd)) {
		::GlobalFree(hData);
		return false;
	}
	::EmptyClipboard();
	::SetClipboardData(
#ifdef UNICODE
		CF_UNICODETEXT,
#else
		CF_TEXT,
#endif
		hData);
	::CloseClipboard();
	return true;
}


class FilterSettingEventHandler : public FilterSettingDialog::EventHandler
{
	ProgramCore &m_Core;
	MainForm &m_MainForm;

public:
	FilterSettingEventHandler(ProgramCore &Core, MainForm &MainForm)
		: m_Core(Core)
		, m_MainForm(MainForm)
	{
	}

	bool OnOK(FilterInfo &Filter)
	{
		FilterManager &Manager = m_Core.GetFilterManager();

		if (Filter.Match == FilterInfo::MATCH_EQUAL)
			m_Core.GetHostName(Filter.Address,
							   Filter.HostName, cvLengthOf(Filter.HostName));
		::GetSystemTimeAsFileTime(&Filter.AddedTime);
		if (!Manager.AddFilter(Filter)) {
			if (Manager.GetLastError() == ERROR_ALREADY_EXISTS) {
				m_MainForm.ShowMessage(IDS_ERROR_FILTER_ALREADY_EXISTS_TITLE,
									   IDS_ERROR_FILTER_ALREADY_EXISTS, 0,
									   TDCBF_OK_BUTTON, TD_INFORMATION_ICON);
			} else {
				SystemErrorDialog(m_MainForm.GetHandle(),
								  m_Core.GetLanguageInstance(),
								  Manager.GetLastError(),
								  IDS_ERROR_FILTER_ADD);
			}
			return false;
		}
		return true;
	}
};


enum
{
	GRAPH_IN_BANDWIDTH,
	GRAPH_OUT_BANDWIDTH,
	GRAPH_CONNECTIONS
};

static const LPCTSTR g_GraphNameList[] = {
	TEXT("InBandwidth"),
	TEXT("OutBandwidth"),
	TEXT("Connections"),
};

static const DWORD g_UpdateIntervalList[] = {
	500, 1000, 2000, 3000, 5000,
};


const LPCTSTR MainForm::m_pClassName = TEXT("ConnectionViewer MainForm");
ATOM MainForm::m_ClassAtom = 0;
HINSTANCE MainForm::m_Instance = nullptr;

bool MainForm::Initialize(HINSTANCE hinst)
{
	m_Instance = hinst;
	if (m_ClassAtom == 0) {
		m_ClassAtom = RegisterWindowClass(m_pClassName, 0,
										  //::LoadCursor(nullptr,IDC_ARROW),
										  nullptr,
										  reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1),
										  ::LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAIN)));
		if (m_ClassAtom == 0)
			return false;
	}
	if (!ListView::Initialize())
		return false;
	return true;
}

MainForm::MainForm(ProgramCore &Core)
	: m_Core(Core)
	, m_ListView(Core, Core.GetConnectionLog())
	, m_LogView(Core, Core.GetConnectionLog())
	, m_InterfaceListView(Core)
	, m_BlockListView(Core)
	, m_PropertyListView(Core)
	, m_ShowPropertyList(true)
	, m_ShowStatusBar(true)
	, m_SplitterWidth(4)
	, m_PropertyListHeight(120)
	, m_Accelerators(nullptr)
	, m_UpdateInterval(1000)
	, m_Paused(false)
	, m_Minimized(false)
	, m_ResolveAddresses(true)
	, m_FilterActive(true)
	, m_EnableNetworkIfStats(false)
	, m_CurTab(TAB_CONNECTION_LIST)
	, m_SaveFilterIndex(0)
	, m_WhoisDialog(m_WhoisServerList)
	, m_GraphInterfaceGUID(GUID_NULL)
{
	m_Position.Width = 800;
	m_Position.Height = 600;

	m_Style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

	const GraphPreferences &GraphPref = m_Core.GetPreferences().Graph;
	GraphView::GraphInfo *pGraph;
	pGraph = new GraphView::GraphInfo;
	pGraph->Type = (GraphView::GraphType)GraphPref.InBandwidth.Type;
	pGraph->Color = GraphPref.InBandwidth.Color;
	pGraph->Scale = GraphPref.InBandwidth.Scale;
	pGraph->Stride = 3;
	pGraph->Visible = true;
	pGraph->LineWidth = 2.0f;
	m_Core.LoadText(IDS_GRAPH_IN_BANDWIDTH, pGraph->szName, GraphView::GraphInfo::MAX_NAME);
	m_GraphView.AddGraph(pGraph);

	pGraph = new GraphView::GraphInfo;
	pGraph->Type = (GraphView::GraphType)GraphPref.OutBandwidth.Type;
	pGraph->Color = GraphPref.OutBandwidth.Color;
	pGraph->Scale = GraphPref.OutBandwidth.Scale;
	pGraph->Stride = 3;
	pGraph->Visible = true;
	pGraph->LineWidth = 2.0f;
	m_Core.LoadText(IDS_GRAPH_OUT_BANDWIDTH, pGraph->szName, GraphView::GraphInfo::MAX_NAME);
	m_GraphView.AddGraph(pGraph);

	pGraph = new GraphView::GraphInfo;
	pGraph->Type = (GraphView::GraphType)GraphPref.Connections.Type;
	pGraph->Color = GraphPref.Connections.Color;
	pGraph->Scale = GraphPref.Connections.Scale;
	pGraph->Stride = 3;
	pGraph->Visible = true;
	pGraph->LineWidth = 1.0f;
	m_Core.LoadText(IDS_GRAPH_CONNECTIONS, pGraph->szName, GraphView::GraphInfo::MAX_NAME);
	m_GraphView.AddGraph(pGraph);

	m_TabWidgetList[TAB_CONNECTION_LIST] = &m_ListView;
	m_TabWidgetList[TAB_CONNECTION_LOG] = &m_LogView;
	m_TabWidgetList[TAB_GRAPH] = &m_GraphView;
	m_TabWidgetList[TAB_INTERFACE_LIST] = &m_InterfaceListView;
	m_TabWidgetList[TAB_BLOCK_LIST] = &m_BlockListView;

	::GetCurrentDirectory(cvLengthOf(m_szListSaveDirectory), m_szListSaveDirectory);

	// TODO: List customize support
	static const char *WhoisServerList[] = {
		"whois.nic.ad.jp",
		"whois.apnic.net",
		"whois.arin.net",
		"whois.ripe.net",
		"whois.lacnic.net",
		"whois.afrinic.net",
		"whois.internic.net",
	};
	for (int i = 0; i < cvLengthOf(WhoisServerList); i++)
		m_WhoisServerList.Add(WhoisServerList[i]);
}

MainForm::~MainForm()
{
	Destroy();
}

bool MainForm::Create(HWND Parent, int ID)
{
	return CustomWidget::Create(MAKEINTATOM(m_ClassAtom), Parent,
								m_Core.LoadMenu(IDM_MAIN));
}

bool MainForm::Show(int CmdShow)
{
	if (m_Handle == nullptr)
		return false;
	return ::ShowWindow(m_Handle, CmdShow) != FALSE;
}

bool MainForm::LoadSettings(Settings *pSettings)
{
	pSettings->Read(TEXT("MainForm.Left"), &m_Position.Left);
	pSettings->Read(TEXT("MainForm.Top"), &m_Position.Top);
	pSettings->Read(TEXT("MainForm.Width"), &m_Position.Width);
	pSettings->Read(TEXT("MainForm.Height"), &m_Position.Height);
	RECT rc;
	::SetRect(&rc, m_Position.Left, m_Position.Top,
			  m_Position.Left + m_Position.Width,
			  m_Position.Top + m_Position.Height);
	HMONITOR hMonitor = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
	if (hMonitor == nullptr) {
		m_Position.Left = 0;
		m_Position.Top = 0;
	}

	int CurTab;
	if (pSettings->Read(TEXT("MainForm.CurTab"), &CurTab))
		SetCurTab(CurTab);

	pSettings->Read(TEXT("PropertyList.Visible"), &m_ShowPropertyList);
	pSettings->Read(TEXT("PropertyList.Height"), &m_PropertyListHeight);
	pSettings->Read(TEXT("StatusBar.Visible"), &m_ShowStatusBar);

	unsigned int Interval;
	if (pSettings->Read(TEXT("List.UpdateInterval"), &Interval))
		SetUpdateInterval(Interval);

	pSettings->Read(TEXT("List.ResolveAddresses"), &m_ResolveAddresses);

	bool HideUnconnected;
	if (pSettings->Read(TEXT("List.HideUnconnected"), &HideUnconnected))
		m_ListView.SetHideUnconnected(HideUnconnected);

	unsigned int ProtocolFilter;
	if (pSettings->Read(TEXT("List.HideProtocols"), &ProtocolFilter))
		m_ListView.SetProtocolFilter(ProtocolFilter);

	LoadListViewSettings(m_ListView, pSettings, TEXT("List"));
	LoadListViewSettings(m_LogView, pSettings, TEXT("Log"));
	LoadListViewSettings(m_InterfaceListView, pSettings, TEXT("InterfaceList"));
	LoadListViewSettings(m_BlockListView, pSettings, TEXT("BlockList"));
	LoadListViewSettings(m_PropertyListView, pSettings, TEXT("PropertyList"));

	for (int i = 0; i < cvLengthOf(g_GraphNameList); i++) {
		GraphView::GraphInfo *pGraph = m_GraphView.GetGraph(i);

		if (pGraph != nullptr) {
			TCHAR szName[64];

			FormatString(szName, cvLengthOf(szName), TEXT("Graph.%s.Visible"), g_GraphNameList[i]);
			pSettings->Read(szName, &pGraph->Visible);
		}
	}

	TCHAR szGuid[64];
	if (pSettings->Read(TEXT("Graph.InterfaceGUID"), szGuid, cvLengthOf(szGuid))) {
		CLSID Guid;
		if (::CLSIDFromString(szGuid, &Guid) == NOERROR)
			m_GraphInterfaceGUID = Guid;
	}

	pSettings->Read(TEXT("SaveList.Filter"), &m_SaveFilterIndex);

	TCHAR szServer[256];
	if (pSettings->Read(TEXT("Whois.DefaultServer"), szServer, cvLengthOf(szServer)))
		m_WhoisDialog.SetDefaultServer(szServer);
	Widget::Position Pos;
	m_WhoisDialog.GetPosition(&Pos);
	if (pSettings->Read(TEXT("Whois.Left"), &Pos.Left)
			| pSettings->Read(TEXT("Whois.Top"), &Pos.Top)) {
		if (Pos.Width == 0)
			Pos.Width = 100;
		m_WhoisDialog.SetPosition(Pos);
	}

	pSettings->Read(TEXT("Filter.Active"), &m_FilterActive);

	return true;
}

bool MainForm::SaveSettings(Settings *pSettings) const
{
	pSettings->Write(TEXT("MainForm.Left"), m_Position.Left);
	pSettings->Write(TEXT("MainForm.Top"), m_Position.Top);
	pSettings->Write(TEXT("MainForm.Width"), m_Position.Width);
	pSettings->Write(TEXT("MainForm.Height"), m_Position.Height);
	pSettings->Write(TEXT("MainForm.CurTab"), m_CurTab);

	pSettings->Write(TEXT("PropertyList.Visible"), m_ShowPropertyList);
	pSettings->Write(TEXT("PropertyList.Height"), m_PropertyListHeight);
	pSettings->Write(TEXT("StatusBar.Visible"), m_ShowStatusBar);

	pSettings->Write(TEXT("List.UpdateInterval"), (unsigned int)m_UpdateInterval);
	pSettings->Write(TEXT("List.ResolveAddresses"), m_ResolveAddresses);
	pSettings->Write(TEXT("List.HideUnconnected"), m_ListView.GetHideUnconnected());
	pSettings->Write(TEXT("List.HideProtocols"), m_ListView.GetProtocolFilter());

	SaveListViewSettings(m_ListView, pSettings, TEXT("List"));
	SaveListViewSettings(m_LogView, pSettings, TEXT("Log"));
	SaveListViewSettings(m_InterfaceListView, pSettings, TEXT("InterfaceList"));
	SaveListViewSettings(m_BlockListView, pSettings, TEXT("BlockList"));
	SaveListViewSettings(m_PropertyListView, pSettings, TEXT("PropertyList"));

	for (int i = 0; i < cvLengthOf(g_GraphNameList); i++) {
		const GraphView::GraphInfo *pGraph = m_GraphView.GetGraph(i);

		if (pGraph != nullptr) {
			TCHAR szName[64];

			FormatString(szName, cvLengthOf(szName), TEXT("Graph.%s.Visible"), g_GraphNameList[i]);
			pSettings->Write(szName, pGraph->Visible);
		}
	}

	TCHAR szGuid[64];
	if (m_GraphInterfaceGUID != GUID_NULL)
		::StringFromGUID2(m_GraphInterfaceGUID, szGuid, cvLengthOf(szGuid));
	else
		szGuid[0] = _T('\0');
	pSettings->Write(TEXT("Graph.InterfaceGUID"), szGuid);

	pSettings->Write(TEXT("SaveList.Filter"), m_SaveFilterIndex);

	pSettings->Write(TEXT("Whois.DefaultServer"), m_WhoisDialog.GetDefaultServer());
	Widget::Position Pos;
	m_WhoisDialog.GetPosition(&Pos);
	pSettings->Write(TEXT("Whois.Left"), Pos.Left);
	pSettings->Write(TEXT("Whois.Top"), Pos.Top);

	pSettings->Write(TEXT("Filter.Active"), m_FilterActive);

	return true;
}

void MainForm::LoadListViewSettings(ListView &List, Settings *pSettings, LPCTSTR pName)
{
	TCHAR szText[64];
	bool Ascending;
	std::vector<int> ColumnOrder, SortOrder;

	Ascending = List.IsSortAscending();
	FormatString(szText, cvLengthOf(szText), TEXT("%s.SortAscending"), pName);
	pSettings->Read(szText, &Ascending);

	List.GetColumnOrder(&ColumnOrder);
	List.GetSortOrder(&SortOrder);

	for (int i = 0; i < List.NumColumns(); i++) {
		ListView::ColumnInfo Column;
		List.GetColumnInfo(i, &Column);
		LPCTSTR pColumnName = List.GetColumnIDName(Column.ID);

		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Visible"), pName, pColumnName);
		bool Visible;
		if (pSettings->Read(szText, &Visible))
			List.SetColumnVisible(Column.ID, Visible);

		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Width"), pName, pColumnName);
		int Width;
		if (pSettings->Read(szText, &Width))
			List.SetColumnWidth(Column.ID, Width);

		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Index"), pName, pColumnName);
		int Index;
		if (pSettings->Read(szText, &Index) && Index >= 0) {
			std::vector<int>::iterator j;

			for (j = ColumnOrder.begin(); j != ColumnOrder.end(); j++) {
				if (*j == Column.ID) {
					ColumnOrder.erase(j);
					break;
				}
			}
			if (Index < (int)ColumnOrder.size()) {
				j = ColumnOrder.begin();
				std::advance(j, Index);
				ColumnOrder.insert(j, Column.ID);
			} else {
				ColumnOrder.push_back(Column.ID);
			}
		}

		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.SortOrder"), pName, pColumnName);
		int Order;
		if (pSettings->Read(szText, &Order) && Order >= 0) {
			std::vector<int>::iterator j;

			for (j = SortOrder.begin(); j != SortOrder.end(); j++) {
				if (*j == i) {
					SortOrder.erase(j);
					break;
				}
			}
			if (Order < (int)SortOrder.size()) {
				j = SortOrder.begin();
				std::advance(j, Order);
				SortOrder.insert(j, i);
			} else {
				SortOrder.push_back(i);
			}
		}
	}

	List.SetSortOrder(SortOrder, Ascending);
	List.SetColumnOrder(ColumnOrder);
}

void MainForm::SaveListViewSettings(const ListView &List, Settings *pSettings, LPCTSTR pName) const
{
	TCHAR szText[64];
	std::vector<int> SortOrder;

	List.GetSortOrder(&SortOrder);
	for (int i = 0; i < List.NumColumns(); i++) {
		ListView::ColumnInfo Column;
		List.GetColumnInfo(i, &Column);

		LPCTSTR pColumnName = List.GetColumnIDName(Column.ID);

		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Visible"), pName, pColumnName);
		pSettings->Write(szText, Column.Visible);
		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Width"), pName, pColumnName);
		pSettings->Write(szText, Column.Width);
		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.Index"), pName, pColumnName);
		pSettings->Write(szText, i);
		int j;
		for (j = (int)SortOrder.size() - 1; j >= 0; j--) {
			if (SortOrder[j] == i)
				break;
		}
		FormatString(szText, cvLengthOf(szText), TEXT("%s.Column.%s.SortOrder"), pName, pColumnName);
		pSettings->Write(szText, j);
	}

	FormatString(szText, cvLengthOf(szText), TEXT("%s.SortAscending"), pName);
	pSettings->Write(szText, List.IsSortAscending());
}

bool MainForm::SetUpdateInterval(DWORD Interval)
{
	if (Interval < USER_TIMER_MINIMUM || Interval > USER_TIMER_MAXIMUM)
		return false;
	if (Interval != m_UpdateInterval) {
		if (m_Handle != nullptr && !m_Paused)
			::SetTimer(m_Handle, TIMER_ID_UPDATE, Interval, nullptr);
		m_UpdateInterval = Interval;
	}
	return true;
}

void MainForm::PauseUpdate(bool Pause)
{
	if (m_Paused != Pause) {
		m_Paused = Pause;
		if (m_Handle != nullptr) {
			if (Pause) {
				::KillTimer(m_Handle, TIMER_ID_UPDATE);
			} else {
				m_EnableNetworkIfStats = false;
				::SetTimer(m_Handle, TIMER_ID_UPDATE, m_UpdateInterval, nullptr);
			}
			::CheckMenuItem(::GetMenu(m_Handle), CM_PAUSE,
							(m_Paused ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			m_ToolBar.CheckItem(CM_PAUSE, m_Paused);
		}
	}
}

void MainForm::SetResolveAddresses(bool Resolve)
{
	if (m_ResolveAddresses != Resolve) {
		m_ResolveAddresses = Resolve;
		if (Resolve)
			ResolveAddresses();
		if (m_Handle != nullptr)
			::CheckMenuItem(::GetMenu(m_Handle), CM_RESOLVE_ADDRESSES,
							(m_ResolveAddresses ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	}
}

bool MainForm::SetCurTab(int Tab)
{
	if (Tab < 0 || Tab >= NUM_TAB_ITEMS)
		return false;
	if (m_CurTab != Tab) {
		if (m_Handle != nullptr) {
			HCURSOR OldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			m_Tab.SetSel(Tab);
			m_TabWidgetList[m_CurTab]->SetVisible(false);
			m_CurTab = Tab;
			RECT rc;
			::GetClientRect(m_Handle, &rc);
			::SendMessage(m_Handle, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));
			m_TabWidgetList[Tab]->SetVisible(true);
			if (m_ShowPropertyList) {
				SetPropertyListNames();
				SetPropertyListValues();
				m_PropertyListView.SetVisible(m_CurTab != TAB_GRAPH);
			}
			SetCurTabStatusText();
			::SetCursor(OldCursor);
		} else {
			m_CurTab = Tab;
		}
	}
	return true;
}

bool MainForm::TranslateAccelerator(MSG *pMessage) const
{
	if (m_Accelerators == nullptr)
		return false;
	return ::TranslateAccelerator(m_Handle, m_Accelerators, pMessage) != FALSE;
}

void MainForm::ShowPropertyList(bool Show)
{
	if (m_ShowPropertyList != Show) {
		m_ShowPropertyList = Show;
		if (m_Handle != nullptr) {
			RECT rc;

			::GetClientRect(m_Handle, &rc);
			if (Show) {
				if (m_CurTab != TAB_GRAPH) {
					SetPropertyListNames();
					SetPropertyListValues();
					::SendMessage(m_Handle, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));
					m_PropertyListView.SetVisible(true);
				}
			} else {
				m_PropertyListView.SetVisible(false);
				::SendMessage(m_Handle, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));
			}
			::CheckMenuItem(::GetMenu(m_Handle), CM_PROPERTY_LIST,
							(m_ShowPropertyList ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		}
	}
}

bool MainForm::SetGraphInterface(const GUID &Guid)
{
	if (m_GraphInterfaceGUID != Guid) {
		m_GraphInterfaceGUID = Guid;

		GraphView::GraphInfo *pGraph;
		pGraph = m_GraphView.GetGraph(GRAPH_IN_BANDWIDTH);
		if (pGraph != nullptr)
			pGraph->List.clear();
		pGraph = m_GraphView.GetGraph(GRAPH_OUT_BANDWIDTH);
		if (pGraph != nullptr)
			pGraph->List.clear();

		m_EnableNetworkIfStats = false;

		if (m_Handle != nullptr) {
			SetGraphCaption();
			m_GraphView.Redraw();
		}
	}
	return true;
}

bool MainForm::SetFilterActive(bool Active)
{
	FilterManager &Manager = m_Core.GetFilterManager();

	if (Active != Manager.IsActive()) {
		if (!Manager.SetActive(Active))
			return false;
		m_FilterActive = Active;
		if (m_Handle != nullptr) {
			::CheckMenuItem(::GetMenu(m_Handle), CM_FILTER_ACTIVE_TOGGLE,
							(Active ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			m_ToolBar.CheckItem(CM_FILTER_ACTIVE_TOGGLE, Active);
			m_Tab.SetImage(TAB_BLOCK_LIST, Active ? TAB_BLOCK_LIST + 1 : TAB_BLOCK_LIST);
		}
	}
	return true;
}

LRESULT MainForm::OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_CREATE:
		{
			TCHAR szCaption[64], szUser[64];
			m_Core.LoadText(IsAdministratorsMember() ?
							IDS_CAPTION_ADMINISTRATOR : IDS_CAPTION_NOT_ADMINISTRATOR,
							szUser, cvLengthOf(szUser));
			FormatString(szCaption, cvLengthOf(szCaption),
						 TEXT("Connection Viewer (%s)"),
						 szUser);
			SetText(szCaption);

			if (m_Accelerators == nullptr)
				m_Accelerators = ::LoadAccelerators(m_Core.GetLanguageInstance(),
													MAKEINTRESOURCE(IDA_ACCEL));

			m_ListView.Create(hwnd, IDC_MAIN_CONNECTION_LIST);
			m_ListView.SetEventHandler(this);

			m_LogView.Create(hwnd, IDC_MAIN_CONNECTION_LOG);
			m_LogView.SetEventHandler(this);

			m_GraphView.Create(hwnd, IDC_MAIN_GRAPH);
			m_GraphView.SetEventHandler(this);

			m_InterfaceListView.Create(hwnd, IDC_MAIN_INTERFACE_LIST);
			m_InterfaceListView.SetEventHandler(this);

			m_BlockListView.Create(hwnd, IDC_MAIN_BLOCK_LIST);
			m_BlockListView.SetEventHandler(this);
			m_BlockListView.OnListUpdated();

			m_TabWidgetList[m_CurTab]->SetVisible(true);

			m_PropertyListView.Create(hwnd, IDC_MAIN_PROPERTY_LIST);
			m_PropertyListView.SetPosition(0, 0, 0, m_PropertyListHeight);
			m_PropertyListView.SetEventHandler(this);
			if (m_ShowPropertyList) {
				SetPropertyListNames();
				m_PropertyListView.SetVisible(true);
			}

			m_Tab.Create(hwnd, IDC_MAIN_TAB);
			m_Tab.SetImageList(
				::ImageList_LoadImage(m_Instance, MAKEINTRESOURCE(IDB_TAB), 16, 1,
									  CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION));
			for (int i = 0; i < NUM_TAB_ITEMS; i++) {
				TCHAR szText[64];
				m_Core.LoadText(IDS_TAB_FIRST + i, szText, cvLengthOf(szText));
				m_Tab.AddItem(szText, i);
			}
			m_Tab.SetFixedWidth(true);
			m_Tab.SetSel(m_CurTab);
			m_Tab.SetVisible(true);

			m_ToolBar.Create(hwnd, IDC_MAIN_TOOLBAR);
			m_ToolBar.SetImageList(
				::ImageList_LoadImage(m_Instance, MAKEINTRESOURCE(IDB_TOOLBAR), 16, 1,
									  CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION));
			static const int ToolBarItemList[] = {
				CM_PAUSE,
				CM_FILTER_ACTIVE_TOGGLE,
			};
			for (int i = 0; i < cvLengthOf(ToolBarItemList); i++) {
				TCHAR szText[64];
				m_Core.LoadText(IDS_COMMAND_NAME_FIRST + ToolBarItemList[i],
								szText, cvLengthOf(szText));
				m_ToolBar.AddItem(ToolBarItemList[i], i, szText);
			}
			m_ToolBar.SetVisible(true);

			m_StatusBar.Create(hwnd, IDC_MAIN_STATUSBAR);
			{
				static const int Margin = 16;
				int Parts[5];

				HFONT hfont = m_StatusBar.GetFont();
				if (hfont == nullptr)
					hfont = static_cast<HFONT>(::GetStockObject(SYSTEM_FONT));
				HDC hdc = ::GetDC(m_StatusBar.GetHandle());
				HGDIOBJ OldFont = ::SelectObject(hdc, hfont);
				TCHAR szText[128], szFormat[64], szValue[64];
				SIZE sz;

				m_Core.LoadText(IDS_STATUS_CONNECTIONS, szFormat, cvLengthOf(szFormat));
				FormatString(szText, cvLengthOf(szText), szFormat, 999, 999, 999);
				::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &sz);
				Parts[0] = sz.cx + Margin;

				m_Core.LoadText(IDS_STATUS_IN_BANDWIDTH, szFormat, cvLengthOf(szFormat));
				FormatBandwidth(/*999999999*/1000 * 1024 * 1024 - 1, szValue, cvLengthOf(szValue));
				FormatString(szText, cvLengthOf(szText), szFormat, szValue);
				::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &sz);
				Parts[1] = sz.cx + Margin;

				m_Core.LoadText(IDS_STATUS_OUT_BANDWIDTH, szFormat, cvLengthOf(szFormat));
				FormatString(szText, cvLengthOf(szText), szFormat, szValue);
				::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &sz);
				Parts[2] = sz.cx + Margin;

				m_Core.LoadText(IDS_STATUS_IN_BYTES, szFormat, cvLengthOf(szFormat));
				FormatUInt64(999999999999ULL, szValue, cvLengthOf(szValue));
				FormatString(szText, cvLengthOf(szText), szFormat, szValue);
				::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &sz);
				Parts[3] = sz.cx + Margin;

				m_Core.LoadText(IDS_STATUS_OUT_BYTES, szFormat, cvLengthOf(szFormat));
				FormatString(szText, cvLengthOf(szText), szFormat, szValue);
				::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &sz);
				Parts[4] = sz.cx + Margin;

				::SelectObject(hdc, OldFont);
				::ReleaseDC(m_StatusBar.GetHandle(), hdc);

				m_StatusBar.SetParts(Parts, cvLengthOf(Parts));
			}
			if (m_ShowStatusBar)
				m_StatusBar.SetVisible(true);

			m_Minimized = ::IsIconic(hwnd) != FALSE;

			HMENU hmenu = ::GetMenu(hwnd);
			::CheckMenuItem(hmenu, CM_PAUSE,
							(m_Paused ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			m_ToolBar.CheckItem(CM_PAUSE, m_Paused);
			for (int i = 0; i < cvLengthOf(g_UpdateIntervalList); i++) {
				if (g_UpdateIntervalList[i] == m_UpdateInterval) {
					::CheckMenuRadioItem(hmenu,
										 CM_UPDATEINTERVAL_FIRST, CM_UPDATEINTERVAL_LAST,
										 CM_UPDATEINTERVAL_FIRST + i, MF_BYCOMMAND);
					break;
				}
			}
			/*
			for (int i=0;i<ConnectionListView::NUM_COLUMN_TYPES;i++) {
				::CheckMenuItem(hmenu,CM_LISTCOLUMN_FIRST+i,
								(m_ListView.IsColumnVisible(i)?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND);
			}
			for (int i=0;i<ConnectionLogView::NUM_COLUMN_TYPES;i++) {
				::CheckMenuItem(hmenu,CM_LOGCOLUMN_FIRST+i,
								(m_LogView.IsColumnVisible(i)?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND);
			}
			for (int i=0;i<InterfaceListView::NUM_COLUMN_TYPES;i++) {
				::CheckMenuItem(hmenu,CM_INTERFACECOLUMN_FIRST+i,
								(m_InterfaceListView.IsColumnVisible(i)?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND);
			}
			*/
			::CheckMenuItem(hmenu, CM_RESOLVE_ADDRESSES,
							(m_ResolveAddresses ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			const unsigned int Filter = m_ListView.GetProtocolFilter();
			for (int i = CM_CONNECTIONLIST_PROTOCOL_FIRST; i <= CM_CONNECTIONLIST_PROTOCOL_LAST; i++) {
				::CheckMenuItem(hmenu, i,
								((Filter & (1 << (i - CM_CONNECTIONLIST_PROTOCOL_FIRST))) == 0 ?
								 MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			}
			::CheckMenuItem(hmenu, CM_HIDE_UNCONNECTED,
							(m_ListView.GetHideUnconnected() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			::CheckMenuItem(hmenu, CM_PROPERTY_LIST,
							(m_ShowPropertyList ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			::CheckMenuItem(hmenu, CM_STATUSBAR,
							(m_ShowStatusBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

			if (m_FilterActive)
				SetFilterActive(true);

			//if (!m_Minimized)
			UpdateStatus();

			SetGraphCaption();

			if (!m_Paused)
				::SetTimer(hwnd, TIMER_ID_UPDATE, m_UpdateInterval, nullptr);
		}
		return 0;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			m_Minimized = true;
		} else {
			int Width = LOWORD(lParam), Height = HIWORD(lParam);

			if (m_Minimized) {
				m_Minimized = false;
				//m_EnableNetworkIfStats = false;
				//UpdateStatus();
			}
			const int TabHeight = m_Tab.GetTabHeight();
			SIZE ToolBarSize;
			m_ToolBar.GetMinSize(&ToolBarSize);
			int Top = max(TabHeight, ToolBarSize.cy);
			m_Tab.SetPosition(0, Top - TabHeight, Width - ToolBarSize.cx, TabHeight);
			m_ToolBar.SetPosition(Width - ToolBarSize.cx, 0, ToolBarSize.cx, ToolBarSize.cy);
			Height -= Top;
			if (m_ShowStatusBar) {
				int StatusHeight = m_StatusBar.GetHeight();
				Height -= StatusHeight;
				m_StatusBar.SetPosition(0, Top + Height, Width, StatusHeight);
			}
			if (m_ShowPropertyList && m_CurTab != TAB_GRAPH) {
				int PropertyListHeight = m_PropertyListHeight;
				if (PropertyListHeight + m_SplitterWidth > Height)
					PropertyListHeight = Height - m_SplitterWidth;
				Height -= PropertyListHeight;
				m_PropertyListView.SetPosition(0, Top + Height, Width, max(PropertyListHeight, 0));
				Height -= m_SplitterWidth;
			}
			m_TabWidgetList[m_CurTab]->SetPosition(0, Top, Width, max(Height, 0));
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			LPCTSTR pCursor = IDC_ARROW;

			if (::GetCapture() == hwnd) {
				Widget::Position PropertyPos, PanePos;

				m_PropertyListView.GetPosition(&PropertyPos);
				m_TabWidgetList[m_CurTab]->GetPosition(&PanePos);
				int Top = y + m_SplitterWidth / 2;
				if (Top > PropertyPos.Top + PropertyPos.Height) {
					Top = PropertyPos.Top + PropertyPos.Height;
				} else {
					if (Top < PanePos.Top + m_SplitterWidth)
						Top = PanePos.Top + m_SplitterWidth;
				}
				if (Top != PropertyPos.Top) {
					PropertyPos.Height -= Top - PropertyPos.Top;
					PanePos.Height += Top - PropertyPos.Top;
					PropertyPos.Top = Top;
					m_PropertyListHeight = PropertyPos.Height;
					m_PropertyListView.SetPosition(PropertyPos);
					m_TabWidgetList[m_CurTab]->SetPosition(PanePos);
				}
				pCursor = IDC_SIZENS;
			} else {
				if (m_ShowPropertyList) {
					Widget::Position Pos;

					m_PropertyListView.GetPosition(&Pos);
					if (y < Pos.Top && y >= Pos.Top - m_SplitterWidth)
						pCursor = IDC_SIZENS;
				}
			}
			::SetCursor(::LoadCursor(nullptr, pCursor));
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int /*x = GET_X_LPARAM(lParam),*/y = GET_Y_LPARAM(lParam);

			if (m_ShowPropertyList && m_PropertyListView.IsVisible()) {
				Widget::Position Pos;

				m_PropertyListView.GetPosition(&Pos);
				if (y < Pos.Top && y >= Pos.Top - m_SplitterWidth) {
					::SetCapture(hwnd);
					::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd)
			::ReleaseCapture();
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd && LOWORD(lParam) == HTCLIENT) {
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			const ListView *pListView =
				dynamic_cast<const ListView*>(m_TabWidgetList[m_CurTab]);
			if (pListView != nullptr)
				return ::SendMessage(pListView->GetHandle(), Msg, wParam, lParam);
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_ID_UPDATE) {
			//if (!m_Minimized)
			UpdateStatus();
		}
		return 0;

	case WM_NOTIFY:
		{
			const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);

			switch (pnmh->code) {
			case TCN_SELCHANGE:
				SetCurTab(m_Tab.GetSel());
				return 0;
			}
		}
		break;

	case WM_INITMENUPOPUP:
		{
			HMENU hmenu = reinterpret_cast<HMENU>(wParam);
			HMENU hmenuView = ::GetSubMenu(::GetMenu(hwnd), MENU_POS_VIEW);
			ListView *pListView;
			int Command;

			if (hmenu == ::GetSubMenu(hmenuView, MENU_POS_VIEW_CONNECTION_COLUMNS)) {
				pListView = &m_ListView;
				Command = CM_LISTCOLUMN_FIRST;
			} else if (hmenu == ::GetSubMenu(hmenuView, MENU_POS_VIEW_LOG_COLUMNS)) {
				pListView = &m_LogView;
				Command = CM_LOGCOLUMN_FIRST;
			} else if (hmenu == ::GetSubMenu(hmenuView, MENU_POS_VIEW_INTERFACE_COLUMNS)) {
				pListView = &m_InterfaceListView;
				Command = CM_INTERFACECOLUMN_FIRST;
			} else if (hmenu == ::GetSubMenu(hmenuView, MENU_POS_VIEW_BLOCK_COLUMNS)) {
				pListView = &m_BlockListView;
				Command = CM_BLOCKLIST_COLUMN_FIRST;
			} else {
				break;
			}
			for (int i = ::GetMenuItemCount(hmenu) - 1; i > 1; i--) {
				::DeleteMenu(hmenu, i, MF_BYPOSITION);
			}
			const int NumColumns = pListView->NumColumns();
			for (int i = 0; i < NumColumns; i++) {
				ListView::ColumnInfo Column;

				pListView->GetColumnInfo(i, &Column);
				::AppendMenu(hmenu,
							 MF_STRING | MF_ENABLED | (Column.Visible ? MF_CHECKED : MF_UNCHECKED),
							 Command + Column.ID,
							 Column.szText);
			}
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam), HIWORD(wParam));
		return 0;

	case WM_APP_HOSTFOUND:
		{
			BlockLock Lock(m_FoundHostLock);

			while (!m_FoundHostList.empty()) {
				if (m_Core.OnHostNameFound(m_FoundHostList.front()))
					m_ListView.OnHostNameFound(m_FoundHostList.front());
				m_FoundHostList.pop_front();
			}
		}
		return 0;

	case WM_DESTROY:
		m_Core.EndHostManager();

		m_Tab.Destroy();
		m_StatusBar.Destroy();

		::PostQuitMessage(0);
		return 0;
	}

	return ::DefWindowProc(hwnd, Msg, wParam, lParam);
}

void MainForm::OnCommand(int Command, int NotifyCode)
{
	switch (Command) {
	case CM_SAVE_LIST:
		{
			TCHAR Filter[256], szFileName[MAX_PATH];

			if (!m_Paused)
				::KillTimer(m_Handle, TIMER_ID_UPDATE);
			m_Core.LoadText(IDS_SAVELIST_FILTERS, Filter, cvLengthOf(Filter));
			ReplaceChar(Filter, _T('|'), _T('\0'));
			szFileName[0] = _T('\0');
			if (FileSaveDialog(m_Handle, Filter, &m_SaveFilterIndex,
							   szFileName, cvLengthOf(szFileName),
							   m_szListSaveDirectory)) {
				HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

				if (*::PathFindExtension(szFileName) == _T('\0')
						&& ::lstrlen(szFileName) + 4 < cvLengthOf(szFileName)) {
					if (m_SaveFilterIndex == 0)
						::lstrcat(szFileName, TEXT(".csv"));
					else if (m_SaveFilterIndex == 1)
						::lstrcat(szFileName, TEXT(".tsv"));
				}

				const ListView *pListView =
					dynamic_cast<const ListView*>(m_TabWidgetList[m_CurTab]);
				if (pListView == nullptr)
					pListView = &m_ListView;

				DWORD Result = SaveListFile(
					szFileName,
					m_SaveFilterIndex == 0 ? LIST_TYPE_CSV : LIST_TYPE_TSV,
					pListView);
				::SetCursor(hcurOld);
				::lstrcpy(m_szListSaveDirectory, szFileName);
				::PathRemoveFileSpec(m_szListSaveDirectory);
				if (Result != ERROR_SUCCESS)
					SystemErrorDialog(m_Handle, m_Core.GetLanguageInstance(),
									  Result, IDS_ERROR_FILE_SAVE);
			}
			if (!m_Paused) {
				ULONGLONG Time = ::GetTickCount64() - m_Core.GetUpdatedTickCount();
				if (Time >= m_UpdateInterval)
					UpdateStatus();
				::SetTimer(m_Handle, TIMER_ID_UPDATE, m_UpdateInterval, nullptr);
			}
		}
		return;

	case CM_EXIT:
		::SendMessage(m_Handle, WM_CLOSE, 0, 0);
		return;

	case CM_COPY_LIST:
		{
			const ListView *pListView =
				dynamic_cast<const ListView*>(m_TabWidgetList[m_CurTab]);
			if (pListView == nullptr)
				pListView = &m_ListView;
			LPTSTR pList = GetListText(pListView, LIST_TYPE_TSV);
			if (pList == nullptr)
				return;
			SIZE_T DataSize = (::lstrlen(pList) + 1) * sizeof(TCHAR);
			HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE, DataSize);
			if (hData == nullptr) {
				delete [] pList;
				SystemErrorDialog(m_Handle,
								  m_Core.GetLanguageInstance(),
								  ::GetLastError(),
								  IDS_ERROR_LIST_COPY);
				return;
			}
			void *pData = ::GlobalLock(hData);
			::CopyMemory(pData, pList, DataSize);
			::GlobalUnlock(hData);
			delete [] pList;

			if (::OpenClipboard(m_Handle)) {
				::EmptyClipboard();
				::SetClipboardData(
#ifdef UNICODE
					CF_UNICODETEXT,
#else
					CF_TEXT,
#endif
					hData);
				::CloseClipboard();
			} else {
				::GlobalFree(hData);
			}
		}
		return;

	case CM_LOG_CLEAR:
		m_Core.ClearConnectionLog();
		m_LogView.OnListUpdated();
		return;

	case CM_PAUSE:
		PauseUpdate(!m_Paused);
		return;

	case CM_UPDATEINTERVAL_500:
	case CM_UPDATEINTERVAL_1000:
	case CM_UPDATEINTERVAL_2000:
	case CM_UPDATEINTERVAL_3000:
	case CM_UPDATEINTERVAL_5000:
		SetUpdateInterval(g_UpdateIntervalList[Command - CM_UPDATEINTERVAL_FIRST]);
		::CheckMenuRadioItem(::GetMenu(m_Handle),
							 CM_UPDATEINTERVAL_FIRST, CM_UPDATEINTERVAL_LAST,
							 Command, MF_BYCOMMAND);
		return;

	case CM_CONNECTION_LIST_COLUMN_SETTINGS:
		{
			ColumnSettingDialog Dialog;

			Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &m_ListView);
		}
		return;

	case CM_CONNECTION_LOG_COLUMN_SETTINGS:
		{
			ColumnSettingDialog Dialog;

			Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &m_LogView);
		}
		return;

	case CM_INTERFACE_LIST_COLUMN_SETTINGS:
		{
			ColumnSettingDialog Dialog;

			Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &m_InterfaceListView);
		}
		return;

	case CM_BLOCK_LIST_COLUMN_SETTINGS:
		{
			ColumnSettingDialog Dialog;

			Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &m_BlockListView);
		}
		return;

	case CM_RESOLVE_ADDRESSES:
		SetResolveAddresses(!m_ResolveAddresses);
		return;

	case CM_CONNECTIONLIST_PROTOCOL_TCP_V4:
	case CM_CONNECTIONLIST_PROTOCOL_TCP_V6:
	case CM_CONNECTIONLIST_PROTOCOL_UDP_V4:
	case CM_CONNECTIONLIST_PROTOCOL_UDP_V6:
		{
			unsigned int Filter = m_ListView.GetProtocolFilter();
			unsigned int Mask = 1U << (Command - CM_CONNECTIONLIST_PROTOCOL_FIRST);

			Filter ^= Mask;
			m_ListView.SetProtocolFilter(Filter);
			::CheckMenuItem(::GetMenu(m_Handle), Command,
							((Filter & Mask) == 0 ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		}
		return;

	case CM_HIDE_UNCONNECTED:
		m_ListView.SetHideUnconnected(!m_ListView.GetHideUnconnected());
		::CheckMenuItem(::GetMenu(m_Handle), CM_HIDE_UNCONNECTED,
						(m_ListView.GetHideUnconnected() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		return;

	case CM_PROPERTY_LIST:
		ShowPropertyList(!m_ShowPropertyList);
		return;

	case CM_STATUSBAR:
		{
			m_ShowStatusBar = !m_ShowStatusBar;
			RECT rc;
			::GetClientRect(m_Handle, &rc);
			::SendMessage(m_Handle, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));
			m_StatusBar.SetVisible(m_ShowStatusBar);
			::CheckMenuItem(::GetMenu(m_Handle), CM_STATUSBAR,
							(m_ShowStatusBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		}
		return;

	case CM_PREFERENCES:
		{
			PreferencesDialog PrefDialog(m_Core.GetPreferences(), this);

			PrefDialog.Show(m_Core.GetLanguageInstance(), m_Handle);
		}
		return;

	case CM_ABOUT:
		{
			AboutDialog Dialog;

			Dialog.Show(m_Core.GetLanguageInstance(), m_Handle);
		}
		return;

	case CM_FILTER_ACTIVE:
		if (!SetFilterActive(true)) {
			FilterManager &Manager = m_Core.GetFilterManager();

			if (Manager.GetLastError() == ERROR_ACCESS_DENIED) {
				ShowMessage(IDS_ERROR_CAPTION,
							IDS_ERROR_FILTER_ACCESS_DENIED_HEADER,
							IDS_ERROR_FILTER_ACCESS_DENIED,
							TDCBF_OK_BUTTON, TD_WARNING_ICON);
			} else {
				SystemErrorDialog(m_Handle, m_Core.GetLanguageInstance(),
								  Manager.GetLastError(), IDS_ERROR_FILTER_BEGIN);
			}
		}
		return;

	case CM_FILTER_INACTIVE:
		SetFilterActive(false);
		return;

	case CM_FILTER_ACTIVE_TOGGLE:
		OnCommand(m_Core.GetFilterManager().IsActive() ? CM_FILTER_INACTIVE : CM_FILTER_ACTIVE);
		return;

	case CM_FILTER_ADD:
		{
			FilterInfo Filter;
			FilterSettingDialog Dialog;
			FilterSettingEventHandler EventHandler(m_Core, *this);

			Filter.Type = FilterInfo::TYPE_BLOCK_ADDRESS;
			Filter.Match = FilterInfo::MATCH_EQUAL;
			Filter.Address.SetV4Address(CV_IP_ADDRESS_V4(0, 0, 0, 0));
			Filter.HostName[0] = _T('\0');
			Filter.Enable = true;
			Filter.Comment[0] = _T('\0');
			if (Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &Filter, &EventHandler)) {
				m_BlockListView.OnListUpdated();
				m_BlockListView.SelectItem(m_BlockListView.FilterIDToIndex(Filter.ID));
				if (m_CurTab == TAB_BLOCK_LIST)
					SetPropertyListValues();
			}
		}
		return;

	case CM_CONNECTION_ITEM_COPY_REMOTE_ADDRESS:
	case CM_CONNECTION_ITEM_COPY_REMOTE_HOST:
	case CM_CONNECTION_ITEM_COPY_PROCESS_PATH:
		{
			const int Sel = m_ListView.GetSelectedItem();

			if (Sel >= 0) {
				int Column;
				TCHAR szText[256];

				switch (Command) {
				case CM_CONNECTION_ITEM_COPY_REMOTE_ADDRESS:
					Column = ConnectionListView::COLUMN_REMOTE_ADDRESS;
					break;
				case CM_CONNECTION_ITEM_COPY_REMOTE_HOST:
					Column = ConnectionListView::COLUMN_REMOTE_HOST;
					break;
				case CM_CONNECTION_ITEM_COPY_PROCESS_PATH:
					Column = ConnectionListView::COLUMN_PROCESS_PATH;
					break;
				}
				if (m_ListView.GetItemText(Sel, Column, szText, cvLengthOf(szText))
						&& szText[0] != _T('\0')) {
					TextCopyToClipboard(m_Handle, szText);
				}
			}
		}
		return;

	case CM_CONNECTION_ITEM_WHOIS:
		{
			const int Sel = m_ListView.GetSelectedItem();

			if (Sel >= 0) {
				ConnectionInfo Info;

				if (m_ListView.GetItemConnectionInfo(Sel, &Info)) {
					m_WhoisDialog.Show(m_Core.GetLanguageInstance(), m_Handle,
									   Info.RemoteAddress);
				}
			}
		}
		return;

	case CM_CONNECTION_ITEM_DISCONNECT:
		{
			const int Sel = m_ListView.GetSelectedItem();

			if (Sel >= 0) {
				ConnectionInfo Info;

				if (m_ListView.GetItemConnectionInfo(Sel, &Info)) {
					if (Info.Protocol == ConnectionProtocol::TCP) {
						MIB_TCPROW Row;

						Row.dwState = MIB_TCP_STATE_DELETE_TCB;
						Row.dwLocalAddr = Info.LocalAddress.V4.Address;
						Row.dwLocalPort = ::htons(Info.LocalPort);
						Row.dwRemoteAddr = Info.RemoteAddress.V4.Address;
						Row.dwRemotePort = ::htons(Info.RemotePort);
						DWORD Result = ::SetTcpEntry(&Row);
						if (Result != NO_ERROR) {
							cvDebugTrace(TEXT("SetTcpEntry() Error %x\n"), Result);
							if (Result == ERROR_ACCESS_DENIED
									// ERROR_ACCESS_DENIED ‚Å‚Í‚È‚­ ERROR_MR_MID_NOT_FOUND ‚ª
									// •Ô‚Á‚Ä‚­‚écB‚È‚º?
									|| Result == ERROR_MR_MID_NOT_FOUND) {
								ShowMessage(IDS_ERROR_DISCONNECT_TITLE,
											IDS_ERROR_DISCONNECT_HEADER,
											Result == ERROR_ACCESS_DENIED ?
											IDS_ERROR_DISCONNECT_ACCESS_DENIED :
											IDS_ERROR_DISCONNECT_MESSAGE,
											TDCBF_OK_BUTTON, TD_WARNING_ICON);
							} else {
								SystemErrorDialog(m_Handle,
												  m_Core.GetLanguageInstance(),
												  Result,
												  IDS_ERROR_DISCONNECT,
												  IDS_ERROR_DISCONNECT_TITLE);
							}
						}
					}
				}
			}
		}
		return;

	case CM_CONNECTION_ITEM_BLOCK:
		{
			const int Sel = m_ListView.GetSelectedItem();

			if (Sel >= 0) {
				ConnectionInfo Info;

				if (m_ListView.GetItemConnectionInfo(Sel, &Info))
					AddBlockAddress(Info.RemoteAddress);
			}
		}
		return;

	case CM_LOG_ITEM_COPY_REMOTE_ADDRESS:
	case CM_LOG_ITEM_COPY_REMOTE_HOST:
	case CM_LOG_ITEM_COPY_PROCESS_PATH:
		{
			const int Sel = m_LogView.GetSelectedItem();

			if (Sel >= 0) {
				int Column;
				TCHAR szText[256];

				switch (Command) {
				case CM_LOG_ITEM_COPY_REMOTE_ADDRESS:
					Column = ConnectionLogView::COLUMN_REMOTE_ADDRESS;
					break;
				case CM_LOG_ITEM_COPY_REMOTE_HOST:
					Column = ConnectionLogView::COLUMN_REMOTE_HOST;
					break;
				case CM_LOG_ITEM_COPY_PROCESS_PATH:
					Column = ConnectionLogView::COLUMN_PROCESS_PATH;
					break;
				}
				if (m_LogView.GetItemText(Sel, Column, szText, cvLengthOf(szText))
						&& szText[0] != _T('\0')) {
					TextCopyToClipboard(m_Handle, szText);
				}
			}
		}
		return;

	case CM_LOG_ITEM_WHOIS:
		{
			const int Sel = m_LogView.GetSelectedItem();

			if (Sel >= 0) {
				ConnectionInfo Info;

				if (m_LogView.GetItemConnectionInfo(Sel, &Info)) {
					m_WhoisDialog.Show(m_Core.GetLanguageInstance(), m_Handle,
									   Info.RemoteAddress);
				}
			}
		}
		return;

	case CM_LOG_ITEM_BLOCK:
		{
			const int Sel = m_LogView.GetSelectedItem();

			if (Sel >= 0) {
				ConnectionInfo Info;

				if (m_LogView.GetItemConnectionInfo(Sel, &Info))
					AddBlockAddress(Info.RemoteAddress);
			}
		}
		return;

	case CM_INTERFACE_ITEM_COPY_DESCRIPTION:
	case CM_INTERFACE_ITEM_COPY_PHYSICAL_ADDRESS:
	case CM_INTERFACE_ITEM_COPY_PERMANENT_PHYSICAL_ADDRESS:
	case CM_INTERFACE_ITEM_COPY_INTERFACE_GUID:
		{
			const int Sel = m_InterfaceListView.GetSelectedItem();

			if (Sel >= 0) {
				int Column;
				TCHAR szText[256];

				switch (Command) {
				case CM_INTERFACE_ITEM_COPY_DESCRIPTION:
					Column = InterfaceListView::COLUMN_DESCRIPTION;
					break;
				case CM_INTERFACE_ITEM_COPY_PHYSICAL_ADDRESS:
					Column = InterfaceListView::COLUMN_PHYSICAL_ADDRESS;
					break;
				case CM_INTERFACE_ITEM_COPY_PERMANENT_PHYSICAL_ADDRESS:
					Column = InterfaceListView::COLUMN_PERMANENT_PHYSICAL_ADDRESS;
					break;
				case CM_INTERFACE_ITEM_COPY_INTERFACE_GUID:
					Column = InterfaceListView::COLUMN_INTERFACE_GUID;
					break;
				}
				if (m_InterfaceListView.GetItemText(Sel, Column, szText, cvLengthOf(szText))
						&& szText[0] != _T('\0')) {
					TextCopyToClipboard(m_Handle, szText);
				}
			}
		}
		return;

	case CM_BLOCK_ITEM_SETTINGS:
		{
			const int Sel = m_BlockListView.GetSelectedItem();

			if (Sel >= 0) {
				FilterManager &Manager = m_Core.GetFilterManager();
				const int Index = m_BlockListView.GetItemFilterIndex(Sel);
				FilterInfo Filter;

				if (Manager.GetFilter(Index, &Filter)) {
					FilterSettingDialog Dialog;

					if (Dialog.Show(m_Core.GetLanguageInstance(), m_Handle, &Filter)) {
						if (Filter.Match == FilterInfo::MATCH_EQUAL)
							m_Core.GetHostName(Filter.Address,
											   Filter.HostName, cvLengthOf(Filter.HostName));
						else
							Filter.HostName[0] = _T('\0');
						if (Manager.SetFilter(Index, Filter)) {
							m_BlockListView.OnListUpdated();
							if (m_CurTab == TAB_BLOCK_LIST)
								SetPropertyListValues();
						} else {
							if (Manager.GetLastError() == ERROR_ALREADY_EXISTS) {
								ShowMessage(IDS_ERROR_FILTER_ALREADY_EXISTS_TITLE,
											IDS_ERROR_FILTER_ALREADY_EXISTS, 0,
											TDCBF_OK_BUTTON, TD_INFORMATION_ICON);
							} else {
								SystemErrorDialog(m_Handle, m_Core.GetLanguageInstance(),
												  Manager.GetLastError(),
												  IDS_ERROR_FILTER_UPDATE);
							}
						}
					}
				}
			}
		}
		return;

	case CM_BLOCK_ITEM_ACTIVE:
		{
			const int Sel = m_BlockListView.GetSelectedItem();

			if (Sel >= 0) {
				FilterManager &Manager = m_Core.GetFilterManager();
				const int Index = m_BlockListView.GetItemFilterIndex(Sel);
				FilterInfo Filter;

				if (Manager.GetFilter(Index, &Filter)) {
					if (Manager.EnableFilter(Index, !Filter.Enable)) {
						//m_BlockListView.RedrawItem(Sel);
						m_BlockListView.OnListUpdated();
						if (m_CurTab == TAB_BLOCK_LIST)
							SetPropertyListValues();
					} else {
						SystemErrorDialog(m_Handle, m_Core.GetLanguageInstance(),
										  Manager.GetLastError(),
										  IDS_ERROR_FILTER_ENABLE);
					}
				}
			}
		}
		return;

	case CM_BLOCK_ITEM_REMOVE:
		{
			const int Sel = m_BlockListView.GetSelectedItem();

			if (Sel >= 0) {
				FilterManager &Manager = m_Core.GetFilterManager();
				const int Index = m_BlockListView.GetItemFilterIndex(Sel);

				if (Manager.RemoveFilter(Index)) {
					m_BlockListView.OnListUpdated();
				}
			}
		}
		return;

	case CM_BLOCK_ITEM_REMOVE_ALL:
		m_Core.GetFilterManager().Clear();
		m_BlockListView.OnListUpdated();
		return;

	case CM_PROPERTY_ITEM_COPY:
		{
			const int Sel = m_PropertyListView.GetSelectedItem();

			if (Sel >= 0) {
				TCHAR szText[256];

				if (m_PropertyListView.GetItemText(Sel, PropertyListView::COLUMN_VALUE,
												   szText, cvLengthOf(szText))
						&& szText[0] != _T('\0')) {
					TextCopyToClipboard(m_Handle, szText);
				}
			}
		}
		return;

	default:
		if (Command >= CM_LISTCOLUMN_FIRST && Command <= CM_LISTCOLUMN_LAST) {
			const int Column = Command - CM_LISTCOLUMN_FIRST;
			const bool Visible = !m_ListView.IsColumnVisible(Column);

			m_ListView.SetColumnVisible(Column, Visible);
			//::CheckMenuItem(::GetMenu(m_Handle), Command,
			//				(Visible ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			return;
		}

		if (Command >= CM_LOGCOLUMN_FIRST && Command <= CM_LOGCOLUMN_LAST) {
			const int Column = Command - CM_LOGCOLUMN_FIRST;
			const bool Visible = !m_LogView.IsColumnVisible(Column);

			m_LogView.SetColumnVisible(Column, Visible);
			//::CheckMenuItem(::GetMenu(m_Handle), Command,
			//				(Visible ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			return;
		}

		if (Command >= CM_INTERFACECOLUMN_FIRST && Command <= CM_INTERFACECOLUMN_LAST) {
			const int Column = Command - CM_INTERFACECOLUMN_FIRST;
			const bool Visible = !m_InterfaceListView.IsColumnVisible(Column);

			m_InterfaceListView.SetColumnVisible(Column, Visible);
			//::CheckMenuItem(::GetMenu(m_Handle), Command,
			//				(Visible ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			return;
		}

		if (Command >= CM_BLOCKLIST_COLUMN_FIRST && Command <= CM_BLOCKLIST_COLUMN_LAST) {
			const int Column = Command - CM_BLOCKLIST_COLUMN_FIRST;
			const bool Visible = !m_InterfaceListView.IsColumnVisible(Column);

			m_BlockListView.SetColumnVisible(Column, Visible);
			//::CheckMenuItem(::GetMenu(m_Handle), Command,
			//				(Visible ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			return;
		}
	}
}

void MainForm::OnItemRButtonUp(ListView *pListView, int x, int y)
{
	POINT pt = {x, y};
	::ClientToScreen(pListView->GetHandle(), &pt);

	if (pListView == &m_PropertyListView) {
		const int Sel = m_PropertyListView.GetSelectedItem();

		if (Sel >= 0) {
			HMENU hmenu = m_Core.LoadMenu(IDM_PROPERTY_ITEM);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
			::DestroyMenu(hmenu);
		}
	} else if (pListView == &m_ListView) {
		const int Sel = m_ListView.GetSelectedItem();

		if (Sel >= 0) {
			ConnectionInfo Info;
			m_ListView.GetItemConnectionInfo(Sel, &Info);
			const bool ZeroOrLocal = Info.RemoteAddress.IsZero() || Info.RemoteAddress.IsLocal();
			HMENU hmenu = m_Core.LoadMenu(IDM_CONNECTION_ITEM);
			::EnableMenuItem(hmenu, CM_CONNECTION_ITEM_WHOIS,
							 ((Info.Protocol == ConnectionProtocol::TCP ||
							   Info.Protocol == ConnectionProtocol::TCP_V6)
							  && !ZeroOrLocal ?
							  MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			::EnableMenuItem(hmenu, CM_CONNECTION_ITEM_DISCONNECT,
							 (Info.Protocol == ConnectionProtocol::TCP
							  && !ZeroOrLocal ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			::EnableMenuItem(hmenu, CM_CONNECTION_ITEM_BLOCK,
							 (!ZeroOrLocal ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
			::DestroyMenu(hmenu);
		}
	} else if (pListView == &m_LogView) {
		const int Sel = m_LogView.GetSelectedItem();

		if (Sel >= 0) {
			ConnectionInfo Info;
			m_LogView.GetItemConnectionInfo(Sel, &Info);
			const bool ZeroOrLocal = Info.RemoteAddress.IsZero() || Info.RemoteAddress.IsLocal();
			HMENU hmenu = m_Core.LoadMenu(IDM_LOG_ITEM);
			::EnableMenuItem(hmenu, CM_LOG_ITEM_WHOIS,
							 ((Info.Protocol == ConnectionProtocol::TCP ||
							   Info.Protocol == ConnectionProtocol::TCP_V6)
							  && !ZeroOrLocal ?
							  MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			::EnableMenuItem(hmenu, CM_LOG_ITEM_BLOCK,
							 (!ZeroOrLocal ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
			::DestroyMenu(hmenu);
		}
	} else if (pListView == &m_InterfaceListView) {
		const int Sel = m_InterfaceListView.GetSelectedItem();

		if (Sel >= 0) {
			HMENU hmenu = m_Core.LoadMenu(IDM_INTERFACE_ITEM);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
			::DestroyMenu(hmenu);
		}
	} else if (pListView == &m_BlockListView) {
		const int Sel = m_BlockListView.GetSelectedItem();

		if (Sel >= 0) {
			const FilterManager &Manager = m_Core.GetFilterManager();
			const int Index = m_BlockListView.GetItemFilterIndex(Sel);
			FilterInfo Filter;
			Manager.GetFilter(Index, &Filter);
			HMENU hmenu = m_Core.LoadMenu(IDM_BLOCK_ITEM);
			::CheckMenuItem(hmenu, CM_BLOCK_ITEM_ACTIVE,
							(Filter.Enable ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
			::DestroyMenu(hmenu);
		}
	}
}

void MainForm::OnHeaderRButtonUp(ListView *pListView, int x, int y)
{
	HMENU hmenu = ::GetMenu(m_Handle);

	if (pListView == &m_ListView) {
		hmenu = ::GetSubMenu(::GetSubMenu(hmenu, MENU_POS_VIEW), MENU_POS_VIEW_CONNECTION_COLUMNS);
	} else if (pListView == &m_LogView) {
		hmenu = ::GetSubMenu(::GetSubMenu(hmenu, MENU_POS_VIEW), MENU_POS_VIEW_LOG_COLUMNS);
	} else if (pListView == &m_InterfaceListView) {
		hmenu = ::GetSubMenu(::GetSubMenu(hmenu, MENU_POS_VIEW), MENU_POS_VIEW_INTERFACE_COLUMNS);
	} else if (pListView == &m_BlockListView) {
		hmenu = ::GetSubMenu(::GetSubMenu(hmenu, MENU_POS_VIEW), MENU_POS_VIEW_BLOCK_COLUMNS);
	} else {
		return;
	}
	POINT pt = {x, y};
	::ClientToScreen(pListView->GetHandle(), &pt);
	::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_Handle, nullptr);
}

void MainForm::OnSelChanged(ListView *pListView)
{
	if (static_cast<Widget*>(pListView) == m_TabWidgetList[m_CurTab])
		SetPropertyListValues();
}

// GraphView::EventHandler
void MainForm::OnRButtonUp(GraphView *pGraphView, int x, int y)
{
	HMENU hmenu = m_Core.LoadMenu(IDM_GRAPH);
	HMENU hmenuSub = ::GetSubMenu(hmenu, 0);

	const int NumInterfaces = m_Core.NumNetworkInterfaces();
	std::vector<GUID> GuidList;
	GuidList.reserve(NumInterfaces);

	int Sel = m_GraphInterfaceGUID == GUID_NULL ? 0 : -1;

	for (int i = 0; i < NumInterfaces; i++) {
		const MIB_IF_ROW2 *pInfo = m_Core.GetNetworkInterfaceInfo(i);

		::AppendMenu(hmenuSub, MF_STRING | MF_ENABLED,
					 CM_GRAPH_INTERFACE_FIRST + i,
					 pInfo->Description);
		GuidList.push_back(pInfo->InterfaceGuid);
		if (Sel < 0 && pInfo->InterfaceGuid == m_GraphInterfaceGUID)
			Sel = i + 1;
	}
	if (Sel >= 0)
		::CheckMenuRadioItem(hmenuSub,
							 0, NumInterfaces, Sel,
							 MF_BYPOSITION);
	POINT pt = {x, y};
	::ClientToScreen(pGraphView->GetHandle(), &pt);
	int Command = ::TrackPopupMenu(hmenuSub, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, m_Handle, nullptr);
	::DestroyMenu(hmenu);
	GUID Guid;
	if (Command == CM_GRAPH_INTERFACE_ALL_HARDWARE) {
		Guid = GUID_NULL;
	} else if (Command >= CM_GRAPH_INTERFACE_FIRST
			   && Command < CM_GRAPH_INTERFACE_FIRST + NumInterfaces) {
		Guid = GuidList[Command - CM_GRAPH_INTERFACE_FIRST];
	} else
		return;
	SetGraphInterface(Guid);
}

int MainForm::ShowMessage(int TitleID, int HeaderID, int MessageID,
						  TASKDIALOG_COMMON_BUTTON_FLAGS Buttons, PCWSTR pIcon)
{
	int Button = 0;
	::TaskDialog(m_Handle, m_Core.GetLanguageInstance(),
				 MAKEINTRESOURCE(TitleID),
				 MAKEINTRESOURCE(HeaderID),
				 MAKEINTRESOURCE(MessageID),
				 Buttons, pIcon, &Button);
	return Button;
}

void MainForm::UpdateStatus()
{
	m_Core.UpdateConnectionStatus();
	const ULONGLONG CurTime = m_Core.GetUpdatedTickCount();

	const int NumConnections = m_Core.NumConnections();

	if (m_ResolveAddresses)
		ResolveAddresses();

	m_ListView.OnListUpdated();
	m_LogView.OnListUpdated();
	m_InterfaceListView.OnListUpdated();

	NetworkInterfaceStatistics IfStats;
	//bool EnableIfStats = m_Core.GetNetworkInterfaceTotalStatistics(&IfStats);
	bool EnableIfStats;
	if (m_GraphInterfaceGUID == GUID_NULL)
		EnableIfStats = m_Core.GetNetworkInterfaceTotalStatistics(&IfStats);
	else
		EnableIfStats = m_Core.GetNetworkInterfaceStatistics(m_GraphInterfaceGUID, &IfStats);

	SetPropertyListValues();

	TCHAR szFormat[64], szText[128], szValue[64];

	SetCurTabStatusText();

	m_Core.LoadText(IDS_STATUS_IN_BANDWIDTH, szFormat, cvLengthOf(szFormat));
	int InBandwidth = 0;
	if (m_EnableNetworkIfStats && EnableIfStats) {
		if (IfStats.InBytes > m_NetworkIfStats.InBytes)
			InBandwidth = (int)((IfStats.InBytes - m_NetworkIfStats.InBytes) * 1000 / (CurTime - m_UpdatedTime));
		FormatBandwidth(InBandwidth, szValue, cvLengthOf(szValue));
	} else {
		::lstrcpy(szValue, TEXT("n/a"));
	}
	FormatString(szText, cvLengthOf(szText), szFormat, szValue);
	m_StatusBar.SetPartText(1, szText);

	m_Core.LoadText(IDS_STATUS_OUT_BANDWIDTH, szFormat, cvLengthOf(szFormat));
	int OutBandwidth = 0;
	if (m_EnableNetworkIfStats && EnableIfStats) {
		if (IfStats.OutBytes > m_NetworkIfStats.OutBytes)
			OutBandwidth = (int)((IfStats.OutBytes - m_NetworkIfStats.OutBytes) * 1000 / (CurTime - m_UpdatedTime));
		FormatBandwidth(OutBandwidth, szValue, cvLengthOf(szValue));
	} else {
		::lstrcpy(szValue, TEXT("n/a"));
	}
	FormatString(szText, cvLengthOf(szText), szFormat, szValue);
	m_StatusBar.SetPartText(2, szText);

	m_Core.LoadText(IDS_STATUS_IN_BYTES, szFormat, cvLengthOf(szFormat));
	if (EnableIfStats)
		FormatUInt64(IfStats.InBytes, szValue, cvLengthOf(szValue));
	else
		::lstrcpy(szValue, TEXT("n/a"));
	FormatString(szText, cvLengthOf(szText), szFormat, szValue);
	m_StatusBar.SetPartText(3, szText);

	m_Core.LoadText(IDS_STATUS_OUT_BYTES, szFormat, cvLengthOf(szFormat));
	if (EnableIfStats)
		FormatUInt64(IfStats.OutBytes, szValue, cvLengthOf(szValue));
	else
		::lstrcpy(szValue, TEXT("n/a"));
	FormatString(szText, cvLengthOf(szText), szFormat, szValue);
	m_StatusBar.SetPartText(4, szText);

	/*
	if (m_GraphInterfaceGUID!=GUID_NULL) {
		const InterfaceListView::ItemInfo *pInfo=
			m_InterfaceListView.GetItemInfo(m_GraphInterfaceGUID);
		if (pInfo!=nullptr) {
			InBandwidth=pInfo->InBytesPerSecond;
			OutBandwidth=pInfo->OutBytesPerSecond;
		} else {
			InBandwidth=0;
			OutBandwidth=0;
		}
	}
	*/
	GraphView::GraphInfo *pGraph;
	pGraph = m_GraphView.GetGraph(GRAPH_IN_BANDWIDTH);
	if (pGraph != nullptr) {
		if (pGraph->List.size() >= MAX_GRAPH_HISTORY)
			pGraph->List.pop_front();
		pGraph->List.push_back(InBandwidth);
	}
	pGraph = m_GraphView.GetGraph(GRAPH_OUT_BANDWIDTH);
	if (pGraph != nullptr) {
		if (pGraph->List.size() >= MAX_GRAPH_HISTORY)
			pGraph->List.pop_front();
		pGraph->List.push_back(OutBandwidth);
	}
	pGraph = m_GraphView.GetGraph(GRAPH_CONNECTIONS);
	if (pGraph != nullptr) {
		if (pGraph->List.size() >= MAX_GRAPH_HISTORY)
			pGraph->List.pop_front();
		pGraph->List.push_back(NumConnections);
	}
	if (m_CurTab == TAB_GRAPH)
		m_GraphView.Redraw();

	m_EnableNetworkIfStats = EnableIfStats;
	if (EnableIfStats)
		m_NetworkIfStats = IfStats;
	m_UpdatedTime = CurTime;
}

void MainForm::ResolveAddresses()
{
	const int NumConnections = m_Core.NumConnections();

	for (int i = 0; i < NumConnections; i++) {
		class GetHostNameRequest : public HostManager::Request
		{
			HWND m_hwnd;
			std::deque<IPAddress> &m_FoundHostList;
			LocalLock &m_FoundHostLock;

		public:
			GetHostNameRequest(const IPAddress &Address, WORD Port, HWND hwnd,
							   std::deque<IPAddress> &FoundHostList, LocalLock &FoundHostLock)
				: Request(Address, Port)
				, m_hwnd(hwnd)
				, m_FoundHostList(FoundHostList)
				, m_FoundHostLock(FoundHostLock)
			{
			}

			void OnHostFound(LPCTSTR pHostName) override
			{
				m_FoundHostLock.Lock();
				m_FoundHostList.push_back(m_Address);
				m_FoundHostLock.Unlock();
				::PostMessage(m_hwnd, WM_APP_HOSTFOUND, 0, 0);
			}
		};

		ConnectionInfo Info;

		m_Core.GetConnectionInfo(i, &Info);
		if (!Info.RemoteAddress.IsZero()
				&& (Info.RemoteAddress.Type != IP_ADDRESS_V4
					|| Info.RemoteAddress.V4.Address != 0xFFFFFFFF)
				&& !m_Core.GetHostName(Info.RemoteAddress, nullptr, 0))
			m_Core.GetHostName(new GetHostNameRequest(Info.RemoteAddress, Info.RemotePort,
							   m_Handle, m_FoundHostList, m_FoundHostLock));
	}
}

void MainForm::SetCurTabStatusText()
{
	TCHAR szFormat[128], szText[256];

	if (m_CurTab == TAB_CONNECTION_LOG) {
		m_Core.LoadText(IDS_STATUS_LOG, szFormat, cvLengthOf(szFormat));
		FormatString(szText, cvLengthOf(szText), szFormat,
					 m_LogView.NumItems(), (int)m_Core.GetConnectionLog().GetMaxLog());
	} else if (m_CurTab == TAB_INTERFACE_LIST) {
		m_Core.LoadText(IDS_STATUS_INTERFACES, szFormat, cvLengthOf(szFormat));
		FormatString(szText, cvLengthOf(szText), szFormat, m_Core.NumNetworkInterfaces());
	} else if (m_CurTab == TAB_BLOCK_LIST) {
		m_Core.LoadText(IDS_STATUS_BLOCK_FILTERS, szFormat, cvLengthOf(szFormat));
		FormatString(szText, cvLengthOf(szText), szFormat,
					 m_Core.GetFilterManager().NumFilters());
	} else {
		m_Core.LoadText(IDS_STATUS_CONNECTIONS, szFormat, cvLengthOf(szFormat));
		FormatString(szText, cvLengthOf(szText), szFormat,
					 m_Core.NumConnections(),
					 m_Core.NumTCPConnections(), m_Core.NumUDPConnections());
	}
	m_StatusBar.SetPartText(0, szText);
}

void MainForm::SetPropertyListNames()
{
	const ListView *pList = dynamic_cast<const ListView*>(m_TabWidgetList[m_CurTab]);

	if (pList == nullptr) {
		m_PropertyListView.SetItemList(nullptr, 0);
		return;
	}
	const int NumColumns = pList->NumColumns();
	std::vector<ListView::ColumnInfo> ColumnList;
	std::vector<LPCTSTR> NameList;
	ColumnList.resize(NumColumns);
	NameList.resize(NumColumns);
	for (int i = 0; i < NumColumns; i++) {
		pList->GetColumnInfo(i, &ColumnList[i]);
		NameList[i] = ColumnList[i].szText;
	}
	m_PropertyListView.SetItemList(&NameList[0], NumColumns);
}

void MainForm::SetPropertyListValues()
{
	const ListView *pList = dynamic_cast<const ListView*>(m_TabWidgetList[m_CurTab]);

	if (pList == nullptr)
		return;
	const int SelItem = pList->GetSelectedItem();
	if (SelItem >= 0) {
		const int NumColumns = pList->NumColumns();
		for (int i = 0; i < NumColumns; i++) {
			ListView::ColumnInfo Column;
			TCHAR szText[256];

			pList->GetColumnInfo(i, &Column);
			pList->GetItemLongText(SelItem, Column.ID, szText, cvLengthOf(szText));
			m_PropertyListView.SetItemValue(i, szText);
		}
	}
}

void MainForm::SetGraphCaption()
{
	LPCTSTR pCaption = nullptr;
	TCHAR szText[64];

	if (m_GraphInterfaceGUID != GUID_NULL) {
		const MIB_IF_ROW2 *pInfo = m_Core.GetNetworkInterfaceInfo(m_GraphInterfaceGUID);
		if (pInfo != nullptr)
			pCaption = pInfo->Description;
	} else {
		m_Core.LoadText(IDS_GRAPH_ALL_HARDWARE_INTERFACES,
						szText, cvLengthOf(szText));
		pCaption = szText;
	}
	m_GraphView.SetCaption(pCaption);
}

bool MainForm::ApplyPreferences(const AllPreferences &Pref)
{
	m_ListView.SetFont(Pref.List.Font);
	m_ListView.ShowGrid(Pref.List.ShowGrid);
	m_ListView.SetColors(Pref.List.TextColor, Pref.List.GridColor,
						 Pref.List.BackColor1, Pref.List.BackColor2,
						 Pref.List.SelTextColor, Pref.List.SelBackColor);
	m_ListView.SetSpecialColors(Pref.List.NewBackColor);

	m_LogView.SetFont(Pref.List.Font);
	m_LogView.ShowGrid(Pref.List.ShowGrid);
	m_LogView.SetColors(Pref.List.TextColor, Pref.List.GridColor,
						Pref.List.BackColor1, Pref.List.BackColor2,
						Pref.List.SelTextColor, Pref.List.SelBackColor);
	m_LogView.SetSpecialColors(Pref.List.NewBackColor);

	const size_t LogItems = m_Core.GetConnectionLog().NumItems();
	m_Core.SetConnectionLogMax(Pref.Log.MaxLog);
	if (LogItems > Pref.Log.MaxLog)
		m_LogView.OnListUpdated();

	m_InterfaceListView.SetFont(Pref.List.Font);
	m_InterfaceListView.ShowGrid(Pref.List.ShowGrid);
	m_InterfaceListView.SetColors(Pref.List.TextColor, Pref.List.GridColor,
								  Pref.List.BackColor1, Pref.List.BackColor2,
								  Pref.List.SelTextColor, Pref.List.SelBackColor);

	m_BlockListView.SetFont(Pref.List.Font);
	m_BlockListView.ShowGrid(Pref.List.ShowGrid);
	m_BlockListView.SetColors(Pref.List.TextColor, Pref.List.GridColor,
							  Pref.List.BackColor1, Pref.List.BackColor2,
							  Pref.List.SelTextColor, Pref.List.SelBackColor);

	m_PropertyListView.SetFont(Pref.List.Font);
	m_PropertyListView.ShowGrid(Pref.List.ShowGrid);
	m_PropertyListView.SetColors(Pref.List.TextColor, Pref.List.GridColor,
								 Pref.List.BackColor1, Pref.List.BackColor2,
								 Pref.List.SelTextColor, Pref.List.SelBackColor);

	m_GraphView.SetColors(Pref.Graph.BackColor,
						  Pref.Graph.GridColor,
						  Pref.Graph.CaptionColor);
	GraphView::GraphInfo *pGraph;
	pGraph = m_GraphView.GetGraph(GRAPH_IN_BANDWIDTH);
	if (pGraph != nullptr) {
		pGraph->Type = (GraphView::GraphType)Pref.Graph.InBandwidth.Type;
		pGraph->Color = Pref.Graph.InBandwidth.Color;
		pGraph->Scale = Pref.Graph.InBandwidth.Scale;
	}
	pGraph = m_GraphView.GetGraph(GRAPH_OUT_BANDWIDTH);
	if (pGraph != nullptr) {
		pGraph->Type = (GraphView::GraphType)Pref.Graph.OutBandwidth.Type;
		pGraph->Color = Pref.Graph.OutBandwidth.Color;
		pGraph->Scale = Pref.Graph.OutBandwidth.Scale;
	}
	pGraph = m_GraphView.GetGraph(GRAPH_CONNECTIONS);
	if (pGraph != nullptr) {
		pGraph->Type = (GraphView::GraphType)Pref.Graph.Connections.Type;
		pGraph->Color = Pref.Graph.Connections.Color;
		pGraph->Scale = Pref.Graph.Connections.Scale;
	}
	if (m_CurTab == TAB_GRAPH)
		m_GraphView.Redraw();

	if (Pref.Core.GeoIPDatabaseFileName[0] != _T('\0')) {
		const GeoIPManager &Manager = m_Core.GetGeoIPManager();
		bool Open;

		if (Manager.IsOpen()) {
			TCHAR szFileName[MAX_PATH];

			Manager.GetFileName(szFileName, cvLengthOf(szFileName));
			Open = ::lstrcmpi(szFileName, Pref.Core.GeoIPDatabaseFileName) != 0;
		} else
			Open = true;
		if (Open)
			m_Core.OpenGeoIP(Pref.Core.GeoIPDatabaseFileName);
	}

	return true;
}

DWORD MainForm::SaveListFile(LPCTSTR pFileName, ListType Type, const ListView *pList) const
{
	HANDLE hFile = ::CreateFile(pFileName, GENERIC_WRITE, 0, nullptr,
							   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return ::GetLastError();

	const char Separator = Type == LIST_TYPE_CSV ? ',' : '\t';
	int Length;
	DWORD Wrote;

	const int NumColumns = pList->NumColumns();

	bool First = true;
	for (int i = 0; i < NumColumns; i++) {
		ListView::ColumnInfo Column;

		pList->GetColumnInfo(i, &Column);
		if (Column.Visible) {
			char szText[ListView::MAX_COLUMN_TEXT + 1];

			if (First) {
				First = false;
				Length = 0;
			} else {
				szText[0] = Separator;
				Length = 1;
			}
#ifdef UNICODE
			Length += ::WideCharToMultiByte(CP_ACP, 0, Column.szText, ::lstrlen(Column.szText),
											szText + Length, cvLengthOf(szText) - Length, nullptr, nullptr);
#else
			::lstrcpyn(szText + Length, Column.szText, cvLengthOf(szText) - Length);
			Length = ::lstrlenA(szText);
#endif
			if (!::WriteFile(hFile, szText, Length, &Wrote, nullptr) || Wrote != (DWORD)Length) {
				DWORD Error = ::GetLastError();
				if (Error == ERROR_SUCCESS)
					Error = ERROR_WRITE_FAULT;
				::CloseHandle(hFile);
				return Error;
			}
		}
	}

	const int NumItems = pList->NumItems();

	for (int i = 0; i < NumItems; i++) {
		if (!::WriteFile(hFile, "\r\n", 2, &Wrote, nullptr) || Wrote != 2) {
			DWORD Error = ::GetLastError();
			if (Error == ERROR_SUCCESS)
				Error = ERROR_WRITE_FAULT;
			::CloseHandle(hFile);
			return Error;
		}

		First = true;
		for (int j = 0; j < NumColumns; j++) {
			ListView::ColumnInfo Column;

			pList->GetColumnInfo(j, &Column);
			if (Column.Visible) {
				TCHAR szText[256];

				if (First) {
					First = false;
					Length = 0;
				} else {
					szText[0] = Separator;
					Length = 1;
				}
				pList->GetItemText(i, j, szText + Length, cvLengthOf(szText) - Length);
				Length = ::lstrlen(szText);
#ifdef UNICODE
				char szAnsiText[256];
				Length = ::WideCharToMultiByte(CP_ACP, 0, szText, Length,
											   szAnsiText, cvLengthOf(szAnsiText), nullptr, nullptr);
#endif
				if (!::WriteFile(hFile,
#ifdef UNICODE
								 szAnsiText,
#else
								 szText,
#endif
								 Length, &Wrote, nullptr) || Wrote != (DWORD)Length) {
					DWORD Error = ::GetLastError();
					if (Error == ERROR_SUCCESS)
						Error = ERROR_WRITE_FAULT;
					::CloseHandle(hFile);
					return Error;
				}
			}
		}
	}
	::CloseHandle(hFile);
	return ERROR_SUCCESS;
}

LPTSTR MainForm::GetListText(const ListView *pList, ListType Type) const
{
	const int NumItems = pList->NumItems();
	if (NumItems == 0)
		return nullptr;

	size_t BufferLength = 32 * 1024, ListLength = 0;
	LPTSTR pBuffer = new TCHAR[BufferLength];

	const int NumColumns = pList->NumColumns();
	const TCHAR Separator = Type == LIST_TYPE_CSV ? _T(',') : _T('\t');

	for (int i = 0; i < NumItems; i++) {
		bool First = true;

		for (int j = 0; j < NumColumns; j++) {
			ListView::ColumnInfo Column;

			pList->GetColumnInfo(j, &Column);
			if (Column.Visible) {
				int Length;
				TCHAR szText[256];

				if (First) {
					First = false;
					Length = 0;
				} else {
					szText[0] = Separator;
					Length = 1;
				}
				pList->GetItemText(i, j, szText + Length, cvLengthOf(szText) - Length);
				Length = ::lstrlen(szText);
				if (ListLength + Length + 3 >= BufferLength) {
					BufferLength *= 2;
					LPTSTR pNewBuffer = new TCHAR[BufferLength];
					::CopyMemory(pNewBuffer, pBuffer, ListLength * sizeof(TCHAR));
					delete [] pBuffer;
					pBuffer = pNewBuffer;
				}
				::lstrcpy(pBuffer + ListLength, szText);
				ListLength += Length;
			}
		}
		if (i + 1 < NumItems) {
			pBuffer[ListLength++] = _T('\r');
			pBuffer[ListLength++] = _T('\n');
		}
	}
	pBuffer[ListLength] = _T('\0');

	return pBuffer;
}

bool MainForm::AddBlockAddress(const IPAddress &Address)
{
	if (Address.IsZero() || Address.IsLocal())
		return false;

	FilterManager &Manager = m_Core.GetFilterManager();

	FilterInfo Filter;
	Filter.Type = FilterInfo::TYPE_BLOCK_ADDRESS;
	Filter.Match = FilterInfo::MATCH_EQUAL;
	Filter.Address = Address;
	if (!m_Core.GetHostName(Address,
							Filter.HostName, cvLengthOf(Filter.HostName)))
		Filter.HostName[0] = _T('\0');
	Filter.Enable = true;
	::GetSystemTimeAsFileTime(&Filter.AddedTime);
	Filter.Comment[0] = _T('\0');
	if (!Manager.AddFilter(Filter)) {
		if (Manager.GetLastError() != ERROR_ALREADY_EXISTS)
			SystemErrorDialog(m_Handle, m_Core.GetLanguageInstance(),
							  Manager.GetLastError(), IDS_ERROR_FILTER_ADD);
		return false;
	}

	m_BlockListView.OnListUpdated();
	m_BlockListView.SelectItem(m_BlockListView.FilterIDToIndex(Filter.ID));
	if (m_CurTab == TAB_BLOCK_LIST)
		SetPropertyListValues();

	return true;
}

}	// namespace CV
