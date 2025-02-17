#include "Slider.h"

#include <afxwin.h>
#include <algorithm>
#include <corecrt_math.h>
#include <string>

namespace {

constexpr COLORREF kThumbColor = RGB(143, 143, 143);
constexpr COLORREF kLineColor = RGB(157, 157, 157);
constexpr COLORREF kSelectionColor = RGB(0, 120, 215);

COLORREF darker(COLORREF Color, int Percent)
{
    int r = GetRValue(Color);
    int g = GetGValue(Color);
    int b = GetBValue(Color);

    r = r - MulDiv(r, Percent, 100);
    g = g - MulDiv(g, Percent, 100);
    b = b - MulDiv(b, Percent, 100);
    return RGB(r, g, b);
}

_NODISCARD std::wstring get_string(const wchar_t* format, double value)
{
    const int size_s = std::swprintf(nullptr, 0, format, value) + 1; // + '\0'
    if (size_s <= 0) { return L"format error"; }
    const auto size = static_cast<size_t>(size_s);
    std::wstring string(size, {});
    std::swprintf(string.data(), size, format, value);
    string.pop_back(); // - '\0'
    return string;
}

void apply_screen_dpi(bool horizontal, unsigned& width, unsigned& height)
{
    const auto screen = ::GetDC(nullptr);
    const auto horScale = ::GetDeviceCaps(screen, LOGPIXELSX) / 100.;
    const auto vertScale = ::GetDeviceCaps(screen, LOGPIXELSY) / 100.;
    ::ReleaseDC(0, screen);

    if (horizontal)
    {
        width = unsigned(width * horScale);
        height = unsigned(height * vertScale);
    }
    else
    {
        width = unsigned(width * vertScale);
        height = unsigned(height * horScale);
    }
}
} // namespace

BEGIN_MESSAGE_MAP(CSlider, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_KEYDOWN()
    ON_WM_GETDLGCODE()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CSlider::OnPaint()
{
    if (m_range.second - m_range.first == 0.)
    {
        ASSERT(FALSE);
        return;
    }

    ASSERT(m_range.first <= m_thumbs.first.position);
    if (GetStyle() & TBS_ENABLESELRANGE)
    {
        ASSERT(m_thumbs.first.position <= m_thumbs.second.position);
        ASSERT(m_thumbs.second.position <= m_range.second);
    }
    else
        ASSERT(m_range.second >= m_thumbs.first.position);

    const auto currentWidth = m_thumbWidth;
    const auto currentLineWidth = m_lineWidth;

    apply_screen_dpi(GetStyle() & TBS_HORZ, m_thumbWidth, m_lineWidth);

    CRect clientRect;
    GetClientRect(&clientRect);

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, clientRect);
    CDC& dc = memDC.GetDC();

    DrawThemeParentBackground(m_hWnd, dc, &clientRect);

    if (GetStyle() & TBS_VERT)
        OnPaintVertical(dc, std::move(clientRect));
    else
        OnPaintHorizontal(dc, std::move(clientRect));

    m_thumbWidth = currentWidth;
    m_lineWidth = currentLineWidth;
}

void CSlider::OnPaintHorizontal(CDC& dc, CRect clientRect)
{
    ASSERT(!(GetStyle() & TBS_VERT));

    dc.FillSolidRect(clientRect, GetSysColor(COLOR_3DFACE));

    const auto height = clientRect.Height();
    const double width = GetControlWidthInPixels();

    const int leftPos = static_cast<int>(round((m_thumbs.first.position - m_range.first) / (m_range.second - m_range.first) * width));
    const int rightPos = static_cast<int>(round((m_thumbs.second.position - m_range.first) / (m_range.second - m_range.first) * width)) + m_thumbWidth;

    m_thumbs.first.rect = CRect(CPoint(leftPos, 0), CSize(m_thumbWidth, height));
    m_thumbs.second.rect = CRect(CPoint(rightPos, 0), CSize(m_thumbWidth, height));

    // draw common line and selection
    clientRect.top = clientRect.bottom = clientRect.CenterPoint().y;
    if (GetStyle() & TBS_ENABLESELRANGE)
        clientRect.InflateRect(-(int)m_thumbWidth, m_lineWidth / 2);
    else
        clientRect.InflateRect(-(int)m_thumbWidth / 2, m_lineWidth / 2);

    if (m_lineWidth % 2 != 0)
        clientRect.bottom += 1;

    auto selectionColor = IsWindowEnabled() ? kSelectionColor : kLineColor;

    if (GetStyle() & TBS_ENABLESELRANGE)
    {
        dc.FillSolidRect(clientRect, kLineColor);

        clientRect.left = m_thumbs.first.rect.right;
        clientRect.right = m_thumbs.second.rect.left;
        clientRect.InflateRect(1, 2);
        dc.FillSolidRect(clientRect, selectionColor);
    }
    else
    {
        dc.FillSolidRect(clientRect, selectionColor);
        m_thumbs.second.rect.SetRectEmpty();
    }

    DrawThumb(dc, m_thumbs.first);
    if (GetStyle() & TBS_ENABLESELRANGE)
        DrawThumb(dc, m_thumbs.second);
}

void CSlider::OnPaintVertical(CDC& dc, CRect clientRect)
{
    ASSERT((GetStyle() & TBS_VERT));

    const auto height = clientRect.Width();
    const double width = GetControlWidthInPixels();

    const int leftPos = int(round((m_thumbs.first.position - m_range.first) / (m_range.second - m_range.first) * width));
    const int rightPos = int(round((m_thumbs.second.position - m_range.first) / (m_range.second - m_range.first) * width) + m_thumbWidth);

    m_thumbs.first.rect = CRect(CPoint(0, leftPos), CSize(height, m_thumbWidth));
    m_thumbs.second.rect = CRect(CPoint(0, rightPos), CSize(height, m_thumbWidth));

    // draw common line and selection
    clientRect.left = clientRect.right = clientRect.CenterPoint().x;
    clientRect.InflateRect(m_lineWidth / 2, -(int)m_thumbWidth);
    if (m_lineWidth % 2 != 0)
        clientRect.right += 1;

    auto selectionColor = IsWindowEnabled() ? kSelectionColor : kLineColor;
    if (GetStyle() & TBS_ENABLESELRANGE)
    {
        dc.FillSolidRect(clientRect, kLineColor);
        clientRect.top = m_thumbs.first.rect.bottom;
        clientRect.bottom = m_thumbs.second.rect.top;
        clientRect.InflateRect(2, 1);
        dc.FillSolidRect(clientRect, selectionColor);
    }
    else
    {
        dc.FillSolidRect(clientRect, selectionColor);
        m_thumbs.second.rect.SetRectEmpty();
    }

    DrawThumb(dc, m_thumbs.first);
    if (GetStyle() & TBS_ENABLESELRANGE)
        DrawThumb(dc, m_thumbs.second);
}

void CSlider::DrawThumb(CDC& dc, const struct ThumbInfo& info)
{
    // We don't draw second thumb if TBS_ENABLESELRANGE is disabled
    ASSERT(&info != &m_thumbs.second || GetStyle() & TBS_ENABLESELRANGE);

    COLORREF color;
    if (IsWindowEnabled())
    {
        switch (info.state)
        {
        case ThumbInfo::State::eHot:
            color = darker(kThumbColor, 20);
            break;
        case ThumbInfo::State::eTracking:
            color = darker(kThumbColor, 40);
            break;
        default:
            ASSERT(info.state == ThumbInfo::State::eNormal);
            color = kThumbColor;
            break;
        }
    }
    else
        color = darker(kThumbColor, -20);

    // draw thumbs
    const CBrush thumbBrush(color);

    dc.SelectStockObject(WHITE_PEN);
    dc.SelectObject(thumbBrush);
    dc.RoundRect(info.rect, CPoint(6, 6));
}

double CSlider::GetControlWidthInPixels() const
{
    CRect clientRect;
    GetClientRect(&clientRect);

    double controlWidthInPixels = (GetStyle() & TBS_VERT ? clientRect.Height() : clientRect.Width());
    if (GetStyle() & TBS_ENABLESELRANGE)
        controlWidthInPixels -= 2 * m_thumbWidth;
    else
        controlWidthInPixels -= m_thumbWidth;

   return controlWidthInPixels;
}

void CSlider::OnLButtonDown(UINT nFlags, CPoint point)
{
    SetFocus();
    Invalidate();

    const bool trackLeft = m_thumbs.first.state == ThumbInfo::State::eTracking;
    const bool trackRight = m_thumbs.second.state == ThumbInfo::State::eTracking;
    if (!trackLeft && !trackRight)
    {
        if (GetStyle() & TBS_ENABLESELRANGE)
        {
            if (m_thumbs.first.rect.PtInRect(point))
            {
                m_thumbs.first.state = ThumbInfo::State::eTracking;
                m_clickOffsetFormThumbCenter = point - m_thumbs.first.rect.CenterPoint();

                ShowSliderTooltip(true, true);
            }
            else if (m_thumbs.second.rect.PtInRect(point))
            {
                m_thumbs.second.state = ThumbInfo::State::eTracking;
                m_clickOffsetFormThumbCenter = point - m_thumbs.second.rect.CenterPoint();
                ShowSliderTooltip(false, true);
            }
            else
            {
                CRect middleRect;
                if (GetStyle() & TBS_VERT)
                    middleRect = CRect(0, m_thumbs.first.rect.bottom + 1, m_thumbs.first.rect.right, m_thumbs.second.rect.top - 1);
                else
                    middleRect = CRect(m_thumbs.first.rect.right + 1, 0, m_thumbs.second.rect.left - 1, m_thumbs.second.rect.bottom);

                if (middleRect.PtInRect(point))
                {
                    m_thumbs.first.state = ThumbInfo::State::eTracking;
                    m_thumbs.second.state = ThumbInfo::State::eTracking;
                    m_clickOffsetFormThumbCenter = point - middleRect.CenterPoint();
                }
                else
                {
                    SetTimer(0, 700, nullptr);
                    OnTimer(NULL);
                }
            }
        }
        else
        {
            m_thumbs.first.state = ThumbInfo::State::eTracking;
            m_clickOffsetFormThumbCenter = {};
            OnMouseMove(nFlags, point);
            ShowSliderTooltip(true, true);
        }

        SetCapture();
    }
    CWnd::OnLButtonDown(nFlags, point);
}

void CSlider::OnLButtonUp(UINT nFlags, CPoint point)
{
    const bool trackLeft = m_thumbs.first.state == ThumbInfo::State::eTracking;
    const bool trackRight = m_thumbs.second.state == ThumbInfo::State::eTracking;
    if (trackLeft || trackRight)
    {
        m_thumbs.first.state = m_thumbs.first.rect.PtInRect(point) ? ThumbInfo::State::eHot : ThumbInfo::State::eNormal;
        m_thumbs.second.state = m_thumbs.second.rect.PtInRect(point) ? ThumbInfo::State::eHot : ThumbInfo::State::eNormal;
        m_tooltip.ShowWindow(SW_HIDE);

        Invalidate();
    }
    else
        KillTimer(NULL);

    ::ReleaseCapture();

    CWnd::OnLButtonUp(nFlags, point);
}

void CSlider::OnMouseMove(UINT nFlags, CPoint point)
{
    const bool trackLeft = m_thumbs.first.state == ThumbInfo::State::eTracking;
    const bool trackRight = m_thumbs.second.state == ThumbInfo::State::eTracking;
    if (trackLeft || trackRight)
    {
        const int position = GetStyle() & TBS_VERT ? (point.y - m_clickOffsetFormThumbCenter.y) : (point.x - m_clickOffsetFormThumbCenter.x);

        const double controlWidthInPixels = GetControlWidthInPixels();
        const double countDataInOnePixel = (m_range.second - m_range.first) / std::max<double>(controlWidthInPixels, 1.);

        const auto previousThumbPositions = std::make_pair(m_thumbs.first.position, m_thumbs.second.position);

        if (trackLeft && trackRight)
        {
            const double delta = m_thumbs.second.position - m_thumbs.first.position;
            ASSERT(delta >= 0.0);
            const double newValue = countDataInOnePixel * double(position - (int)m_thumbWidth) + m_range.first - delta / 2.0;

            m_thumbs.first.position = ApplyIncrementStep(m_thumbs.first.position, newValue);
            m_thumbs.second.position = m_thumbs.first.position + delta;
            if (m_thumbs.first.position <= m_range.first)
            {
                m_thumbs.first.position = m_range.first;
                m_thumbs.second.position = m_thumbs.first.position + delta;
            }
            if (m_thumbs.second.position >= m_range.second)
            {
                m_thumbs.second.position = m_range.second;
                m_thumbs.first.position = m_thumbs.second.position - delta;
            }
        }
        else if (trackLeft)
        {
            const double newValue = countDataInOnePixel * double(position - (int)m_thumbWidth / 2) + m_range.first;

            m_thumbs.first.position = std::clamp(ApplyIncrementStep(m_thumbs.first.position, newValue),
                m_range.first, GetStyle() & TBS_ENABLESELRANGE ? m_thumbs.second.position : m_range.second);
            ShowSliderTooltip(true);
        }
        else
        {
            const double newValue = countDataInOnePixel * double(position - (int)m_thumbWidth * 3 / 2) + m_range.first;

            m_thumbs.second.position = std::clamp(ApplyIncrementStep(m_thumbs.second.position, newValue),
                m_thumbs.first.position, m_range.second);
            ShowSliderTooltip(false);
        }

        if (previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position))
        {
            SendChangePositionEvent(previousThumbPositions);
            RedrawWindow();
        }
    }
    else
    {
        const auto previousThumbStates = std::make_pair(m_thumbs.first.state, m_thumbs.second.state);
        if (GetStyle() & TBS_ENABLESELRANGE)
        {
            if (m_thumbs.first.rect.PtInRect(point))
            {
                m_thumbs.first.state = ThumbInfo::State::eHot;
                m_thumbs.second.state = ThumbInfo::State::eNormal;
            }
            else if (m_thumbs.second.rect.PtInRect(point))
            {
                m_thumbs.first.state = ThumbInfo::State::eNormal;
                m_thumbs.second.state = ThumbInfo::State::eHot;
            }
            else
            {
                CRect middleRect;
                if (GetStyle() & TBS_VERT)
                    middleRect = CRect(0, m_thumbs.first.rect.bottom + 1, m_thumbs.first.rect.right, m_thumbs.second.rect.top - 1);
                else
                    middleRect = CRect(m_thumbs.first.rect.right + 1, 0, m_thumbs.second.rect.left - 1, m_thumbs.second.rect.bottom);

                if (middleRect.PtInRect(point))
                {
                    m_thumbs.first.state = ThumbInfo::State::eHot;
                    m_thumbs.second.state = ThumbInfo::State::eHot;
                }
                else
                {
                    m_thumbs.first.state = ThumbInfo::State::eNormal;
                    m_thumbs.second.state = ThumbInfo::State::eNormal;
                }
            }
        }
        else
        {
            m_thumbs.first.state = ThumbInfo::State::eHot;
        }

        if (previousThumbStates != std::make_pair(m_thumbs.first.state, m_thumbs.second.state))
            RedrawWindow();
    }

    if (!m_trackMouse)
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        if (::TrackMouseEvent(&tme))
            m_trackMouse = true;
    }

    CWnd::OnMouseMove(nFlags, point);
}

void CSlider::OnMouseLeave()
{
    m_trackMouse = false;

    const auto previousThumbStates = std::make_pair(m_thumbs.first.state, m_thumbs.second.state);

    if (m_thumbs.first.state != ThumbInfo::State::eTracking)
        m_thumbs.first.state = ThumbInfo::State::eNormal;
    if (m_thumbs.second.state != ThumbInfo::State::eTracking)
        m_thumbs.second.state = ThumbInfo::State::eNormal;

    if (previousThumbStates != std::make_pair(m_thumbs.first.state, m_thumbs.second.state))
        RedrawWindow();

    CWnd::OnMouseLeave();
}

void CSlider::SetRange(_In_ std::pair<double, double> range, _In_ bool redraw)
{
    ASSERT(range.first <= range.second);
    if (range.first > range.second)
        std::swap(range.first, range.second);

    m_range = std::move(range);

    NormalizePositions();
    if (redraw)
        Invalidate();
}

std::pair<double, double> CSlider::GetRange() const
{
    return m_range;
}

void CSlider::SetIncrementStep(double step)
{
    m_incrementStep = step;
}

void CSlider::SetPositions(_In_ std::pair<double, double> positions, _In_ bool redraw)
{
    ASSERT(positions.first <= positions.second);
    if (positions.first > positions.second)
        std::swap(positions.first, positions.second);
    if (m_thumbs.first.position == positions.first && m_thumbs.second.position == positions.second)
        return;

    const auto previousThumbPositions = std::make_pair(m_thumbs.first.position, m_thumbs.second.position);
    m_thumbs.first.position = positions.first;
    m_thumbs.second.position = positions.second;

    if (!NormalizePositions() && previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position))
        SendChangePositionEvent(previousThumbPositions);

    if (redraw)
        Invalidate();
}

std::pair<double, double> CSlider::GetPositions() const
{
    return std::make_pair(m_thumbs.first.position, m_thumbs.second.position);
}

void CSlider::SetThumbWidth(_In_ unsigned width)
{
    m_thumbWidth = width;
    Invalidate();
}

unsigned CSlider::GetThumbWidth() const
{
    return m_thumbWidth;
}

void CSlider::SetLineWidth(_In_ unsigned width)
{
    m_lineWidth = width;
    Invalidate();
}

unsigned CSlider::GetLineWidth() const
{
    return m_lineWidth;
}

void CSlider::SetTooltipTextFormat(const wchar_t* format)
{
    m_tooltipFormat = format;
}

bool CSlider::NormalizePositions()
{
    const auto previousThumbPositions = std::make_pair(m_thumbs.first.position, m_thumbs.second.position);
    if (m_thumbs.first.position < m_range.first)
    {
        m_thumbs.first.position = m_range.first;
        if (GetStyle() & TBS_ENABLESELRANGE && m_thumbs.second.position < m_thumbs.first.position)
            m_thumbs.second.position = m_thumbs.first.position;
    }
    if (GetStyle() & TBS_ENABLESELRANGE)
    {
        if (m_thumbs.second.position > m_range.second)
        {
            m_thumbs.second.position = m_range.second;
            if (m_thumbs.first.position > m_thumbs.second.position)
                m_thumbs.first.position = m_thumbs.second.position;
        }
    }
    else if (m_thumbs.first.position > m_range.second)
        m_thumbs.first.position = m_range.second;

    if (previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position))
        SendChangePositionEvent(previousThumbPositions);
    return previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position);
}

void CSlider::SendChangePositionEvent(const std::pair<double, double>& previousThumbPositions) const
{ 
    ASSERT(previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position));

    NMTRBTHUMBPOSCHANGING posChanging;
    posChanging.hdr.hwndFrom = m_hWnd;
    posChanging.hdr.idFrom = GetWindowLong(m_hWnd, GWL_ID);
    posChanging.hdr.code = (UINT)TRBN_THUMBPOSCHANGING;
    if (previousThumbPositions.second == m_thumbs.second.position)
    {
        posChanging.nReason = (int)TrackMode::TRACK_LEFT;
        posChanging.dwPos = (DWORD)m_thumbs.first.position;
    }
    else if (GetStyle() & TBS_ENABLESELRANGE)
    {
        if (previousThumbPositions.first == m_thumbs.first.position)
        {
            posChanging.nReason = (int)TrackMode::TRACK_RIGHT;
            posChanging.dwPos = (DWORD)m_thumbs.second.position;
        }
        else
        {
            posChanging.nReason = (int)TrackMode::TRACK_MIDDLE;
            posChanging.dwPos = (DWORD)m_thumbs.first.position; // changes in both thumbs
        }
    }
    else
        return;

    ::SendMessage(GetParent()->GetSafeHwnd(), GetStyle() & TBS_VERT ? WM_VSCROLL : WM_HSCROLL, posChanging.dwPos, NULL);
    ::SendMessage(GetParent()->GetSafeHwnd(), WM_NOTIFY, posChanging.hdr.idFrom, reinterpret_cast<LPARAM>(&posChanging));
}

void CSlider::ShowSliderTooltip(bool left, bool createTip)
{
    CRect windowRect;
    GetWindowRect(&windowRect);

    const double controlWidthInPixels = (GetStyle() & TBS_VERT ? windowRect.Height() : windowRect.Width()) - 2 * (int)m_thumbWidth;
    const double countDataInOnePixel = (m_range.second - m_range.first) / std::max<double>(controlWidthInPixels, 1.);
    const double& sliderValue = left ? m_thumbs.first.position : m_thumbs.second.position;

    const LONG thumbDelta = left ? (LONG)m_thumbWidth * 2 / 3 : (LONG)m_thumbWidth * 4 / 3;
    const CRect& thumbRect = left ? m_thumbs.first.rect : m_thumbs.second.rect;

    m_tooltip.SetLabel(get_string(m_tooltipFormat, sliderValue).c_str());

    const CSize windowSize = m_tooltip.GetWindowSize();
    CRect toolWindowRect;
    if (GetStyle() & TBS_VERT)
    {
        toolWindowRect = CRect({ windowRect.right - 1, windowRect.top + thumbRect.CenterPoint().y }, windowSize);
        toolWindowRect.OffsetRect(0, -windowSize.cy / 2);
    }
    else
    {
        toolWindowRect = CRect({ windowRect.left + thumbRect.CenterPoint().x, windowRect.top - 1 }, windowSize);

        //toolWindowRect = CRect({ windowRect.left + thumbDelta + (LONG)(abs(sliderValue - m_range.first) / countDataInOnePixel), windowRect.top - 1 }, windowSize);
        toolWindowRect.OffsetRect(-windowSize.cx / 2, -windowSize.cy);
    }

    m_tooltip.SetWindowPos(NULL, toolWindowRect.left, toolWindowRect.top, toolWindowRect.Width(), toolWindowRect.Height(),
                           SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

double CSlider::ApplyIncrementStep(double oldValue, double newValue)
{
    if (!m_incrementStep.has_value())
        return newValue;
    return oldValue + *m_incrementStep * round((newValue - oldValue) / *m_incrementStep);
}

UINT CSlider::OnGetDlgCode()
{
    return DLGC_WANTARROWS;
}

void CSlider::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);

    const bool ctrlPressed = ::GetKeyState(VK_CONTROL) < 0;

    const UINT keyMoveLeft = GetStyle() & TBS_VERT ? VK_UP : VK_LEFT;
    const UINT keyMoveRight = GetStyle() & TBS_VERT ? VK_DOWN : VK_RIGHT;

    if ((nChar == keyMoveLeft || nChar == keyMoveRight) && !ctrlPressed)
    {
        const auto previousThumbPositions = std::make_pair(m_thumbs.first.position, m_thumbs.second.position);

        const double controlWidthInPixels = GetControlWidthInPixels();
        if (controlWidthInPixels == 0.0)
            return;
        double countDataInOnePixel = (m_range.second - m_range.first) / std::max<double>(controlWidthInPixels, 1.);
        if (m_incrementStep.has_value())
        {
            if (countDataInOnePixel <= *m_incrementStep)
                countDataInOnePixel = *m_incrementStep;
            else
                countDataInOnePixel = round(countDataInOnePixel / *m_incrementStep) * *m_incrementStep;
        }

        if (GetStyle() & TBS_ENABLESELRANGE)
        {
            const bool shiftPressed = ::GetKeyState(VK_SHIFT) < 0;
            if (nChar == keyMoveLeft)
            {
                if (!shiftPressed) // Shift not pressed => move interval
                    m_thumbs.first.position = std::max<double>(m_thumbs.first.position - countDataInOnePixel, m_range.first);

                m_thumbs.second.position = std::max<double>(m_thumbs.second.position - countDataInOnePixel, m_range.first);
            }
            else
            {
                if (!shiftPressed) // Shift not pressed => move interval
                    m_thumbs.first.position = std::min<double>(m_thumbs.first.position + countDataInOnePixel, m_range.second);

                m_thumbs.second.position = std::min<double>(m_thumbs.second.position + countDataInOnePixel, m_range.second);
            }
        }
        else
        {
            if (nChar == keyMoveLeft)
                m_thumbs.first.position = std::max<double>(m_thumbs.first.position - countDataInOnePixel, m_range.first);
            else
                m_thumbs.first.position = std::min<double>(m_thumbs.first.position + countDataInOnePixel, m_range.second);
        }

        if (previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position))
        {
            SendChangePositionEvent(previousThumbPositions);
            Invalidate();
        }
    }
}

void CSlider::OnTimer(UINT_PTR nIDEvent)
{
    CWnd::OnTimer(nIDEvent);

    if (nIDEvent != NULL)
        return;

    CPoint mousePos;
    if (!GetCursorPos(&mousePos))
        return;
    ScreenToClient(&mousePos);

    CRect clientRect;
    GetClientRect(&clientRect);
    if (!clientRect.PtInRect(mousePos))
        return;

    // Move cursor to mouse on line click
    const int controlWidthInPixels = (int)GetControlWidthInPixels();
    const double mousePoint = GetStyle() & TBS_VERT ? mousePos.y : mousePos.x;

    double valueAtMousePoint;
    if (mousePoint <= m_thumbWidth)
        valueAtMousePoint = m_range.first;
    else if (mousePoint >= controlWidthInPixels + m_thumbWidth)
        valueAtMousePoint = m_range.second;
    else
    {
        const double countDataInOnePixel = (m_range.second - m_range.first) / std::max<double>(controlWidthInPixels, 1.);
        valueAtMousePoint = m_range.first + double(mousePoint - (int)m_thumbWidth / 2) * countDataInOnePixel;
    }

    double moveCursorDistance = (m_range.second - m_range.first) / 10;
    if (m_incrementStep.has_value() && moveCursorDistance < *m_incrementStep)
        moveCursorDistance = *m_incrementStep;

    const auto previousThumbPositions = std::make_pair(m_thumbs.first.position, m_thumbs.second.position);;
    if (valueAtMousePoint < m_thumbs.first.position)
    {
        const auto newValue = ApplyIncrementStep(m_thumbs.first.position, m_thumbs.first.position - moveCursorDistance);
        if (newValue <= valueAtMousePoint)
        {
            m_thumbs.first.position = ApplyIncrementStep(m_thumbs.first.position, valueAtMousePoint);
            m_thumbs.first.state = ThumbInfo::State::eTracking;

            m_clickOffsetFormThumbCenter = { 0, 0 };
            KillTimer(NULL);
            ShowSliderTooltip(true, true);
        }
        else
            m_thumbs.first.position = newValue;
    }
    else if (valueAtMousePoint > m_thumbs.second.position)
    {
        const auto newValue = ApplyIncrementStep(m_thumbs.second.position, m_thumbs.second.position + moveCursorDistance);
        if (newValue >= valueAtMousePoint)
        {
            m_thumbs.second.position = ApplyIncrementStep(m_thumbs.second.position, valueAtMousePoint);
            m_thumbs.second.state = ThumbInfo::State::eTracking;

            m_clickOffsetFormThumbCenter = { 0, 0 };
            KillTimer(NULL);
            ShowSliderTooltip(false, true);
        }
        else
            m_thumbs.second.position = newValue;
    }

    if (previousThumbPositions != std::make_pair(m_thumbs.first.position, m_thumbs.second.position))
    {
        SendChangePositionEvent(previousThumbPositions);
        Invalidate();
    }
}

BOOL CSlider::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE; // remove flickering
}

void CSlider::PreSubclassWindow()
{
    CWnd::PreSubclassWindow();

    m_tooltip.Create(GetSafeHwnd(), true);
}
