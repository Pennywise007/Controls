#pragma once
#include <afxwin.h>
#include <map>
#include <optional>

////////////////////////////////////////////////////////////////////////////////
class ComboWithSearch : public CComboBox
{
    typedef CComboBox BaseCtrl;
public:
    ComboWithSearch();

    void SetWindowText(const CString& text);
    // Resize combobox dropdown size to fit it's content
    void AdjustComboBoxToContent();
    // Allows control to have its own text(not one of the inserted items)
    void AllowCustomText(bool allow = true);
    // Changing contol colors
    void SetBkColor(std::optional<COLORREF> color);
    void SetTextColor(std::optional<COLORREF> color);
    void SetDropdownBkColor(std::optional<COLORREF> color);
private:
    void executeSearch();
    bool resetSearch();
    // Updating real selection(among all items) from currently selected item
    void updateRealCurSelFromCurrent();

private: // функции конвертации индексов
    // из текущего индекса в реальный индекс
    int convertToRealIndex(int index);
    // из реального индекса в текущий
    int convertFromRealIndex(int index);

protected:
    DECLARE_MESSAGE_MAP();

    // for processing Windows messages
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnCbnEditchange();
    afx_msg BOOL OnCbnSelendok();
    afx_msg BOOL OnCbnSelendcancel();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

private:
    // filtered items
    //      item | text
    std::map<int, std::wstring> m_filteredItems;

    // If true doesn't force control to have any of the items values
    bool m_allowCustomText = false;
    // Current control item selected by user in non m_allowCustomText mode
    int m_selectedItemIndex = -1;
    // Flag that we hiding dropdown window from internal functions
    bool m_internalHidingDropdown = false;
    // Control collors
    CBrush m_editBkBrush;   // used only when cue bunner is shown
    std::optional<COLORREF> m_editBkColor;
    std::optional<COLORREF> m_textColor;
    CBrush m_dropdownBkBrush;
};