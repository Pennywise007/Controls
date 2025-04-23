﻿#include "ListGroupCtrl.h"

#include <VersionHelpers.h>
#include <vector>
#include <shlwapi.h>
#include "Resource.h"
#include <algorithm>
#include <memory>

#include "Controls/ThemeManagement.h"

namespace {

struct ListItemData
{
    void* userDataPtr = nullptr;
    int defaultItemIndex = 0;
};

bool is_key_pressed(UINT keyCode)
{
    return GetKeyState(keyCode) & 0x8000;
}

} // namespace

static CString sExpandAllGroups;
static CString sCollapseAllGroups;

static CString sGroupBy;
static CString sDisableGrouping;

static CString sCheckAll;
static CString sUnCheckALL;

static CString sCheckGroup;
static CString sUnCheckGroup;
static CString sDeleteGroup;

static CString sExpandGroup;
static CString sCollapseGroup;

BEGIN_MESSAGE_MAP(CListGroupCtrl, CListCtrl)
    ON_WM_CONTEXTMENU()	// OnContextMenu
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT_EX(LVN_COLUMNCLICK, OnHeaderClick)	// Column Click
#if _WIN32_WINNT >= 0x0600
    ON_NOTIFY_REFLECT_EX(LVN_LINKCLICK, OnGroupTaskClick)	// Column Click
#endif
    ON_NOTIFY_EX(HDN_ITEMSTATEICONCLICK, 0, OnHdnItemStateIconClick)
    ON_NOTIFY_EX(HDN_BEGINTRACK, 0, OnHeaderBeginDrag)
    ON_NOTIFY_EX(HDN_ENDTRACK, 0, OnHeaderEndDrag)
    ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, OnListItemChanged)    // изменение в листе
    ON_WM_KEYUP()
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CListGroupCtrl::CListGroupCtrl()
{
#ifdef _TRANSLATE
    sExpandAllGroups	= TranslateString(L"Развернуть все группы");
    sCollapseAllGroups	= TranslateString(L"Свернуть все группы");
    sGroupBy			= TranslateString(L"Группировать по");
    sDisableGrouping	= TranslateString(L"Отменить группировку");
    sCheckGroup			= TranslateString(L"Выбрать все элементы группы");
    sCheckAll			= TranslateString(L"Выбрать все элементы");
    sUnCheckALL			= TranslateString(L"Отменить выбор всех элементов");
    sUnCheckGroup		= TranslateString(L"Отменить выбор всех элементов группы");
    sDeleteGroup		= TranslateString(L"Удалить группу");
    sExpandGroup		= TranslateString(L"Развернуть группу");
    sCollapseGroup		= TranslateString(L"Свернуть группу");
#else
    sExpandAllGroups	= L"Развернуть все группы";
    sCollapseAllGroups	= L"Свернуть все группы";
    sGroupBy			= L"Группировать по";
    sDisableGrouping	= L"Отменить группировку";
    sCheckGroup			= L"Выбрать все элементы группы";
    sUnCheckGroup		= L"Отменить выбор всех элементов группы";
    sDeleteGroup		= L"Удалить группу";
    sExpandGroup		= L"Развернуть группу";
    sCollapseGroup		= L"Свернуть группу";
    sCheckAll			= L"Выбрать все элементы";
    sUnCheckALL			= L"Отменить выбор всех элементов";
#endif
}

bool CListGroupCtrl::CheckInsertedElement(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem)
{
    bool bRes(true);
    CString NewText(lpszItem);
    // проверяем что элемент удовлетворяет условиям поиска
    bool bFind(m_bMatchAll || m_sFindStrings.empty());
    for (auto& String : m_sFindStrings)
    {
        bool bFindInColumn = NewText.MakeLower().Find(String.MakeLower()) != -1;

        if (m_bMatchAll)	// првоеряем что строчка таблицы должна совпадать со всеми элементами вектора
            bFind &= bFindInColumn;
        else            // проверяем что строчка таблицы должна совпадать хотябы с одним элементом вектора
            bFind |= bFindInColumn;
    }

    // если строчка не совпала с теми что мы ищем то удаляем ее
    if (!bFind)
    {
        // для проверки была ли создана группа вообще
        LVGROUP Str = { 0 };
        Str.cbSize = sizeof(LVGROUP);
        Str.stateMask = LVM_GETGROUPINFO;
        Str.mask = LVGF_GROUPID;

        LVITEM lvi = { 0 };
        lvi.mask = LVIF_GROUPID | TVIF_PARAM | LVIF_STATE | LVIF_TEXT;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        lvi.iItem = nItem;
        lvi.iSubItem = 0;

        auto itemData = std::make_unique<ListItemData>();
        itemData->defaultItemIndex = nItem;
        lvi.lParam = LPARAM(itemData.release());

        lvi.iGroupId = CListCtrl::GetGroupInfo(nGroupID, &Str);
        lvi.pszText = NewText.GetBuffer();
        lvi.cchTextMax = NewText.GetLength();

        // всем элементам стоящим после добавляемого элемента необходимо сдвинуть номер
        for (auto& DeletedItem : m_DeletedItems)
        {
            if (DeletedItem.ItemData.iItem >= nItem)
                DeletedItem.ItemData.iItem++;
        }
        // сохраняем новый элемент
        m_DeletedItems.emplace_back(DeletedItemsInfo{ FALSE, lvi });
        m_DeletedItems.back().ColumnsText.emplace_back(std::move(NewText));

        bRes = false;
    }
    else
    {
        // всем элементам стоящим после добавляемого элемента необходимо сдвинуть номер// сдвигаем
        for (auto& DeletedItem : m_DeletedItems)
        {
            if (DeletedItem.ItemData.iItem >= nItem)
                DeletedItem.ItemData.iItem++;
        }
    }

    return bRes;
}

int CListGroupCtrl::InsertItem(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem)
{
    int Res(nItem);
    if (CheckInsertedElement(nItem, nGroupID, lpszItem))
    {
        Res = CListCtrl::InsertItem(nItem, lpszItem);
        SetDefaultItemIndex(Res, nItem);
        SetRowGroupId(Res, nGroupID);

        //Resort();
    }

    return Res;
}

int CListGroupCtrl::InsertItem(_In_ int nItem, _In_z_ LPCTSTR lpszItem)
{
    int Res(nItem);
    if (CheckInsertedElement(nItem, -1, lpszItem))
    {
        Res = CListCtrl::InsertItem(nItem, lpszItem);
        SetDefaultItemIndex(Res, nItem);

        //Resort();
    }

    return Res;
}

int CListGroupCtrl::InsertItem(_In_ const LVITEM* pItem)
{
    int Res(pItem->iItem);
    if (CheckInsertedElement(pItem->iItem, -1, pItem->pszText))
    {
        Res = CListCtrl::InsertItem(pItem);
        SetDefaultItemIndex(Res, pItem->iItem);

        //Resort();
    }

    return Res;
}

BOOL CListGroupCtrl::SetItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText)
{
    // ещем среди удаленных элементов если ли там элемент который мы хотим изменить
    auto it = std::find_if(m_DeletedItems.begin(), m_DeletedItems.end(), [&](const DeletedItemsInfo& Item)
    {
        return Item.ItemData.iItem == nItem;
    });

    if (it == m_DeletedItems.end())
    {
        // если не нашли то устанавливаем текст
        return CListCtrl::SetItemText(nItem, nSubItem, lpszText);
    }
    else
    {
        // заносим новые данные в необходимую колонку удаленного элемента
        if ((int)it->ColumnsText.size() <= nSubItem)
            it->ColumnsText.resize(nSubItem + 1);
        it->ColumnsText[nSubItem] = lpszText;

        if (bIsNeedToRestoreDeletedItem(it))
            m_DeletedItems.erase(it);
    }

    return TRUE;
}

void CListGroupCtrl::SetItemDataPtr(int nIndex, void* pData)
{
    ASSERT(nIndex < GetItemCount());

    auto data = CListCtrl::GetItemData(nIndex);
    ASSERT(data != 0);
    ListItemData* itemData = (ListItemData*)data;
    itemData->userDataPtr = pData;
}

void* CListGroupCtrl::GetItemDataPtr(int nIndex) const
{
    ASSERT(nIndex < GetItemCount());
    ListItemData* itemData = (ListItemData*)CListCtrl::GetItemData(nIndex);
    if (!itemData)
        return nullptr;
    return itemData->userDataPtr;
}

int CListGroupCtrl::GetDefaultItemIndex(int nCurrentItem) const
{
    auto itemData = CListCtrl::GetItemData(nCurrentItem);
    ASSERT(itemData != 0);
    return ((ListItemData*)itemData)->defaultItemIndex;
}

void CListGroupCtrl::SetDefaultItemIndex(int nCurrentItem, int nRealItem)
{
    ASSERT(CListCtrl::GetItemData(nCurrentItem) == 0);
    auto itemData = std::make_unique<ListItemData>();
    itemData->defaultItemIndex = nRealItem;
    CListCtrl::SetItemData(nCurrentItem, DWORD_PTR(itemData.release()));
}

int CListGroupCtrl::GetCurrentIndexFromDefaultItem(_In_ int nDefaultItem) const
{
    // ищем по отображаемым данным, есть ли там запрашиваемый жлемент
    for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
    {
        if (GetDefaultItemIndex(Row) == nDefaultItem)
            return Row;
    }

    return -1;
}

int CListGroupCtrl::MoveItem(int itemIndex, bool moveUp)
{
    const int itemsCount = GetItemCount();

    if (itemsCount <= 1 || itemIndex == -1)
        return itemIndex;

    const auto swapItemsData = [list = this](int itemFirst, int itemSecond)
    {
        CString strFirst, strSecond;
        for (int i = 0, countColumns = list->GetHeaderCtrl()->GetItemCount(); i < countColumns; ++i)
        {
            strFirst = std::move(list->GetItemText(itemFirst, i));
            strSecond = std::move(list->GetItemText(itemSecond, i));
            list->SetItemText(itemFirst, i, strSecond);
            list->SetItemText(itemSecond, i, strFirst);
        }

        ListItemData* dataFirst = (ListItemData*)list->GetItemData(itemFirst);
        ListItemData* dataSecond = (ListItemData*)list->GetItemData(itemSecond);
        list->SetItemData(itemFirst, (DWORD_PTR)dataSecond);
        list->SetItemData(itemSecond, (DWORD_PTR)dataFirst);

        // Swap selection
        const bool bItemFirstSelected = list->GetItemState(itemFirst, LVIS_SELECTED) & LVNI_SELECTED;
        const bool bItemSecondSelected = list->GetItemState(itemSecond, LVIS_SELECTED) & LVNI_SELECTED;

        list->SetItemState(itemFirst, bItemSecondSelected ? LVNI_SELECTED : ~LVNI_SELECTED, LVIS_SELECTED);
        list->SetItemState(itemSecond, bItemFirstSelected ? LVNI_SELECTED : ~LVNI_SELECTED, LVIS_SELECTED);
    };

    // swap items index
    int swapItem = moveUp ? itemIndex - 1 : itemIndex + 1;

    if (swapItem < 0)
    {
        swapItem = InsertItem(GetItemCount(), L"");
        swapItemsData(itemIndex, swapItem--);
        CListCtrl::DeleteItem(itemIndex);
    }
    else if (swapItem >= GetItemCount())
    {
        swapItem = InsertItem(0, L"");
        swapItemsData(++itemIndex, swapItem);
        CListCtrl::DeleteItem(itemIndex);
    }
    else
    {
        swapItemsData(itemIndex, swapItem);
    }

    EnsureVisible(swapItem, FALSE);
    return swapItem;
}

void CListGroupCtrl::MoveSelectedItems(bool moveUp)
{
    std::vector<int> selectedItems = GetSelectedItems();
    ASSERT(!selectedItems.empty());

    if (moveUp)
    {
        if (selectedItems.front() == 0)
        {
            MoveItem(selectedItems.front(), true);
        }
        else
        {
            for (auto item : selectedItems)
            {
                MoveItem(item, true);
            }
        }
    }
    else
    {
        if (selectedItems.back() == GetItemCount() - 1)
        {
            MoveItem(selectedItems.back(), false);
        }
        else
        {
            for (auto it = selectedItems.rbegin(), end = selectedItems.rend(); it != end; ++it)
            {
                MoveItem(*it, false);
            }
        }
    }
}

BOOL CListGroupCtrl::DeleteAllItems()
{
    for (int item = 0, size = GetItemCount(); item < size; ++item)
    {
        std::unique_ptr<ListItemData> itemData((ListItemData*)CListCtrl::GetItemData(item));
    }
    for (auto& deletedItem : m_DeletedItems)
    {
        std::unique_ptr<ListItemData> itemData((ListItemData*)deletedItem.ItemData.lParam);
    }
    m_DeletedItems.clear();
    return CListCtrl::DeleteAllItems();
}

BOOL CListGroupCtrl::DeleteItem(_In_ int nItem)
{
    std::unique_ptr<ListItemData> itemData((ListItemData*)CListCtrl::GetItemData(nItem));
    if (itemData != nullptr)
    {
        // When Vert scroll is visible and scrolled to the bottom, we might want to remove any item which will force Vert scroll to disappear
        // in this case first item might `lose` its vert position and appear in the middle of control, to avoid it
        // - check if vert scroll is still needed and if not - force EnsureVisible to force VScroll to be in Pos 0
        // P.S. it happens only when we execute SetColumnWidth in the OnSize

        int totalItems = GetItemCount();
        int maxVisibleItems = GetCountPerPage();

        bool scrollVisibleNow = totalItems > maxVisibleItems;
        bool scrollWillBeVisibleAfterItemDeletion = (totalItems - 1) > maxVisibleItems;
        if (scrollVisibleNow && !scrollWillBeVisibleAfterItemDeletion)
            EnsureVisible(0, TRUE);
    }

    return CListCtrl::DeleteItem(nItem);
}

void CListGroupCtrl::FindItems(_In_ CString sFindString)
{
    FindItems(std::list<CString>{ std::move(sFindString) });
}

void CListGroupCtrl::FindItems(_In_ std::list<CString> sFindStrings, _In_opt_ bool bMatchAll /*= false*/)
{
    m_sFindStrings = sFindStrings;
    m_bMatchAll = bMatchAll;

    // сортируем данные в массиве чтобы последовательно их загрузить в контрол
    m_DeletedItems.sort([](const DeletedItemsInfo& Item1, const DeletedItemsInfo& Item2) -> bool
    {
        return Item1.ItemData.iItem < Item2.ItemData.iItem;
    });

    // восстанавливаем список удаленных элементов m_DeletedItems
    for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
    {
        if (bIsNeedToRestoreDeletedItem(it))
            it = m_DeletedItems.erase(it);
        else
            ++it;
    }

    int nTotalColumns = GetHeaderCtrl()->GetItemCount();
    // проходим по всем элементам и удаляем те, которые не попали под критерии поиска
    for (int Row = CListCtrl::GetItemCount() - 1; Row >= 0; Row--)
    {
        // флаг того, что в строчке есть элементы удовлетворяющие поиску
        bool bFind = bMatchAll;
        for (auto&& it : sFindStrings)
        {
            if (bMatchAll)// првоеряем что строчка таблицы должна совпадать со всеми элементами вектора
                bFind &= FindItemInTable(std::move(it), Row);
            else // првоеряем что строчка таблицы должна совпадать хотябы с одним элементом вектора
                bFind |= FindItemInTable(std::move(it), Row);
        }

        // если строчка не совпала с существующими, то удаляем ее
        if (!bFind)
        {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_GROUPID | TVIF_PARAM | LVIF_STATE | LVIF_TEXT;
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.iItem = Row;
            CListCtrl::GetItem(&lvi);

            m_DeletedItems.push_back({ CListCtrl::GetCheck(Row), lvi });
            m_DeletedItems.back().ColumnsText.resize(nTotalColumns);
            for (auto Column = 0; Column < nTotalColumns; ++Column)
            {
                m_DeletedItems.back().ColumnsText[Column] = CListCtrl::GetItemText(Row, Column);
            }

            CListCtrl::DeleteItem(Row);
        }
    }

    Resort();
}

void CListGroupCtrl::ResetSearch()
{
    FindItems(L"");
}

void CListGroupCtrl::SelectItem(int nItem, bool ensureVisible /*= true*/, bool select /*= true*/)
{
    SetItemState(nItem, select ? LVIS_SELECTED : 0, LVIS_SELECTED);
    if (ensureVisible)
        EnsureVisible(nItem, TRUE);
}

int CListGroupCtrl::GetLastSelectedItem() const
{
    POSITION pos = GetFirstSelectedItemPosition();
    int nItem = -1;
    while (pos)
    {
        nItem = GetNextSelectedItem(pos);
    }
    return nItem;
}

std::vector<int> CListGroupCtrl::GetSelectedItems() const
{
    std::vector<int> selectedItems;
    if (const UINT selCount = GetSelectedCount(); selCount != 0)
    {
        selectedItems.resize(selCount);

        POSITION pos = GetFirstSelectedItemPosition();
        for (UINT index = 0; index < selCount; ++index)
        {
            selectedItems[index] = GetNextSelectedItem(pos);
        }
    }
    return selectedItems;
}

void CListGroupCtrl::ClearSelection()
{
    if (const UINT selCount = GetSelectedCount(); selCount != 0)
    {
        POSITION pos = GetFirstSelectedItemPosition();
        for (UINT index = 0; index < selCount; ++index)
        {
            SetItemState(GetNextSelectedItem(pos), 0, LVIS_SELECTED);
        }
    }
}

int CListGroupCtrl::InsertColumn(_In_ int nCol, _In_ LPCTSTR lpszColumnHeading, _In_opt_ int nFormat /*= LVCFMT_LEFT*/, _In_opt_ int nWidth /*= -1*/,
                                 _In_opt_ int nSubItem /*= -1*/, _In_opt_ std::function<int(const CString&, const CString&)> SortFunct /*= nullptr*/)
{
    if (SortFunct != nullptr)
        m_SortFunct.emplace_back(std::make_pair(nCol, SortFunct));

    const int Res = CListCtrl::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);
    if (Res == 0 && nFormat != LVCFMT_LEFT)
    {
        // Sometimes control doesn't want to set special formats to first column
        LVCOLUMN colInfo;
        colInfo.mask = LVCF_FMT;
        GetColumn(0, &colInfo);
        colInfo.fmt |= LVCFMT_CENTER;
        SetColumn(0, &colInfo);
    }

    // добавляем первой колонке общий чекбокс
    if (GetExtendedStyle() & LVS_EX_CHECKBOXES && Res == 0)
    {
        CHeaderCtrl* header = GetHeaderCtrl();
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        Header_GetItem(*header, 0, &hdi);
        hdi.fmt |= HDF_CHECKBOX;
        Header_SetItem(*header, 0, &hdi);
    }

    return Res;
}

int CListGroupCtrl::InsertColumn(_In_ int nCol, _In_ const LVCOLUMN* pColumn)
{
    return CListCtrl::InsertColumn(nCol, pColumn);
}

void CListGroupCtrl::AutoScaleColumns()
{
    const int CountColumns = GetHeaderCtrl()->GetItemCount();
    if (CountColumns != 0)
    {
        CRect Rect;
        GetClientRect(Rect);

        const int Width = Rect.Width() / CountColumns;
        for (int Column = 0; Column < CountColumns; Column++)
            CListCtrl::SetColumnWidth(Column, Width);
    }
}

void CListGroupCtrl::FitToContentColumn(int nCol, bool bIncludeHeaderContent)
{
    const int flag = bIncludeHeaderContent ? LVSCW_AUTOSIZE_USEHEADER : LVSCW_AUTOSIZE;
    ListView_SetColumnWidth(m_hWnd, nCol, flag);
}

void CListGroupCtrl::SetProportionalResizingColumns(const std::unordered_set<int>& columns)
{
    m_columnsProportions.clear();
    if (columns.empty())
        return;

    CRect clientRect;
    GetClientRect(clientRect);
    int availableWidth = clientRect.Width();

    const auto columnsCount = GetHeaderCtrl()->GetItemCount();
    for (int column = 0; column < columnsCount; ++column)
    {
        if (columns.find(column) == columns.end())
            availableWidth -= GetColumnWidth(column);
    }

    for (auto& column : columns)
    {
        VERIFY(column < columnsCount);
        m_columnsProportions[column] = (double)availableWidth / double(GetColumnWidth(column));
    }
}

BOOL CListGroupCtrl::ModifyStyle(_In_ DWORD dwRemove, _In_ DWORD dwAdd, _In_opt_ UINT nFlags /*= 0*/)
{
    ASSERT(!(dwAdd & LVS_EX_CHECKBOXES)); // Deprecated, use ModifyExtendedStyle

    return CListCtrl::ModifyStyle(dwRemove, dwAdd, nFlags);
}

void CListGroupCtrl::ModifyExtendedStyle(_In_ DWORD dwRemove, _In_ DWORD dwAdd)
{
    auto style = (GetExtendedStyle() & ~dwRemove) | dwAdd;
    SetExtendedStyle(style);

    if (dwAdd & LVS_EX_CHECKBOXES)
    {
        const HWND header = *GetHeaderCtrl();
        DWORD dwHeaderStyle = ::GetWindowLong(header, GWL_STYLE);
        // add HDS_BUTTONS because in LVS_NOSORTHEADER mode we don't receive clicks on main checkbox
        dwHeaderStyle |= HDS_CHECKBOXES | HDS_BUTTONS;
        ::SetWindowLong(header, GWL_STYLE, dwHeaderStyle);
    }
    else if (dwRemove & LVS_EX_CHECKBOXES)
    {
        const HWND header = *GetHeaderCtrl();
        DWORD dwHeaderStyle = ::GetWindowLong(header, GWL_STYLE);
        dwHeaderStyle &= ~HDS_CHECKBOXES;
        // restore default state in LVS_NOSORTHEADER mode
        if (GetStyle() & LVS_NOSORTHEADER)
            dwHeaderStyle &= ~HDS_BUTTONS;
        ::SetWindowLong(header, GWL_STYLE, dwHeaderStyle);
    }
}

void CListGroupCtrl::CheckAllElements(bool bChecked)
{
    for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end(); ++it)
    {
        it->bChecked = bChecked;
    }

    CheckElements(bChecked);
}

void CListGroupCtrl::CheckEntireGroup(int nGroupId, bool bChecked)
{
    if (!(GetExtendedStyle() & LVS_EX_CHECKBOXES))
        return;

    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        if (GetRowGroupId(nRow) == nGroupId)
        {
            SetCheck(nRow, bChecked ? TRUE : FALSE);
        }
    }
}

std::vector<int> CListGroupCtrl::GetCheckedList(_In_opt_ bool currentIndexes /*= true*/) const
{
    std::vector<int> checkedList;
    const int nItemsCount = CListCtrl::GetItemCount();

    if (currentIndexes)
    {
        checkedList.reserve(nItemsCount);

        for (int Row = 0; Row < nItemsCount; ++Row)
        {
            if (GetCheck(Row) != BST_UNCHECKED)
                checkedList.push_back(Row);
        }
    }
    else
    {
        checkedList.reserve(nItemsCount + m_DeletedItems.size());

        for (int Row = 0; Row < nItemsCount; ++Row)
        {
            if (GetCheck(Row) != BST_UNCHECKED)
                checkedList.push_back(GetDefaultItemIndex(Row));
        }

        for (const auto& it : m_DeletedItems)
        {
            if (it.bChecked)
                checkedList.push_back(((ListItemData*)it.ItemData.lParam)->defaultItemIndex);
        }
    }
    return checkedList;
}

LRESULT CListGroupCtrl::InsertGroupHeader(_In_ int nIndex, _In_ int nGroupID, _In_ const CString& strHeader,
                                          _In_opt_ DWORD dwState /*= LVGS_NORMAL*/, _In_opt_ DWORD dwAlign /*= LVGA_HEADER_LEFT*/)
{
    EnableGroupView(TRUE);

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.iGroupId = nGroupID;
    lg.state = dwState;
    lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN | LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
    lg.uAlign = dwAlign;

    // Header-title must be unicode (Convert if necessary)
#ifdef UNICODE
    lg.pszHeader = (LPWSTR)(LPCTSTR)strHeader;
    lg.cchHeader = strHeader.GetLength();
#else
    CComBSTR header = strHeader;
    lg.pszHeader = header;
    lg.cchHeader = header.Length();
#endif

    int Res = InsertGroup(nIndex, (PLVGROUP)&lg);

    if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
    {
        SetGroupTask(nGroupID, sCheckGroup);	// Check Group
    }

    return Res;
}

CString CListGroupCtrl::GetGroupHeader(int nGroupId)
{
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.iGroupId = nGroupId;
    lg.mask = LVGF_HEADER | LVGF_GROUPID;
    VERIFY(GetGroupInfo(nGroupId, (PLVGROUP)&lg) != -1);

#ifdef UNICODE
    return lg.pszHeader;
#else
    CComBSTR header(lg.pszHeader);
    return (LPCTSTR)COLE2T(header);
#endif
}

int CListGroupCtrl::GetRowGroupId(int nRow)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_GROUPID;
    lvi.iItem = nRow;
    VERIFY(GetItem(&lvi));
    return lvi.iGroupId;
}

BOOL CListGroupCtrl::SetRowGroupId(int nRow, int nGroupId)
{
    //OBS! Rows not assigned to a group will not show in group-view
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_GROUPID;
    lvItem.iItem = nRow;
    lvItem.iSubItem = 0;
    lvItem.iGroupId = nGroupId;
    return SetItem(&lvItem);
}

int CListGroupCtrl::GroupHitTest(const CPoint& point)
{
    if (!IsGroupViewEnabled())
        return -1;

    if (HitTest(point) != -1)
        return -1;

    if (IsGroupStateEnabled())
    {
        // Running on Vista or newer, but compiled without _WIN32_WINNT >= 0x0600
#ifndef LVM_GETGROUPINFOBYINDEX
#define LVM_GETGROUPINFOBYINDEX   (LVM_FIRST + 153)
#endif
#ifndef LVM_GETGROUPCOUNT
#define LVM_GETGROUPCOUNT         (LVM_FIRST + 152)
#endif
#ifndef LVM_GETGROUPRECT
#define LVM_GETGROUPRECT          (LVM_FIRST + 98)
#endif
#ifndef LVGGR_HEADER
#define LVGGR_HEADER		      (1)
#endif

        LRESULT groupCount = SNDMSG((m_hWnd), LVM_GETGROUPCOUNT, (WPARAM)0, (LPARAM)0);
        if (groupCount <= 0)
            return -1;
        for (int i = 0; i < groupCount; ++i)
        {
            LVGROUP lg = { 0 };
            lg.cbSize = sizeof(lg);
            lg.mask = LVGF_GROUPID;

            VERIFY(SNDMSG((m_hWnd), LVM_GETGROUPINFOBYINDEX, (WPARAM)(i), (LPARAM)(&lg)));

            CRect rect(0, LVGGR_HEADER, 0, 0);
            VERIFY(SNDMSG((m_hWnd), LVM_GETGROUPRECT, (WPARAM)(lg.iGroupId), (LPARAM)(RECT*)(&rect)));

            if (rect.PtInRect(point))
                return lg.iGroupId;
        }
        // Don't try other ways to find the group
        return -1;
    }

    // We require that each group contains atleast one item
    if (GetItemCount() == 0)
        return -1;

    // This logic doesn't support collapsible groups
    int nFirstRow = -1;
    CRect gridRect;
    GetWindowRect(&gridRect);
    for (CPoint pt = point; pt.y < gridRect.bottom; pt.y += 2)
    {
        nFirstRow = HitTest(pt);
        if (nFirstRow != -1)
            break;
    }

    if (nFirstRow == -1)
        return -1;

    const int nGroupId = GetRowGroupId(nFirstRow);

    // Extra validation that the above row belongs to a different group
    const int nAboveRow = GetNextItem(nFirstRow, LVNI_ABOVE);
    if (nAboveRow != -1 && nGroupId == GetRowGroupId(nAboveRow))
        return -1;

    return nGroupId;
}

BOOL CListGroupCtrl::GroupByColumn(int nCol)
{
    if (!IsCommonControlsEnabled())
        return FALSE;

    CWaitCursor waitCursor;

    SetSortArrow(-1, SortType::eNone);

    SetRedraw(FALSE);

    RemoveAllGroups();

    EnableGroupView(GetItemCount() > 0);

    if (IsGroupViewEnabled())
    {
        CSimpleMap<CString, CSimpleArray<int> > groups;

        // Loop through all rows and find possible groups
        for (int nRow = 0; nRow < GetItemCount(); ++nRow)
        {
            CString cellText = GetItemText(nRow, nCol);

            int nGroupId = groups.FindKey(cellText);
            if (nGroupId == -1)
            {
                CSimpleArray<int> rows;
                groups.Add(cellText, rows);
                nGroupId = groups.FindKey(cellText);
            }
            groups.GetValueAt(nGroupId).Add(nRow);
        }

        // Look through all groups and assign rows to group
        for (int nGroupId = 0; nGroupId < groups.GetSize(); ++nGroupId)
        {
            const CSimpleArray<int>& groupRows = groups.GetValueAt(nGroupId);
            DWORD dwState = LVGS_NORMAL;

#ifdef LVGS_COLLAPSIBLE
            if (IsGroupStateEnabled())
                dwState = LVGS_COLLAPSIBLE;
#endif

            VERIFY(InsertGroupHeader(nGroupId, nGroupId, groups.GetKeyAt(nGroupId), dwState) != -1);
            SetGroupTask(nGroupId, _T("Task: ") + sCheckGroup);	// Task: Check Group
            CString subtitle;
            subtitle.Format(_T("Subtitle: %i rows"), groupRows.GetSize());
            SetGroupSubtitle(nGroupId, subtitle);
            SetGroupFooter(nGroupId, _T("Group Footer"));

            for (int groupRow = 0; groupRow < groupRows.GetSize(); ++groupRow)
            {
                VERIFY(SetRowGroupId(groupRows[groupRow], nGroupId));
            }
        }
        SetRedraw(TRUE);
        Invalidate(FALSE);
        return TRUE;
    }

    SetRedraw(TRUE);
    Invalidate(FALSE);
    return FALSE;
}

void CListGroupCtrl::DeleteEntireGroup(int nGroupId)
{
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        if (GetRowGroupId(nRow) == nGroupId)
        {
            DeleteItem(nRow);
            nRow--;
        }
    }
    RemoveGroup(nGroupId);
}

// Vista SDK - ListView_GetGroupState / LVM_GETGROUPSTATE
BOOL CListGroupCtrl::HasGroupState(int nGroupId, DWORD dwState)
{
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_STATE;
    lg.stateMask = dwState;
    if (GetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
        return FALSE;

    return lg.state == dwState;
}

// Vista SDK - ListView_SetGroupState / LVM_SETGROUPINFO
BOOL CListGroupCtrl::SetGroupState(int nGroupId, DWORD dwState)
{
    if (!IsGroupStateEnabled())
        return FALSE;

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_STATE;
    lg.state = dwState;
    lg.stateMask = dwState;

#ifdef LVGS_COLLAPSIBLE
    // Maintain LVGS_COLLAPSIBLE state
    if (HasGroupState(nGroupId, LVGS_COLLAPSIBLE))
        lg.state |= LVGS_COLLAPSIBLE;
#endif

    if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
}

BOOL CListGroupCtrl::IsGroupStateEnabled()
{
    if (!IsGroupViewEnabled())
        return FALSE;

    if (!IsWindowsVistaOrGreater())
        return FALSE;

    return TRUE;
}

void CListGroupCtrl::CollapseAllGroups()
{
    if (!IsGroupStateEnabled())
        return;

    // Loop through all rows and find possible groups
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        const int nGroupId = GetRowGroupId(nRow);
        if (nGroupId != -1)
        {
            if (!HasGroupState(nGroupId, LVGS_COLLAPSED))
            {
                SetGroupState(nGroupId, LVGS_COLLAPSED);
            }
        }
    }
}

void CListGroupCtrl::ExpandAllGroups()
{
    if (!IsGroupStateEnabled())
        return;

    // Loop through all rows and find possible groups
    for (int nRow = 0; nRow < GetItemCount(); ++nRow)
    {
        const int nGroupId = GetRowGroupId(nRow);
        if (nGroupId != -1)
        {
            if (HasGroupState(nGroupId, LVGS_COLLAPSED))
            {
                SetGroupState(nGroupId, LVGS_NORMAL);
            }
        }
    }
}

BOOL CListGroupCtrl::SetGroupFooter(int nGroupID, const CString& footer, DWORD dwAlign /*= LVGA_FOOTER_CENTER*/)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_FOOTER | LVGF_ALIGN;
    lg.uAlign = dwAlign;
#ifdef UNICODE
    lg.pszFooter = const_cast<decltype(lg.pszSubtitle)>(footer.GetString());
    lg.cchFooter = footer.GetLength();
#else
    CComBSTR bstrFooter = footer;
    lg.pszFooter = bstrFooter;
    lg.cchFooter = bstrFooter.Length();
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupTask(int nGroupID, const CString& task)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TASK;
#ifdef UNICODE
    lg.pszTask = const_cast<decltype(lg.pszSubtitle)>(task.GetString());
    lg.cchTask = task.GetLength();
#else
    CComBSTR bstrTask = task;
    lg.pszTask = bstrTask;
    lg.cchTask = bstrTask.Length();
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupSubtitle(int nGroupID, const CString& subtitle)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_SUBTITLE;
#ifdef UNICODE
    lg.pszSubtitle = const_cast<decltype(lg.pszSubtitle)>(subtitle.GetString());
    lg.cchSubtitle = subtitle.GetLength();
#else
    CComBSTR bstrSubtitle = subtitle;
    lg.pszSubtitle = bstrSubtitle;
    lg.cchSubtitle = bstrSubtitle.Length();
#endif

    if (SetGroupInfo(nGroupID, &lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CListGroupCtrl::SetGroupTitleImage(int nGroupID, int nImage, const CString& topDesc, const CString& bottomDesc)
{
    if (!IsGroupStateEnabled())
        return FALSE;

#if _WIN32_WINNT >= 0x0600
    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TITLEIMAGE;
    lg.iTitleImage = nImage;	// Index of the title image in the control imagelist.

#ifdef UNICODE
    if (!topDesc.IsEmpty())
    {
        // Top description is drawn opposite the title image when there is
        // a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
        lg.mask |= LVGF_DESCRIPTIONTOP;
        lg.pszDescriptionTop = (LPWSTR)(LPCTSTR)topDesc;
        lg.cchDescriptionTop = topDesc.GetLength();
    }
    if (!bottomDesc.IsEmpty())
    {
        // Bottom description is drawn under the top description text when there is
        // a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
        lg.mask |= LVGF_DESCRIPTIONBOTTOM;
        lg.pszDescriptionBottom = (LPWSTR)bottomDesc.GetString();
        lg.cchDescriptionBottom = bottomDesc.GetLength();
    }
#else
    CComBSTR bstrTopDesc = topDesc;
    CComBSTR bstrBottomDesc = bottomDesc;
    if (!topDesc.IsEmpty())
    {
        lg.mask |= LVGF_DESCRIPTIONTOP;
        lg.pszDescriptionTop = bstrTopDesc;
        lg.cchDescriptionTop = bstrTopDesc.Length();
    }
    if (!bottomDesc.IsEmpty())
    {
        lg.mask |= LVGF_DESCRIPTIONBOTTOM;
        lg.pszDescriptionBottom = bstrBottomDesc;
        lg.cchDescriptionBottom = bstrBottomDesc.Length();
    }
#endif

    if (SetGroupInfo(nGroupID, (PLVGROUP)&lg) == -1)
        return FALSE;

    return TRUE;
#else
    return FALSE;
#endif
}

void CListGroupCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (pWnd == GetHeaderCtrl())
    {
        CPoint pt = point;
        ScreenToClient(&pt);

        HDHITTESTINFO hdhti = { 0 };
        hdhti.pt = pt;
        hdhti.pt.x += GetScrollPos(SB_HORZ);
        ::SendMessage(GetHeaderCtrl()->GetSafeHwnd(), HDM_HITTEST, 0, (LPARAM)&hdhti);
        if (hdhti.iItem != -1)
        {
            // Retrieve column-title
            LVCOLUMN lvc = { 0 };
            lvc.mask = LVCF_TEXT;
            TCHAR sColText[256] = { 0 };
            lvc.pszText = sColText;
            lvc.cchTextMax = sizeof(sColText) - 1;
            VERIFY(GetColumn(hdhti.iItem, &lvc));

            CMenu menu;
            const UINT uFlags = MF_BYPOSITION | MF_STRING;
            VERIFY(menu.CreatePopupMenu());
            if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
            {
                menu.InsertMenu(0, uFlags, 1, sCheckAll);						// Check All
                menu.InsertMenu(1, uFlags, 2, sUnCheckALL);						// UnCheck All
                menu.InsertMenu(2, uFlags | MF_SEPARATOR, 3, _T(""));
            }
            menu.InsertMenu(3, uFlags, 4, sGroupBy + L": " + lvc.pszText);	// Group by:
            if (IsGroupViewEnabled())
                menu.InsertMenu(4, uFlags, 5, sDisableGrouping);			// Disable grouping
            int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
            switch (nResult)
            {
            case 1: CheckElements(true);  break;
            case 2: CheckElements(false); break;
            case 4:	GroupByColumn(hdhti.iItem); break;
            case 5: RemoveAllGroups(); EnableGroupView(FALSE); break;
            }
        }
        return;
    }

    if (IsGroupViewEnabled())
    {
        if (point.x != -1 && point.y != -1)
        {
            CMenu menu;
            const UINT uFlags = MF_BYPOSITION | MF_STRING;
            VERIFY(menu.CreatePopupMenu());

            CPoint pt = point;
            ScreenToClient(&pt);
            int nGroupId = GroupHitTest(pt);
            if (nGroupId >= 0)
            {
                const CString& groupHeader = GetGroupHeader(nGroupId);

                if (IsGroupStateEnabled())
                {
                    if (HasGroupState(nGroupId, LVGS_COLLAPSED))
                    {
                        const CString menuText = sExpandGroup + L": " + groupHeader;		// Expand group:
                        menu.InsertMenu(0, uFlags, 1, menuText);
                    }
                    else
                    {
                        const CString menuText = sCollapseGroup + L": " + groupHeader;	// Collapse group:
                        menu.InsertMenu(0, uFlags, 2, menuText);
                    }
                }
                CString menuText = sCheckGroup + L": " + groupHeader;	// Check group:
                menu.InsertMenu(1, uFlags, 3, menuText);
                menuText = sUnCheckGroup + L": " + groupHeader;			// Uncheck group:
                menu.InsertMenu(2, uFlags, 4, menuText);
                menuText = sDeleteGroup + L": " + groupHeader;			// Delete group
                menu.InsertMenu(3, uFlags, 5, menuText);

                menu.InsertMenu(4, uFlags | MF_SEPARATOR, 6, _T(""));
            }

            int nRow = HitTest(pt);
            if (nRow == -1)
            {
                if (IsGroupStateEnabled())
                {
                    menu.InsertMenu(5, uFlags, 7, sExpandAllGroups);	// Expand all groups
                    menu.InsertMenu(6, uFlags, 8, sCollapseAllGroups);	// Collapse all groups
                }
                menu.InsertMenu(7, uFlags, 9, sDisableGrouping);		// Disable grouping
            }
            else
            {
                nGroupId = GetRowGroupId(nRow);
                if (IsGroupStateEnabled())
                {
                    const CString& groupHeader = GetGroupHeader(nGroupId);

                    if (HasGroupState(nGroupId, LVGS_COLLAPSED))
                    {
                        const CString menuText = sExpandGroup + L": " + groupHeader;		// Expand group:
                        menu.InsertMenu(0, uFlags, 1, menuText);
                    }
                    else
                    {
                        const CString menuText = sCollapseGroup + L": " + groupHeader;	// Collapse group:
                        menu.InsertMenu(0, uFlags, 2, menuText);
                    }
                }
            }

            const int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
            switch (nResult)
            {
            case 1: SetGroupState(nGroupId, LVGS_NORMAL); break;
            case 2: SetGroupState(nGroupId, LVGS_COLLAPSED); break;
            case 3: CheckEntireGroup(nGroupId, true); break;
            case 4: CheckEntireGroup(nGroupId, false); break;
            case 5: DeleteEntireGroup(nGroupId); break;
            case 7: ExpandAllGroups(); break;
            case 8: CollapseAllGroups(); break;
            case 9: RemoveAllGroups(); EnableGroupView(FALSE); break;
            }
        }
    }
}

namespace {
struct PARAMSORT
{
    CListGroupCtrl* m_control;
    int  m_ColumnIndex;
    CListGroupCtrl::SortType m_sortType;
    CSimpleMap<int, CString> m_GroupNames;
    std::function<int(CString, CString)> m_SortFunction;

    explicit PARAMSORT(CListGroupCtrl* control, int nCol, CListGroupCtrl::SortType sortType,
                       const std::function<int(CString, CString)>& sortFunc = _tcscmp)
        : m_control(control)
        , m_ColumnIndex(nCol)
        , m_sortType(sortType)
        , m_SortFunction(sortFunc)
    {}

    const CString& LookupGroupName(int nGroupId)
    {
        const int groupIdx = m_GroupNames.FindKey(nGroupId);
        if (groupIdx == -1)
        {
            static const CString emptyStr;
            return emptyStr;
        }
        return m_GroupNames.GetValueAt(groupIdx);
    }
};

// Comparison extracts values from the List-Control
int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PARAMSORT* ps = (PARAMSORT*)lParamSort;

    if (ps->m_sortType != CListGroupCtrl::SortType::eAscending &&
        ps->m_sortType != CListGroupCtrl::SortType::eDescending)
    {
        return ps->m_control->GetDefaultItemIndex((int)lParam1) < ps->m_control->GetDefaultItemIndex((int)lParam2);
    }

    TCHAR left[256] = _T(""), right[256] = _T("");
    ListView_GetItemText(ps->m_control->m_hWnd, lParam1, ps->m_ColumnIndex, left, sizeof(left));
    ListView_GetItemText(ps->m_control->m_hWnd, lParam2, ps->m_ColumnIndex, right, sizeof(right));

    switch (ps->m_sortType)
    {
    case CListGroupCtrl::SortType::eAscending:
        return ps->m_SortFunction(left, right);
    case CListGroupCtrl::SortType::eDescending:
        return ps->m_SortFunction(right, left);
    default:
        ASSERT(false);
        return 0;
    }
}

int CALLBACK SortFuncGroup(int nLeftId, int nRightId, void* lParamSort)
{
    PARAMSORT* ps = (PARAMSORT*)lParamSort;

    const CString& left = ps->LookupGroupName(nLeftId);
    const CString& right = ps->LookupGroupName(nRightId);

    if (ps->m_sortType != CListGroupCtrl::SortType::eDescending)
        return _tcscmp(left, right);
    else
        return _tcscmp(right, left);
}
}

bool CListGroupCtrl::SortColumn(int columnIndex, SortType sortType)
{
    m_SortCol = columnIndex;
    m_sortType = sortType;

    CWaitCursor waitCursor;

    const auto it = std::find_if(m_SortFunct.begin(), m_SortFunct.end(),
                                 [&columnIndex](const std::pair<int, std::function<int(CString, CString)>>& Element)
                                 {
                                     return Element.first == columnIndex;
                                 });

    PARAMSORT paramSort(this, columnIndex, sortType);

    if (it != std::end(m_SortFunct))
        paramSort.m_SortFunction = it->second;
    else
        paramSort.m_SortFunction = _tcscmp;

    if (IsGroupViewEnabled())
    {
        SetRedraw(FALSE);

        if (m_ChangeGroupsWhileSort)
            GroupByColumn(columnIndex);

        // Cannot use GetGroupInfo during sort
        for (int nRow = 0; nRow < GetItemCount(); ++nRow)
        {
            int nGroupId = GetRowGroupId(nRow);
            if (nGroupId != -1 && paramSort.m_GroupNames.FindKey(nGroupId) == -1)
                paramSort.m_GroupNames.Add(nGroupId, GetGroupHeader(nGroupId));
        }

        SetRedraw(TRUE);
        Invalidate(FALSE);

        // Avoid bug in CListCtrl::SortGroups() which differs from ListView_SortGroups
        if (!ListView_SortGroups(m_hWnd, SortFuncGroup, &paramSort))
            return false;

        // СОРТИРУЕМ ЭЛЕМЕНТЫ ПО УМОЛЧАНИЮ
        ListView_SortItemsEx(m_hWnd, SortFunc, &paramSort);
    }
    else
    {
        ListView_SortItemsEx(m_hWnd, SortFunc, &paramSort);
    }

    SetSortArrow(m_SortCol, m_sortType);
    return true;
}

BOOL CListGroupCtrl::OnGroupTaskClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
#if _WIN32_WINNT >= 0x0600
    const NMLVLINK* pLinkInfo = (NMLVLINK*)pNMHDR;
    const int nGroupId = pLinkInfo->iSubItem;

    if (!IsGroupStateEnabled())
        return FALSE;

    // текст
    CString TaskText;
    DWORD Size = std::max<int>(sCheckGroup.GetLength(), sUnCheckGroup.GetLength()) + 1;

    LVGROUP lg = { 0 };
    lg.cbSize = sizeof(lg);
    lg.mask = LVGF_TASK;
    lg.pszTask = TaskText.GetBuffer(Size);
    lg.cchTask = Size;
    GetGroupInfo(nGroupId, (PLVGROUP)&lg);
    TaskText.ReleaseBuffer();

    // если группы были созданы автоматически, то нажатие на них включает элементы таблицы
    if ((TaskText == sCheckGroup) || (TaskText == sUnCheckGroup))
    {
        if (TaskText == sCheckGroup)
        {
            TaskText = sUnCheckGroup;
            CheckEntireGroup(nGroupId, true);
        }
        else
        {
            TaskText = sCheckGroup;
            CheckEntireGroup(nGroupId, false);
        }

        Size = TaskText.GetLength();
        lg.pszTask = TaskText.GetBuffer(Size);
        lg.cchTask = Size;
        SetGroupInfo(nGroupId, (PLVGROUP)&lg);
    }
    else
        CheckEntireGroup(nGroupId, true);
#endif
    return FALSE;
}

void CListGroupCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CListCtrl::OnLButtonDblClk(nFlags, point);

    if (!IsGroupStateEnabled())
        return;

    GroupHitTest(point);
}

BOOL CListGroupCtrl::OnHeaderClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
    // We enable HDS_BUTTONS style for checkboxes and can receive clicks there
    if (GetStyle() & LVS_NOSORTHEADER)
        return true;

    const NMLISTVIEW* pLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

    const int nCol = pLV->iSubItem;
    SortType sortType;

    if (m_SortCol == nCol)
    {
        switch (m_sortType)
        {
        case SortType::eNone:
            sortType = SortType::eAscending;
            break;
        case SortType::eAscending:
            sortType = SortType::eDescending;
            break;
        case SortType::eDescending:
            sortType = SortType::eNone;
            break;
        default:
            ASSERT(false);
            break;
        }
    }
    else
        sortType = SortType::eAscending;

    SortColumn(nCol, sortType);
    return FALSE;	// Let parent-dialog get chance
}

//----------------------------------------------------------------------------//
BOOL CListGroupCtrl::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
    const LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // отрабатываем изменение состояния чека элементов на основной кнопке
    if ((pNMLV->uChanged & LVIF_STATE) && (CListCtrl::GetExtendedStyle() & LVS_EX_CHECKBOXES))
    {
        const auto newState = pNMLV->uNewState & LVIS_STATEIMAGEMASK;
        if ((newState & 0x2000) == 0 && (newState & 0x1000) == 0)
            return FALSE;

        // проверяем нужно ли поменять состояние основного чекбокса
        bool bAllChecked(newState & 0x2000);
        // ищем по отображаемым данным, есть ли там запрашиваемый жлемент
        for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); bAllChecked && Row < nItemsCount; ++Row)
        {
            bAllChecked = CListCtrl::GetCheck(Row) == BST_CHECKED;
        }

        CHeaderCtrl* header = CListCtrl::GetHeaderCtrl();
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        Header_GetItem(*header, 0, &hdi);
        const auto prevFormat = hdi.fmt;
        if (bAllChecked)
            hdi.fmt |= HDF_CHECKED;
        else
            hdi.fmt &= ~HDF_CHECKED;
        if (prevFormat != hdi.fmt)
            Header_SetItem(*header, 0, &hdi);
    }

    return FALSE;	// Let parent-dialog get chance
}

void CListGroupCtrl::Resort()
{
    if (m_SortCol >= 0)
        SortColumn(m_SortCol, m_sortType);
}

void CListGroupCtrl::SetSortArrow(int colIndex, SortType sortType)
{
    // Для задания собственной иконки сортировки необходимо загрузить BMP изображения в проект с идентификаторами:
    // IDB_DOWNARROW для сортировки вниз и IDB_UPARROW для сортировки вверх
#if defined(IDB_DOWNARROW) && defined(IDB_UPARROW)
    UINT bitmapID = sortType == SortType::eAscending ? IDB_DOWNARROW : IDB_UPARROW;
    for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
    {
        HDITEM hditem = { 0 };
        hditem.mask = HDI_BITMAP | HDI_FORMAT;
        VERIFY(GetHeaderCtrl()->GetItem(i, &hditem));
        if (hditem.fmt & HDF_BITMAP && hditem.fmt & HDF_BITMAP_ON_RIGHT)
        {
            if (hditem.hbm)
            {
                DeleteObject(hditem.hbm);
                hditem.hbm = NULL;
            }
            hditem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hditem));
        }
        if (i == colIndex && sortType != SortType::eNone)
        {
            hditem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
            hditem.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
            VERIFY(hditem.hbm != NULL);
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hditem));
        }
    }
#else
#if (_WIN32_WINNT >= 0x501)
    if (IsThemeEnabled())
    {
        for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
        {
            HDITEM hdItem = { 0 };
            hdItem.mask = HDI_FORMAT;
            VERIFY(GetHeaderCtrl()->GetItem(i, &hdItem));
            hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (i == colIndex)
            {
                switch (sortType)
                {
                case SortType::eAscending:
                    hdItem.fmt |= HDF_SORTDOWN;
                    break;
                case SortType::eDescending:
                    hdItem.fmt |= HDF_SORTUP;
                    break;
                default:
                    break;
                }
            }
            VERIFY(CListCtrl::GetHeaderCtrl()->SetItem(i, &hdItem));
        }
    }
#endif
#endif
}

void CListGroupCtrl::PreSubclassWindow()
{
    CListCtrl::PreSubclassWindow();

    DWORD extendedListStyle = GetExtendedStyle();
    // Focus retangle is not painted properly without double-buffering
#if (_WIN32_WINNT >= 0x501)
    extendedListStyle |= LVS_EX_DOUBLEBUFFER;
#endif
    extendedListStyle |= LVS_EX_FULLROWSELECT |
        LVS_EX_HEADERDRAGDROP |
        LVS_EX_GRIDLINES |
        LVS_EX_INFOTIP | LVS_EX_LABELTIP;
    SetExtendedStyle(extendedListStyle);

    // Enable Vista-look if possible
    EnableWindowTheme(GetSafeHwnd(), L"ListView", L"Explorer", NULL);
    EnableWindowTheme(GetHeaderCtrl()->GetSafeHwnd(), L"ListView", L"Explorer", NULL);

    if (extendedListStyle & LVS_EX_CHECKBOXES)
    {
        ModifyExtendedStyle(0, LVS_EX_CHECKBOXES);
    }
}

bool CListGroupCtrl::FindItemInTable(_In_ CString&& psSearchText, _In_ unsigned RowNumber, _In_opt_ bool bCaseSensitive /*= false*/)
{
    const unsigned nTotalRows = CListCtrl::GetItemCount();
    const int nTotalColumns = CListCtrl::GetHeaderCtrl()->GetItemCount();

    if (RowNumber < nTotalRows)
    {
        for (int Column = 0; Column < nTotalColumns; ++Column)
        {
            CString&& sColumnText = CListCtrl::GetItemText(RowNumber, Column);
            if (!bCaseSensitive)
            {
                sColumnText = sColumnText.MakeLower();
                psSearchText = psSearchText.MakeLower();
            }

            if (sColumnText.Find(psSearchText) != -1)
                return true;
        }
    }

    return false;
}

// BOOL CListGroupCtrl::GetDefaultItemCheck(_In_ int nRow)
// {
// 	if (GetItemRealIndex(nRow) == nRow)
// 		return CListCtrl::GetCheck(nRow);
//
// 	// ищем по отображаемым данным, есть ли там запрашиваемый жлемент
// 	for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
// 	{
// 		if (GetItemRealIndex(Row) == nRow)
// 			return CListCtrl::GetCheck(Row);
// 	}
//
// 	// ищем по удаленным элементам
// 	for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
// 	{
// 		if (((ListItemData*)it->ItemData.lParam)->defaultItemIndex == nRow)
// 			return it->bChecked;
// 	}
//
// 	// если нигде нету, то вызываем функцию базового класса
// 	return CListCtrl::GetCheck(nRow);;
// }

// BOOL CListGroupCtrl::SetDefaultItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText)
// {
// 	// ищем по отображаемым данным, есть ли там запрашиваемый жлемент
// 	for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
// 	{
// 		if (GetItemRealIndex(Row) == nItem)
// 			return CListCtrl::SetItemText(Row, nSubItem, lpszText);
// 	}
//
// 	// ищем по удаленным элементам
// 	for (auto it = m_DeletedItems.begin(); it != m_DeletedItems.end();)
// 	{
// 		if (((ListItemData*)it->ItemData.lParam)->defaultItemIndex == nItem)
// 		{
// 			if ((int)it->ColumnsText.size() > nSubItem)
// 			{
// 				it->ColumnsText[nSubItem] = lpszText;
// 				return TRUE;
// 			}
// 			else
// 				break;
// 		}
// 	}
//
// 	return CListCtrl::SetItemText(nItem, nSubItem, lpszText);
// }

void CListGroupCtrl::CheckElements(bool bChecked)
{
    // ищем по отображаемым данным, есть ли там запрашиваемый жлемент
    for (int Row = 0, nItemsCount = CListCtrl::GetItemCount(); Row < nItemsCount; ++Row)
    {
        CListCtrl::SetCheck(Row, bChecked);
    }

    CHeaderCtrl* header = CListCtrl::GetHeaderCtrl();
    HDITEM hdi = { 0 };
    hdi.mask = HDI_FORMAT;
    Header_GetItem(*header, 0, &hdi);
    if (bChecked)
        hdi.fmt |= HDF_CHECKED;
    else
        hdi.fmt &= ~HDF_CHECKED;
    Header_SetItem(*header, 0, &hdi);
}

BOOL CListGroupCtrl::OnHdnItemStateIconClick(UINT,NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMHEADER pNMHeader = (LPNMHEADER)pNMHDR;

    // first determine whether the click was a checkbox change
    if (pNMHeader->pitem->mask & HDI_FORMAT && pNMHeader->pitem->fmt & HDF_CHECKBOX)
    {
        // now determine whether it was checked or unchecked
        const BOOL bUnChecked = pNMHeader->pitem->fmt & HDF_CHECKED;

        CheckElements(bUnChecked == FALSE);
    }

    return FALSE;
}

BOOL CListGroupCtrl::OnHeaderBeginDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    m_columnDragging = true;
    return FALSE;
}

BOOL CListGroupCtrl::OnHeaderEndDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    m_columnDragging = false;
    return FALSE;
}

void CListGroupCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_SPACE && !is_key_pressed(VK_SHIFT) && (GetExtendedStyle() & LVS_EX_CHECKBOXES))
    {
        if (const int selCount = GetSelectedCount(); selCount != 0)
        {
            POSITION pos = GetFirstSelectedItemPosition();
            for (int index = 0; index < selCount; ++index)
            {
                const int realIndex = GetNextSelectedItem(pos);
                CListCtrl::SetCheck(realIndex, CListCtrl::GetCheck(realIndex) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
            }
            return;
        }
    }
    CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CListGroupCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == 'A' && is_key_pressed(VK_CONTROL) && !is_key_pressed(VK_SHIFT))
    {
        SetRedraw(FALSE);
        for (int i = 0, itemCount = GetItemCount(); i < itemCount; ++i)
        {
            SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
        }
        SetRedraw(TRUE);
        Invalidate();
    }

    CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CListGroupCtrl::OnSize(UINT nType, int cx, int cy)
{
    CListCtrl::OnSize(nType, cx, cy);

    // During resizing we scroll can appear and we will receive OnSize again, add this check to avoid loops
    if (m_resizingColumns)
        return;

    if (!m_columnsProportions.empty() && !m_columnDragging)
    {
        int controlWidth = cx;
        const auto columnsCount = GetHeaderCtrl()->GetItemCount();
        for (int column = 0; column < columnsCount; ++column)
        {
            if (m_columnsProportions.find(column) == m_columnsProportions.end())
                controlWidth -= GetColumnWidth(column);
        }

        if (controlWidth < 0)
            controlWidth = 0;

        static const int vertScrollWidth = GetSystemMetrics(SM_CXVSCROLL);

        m_resizingColumns = true;
        // Sometimes scroll lagging and we can't predict it
        for (int i = 0; i < vertScrollWidth; ++i)
        {
            for (auto&& [columnIndex, proportion] : m_columnsProportions)
            {
                SetColumnWidth(columnIndex, static_cast<int>((double)controlWidth / proportion));
            }

            if (!(GetWindowLong(m_hWnd, GWL_STYLE) & WS_HSCROLL))
                break;

            --controlWidth;
        }
        m_resizingColumns = false;
    }

    // Forcing redraw to fix problems:
    // - Flickering control data on resize
    // - Control full with data and has a vertical scroll. After increasing control size at
    //   the moment when scroll dissapears control becomes empty
    RedrawWindow();
    RedrawItems(0, GetItemCount());
}

BOOL CListGroupCtrl::OnEraseBkgnd(CDC* pDC)
{
    // fix flickering
    CRect clientRect;
    GetClientRect(&clientRect);

    if (CHeaderCtrl* pHeadCtrl = GetHeaderCtrl())
    {
        CRect  rcHead;
        pHeadCtrl->GetWindowRect(&rcHead);
        const auto nHeadHeight = rcHead.Height();
        clientRect.top += nHeadHeight;
    }

    CRect itemsRect;
    if (const auto nItems = GetItemCount())
    {
        CPoint  ptItem;
        CRect   rcItem;

        GetItemRect(nItems - 1, &rcItem, LVIR_BOUNDS);
        GetItemPosition(nItems - 1, &ptItem);

        itemsRect.top = clientRect.top;
        itemsRect.left = ptItem.x;
        itemsRect.right = rcItem.right;
        itemsRect.bottom = rcItem.bottom;

        if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
            itemsRect.left -= GetSystemMetrics(SM_CXEDGE) + 16;
    }

    CBrush brush(GetBkColor());
    if (itemsRect.IsRectEmpty())
        pDC->FillRect(clientRect, &brush);
    else
    {
        if (itemsRect.left > clientRect.left)     // fill left rectangle
            pDC->FillRect(CRect(0, clientRect.top, itemsRect.left, clientRect.bottom), &brush);
        if (itemsRect.bottom < clientRect.bottom) // fill bottom rectangle
            pDC->FillRect(CRect(0, itemsRect.bottom, clientRect.right, clientRect.bottom), &brush);
        if (itemsRect.right < clientRect.right)   // fill right rectangle
            pDC->FillRect(CRect(itemsRect.right, clientRect.top, clientRect.right, clientRect.bottom), &brush);
    }

    return TRUE;
}

bool CListGroupCtrl::bIsNeedToRestoreDeletedItem(std::list<DeletedItemsInfo>::iterator Item)
{
    // проверяем что элемент удовлетворяет условиям поиска
    bool bFind = m_bMatchAll;
    for (auto& String : m_sFindStrings)
    {
        bool bFindInColumn = false;
        for (auto& CurText : Item->ColumnsText)
        {
            if (CurText.MakeLower().Find(String.MakeLower()) != -1)
            {
                bFindInColumn = true;
                break;
            }
        }

        if (m_bMatchAll)	// проверяем что строчка таблицы должна совпадать со всеми элементами вектора
            bFind &= bFindInColumn;
        else            // проверяем что строчка таблицы должна совпадать хотябы с одним элементом вектора
            bFind |= bFindInColumn;
    }

    bool bDelete(false);
    // если этот элемент удовлетворяет условиям поиска то восстанавливаем его, если нет то переходим к следующему
    if (bFind)
    {
        int n(0);
        for (size_t Column = 0, Column_Count = Item->ColumnsText.size(); Column < Column_Count; Column++)
        {
            if (Column == 0)
            {
                Item->ItemData.cchTextMax = Item->ColumnsText.front().GetLength();
                Item->ItemData.pszText = Item->ColumnsText.front().GetBuffer();
                n = CListCtrl::InsertItem(&Item->ItemData);
                Item->ColumnsText.front().ReleaseBuffer();
                CListCtrl::SetCheck(n, Item->bChecked);
            }
            else
                CListCtrl::SetItemText(n, (int)Column, Item->ColumnsText[Column]);
        }

        bDelete = true;
    }

    return bDelete;
}

void CListGroupCtrl::OnDestroy()
{
    DeleteAllItems();
    CListCtrl::OnDestroy();
}
