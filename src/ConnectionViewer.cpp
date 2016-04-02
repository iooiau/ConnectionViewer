/******************************************************************************
*                                                                             *
*    ConnectionViewer.cpp                   Copyright(c) 2010-2016 itow,y.    *
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
#include "resource.h"


namespace CV
{

class ProgramMain
{
public:
	ProgramMain(HINSTANCE hinst);
	~ProgramMain();
	bool Initialize(int CmdShow);
	int MainLoop();

private:
	HINSTANCE m_hInstance;
	ProgramCore m_Core;
	MainForm m_MainForm;
	TCHAR m_szIniFileName[MAX_PATH];
};

ProgramMain::ProgramMain(HINSTANCE hinst)
	: m_hInstance(hinst)
	, m_MainForm(m_Core)
{
	::CoInitialize(nullptr);

	::GetModuleFileName(nullptr, m_szIniFileName, cvLengthOf(m_szIniFileName));
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));
}

ProgramMain::~ProgramMain()
{
	::CoUninitialize();
}

bool ProgramMain::Initialize(int CmdShow)
{
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_LISTVIEW_CLASSES;
	::InitCommonControlsEx(&iccx);

	WSADATA WSAData;
	::WSAStartup(MAKEWORD(2, 2), &WSAData);

	MainForm::Initialize(m_hInstance);

	TCHAR szDirectory[MAX_PATH], szFileName[MAX_PATH];
	::GetModuleFileName(m_hInstance, szDirectory, cvLengthOf(szDirectory));
	::PathRemoveFileSpec(szDirectory);
	if (GeoIPManager::FindDatabaseFile(szDirectory, szFileName, cvLengthOf(szFileName)))
		::lstrcpy(m_Core.GetPreferences().Core.GeoIPDatabaseFileName,
				  ::PathFindFileName(szFileName));

	Settings Setting;
	if (Setting.Open(m_szIniFileName, TEXT("Settings"), Settings::OPEN_READ)) {
		m_Core.LoadPreferences(&Setting);
		m_MainForm.LoadSettings(&Setting);
		Setting.Close();
		m_Core.GetFilterManager().LoadFilterList(m_szIniFileName);
	}

	m_MainForm.ApplyPreferences(m_Core.GetPreferences());

	if (!m_MainForm.Create()) {
		ErrorDialog(nullptr, m_hInstance, IDS_ERROR_WINDOW_CREATE);
		return false;
	}

	m_MainForm.Show(CmdShow);
	m_MainForm.Update();

	return true;
}

int ProgramMain::MainLoop()
{
	BOOL Result;
	MSG msg;

	while ((Result = ::GetMessage(&msg, nullptr, 0, 0)) != 0 && Result != -1) {
		if (!m_MainForm.TranslateAccelerator(&msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	::WSACleanup();

	Settings Setting;
	if (Setting.Open(m_szIniFileName, TEXT("Settings"), Settings::OPEN_WRITE)) {
		m_Core.SavePreferences(&Setting);
		m_MainForm.SaveSettings(&Setting);
		Setting.Close();
		m_Core.GetFilterManager().SaveFilterList(m_szIniFileName);
	}

	return (int)msg.wParam;
}

}	// namespace CV


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					   LPTSTR pszCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#endif

	::SetDllDirectory(TEXT(""));

	CV::ProgramMain Main(hInstance);

	if (!Main.Initialize(nCmdShow))
		return 0;

	return Main.MainLoop();
}
