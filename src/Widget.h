/******************************************************************************
*                                                                             *
*    Widget.h                               Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_WIDGET_H
#define CV_WIDGET_H


namespace CV
{

class Widget
{
public:
	struct Position
	{
		int Left;
		int Top;
		int Width;
		int Height;

		Position() : Left(0), Top(0), Width(0), Height(0) {}
		Position(int l, int t, int w, int h)  : Left(l), Top(t), Width(w), Height(h) {}
	};

	Widget();
	Widget(HWND Handle);
	virtual ~Widget();
	virtual bool Create(HWND Parent, int ID = 0);
	virtual void Destroy();
	bool IsCreated() const;
	void Attach(HWND Handle);
	bool SetPosition(int Left, int Top, int Width, int Height);
	bool SetPosition(const Position &Pos);
	bool GetPosition(Position *pPos) const;
	int GetWidth() const;
	int GetHeight() const;
	virtual bool SetVisible(bool Visible);
	bool IsVisible() const;
	virtual bool SetText(LPCTSTR pText);
	virtual DWORD GetStyle() const;
	virtual bool SetStyle(DWORD Style);
	virtual DWORD GetExStyle() const;
	virtual bool SetExStyle(DWORD ExStyle);
	virtual HFONT GetFont() const;
	bool Redraw(const RECT *pRect = nullptr) const;
	bool Invalidate(const RECT *pRect = nullptr) const;
	bool Update() const;
	HWND GetHandle() const;

protected:
	HWND m_Handle;
	Position m_Position;
};

cvAbstractClass(CustomWidget) : public Widget
{
public:
	CustomWidget();
	virtual ~CustomWidget();
	virtual bool Create(HWND Parent, int ID = 0) = 0;
	virtual bool SetVisible(bool Visible) override;
	DWORD GetStyle() const override;
	bool SetStyle(DWORD Style) override;
	DWORD GetExStyle() const override;
	bool SetExStyle(DWORD ExStyle) override;

protected:
	static ATOM RegisterWindowClass(LPCTSTR pClassName, UINT Style = 0,
	HCURSOR hCursor = nullptr, HBRUSH hbrBackground = nullptr, HICON hIcon = nullptr);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	bool Create(LPCTSTR pClassName, HWND Parent = nullptr, HMENU hmenu = nullptr);

	DWORD m_Style;
	DWORD m_ExStyle;
};

}	// namespace CV


#endif	// ndef CV_WIDGET_H
