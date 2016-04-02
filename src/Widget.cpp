/******************************************************************************
*                                                                             *
*    Widget.cpp                             Copyright(c) 2010-2016 itow,y.    *
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
#include "Widget.h"


namespace CV
{

Widget::Widget()
	: m_Handle(nullptr)
{
}

Widget::Widget(HWND Handle)
	: m_Handle(Handle)
{
}

Widget::~Widget()
{
	Destroy();
}

bool Widget::Create(HWND Parent, int ID)
{
	return false;
}

void Widget::Destroy()
{
	if (m_Handle != nullptr) {
		GetPosition(&m_Position);
		::DestroyWindow(m_Handle);
		m_Handle = nullptr;
	}
}

bool Widget::IsCreated() const
{
	return m_Handle != nullptr;
}

void Widget::Attach(HWND Handle)
{
	m_Handle = Handle;
}

bool Widget::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_Handle != nullptr) {
		::MoveWindow(m_Handle, Left, Top, Width, Height, TRUE);
	} else {
		m_Position.Left = Left;
		m_Position.Top = Top;
		m_Position.Width = Width;
		m_Position.Height = Height;
	}
	return true;
}

bool Widget::SetPosition(const Position &Pos)
{
	return SetPosition(Pos.Left, Pos.Top, Pos.Width, Pos.Height);
}

bool Widget::GetPosition(Position *pPos) const
{
	if (m_Handle != nullptr) {
		RECT rc;

		::GetWindowRect(m_Handle, &rc);
		HWND Parent = ::GetAncestor(m_Handle, GA_PARENT);
		if (Parent != nullptr) {
			::MapWindowPoints(nullptr, Parent, reinterpret_cast<POINT*>(&rc), 2);
		} else {
			WINDOWPLACEMENT wp;

			wp.length = sizeof(WINDOWPLACEMENT);
			if (::GetWindowPlacement(m_Handle, &wp) && wp.showCmd != SW_SHOWNORMAL) {
				if ((GetExStyle() & WS_EX_TOOLWINDOW) == 0) {
					HMONITOR hMonitor = ::MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					if (::GetMonitorInfo(hMonitor, &mi)) {
						::OffsetRect(&wp.rcNormalPosition,
									 mi.rcWork.left - mi.rcMonitor.left,
									 mi.rcWork.top - mi.rcMonitor.top);
					}
				}
				rc = wp.rcNormalPosition;
			}
		}
		pPos->Left = rc.left;
		pPos->Top = rc.top;
		pPos->Width = rc.right - rc.left;
		pPos->Height = rc.bottom - rc.top;
	} else {
		*pPos = m_Position;
	}
	return true;
}

int Widget::GetWidth() const
{
	if (m_Handle != nullptr) {
		RECT rc;

		::GetWindowRect(m_Handle, &rc);
		return rc.right - rc.left;
	}
	return m_Position.Width;
}

int Widget::GetHeight() const
{
	if (m_Handle != nullptr) {
		RECT rc;

		::GetWindowRect(m_Handle, &rc);
		return rc.bottom - rc.top;
	}
	return m_Position.Height;
}

bool Widget::SetVisible(bool Visible)
{
	if (m_Handle == nullptr)
		return false;
	return ::ShowWindow(m_Handle, Visible ? SW_SHOW : SW_HIDE) != FALSE;
}

bool Widget::IsVisible() const
{
	if (m_Handle == nullptr)
		return false;
	return ::IsWindowVisible(m_Handle) != FALSE;
}

bool Widget::SetText(LPCTSTR pText)
{
	if (m_Handle == nullptr)
		return false;
	return ::SetWindowText(m_Handle, pText) != FALSE;
}

DWORD Widget::GetStyle() const
{
	if (m_Handle == nullptr)
		return 0;
	return (DWORD)::GetWindowLong(m_Handle, GWL_STYLE);
}

bool Widget::SetStyle(DWORD Style)
{
	if (m_Handle == nullptr)
		return false;
	::SetWindowLong(m_Handle, GWL_STYLE, Style);
	return true;
}

DWORD Widget::GetExStyle() const
{
	if (m_Handle == nullptr)
		return 0;
	return (DWORD)::GetWindowLong(m_Handle, GWL_EXSTYLE);
}

bool Widget::SetExStyle(DWORD ExStyle)
{
	if (m_Handle == nullptr)
		return false;
	::SetWindowLong(m_Handle, GWL_EXSTYLE, ExStyle);
	return true;
}

HFONT Widget::GetFont() const
{
	if (m_Handle == nullptr)
		return nullptr;
	return reinterpret_cast<HFONT>(::SendMessage(m_Handle, WM_GETFONT, 0, 0));
}

bool Widget::Redraw(const RECT *pRect) const
{
	if (m_Handle == nullptr)
		return false;
	return ::RedrawWindow(m_Handle, pRect, nullptr,
						  RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW) != FALSE;
}

bool Widget::Invalidate(const RECT *pRect) const
{
	if (m_Handle == nullptr)
		return false;
	return ::InvalidateRect(m_Handle, pRect, TRUE) != FALSE;
}

bool Widget::Update() const
{
	if (m_Handle == nullptr)
		return false;
	return ::UpdateWindow(m_Handle) != FALSE;
}

HWND Widget::GetHandle() const
{
	return m_Handle;
}


CustomWidget::CustomWidget()
	: m_Style(0)
	, m_ExStyle(0)
{
}

CustomWidget::~CustomWidget()
{
}

bool CustomWidget::SetVisible(bool Visible)
{
	if (m_Handle == nullptr) {
		if (Visible)
			m_Style |= WS_VISIBLE;
		else
			m_Style &= ~WS_VISIBLE;
		return true;
	}
	return Widget::SetVisible(Visible);
}

DWORD CustomWidget::GetStyle() const
{
	if (m_Handle == nullptr)
		return m_Style;
	return Widget::GetStyle();
}

bool CustomWidget::SetStyle(DWORD Style)
{
	m_Style = Style;
	return Widget::SetStyle(Style);
}

DWORD CustomWidget::GetExStyle() const
{
	if (m_Handle == nullptr)
		return m_ExStyle;
	return Widget::GetExStyle();
}

bool CustomWidget::SetExStyle(DWORD ExStyle)
{
	m_ExStyle = ExStyle;
	return Widget::SetExStyle(ExStyle);
}

ATOM CustomWidget::RegisterWindowClass(LPCTSTR pClassName, UINT Style,
									   HCURSOR hCursor, HBRUSH hbrBackground, HICON hIcon)
{
	WNDCLASS wc;

	wc.style = Style;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = ::GetModuleHandle(nullptr);
	wc.hIcon = hIcon;
	wc.hCursor = hCursor;
	wc.hbrBackground = hbrBackground;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = pClassName;
	return ::RegisterClass(&wc);
}

LRESULT CALLBACK CustomWidget::WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	CustomWidget *pThis;

	if (Msg == WM_CREATE) {
		LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);

		pThis = static_cast<CustomWidget*>(pcs->lpCreateParams);
		pThis->m_Handle = hwnd;
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	} else {
		pThis = reinterpret_cast<CustomWidget*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (pThis == nullptr)
			return ::DefWindowProc(hwnd, Msg, wParam, lParam);
		if (Msg == WM_DESTROY) {
			pThis->GetPosition(&pThis->m_Position);
		} else if (Msg == WM_NCDESTROY) {
			pThis->OnMessage(hwnd, Msg, wParam, lParam);
			pThis->m_Handle = nullptr;
			return 0;
		}
	}

	return pThis->OnMessage(hwnd, Msg, wParam, lParam);
}

LRESULT CustomWidget::OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_DESTROY:
		m_Style = GetStyle();
		m_ExStyle = GetExStyle();
		return 0;
	}
	return ::DefWindowProc(hwnd, Msg, wParam, lParam);
}

bool CustomWidget::Create(LPCTSTR pClassName, HWND Parent, HMENU hmenu)
{
	if (m_Handle != nullptr)
		Destroy();

	return ::CreateWindowEx(m_ExStyle, pClassName, TEXT(""), m_Style,
							m_Position.Left, m_Position.Top,
							m_Position.Width, m_Position.Height,
							Parent, hmenu, ::GetModuleHandle(nullptr), this) != nullptr;
}

}	// namespace AFE
