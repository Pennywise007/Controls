#pragma once

#include <afxcmn.h>
#include <list>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// Extension for control with tabs allows you to insert dialogs into tabs
// and switch between them / place them in place and size them together with the control
class CTabControl : protected CTabCtrl
{
public:
    CTabControl() = default;

    operator CWnd&() { return *this; }
    CWnd& operator()() { return *this; }

public:
    LONG AddTab(_In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CWnd>& tabWindow);
    LONG AddTab(_In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CDialog>& tabDialog, UINT nIDTemplate);

    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CWnd>& tabWindow);
    LONG InsertTab(_In_ int nItem, _In_z_ LPCTSTR lpszItem, _In_ const std::shared_ptr<CDialog>& tabDialog, UINT nIDTemplate);

    int GetCurSel() const;
    int SetCurSel(_In_ int nItem);

    int GetItemCount() const;
    
    BOOL DeleteTab(_In_ int nItem);
    BOOL DeleteAllItems();

    /// <summary>
    /// Get window from tab control by index
    /// </summary>
    /// <param name="index">Tab index</param>
    /// <returns>Inserted in tab window by requested index</returns>
    std::shared_ptr<CWnd> GetTabWindow(LONG index);

public:
    DECLARE_MESSAGE_MAP()

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);

private:
    // positions the currently active window in the client tab area
    void layoutCurrentWindow();

    void onSelChanged();

private:
    std::list<std::shared_ptr<CWnd>> m_tabWindows;
};