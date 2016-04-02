/******************************************************************************
*                                                                             *
*    GraphView.cpp                          Copyright(c) 2010-2016 itow,y.    *
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
#include "GraphView.h"
#include "Utility.h"


namespace CV
{

const LPCTSTR GraphView::m_pClassName = TEXT("ConnectionViewer GraphView");
ATOM GraphView::m_ClassAtom = 0;

bool GraphView::Initialize()
{
	if (m_ClassAtom == 0) {
		m_ClassAtom = RegisterWindowClass(m_pClassName, CS_HREDRAW | CS_VREDRAW,
										  ::LoadCursor(nullptr, IDC_ARROW));
		if (m_ClassAtom == 0)
			return false;
	}
	return true;
}

GraphView::GraphView()
	: m_BackColor(RGB(0, 0, 0))
	, m_GridColor(RGB(0, 64, 128))
	, m_CaptionColor(RGB(255, 255, 255))
	, m_FillOpacity(96)
	, m_ShowGraphName(true)
	, m_GraphNameItemWidth(0)
	, m_GraphNameItemHeight(0)
	, m_pCaption(nullptr)
	, m_pEventHandler(nullptr)
{
	m_Style = WS_CHILD | WS_CLIPSIBLINGS;

	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	m_GraphNameFont = ncm.lfMessageFont;
	m_GraphNameFont.lfWeight = FW_BOLD;
}

GraphView::~GraphView()
{
	Destroy();
	DeleteAllGraphs();
	delete [] m_pCaption;
}

bool GraphView::Create(HWND Parent, int ID)
{
	if (!Initialize())
		return false;

	return CustomWidget::Create(MAKEINTATOM(m_ClassAtom), Parent,
								reinterpret_cast<HMENU>(ID));
}

void GraphView::DeleteAllGraphs()
{
	for (size_t i = 0; i < m_GraphList.size(); i++)
		delete m_GraphList[i];
	m_GraphList.clear();
}

bool GraphView::AddGraph(GraphInfo *pInfo)
{
	if (pInfo == nullptr)
		return false;
	m_GraphList.push_back(pInfo);
	return true;
}

GraphView::GraphInfo *GraphView::GetGraph(int Index)
{
	if (Index < 0 || (size_t)Index >= m_GraphList.size())
		return nullptr;
	return m_GraphList[Index];
}

const GraphView::GraphInfo *GraphView::GetGraph(int Index) const
{
	if (Index < 0 || (size_t)Index >= m_GraphList.size())
		return nullptr;
	return m_GraphList[Index];
}

void GraphView::SetColors(COLORREF BackColor, COLORREF GridColor, COLORREF CaptionColor)
{
	m_BackColor = BackColor;
	m_GridColor = GridColor;
	m_CaptionColor = CaptionColor;
	if (m_Handle != nullptr)
		Invalidate();
}

void GraphView::SetCaption(LPCTSTR pCaption)
{
	delete [] m_pCaption;
	m_pCaption = DuplicateString(pCaption);
}

void GraphView::SetEventHandler(EventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

LRESULT GraphView::OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_CREATE:
		m_Direct2D.Initialize();

		m_HotItem = -1;
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
#if 0
			BP_PAINTPARAMS Params = {sizeof(BP_PAINTPARAMS), 0, nullptr, nullptr};
			HDC hdcBuffer;
			HPAINTBUFFER hBuffer = ::BeginBufferedPaint(ps.hdc, &ps.rcPaint,
								  BPBF_COMPATIBLEBITMAP/*BPBF_TOPDOWNDIB*/, &Params, &hdcBuffer);
			if (hBuffer != nullptr) {
				Draw(hdcBuffer, ps.rcPaint);
				::BufferedPaintSetAlpha(hBuffer, &ps.rcPaint, 255);
				::EndBufferedPaint(hBuffer, TRUE);
			} else {
				Draw(ps.hdc, ps.rcPaint);
			}
#else
			Draw(ps.hdc, ps.rcPaint);
#endif
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int Item = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (Item >= 0) {
				GraphInfo *pInfo = m_GraphList[Item];
				pInfo->Visible = !pInfo->Visible;
				Redraw();
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (m_pEventHandler != nullptr) {
			m_pEventHandler->OnRButtonDown(this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_pEventHandler != nullptr) {
			m_pEventHandler->OnRButtonUp(this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			int Item = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (Item != m_HotItem) {
				/*
				RECT rc;
				if (m_HotItem>=0) {
					GetGraphNameItemRect(m_HotItem,&rc);
					::InvalidateRect(hwnd,&rc,FALSE);
				}
				if (Item>=0) {
					GetGraphNameItemRect(Item,&rc);
					::InvalidateRect(hwnd,&rc,FALSE);
				}
				*/
				m_HotItem = Item;
				//Update();
				Redraw();
			}
		}
		return 0;

	case WM_DESTROY:
		m_Direct2D.Finalize();
		return 0;
	}

	return ::DefWindowProc(hwnd, Msg, wParam, lParam);
}

static COLORREF MixColor(COLORREF Color1, COLORREF Color2, int Ratio = 128)
{
	const int Ratio2 = 255 - Ratio;
	return RGB((GetRValue(Color1) * Ratio + GetRValue(Color2) * Ratio2) / 255,
			   (GetGValue(Color1) * Ratio + GetGValue(Color2) * Ratio2) / 255,
			   (GetBValue(Color1) * Ratio + GetBValue(Color2) * Ratio2) / 255);
}

void GraphView::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rc;
	::GetClientRect(m_Handle, &rc);

	m_Direct2D.BeginDraw(hdc, rc);
	{

		m_Direct2D.Clear(GetRValue(m_BackColor),
						 GetGValue(m_BackColor),
						 GetBValue(m_BackColor));

		Direct2D::Brush GridBrush;
		m_Direct2D.CreateSolidBrush(&GridBrush,
									GetRValue(m_GridColor),
									GetGValue(m_GridColor),
									GetBValue(m_GridColor));
		for (int i = 0; i <= 10; i++) {
			float y = (float)((rc.bottom - 1) * i) / 10.0f;
			m_Direct2D.DrawLine(0.0f, y, (float)(rc.right - 1), y, GridBrush);
		}
		for (int i = 0; i <= 10; i++) {
			float x = (float)((rc.right - 1) * i) / 10.0f;
			m_Direct2D.DrawLine(x, 0.0f, x, (float)(rc.bottom - 1), GridBrush);
		}

		Direct2D::Font Font;
		m_Direct2D.CreateFont(&Font,
							  m_GraphNameFont.lfFaceName,
							  abs(m_GraphNameFont.lfHeight),
							  m_GraphNameFont.lfWeight);

		if (m_ShowGraphName) {
			if (m_GraphNameItemWidth == 0 || m_GraphNameItemHeight == 0) {
				int MaxWidth = 0, MaxHeight = 0;
				for (size_t i = 0; i < m_GraphList.size(); i++) {
					const GraphInfo &Graph = *m_GraphList[i];
					SIZE Size;

					m_Direct2D.GetTextSize(Graph.szName, Font, &Size);
					if (Size.cx > MaxWidth)
						MaxWidth = Size.cx;
					if (Size.cy > MaxHeight)
						MaxHeight = Size.cy;
				}
				m_GraphNameItemWidth = MaxWidth + 16;
				m_GraphNameItemHeight = MaxHeight + 4;
			}
		}

		if (m_pCaption != nullptr) {
			Direct2D::Brush Brush;
			m_Direct2D.CreateSolidBrush(&Brush,
										GetRValue(m_CaptionColor),
										GetGValue(m_CaptionColor),
										GetBValue(m_CaptionColor),
										192);
			RECT Rect = rc;
			::InflateRect(&Rect, -8, -8);
			if (m_ShowGraphName) {
				RECT rcGraphName;
				GetGraphNameItemRect(0, &rcGraphName);
				Rect.left += rcGraphName.right;
			}
			m_Direct2D.DrawText(m_pCaption, Font, Rect, Brush,
								Direct2D::DRAW_TEXT_RIGHT |
								Direct2D::DRAW_TEXT_TOP |
								Direct2D::DRAW_TEXT_ELLIPSIS);
		}

		std::vector<Direct2D::PointF> PointList;

		for (size_t i = 0; i < m_GraphList.size(); i++) {
			const GraphInfo &Graph = *m_GraphList[i];

			if (!Graph.Visible)
				continue;

			Direct2D::PointF Point;

			Direct2D::Brush LineBrush;
			m_Direct2D.CreateSolidBrush(&LineBrush,
										GetRValue(Graph.Color),
										GetGValue(Graph.Color),
										GetBValue(Graph.Color),
										m_HotItem == (int)i ? 255 : 240);

			float Scale;
			if (Graph.Scale != 0) {
				Scale = (float)(rc.bottom - 1) / (float)Graph.Scale;
			} else {
				int Max = 1;
				for (size_t j = 0; j < Graph.List.size(); j++) {
					if (Graph.List[j] > Max)
						Max = Graph.List[j];
				}
				Scale = (float)(rc.bottom - 1) / (float)Max;
			}

			PointList.clear();
			if (Graph.Type == GraphType::LINE) {
				if (Graph.List.size() > 1) {
					PointList.reserve(rc.right / Graph.Stride + 1);
					int x = rc.right - 1;
					for (int j = (int)Graph.List.size() - 1; j >= 0; j--) {
						Point.x = (float)x;
						Point.y = (float)(rc.bottom - 1) - (float)Graph.List[j] * Scale;
						PointList.push_back(Point);
						if (x <= 0)
							break;
						x -= Graph.Stride;
					}
					/*
					if (x > 0) {
						Point.x = x;
						Point.y = rc.bottom-1;
						PointList.push_back(Point);
						Point.x = 0;
						PointList.push_back(Point);
					}
					*/
					m_Direct2D.DrawPolyline(&PointList[0], (int)PointList.size(),
											LineBrush, Graph.LineWidth);
				}
			} else {
				if (Graph.List.size() > 0) {
					Direct2D::Brush FillBrush;
					m_Direct2D.CreateSolidBrush(&FillBrush,
												GetRValue(Graph.Color),
												GetGValue(Graph.Color),
												GetBValue(Graph.Color),
												m_HotItem == (int)i ? (m_FillOpacity + 255) / 2 : m_FillOpacity);
					PointList.reserve(rc.right / Graph.Stride + 3);
					Point.x = (float)rc.right + Graph.LineWidth;
					Point.y = (float)rc.bottom + Graph.LineWidth;
					PointList.push_back(Point);
					Point.y = (float)(rc.bottom - 1) - (float)Graph.List[Graph.List.size() - 1] * Scale;
					PointList.push_back(Point);
					int x = rc.right - 1;
					for (int j = (int)Graph.List.size() - 1; j >= 0; j--) {
						Point.x = (float)x;
						Point.y = (float)(rc.bottom - 1) - (float)Graph.List[j] * Scale;
						PointList.push_back(Point);
						if (x < 0)
							break;
						x -= Graph.Stride;
					}
					Point.x = (float)(x);
					Point.y = (float)rc.bottom + Graph.LineWidth;
					PointList.push_back(Point);
					m_Direct2D.DrawPolygon(&PointList[0], (int)PointList.size(),
										   &FillBrush, &LineBrush, Graph.LineWidth);
				}
			}
		}

		if (m_ShowGraphName) {
			Direct2D::Brush TextBrush;
			m_Direct2D.CreateSolidBrush(&TextBrush,
										GetRValue(m_BackColor),
										GetGValue(m_BackColor),
										GetBValue(m_BackColor));

			for (size_t i = 0; i < m_GraphList.size(); i++) {
				const GraphInfo &Graph = *m_GraphList[i];

				COLORREF Color = Graph.Color;
				Direct2D::Brush LineBrush;
				m_Direct2D.CreateSolidBrush(&LineBrush,
											GetRValue(Color),
											GetGValue(Color),
											GetBValue(Color));
				if (!Graph.Visible)
					Color = MixColor(Color, m_BackColor);
				Direct2D::Brush FillBrush;
				m_Direct2D.CreateSolidBrush(&FillBrush,
											GetRValue(Color),
											GetGValue(Color),
											GetBValue(Color),
											m_HotItem == (int)i ? 240 : 192);
				RECT Rect;
				GetGraphNameItemRect((int)i, &Rect);
				m_Direct2D.DrawRectangle((float)Rect.left, (float)Rect.top,
										 (float)Rect.right, (float)Rect.bottom,
										 &FillBrush, &LineBrush);
				m_Direct2D.DrawText(Graph.szName, Font, Rect, TextBrush,
									Direct2D::DRAW_TEXT_HORIZONTAL_CENTER |
									Direct2D::DRAW_TEXT_VERTICAL_CENTER);
			}
		}

	}
	m_Direct2D.EndDraw();
}

void GraphView::GetGraphNameItemRect(int Index, RECT *pRect) const
{
	pRect->left = 8;
	pRect->top = 8 + Index * (m_GraphNameItemHeight + 4);
	pRect->right = pRect->left + m_GraphNameItemWidth;
	pRect->bottom = pRect->top + m_GraphNameItemHeight;
}

int GraphView::HitTest(int x, int y) const
{
	POINT pt = {x, y};

	for (int i = 0; i < (int)m_GraphList.size(); i++) {
		RECT rc;
		GetGraphNameItemRect(i, &rc);
		if (::PtInRect(&rc, pt))
			return i;
	}
	return -1;
}

}	// namespace CV
