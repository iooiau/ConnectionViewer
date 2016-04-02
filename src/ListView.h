/******************************************************************************
*                                                                             *
*    ListView.h                             Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_LIST_VIEW_H
#define CV_LIST_VIEW_H


#include <vector>
#include "Widget.h"
#include "Theme.h"
#include "ToolTip.h"


namespace CV
{

cvAbstractClass(ListView) : public CustomWidget
{
public:
	enum ColumnAlign
	{
		COLUMN_ALIGN_LEFT,
		COLUMN_ALIGN_RIGHT,
		COLUMN_ALIGN_CENTER
	};

	enum { MAX_COLUMN_TEXT = 64 };

	struct ColumnInfo
	{
		int ID;
		TCHAR szText[MAX_COLUMN_TEXT];
		ColumnAlign Align;
		bool Visible;
		int Width;
	};

	enum SelectionType
	{
		SELECTION_NONE,
		SELECTION_SINGLE,
		SELECTION_MULTI
	};

	cvAbstractClass(EventHandler)
	{
	public:
		virtual ~EventHandler();
		virtual void OnItemLDoubleClick(ListView *pListView, int x, int y) {}
		virtual void OnItemRButtonDown(ListView *pListView, int x, int y) {}
		virtual void OnItemRButtonUp(ListView *pListView, int x, int y) {}
		virtual void OnHeaderRButtonDown(ListView *pListView, int x, int y) {}
		virtual void OnHeaderRButtonUp(ListView *pListView, int x, int y) {}
		virtual void OnSelChanged(ListView *pListView) {}
	};

	static bool Initialize();

	ListView();
	virtual ~ListView();
	bool Create(HWND Parent, int ID = 0) override;
	void SetColors(COLORREF TextColor, COLORREF GridColor,
				   COLORREF BackColor1, COLORREF BackColor2,
				   COLORREF SelTextColor, COLORREF SelBackColor);
	void GetDefaultFont(LOGFONT *pFont) const;
	bool SetFont(const LOGFONT &Font);
	void ShowGrid(bool Show);
	int NumColumns() const;
	bool GetColumnInfo(int Index, ColumnInfo *pInfo) const;
	int GetColumnIndex(int ID) const;
	int GetColumnWidth(int ID) const;
	bool SetColumnWidth(int ID, int Width);
	bool IsColumnVisible(int ID) const;
	bool SetColumnVisible(int ID, bool Visible);
	void GetColumnOrder(std::vector<int> *pOrder) const;
	bool SetColumnOrder(const std::vector<int> &Order);
	void GetSortOrder(std::vector<int> *pOrder) const;
	bool SetSortOrder(const std::vector<int> &Order, bool Ascending = true);
	bool IsSortAscending() const;
	void SetEventHandler(EventHandler *pEventHandler);
	bool SelectItem(int Item);
	int GetSelectedItem() const;

	virtual int NumItems() const = 0;
	virtual bool GetItemText(int Row, int Column, LPTSTR pText, int MaxTextLength) const = 0;
	virtual bool GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const;
	virtual LPCTSTR GetColumnIDName(int ID) const = 0;
	virtual bool IsItemSelected(int Item) const;

	void GetItemRect(int Index, RECT *pRect) const;
	void GetSubItemRect(int Index, int Column, RECT *pRect) const;
	void RedrawItem(int Index) const;

protected:
	enum { MAX_ITEM_TEXT = 256 };

	enum PartType
	{
		PART_NOWHERE,
		PART_HEADER,
		PART_HEADER_SEPARATOR,
		PART_ITEM
	};

	LRESULT OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;
	void OnMouseMove(int x, int y, bool ForceUpdate = false);
	void Draw(HDC hdc, const RECT &PaintRect);
	void CalcItemMetrics();
	int CalcWidth() const;
	void Scroll(int XOffset, int YOffset);
	void SetScrollBar();
	void AdjustScrollPos(bool Update);
	bool HitTest(int x, int y, PartType *pPart, int *pItem) const;
	void RedrawHeader() const;
	DWORD GetDrawTextAlignFlag(ColumnAlign Align) const;

	virtual void DrawItem(HDC hdc, int Row, const ListView::ColumnInfo &Column,
						  const RECT &rcBound, const RECT &rcItem);
	virtual bool DrawItemBackground(HDC hdc, int Row, const RECT &rcBound);
	virtual void AdjustItemTextRect(HDC hdc, int Row, const ListView::ColumnInfo &Column,
									const RECT &rcBound, RECT *pRect);
	virtual int CalcSubItemWidth(HDC hdc, int Row, const ListView::ColumnInfo &Column);
	virtual bool OnSelChange(int OldSel, int NewSel);
	virtual bool SortItems() = 0;

	static const LPCTSTR m_pClassName;
	static ATOM m_ClassAtom;

	HFONT m_Font;
	int m_FontHeight;
	RECT m_HeaderMargin;
	int m_HeaderHeight;
	RECT m_ItemMargin;
	int m_ItemHeight;
	bool m_ScrollBeyondBottom;
	std::vector<ColumnInfo> m_ColumnList;
	std::vector<int> m_SortOrder;
	bool m_SortAscending;
	COLORREF m_ItemBackColor[2];
	COLORREF m_ItemTextColor;
	COLORREF m_SelItemBackColor;
	COLORREF m_SelItemTextColor;
	COLORREF m_GridColor;
	bool m_ShowGrid;
	int m_ScrollLeft;
	int m_ScrollTop;
	ThemePainter m_ThemePainter;
	InPlaceToolTip m_ToolTip;
	EventHandler *m_pEventHandler;
	SelectionType m_SelectionType;
	int m_SelectedItem;
	PartType m_HotPart;
	int m_HotItem;
	int m_HotSubItem;
	int m_HotColumn;
	int m_PushedColumn;
	PartType m_DraggingPart;
	int m_DraggingItem;
	POINT m_DragStartPos;
	int m_DragStartColumnWidth;
};

}	// namespace CV


#endif	// ndef CV_LIST_VIEW_H
