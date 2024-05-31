#pragma once
#include "afxcmn.h"
#include <list>
#include <vector>
#include <functional>
#include <unordered_set>
#include <unordered_map>
//*************************************************************************************************
/*  Ремарки
    1) Для корректного перевода, добавьте в stdafx.h подключение библиотеки перевода
    2) Для задания собственной иконки сортировки необходимо загрузить BMP изображения в проект с идентификаторами:
        IDB_DOWNARROW для сортировки вниз и IDB_UPARROW для сортировки вверх
    3) При поиске и сортировке индексы строк меняются, для того чтобы узнать какой индекс был изначально у строки
        используйте GetDefaultRowIndex подав в нее текущий индекс, для того чтобы узнать какой сейчас индекс у элемента
        который был добавлен импользуйте GetCurrentRowIndex подав в нее исходный индекс
    3) При ловле событий lparam является индексом строчки который был при добавлении ее в таблце
        а iItem текущий индекс строчки
    4) Если установлен флаг работы с галочками LVS_EX_CHECKBOXES и есть необходимость переключения основной галочки,
        то в функции обработчике OnLvnItemchanged  необходимо вызвать функцию OnCheckItem

*/
//*************************************************************************************************
/* Пример работы с классом:

    // отобрааем галочки
    m_Tree.ModifyStyle(0, LVS_EX_CHECKBOXES);
    // добавляем колонку
    m_Tree.InsertColumn(1, L"Название колонки", LVCFMT_CENTER, 100);

    int GroupID = 0;
    int CurIndex = 0;
Способ 1:
    m_Tree.InsertGroupHeader(0, GroupID++, GroupName, LVGS_COLLAPSIBLE | LVGS_COLLAPSED, LVGA_HEADER_LEFT);
    m_Tree.InsertItem(CurIndex++, GroupID, Name1);
    m_Tree.InsertItem(CurIndex++, GroupID, Name2);
Способ 2:
    m_Tree.InsertItem(CurIndex++, Name1);
    m_Tree.InsertItem(CurIndex++, Name2);
    m_Tree.GroupByColumn(0);
Добавление функции для сортировки колонки:
    m_DeviceTree.InsertColumn(1, TranslateString(L"Добавлено на склад"), LVCFMT_CENTER, 100, -1, SortDateFunct);
Функция сортировки
    int SortDateFunct(CString First, CString Second)
    {
        CZetTime ztFirst;
        ztFirst.ConvertFromString(First.GetBuffer(), L"dd-MM-yyyy HH:mm:ss");
        CZetTime ztSecond;
        ztSecond.ConvertFromString(Second.GetBuffer(), L"dd-MM-yyyy HH:mm:ss");

        if (ztFirst == ztSecond)
            return 0;
        else if (ztFirst < ztSecond)
            return -1;
        else
            return 1;
    }
*/
//*************************************************************************************************
class CListGroupCtrl : public CListCtrl
{
    // Store items real index(with which it was inserted)
    void SetDefaultItemIndex(int nCurrentItem, int nRealItem);
    // Use SetItemDataPtr and GetItemDataPtr
    using CListCtrl::GetItemData;
    using CListCtrl::SetItemData;
public:	//*****************************************************************************************
    // если true - то при сортировке произойдет обьединение в группы по номеру колонки
    // если false - то просто отсортируются
    bool m_ChangeGroupsWhileSort = false;
public:	//*****************************************************************************************
    CListGroupCtrl();
    //*********************************************************************************************
    // Добавление строчки в таблицу с привязкой к группе
    // Returns:   int		- индекс добавленного элемента
    // Parameter: nItem		- индекс элемента в таблице
    // Parameter: nGroupID	- индекс группы к которой привязываем элемент
    // Parameter: lpszItem	- название элемента
    int InsertItem(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem);
    // стандартное добавление элемента
    int InsertItem(_In_ int nItem, _In_z_ LPCTSTR lpszItem);
    int InsertItem(_In_ const LVITEM* pItem);
    //*********************************************************************************************
    // Sets the text associated with a particular item.
    BOOL SetItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText);
    //*********************************************************************************************
    void SetItemDataPtr(int nIndex, void* pData);
    void* GetItemDataPtr(int nIndex) const;
    // During data sorting control will remove items
    // Returns: default(inserted) item index of currently visible item
    int GetDefaultItemIndex(int nCurrentItem) const;
    // During data sorting control will remove items
    // Returns: current index in table based on default(inserted) item, -1 if item was sorted
    int GetCurrentIndexFromDefaultItem(int nDefaultItem) const;
    //**********************************************************************************************
// 	BOOL SetDefaultItemText(_In_ int nItem, _In_ int nSubItem, _In_z_ LPCTSTR lpszText);
// 	// возвращает выбран элемент сейчас или нет
// 	BOOL GetDefaultItemCheck(_In_ int nRow);
    //**********************************************************************************************
    // Перемещение элемента по списку
    // itemFirst - индекс перемещаемого элемента
    // moveUp    - перемещать вверх или вниз
    // Returns:  - индекс элемента после перемещения
    int MoveItem(_In_ int itemFirst, _In_ bool moveUp);
    // Move currently selected items in list
    // moveUp    - move up or down
    void MoveSelectedItems(_In_ bool moveUp);
    // Removes all items from the control.
    BOOL DeleteAllItems();
    BOOL DeleteItem(_In_ int nItem);
    // поиск по таблице
    void FindItems(_In_ CString sFindString);
    // bMatchAll - если true то ищем совпадение со всеми элементами массива, если false - то хотябы с одним
    void FindItems(_In_ std::list<CString> sFindStrings, _In_opt_ bool bMatchAll = false);
    void ResetSearch();
    //*********************************************************************************************
    void SelectItem(int nItem, bool ensureVisible = true);
    int GetLastSelectedItem() const;
    std::vector<int> GetSelectedItems() const;
    void ClearSelection();
    //*********************************************************************************************
    // добавление колонки с указателем на функцию сортировки
    // SortFunct - функция сортировки, должна возвращать число и принимать 2 строки типа CString
    // возвращаемые значения функции сортировки:
    // <0 string1 is less than string2
    // 0  string1 is identical to string2
    // >0 string1 is greater than string2
    int InsertColumn(_In_ int nCol, _In_ LPCTSTR lpszColumnHeading, _In_opt_ int nFormat = LVCFMT_LEFT, _In_opt_ int nWidth = -1,
                     _In_opt_ int nSubItem = -1, _In_opt_ std::function<int(const CString&, const CString&)> SortFunct = nullptr);
    // стандартное добавление колонки
    int InsertColumn(_In_ int nCol, _In_ const LVCOLUMN* pColumn);
    //*********************************************************************************************
    // автомасштабирование колонок в зависимости от размеров контрола, колонки становятся одинакового размера
    void AutoScaleColumns();
    // автомасштабирование колонки по содержимому
    void FitToContentColumn(int nCol, bool bIncludeHeaderContent);
    // Add list of columns which will keep the size on resize control
    void SetProportionalResizingColumns(const std::unordered_set<int>& columns);
    //*********************************************************************************************
    // меняем стиль таблицы LVS_EX_CHECKBOXES - чекбоксы
    BOOL ModifyStyle(_In_ DWORD dwRemove, _In_ DWORD dwAdd, _In_opt_ UINT nFlags = 0);
    //*********************************************************************************************
    // все элементы таблицы будут выбраны в зависимости от bCkeched
    void CheckAllElements(bool bChecked);
    void CheckEntireGroup(int nGroupId, bool bChecked);
    // получаем список выбранных элементов
    // bCurrentIndexes - true то возвращаются текущие индкексы | false - индексы которые были при добавлении данных
    std::vector<int> GetCheckedList(_In_opt_ bool bCurrentIndexes = true) const;
    //*********************************************************************************************
    // Создаем группу
    // Parameter: nIndex    - индекс группы в дереве, можно задать нулевыми
    // Parameter: nGroupID  - индекс группы
    // Parameter: strHeader - название группы
    // Parameter: dwState	- состояние группы например LVGS_COLLAPSIBLE | LVGS_COLLAPSED
    // Parameter: dwAlign	- привязка названия группы относительно всего контрола
    LRESULT InsertGroupHeader(_In_ int nIndex, _In_ int nGroupID, _In_ const CString& strHeader,
                              _In_opt_ DWORD dwState = LVGS_NORMAL, _In_opt_ DWORD dwAlign = LVGA_HEADER_LEFT);
    //*********************************************************************************************
    CString GetGroupHeader(int nGroupID);
    int GetRowGroupId(int nRow);
    BOOL SetRowGroupId(int nRow, int nGroupID);
    int GroupHitTest(const CPoint& point);
    //*********************************************************************************************
    // стандартная группировка колонок по названиям строк
    BOOL GroupByColumn(int nCol);
    void DeleteEntireGroup(int nGroupId);
    BOOL HasGroupState(int nGroupID, DWORD dwState);
    BOOL SetGroupState(int nGroupID, DWORD dwState);
    BOOL IsGroupStateEnabled();
    //*********************************************************************************************
    void CollapseAllGroups();	// свернуть все группы
    void ExpandAllGroups();		// развернуть все группы
    //*********************************************************************************************
    BOOL SetGroupFooter(int nGroupID, const CString& footer, DWORD dwAlign = LVGA_FOOTER_CENTER);
    BOOL SetGroupTask(int nGroupID, const CString& task);
    BOOL SetGroupSubtitle(int nGroupID, const CString& subtitle);
    BOOL SetGroupTitleImage(int nGroupID, int nImage, const CString& topDesc, const CString& bottomDesc);
public://******************************************************************************************
    enum class SortType
    {
        eNone = 0,
        eAscending,
        eDescending,
    };
    bool SortColumn(int columnIndex, SortType sortType);
    SortType GetSortType() const	{ return m_sortType; }
    int  GetSortCol() const		    { return m_SortCol; }

protected:
    void SetSortArrow(int colIndex, SortType sortType);
    void Resort();

protected:
    int m_SortCol = -1;			    // колонка для сортировки
    SortType m_sortType = SortType::eNone;

protected://***************************************************************************************
    virtual void PreSubclassWindow() override;
    //*********************************************************************************************
    afx_msg void OnDestroy();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnGroupTaskClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg BOOL OnHdnItemStateIconClick(UINT,NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //*********************************************************************************************
    DECLARE_MESSAGE_MAP()
protected://***************************************************************************************
    
    //*********************************************************************************************
    struct DeletedItemsInfo
    {
        LVITEM ItemData;
        BOOL bChecked;
        std::vector<CString> ColumnsText;
        DeletedItemsInfo() noexcept
            : bChecked		(TRUE)
        {
            ZeroMemory(&ItemData, sizeof(LVITEM));
        }
        DeletedItemsInfo(_In_ const DeletedItemsInfo &Val) noexcept
            : ItemData		(Val.ItemData)
            , bChecked		(Val.bChecked)
            , ColumnsText	(Val.ColumnsText)
        {}
        DeletedItemsInfo(_In_ BOOL _bChecked, _In_ LVITEM _ItemData) noexcept
            : ItemData		(_ItemData)
            , bChecked		(_bChecked)
        {}
        DeletedItemsInfo& operator=(_In_ const DeletedItemsInfo &Val) noexcept
        {
            ItemData = Val.ItemData;
            bChecked = Val.bChecked;
            ColumnsText = Val.ColumnsText;
            return *this;
        }
        DeletedItemsInfo& operator=(_In_ DeletedItemsInfo&& Val) noexcept
        {
            ItemData = std::move(Val.ItemData);
            bChecked = std::move(Val.bChecked);
            ColumnsText = std::move(Val.ColumnsText);
            return *this;
        }
    };
    std::list<DeletedItemsInfo> m_DeletedItems;
    bool m_bMatchAll = false;
    std::list<CString> m_sFindStrings;
    // список функций для сортировки каждой колонки
    std::vector<std::pair<int, std::function<int(CString, CString)>>> m_SortFunct;

    // list of columns widths, on resizing control all columns will be resize proportionally initial width
    // @see SetProportionalResizingColumns
    std::unordered_map<int, double> m_columnsProportions;
protected://***************************************************************************************
    // поиск текста в заданной строке таблицы
    bool FindItemInTable(_In_ CString&& psSearchText, _In_ unsigned RowNumber, _In_opt_ bool bCaseSensitive = false);
    // проверка на необходимость восстановить удаленный элемент
    bool bIsNeedToRestoreDeletedItem(std::list<DeletedItemsInfo>::iterator Item);
    void CheckElements(bool bChecked);
    //*********************************************************************************************
    // проверяем добавляемый элемент
    bool CheckInsertedElement(_In_ int nItem, _In_ int nGroupID, _In_z_ LPCTSTR lpszItem);
};	//*********************************************************************************************
