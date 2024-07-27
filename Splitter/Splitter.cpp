#include "Splitter.h"

#include "../DefaultWindowProc.h"

CSplitter::AttachedWindowsSettings::AttachedWindowsSettings(CSplitter& splitter, CWnd& wndToAttach,
                                                            CMFCDynamicLayout::MoveSettings&& moveSettings,
                                                            CMFCDynamicLayout::SizeSettings&& sizeSettings)
    : m_hWndToAttach(wndToAttach.m_hWnd)
    , m_moveSettings(std::move(moveSettings))
    , m_sizeSettings(std::move(sizeSettings))
{
    wndToAttach.GetClientRect(m_initialWindowRect);

    CRect sliderRect;
    splitter.GetWindowRect(sliderRect);
    splitter.GetParent()->ScreenToClient(sliderRect);
    m_initialSlitterRect = sliderRect;
}

CSplitter::CSplitter(_In_opt_ Orientation controlOrientation)
    : m_orientation(controlOrientation)
    , m_hCursor(AfxGetApp()->LoadStandardCursor(m_orientation == Orientation::eHorizontal ? IDC_SIZENS : IDC_SIZEWE))
{}

void CSplitter::SetCallbacks(_In_opt_ const std::function<bool(CSplitter* splitter, CRect& newRect)>& onPosChanging,
                             _In_opt_ const std::function<void(CSplitter* splitter)>& onEndDruggingSplitter)
{
    m_onPosChanging = onPosChanging;
    m_onEndDruggingSplitter = onEndDruggingSplitter;
}

BEGIN_MESSAGE_MAP(CSplitter, CWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_PAINT()
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
    ON_WM_NCCREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

void CSplitter::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_drugStartMousePoint = point;
    SetCapture();
}

void CSplitter::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_drugStartMousePoint.reset();
    ReleaseCapture();

    if (m_onEndDruggingSplitter)
        m_onEndDruggingSplitter(this);
}

void CSplitter::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_drugStartMousePoint.has_value())
    {
        CRect currentRect;
        GetWindowRect(currentRect);
        GetParent()->ScreenToClient(currentRect);

        // Calculate control moving offset
        CSize movingOffset(NULL, NULL);
        switch (m_orientation)
        {
        case Orientation::eVertical:
            movingOffset.cx = point.x - m_drugStartMousePoint->x;
            break;
        case Orientation::eHorizontal:
            movingOffset.cy = point.y - m_drugStartMousePoint->y;
            break;
        }

        CRect newPosition(currentRect);
        newPosition.OffsetRect(movingOffset);

        ApplyNewRect(newPosition, currentRect);
    }
}

void CSplitter::SetMovingCursor(_In_ UINT cursorResourceId)
{
    m_hCursor = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(cursorResourceId));
}

void CSplitter::SetControlBounds(_In_ BoundsType type, _In_opt_ CRect bounds)
{
    m_boundsType = type;
    m_bounds = bounds;

    CRect rect;
    GetWindowRect(rect);
    GetParent()->ScreenToClient(rect);

    ApplyNewRect(rect, rect);
}

void CSplitter::ApplyNewRect(CRect& newRect, std::optional<CRect> currentRect)
{
    if (m_bounds != CRect(kNotSpecified, kNotSpecified, kNotSpecified, kNotSpecified))
    {
        CRect controlBounds(m_bounds);
        if (m_boundsType == BoundsType::eOffsetFromParentBounds)
        {
            CRect parentRect;
            GetParent()->GetClientRect(parentRect);
            if (controlBounds.right != kNotSpecified)
                controlBounds.right = parentRect.right - controlBounds.right;
            if (controlBounds.bottom != kNotSpecified)
                controlBounds.bottom = parentRect.bottom - controlBounds.bottom;
        }

        controlBounds.top = controlBounds.top != kNotSpecified ? controlBounds.top : newRect.top;
        controlBounds.left = controlBounds.left != kNotSpecified ? controlBounds.left : newRect.left;
        controlBounds.bottom = controlBounds.bottom != kNotSpecified ? controlBounds.bottom : newRect.bottom;
        controlBounds.right = controlBounds.right != kNotSpecified ? controlBounds.right : newRect.right;

        // validating new rect bounds
        if (newRect.left < controlBounds.left)
            newRect.OffsetRect(controlBounds.left - newRect.left, 0);
        if (newRect.right > controlBounds.right)
            newRect.OffsetRect(controlBounds.right - newRect.right, 0);
        if (newRect.top < controlBounds.top)
            newRect.OffsetRect(0, controlBounds.top - newRect.top);
        if (newRect.bottom > controlBounds.bottom)
            newRect.OffsetRect(0, controlBounds.bottom - newRect.bottom);

        // after the shift, the border can crawl out from other side, reduce the width to the required
        if (newRect.left < controlBounds.left)
            newRect.left = controlBounds.left;
        if (newRect.top < controlBounds.top)
            newRect.top = controlBounds.top;
    }

    if (!currentRect.has_value())
    {
        currentRect.emplace();
        GetWindowRect(*currentRect);
        GetParent()->ScreenToClient(*currentRect);
    }

    if (newRect == *currentRect)
        return;

    CWnd::MoveWindow(newRect);
    //CWnd::RedrawWindow(); // fix drawing lag
}

void CSplitter::AttachSplitterToWindow(CWnd& wnd, CMFCDynamicLayout::MoveSettings moveSettings, CMFCDynamicLayout::SizeSettings sizeSettings)
{
    UnhookFromAttachedWindow();

    m_splitterLayoutSettings.emplace(*this, wnd, std::move(moveSettings), std::move(sizeSettings));

    DefaultWindowProc::OnWindowMessage(wnd, WM_SIZE,
        [&](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            OnAttachedWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
        }, this);
}

void CSplitter::UnhookFromAttachedWindow()
{
    if (!m_splitterLayoutSettings.has_value())
        return;

    DefaultWindowProc::RemoveCallback(*CWnd::FromHandle(m_splitterLayoutSettings->m_hWndToAttach), WM_SIZE, this);
    m_splitterLayoutSettings.reset();
}

void CSplitter::OnAttachedWindowSizeChanged(int cx, int cy)
{
    ASSERT(m_splitterLayoutSettings.has_value());

    CRect newRect(m_splitterLayoutSettings->m_initialSlitterRect);
    ApplyLayout(newRect, *m_splitterLayoutSettings, CSize(cx, cy) - m_splitterLayoutSettings->m_initialWindowRect.Size());

    m_movingWithParentBounds = true;
    ApplyNewRect(newRect);
    m_movingWithParentBounds = false;
}

void CSplitter::ApplyLayout(CRect& rect, const AttachedWindowsSettings& settings, const CSize& positionDiff)
{
    rect.OffsetRect((int)std::round((double)positionDiff.cx * settings.m_moveSettings.m_nXRatio / 100.),
                    (int)std::round((double)positionDiff.cy * settings.m_moveSettings.m_nYRatio / 100.));

    rect.right += (LONG)std::round((double)positionDiff.cx * settings.m_sizeSettings.m_nXRatio / 100.);
    rect.bottom += (LONG)std::round((double)positionDiff.cy * settings.m_sizeSettings.m_nYRatio / 100.);
}

void CSplitter::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    CRect clientRect;
    GetClientRect(clientRect);
    dc.FillSolidRect(clientRect, GetSysColor(COLOR_3DFACE));

    CPen penDark(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
    CPen penWhite(PS_SOLID, 1, RGB(255, 255, 255));

    CRect centerRect(CRect(clientRect).CenterPoint(), CSize(2, 2));
    centerRect.OffsetRect(-1, -1);

    CPen* pOrigPen = dc.SelectObject(&penWhite);
    const CSize pointOffset = (m_orientation == Orientation::eVertical ? CSize{ 0, 4 } : CSize{ 4, 0 });

    CRect rc(centerRect);
    rc.OffsetRect(1, 1);
    rc.OffsetRect(-pointOffset);

    for (int i = 0; i < 3; ++i)
    {
        dc.Rectangle(&rc);
        rc.OffsetRect(pointOffset);
    }

    dc.SelectObject(&penDark);

    rc = centerRect;
    rc.OffsetRect(-pointOffset);

    for (int i = 0; i < 3; ++i)
    {
        dc.Rectangle(&rc);
        rc.OffsetRect(pointOffset);
    }

    dc.SelectObject(pOrigPen);
}

void CSplitter::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CWnd::OnWindowPosChanged(lpwndpos);

    CRect currentRect;
    GetWindowRect(currentRect);
    GetParent()->ScreenToClient(currentRect);

    if (!m_movingWithParentBounds && m_splitterLayoutSettings.has_value())
    {
        m_splitterLayoutSettings->m_initialSlitterRect = currentRect;
        ::GetClientRect(m_splitterLayoutSettings->m_hWndToAttach, m_splitterLayoutSettings->m_initialWindowRect);
    }
}

void CSplitter::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (m_onPosChanging && (!(lpwndpos->flags & SWP_NOMOVE) || !(lpwndpos->flags & SWP_NOSIZE)))
    {
        CRect currentRect;
        GetWindowRect(currentRect);
        GetParent()->ScreenToClient(currentRect);

        CRect newRect(currentRect);
        if (!(lpwndpos->flags & SWP_NOMOVE))
        {
            newRect.left = lpwndpos->x;
            newRect.top = lpwndpos->y;
        }
        if (!(lpwndpos->flags & SWP_NOSIZE))
        {
            newRect.right = newRect.left + lpwndpos->cx;
            newRect.bottom = newRect.top + lpwndpos->cy;
        }

        if (!m_onPosChanging(this, newRect))
            newRect = currentRect;

        lpwndpos->x = newRect.left;
        lpwndpos->y = newRect.top;
        lpwndpos->cx = newRect.Width();
        lpwndpos->cy = newRect.Height();
    }
    CWnd::OnWindowPosChanging(lpwndpos);
}

BOOL CSplitter::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

BOOL CSplitter::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_hCursor != NULL)
    {
        CRect rectClient;
        GetClientRect(rectClient);

        CPoint ptCursor;
        ::GetCursorPos(&ptCursor);
        ScreenToClient(&ptCursor);

        if (rectClient.PtInRect(ptCursor))
        {
            ::SetCursor(m_hCursor);
            return TRUE;
        }
    }

    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CSplitter::OnNcCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (!CWnd::OnNcCreate(lpCreateStruct))
        return FALSE;

    lpCreateStruct->style &= ~WS_BORDER;

    return TRUE;
}

void CSplitter::OnDestroy()
{
    CWnd::OnDestroy();
    UnhookFromAttachedWindow();
}
