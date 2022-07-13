#include "ScrollWnd.h"

#include <afxcontrolbarutil.h>
#include <algorithm>
#include <utility>

#undef max
#undef min

BEGIN_MESSAGE_MAP(ScrollWnd, CWnd)
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_MOUSEWHEEL()
    ON_WM_MOUSEHWHEEL()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

void ScrollWnd::CalculateScrolls()
{
    if (m_scrollCalculating)
        return;

    m_scrollCalculating = true;

    const static LONG scrollWidth = GetSystemMetrics(SM_CXVSCROLL);
    const static LONG scrollHeight = GetSystemMetrics(SM_CXHSCROLL);

    CRect clientRect;
    CWnd::GetClientRect(clientRect);
    if (CWnd::GetStyle() & WS_VSCROLL)
        clientRect.right += scrollWidth;
    if (CWnd::GetStyle() & WS_HSCROLL)
        clientRect.bottom += scrollHeight;

    const int necessaryControlWidth = m_totalSize.cx;
    const int necessaryControlHeight = m_totalSize.cy;

    bool needVScroll = clientRect.Height() < necessaryControlHeight;
    if (needVScroll)
        clientRect.right = std::max(0l, clientRect.right - scrollWidth);
    const bool needHScroll = clientRect.Width() < necessaryControlWidth;
    if (needHScroll)
    {
        clientRect.bottom = std::max(0l, clientRect.bottom - scrollHeight);
        if (!needVScroll)
        {
            needVScroll = clientRect.Height() < necessaryControlHeight;
            if (needVScroll)
                clientRect.right = std::max(0l, clientRect.right - scrollWidth);
        }
    }

    // nPage should be + 1 - because when nMax == nPage scroll is showing
    SCROLLINFO info{ sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE };
    CWnd::GetScrollInfo(SB_VERT, &info, info.fMask);
    info.nMax = necessaryControlHeight;
    info.nPage = clientRect.Height() + 1;
    CWnd::SetScrollInfo(SB_VERT, &info, TRUE);

    CWnd::GetScrollInfo(SB_HORZ, &info, info.fMask);
    info.nMax = necessaryControlWidth;
    info.nPage = clientRect.Width() + 1;
    CWnd::SetScrollInfo(SB_HORZ, &info, TRUE);
    CWnd::Invalidate();

    m_scrollCalculating = false;
}

void ScrollWnd::SetScrollPosition(int nBar, int newPos)
{
    SCROLLINFO info{ sizeof(SCROLLINFO), SIF_POS | SIF_RANGE };
    CWnd::GetScrollInfo(nBar, &info, info.fMask);

    newPos = std::clamp(newPos, info.nMin, info.nMax);
    if (newPos == info.nPos)
        return;
    CWnd::SetScrollPos(nBar, newPos, TRUE);
    if (nBar == SB_VERT)
        OnVerticalVisibleAreaChanged();
    CWnd::Invalidate();
}

void ScrollWnd::SetTotalSize(CSize sizeTotal)
{
    m_totalSize = std::move(sizeTotal);
    CalculateScrolls();
}

const CSize& ScrollWnd::GetTotalSize() const
{
    return m_totalSize;
}

void ScrollWnd::GetVisibleClientArea(CRect& rect) const
{
    CWnd::GetClientRect(rect);
    rect.OffsetRect(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
}

void ScrollWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    CalculateScrolls();
    OnVerticalVisibleAreaChanged();
}

void ScrollWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CRect rect;
    CWnd::GetClientRect(rect);

    switch (nSBCode)
    {
    case SB_THUMBTRACK:
        {
            SCROLLINFO info{ sizeof(SCROLLINFO), SIF_TRACKPOS };
            CWnd::GetScrollInfo(SB_VERT, &info, info.fMask);
            SetScrollPosition(SB_VERT, info.nTrackPos);
        }
        break;
    case SB_LINEUP:
        SetScrollPosition(SB_VERT, CWnd::GetScrollPos(SB_VERT) - 1);
        break;
    case SB_LINEDOWN:
        SetScrollPosition(SB_VERT, CWnd::GetScrollPos(SB_VERT) + 1);
        break;
    case SB_PAGEUP:
        SetScrollPosition(SB_VERT, CWnd::GetScrollPos(SB_VERT) - rect.Height());
        break;
    case TB_PAGEDOWN:
        SetScrollPosition(SB_VERT, CWnd::GetScrollPos(SB_VERT) + rect.Height());
        break;
    default:
        break;
    }

    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void ScrollWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CRect rect;
    CWnd::GetClientRect(rect);

    switch (nSBCode)
    {
    case SB_THUMBTRACK:
        {
            SCROLLINFO info{ sizeof(SCROLLINFO), SIF_POS | SIF_TRACKPOS };
            CWnd::GetScrollInfo(SB_HORZ, &info, info.fMask);
            SetScrollPosition(SB_HORZ, info.nTrackPos);
        }
        break;
    case SB_LINELEFT:
        SetScrollPosition(SB_HORZ, CWnd::GetScrollPos(SB_HORZ) - 1);
        break;
    case SB_LINERIGHT:
        SetScrollPosition(SB_HORZ, CWnd::GetScrollPos(SB_HORZ) + 1);
        break;
    case SB_PAGELEFT:
        SetScrollPosition(SB_HORZ, CWnd::GetScrollPos(SB_HORZ) - rect.Width());
        break;
    case SB_PAGERIGHT:
        SetScrollPosition(SB_HORZ, CWnd::GetScrollPos(SB_HORZ) + rect.Width());
        break;
    default:
        break;
    }

    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void ScrollWnd::OnPaint()
{
    CRect rect;
    CWnd::GetClientRect(rect);

    if (rect.IsRectEmpty())
        return;

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, rect);
    CDC& dc = memDC.GetDC();

    dc.SaveDC();
    dc.SetWindowOrg(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

    OnDraw(dc);

    dc.RestoreDC(-1);
}

BOOL ScrollWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    ScrollWnd::SetScrollPosition(SB_VERT, CWnd::GetScrollPos(SB_VERT) - zDelta);
    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void ScrollWnd::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
    ScrollWnd::SetScrollPosition(SB_HORZ, CWnd::GetScrollPos(SB_HORZ) - zDelta);
    CWnd::OnMouseHWheel(nFlags, zDelta, pt);
}

void ScrollWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
    case VK_HOME:
        {
            const bool ctrlPressed = ::GetKeyState(VK_CONTROL) < 0;
            if (ctrlPressed)
                SetScrollPosition(SB_VERT, 0);
            else
            {
                CRect rect;
                CWnd::GetClientRect(rect);
                SetScrollPosition(SB_VERT, GetScrollPos(SB_VERT) - rect.Height());
            }
        }
        break;
    case VK_END:
        {
            const bool ctrlPressed = ::GetKeyState(VK_CONTROL) < 0;

            if (ctrlPressed)
                SetScrollPosition(SB_VERT, m_totalSize.cy);
            else
            {
                CRect rect;
                CWnd::GetClientRect(rect);
                SetScrollPosition(SB_VERT, GetScrollPos(SB_VERT) + rect.Height());
            }
        }
        break;
    default:
        break;
    }

    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}
