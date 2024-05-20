#include <assert.h>

#include "TabControl.h"

BEGIN_MESSAGE_MAP(CTabControl, CTabCtrl)
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_NOTIFY_REFLECT_EX(TCN_SELCHANGE, &CTabControl::OnTcnSelchange)
END_MESSAGE_MAP()

void CTabControl::AutoResizeTabsToFitFullControlWidth(bool resize)
{
    m_resizeTabsToFitFullControlWidth = resize;

    ModifyStyle(0, TCS_FIXEDWIDTH);

    layoutCurrentWindow();
}

LONG CTabControl::AddTab(_In_z_ LPCTSTR lpszItem, _In_ const  std::shared_ptr<CWnd>& tabWindow)
{
    return CTabControl::InsertTab(GetItemCount(), lpszItem, tabWindow);
}

LONG CTabControl::AddTab(_In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CDialog>& tabDialog, UINT nIDTemplate)
{
    return CTabControl::InsertTab(GetItemCount(), lpszItem, tabDialog, nIDTemplate);
}

LONG CTabControl::InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CWnd>& tabWindow)
{
    LONG insertedIndex = CTabCtrl::InsertItem(nItem, lpszItem);

    m_tabWindows.insert(std::next(m_tabWindows.begin(), insertedIndex), tabWindow);

    // If inserted tab becomes current
    if (CTabCtrl::GetCurSel() == insertedIndex)
    {
        tabWindow->ShowWindow(SW_SHOW);
        tabWindow->SetFocus();

        layoutCurrentWindow();
    }

    return insertedIndex;
}

LONG CTabControl::InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CDialog>& tabDialog, UINT nIDTemplate)
{
    // create dialog if not created
    if (!::IsWindow(tabDialog->m_hWnd))
        tabDialog->Create(nIDTemplate, this);

    return InsertTab(nItem, lpszItem, tabDialog);
}

std::shared_ptr<CWnd> CTabControl::GetTabWindow(LONG index)
{
    ASSERT(index < (int)m_tabWindows.size());
    return *std::next(m_tabWindows.begin(), index);
}

BOOL CTabControl::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

void CTabControl::OnSize(UINT nType, int cx, int cy)
{
    CTabCtrl::OnSize(nType, cx, cy);

    layoutCurrentWindow();
}

int CTabControl::GetCurSel() const
{
   return CTabCtrl::GetCurSel();
}

int CTabControl::SetCurSel(_In_ int nItem)
{
    const auto res = CTabCtrl::SetCurSel(nItem);
    onSelChanged();
    return res;
}

int CTabControl::GetItemCount() const
{
   return CTabCtrl::GetItemCount();
}

BOOL CTabControl::DeleteTab(_In_ int nItem)
{
    const auto res = CTabCtrl::DeleteItem(nItem);
    if (res == TRUE)
    {
        ASSERT(nItem < (int)m_tabWindows.size());
        auto tabIt = std::next(m_tabWindows.begin(), nItem);
        tabIt->get()->DestroyWindow();
        m_tabWindows.erase(tabIt);
    }

    return res;
}

BOOL CTabControl::DeleteAllItems()
{
    const auto res = CTabCtrl::DeleteAllItems();
    if (res == TRUE)
        m_tabWindows.clear();

    return res;
}

BOOL CTabControl::OnTcnSelchange(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    onSelChanged();
    *pResult = 0;
    return FALSE;
}

void CTabControl::onSelChanged()
{
    const int curSel = CTabCtrl::GetCurSel();

    SetRedraw(FALSE);

    // Переключаем активную вкладку
    int curIndex = 0;
    for (auto& windowTab : m_tabWindows)
    {
        if (curIndex == curSel)
        {
            windowTab->ShowWindow(SW_SHOW);
            windowTab->SetFocus();

            layoutCurrentWindow();
        }
        else
            windowTab->ShowWindow(SW_HIDE);
        ++curIndex;
    }

    SetRedraw(TRUE);
}

void CTabControl::layoutCurrentWindow()
{
    int curSel = CTabCtrl::GetCurSel();
    if (curSel == -1)
        return;

    ASSERT(curSel < (int)m_tabWindows.size());
    auto window = *std::next(m_tabWindows.begin(), curSel);

    // рассчитываем позицию где должен быть диалог
    CRect lpRect;
    CTabCtrl::GetItemRect(curSel, &lpRect);
    CRect clRc;
    CTabCtrl::GetClientRect(&clRc);
    auto controlWidth = clRc.Width();
    clRc.top += lpRect.bottom;
    clRc.InflateRect(-1, -1);
    clRc.right -= 2;
    clRc.bottom--;

    window->MoveWindow(&clRc, TRUE);

    if (m_resizeTabsToFitFullControlWidth)
    {
        CRect rectItem;
        if (GetItemRect(0, &rectItem))
        {
            auto scrollHwnd = ::FindWindowEx(m_hWnd, NULL, UPDOWN_CLASS, NULL);

            do
            {
                SetItemSize(CSize((controlWidth - 2) / CTabCtrl::GetItemCount(), rectItem.Height()));
            } while (scrollHwnd && ::IsWindowVisible(scrollHwnd) && (--controlWidth > 0));
        }
    }
}
