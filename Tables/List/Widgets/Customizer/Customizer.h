#pragma once

#include "afxcmn.h"

#include <optional>
#include <map>

#include "Controls/Tables/List/SubItemsInfo.h"

namespace controls::list::widgets {

class CHeaderCtrlEx : public CHeaderCtrl
{
public:
    void SetRealFont(CFont* font);
    void SetColumnTooltip(int column, CString&& text);
    CString GetColumnTooltip(int column) const;

public:
    DECLARE_MESSAGE_MAP()
    afx_msg BOOL OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const override;

protected:
    HEADERITEMSTATES GetItemState(LPNMLVCUSTOMDRAW pNMCD);

    void DrawItem(LPNMLVCUSTOMDRAW pNMCD);
    void DrawSortArrow(LPNMLVCUSTOMDRAW pNMCD, HTHEME hTheme, bool ascending) const;
    void DrawCheckbox(LPNMLVCUSTOMDRAW pNMCD, bool checked) const;
    void DrawItemText(LPNMLVCUSTOMDRAW pNMCD, HTHEME hTheme, HEADERITEMSTATES state, const HDITEM& hdi) const;

protected:
    CFont* m_realFont = CFont::FromHandle(HFONT(GetStockObject(DEFAULT_GUI_FONT)));
    std::map<int, CString> m_tooltipByColumns;
};

//****************************************************************************//
/*
    Extended class above the list, can be used as a widget for a table

    Allows:
    1. Make multi-line column headers
    2. Set colors for cells
    3. Set tooltips on full columns and special cells
*/
template <typename CBaseList = CListCtrl>
class Customizer : public CBaseList
{
public:
    void SetMutilineHeader(bool multiline);

    void SetColor(int item, int subItem, std::optional<COLORREF> backColor, std::optional<COLORREF> textColor);
    void SetTooltip(int item, int subItem, CString text);
    void SetTooltipOnColumn(int column, CString text);

private:
    int GetRealItemIndex(int item);

protected:
    DECLARE_MESSAGE_MAP()
    virtual void PreSubclassWindow() override;
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
    virtual BOOL OnChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
    afx_msg BOOL OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const override;

protected:
    struct SubItemInfo
    {
        std::optional<COLORREF> backColor;
        std::optional<COLORREF> textColor;
        CString tooltip;
    };
    SubItemsInfo<SubItemInfo> m_subItemInfo;

    CFont m_NewHeaderFont;
    CHeaderCtrlEx m_HeaderCtrl;
};

} // namespace controls::list::widgets

#include "CustomizerImpl.h"