// CTabCtrl widget, allows to draw tab control in 2 ways:
// CCenterTextTabCtrl - centered text in headers
// CButtonsTabCtrl - buttons as tab headers

#pragma once

#include <afxcmn.h>
#include <afxcontrolbarutil.h>

template <class CBaseTabCtrl = CTabCtrl>
class CCustomDrawTabCtrl;

// Tab control with centered text in headers
template <class CBaseTabCtrl = CTabCtrl>
class CCenterTextTabCtrl : public CCustomDrawTabCtrl<CBaseTabCtrl>
{
public:
	CCenterTextTabCtrl();
	~CCenterTextTabCtrl();

	// If true - will draw selected item as window caption
	void SetDrawSelectedAsWindow(bool set = true) { m_bDrawSelectedAsWindow = set; }

private:
	virtual void DrawTab(CDC& pDC, int nIndex, bool selected, bool hovered) override;
	virtual COLORREF BorderColor() const override { return RGB(229, 229, 229); }

private:
	HTHEME m_hWindowTheme;
	HTHEME m_hTabTheme;
	bool m_bDrawSelectedAsWindow = false;
};

// Tab control with buttons as tab headers
template <class CBaseTabCtrl = CTabCtrl>
class CButtonsTabCtrl : public CCustomDrawTabCtrl<CBaseTabCtrl>
{
public:
	CButtonsTabCtrl();
	~CButtonsTabCtrl();

	// If true - will draw selected item as window caption
	void SetDrawSelectedAsWindow(bool set = true) { m_bDrawSelectedAsWindow = set; }

private:
	virtual void DrawTab(CDC& pDC, int nIndex, bool selected, bool hovered) override;
	virtual COLORREF BorderColor() const override { return RGB(208, 208, 208); }

private:
	HTHEME m_hWindowTheme;
	HTHEME m_hButtonTheme;
	bool m_bDrawSelectedAsWindow = false;
};

template <class CBaseTabCtrl>
class CCustomDrawTabCtrl : public CBaseTabCtrl
{
public:
	CCustomDrawTabCtrl() = default;
	virtual ~CCustomDrawTabCtrl() = default;

protected:
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);

	DECLARE_MESSAGE_MAP()

protected:
	virtual void DrawTab(CDC & pDC, int nIndex, bool selected, bool hovered) = 0;
	virtual COLORREF BorderColor() const = 0;

private:
	CRect GetTabsAreaRect() const;
	int GetTabFromPoint(const CPoint & point);

protected:
	int m_nHoverTab = -1;
	bool m_bTrackingMouse = false;
};

BEGIN_TEMPLATE_MESSAGE_MAP(CCustomDrawTabCtrl, CBaseTabCtrl, CBaseTabCtrl)
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

template <class CBaseTabCtrl>
void CCustomDrawTabCtrl<CBaseTabCtrl>::OnPaint()
{
	CRect rcClient;
	CBaseTabCtrl::GetClientRect(rcClient);

	if (rcClient.IsRectEmpty())
		return;

	CPaintDC dcPaint(this);
	CMemDC memDC(dcPaint, rcClient);
	CDC& dc = memDC.GetDC();

	// Fill the background with the dialog color
	dc.FillSolidRect(rcClient, ::GetSysColor(COLOR_3DFACE));

	// Draw the main client area background
	CRect rcTabsArea = GetTabsAreaRect();
	CBrush frameBrush(BorderColor());
	dc.FillSolidRect(&rcTabsArea, RGB(249, 249, 249));

	// Draw each tab
	auto oldFont = dc.SelectObject(CBaseTabCtrl::GetFont());
	int nCurSel = CBaseTabCtrl::GetCurSel();
	for (int i = 0, nTabs = CBaseTabCtrl::GetItemCount(); i < nTabs; ++i)
	{
		if (i == nCurSel || i == m_nHoverTab)
			continue;
		DrawTab(dc, i, false, false);
	}

	if (m_nHoverTab != -1 && m_nHoverTab != nCurSel)
		DrawTab(dc, m_nHoverTab, false, true);

	// draw frame after drawing tabs just to overlab frame
	dc.FrameRect(&rcTabsArea, &frameBrush);

	if (nCurSel != -1)
		DrawTab(dc, nCurSel, true, false);

	dc.SelectObject(oldFont);
}

template <class CBaseTabCtrl>
void CCustomDrawTabCtrl<CBaseTabCtrl>::OnMouseMove(UINT nFlags, CPoint point)
{
	m_nHoverTab = GetTabFromPoint(point);

	if (!m_bTrackingMouse)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = CBaseTabCtrl::m_hWnd;
		TrackMouseEvent(&tme);
		m_bTrackingMouse = true;
	}

	CBaseTabCtrl::OnMouseMove(nFlags, point);
}

template <class CBaseTabCtrl>
void CCustomDrawTabCtrl<CBaseTabCtrl>::OnMouseLeave()
{
	m_nHoverTab = -1;
	m_bTrackingMouse = false;

	CBaseTabCtrl::OnMouseLeave();
}

template <class CBaseTabCtrl>
BOOL CCustomDrawTabCtrl<CBaseTabCtrl>::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

template <class CBaseTabCtrl>
int CCustomDrawTabCtrl<CBaseTabCtrl>::GetTabFromPoint(const CPoint& point)
{
	TCHITTESTINFO hitTestInfo;
	hitTestInfo.pt = point;
	hitTestInfo.flags = 0;

	return CBaseTabCtrl::HitTest(&hitTestInfo);
}

template <class CBaseTabCtrl>
CRect CCustomDrawTabCtrl<CBaseTabCtrl>::GetTabsAreaRect() const
{
	CRect rcClient;
	CBaseTabCtrl::GetClientRect(&rcClient);

	CRect rcTab;
	CBaseTabCtrl::GetItemRect(0, &rcTab);

	rcClient.top += rcTab.Height() + 2;
	rcClient.right -= 2;
	return rcClient;
}

template <class CBaseTabCtrl>
CCenterTextTabCtrl<CBaseTabCtrl>::CCenterTextTabCtrl()
{
	m_hWindowTheme = OpenThemeData(CBaseTabCtrl::m_hWnd, L"Window");
	m_hTabTheme = OpenThemeData(CBaseTabCtrl::m_hWnd, L"Tab");
	ASSERT(m_hWindowTheme && m_hTabTheme);
}

template <class CBaseTabCtrl>
CCenterTextTabCtrl<CBaseTabCtrl>::~CCenterTextTabCtrl()
{
	if (m_hWindowTheme)
		CloseThemeData(m_hWindowTheme);
	if (m_hTabTheme)
		CloseThemeData(m_hTabTheme);
}

template <class CBaseTabCtrl>
void CCenterTextTabCtrl<CBaseTabCtrl>::DrawTab(CDC& pDC, int nIndex, bool selected, bool hovered)
{
	TCHAR szLabel[256] = {};
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = szLabel;
	item.cchTextMax = sizeof(szLabel) / sizeof(TCHAR);
	CBaseTabCtrl::GetItem(nIndex, &item);

	CRect rcFullItem;
	CBaseTabCtrl::GetItemRect(nIndex, &rcFullItem);
	CRect textRect = rcFullItem;

	bool lastItem = (nIndex == CBaseTabCtrl::GetItemCount() - 1);
	// get the real tab item rect
	if (selected) {
		if (nIndex == 0)
			rcFullItem.left -= 2;
		else
			rcFullItem.left--;
	}
	else
	{
		rcFullItem.top += 2;
		textRect.top += 2;
	}
	rcFullItem.bottom++;
	if (lastItem)
	{
		rcFullItem.right++;
		if (!selected)
			rcFullItem.right++;
	}

	HTHEME theme = m_hTabTheme;

	int iPartId = TABP_TOPTABITEM;
	if (nIndex == 0) {
		// First item
		if (lastItem)
			iPartId = TABP_TOPTABITEMBOTHEDGE; // First & Last item
		else
			iPartId = TABP_TOPTABITEMLEFTEDGE;
	}
	else if (lastItem)
		iPartId = TABP_TOPTABITEMRIGHTEDGE;

	int iStateId;
	if (selected)
	{
		if (m_bDrawSelectedAsWindow)
		{
			theme = m_hWindowTheme;

			iPartId = WP_MINCAPTION;
			iStateId = MNCS_ACTIVE;
		}
		else
		{
			if (lastItem)
				rcFullItem.right += 2;
			iStateId = TTIS_SELECTED;
		}
	}
	else
	{
		iStateId = hovered ? TIS_HOT : TIS_NORMAL;
	}
	if (IsThemeBackgroundPartiallyTransparent(theme, iPartId, iStateId))
		DrawThemeParentBackground(CBaseTabCtrl::m_hWnd, pDC, &rcFullItem);
	DrawThemeBackground(theme, pDC, iPartId, iStateId, &rcFullItem, NULL);

	auto oldMode = pDC.SetBkMode(TRANSPARENT);
	pDC.DrawText(szLabel, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC.SetBkMode(oldMode);
}

template <class CBaseTabCtrl>
CButtonsTabCtrl<CBaseTabCtrl>::CButtonsTabCtrl()
{
	m_hWindowTheme = OpenThemeData(CBaseTabCtrl::m_hWnd, L"Window");
	m_hButtonTheme = OpenThemeData(CBaseTabCtrl::m_hWnd, L"Button");
	ASSERT(m_hWindowTheme && m_hButtonTheme);
}

template <class CBaseTabCtrl>
CButtonsTabCtrl<CBaseTabCtrl>::~CButtonsTabCtrl()
{
	if (m_hWindowTheme)
		CloseThemeData(m_hWindowTheme);
	if (m_hButtonTheme)
		CloseThemeData(m_hButtonTheme);
}

template <class CBaseTabCtrl>
void CButtonsTabCtrl<CBaseTabCtrl>::DrawTab(CDC& pDC, int nIndex, bool selected, bool hovered)
{
	TCHAR szLabel[256] = {};
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = szLabel;
	item.cchTextMax = sizeof(szLabel) / sizeof(TCHAR);
	CBaseTabCtrl::GetItem(nIndex, &item);

	CRect rcFullItem;
	CBaseTabCtrl::GetItemRect(nIndex, &rcFullItem);

	// Increase selection window for first tab
	if (selected && nIndex == 0)
		rcFullItem.left -= 2;
	if (nIndex == CBaseTabCtrl::GetItemCount() - 1)
		rcFullItem.right++;
	rcFullItem.bottom++;

	CRect rect = rcFullItem;

	// draw buttons bigger then they are
	if (!selected || !m_bDrawSelectedAsWindow)
	{
		rcFullItem.bottom += 6;
		rcFullItem.left--;
	}

	HTHEME theme = m_hButtonTheme;

	int iPartId = BP_PUSHBUTTON;
	int iStateId;
	if (selected)
	{
		if (m_bDrawSelectedAsWindow)
		{
			theme = m_hWindowTheme;

			iPartId = WP_MINCAPTION;
			iStateId = MNCS_ACTIVE;
		}
		else
			iStateId = PBS_PRESSED;
	}
	else
	{
		iStateId = hovered ? PBS_HOT : PBS_NORMAL;
	}

	// Cut not interesting draw button stuff like extra gap from drawing frame
	int nSavedDC = pDC.SaveDC();
	pDC.IntersectClipRect(rect);
	auto intersectedDc = pDC.GetSafeHdc();

	if (IsThemeBackgroundPartiallyTransparent(theme, iPartId, iStateId))
		DrawThemeParentBackground(CBaseTabCtrl::m_hWnd, intersectedDc, &rcFullItem);
	DrawThemeBackground(theme, intersectedDc, iPartId, iStateId, &rcFullItem, NULL);

	SetBkMode(intersectedDc, TRANSPARENT);
	DrawText(intersectedDc, szLabel, (int)wcslen(szLabel), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	pDC.RestoreDC(nSavedDC);
}