/******************************************************************************
*                                                                             *
*    Direct2D.cpp                           Copyright(c) 2010-2016 itow,y.    *
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
#include "Direct2D.h"
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
//#pragma comment(lib, "WindowsCodecs.lib")


namespace CV
{

template<typename T> inline void SafeRelease(T *&p)
{
	if (p) {
		p->Release();
		p = nullptr;
	}
}


Direct2D::Direct2D()
	: m_pD2DFactory(nullptr)
	, m_pDWriteFactory(nullptr)
	, m_pRenderTarget(nullptr)
{
}

Direct2D::~Direct2D()
{
	Finalize();
}

bool Direct2D::Initialize()
{
	HRESULT hr;

	if (m_pD2DFactory == nullptr) {
		hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
								 __uuidof(ID2D1Factory),
								 reinterpret_cast<void**>(&m_pD2DFactory));
		if (FAILED(hr))
			return false;
	}

	if (m_pDWriteFactory == nullptr) {
		hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
								   __uuidof(IDWriteFactory),
								   reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (FAILED(hr))
			return false;
	}

	return true;
}

void Direct2D::Finalize()
{
	SafeRelease(m_pRenderTarget);
	SafeRelease(m_pDWriteFactory);
	SafeRelease(m_pD2DFactory);
}

bool Direct2D::BeginDraw(HDC hdc, const RECT &Rect)
{
	if (m_pD2DFactory == nullptr)
		return false;

	if (m_pRenderTarget == nullptr) {
		HRESULT hr;
		D2D1_RENDER_TARGET_PROPERTIES Props = {
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
			0.0f, 0.0f,
			D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
			D2D1_FEATURE_LEVEL_DEFAULT,
		};
		hr = m_pD2DFactory->CreateDCRenderTarget(&Props, &m_pRenderTarget);
		if (FAILED(hr))
			return false;
	}

	m_pRenderTarget->BindDC(hdc, &Rect);
	m_pRenderTarget->BeginDraw();

	return true;
}

void Direct2D::EndDraw()
{
	if (m_pRenderTarget != nullptr) {
		if (m_pRenderTarget->EndDraw() == D2DERR_RECREATE_TARGET) {
			m_pRenderTarget->Release();
			m_pRenderTarget = nullptr;
		}
	}
}

bool Direct2D::Clear(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	if (m_pRenderTarget == nullptr)
		return false;

	m_pRenderTarget->Clear(
		D2D1::ColorF((float)Red / 255.0f,
					 (float)Green / 255.0f,
					 (float)Blue / 255.0f,
					 (float)Alpha / 255.0f));
	return true;
}

bool Direct2D::CreateSolidBrush(Brush *pBrush, BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	if (m_pRenderTarget == nullptr || pBrush == nullptr)
		return false;

	SafeRelease(pBrush->m_pBrush);

	ID2D1SolidColorBrush *pSolidBrush;
	if (m_pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF((float)Red / 255.0f,
						 (float)Green / 255.0f,
						 (float)Blue / 255.0f,
						 (float)Alpha / 255.0f),
			&pSolidBrush) != S_OK)
		return false;
	pBrush->m_pBrush = pSolidBrush;
	return true;
}

bool Direct2D::CreateFont(Font *pFont, LPCWSTR pFontFamilyName, int Size,
						  int Weight, Font::Style Style)
{
	if (m_pDWriteFactory == nullptr || pFont == nullptr)
		return false;

	SafeRelease(pFont->m_pFormat);

	HRESULT hr;
	IDWriteTextFormat *pFormat;
	hr = m_pDWriteFactory->CreateTextFormat(
		pFontFamilyName, nullptr,
		(DWRITE_FONT_WEIGHT)Weight,
		(DWRITE_FONT_STYLE)Style,
		DWRITE_FONT_STRETCH_NORMAL,
		(FLOAT)Size,
		L"",
		&pFormat);
	if (FAILED(hr))
		return false;

	pFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	static const DWRITE_TRIMMING Trimming = {DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0};
	pFormat->SetTrimming(&Trimming, nullptr);

	pFont->m_pFormat = pFormat;

	return true;
}

bool Direct2D::DrawRectangle(float Left, float Top, float Right, float Bottom,
							 Brush *pFillBrush, Brush *pLineBrush, float LineWidth)
{
	if (m_pRenderTarget == nullptr)
		return false;

	D2D1_RECT_F Rect = { Left, Top, Right, Bottom };
	if (pFillBrush != nullptr && pFillBrush->m_pBrush != nullptr)
		m_pRenderTarget->FillRectangle(Rect, pFillBrush->m_pBrush);
	if (pLineBrush != nullptr && pLineBrush->m_pBrush != nullptr)
		m_pRenderTarget->DrawRectangle(Rect, pLineBrush->m_pBrush, LineWidth);
	return true;
}

bool Direct2D::DrawLine(float x1, float y1, float x2, float y2, Brush &Brush, float Width)
{
	if (m_pRenderTarget == nullptr || Brush.m_pBrush == nullptr)
		return false;

	m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2),
							  Brush.m_pBrush, Width);
	return true;
}

bool Direct2D::DrawPolyline(const PointF *pPoints, int NumPoints,
							Brush &Brush, float Width)
{
	if (m_pRenderTarget == nullptr || pPoints == nullptr || NumPoints < 2 || Brush.m_pBrush == nullptr)
		return false;

	ID2D1PathGeometry *pPathGeometry = CreatePolylineGeometry(pPoints, NumPoints, false);
	if (pPathGeometry == nullptr)
		return false;
	m_pRenderTarget->DrawGeometry(pPathGeometry, Brush.m_pBrush, Width);
	pPathGeometry->Release();
	return true;
}

bool Direct2D::DrawPolygon(const PointF *pPoints, int NumPoints,
						   Brush *pFillBrush, Brush *pLineBrush, float LineWidth)
{
	if (m_pRenderTarget == nullptr || pPoints == nullptr || NumPoints < 2)
		return false;

	ID2D1PathGeometry *pPathGeometry = CreatePolylineGeometry(pPoints, NumPoints, true);
	if (pPathGeometry == nullptr)
		return false;
	if (pFillBrush != nullptr && pFillBrush->m_pBrush != nullptr)
		m_pRenderTarget->FillGeometry(pPathGeometry, pFillBrush->m_pBrush);
	if (pLineBrush != nullptr && pLineBrush->m_pBrush != nullptr)
		m_pRenderTarget->DrawGeometry(pPathGeometry, pLineBrush->m_pBrush, LineWidth);
	pPathGeometry->Release();
	return true;
}

bool Direct2D::DrawText(LPCWSTR pText, Font &Font, const RECT &Rect, Brush &Brush, UINT Flags)
{
	if (m_pRenderTarget == nullptr || pText == nullptr || Font.m_pFormat == nullptr || Brush.m_pBrush == nullptr)
		return false;

	Font.m_pFormat->SetParagraphAlignment(
		(Flags & DRAW_TEXT_BOTTOM) != 0 ? DWRITE_PARAGRAPH_ALIGNMENT_FAR :
		(Flags & DRAW_TEXT_VERTICAL_CENTER) != 0 ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
		DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	Font.m_pFormat->SetTextAlignment(
		(Flags & DRAW_TEXT_RIGHT) != 0 ? DWRITE_TEXT_ALIGNMENT_TRAILING :
		(Flags & DRAW_TEXT_HORIZONTAL_CENTER) != 0 ? DWRITE_TEXT_ALIGNMENT_CENTER :
		DWRITE_TEXT_ALIGNMENT_LEADING);
	DWRITE_TRIMMING Trimming;
	Trimming.granularity =
		(Flags & DRAW_TEXT_ELLIPSIS) != 0 ?
			DWRITE_TRIMMING_GRANULARITY_CHARACTER : DWRITE_TRIMMING_GRANULARITY_NONE;
	Trimming.delimiter = 0;
	Trimming.delimiterCount = 0;
	Font.m_pFormat->SetTrimming(&Trimming, nullptr);

	m_pRenderTarget->DrawText(
		pText, ::lstrlenW(pText),
		Font.m_pFormat,
		D2D1::RectF((float)Rect.left, (float)Rect.top,
		(float)Rect.right, (float)Rect.bottom),
		Brush.m_pBrush);

	return true;
}

bool Direct2D::GetTextSize(LPCWSTR pText, Font &Font, SIZE *pSize)
{
	if (m_pDWriteFactory == nullptr || pText == nullptr || Font.m_pFormat == nullptr || pSize == nullptr)
		return false;

	pSize->cx = 0;
	pSize->cy = 0;

	HRESULT hr;
	IDWriteTextLayout *pLayout;
	hr = m_pDWriteFactory->CreateTextLayout(
		pText, ::lstrlenW(pText),
		Font.m_pFormat,
		640.0f, 480.0f,
		&pLayout);
	if (FAILED(hr))
		return false;

	DWRITE_TEXT_METRICS Metrics;
	hr = pLayout->GetMetrics(&Metrics);
	pLayout->Release();
	if (FAILED(hr))
		return false;

	pSize->cx = (LONG)std::ceil(Metrics.widthIncludingTrailingWhitespace);
	pSize->cy = (LONG)std::ceil(Metrics.height);

	return true;
}

ID2D1PathGeometry *Direct2D::CreatePolylineGeometry(const PointF *pPoints, int NumPoints, bool Close) const
{
	ID2D1PathGeometry *pPathGeometry;
	HRESULT hr;

	hr = m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
	if (FAILED(hr))
		return false;

	ID2D1GeometrySink *pSink;
	hr = pPathGeometry->Open(&pSink);
	if (FAILED(hr)) {
		pPathGeometry->Release();
		return nullptr;
	}

	pSink->BeginFigure(D2D1::Point2F(pPoints[0].x, pPoints[0].y),
					   D2D1_FIGURE_BEGIN_FILLED);
	for (int i = 1; i < NumPoints; i++) {
		pSink->AddLine(D2D1::Point2F(pPoints[i].x, pPoints[i].y));
	}
	pSink->EndFigure(Close ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
	pSink->Close();
	pSink->Release();

	return pPathGeometry;
}


Direct2D::Brush::Brush()
	: m_pBrush(nullptr)
{
}

Direct2D::Brush::Brush(const Brush &Src)
	: m_pBrush(Src.m_pBrush)
{
	if (m_pBrush != nullptr)
		m_pBrush->AddRef();
}

Direct2D::Brush::~Brush()
{
	if (m_pBrush != nullptr)
		m_pBrush->Release();
}

Direct2D::Brush &Direct2D::Brush::operator=(const Brush &Src)
{
	if (&Src != this) {
		if (m_pBrush != nullptr)
			m_pBrush->Release();
		m_pBrush = Src.m_pBrush;
		if (m_pBrush != nullptr)
			m_pBrush->AddRef();
	}
	return *this;
}


Direct2D::Font::Font()
	: m_pFormat(nullptr)
{
}

Direct2D::Font::Font(const Font &Src)
	: m_pFormat(Src.m_pFormat)
{
	if (m_pFormat != nullptr)
		m_pFormat->AddRef();
}

Direct2D::Font::~Font()
{
	if (m_pFormat != nullptr)
		m_pFormat->Release();
}

Direct2D::Font &Direct2D::Font::operator=(const Font &Src)
{
	if (&Src != this) {
		if (m_pFormat != nullptr)
			m_pFormat->Release();
		m_pFormat = Src.m_pFormat;
		if (m_pFormat != nullptr)
			m_pFormat->AddRef();
	}
	return *this;
}

}	// namespace CV
