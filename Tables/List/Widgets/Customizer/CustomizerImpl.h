#pragma once

#include "Controls/ThemeManagement.h"

#include "Customizer.h"

namespace controls::list::widgets {

#define BEGIN_MESSAGE_MAP_INLINE(theClass, baseClass) \
	PTM_WARNING_DISABLE \
	inline const AFX_MSGMAP* theClass::GetMessageMap() const \
		{ return GetThisMessageMap(); } \
	inline const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap() \
	{ \
		typedef theClass ThisClass;						   \
		typedef baseClass TheBaseClass;					   \
		__pragma(warning(push))							   \
		__pragma(warning(disable: 4640)) /* message maps can only be called by single threaded message pump */ \
		static const AFX_MSGMAP_ENTRY _messageEntries[] =  \
		{

BEGIN_MESSAGE_MAP_INLINE(CHeaderCtrlEx, CHeaderCtrl)
    ON_NOTIFY_REFLECT_EX(NM_CUSTOMDRAW, &CHeaderCtrlEx::OnNMCustomdraw)
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

#undef BEGIN_MESSAGE_MAP_INLINE

inline void CHeaderCtrlEx::SetRealFont(CFont* font)
{
    m_realFont = font;
}

inline void CHeaderCtrlEx::SetColumnTooltip(int column, CString&& text)
{
    if (!text.IsEmpty())
        m_tooltipByColumns[column] = std::move(text);
    else
        m_tooltipByColumns.erase(column);

    if (m_tooltipByColumns.empty())
        CHeaderCtrl::CancelToolTips();
    else
        CHeaderCtrl::EnableToolTips();
}

inline CString CHeaderCtrlEx::GetColumnTooltip(int column) const
{
    auto it = m_tooltipByColumns.find(column);
    if (it == m_tooltipByColumns.end())
        return {};
    return it->second;
}

inline BOOL CHeaderCtrlEx::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

    switch (pNMCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;          // ask for subitem notifications.
        return TRUE;

    case CDDS_ITEMPREPAINT:
        DrawItem(pNMCD);
        *pResult = CDRF_SKIPDEFAULT;
        return TRUE;

    default:
        break;
    }

    *pResult = CDRF_DODEFAULT;
    return FALSE;
}

inline void CHeaderCtrlEx::OnMouseMove(UINT flags, CPoint point)
{
    CHeaderCtrl::OnMouseMove(flags, point);
    Invalidate(); // redraw after changing hot item
}

inline INT_PTR CHeaderCtrlEx::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
    HDHITTESTINFO hdHitTestInfo;
    hdHitTestInfo.pt = std::move(point);

    const auto hitItem = const_cast<CHeaderCtrlEx&>(*this).HitTest(&hdHitTestInfo);
    const auto it = m_tooltipByColumns.find(hitItem);
    if (it != m_tooltipByColumns.end())
    {
        ASSERT(!it->second.IsEmpty());

        pTI->lpszText = (LPWSTR)malloc((it->second.GetLength() + 1) * sizeof(wchar_t));
        lstrcpyn(pTI->lpszText, it->second, it->second.GetLength() + 1);

        pTI->hwnd = m_hWnd;
        pTI->uId = hitItem;

        CHeaderCtrl::GetItemRect(hitItem, &pTI->rect);
        return pTI->uId;
    }

    return CHeaderCtrl::OnToolHitTest(point, pTI);
}

inline HEADERITEMSTATES CHeaderCtrlEx::GetItemState(LPNMLVCUSTOMDRAW pNMCD)
{
    if (IsWindowEnabled() == TRUE)
    {
        if (pNMCD->nmcd.uItemState == CDIS_SELECTED)
            return HIS_PRESSED;

        CPoint ptCursor;
        ::GetCursorPos(&ptCursor);
        ScreenToClient(&ptCursor);

        HDHITTESTINFO hdHitTestInfo;
        hdHitTestInfo.pt = ptCursor;

        const auto hitItem = HitTest(&hdHitTestInfo);
        bool bIsHighlighted = hitItem == pNMCD->nmcd.dwItemSpec && (hdHitTestInfo.flags & HHT_ONHEADER);
        if (bIsHighlighted)
            return HIS_HOT;
    }
    return HIS_NORMAL;
}

inline void CHeaderCtrlEx::DrawItem(LPNMLVCUSTOMDRAW pNMCD)
{
    const static ThemeHolder headerTheme(m_hWnd, L"HEADER");
    const HEADERITEMSTATES itemState = GetItemState(pNMCD);

    VERIFY(SUCCEEDED(DrawThemeBackground(headerTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                                         itemState, &pNMCD->nmcd.rc, 0)));

    TCHAR itemText[256];

    HDITEM hdi = { 0 };
    hdi.mask = HDI_TEXT | HDI_FORMAT;
    hdi.pszText = itemText;
    hdi.cchTextMax = sizeof(itemText) - 1;
    VERIFY(GetItem(pNMCD->nmcd.dwItemSpec, &hdi));

    if (hdi.fmt & HDF_SORTDOWN)
        DrawSortArrow(pNMCD, headerTheme, false);
    else if (hdi.fmt & HDF_SORTUP)
        DrawSortArrow(pNMCD, headerTheme, true);

    // see CMFCHeaderCtrl::OnDrawItem
    ASSERT(!(hdi.fmt & HDF_IMAGE));

    ::InflateRect(&pNMCD->nmcd.rc, -2, -2);

    if (hdi.fmt & HDF_CHECKBOX)
        DrawCheckbox(pNMCD, hdi.fmt & HDF_CHECKED);

    DrawItemText(pNMCD, headerTheme, itemState, hdi);
}

inline void CHeaderCtrlEx::DrawSortArrow(LPNMLVCUSTOMDRAW pNMCD,
                                         HTHEME hTheme,
                                         bool ascending) const
{
    CRect rect = pNMCD->nmcd.rc;
    rect.bottom = rect.top + 5;
    VERIFY(SUCCEEDED(DrawThemeBackground(hTheme, pNMCD->nmcd.hdc, HP_HEADERSORTARROW,
                                         ascending ? HSAS_SORTEDUP : HSAS_SORTEDDOWN, rect, 0)));
}

inline void CHeaderCtrlEx::DrawCheckbox(LPNMLVCUSTOMDRAW pNMCD,
                                        bool checked) const
{
    const static ThemeHolder checkboxTheme(m_hWnd, L"BUTTON");

    CHECKBOXSTATES state;
    if (!IsWindowEnabled())
        state = checked ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
    else
        state = checked ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;

    CRect rect = pNMCD->nmcd.rc;
    rect.right = rect.left + 18;

    VERIFY(SUCCEEDED(DrawThemeBackground(checkboxTheme, pNMCD->nmcd.hdc, BP_CHECKBOX,
                                         state, rect, 0)));

    pNMCD->nmcd.rc.left = rect.right;
}

inline void CHeaderCtrlEx::DrawItemText(LPNMLVCUSTOMDRAW pNMCD, HTHEME headerTheme,
                                        HEADERITEMSTATES itemState, const HDITEM& hdi) const
{
    CDC* pDC = CDC::FromHandle(pNMCD->nmcd.hdc);
    const auto oldFont = pDC->SelectObject(*m_realFont);

    DWORD drawTextStyle = DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;
    if (hdi.fmt & HDF_RIGHT)
        drawTextStyle |= DT_RIGHT;
    else if (hdi.fmt & HDF_CENTER)
        drawTextStyle |= DT_CENTER;
    else
        drawTextStyle |= DT_LEFT;

    if (hdi.fmt & HDF_RTLREADING)
        drawTextStyle |= DT_RTLREADING;

    CRect drawRect(pNMCD->nmcd.rc);
    if (itemState == HIS_PRESSED)
    {
        ++drawRect.left;
        ++drawRect.top;
    }

    const auto centerDrawRectVertically = [&]()
    {
        CRect rcText = drawRect;
        // using DrawThemeTextEx because DrawThemeText not supporting DT_CALCRECT
        DTTOPTS dttopts = { sizeof(DTTOPTS) };
        dttopts.dwFlags = DTT_CALCRECT;
        DrawThemeTextEx(headerTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                        itemState, hdi.pszText, wcslen(hdi.pszText),
                        DT_CALCRECT | drawTextStyle,
                        rcText, &dttopts);

        if (rcText.Height() < drawRect.Height())
            drawRect.OffsetRect(0, (drawRect.Height() - rcText.Height()) / 2);
    };
    centerDrawRectVertically();

    DrawThemeText(headerTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                  itemState, hdi.pszText, wcslen(hdi.pszText),
                  drawTextStyle, 0, drawRect);

    pDC->SelectObject(oldFont);
}

BEGIN_TEMPLATE_MESSAGE_MAP(Customizer, CBaseList, CBaseList)
END_MESSAGE_MAP()

template<typename CBaseList>
inline BOOL Customizer<CBaseList>::OnChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    if (uMsg == WM_NOTIFY)
    {
        auto* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
        if (pNMHDR->code == NM_CUSTOMDRAW && OnNMCustomdraw(pNMHDR, pResult) == TRUE)
            return TRUE;
    }

    return CBaseList::OnChildNotify(uMsg, wParam, lParam, pResult);
}

template<typename CBaseList>
inline void Customizer<CBaseList>::SetMutilineHeader(bool multiline)
{
    if (multiline)
    {
        // to make control bigger - install bigger font
        m_NewHeaderFont.CreatePointFont(190, L"MS Serif");
        m_HeaderCtrl.SetFont(&m_NewHeaderFont);
    }
    else
        m_HeaderCtrl.SetFont(CBaseList::GetFont());
}

template<typename CBaseList>
inline void Customizer<CBaseList>::
SetColor(int item, int subItem, std::optional<COLORREF> backColor, std::optional<COLORREF> textColor)
{
    if (!backColor.has_value() && !textColor.has_value())
    {
        auto* info = m_subItemInfo.Get(item, subItem);
        if (!info || info->tooltip.IsEmpty())
            m_subItemInfo.Remove(item, subItem);
    }

    auto* info = m_subItemInfo.Add(item, subItem);
    info->backColor = backColor;
    info->textColor = textColor;
}

template<typename CBaseList>
inline void Customizer<CBaseList>::
SetTooltip(int item, int subItem, CString text)
{
    if (text.IsEmpty())
    {
        auto* info = m_subItemInfo.Get(item, subItem);
        if (!info || (!info->backColor.has_value() && !info->textColor.has_value()))
        {
            m_subItemInfo.Remove(item, subItem);
            return;
        }
    }

    auto* info = m_subItemInfo.Add(item, subItem);
    info->tooltip = std::move(text);
}

template<typename CBaseList>
inline void Customizer<CBaseList>::
SetTooltipOnColumn(int column, CString text)
{
    m_HeaderCtrl.SetColumnTooltip(column, std::move(text));
}

template <typename CBaseList>
void Customizer<CBaseList>::PreSubclassWindow()
{
    CBaseList::PreSubclassWindow();

    CHeaderCtrl* pHeader = CBaseList::GetHeaderCtrl();
    if (pHeader == NULL)
        return;

    VERIFY(m_HeaderCtrl.SubclassWindow(pHeader->m_hWnd));
    m_HeaderCtrl.SetRealFont(CBaseList::GetFont());

    HDITEM hdItem;
    hdItem.mask = HDI_FORMAT;
    for(int i = m_HeaderCtrl.GetItemCount() - 1; i >= 0; --i)
    {
        m_HeaderCtrl.GetItem(i, &hdItem);
        hdItem.fmt |= HDF_OWNERDRAW;
        m_HeaderCtrl.SetItem(i, &hdItem);
    }

    CBaseList::EnableToolTips();
}

template<typename CBaseList>
inline LRESULT Customizer<CBaseList>::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    m_subItemInfo.ProcessWindowProc(message, wParam, lParam, [&](int i) { return CBaseList::GetItemData(i); });

    switch (message)
    {
    case LVM_INSERTITEM:
        {
            const auto* pItem = reinterpret_cast<const LVITEM*>(lParam);

            const auto res = CBaseList::WindowProc(message, wParam, lParam);
            CBaseList::SetItemData(pItem->iItem, pItem->iItem);
            return res;
        }
    default:
        break;
    }

    return CBaseList::WindowProc(message, wParam, lParam);
}

template<typename CBaseList>
BOOL Customizer<CBaseList>::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

    // Сначало надо определить текущую стадию
    switch (pNMCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        // если рисуется весь элемент целиком - запрашиваем получение сообщений
        // для каждого элемента списка.
        *pResult = CDRF_NOTIFYITEMDRAW;
        return TRUE;

    case CDDS_ITEMPREPAINT:
        // если рисуется весь элемент списка целиком - запрашиваем получение сообщений
        // для каждого подэлемента списка.
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        return TRUE;

    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
        {
            // Стадия, которая наступает перед отрисовкой каждого элемента списка.
            const DWORD iItem = pNMCD->nmcd.dwItemSpec;
            const DWORD iSubItem = pNMCD->iSubItem;

            if (CBaseList::GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED)
                break;

            const auto* itemInfo = m_subItemInfo.Get(CBaseList::GetItemData(iItem), iSubItem);
            if (!itemInfo || (!itemInfo->backColor.has_value() && !itemInfo->textColor.has_value()))
                break;

            if (itemInfo->backColor.has_value())
                pNMCD->clrTextBk = itemInfo->backColor.value();

            if (itemInfo->textColor.has_value())
                pNMCD->clrText = itemInfo->textColor.value();

            //if (GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED)
            //{
            //    // см https://docs.microsoft.com/ru-ru/windows/win32/controls/parts-and-states?redirectedfrom=MSDN
            //    HTHEME hTheme = OpenThemeData(m_hWnd, L"LISTVIEW");

            //    //pNMCD->clrText   = GetThemeSysColor(hTheme, COLOR_HIGHLIGHTTEXT);
            //    pNMCD->clrTextBk = GetThemeSysColor(hTheme, COLOR_HIGHLIGHT);

            //    // прекращаем работу с темой
            //    if (hTheme)
            //        CloseThemeData(hTheme);
            //}

            // Уведомляем систему, чтобы она самостоятельно нарисовала элемент.
            *pResult = CDRF_DODEFAULT | CDRF_NEWFONT;
        }
        return TRUE;

    default:
        // Будем выполнять стандартную обработку для всех сообщений по умолчанию
        break;
    }

    *pResult = CDRF_DODEFAULT;
    return FALSE;
}

template<typename CBaseList>
inline INT_PTR Customizer<CBaseList>::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
    UINT uFlags;
    const auto currentItem = CBaseList::HitTest(point, &uFlags);
    // CMT: If the click has been made on some valid item
    if (-1 != currentItem && uFlags & LVHT_ONITEMLABEL)
    {
        LVHITTESTINFO hti;
        hti.pt = point;
        hti.flags = LVM_SUBITEMHITTEST;
        const_cast<Customizer<CBaseList>&>(*this).SubItemHitTest(&hti);

        const auto getToolTipText = [&]()
        {
            const auto* itemInfo = m_subItemInfo.Get(currentItem, hti.iSubItem);
            if (!!itemInfo && !itemInfo->tooltip.IsEmpty())
                return itemInfo->tooltip;
            return m_HeaderCtrl.GetColumnTooltip(hti.iSubItem);
        };

        const auto tooltip = getToolTipText();
        if (!tooltip.IsEmpty())
        {
            pTI->lpszText = (LPWSTR)malloc((tooltip.GetLength() + 1) * sizeof(wchar_t));
            lstrcpyn(pTI->lpszText, tooltip, tooltip.GetLength() + 1);
            //pTI->lpszText = LPSTR_TEXTCALLBACK;

            pTI->hwnd = CBaseList::m_hWnd;
            pTI->uId = (UINT)((currentItem<<10)+(hti.iSubItem&0x3ff)+1);

            CRect cellRect;
            CBaseList::GetSubItemRect(currentItem, hti.iSubItem, LVIR_BOUNDS, cellRect);
            pTI->rect = cellRect;
            return pTI->uId;
        }
    }

    return CBaseList::OnToolHitTest(point, pTI);
}

} // namespace controls::list::widgets
