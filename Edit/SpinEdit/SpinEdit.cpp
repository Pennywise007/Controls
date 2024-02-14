#include <afxcontrolbarutil.h>
#include <algorithm>
#include <math.h>

#include "SpinEdit.h"


namespace {

constexpr auto kDefaultSpinWidth = 19;

} // namespace

BEGIN_MESSAGE_MAP(CSpinEdit, CEditBase)
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_CREATE()
    ON_WM_SHOWWINDOW()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_ENABLE()
END_MESSAGE_MAP()

CSpinEdit::CSpinEdit(_In_opt_ UINT controlID /* = kUndefinedControlId*/)
    : CEditBase		(true)
    , m_spinAlign	(spin_edit::SpinAlign::RIGHT)
    , m_spinWidth	(kDefaultSpinWidth)
    , m_bFromCreate	(false)
    , m_spinCtrlID	(controlID)
    , m_spinCtrl    (std::make_unique<CSpinButton>())
    , m_spinRange   (std::make_pair(-DBL_MAX, DBL_MAX))
{}

CSpinEdit::~CSpinEdit()
{
    if (IsWindow(*m_spinCtrl))
    {
        m_spinCtrl->DestroyWindow();
    }
}

void CSpinEdit::InitEdit()
{
    if (IsWindow(*m_spinCtrl))
        m_spinCtrl->DestroyWindow();

    DWORD Style = CEditBase::GetStyle() & WS_VISIBLE ? WS_VISIBLE : 0;
    Style |= WS_CHILD | UDS_SETBUDDYINT | UDS_HOTTRACK | UDS_NOTHOUSANDS;
    // создаем спинконтрол
    m_spinCtrl->Create(Style, CRect(), CEditBase::GetParent(),
                       m_spinCtrlID == kUndefinedControlId ? CEditBase::GetDlgCtrlID() : m_spinCtrlID);
    m_spinCtrl->SetBuddy(this);
    m_spinCtrl->SetRange(m_spinRange.first, m_spinRange.second);

    // сдвигаем основное окно чтобы размер окна со спином остался прежним
    CRect Rect;
    CEditBase::GetWindowRect(Rect);
    GetParent()->ScreenToClient(Rect);
    if (m_spinAlign == spin_edit::SpinAlign::RIGHT)
        Rect.right	-= m_spinWidth;
    else
        Rect.left	+= m_spinWidth;
    CEditBase::MoveWindow(Rect);

    ReposCtrl(Rect);
}

void CSpinEdit::PreSubclassWindow()
{
    if (!m_bFromCreate)
        InitEdit();
    m_bFromCreate = false;
    CEditBase::PreSubclassWindow();
}

void CSpinEdit::SetSpinAlign(_In_opt_  spin_edit::SpinAlign Align /*=  spin_edit::SpinAligns::RIGHT*/)
{
    if (m_spinAlign != Align)
    {
        CRect EditRect;
        GetWindowRect(EditRect);
        CEditBase::GetParent()->ScreenToClient(EditRect);

        m_spinAlign = Align;
        CEditBase::MoveWindow(EditRect);
    }
}

void CSpinEdit::SetRange(_In_ double Left, _In_ double Right)
{
    if (IsWindow(*m_spinCtrl))
        m_spinCtrl->SetRange(CEditBase::m_bUsePositivesDigitsOnly ? max(0, Left) : Left, Right);
    m_spinRange = std::make_pair(Left, Right);

    CEditBase::SetUseCtrlLimits(true);
    CEditBase::SetMinMaxLimits(CEditBase::m_bUsePositivesDigitsOnly ? max(0, Left) : Left, Right);
}

void CSpinEdit::GetRange(_Out_ double& Left, _Out_ double& Right) const
{
    Left = m_spinRange.first;
    Right = m_spinRange.second;
}

void CSpinEdit::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    lpwndpos->cx -= m_spinWidth;

    if (m_spinAlign == spin_edit::SpinAlign::LEFT)
        lpwndpos->x += m_spinWidth;

    CEditBase::OnWindowPosChanging(lpwndpos);
}

void CSpinEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CEditBase::OnWindowPosChanged(lpwndpos);

    // масштабируем контрол и спин
    ReposCtrl(CRect(lpwndpos->x, lpwndpos->y, lpwndpos->x + lpwndpos->cx, lpwndpos->y + lpwndpos->cy));
}

void CSpinEdit::SetSpinWidth(_In_ int NewWidth)
{
    CRect Rect;
    GetWindowRect(Rect);
    CEditBase::GetParent()->ScreenToClient(Rect);

    m_spinWidth = NewWidth;
    CEditBase::MoveWindow(Rect);
}

void CSpinEdit::ReposCtrl(_In_opt_ const CRect& EditRect)
{
    CRect SpinRect = EditRect;

    if (m_spinAlign == spin_edit::SpinAlign::LEFT)
    {
        SpinRect.right = SpinRect.left;
        SpinRect.left -= m_spinWidth;
    }
    else
    {
        SpinRect.left = SpinRect.right;
        SpinRect.right += m_spinWidth;
    }
    m_spinCtrl->MoveWindow(SpinRect);
    m_spinCtrl->RedrawWindow();
}

BOOL CSpinEdit::PreCreateWindow(CREATESTRUCT& cs)
{
    m_bFromCreate = true;
    return CEditBase::PreCreateWindow(cs);
}

int CSpinEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CEditBase::OnCreate(lpCreateStruct) == -1)
        return -1;

    InitEdit();
    return 0;
}

void CSpinEdit::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CEditBase::OnShowWindow(bShow, nStatus);
    m_spinCtrl->ShowWindow(bShow);
}

void CSpinEdit::GetWindowRect(LPRECT lpRect) const
{
    CEditBase::GetWindowRect(lpRect);
    if (m_spinAlign == spin_edit::SpinAlign::LEFT)
        lpRect->left -= m_spinWidth;
    else
        lpRect->right += m_spinWidth;
}

void CSpinEdit::GetClientRect(LPRECT lpRect) const
{
    CEditBase::GetClientRect(lpRect);
    lpRect->right += m_spinWidth;
}

void CSpinEdit::UsePositiveDigitsOnly(_In_opt_ bool bUsePositiveDigitsOnly /*= true*/)
{
    if (bUsePositiveDigitsOnly)
        m_spinCtrl->SetRange(std::max<double>(0., m_spinRange.first), m_spinRange.second);
    else
        m_spinCtrl->SetRange(m_spinRange.first, m_spinRange.second);

    CEditBase::UsePositiveDigitsOnly(bUsePositiveDigitsOnly);
}

void CSpinEdit::OnEnable(BOOL bEnable)
{
    CEditBase::OnEnable(bEnable);

    m_spinCtrl->EnableWindow(bEnable);
}
