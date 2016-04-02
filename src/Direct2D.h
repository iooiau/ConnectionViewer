/******************************************************************************
*                                                                             *
*    Direct2D.h                             Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_DIRECT_2D_H
#define CV_DIRECT_2D_H


#include <d2d1.h>
#include <dwrite.h>


namespace CV
{

class Direct2D
{
public:
	struct PointF
	{
		float x, y;
	};

	class Brush
	{
	public:
		Brush();
		Brush(const Brush &Src);
		~Brush();
		Brush &operator=(const Brush &Src);

	private:
		ID2D1Brush *m_pBrush;

		friend class Direct2D;
	};

	class Font
	{
	public:
		enum Style
		{
			STYLE_NORMAL,
			STYLE_OBLIQUE,
			STYLE_ITALIC
		};

		Font();
		Font(const Font &Src);
		~Font();
		Font &operator=(const Font &Src);

	private:
		IDWriteTextFormat *m_pFormat;

		friend class Direct2D;
	};

	Direct2D();
	~Direct2D();
	bool Initialize();
	void Finalize();
	bool BeginDraw(HDC hdc, const RECT &Rect);
	void EndDraw();
	bool Clear(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
	bool Clear() { return Clear(0, 0, 0, 0); }
	bool CreateSolidBrush(Brush *pBrush, BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
	bool CreateFont(Font *pFont, LPCWSTR pFontFamilyName, int Size,
					int Weight = 400, Font::Style Style = Font::STYLE_NORMAL);
	bool DrawRectangle(float Left, float Top, float Right, float Bottom,
					   Brush *pFillBrush, Brush *pLineBrush = nullptr, float LineWidth = 1.0f);
	bool DrawLine(float x1, float y1, float x2, float y2, Brush &Brush, float Width = 1.0f);
	bool DrawPolyline(const PointF *pPoints, int NumPoints,
					  Brush &Brush, float Width = 1.0f);
	bool DrawPolygon(const PointF *pPoints, int NumPoints,
					 Brush *pFillBrush, Brush *pLineBrush = nullptr, float LineWidth = 1.0f);
	enum
	{
		DRAW_TEXT_LEFT				= 0x0000,
		DRAW_TEXT_RIGHT				= 0x0001,
		DRAW_TEXT_HORIZONTAL_CENTER	= 0x0002,
		DRAW_TEXT_TOP				= 0x0000,
		DRAW_TEXT_BOTTOM			= 0x0004,
		DRAW_TEXT_VERTICAL_CENTER	= 0x0008,
		DRAW_TEXT_ELLIPSIS			= 0x0010
	};
	bool DrawText(LPCWSTR pText, Font &Font, const RECT &Rect, Brush &Brush,
				  UINT Flags = DRAW_TEXT_LEFT | DRAW_TEXT_TOP);
	bool GetTextSize(LPCWSTR pText, Font &Font, SIZE *pSize);

private:
	ID2D1PathGeometry *CreatePolylineGeometry(const PointF *pPoints, int NumPoints, bool Close) const;

	ID2D1Factory *m_pD2DFactory;
	IDWriteFactory *m_pDWriteFactory;
	ID2D1DCRenderTarget *m_pRenderTarget;
};

}	// namespace CV


#endif	// ndef CV_DIRECT_2D_H
