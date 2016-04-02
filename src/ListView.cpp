/******************************************************************************
*                                                                             *
*    ListView.cpp                           Copyright(c) 2010-2016 itow,y.    *
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
#include "ListView.h"


namespace CV
{

static void SubtractMargin(const RECT &Rect, const RECT &Margin, RECT *pResult)
{
	pResult->left = Rect.left + Margin.left;
	pResult->top = Rect.top + Margin.top;
	pResult->right = Rect.right - Margin.right;
	pResult->bottom = Rect.bottom - Margin.bottom;
}


const LPCTSTR ListView::m_pClassName = TEXT("ConnectionViewer ListView");
ATOM ListView::m_ClassAtom = 0;

bool ListView::Initialize()
{
	if (m_ClassAtom == 0) {
		m_ClassAtom = RegisterWindowClass(m_pClassName, CS_HREDRAW | CS_DBLCLKS);
		if (m_ClassAtom == 0)
			return false;
	}
	return true;
}

ListView::ListView()
	: m_Font(nullptr)
	, m_ScrollBeyondBottom(false)
	, m_SortAscending(true)
	, m_ShowGrid(true)
	, m_pEventHandler(nullptr)
	, m_SelectionType(SELECTION_SINGLE)
	, m_SelectedItem(-1)
{
	m_Style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL | WS_BORDER;
	//m_ExStyle=WS_EX_CLIENTEDGE;

	m_HeaderMargin.left = 4;
	m_HeaderMargin.right = 4;
	m_HeaderMargin.top = 4;
	m_HeaderMargin.bottom = 4;

	m_ItemMargin.left = 4;
	m_ItemMargin.right = 4;
	m_ItemMargin.top = 1;
	m_ItemMargin.bottom = 1;

	m_ItemBackColor[0] = RGB(255, 255, 255);
	m_ItemBackColor[1] = RGB(240, 240, 240);
	m_ItemTextColor = RGB(0, 0, 0);
	m_SelItemBackColor = RGB(0, 0, 128);
	m_SelItemTextColor = RGB(255, 255, 255);
	m_GridColor = RGB(224, 224, 224);
}

ListView::~ListView()
{
	Destroy();

	if (m_Font != nullptr)
		::DeleteObject(m_Font);
}

bool ListView::Create(HWND Parent, int ID)
{
	if (!Initialize())
		return false;

	return CustomWidget::Create(MAKEINTATOM(m_ClassAtom), Parent,
								reinterpret_cast<HMENU>(ID));
}

void ListView::SetColors(COLORREF TextColor, COLORREF GridColor,
						 COLORREF BackColor1, COLORREF BackColor2,
						 COLORREF SelTextColor, COLORREF SelBackColor)
{
	m_ItemTextColor = TextColor;
	m_GridColor = GridColor;
	m_ItemBackColor[0] = BackColor1;
	m_ItemBackColor[1] = BackColor2;
	m_SelItemBackColor = SelBackColor;
	m_SelItemTextColor = SelTextColor;
	if (m_Handle != nullptr)
		Invalidate();
}

void ListView::GetDefaultFont(LOGFONT *pFont) const
{
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), pFont);
}

bool ListView::SetFont(const LOGFONT &Font)
{
	HFONT hFont = ::CreateFontIndirect(&Font);
	if (hFont == nullptr)
		return false;
	if (m_Font != nullptr)
		::DeleteObject(m_Font);
	m_Font = hFont;
	if (m_Handle != nullptr) {
		CalcItemMetrics();
		SetScrollBar();
		AdjustScrollPos(false);
		Invalidate();
	}
	return true;
}

void ListView::ShowGrid(bool Show)
{
	if (m_ShowGrid != Show) {
		m_ShowGrid = Show;
		if (m_Handle != nullptr)
			Invalidate();
	}
}

int ListView::NumColumns() const
{
	return (int)m_ColumnList.size();
}

bool ListView::GetColumnInfo(int Index, ColumnInfo *pInfo) const
{
	if (Index < 0 || (size_t)Index >= m_ColumnList.size())
		return false;
	*pInfo = m_ColumnList[Index];
	return true;
}

int ListView::GetColumnIndex(int ID) const
{
	for (size_t i = 0; i < m_ColumnList.size(); i++) {
		if (m_ColumnList[i].ID == ID)
			return (int)i;
	}
	return -1;
}

int ListView::GetColumnWidth(int ID) const
{
	int Index = GetColumnIndex(ID);
	if (Index < 0)
		return 0;

	return m_ColumnList[Index].Width;
}

bool ListView::SetColumnWidth(int ID, int Width)
{
	if (Width < 0)
		return false;

	int Index = GetColumnIndex(ID);
	if (Index < 0)
		return false;

	if (m_ColumnList[Index].Width != Width) {
		m_ColumnList[Index].Width = Width;
		if (m_Handle != nullptr) {
			SetScrollBar();
			AdjustScrollPos(false);
			Invalidate();
		}
	}

	return true;
}

bool ListView::IsColumnVisible(int ID) const
{
	int Index = GetColumnIndex(ID);
	if (Index < 0)
		return 0;

	return m_ColumnList[Index].Visible;
}

bool ListView::SetColumnVisible(int ID, bool Visible)
{
	int Index = GetColumnIndex(ID);
	if (Index < 0)
		return false;

	if (m_ColumnList[Index].Visible != Visible) {
		m_ColumnList[Index].Visible = Visible;
		if (m_Handle != nullptr) {
			SetScrollBar();
			AdjustScrollPos(false);
			Invalidate();
		}
	}

	return true;
}

void ListView::GetColumnOrder(std::vector<int> *pOrder) const
{
	pOrder->resize(m_ColumnList.size());
	for (size_t i = 0; i < m_ColumnList.size(); i++) {
		(*pOrder)[i] = m_ColumnList[i].ID;
	}
}

bool ListView::SetColumnOrder(const std::vector<int> &Order)
{
	if (Order.size() != m_ColumnList.size())
		return false;
	for (size_t i = 0; i < Order.size(); i++) {
		size_t j;
		for (j = 0; j < m_ColumnList.size(); j++) {
			if (m_ColumnList[j].ID == Order[i])
				break;
		}
		if (j == m_ColumnList.size())
			return false;
		for (j = 0; j < i; j++) {
			if (Order[j] == Order[i])
				return false;
		}
	}

	std::vector<ColumnInfo> ColumnList;
	ColumnList.reserve(Order.size());
	for (size_t i = 0; i < Order.size(); i++) {
		ColumnList.push_back(m_ColumnList[GetColumnIndex(Order[i])]);
	}
	m_ColumnList.swap(ColumnList);

	if (m_Handle != nullptr)
		Invalidate();

	return true;
}

void ListView::GetSortOrder(std::vector<int> *pOrder) const
{
	*pOrder = m_SortOrder;
}

bool ListView::SetSortOrder(const std::vector<int> &Order, bool Ascending)
{
	for (size_t i = 0; i < Order.size(); i++) {
		for (size_t j = i + 1; j < Order.size(); j++) {
			if (Order[j] == Order[i])
				return false;
		}
	}
	m_SortOrder = Order;
	m_SortAscending = Ascending;
	return true;
}

bool ListView::IsSortAscending() const
{
	return m_SortAscending;
}

void ListView::SetEventHandler(EventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

int ListView::GetSelectedItem() const
{
	if (m_SelectedItem >= NumItems())
		return -1;
	return m_SelectedItem;
}

bool ListView::SelectItem(int Item)
{
	if (m_SelectedItem != Item) {
		if (Item < 0 || Item >= NumItems())
			return false;
		if (!OnSelChange(m_SelectedItem, Item))
			return false;

		int OldSelItem = m_SelectedItem;
		m_SelectedItem = Item;
		if (OldSelItem >= 0)
			RedrawItem(OldSelItem);
		RedrawItem(Item);
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnSelChanged(this);
	}
	return true;
}

bool ListView::GetItemLongText(int Row, int Column, LPTSTR pText, int MaxTextLength) const
{
	return GetItemText(Row, Column, pText, MaxTextLength);
}

bool ListView::IsItemSelected(int Item) const
{
	return Item == m_SelectedItem;
}

LRESULT ListView::OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_CREATE:
		{
			::BufferedPaintInit();

			if (m_Font == nullptr) {
				LOGFONT lf;
				GetDefaultFont(&lf);
				m_Font = ::CreateFontIndirect(&lf);
			}

			CalcItemMetrics();

			m_ScrollLeft = 0;
			m_ScrollTop = 0;

			m_ThemePainter.Open(hwnd, L"HEADER");

			m_ToolTip.Create(hwnd);
			m_ToolTip.SetFont(m_Font);

			m_SelectedItem = -1;
			m_HotPart = PART_NOWHERE;
			m_HotItem = -1;
			m_HotSubItem = -1;
			m_HotColumn = -1;
			m_PushedColumn = -1;
			m_DraggingPart = PART_NOWHERE;
			m_DraggingItem = -1;
		}
		return 0;

	case WM_SIZE:
		SetScrollBar();
		AdjustScrollPos(true);
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			PartType Part;
			int Item;

			if (HitTest(x, y, &Part, &Item)) {
				if (Part == PART_HEADER) {
					::SetCapture(hwnd);
					::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
					m_DraggingPart = Part;
					m_DraggingItem = Item;
					m_PushedColumn = Item;
					m_HotColumn = -1;
					RedrawHeader();
				} else if (Part == PART_HEADER_SEPARATOR) {
					::SetCapture(hwnd);
					::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
					m_DraggingPart = Part;
					m_DraggingItem = Item;
					m_DragStartPos.x = x;
					m_DragStartPos.y = y;
					m_DragStartColumnWidth = m_ColumnList[Item].Width;
				} else if (Part == PART_ITEM) {
					if (m_SelectionType == SELECTION_SINGLE) {
						SelectItem(Item);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd) {
			if (m_DraggingPart == PART_HEADER) {
				if (m_PushedColumn >= 0) {
					int ColumnID = m_ColumnList[m_PushedColumn].ID;

					if (m_SortOrder.size() > 0 && m_SortOrder[0] == ColumnID) {
						m_SortAscending = !m_SortAscending;
					} else {
						std::vector<int>::iterator i = m_SortOrder.begin();
						for (i++; i != m_SortOrder.end(); i++) {
							if (*i == ColumnID) {
								m_SortOrder.erase(i);
								break;
							}
						}
						m_SortOrder.insert(m_SortOrder.begin(), ColumnID);
						m_SortAscending = true;
					}
					if (SortItems())
						Redraw();
				}
			}
			::ReleaseCapture();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			PartType Part;
			int Item;

			if (HitTest(x, y, &Part, &Item)) {
				if (Part == PART_ITEM) {
					if (SelectItem(Item)) {
						if (m_pEventHandler != nullptr)
							m_pEventHandler->OnItemLDoubleClick(this, x, y);
					}
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			PartType Part;
			int Item;

			if (HitTest(x, y, &Part, &Item)) {
				if (Part == PART_HEADER || Part == PART_HEADER_SEPARATOR) {
					if (m_pEventHandler != nullptr) {
						if (Msg == WM_RBUTTONDOWN)
							m_pEventHandler->OnHeaderRButtonDown(this, x, y);
						else
							m_pEventHandler->OnHeaderRButtonUp(this, x, y);
					}
				} else if (Part == PART_ITEM) {
					if (SelectItem(Item)) {
						if (m_pEventHandler != nullptr) {
							if (Msg == WM_RBUTTONDOWN)
								m_pEventHandler->OnItemRButtonDown(this, x, y);
							else
								m_pEventHandler->OnItemRButtonUp(this, x, y);
						}
					}
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd && LOWORD(lParam) == HTCLIENT) {
			return TRUE;
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_PushedColumn >= 0) {
			m_PushedColumn = -1;
			RedrawHeader();
		}
		m_DraggingPart = PART_NOWHERE;
		m_DraggingItem = -1;
		return 0;

	case WM_MOUSELEAVE:
		m_HotPart = PART_NOWHERE;
		m_HotItem = -1;
		m_HotSubItem = -1;
		m_ToolTip.Hide();
		if (m_HotColumn >= 0) {
			m_HotColumn = -1;
			RedrawHeader();
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			BP_PAINTPARAMS Params = {sizeof(BP_PAINTPARAMS), 0, nullptr, nullptr};
			HDC hdcBuffer;
			HPAINTBUFFER hBuffer = ::BeginBufferedPaint(
				ps.hdc, &ps.rcPaint,
				BPBF_COMPATIBLEBITMAP/*BPBF_TOPDOWNDIB*/, &Params, &hdcBuffer);
			if (hBuffer != nullptr) {
				Draw(hdcBuffer, ps.rcPaint);
				::BufferedPaintSetAlpha(hBuffer, &ps.rcPaint, 255);
				::EndBufferedPaint(hBuffer, TRUE);
			} else {
				Draw(ps.hdc, ps.rcPaint);
			}
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		{
			int Pos = m_ScrollTop;

			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
			::GetScrollInfo(hwnd, SB_VERT, &si);
			const int Max = max(si.nMax + 1 - si.nPage, 0);

			if (Msg == WM_VSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_TOP:		Pos = si.nMin;		break;
				case SB_BOTTOM:		Pos = Max;			break;
				case SB_LINEUP:		Pos--;				break;
				case SB_LINEDOWN:	Pos++;				break;
				case SB_PAGEUP:		Pos -= si.nPage;	break;
				case SB_PAGEDOWN:	Pos += si.nPage;	break;
				case SB_THUMBTRACK:	Pos = si.nTrackPos;	break;
				}
			} else {
				Pos -= GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			}
			if (Pos < si.nMin)
				Pos = si.nMin;
			else if (Pos > Max)
				Pos = Max;
			if (m_ScrollTop != Pos)
				Scroll(0, Pos - m_ScrollTop);
		}
		return 0;

	case WM_HSCROLL:
	case WM_MOUSEHWHEEL:
		{
			int Pos = m_ScrollLeft;

			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
			::GetScrollInfo(hwnd, SB_HORZ, &si);
			const int Max = max(si.nMax + 1 - si.nPage, 0);

			if (Msg == WM_HSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LEFT:		Pos = si.nMin;		break;
				case SB_RIGHT:		Pos = Max;			break;
				case SB_LINELEFT:	Pos--;				break;
				case SB_LINERIGHT:	Pos++;				break;
				case SB_PAGELEFT:	Pos -= si.nPage;	break;
				case SB_PAGERIGHT:	Pos += si.nPage;	break;
				case SB_THUMBTRACK:	Pos = si.nTrackPos;	break;
				}
			} else {
				Pos += GET_WHEEL_DELTA_WPARAM(wParam) * m_FontHeight / WHEEL_DELTA;
			}
			if (Pos < si.nMin)
				Pos = si.nMin;
			else if (Pos > Max)
				Pos = Max;
			if (m_ScrollLeft != Pos)
				Scroll(Pos - m_ScrollLeft, 0);
		}
		return 0;

	case WM_THEMECHANGED:
		m_ThemePainter.Open(hwnd, L"HEADER");
		return 0;

	case WM_DESTROY:
		m_ToolTip.Destroy();
		return 0;

	case WM_NCDESTROY:
		m_ThemePainter.Close();
		::BufferedPaintUnInit();
		return 0;
	}

	return ::DefWindowProc(hwnd, Msg, wParam, lParam);
}

void ListView::OnMouseMove(int x, int y, bool ForceUpdate)
{
	if (::GetCapture() != m_Handle) {
		LPCTSTR pszCursor = IDC_ARROW;
		PartType Part;
		int Item, HotSubItem = -1;

		if (HitTest(x, y, &Part, &Item)) {
			if (Part == PART_HEADER_SEPARATOR)
				pszCursor = IDC_SIZEWE;
		}
		::SetCursor(::LoadCursor(nullptr, pszCursor));

		if (Part == PART_ITEM) {
			int Width = -m_ScrollLeft;
			for (size_t i = 0; i < m_ColumnList.size(); i++) {
				const ColumnInfo &Column = m_ColumnList[i];

				if (Column.Visible) {
					if (x >= Width && x < Width + Column.Width) {
						HotSubItem = (int)i;
						break;
					}
					Width += Column.Width;
				}
			}
		}

		if (ForceUpdate || m_HotPart != Part || m_HotItem != Item || m_HotSubItem != HotSubItem) {
			bool ShowToolTip = false;

			if (Part == PART_ITEM) {
				if (HotSubItem >= 0) {
					const ColumnInfo &Column = m_ColumnList[HotSubItem];
					TCHAR szText[MAX_ITEM_TEXT];

					GetItemText(Item, Column.ID, szText, cvLengthOf(szText));
					if (szText[0] != '\0') {
						HDC hdc = ::GetDC(m_Handle);
						HFONT hfontOld = static_cast<HFONT>(::SelectObject(hdc, m_Font));
						RECT rcBound, rcItem, rcText;
						GetSubItemRect(Item, HotSubItem, &rcBound);
						SubtractMargin(rcBound, m_ItemMargin, &rcItem);
						rcText = rcItem;
						AdjustItemTextRect(hdc, Item, Column, rcBound, &rcText);
						int ItemWidth = CalcSubItemWidth(hdc, Item, Column);
						::SelectObject(hdc, hfontOld);
						::ReleaseDC(m_Handle, hdc);
						if (rcItem.left + ItemWidth > rcText.right) {
							m_ToolTip.Show(rcText, szText);
							ShowToolTip = true;
						}
					}
				}
			}
			if (!ShowToolTip)
				m_ToolTip.Hide();

			int HotColumn;
			if (Part == PART_HEADER || Part == PART_HEADER_SEPARATOR)
				HotColumn = Item;
			else
				HotColumn = -1;
			if (HotColumn != m_HotColumn) {
				m_HotColumn = HotColumn;
				RedrawHeader();
			}

			m_HotPart = Part;
			m_HotItem = Item;
			m_HotSubItem = HotSubItem;
		}

		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_Handle;
		::TrackMouseEvent(&tme);
	} else {
		if (m_DraggingPart == PART_HEADER) {
			PartType Part;
			int Item;

			bool Pushed = HitTest(x, y, &Part, &Item)
						  && Part == PART_HEADER && Item == m_DraggingItem;
			if ((Pushed && m_PushedColumn < 0) || (!Pushed && m_PushedColumn >= 0)) {
				m_PushedColumn = Pushed ? m_DraggingItem : -1;
				RedrawHeader();
			}
		} else if (m_DraggingPart == PART_HEADER_SEPARATOR) {
			if (m_DraggingItem >= 0 && (size_t)m_DraggingItem < m_ColumnList.size()) {
				ColumnInfo &Column = m_ColumnList[m_DraggingItem];
				int Width = m_DragStartColumnWidth + (x - m_DragStartPos.x);
				if (Width < 16 + m_ItemMargin.left + m_ItemMargin.right)
					Width = 16 + m_ItemMargin.left + m_ItemMargin.right;
				if (Column.Width != Width)
					SetColumnWidth(Column.ID, Width);
			}
		}
	}
}

DWORD ListView::GetDrawTextAlignFlag(ColumnAlign Align) const
{
	if (Align == COLUMN_ALIGN_RIGHT)
		return DT_RIGHT;
	if (Align == COLUMN_ALIGN_CENTER)
		return DT_CENTER;
	return DT_LEFT;
}

void ListView::Draw(HDC hdc, const RECT &PaintRect)
{
	HFONT hfontOld = static_cast<HFONT>(::SelectObject(hdc, m_Font));
	int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	RECT rcClient, rcBound, rcItem;
	::GetClientRect(m_Handle, &rcClient);

	if (PaintRect.top < m_HeaderHeight) {
		rcBound.left = -m_ScrollLeft;
		rcBound.top = 0;
		rcBound.bottom = rcBound.top + m_HeaderHeight;
		for (size_t i = 0; i < m_ColumnList.size(); i++) {
			const ColumnInfo &Column = m_ColumnList[i];

			if (Column.Visible) {
				const bool Sorted = m_SortOrder.size() > 0 && Column.ID == m_SortOrder[0];
				const bool Pushed = m_PushedColumn == (int)i;
				COLORREF TextColor = ::GetSysColor(COLOR_WINDOWTEXT), OldTextColor;

				rcBound.right = rcBound.left + Column.Width;
				if (m_ThemePainter.IsOpen()) {
					int StateID = Pushed ? (Sorted ? HIS_SORTEDPRESSED : HIS_PRESSED) :
								  m_HotColumn == (int)i ?
								  (Sorted ? HIS_SORTEDHOT : HIS_HOT) :
								  (Sorted ? HIS_SORTEDNORMAL : HIS_NORMAL);
					m_ThemePainter.DrawBackground(hdc, HP_HEADERITEM, StateID, rcBound);
					m_ThemePainter.GetColor(HP_HEADERITEM, StateID, TMT_TEXTCOLOR, &TextColor);
					if (Sorted) {
						SIZE sz;
						if (m_ThemePainter.GetPartSize(hdc, HP_HEADERSORTARROW,
													   m_SortAscending ? HSAS_SORTEDUP : HSAS_SORTEDDOWN,
													   TS_TRUE, &sz)) {
							RECT rc;
							rc.left = rcBound.left + (Column.Width - sz.cx) / 2;
							rc.top = rcBound.top; //+(m_HeaderHeight-sz.cy)/2;
							rc.right = rc.left + sz.cx;
							rc.bottom = rc.top + sz.cy;
							m_ThemePainter.DrawBackground(hdc, HP_HEADERSORTARROW,
														  m_SortAscending ? HSAS_SORTEDUP : HSAS_SORTEDDOWN,
														  rc);
						}
					}
				} else {
					::DrawFrameControl(hdc, &rcBound, DFC_BUTTON,
									   DFCS_BUTTONPUSH | (Pushed ? DFCS_PUSHED : 0));
				}
				SubtractMargin(rcBound, m_HeaderMargin, &rcItem);
				/*
				TCHAR szText[MAX_COLUMN_TEXT+3];
				::lstrcpy(szText, Column.szText);
				if (Sorted && !m_ThemePainter.IsOpen()) {
					::lstrcat(szText, m_SortAscending ? TEXT(" £") : TEXT(" ¥"));
				}
				*/
				OldTextColor = ::SetTextColor(hdc, TextColor);
				::DrawText(hdc, Column.szText, -1, &rcItem,
						   GetDrawTextAlignFlag(Column.Align) |
						   DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
				::SetTextColor(hdc, OldTextColor);
				rcBound.left = rcBound.right;
			}
		}
		if (PaintRect.right > rcBound.right) {
			rcBound.right = rcClient.right;
			if (m_ThemePainter.IsOpen())
				m_ThemePainter.DrawBackground(hdc, HP_HEADERITEM, HIS_NORMAL, rcBound);
			else
				::DrawFrameControl(hdc, &rcBound, DFC_BUTTON, DFCS_BUTTONPUSH);
		}
	}

	HBRUSH BackBrush[2];
	BackBrush[0] = ::CreateSolidBrush(m_ItemBackColor[0]);
	BackBrush[1] = ::CreateSolidBrush(m_ItemBackColor[1]);
	const COLORREF OldTextColor = ::GetTextColor(hdc);

	HPEN GridPen, OldPen;
	if (m_ShowGrid) {
		GridPen = ::CreatePen(PS_SOLID, 1, m_GridColor);
		OldPen = static_cast<HPEN>(::SelectObject(hdc, GridPen));
	}

	const int RowWidth = CalcWidth();

	rcBound.top = m_HeaderHeight;
	for (int i = m_ScrollTop; i < NumItems(); i++) {
		const bool Selected = IsItemSelected(i);

		rcBound.bottom = rcBound.top + m_ItemHeight;
		if (rcBound.top < PaintRect.bottom && rcBound.bottom > PaintRect.top) {
			rcBound.left = PaintRect.left;
			rcBound.right = PaintRect.right;
			if (!DrawItemBackground(hdc, i, rcBound)) {
				if (Selected) {
					if (rcBound.right > RowWidth - m_ScrollLeft)
						rcBound.right = RowWidth - m_ScrollLeft;
					if (rcBound.left < rcBound.right) {
						HBRUSH hbr = ::CreateSolidBrush(m_SelItemBackColor);
						::FillRect(hdc, &rcBound, hbr);
						::DeleteObject(hbr);
					}
					if (rcBound.right < PaintRect.right) {
						rcBound.left = rcBound.right;
						rcBound.right = PaintRect.right;
						::FillRect(hdc, &rcBound, BackBrush[i % 2]);
					}
				} else {
					::FillRect(hdc, &rcBound, BackBrush[i % 2]);
				}
			}
			if (m_ShowGrid) {
				::MoveToEx(hdc, PaintRect.left, rcBound.bottom - 1, nullptr);
				::LineTo(hdc, PaintRect.right, rcBound.bottom - 1);
			}
			rcBound.left = -m_ScrollLeft;
			for (size_t j = 0; j < m_ColumnList.size(); j++) {
				const ColumnInfo &Column = m_ColumnList[j];

				if (Column.Visible) {
					rcBound.right = rcBound.left + Column.Width;
					if (m_ShowGrid) {
						::MoveToEx(hdc, rcBound.right - 1, rcBound.top, nullptr);
						::LineTo(hdc, rcBound.right - 1, rcBound.bottom);
					}
					SubtractMargin(rcBound, m_ItemMargin, &rcItem);
					if (rcItem.right > rcItem.left) {
						::SetTextColor(hdc, Selected ? m_SelItemTextColor : m_ItemTextColor);
						DrawItem(hdc, i, Column, rcBound, rcItem);
					}
					rcBound.left = rcBound.right;
				}
			}
		}
		rcBound.top = rcBound.bottom;
	}

	if (PaintRect.bottom > rcBound.bottom) {
		rcBound.left = PaintRect.left;
		rcBound.right = PaintRect.right;
		rcBound.bottom = PaintRect.bottom;
		::FillRect(hdc, &rcBound, BackBrush[0]);
	}

	if (m_ShowGrid) {
		::SelectObject(hdc, OldPen);
		::DeleteObject(GridPen);
	}
	::DeleteObject(BackBrush[0]);
	::DeleteObject(BackBrush[1]);
	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hfontOld);
}

void ListView::DrawItem(HDC hdc, int Row, const ListView::ColumnInfo &Column,
						const RECT &rcBound, const RECT &rcItem)
{
	TCHAR szText[MAX_ITEM_TEXT];

	GetItemText(Row, Column.ID, szText, cvLengthOf(szText));
	if (szText[0] != '\0') {
		RECT rcDraw = rcItem;

		::DrawText(hdc, szText, -1, &rcDraw,
				   GetDrawTextAlignFlag(Column.Align)
				   | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}

bool ListView::DrawItemBackground(HDC hdc, int Row, const RECT &rcBound)
{
	return false;
}

void ListView::AdjustItemTextRect(HDC hdc, int Row, const ListView::ColumnInfo &Column,
								  const RECT &rcBound, RECT *pRect)
{
}

int ListView::CalcSubItemWidth(HDC hdc, int Row, const ListView::ColumnInfo &Column)
{
	TCHAR szText[MAX_ITEM_TEXT];

	GetItemText(Row, Column.ID, szText, cvLengthOf(szText));
	if (szText[0] == '\0')
		return 0;
	RECT rc = {0, 0, 0, 0};
	::DrawText(hdc, szText, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
	return rc.right - rc.left;
}

bool ListView::OnSelChange(int OldSel, int NewSel)
{
	return true;
}

void ListView::CalcItemMetrics()
{
	HDC hdc = ::GetDC(m_Handle);
	HGDIOBJ OldFont = ::SelectObject(hdc, m_Font);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	::SelectObject(hdc, OldFont);
	::ReleaseDC(m_Handle, hdc);
	m_FontHeight = tm.tmHeight; //-tm.tmInternalLeading;
	m_HeaderHeight = m_FontHeight + m_HeaderMargin.top + m_HeaderMargin.bottom;
	m_ItemHeight = max(m_FontHeight + m_ItemMargin.top + m_ItemMargin.bottom, 16);
}

int ListView::CalcWidth() const
{
	int Width = 0;
	for (size_t i = 0; i < m_ColumnList.size(); i++) {
		const ColumnInfo &Column = m_ColumnList[i];

		if (Column.Visible)
			Width += Column.Width;
	}
	return Width;
}

void ListView::Scroll(int XOffset, int YOffset)
{
	if (XOffset == 0 && YOffset == 0)
		return;

	m_ScrollLeft += XOffset;
	m_ScrollTop += YOffset;
	if (XOffset != 0 && YOffset != 0) {
		Redraw();
	} else {
		RECT rc;

		::GetClientRect(m_Handle, &rc);
		if (YOffset != 0)
			rc.top += m_HeaderHeight;
		::ScrollWindowEx(m_Handle, -XOffset, -YOffset * m_ItemHeight,
						 &rc, &rc, nullptr, nullptr, SW_INVALIDATE);
	}
	SetScrollBar();
}

void ListView::SetScrollBar()
{
	RECT rcAdjust = {0, 0, 0, 0};
	::AdjustWindowRectEx(&rcAdjust, m_Style, FALSE, m_ExStyle);
	RECT rc;
	::GetWindowRect(m_Handle, &rc);
	rc.left += rcAdjust.left;
	rc.top += rcAdjust.top;
	rc.right -= rcAdjust.right;
	rc.bottom -= rcAdjust.bottom;
	rc.top += m_HeaderHeight;

	int Width = CalcWidth();
	int Height = NumItems() * m_ItemHeight;

	int ClientWidth = rc.right - rc.left, ClientHeight = rc.bottom - rc.top;
	if (m_ScrollBeyondBottom)
		Height += max(ClientHeight / m_ItemHeight - 1, 0) * m_ItemHeight;
	bool HScroll = false, VScroll = false;
	if (ClientWidth < Width) {
		HScroll = true;
		ClientHeight -= ::GetSystemMetrics(SM_CYHSCROLL);
	}
	if (ClientHeight < Height) {
		VScroll = true;
		ClientWidth -= ::GetSystemMetrics(SM_CXVSCROLL);
		if (!HScroll && ClientWidth < Width) {
			HScroll = true;
			ClientHeight -= ::GetSystemMetrics(SM_CYHSCROLL);
		}
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);

	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = Width - 1;
	si.nPage = ClientWidth;
	si.nPos = m_ScrollLeft;
	::SetScrollInfo(m_Handle, SB_HORZ, &si, TRUE);
	if (!HScroll) {
		::ShowScrollBar(m_Handle, SB_HORZ, FALSE);
	}

	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nPage = max(ClientHeight / m_ItemHeight, 1);
	si.nMin = 0;
	if (NumItems() < 1) {
		si.nMax = 0;
	} else {
		si.nMax = NumItems() - 1;
		if (m_ScrollBeyondBottom)
			si.nMax += si.nPage - 1;
	}
	si.nPos = m_ScrollTop;
	::SetScrollInfo(m_Handle, SB_VERT, &si, TRUE);
	if (!VScroll) {
		::ShowScrollBar(m_Handle, SB_VERT, FALSE);
	}

	if (::IsWindowVisible(m_Handle) && ::GetCapture() != m_Handle) {
		POINT pt;

		::GetCursorPos(&pt);
		if (::WindowFromPoint(pt) == m_Handle) {
			::ScreenToClient(m_Handle, &pt);
			::GetClientRect(m_Handle, &rc);
			if (::PtInRect(&rc, pt))
				OnMouseMove(pt.x, pt.y, true);
		}
	}
}

void ListView::AdjustScrollPos(bool Update)
{
	SCROLLINFO si;

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE;
	::GetScrollInfo(m_Handle, SB_HORZ, &si);
	const int HMax = max(si.nMax + 1 - si.nPage, 0);
	::GetScrollInfo(m_Handle, SB_VERT, &si);
	const int VMax = max(si.nMax + 1 - si.nPage, 0);
	if (m_ScrollLeft > HMax || m_ScrollTop > VMax) {
		if (Update) {
			Scroll(m_ScrollLeft > HMax ? HMax - m_ScrollLeft : 0,
				   m_ScrollTop > VMax ? VMax - m_ScrollTop : 0);
		} else {
			if (m_ScrollLeft > HMax)
				m_ScrollLeft = HMax;
			if (m_ScrollTop > VMax)
				m_ScrollTop = VMax;
			SetScrollBar();
		}
	}
}

bool ListView::HitTest(int x, int y, PartType *pPart, int *pItem) const
{
	x += m_ScrollLeft;
	if (y >= 0 && y < m_HeaderHeight) {
		int PrevVisibleItem = -1;
		int Left, Right;

		Left = 0;
		for (size_t i = 0; i < m_ColumnList.size(); i++) {
			const ColumnInfo &Column = m_ColumnList[i];

			if (Column.Visible) {
				Right = Left + Column.Width;
				if (x >= Left && x < Right) {
					if (PrevVisibleItem >= 0 && x < Left + min(Column.Width / 2, 4)) {
						*pPart = PART_HEADER_SEPARATOR;
						*pItem = (int)PrevVisibleItem;
					} else if (x >= Right - min(Column.Width / 2, 4)) {
						*pPart = PART_HEADER_SEPARATOR;
						*pItem = (int)i;
					} else {
						*pPart = PART_HEADER;
						*pItem = (int)i;
					}
					return true;
				}
				Left = Right;
				PrevVisibleItem = (int)i;
			}
		}
	} else if (y >= m_HeaderHeight) {
		int Item = (y - m_HeaderHeight) / m_ItemHeight + m_ScrollTop;

		if (Item >= 0 && Item < NumItems()) {
			*pPart = PART_ITEM;
			*pItem = Item;
			return true;
		}
	}
	*pPart = PART_NOWHERE;
	*pItem = -1;
	return false;
}

void ListView::GetItemRect(int Index, RECT *pRect) const
{
	pRect->top = (Index - m_ScrollTop) * m_ItemHeight + m_HeaderHeight;
	pRect->bottom = pRect->top + m_ItemHeight;
	pRect->left = -m_ScrollLeft;
	pRect->right = pRect->left + CalcWidth();
}

void ListView::GetSubItemRect(int Index, int Column, RECT *pRect) const
{
	pRect->top = (Index - m_ScrollTop) * m_ItemHeight + m_HeaderHeight;
	pRect->bottom = pRect->top + m_ItemHeight;
	pRect->left = -m_ScrollLeft;
	for (int i = 0; i < (int)m_ColumnList.size(); i++) {
		const ColumnInfo &Info = m_ColumnList[i];

		pRect->right = pRect->left;
		if (Info.Visible)
			pRect->right += Info.Width;
		if (i == Column)
			break;
		pRect->left = pRect->right;
	}
}

void ListView::RedrawItem(int Index) const
{
	if (m_Handle != nullptr) {
		RECT rc;

		GetItemRect(Index, &rc);
		if (rc.bottom > m_HeaderHeight)
			Redraw(&rc);
	}
}

void ListView::RedrawHeader() const
{
	RECT rc;

	::GetClientRect(m_Handle, &rc);
	rc.bottom = m_HeaderHeight;
	::InvalidateRect(m_Handle, &rc, FALSE);
}


ListView::EventHandler::~EventHandler()
{
}

}	// namespace CV
