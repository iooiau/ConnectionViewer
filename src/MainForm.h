/******************************************************************************
*                                                                             *
*    MainForm.h                             Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_MAIN_FORM_H
#define CV_MAIN_FORM_H


#include <deque>
#include "ConnectionListView.h"
#include "ConnectionLogView.h"
#include "GraphView.h"
#include "InterfaceListView.h"
#include "BlockListView.h"
#include "PropertyListView.h"
#include "Tab.h"
#include "ToolBar.h"
#include "StatusBar.h"
#include "Whois.h"
#include "Settings.h"


namespace CV
{

class MainForm
	: public CustomWidget
	, protected PreferencesDialog::EventHandler
	, protected ListView::EventHandler
	, protected GraphView::EventHandler
{
public:
	enum
	{
		TAB_CONNECTION_LIST,
		TAB_CONNECTION_LOG,
		TAB_GRAPH,
		TAB_INTERFACE_LIST,
		TAB_BLOCK_LIST,
		NUM_TAB_ITEMS
	};

	static bool Initialize(HINSTANCE hinst);

	MainForm(ProgramCore &Core);
	~MainForm();
	bool Create(HWND Parent = nullptr, int ID = 0) override;
	bool Show(int CmdShow);
	bool LoadSettings(Settings *pSettings);
	bool SaveSettings(Settings *pSettings) const;
	bool SetUpdateInterval(DWORD Interval);
	void PauseUpdate(bool Pause);
	void SetResolveAddresses(bool Resolve);
	bool SetCurTab(int Tab);
	bool TranslateAccelerator(MSG *pMessage) const;
	void ShowPropertyList(bool Show);
	bool SetGraphInterface(const GUID &Guid);
	bool SetFilterActive(bool Active);
	int ShowMessage(int TitleID, int HeaderID, int MessageID = 0,
					TASKDIALOG_COMMON_BUTTON_FLAGS Buttons = TDCBF_OK_BUTTON,
					PCWSTR pIcon = nullptr);
	// PreferencesDialog::EventHandler
	bool ApplyPreferences(const AllPreferences &Pref) override;

private:
	// CustomWidget
	LRESULT OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;
	// ListView::EventHandler
	void OnItemRButtonUp(ListView *pListView, int x, int y) override;
	void OnHeaderRButtonUp(ListView *pListView, int x, int y) override;
	void OnSelChanged(ListView *pListView) override;
	// GraphView::EventHandler
	void OnRButtonUp(GraphView *pGraphView, int x, int y) override;

	void OnCommand(int Command, int NotifyCode = 0);
	void UpdateStatus();
	void ResolveAddresses();
	void SetCurTabStatusText();
	void SetPropertyListNames();
	void SetPropertyListValues();
	void SetGraphCaption();

	void LoadListViewSettings(ListView &List, Settings *pSettings, LPCTSTR pName);
	void SaveListViewSettings(const ListView &List, Settings *pSettings, LPCTSTR pName) const;

	enum ListType
	{
		LIST_TYPE_CSV,
		LIST_TYPE_TSV
	};
	DWORD SaveListFile(LPCTSTR pFileName, ListType Type, const ListView *pList) const;
	LPTSTR GetListText(const ListView *pList, ListType Type) const;

	bool AddBlockAddress(const IPAddress &Address);

	static const LPCTSTR m_pClassName;
	static ATOM m_ClassAtom;
	static HINSTANCE m_Instance;

	ProgramCore &m_Core;
	ConnectionListView m_ListView;
	ConnectionLogView m_LogView;
	GraphView m_GraphView;
	InterfaceListView m_InterfaceListView;
	BlockListView m_BlockListView;
	Widget *m_TabWidgetList[NUM_TAB_ITEMS];
	PropertyListView m_PropertyListView;
	Tab m_Tab;
	ToolBar m_ToolBar;
	StatusBar m_StatusBar;
	bool m_ShowPropertyList;
	bool m_ShowStatusBar;
	int m_SplitterWidth;
	int m_PropertyListHeight;
	HACCEL m_Accelerators;
	DWORD m_UpdateInterval;
	bool m_Paused;
	bool m_Minimized;
	bool m_ResolveAddresses;
	bool m_FilterActive;
	ULONGLONG m_UpdatedTime;
	bool m_EnableNetworkIfStats;
	NetworkInterfaceStatistics m_NetworkIfStats;
	std::deque<IPAddress> m_FoundHostList;
	LocalLock m_FoundHostLock;
	int m_CurTab;
	int m_SaveFilterIndex;
	TCHAR m_szListSaveDirectory[MAX_PATH];
	WhoisServerList m_WhoisServerList;
	WhoisDialog m_WhoisDialog;
	GUID m_GraphInterfaceGUID;
};

}	// namespace CV


#endif	// ndef CV_MAIN_FORM_H
