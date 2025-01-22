#include "Afxglobals.h"
#include "TextProgressCtrl.h"

#include "Controls/ThemeManagement.h"

#include <uxtheme.h>

#pragma comment(lib, "uxtheme.lib")

IMPLEMENT_DYNAMIC(TextProgressCtrl, CProgressCtrl)

BEGIN_MESSAGE_MAP(TextProgressCtrl, CProgressCtrl)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

TextProgressCtrl::~TextProgressCtrl()
{
    struct TaskBarReleaser {
        ~TaskBarReleaser()
        {
            // Free resources after GetITaskbarList3
            AfxGetApp()->ReleaseTaskBarRefs();
        }
    };


    if (m_taskBarChanged)
    {
        // Hiding task bar changes
        auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
        if (NULL == pTaskbarList) return;
        pTaskbarList->SetProgressState(GetParent()->GetSafeHwnd(), TBPF_NOPROGRESS);
    }

    const static TaskBarReleaser releaser;
}

void TextProgressCtrl::OnPaint()
{
    CRect rcClient;
    GetClientRect(&rcClient);

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, rcClient);
    CDC& dc = memDC.GetDC();

    BOOL isMarquee = (GetStyle() & PBS_MARQUEE) != 0;
    if (isMarquee)
    {
        CProgressCtrl::DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
    }
    else
    {
        // The default ProgressCtrl draw the progress with delay, which we don't want, draw real position
        const static ThemeHolder theme(m_hWnd, L"Progress");

        DrawThemeBackground(theme, dc.GetSafeHdc(), PP_BAR, 0, &rcClient, nullptr);

        // Fill the progress bar
        int nPos = GetPos();
        int nMin = 0, nMax = 100;
        GetRange(nMin, nMax);

        double fraction = static_cast<double>(nPos - nMin) / (nMax - nMin);
        CRect rcFill = rcClient;
        rcFill.right = rcFill.left + static_cast<int>(fraction * rcClient.Width());

        DrawThemeBackground(theme, dc.GetSafeHdc(), PP_CHUNK, 0, &rcFill, nullptr);
    }

    // Устанавливаем НОРМАЛЬНЫЙ шрифт
    CFont* pOldFont = dc.SelectObject( GetParent()->GetFont() );
    // Рисуем текст
    dc.SetBkMode( TRANSPARENT );

    CString	showText;
    if (!m_outputFormat.empty())
        showText.Format(m_outputFormat.c_str(), CProgressCtrl::GetPos());
    else
        CProgressCtrl::GetWindowTextW(showText);

    dc.DrawText( showText, rcClient, DT_SINGLELINE | DT_CENTER | DT_VCENTER );

    // Восстанавливаем предыдущий шрифт
    dc.SelectObject( pOldFont );
}

BOOL TextProgressCtrl::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void TextProgressCtrl::SetZeroRange( short range )
{
    SetRange( 0, range );
    SetPos( 0 );

    // Таскбар Win7
    auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressValue( GetParent()->GetSafeHwnd(), 0, range );
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_NOPROGRESS );
}

void TextProgressCtrl::SetPosition(int pos)
{
    SetPos(pos);

    // Таскбар Win7
    auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    int _min, _max; GetRange( _min, _max );
    m_taskBarChanged = true;
    pTaskbarList->SetProgressValue( GetParent()->GetSafeHwnd(), pos, _max );
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_NORMAL );
}

void TextProgressCtrl::SetIndeterminate( BOOL bInf )
{
    // Таскбар Win7
    auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), bInf ? TBPF_INDETERMINATE : TBPF_NOPROGRESS );
    m_taskBarChanged = bInf;
}

void TextProgressCtrl::Pause()
{
    // Таскбар Win7
    auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    m_taskBarChanged = true;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_PAUSED );
}

void TextProgressCtrl::Error()
{
    // Таскбар Win7
    auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    m_taskBarChanged = true;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_ERROR );
}

void TextProgressCtrl::SetOutputFormat(const std::wstring& format)
{
    m_outputFormat = format;
}

void TextProgressCtrl::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CProgressCtrl::OnShowWindow(bShow, nStatus);

    if (m_taskBarChanged && !bShow)
    {
        // Hiding task bar changes
        auto pTaskbarList = GetGlobalData()->GetITaskbarList3();
        if (NULL == pTaskbarList) return;
        pTaskbarList->SetProgressState(GetParent()->GetSafeHwnd(), TBPF_NOPROGRESS);
        m_taskBarChanged = false;
    }
}
