/******************************************************************************
*                                                                             *
*    GraphView.h                            Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_GRAPH_VIEW_H
#define CV_GRAPH_VIEW_H


#include <deque>
#include <vector>
#include "Widget.h"
#include "Direct2D.h"


namespace CV
{

class GraphView : public CustomWidget
{
public:
	enum class GraphType
	{
		LINE,
		AREA
	};

	typedef std::deque<int> PointList;

	struct GraphInfo
	{
		enum { MAX_NAME = 64 };

		GraphType Type;
		COLORREF Color;
		PointList List;
		int Scale;
		int Stride;
		bool Visible;
		float LineWidth;
		TCHAR szName[MAX_NAME];
	};

	cvAbstractClass(EventHandler)
	{
public:
		virtual ~EventHandler() {}
		virtual void OnRButtonDown(GraphView *pGraphView, int x, int y) {}
		virtual void OnRButtonUp(GraphView *pGraphView, int x, int y) {}
	};

	static bool Initialize();

	GraphView();
	~GraphView();
	bool Create(HWND Parent, int ID = 0) override;
	void DeleteAllGraphs();
	bool AddGraph(GraphInfo *pInfo);
	GraphInfo *GetGraph(int Index);
	const GraphInfo *GetGraph(int Index) const;
	void SetColors(COLORREF BackColor, COLORREF GridColor, COLORREF CaptionColor);
	void SetCaption(LPCTSTR pCaption);
	void SetEventHandler(EventHandler *pEventHandler);

private:
	LRESULT OnMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;
	void Draw(HDC hdc, const RECT &PaintRect);
	void GetGraphNameItemRect(int Index, RECT *pRect) const;
	int HitTest(int x, int y) const;

	static const LPCTSTR m_pClassName;
	static ATOM m_ClassAtom;

	typedef std::vector<GraphInfo*> GraphList;

	COLORREF m_BackColor;
	COLORREF m_GridColor;
	COLORREF m_CaptionColor;
	int m_FillOpacity;
	GraphList m_GraphList;
	LOGFONT m_GraphNameFont;
	bool m_ShowGraphName;
	int m_GraphNameItemWidth;
	int m_GraphNameItemHeight;
	Direct2D m_Direct2D;
	LPTSTR m_pCaption;
	EventHandler *m_pEventHandler;
	int m_HotItem;
};

}	// namespace CV


#endif	// ndef CV_GRAPH_VIEW_H
