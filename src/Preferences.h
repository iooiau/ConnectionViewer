/******************************************************************************
*                                                                             *
*    Preferences.h                          Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_PREFERENCES_H
#define CV_PREFERENCES_H


namespace CV
{

struct CorePreferences
{
	TCHAR GeoIPDatabaseFileName[MAX_PATH];

	CorePreferences();
	void SetDefault();
};

struct ListPreferences
{
	LOGFONT Font;
	bool ShowGrid;
	COLORREF TextColor;
	COLORREF GridColor;
	COLORREF BackColor1;
	COLORREF BackColor2;
	COLORREF SelTextColor;
	COLORREF SelBackColor;
	COLORREF NewBackColor;

	ListPreferences();
	void SetDefault();
};

struct LogPreferences
{
	unsigned int MaxLog;

	LogPreferences();
	void SetDefault();
};

struct GraphPreferences
{
	struct GraphInfo
	{
		enum GraphType
		{
			TYPE_LINE,
			TYPE_AREA
		};

		GraphType Type;
		COLORREF Color;
		int Scale;
	};

	COLORREF BackColor;
	COLORREF GridColor;
	COLORREF CaptionColor;
	GraphInfo InBandwidth;
	GraphInfo OutBandwidth;
	GraphInfo Connections;

	GraphPreferences();
	void SetDefault();
};

struct AllPreferences
{
	CorePreferences Core;
	ListPreferences List;
	LogPreferences Log;
	GraphPreferences Graph;

	void SetDefault();
};

class PreferencesDialog
{
public:
	cvAbstractClass(EventHandler)
	{
public:
		virtual ~EventHandler() {}
		virtual bool ApplyPreferences(const AllPreferences & Pref) { return true; }
	};

	PreferencesDialog(AllPreferences &Pref, EventHandler *pHandler);
	~PreferencesDialog();
	bool Show(HINSTANCE hinst, HWND Owner);
	const ListPreferences &GetListPreferences() const;

private:
	static INT_PTR CALLBACK GeneralDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK GraphDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static PreferencesDialog *GetThis(HWND hDlg);

	EventHandler *m_pEventHandler;
	AllPreferences &m_CurPref;
	AllPreferences m_TempPref;
	static int m_StartPage;
};

}	// namespace CV


#endif	// ndef CV_PREFERENCES_H
