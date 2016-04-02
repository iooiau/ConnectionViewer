/******************************************************************************
*                                                                             *
*    GeoIPDownloader.cpp                    Copyright(c) 2010-2016 itow,y.    *
*                                                                             *
******************************************************************************/

/*
  Connection Viewer
  Copyright(c) 2010-2016 itow,y.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif

#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlwapi.h>
#include <tchar.h>
#include "../zlib/zlib.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "zlib.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#define cvLengthOf(a) (sizeof(a) / sizeof(a[0]))

#define MAX_URL_LENGTH		256
#define MAX_PROXY_LENGTH	256

static const LPCTSTR g_pURLList[] = {
	TEXT("http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz"),
	TEXT("http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz"),
};

enum DownloadError
{
	ERR_SUCCESS,
	ERR_ABORT,
	ERR_CONNECT,
	ERR_OPEN_INTERNET,
	ERR_OPEN_URL,
	ERR_QUERY_SIZE,
	ERR_MEMORY_ALLOC,
	ERR_FILE_CREATE,
	ERR_FILE_OPEN,
	ERR_FILE_READ,
	ERR_FILE_WRITE,
	ERR_DECOMPRESS,
	ERR_REPLACE_FILE
};

struct DownloadInfo
{
	TCHAR szURL[MAX_URL_LENGTH];
	TCHAR szProxy[MAX_PROXY_LENGTH];
	TCHAR szFileName[MAX_PATH];
	LPCTSTR pUserAgent;
	HWND hDlg;
	HANDLE hAbortEvent;
	HANDLE hThread;
	DownloadError Error;
	ULONGLONG StartTime;
	ULONGLONG PrevTime;
	DWORD DownloadRate;
};

struct DownloadSettings
{
	TCHAR szURL[MAX_URL_LENGTH];
	bool fUseProxy;
	TCHAR szProxy[MAX_PROXY_LENGTH];
};


static HINSTANCE g_hInst;
static DownloadSettings g_Settings;


static int ShowMessage(HWND hwndOwner, UINT MessageID, UINT CaptionID, DWORD Style)
{
	MSGBOXPARAMS msp;

	msp.cbSize = sizeof(msp);
	msp.hwndOwner = hwndOwner;
	msp.hInstance = g_hInst;
	msp.lpszText = MAKEINTRESOURCE(MessageID);
	msp.lpszCaption = MAKEINTRESOURCE(CaptionID);
	msp.dwStyle = Style;
	msp.lpfnMsgBoxCallback = nullptr;
	msp.dwLanguageId = 0;
	return ::MessageBoxIndirect(&msp);
}


static void EnableDlgItem(HWND hDlg, int ID, bool fEnable)
{
	::EnableWindow(::GetDlgItem(hDlg, ID), fEnable);
}


static void SetDlgItemTextFromResource(HWND hDlg, int ControlID, UINT TextID)
{
	TCHAR szText[256];

	::LoadString(g_hInst, TextID, szText, cvLengthOf(szText));
	::SetDlgItemText(hDlg, ControlID, szText);
}


static bool DownloadProgress(DownloadInfo *pInfo, DWORD Pos, DWORD Max)
{
	if (Pos == 0) {
		::SendDlgItemMessage(pInfo->hDlg, IDC_MAIN_PROGRESS, PBM_SETRANGE32, 0, Max);
		pInfo->StartTime = ::GetTickCount64();
		pInfo->PrevTime = pInfo->StartTime;
		pInfo->DownloadRate = 0;
	} else {
		ULONGLONG CurTime = ::GetTickCount64();
		if (CurTime - pInfo->PrevTime >= 1000) {
			pInfo->PrevTime = CurTime;
			pInfo->DownloadRate = (DWORD)::MulDiv(Pos, 1000, (int)(CurTime - pInfo->StartTime));

			TCHAR szText[256];
			int Length = ::LoadString(g_hInst, IDS_DOWNLOADING, szText, cvLengthOf(szText));
			::wsprintf(szText + Length, TEXT(" %d%% ( %u / %u KB) %u KB/s"),
					   ::MulDiv(Pos, 100, Max),
					   (unsigned int)((Pos + 512) / 1024), (unsigned int)((Max + 512) / 1024),
					   (unsigned int)((pInfo->DownloadRate + 512) / 1024));
			::SetDlgItemText(pInfo->hDlg, IDC_MAIN_INFO, szText);
		}
	}
	::SendDlgItemMessage(pInfo->hDlg, IDC_MAIN_PROGRESS, PBM_SETPOS, Pos, 0);

	return ::WaitForSingleObject(pInfo->hAbortEvent, 0) == WAIT_TIMEOUT;
}


static DWORD WINAPI DownloadThread(LPVOID pParam)
{
	DownloadInfo *pInfo = static_cast<DownloadInfo*>(pParam);
	HINTERNET hInternet, hURL;

	if (::InternetAttemptConnect(0) != ERROR_SUCCESS) {
		pInfo->Error = ERR_CONNECT;
		return 1;
	}

	SetDlgItemTextFromResource(pInfo->hDlg, IDC_MAIN_INFO, IDS_OPENING);
	if (pInfo->szProxy[0] != _T('\0')) {
		hInternet = ::InternetOpen(pInfo->pUserAgent, INTERNET_OPEN_TYPE_PROXY, pInfo->szProxy, nullptr, 0);
	} else {
		hInternet = ::InternetOpen(pInfo->pUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	}
	if (hInternet == nullptr) {
		pInfo->Error = ERR_OPEN_INTERNET;
		return 1;
	}

	SetDlgItemTextFromResource(pInfo->hDlg, IDC_MAIN_INFO, IDS_CONNECTING);
	hURL = ::InternetOpenUrl(hInternet, pInfo->szURL, nullptr, 0,
							 INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hURL == nullptr) {
		::InternetCloseHandle(hInternet);
		pInfo->Error = ERR_OPEN_URL;
		return 1;
	}

	DWORD FileSize, BufferSize;
	BufferSize = sizeof(DWORD);
	if (!::HttpQueryInfo(hURL,
						 HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
						 &FileSize, &BufferSize, nullptr)) {
		::InternetCloseHandle(hURL);
		::InternetCloseHandle(hInternet);
		pInfo->Error = ERR_QUERY_SIZE;
		return 1;
	}

	if (!DownloadProgress(pInfo, 0, FileSize)) {
		::InternetCloseHandle(hURL);
		::InternetCloseHandle(hInternet);
		pInfo->Error = ERR_ABORT;
		return 1;
	}

	if (!::InternetQueryDataAvailable(hURL, &BufferSize, 0, 0))
		BufferSize = 1024;
	void *pBuffer = ::HeapAlloc(::GetProcessHeap(), 0, BufferSize);
	if (pBuffer == nullptr) {
		::InternetCloseHandle(hURL);
		::InternetCloseHandle(hInternet);
		pInfo->Error = ERR_MEMORY_ALLOC;
		return 1;
	}

	HANDLE hDstFile = ::CreateFile(pInfo->szFileName, GENERIC_WRITE, 0, nullptr,
								   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hDstFile == INVALID_HANDLE_VALUE) {
		::InternetCloseHandle(hURL);
		::InternetCloseHandle(hInternet);
		pInfo->Error = ERR_FILE_CREATE;
		return 1;
	}

	DWORD ReadSize = 0;
	while (true) {
		DWORD Size, Write;

		if (!::InternetReadFile(hURL, pBuffer, BufferSize, &Size)) {
			pInfo->Error = ERR_FILE_READ;
			break;
		}
		if (Size == 0) {
			::FlushFileBuffers(hDstFile);
			pInfo->Error = ERR_SUCCESS;
			break;
		}
		if (!::WriteFile(hDstFile, pBuffer, Size, &Write, nullptr) || Write != Size) {
			pInfo->Error = ERR_FILE_WRITE;
			break;
		}
		ReadSize += Size;
		if (!DownloadProgress(pInfo, ReadSize, FileSize)) {
			pInfo->Error = ERR_ABORT;
			break;
		}
	}

	::CloseHandle(hDstFile);

	::HeapFree(::GetProcessHeap(), 0, pBuffer);
	::InternetCloseHandle(hURL);
	::InternetCloseHandle(hInternet);

	return 0;
}


static DownloadError DecompressGZip(DownloadInfo *pInfo)
{
	char szSrcFileName[MAX_PATH];
	TCHAR szTempFileName[MAX_PATH], szDstFileName[MAX_PATH];

#ifdef UNICODE
	::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, pInfo->szFileName, -1,
						  szSrcFileName, cvLengthOf(szSrcFileName),
						  nullptr, nullptr);
#else
	::lstrcpy(szSrcFileName, pInfo->szFileName);
#endif
	::lstrcpy(szDstFileName, pInfo->szFileName);
	::PathRemoveExtension(szDstFileName);
	BOOL Exists = ::PathFileExists(szDstFileName);
	if (Exists) {
		::lstrcpy(szTempFileName, szDstFileName);
		::lstrcat(szTempFileName, TEXT(".tmp"));
	} else {
		::lstrcpy(szTempFileName, szDstFileName);
	}

	gzFile InFile = ::gzopen(szSrcFileName, "rb");
	if (InFile == nullptr)
		return ERR_FILE_OPEN;

	HANDLE hOutFile = ::CreateFile(szTempFileName, GENERIC_WRITE, 0, nullptr,
								   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hOutFile == INVALID_HANDLE_VALUE) {
		gzclose(InFile);
		return ERR_FILE_CREATE;
	}

	DownloadError Error;
	while (true) {
		BYTE Buffer[1024];

		int Length = ::gzread(InFile, Buffer, sizeof(Buffer));
		if (Length < 0) {
			Error = ERR_DECOMPRESS;
			break;
		}
		if (Length == 0) {
			::FlushFileBuffers(hOutFile);
			Error = ERR_SUCCESS;
			break;
		}
		DWORD Wrote;
		if (!::WriteFile(hOutFile, Buffer, Length, &Wrote, nullptr) || Wrote != (DWORD)Length) {
			Error = ERR_FILE_WRITE;
			break;
		}
	}

	::CloseHandle(hOutFile);
	::gzclose(InFile);

	if (Error == ERR_SUCCESS && Exists) {
		if (!::ReplaceFile(szDstFileName, szTempFileName, nullptr, 0, nullptr, nullptr))
			Error = ERR_REPLACE_FILE;
	}

	if (Error != ERR_SUCCESS)
		::DeleteFile(szTempFileName);

	return Error;
}


static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static DownloadInfo Info;

	switch (Msg) {
	case WM_INITDIALOG:
		{
			HWND hwndURL = ::GetDlgItem(hDlg, IDC_MAIN_URL);
			::SendMessage(hwndURL, CB_LIMITTEXT, MAX_URL_LENGTH - 1, 0);
			int Cur = -1;
			for (int i = 0; i < cvLengthOf(g_pURLList); i++) {
				::SendMessage(hwndURL, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g_pURLList[i]));
				if (::lstrcmpi(g_pURLList[i], g_Settings.szURL) == 0)
					Cur = i;
			}
			if (Cur < 0) {
				::SendMessage(hwndURL, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(g_Settings.szURL));
				Cur = 0;
			}
			::SendMessage(hwndURL, CB_SETCURSEL, Cur, 0);
			//::SetDlgItemText(hDlg, IDC_MAIN_URL, g_Settings.szURL);
			HDC hdc = ::GetDC(hwndURL);
			if (hdc != nullptr) {
				HGDIOBJ hOldFont =
					::SelectObject(hdc, reinterpret_cast<HFONT>(::SendMessage(hwndURL, WM_GETFONT, 0, 0)));
				int Items = (int)::SendMessage(hwndURL, CB_GETCOUNT, 0, 0);
				int MaxWidth = 0;
				for (int i = 0; i < Items; i++) {
					TCHAR szURL[MAX_URL_LENGTH];
					::SendMessage(hwndURL, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(szURL));
					SIZE size;
					::GetTextExtentPoint32(hdc, szURL, ::lstrlen(szURL), &size);
					if (size.cx > MaxWidth)
						MaxWidth = size.cx;
				}
				::SelectObject(hdc, hOldFont);
				::ReleaseDC(hwndURL, hdc);
				MaxWidth += 16 +::GetSystemMetrics(SM_CXVSCROLL);
				::SendMessage(hwndURL, CB_SETDROPPEDWIDTH, MaxWidth, 0);
			}

			::CheckDlgButton(hDlg, IDC_MAIN_USE_PROXY, g_Settings.fUseProxy);
			::SetDlgItemText(hDlg, IDC_MAIN_PROXY, g_Settings.szProxy);
			::SendDlgItemMessage(hDlg, IDC_MAIN_PROXY, EM_LIMITTEXT, MAX_PROXY_LENGTH - 1, 0);
			::EnableDlgItem(hDlg, IDC_MAIN_PROXY, g_Settings.fUseProxy);

			RECT rc;
			::GetWindowRect(hDlg, &rc);
			::SetWindowPos(hDlg, nullptr,
						   rc.left + (::GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
						   rc.top + (::GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2,
						   0, 0, SWP_NOZORDER | SWP_NOSIZE);

			::SendMessage(hDlg, WM_SETICON, ICON_BIG,
						  reinterpret_cast<LPARAM>(::LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON))));
			::SendMessage(hDlg, WM_SETICON, ICON_SMALL,
						  reinterpret_cast<LPARAM>(::LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON,
												   ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_SHARED)));
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MAIN_USE_PROXY:
			::EnableDlgItem(hDlg, IDC_MAIN_PROXY,
							::IsDlgButtonChecked(hDlg, IDC_MAIN_USE_PROXY) == BST_CHECKED);
			return TRUE;

		case IDOK:
			if (Info.hThread == nullptr) {
				::GetDlgItemText(hDlg, IDC_MAIN_URL, Info.szURL, cvLengthOf(Info.szURL));
				if (Info.szURL[0] == _T('\0'))
					break;

				LPTSTR pName = ::StrRChr(Info.szURL, nullptr, _T('/'));
				if (pName == nullptr || *(pName + 1) == _T('\0')) {
					ShowMessage(hDlg, IDS_URL_NO_FILE_NAME, 0,
								MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				::GetModuleFileName(nullptr, Info.szFileName, cvLengthOf(Info.szFileName));
				::lstrcpy(::PathFindFileName(Info.szFileName), pName);

				if (::IsDlgButtonChecked(hDlg, IDC_MAIN_USE_PROXY) == BST_CHECKED)
					::GetDlgItemText(hDlg, IDC_MAIN_PROXY, Info.szProxy, cvLengthOf(Info.szProxy));
				else
					Info.szProxy[0] = _T('\0');

				Info.pUserAgent = TEXT("Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0)");
				Info.hDlg = hDlg;
				Info.hAbortEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				Info.hThread = ::CreateThread(nullptr, 0, DownloadThread, &Info, 0, nullptr);

				SetDlgItemTextFromResource(hDlg, IDOK, IDS_ABORT);
				EnableDlgItem(hDlg, IDCANCEL, false);

				bool Quit = false;
				while (true) {
					MSG msg;

					while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
						if (msg.message == WM_QUIT) {
							::SetEvent(Info.hAbortEvent);
							::WaitForSingleObject(Info.hThread, INFINITE);
							Quit = true;
							break;
						}
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					if (::WaitForSingleObject(Info.hThread, 100) != WAIT_TIMEOUT)
						break;
				}

				::CloseHandle(Info.hThread);
				Info.hThread = nullptr;
				::CloseHandle(Info.hAbortEvent);
				Info.hAbortEvent = nullptr;

				if (Quit)
					return TRUE;

				if (Info.Error == ERR_SUCCESS) {
					SetDlgItemTextFromResource(hDlg, IDC_MAIN_INFO, IDS_DOWNLOADED);

					if (::lstrcmpi(::PathFindExtension(Info.szFileName), TEXT(".gz")) == 0) {
						/*if (ShowMessage(hDlg,
								IDS_CONFIRM_DECOMPRESS,
								IDS_CONFIRM_DECOMPRESS_CAPTION,
								MB_YESNO | MB_ICONQUESTION)==IDYES) */{
							HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
							SetDlgItemTextFromResource(hDlg, IDC_MAIN_INFO, IDS_DECOMPRESSING);
							Info.Error = DecompressGZip(&Info);
							::SetCursor(hcurOld);
							if (Info.Error == ERR_SUCCESS)
								SetDlgItemTextFromResource(hDlg, IDC_MAIN_INFO, IDS_DECOMPRESSED);
						}
					} else {
						ShowMessage(hDlg,
									IDS_NOT_GZ_FILE,
									IDS_NOT_GZ_FILE_CAPTION,
									MB_OK | MB_ICONINFORMATION);
					}
				}

				if (Info.Error != ERR_SUCCESS) {
					if (Info.Error != ERR_ABORT) {
						ShowMessage(hDlg, IDS_ERROR_FIRST + Info.Error, 0,
									MB_OK | MB_ICONEXCLAMATION);
						::SetDlgItemText(hDlg, IDC_MAIN_INFO, TEXT(""));
					} else {
						SetDlgItemTextFromResource(hDlg, IDC_MAIN_INFO, IDS_DOWNLOAD_ABORTED);
					}
				}

				SetDlgItemTextFromResource(hDlg, IDOK, IDS_DOWNLOAD);
				EnableDlgItem(hDlg, IDCANCEL, true);
			} else {
				::SetEvent(Info.hAbortEvent);
			}
			return TRUE;

		case IDCANCEL:
			if (Info.hThread != nullptr)
				return TRUE;

			::GetDlgItemText(hDlg, IDC_MAIN_URL, g_Settings.szURL, cvLengthOf(g_Settings.szURL));
			g_Settings.fUseProxy = ::IsDlgButtonChecked(hDlg, IDC_MAIN_USE_PROXY) == BST_CHECKED;
			::GetDlgItemText(hDlg, IDC_MAIN_PROXY, g_Settings.szProxy, cvLengthOf(g_Settings.szProxy));

			::EndDialog(hDlg, 0);
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					   LPTSTR pszCmdLine, int nCmdShow)
{
	g_hInst = hInstance;

	TCHAR szIniFileName[MAX_PATH];
	::GetModuleFileName(nullptr, szIniFileName, cvLengthOf(szIniFileName));
	::PathRemoveFileSpec(szIniFileName);
	::PathAppend(szIniFileName, TEXT("ConnectionViewer.ini"));
	::GetPrivateProfileString(TEXT("Settings"), TEXT("GeoIP.DatabaseURL"),
							  g_pURLList[0], g_Settings.szURL, cvLengthOf(g_Settings.szURL),
							  szIniFileName);
	g_Settings.fUseProxy =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("GeoIP.UseProxy"),
							   g_Settings.fUseProxy, szIniFileName) != 0;
	::GetPrivateProfileString(TEXT("Settings"), TEXT("GeoIP.Proxy"),
							  TEXT("0.0.0.0:8080"), g_Settings.szProxy, cvLengthOf(g_Settings.szProxy),
							  szIniFileName);

	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_PROGRESS_CLASS;
	::InitCommonControlsEx(&iccx);

	::DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), nullptr, DlgProc);

	::WritePrivateProfileString(TEXT("Settings"), TEXT("GeoIP.DatabaseURL"),
								::lstrcmp(g_Settings.szURL, g_pURLList[0]) != 0 ? g_Settings.szURL : nullptr,
								szIniFileName);
	::WritePrivateProfileString(TEXT("Settings"), TEXT("GeoIP.UseProxy"),
								g_Settings.fUseProxy ? TEXT("true") : TEXT("false"),
								szIniFileName);
	::WritePrivateProfileString(TEXT("Settings"), TEXT("GeoIP.Proxy"),
								g_Settings.szProxy,
								szIniFileName);

	return 0;
}
