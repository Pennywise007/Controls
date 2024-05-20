#include <afxcontrolbarutil.h>
#include <algorithm>
#include <math.h>
#include <typeinfo>
#include <limits>
#include <iomanip>
#include <sstream>

#include "SpinButton.h"

#include "../Utils/WindowClassRegistration.h"

#undef max

namespace {

constexpr UINT kDeltaPosTimerId = 1;

constexpr UINT kTimerTickSpeedFirst = 600;
constexpr UINT kTimerTickSpeedSecond = 400;
constexpr UINT kTimerTickSpeedThird = 200;
constexpr UINT kTimerTickSpeedMax = 100;

template<class T>
T change_spin_val(_In_ bool increament, _In_ T currentVal, _In_ size_t& countChanges, _Inout_ size_t& changingsInOrder)
{
    long long changingNumberInValue = 0;
    long long movingValue = (long long)pow(10ll, (long long)changingsInOrder);

    if (movingValue <= abs(currentVal))
        changingNumberInValue = ((long long)currentVal / movingValue) % 10ll;

    const long long increamentLastVal = changingNumberInValue < 0 ? 0 : 10;
    const long long dereamentLastVal = changingNumberInValue < 0 ? -10 : 0;

    const long long nextLastVal = increament ? increamentLastVal : dereamentLastVal;
    long long increamentVal = increament ? 1 : -1;

    if (countChanges % 10 >= 7 && changingNumberInValue + increamentVal == nextLastVal)
    {
        // helping to switch values on borders
        countChanges += 10 - countChanges % 10;
        ++changingsInOrder;

        increamentVal = nextLastVal - changingNumberInValue;
    }
    else if (countChanges % 10 == 0)
    {
        if (changingNumberInValue + increamentVal != nextLastVal)
        {
            // wait for switching current value
            --countChanges;
        }
        else
            ++changingsInOrder;
    }

    return currentVal + increamentVal * movingValue;
}

} // namespace

BEGIN_MESSAGE_MAP(CSpinButton, CWnd)
    ON_NOTIFY_REFLECT_EX(UDN_DELTAPOS, &CSpinButton::OnDeltapos)
    ON_WM_TIMER()
    ON_WM_ENABLE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CSpinButton::CSpinButton()
    : m_hTheme(OpenThemeData(m_hWnd, L"Spin"))
{
    ASSERT(m_hTheme);
}

CSpinButton::~CSpinButton()
{
    if (m_hTheme)
        CloseThemeData(m_hTheme);
}

void CSpinButton::SetRange(_In_ double Left, _In_ double Right)
{
    m_spinRange = std::make_pair(Left, Right);
}

void CSpinButton::GetRange(_Out_ double& Left, _Out_ double& Right) const
{
    Left  = m_spinRange.first;
    Right = m_spinRange.second;
}

void CSpinButton::SetBuddy(_In_ CWnd* pWndBuddy)
{
    m_linkedBuddy = pWndBuddy;
}

CWnd* CSpinButton::GetBuddy() const
{
    return m_linkedBuddy;
}

BOOL CSpinButton::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    HINSTANCE instance = AfxGetInstanceHandle();
    const CString className(typeid(*this).name());

    // регистрируем наш клас
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, className, &wndClass))
    {
        // Регистрация класса окна которое используется для редактирования ячеек
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = className;

        static WindowClassRegistrationLock registration(wndClass);
    }

    return CWnd::Create(className, className, dwStyle, rect, pParentWnd, nID, pContext);
}

CRect CSpinButton::GetUpRect() const
{
    CRect rect;
    GetClientRect(rect);
    rect.bottom -= rect.Height() / 2;
    return rect;
}

CRect CSpinButton::GetDownRect() const
{
    CRect rect;
    GetClientRect(rect);
    rect.top = rect.bottom - rect.Height() / 2;
    return rect;
}

CSpinButton::HitTest CSpinButton::GetHitTest() const
{
    CPoint cursor;
    ::GetCursorPos(&cursor);
    CWnd::ScreenToClient(&cursor);

    const auto& upRect = GetUpRect();
    if (GetUpRect().PtInRect(cursor))
        return HitTest::eUp;
    else if (GetDownRect().PtInRect(cursor))
        return HitTest::eDown;
    else
        return HitTest::eUnknown;
}

void CSpinButton::RestartTimer()
{
    m_changeValueCount = 0;
    m_changeValueOrder = 0;

    CWnd::KillTimer(kDeltaPosTimerId);
    CWnd::SetTimer(kDeltaPosTimerId, kTimerTickSpeedFirst, nullptr);
    OnTimer(kDeltaPosTimerId);
}

void CSpinButton::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_buttonDown = GetHitTest();
    CWnd::OnLButtonDown(nFlags, point);

    m_timerTickCount = 0;
    if (CWnd::IsWindowEnabled() == TRUE && m_buttonDown != HitTest::eUnknown)
        RestartTimer();

    CWnd::SetCapture();
}

void CSpinButton::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnLButtonDown(nFlags, point);

    CWnd::OnLButtonDblClk(nFlags, point);
}

void CSpinButton::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_buttonDown = HitTest::eUnknown;
    CWnd::KillTimer(kDeltaPosTimerId);
    CWnd::OnLButtonUp(nFlags, point);
    CWnd::RedrawWindow();
    ::ReleaseCapture();
}

BOOL CSpinButton::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CSpinButton::OnPaint()
{
    CRect rect;
    GetClientRect(rect);

    if (rect.IsRectEmpty())
        return;

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, rect);
    CDC& dc = memDC.GetDC();

    dc.FillSolidRect(rect, GetBkColor(dc.m_hDC));

    const UPSTATES stateUp = [&]()
    {
        if (CWnd::IsWindowEnabled() == FALSE)
            return UPS_DISABLED;
        if (m_buttonDown == HitTest::eUp)
            return UPS_PRESSED;
        if (m_hoveredButton == HitTest::eUp && m_buttonDown != HitTest::eDown)
            return UPS_HOT;
        return UPS_NORMAL;
    }();
    const DOWNSTATES stateDown = [&]()
    {
        if (CWnd::IsWindowEnabled() == FALSE)
            return DNS_DISABLED;
        if (m_buttonDown == HitTest::eDown)
            return DNS_PRESSED;
        if (m_hoveredButton == HitTest::eDown && m_buttonDown != HitTest::eUp)
            return DNS_HOT;
        return DNS_NORMAL;
    }();

    DrawThemeBackground(m_hTheme, dc.m_hDC, SPNP_UP, stateUp, GetUpRect(), 0);
    DrawThemeBackground(m_hTheme, dc.m_hDC, SPNP_DOWN, stateDown, GetDownRect(), 0);
}

void CSpinButton::OnMouseMove(UINT nFlags, CPoint point)
{
    CWnd::OnMouseMove(nFlags, point);

    if (m_hoveredButton == HitTest::eUnknown)
    {
        TRACKMOUSEEVENT MouseBehaviour;
        MouseBehaviour.cbSize = sizeof(TRACKMOUSEEVENT);
        MouseBehaviour.dwFlags = TME_HOVER | TME_LEAVE;
        MouseBehaviour.hwndTrack = GetSafeHwnd();
        MouseBehaviour.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&MouseBehaviour);
    }

    const auto curHitTest = GetHitTest();
    if (m_hoveredButton != curHitTest)
    {
        m_hoveredButton = curHitTest;
        CWnd::RedrawWindow();

        if (m_hoveredButton != m_buttonDown)
            CWnd::KillTimer(kDeltaPosTimerId);
        else
            RestartTimer();
    }
}

void CSpinButton::OnMouseLeave()
{
    m_hoveredButton = HitTest::eUnknown;
    CWnd::RedrawWindow();
    CWnd::KillTimer(kDeltaPosTimerId);

    CWnd::OnMouseLeave();
}

void CSpinButton::OnTimer(UINT_PTR nIDEvent)
{
    const auto processUpDown = [&]()
    {
        NMUPDOWN upDown;
        upDown.hdr.hwndFrom = m_hWnd;
        upDown.hdr.idFrom = GetWindowLong(m_hWnd, GWL_ID);
        upDown.hdr.code = (UINT)UDN_DELTAPOS;
        upDown.iDelta = m_buttonDown == HitTest::eDown ? -1 : 1;
        upDown.iPos = 0;

        if (::SendMessage(GetParent()->GetSafeHwnd(), WM_NOTIFY, upDown.hdr.idFrom, reinterpret_cast<LPARAM>(&upDown)) == TRUE)
            return;
    };

    processUpDown();

    switch (m_timerTickCount)
    {
    case 2:
        CWnd::KillTimer(kDeltaPosTimerId);
        CWnd::SetTimer(kDeltaPosTimerId, kTimerTickSpeedSecond, nullptr);
        ++m_timerTickCount;
        break;
    case 5:
        CWnd::KillTimer(kDeltaPosTimerId);
        CWnd::SetTimer(kDeltaPosTimerId, kTimerTickSpeedThird, nullptr);
        ++m_timerTickCount;
        break;
    case 11:
        CWnd::KillTimer(kDeltaPosTimerId);
        CWnd::SetTimer(kDeltaPosTimerId, kTimerTickSpeedMax, nullptr);
        ++m_timerTickCount;
        break;
    default:
        constexpr auto maxCount = std::numeric_limits<decltype(m_timerTickCount)>::max();
        if (m_timerTickCount < maxCount)
            ++m_timerTickCount;
        break;
    }
    CWnd::OnTimer(nIDEvent);
}

BOOL CSpinButton::OnDeltapos(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

    CString editText;
    GetBuddy()->GetWindowTextW(editText);

    const double oldVal = _wtof(editText);
    const double newVal = std::clamp(change_spin_val(pNMUpDown->iDelta > 0, oldVal, ++m_changeValueCount, m_changeValueOrder), m_spinRange.first, m_spinRange.second);
    if (newVal != oldVal)
    {
        std::wostringstream stream;
        stream << std::setprecision(std::numeric_limits<double>::digits10) << newVal;
        GetBuddy()->SetWindowTextW(stream.str().c_str());
    }

    *pResult = 1;
    return TRUE;
}

void CSpinButton::OnEnable(BOOL bEnable)
{
    CWnd::OnEnable(bEnable);
    Invalidate();
}
