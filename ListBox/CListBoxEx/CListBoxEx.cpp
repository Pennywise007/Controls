#include "Controls/ThemeManagement.h"

#include <afxcontrolbarutil.h>
#include <algorithm>
#include <tuple>

#include "CListBoxEx.h"

BEGIN_MESSAGE_MAP(CListBoxEx, CListBox)
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MEASUREITEM_REFLECT()
END_MESSAGE_MAP()

CListBoxEx::CListBoxEx(bool multilineText /*= true*/)
    : m_multilineText(multilineText)
{}

int CListBoxEx::AddItem(const CString& itemText, COLORREF color, int nIndex /*= -1*/)
{
    // индекс куда вставл€ем элемент
    int index;

    if (nIndex == -1)
        index = AddString(itemText);
    else
        index = InsertString(nIndex, itemText);

    m_lineInfo[index]->lineColor = color;

    // иногда при добавлении элементов не возникает перерисовки
    Invalidate();

    return index;
}

int CListBoxEx::AddString(const CString& text)
{
    m_lineInfo.emplace_back(std::make_shared<LineInfo>(text));
    m_internalInsertingText = true;
    const auto result = CListBox::AddString(L"");
    m_internalInsertingText = false;
    Invalidate();
    return result;
}

int CListBoxEx::InsertString(int index, const CString& text)
{
    if (index < 0 || static_cast<size_t>(index) >= m_lineInfo.size())
        return AddString(text);

    const auto insertedIt = m_lineInfo.insert(std::next(m_lineInfo.begin(), index),
                                              std::make_shared<LineInfo>(text));
    m_internalInsertingText = true;
    const auto insertIndex = CListBox::InsertString(index, L"");
    m_internalInsertingText = false;
    Invalidate();
    return insertIndex;
}

void CListBoxEx::GetText(int index, CString& sLabel) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_lineInfo.size())
    {
        ASSERT(false);
        return CListBox::GetText(index, sLabel);
    }

    sLabel = m_lineInfo[index]->text;
}

void CListBoxEx::PreSubclassWindow()
{
    // провер€ем необходимые стили, их невозможно установить через ModifyStyle(не будут работать)
    if (m_multilineText && (!(CListBox::GetStyle() & LBS_HASSTRINGS | LBS_OWNERDRAWVARIABLE)))
        ASSERT(!"” списка должен быть установлен стиль LBS_OWNERDRAWVARIABLE и LBS_HASSTRINGS дл€ корректной работы!");

    EnableWindowTheme(GetSafeHwnd(), L"ListBox", L"Explorer", NULL);

    CListBox::PreSubclassWindow();
}

void CListBoxEx::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    // „тобы вызывалс€ нужен стиль LBS_OWNERDRAWVARIABLE
    const int nItem = lpMeasureItemStruct->itemID;
    CString sLabel;
    CRect rcLabel;

    GetText( nItem, sLabel );
    CListBox::GetItemRect(nItem, rcLabel);

    // рассчитываем высоту элемента
    CPaintDC dc(this);
    dc.SelectObject(CListBox::GetFont());

    lpMeasureItemStruct->itemHeight = dc.DrawText(sLabel, sLabel.GetLength(), rcLabel, DT_WORDBREAK | DT_CALCRECT);
}

void CListBoxEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    // ѕри клике в пустую область может придти DrawItem с невалидным идентификеатором
    if (lpDIS->itemID == -1)
        return;

    const auto getColors = [](HWND hwnd)
    {
        HTHEME hTheme = ::OpenThemeData(hwnd, L"LISTBOX");
        ASSERT(hTheme);

        const COLORREF highLightColor = ::GetThemeSysColor(hTheme, COLOR_HIGHLIGHT);
        const COLORREF highLightTextColor = ::GetThemeSysColor(hTheme, COLOR_HIGHLIGHTTEXT);
        ::CloseThemeData(hTheme);

        return std::make_tuple(highLightColor, highLightTextColor);
    };
    const static auto colors = getColors(m_hWnd);
    const COLORREF highLightColor = std::get<0>(colors);
    const COLORREF highLightTextColor = std::get<1>(colors);

    CDC& dc = *CDC::FromHandle(lpDIS->hDC);

    const COLORREF backColor = [&]()
    {
        try
        {
            return m_lineInfo.at(lpDIS->itemID)->lineColor.value_or(dc.GetBkColor());
        }
        catch (...)
        {
            ASSERT(false);
        }
        return dc.GetBkColor();
    }();

    CString sLabel;
    GetText(lpDIS->itemID, sLabel);
    sLabel.Remove(L'\0');

    // item selected
    if ((lpDIS->itemState & ODS_SELECTED) &&
        (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
    {
        // draw label background
        CBrush labelBrush(highLightColor);
        CRect labelRect = lpDIS->rcItem;
        dc.FillRect(&labelRect,&labelBrush);

        // draw label text
        COLORREF colorTextSave = dc.SetTextColor(highLightTextColor);
        COLORREF colorBkSave = dc.SetBkColor(highLightColor);
        dc.DrawText( sLabel, sLabel.GetLength(), &lpDIS->rcItem, m_multilineText ? DT_WORDBREAK : 0);

        dc.SetTextColor(colorTextSave);
        dc.SetBkColor(colorBkSave);
    }
    // item brought into box
    else if (lpDIS->itemAction & ODA_DRAWENTIRE)
    {
        CBrush brush(backColor);
        CRect rect = lpDIS->rcItem;
        dc.SetBkColor(backColor);
        dc.FillRect(&rect,&brush);
        dc.DrawText( sLabel, sLabel.GetLength(), &lpDIS->rcItem, m_multilineText ? DT_WORDBREAK : 0);
    }
    // item deselected
    else if (!(lpDIS->itemState & ODS_SELECTED) && (lpDIS->itemAction & ODA_SELECT))
    {
        CRect rect = lpDIS->rcItem;
        CBrush brush(backColor);
        dc.SetBkColor(backColor);
        dc.FillRect(&rect,&brush);
        dc.DrawText( sLabel, sLabel.GetLength(), &lpDIS->rcItem, m_multilineText ? DT_WORDBREAK : 0);
    }
}

void CListBoxEx::OnSize(UINT nType, int cx, int cy)
{
    if (m_multilineText)
    {
        MEASUREITEMSTRUCT measureStruct;

        // дл€ каждого элемента пересчитываем размер
        for (int item = 0, count = CListBox::GetCount();
             item < count; ++item)
        {
            // смотрим сколько он занимает по высоте
            measureStruct.itemID = item;
            MeasureItem(&measureStruct);

            // если высота изменилась надо еЄ переустановить
            if (GetItemHeight(item) != measureStruct.itemHeight)
                SetItemHeight(item, measureStruct.itemHeight);
        }

        // иногда возникают случаи(при показе скроллера) когда не происходит перерисовка
        Invalidate();
    }

    CListBox::OnSize(nType, cx, cy);
}

BOOL CListBoxEx::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

LRESULT CListBoxEx::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == LB_RESETCONTENT)
        m_lineInfo.clear();
    else if (message == LB_DELETESTRING)
        m_lineInfo.erase(std::next(m_lineInfo.begin(), static_cast<size_t>(wParam)));
    else if (message == LB_ADDSTRING && !m_internalInsertingText)
        return AddString(LPCTSTR(lParam));
    else if (message == LB_INSERTSTRING && !m_internalInsertingText)
        return InsertString(int(wParam), LPCTSTR(lParam));

    return CListBox::WindowProc(message, wParam, lParam);
}

void CListBoxEx::OnPaint()
{
    CRect drawRect;
    CListBox::GetClientRect(drawRect);

    if (drawRect.IsRectEmpty())
        return;

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, drawRect);
    CDC& dc = memDC.GetDC();

    dc.SaveDC();
    dc.SetWindowOrg(GetScrollPos(SB_HORZ), 0);

    dc.SelectObject(CListBox::GetFont());

    {
        int minPos = 0, maxPos = 0;
        GetScrollRange(SB_HORZ, &minPos, &maxPos);

        CRect visibleRect = drawRect;
        visibleRect.right = std::max<LONG>(visibleRect.right, maxPos);
        dc.FillSolidRect(&visibleRect, GetBkColor(dc.m_hDC));
    }

    const auto getItemUnderCursor = [&]()
    {
        CPoint cursor;
        ::GetCursorPos(&cursor);
        CListBox::ScreenToClient(&cursor);
        BOOL itemUnderCursor;
        int cursorItem = ItemFromPoint(cursor, itemUnderCursor);
        if (itemUnderCursor == FALSE)
            cursorItem = -1;
        return cursorItem;
    };
    const int itemUnderCursor = getItemUnderCursor();
    const bool focused = CListBox::GetFocus() == this;
    const int countItems = CListBox::GetCount();
    if (countItems == 0)
    {
        dc.RestoreDC(-1);
        return;
    }

    DRAWITEMSTRUCT drawStruct;
    drawStruct.CtlType = ODT_LISTBOX;
    drawStruct.CtlID = CListBox::GetDlgCtrlID();
    drawStruct.hwndItem = m_hWnd;
    drawStruct.hDC = dc.m_hDC;
    drawStruct.itemAction = ODA_DRAWENTIRE;
    drawStruct.itemState = CListBox::IsWindowEnabled() == FALSE ? ODS_DISABLED : 0;
    drawStruct.itemID = CListBox::GetTopIndex();

    do
    {
        drawStruct.itemData = CListBox::GetItemData(drawStruct.itemID);
        CListBox::GetItemRect(drawStruct.itemID, &drawStruct.rcItem);

        const auto prevAction = drawStruct.itemAction;
        const auto prevState = drawStruct.itemState;

        const bool selected = CListBox::GetSel(drawStruct.itemID) > 0;
        if (selected)
        {
            drawStruct.itemAction |= ODA_SELECT | (focused ? ODS_FOCUS : ODS_NOFOCUSRECT);
            drawStruct.itemState |= ODS_SELECTED;
        }
        else
            drawStruct.itemAction |= ODS_DEFAULT;

        if (itemUnderCursor)
            drawStruct.itemState |= ODS_HOTLIGHT;

        DrawItem(&drawStruct);

        drawStruct.itemAction = prevAction;
        drawStruct.itemState = prevState;
        if (++drawStruct.itemID == countItems)
            break;
    } while (drawStruct.rcItem.bottom <= drawRect.bottom);

    dc.RestoreDC(-1);
}
