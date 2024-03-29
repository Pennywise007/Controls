﻿#include "Afxglobals.h"
#include "TextProgressCtrl.h"

IMPLEMENT_DYNAMIC(TextProgressCtrl, CProgressCtrl )

BEGIN_MESSAGE_MAP(TextProgressCtrl, CProgressCtrl )
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

TextProgressCtrl::~TextProgressCtrl()
{
    // Free resources after GetITaskbarList3
    AfxGetApp()->ReleaseTaskBarRefs();
}

void TextProgressCtrl::OnPaint()
{
    CClientDC	dc( this );
    CRect		rc;

    CProgressCtrl::OnPaint();

    // Устанавливаем НОРМАЛЬНЫЙ шрифт
    CFont* pOldFont = dc.SelectObject( GetParent()->GetFont() );
    GetClientRect( &rc );
    // Рисуем текст
    dc.SetBkMode( TRANSPARENT );

    CString	showText;
    if (!m_outputFormat.empty())
        showText.Format(m_outputFormat.c_str(), CProgressCtrl::GetPos());
    else
        CProgressCtrl::GetWindowTextW(showText);

    dc.DrawText( showText, rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER );

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
    auto pTaskbarList = afxGlobalData.GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressValue( GetParent()->GetSafeHwnd(), 0, range );
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_NOPROGRESS );
}

void TextProgressCtrl::SetPosition( int pos )
{
    SetPos( pos );

    // Таскбар Win7
    auto pTaskbarList = afxGlobalData.GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    int _min, _max; GetRange( _min, _max );
    pTaskbarList->SetProgressValue( GetParent()->GetSafeHwnd(), pos, _max );
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_NORMAL );
}

void TextProgressCtrl::SetIndeterminate( BOOL bInf ) const
{
    // Таскбар Win7
    auto pTaskbarList = afxGlobalData.GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), bInf ? TBPF_INDETERMINATE : TBPF_NOPROGRESS );
}

void TextProgressCtrl::Pause() const
{
    // Таскбар Win7
    auto pTaskbarList = afxGlobalData.GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_PAUSED );
}

void TextProgressCtrl::Error() const
{
    // Таскбар Win7
    auto pTaskbarList = afxGlobalData.GetITaskbarList3();
    if (NULL == pTaskbarList) return;
    pTaskbarList->SetProgressState( GetParent()->GetSafeHwnd(), TBPF_ERROR );
}

void TextProgressCtrl::SetOutputFormat(const std::wstring& format)
{
    m_outputFormat = format;
}
