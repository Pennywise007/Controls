#include <assert.h>
#include <regex>
#include <algorithm>

#include "ComboWithSearch.h"

////////////////////////////////////////////////////////////////////////////////
// константы
// идентификатор таймера подбора
constexpr UINT_PTR kTextChangeTimerId = 0;

// время через которое должен сработать таймер подбора, мс
constexpr UINT kSearchTime = 200;

BEGIN_MESSAGE_MAP(ComboWithSearch, CComboBox)
    ON_CONTROL_REFLECT_EX(CBN_SELENDOK,        &ComboWithSearch::OnCbnSelendok)
    ON_CONTROL_REFLECT_EX(CBN_SELENDCANCEL,    &ComboWithSearch::OnCbnSelendcancel)
    ON_CONTROL_REFLECT_EX(CBN_EDITCHANGE,      &ComboWithSearch::OnCbnEditchange)
    ON_WM_CTLCOLOR()
    ON_WM_TIMER()
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
ComboWithSearch::ComboWithSearch()
{
    m_editBkBrush.CreateSolidBrush(RGB(255, 255, 255));
    m_dropdownBkBrush.CreateSolidBrush(RGB(255, 255, 255));
}

//----------------------------------------------------------------------------//
void ComboWithSearch::SetWindowText(const CString& text)
{
    int item = FindStringExact(0, text);
    BaseCtrl::SetCurSel(item);
    BaseCtrl::SetWindowText(text);
}

//----------------------------------------------------------------------------//
void ComboWithSearch::AdjustComboBoxToContent()
{
    if (!BaseCtrl::GetSafeHwnd() || !::IsWindow(BaseCtrl::GetSafeHwnd()))
    {
        ASSERT(FALSE);
        return;
    }

    // Make sure the drop rect for this combobox is at least tall enough to
    // show 3 items in the dropdown list.
    int nHeight = 0;
    int nItemsToShow = std::min<int>(GetMinVisible(), BaseCtrl::GetCount());

    for (int i = 0; i < nItemsToShow; i++)
    {
        int nItemH = BaseCtrl::GetItemHeight(i);
        nHeight += nItemH;
    }

    nHeight += (4 * ::GetSystemMetrics(SM_CYEDGE));

    // Set the height if necessary -- save current size first
    COMBOBOXINFO cmbxInfo;
    cmbxInfo.cbSize = sizeof(COMBOBOXINFO);
    if (BaseCtrl::GetComboBoxInfo(&cmbxInfo))
    {
        CRect rcListBox;
        ::GetWindowRect(cmbxInfo.hwndList, &rcListBox);

        if (rcListBox.Height() != nHeight)
            ::SetWindowPos(cmbxInfo.hwndList, 0, 0, 0, rcListBox.Width(),
                           nHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
    }
}

//----------------------------------------------------------------------------//
HBRUSH ComboWithSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (m_editBkColor.has_value())
        pDC->SetBkColor(m_editBkColor.value());
    if (m_textColor.has_value())
        pDC->SetTextColor(m_textColor.value());

    auto* currentFocus = GetFocus();
    if (currentFocus != this && currentFocus != GetWindow(GW_CHILD))
        // If window is not focused for drowing edit background will be used brush instead of pDC
        return m_editBkBrush;

    return m_dropdownBkBrush;
}
//----------------------------------------------------------------------------//
void ComboWithSearch::AllowCustomText(bool allow)
{
    m_allowCustomText = allow;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::SetBkColor(std::optional<COLORREF> color)
{
    m_editBkBrush.DeleteObject();
    m_editBkBrush.CreateSolidBrush(color.value_or(RGB(255, 255, 255)));

    m_editBkColor = std::move(color);
    Invalidate();
}

//----------------------------------------------------------------------------//
void ComboWithSearch::SetTextColor(std::optional<COLORREF> color)
{
    m_textColor = std::move(color);
    Invalidate();
}

//----------------------------------------------------------------------------//
void ComboWithSearch::SetDropdownBkColor(std::optional<COLORREF> color)
{
    m_dropdownBkBrush.DeleteObject();
    m_dropdownBkBrush.CreateSolidBrush(color.value_or(RGB(255, 255, 255)));
    Invalidate();
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnSelendok()
{
    if (!m_allowCustomText)
    {
        // если нет выбранной строки, но есть строки в списке
        // ставим выделение на первую существующую
        if (BaseCtrl::GetCurSel() == -1 && BaseCtrl::GetCount() > 0)
            BaseCtrl::SetCurSel(0);

        if (BaseCtrl::GetCurSel() != -1)
            updateRealCurSelFromCurrent();
    }

    SetRedraw(FALSE);
    // Hide dropdown to don`t show user if it is resized during filling items in resetSearch function
    m_internalHidingDropdown = true;
    if (BaseCtrl::GetDroppedState())
        BaseCtrl::ShowDropDown(FALSE);
    m_internalHidingDropdown = false;
    // Returing filtered lines to control
    resetSearch();
    SetRedraw(TRUE);

    if (!m_allowCustomText)
    {
        // восстанавливаем предыдущий выделенный элемент, делаем принудительно
        // потому что в поле может быть текст не соответствующий выделению(пустой например)
        if (BaseCtrl::GetCurSel() == -1)
            BaseCtrl::SetCurSel(m_selectedItemIndex != -1 ? m_selectedItemIndex : 0);
    }
    Invalidate();

    return FALSE;
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnSelendcancel()
{
    // When we call ShowDropDown(FALSE) internally control sends OnCbnSelendcancel which we ignore
    if (m_internalHidingDropdown)
        return TRUE;

    CString customText;
    DWORD customSelectionStart, customSelectionEnd;
    if (m_allowCustomText)
    {
        // user might click on control or press esc to stop editing/hide drop down
        GetWindowText(customText);
        DWORD selection = BaseCtrl::GetEditSel();
        customSelectionStart = LOWORD(selection);
        customSelectionEnd = HIWORD(selection);
    }

    SetRedraw(FALSE);
    // Hide dropdown to don`t show user if it is resized during filling items in resetSearch function
    m_internalHidingDropdown = true;
    if (BaseCtrl::GetDroppedState())
        BaseCtrl::ShowDropDown(FALSE);
    m_internalHidingDropdown = false;
    // Returing filtered lines to control
    resetSearch();
    SetRedraw(TRUE);

    if (m_allowCustomText)
    {
        BaseCtrl::SetWindowText(customText);
        BaseCtrl::SetEditSel(customSelectionStart, customSelectionEnd);
    }
    else
    {
        // восстанавливаем предыдущий выделенный элемент, делаем принудительно
        // потому что в поле может быть текст не соответствующий выделению(пустой например)
        if (m_selectedItemIndex != -1)
            BaseCtrl::SetCurSel(m_selectedItemIndex);
    }
    Invalidate();

    return FALSE;
}

//----------------------------------------------------------------------------//
LRESULT ComboWithSearch::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // ignore when we execute search
    if (m_filteredItems.empty())
    {
        switch (message)
        {
        case CB_SETCURSEL:
            {
                auto res = BaseCtrl::WindowProc(message, wParam, lParam);
                updateRealCurSelFromCurrent();
                return res;
            }
        }
    }

    return BaseCtrl::WindowProc(message, wParam, lParam);
}

//----------------------------------------------------------------------------//
BOOL ComboWithSearch::OnCbnEditchange()
{
    BaseCtrl::KillTimer(kTextChangeTimerId);
    BaseCtrl::SetTimer(kTextChangeTimerId, kSearchTime, nullptr);

    return FALSE;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::executeSearch()
{
    ::SetCursor(LoadCursor(NULL, IDC_WAIT));

    // запоминаем теущее состояние ыпадающего списка
    bool dropShow = !!BaseCtrl::GetDroppedState();

    // получаем текущий текст
    CString curWindowText;
    BaseCtrl::GetWindowText(curWindowText);

    bool bChangeInStrings = !curWindowText.IsEmpty();
    {
        if (!curWindowText.IsEmpty())
        {
            try
            {
                // экранируем спец символы регекса
                const std::wregex re_regexEscape(_T("[.^$|()\\[\\]{}+?\\\\]"));
                const std::wstring rep(_T("\\\\&"));
                CString regexStr = std::regex_replace(curWindowText.GetString(),
                                                      re_regexEscape,
                                                      rep,
                                                      std::regex_constants::format_sed |
                                                      std::regex_constants::match_default).c_str();
                // формируем строку поиска для регекса
                regexStr.Replace(L"*", L".*");
                regexStr.Replace(L" ", L".*");

                const std::wregex xRegEx(regexStr, std::regex::icase);

                int existElementsIndex = 0;
                auto currentFilteredElementIt = m_filteredItems.begin();
                // проходим по всем строкам и применяем к ним фильтр
                for (int i = 0, count = BaseCtrl::GetCount() + (int)m_filteredItems.size(); i < count; ++i)
                {
                    std::wsmatch xResults;

                    // если текущая строка находится в списке удаленных элементов
                    if (currentFilteredElementIt != m_filteredItems.end() &&
                        i == currentFilteredElementIt->first)
                    {
                        // проверяем надо ли оставить эту строку
                        // если с новым фильтром строка удовлетворяет поиску - восстанавливем её
                        if (std::regex_search(currentFilteredElementIt->second,
                            xResults, xRegEx))
                        {
                            // возвращаем строку в контрол
                            BaseCtrl::InsertString(existElementsIndex,
                                                   currentFilteredElementIt->second.c_str());
                            bChangeInStrings = true;
                            // удаляем ее из списка отфильтрованных
                            currentFilteredElementIt = m_filteredItems.erase(currentFilteredElementIt);

                            existElementsIndex++;
                        }
                        else
                            // переходим к след элементу
                            ++currentFilteredElementIt;
                    }
                    else
                    {
                        // проверяем строку из комбобокса
                        std::wstring str;
                        str.resize(BaseCtrl::GetLBTextLen(existElementsIndex) + 1);
                        BaseCtrl::GetLBText(existElementsIndex,
                                            const_cast<WCHAR*>(str.c_str()));

                        // если она не удовлетворяет фильтру выкидываем её
                        if (!std::regex_search(str, xResults, xRegEx))
                        {
                            // добавляем в список удалённых
                            assert(m_filteredItems.find(i) == m_filteredItems.end());
                            m_filteredItems[i] = str;

                            // убираем отфильтрованную строку
                            BaseCtrl::DeleteString(existElementsIndex);

                            bChangeInStrings = true;
                        }
                        else
                            ++existElementsIndex;
                    }
                }
            }
            catch (const std::regex_error& err)
            {
                ::MessageBox(0, CString(err.what()),
                             L"Возникло исключение!", MB_ICONERROR | MB_OK);
            }
        }
        else
            bChangeInStrings |= resetSearch();
    }

    if (!dropShow)
    {
        DWORD curSel = BaseCtrl::GetEditSel();
        SetRedraw(FALSE);

        // If drop list was closed we will open it just to reseive OnCbnSelendok on VK_ENTER
        BaseCtrl::ShowDropDown(true);

        if (BaseCtrl::GetCount() == 0)
            AdjustComboBoxToContent();

        // After showing the drop down it replaces edit text with first string from the dropdown
        BaseCtrl::SetWindowText(curWindowText);
        BaseCtrl::SetEditSel(LOWORD(curSel), HIWORD(curSel));

        SetRedraw(TRUE);
        Invalidate();
    }
    else
    {
        // если были изменения - надо пересчитать высоту выпадающего списка
        if (bChangeInStrings)
            AdjustComboBoxToContent();
    }

    ::SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//----------------------------------------------------------------------------//
void ComboWithSearch::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kTextChangeTimerId)
    {
        KillTimer(kTextChangeTimerId);
        executeSearch();
    }

    BaseCtrl::OnTimer(nIDEvent);
}

//----------------------------------------------------------------------------//
bool ComboWithSearch::resetSearch()
{
    if (!m_filteredItems.empty())
    {
        // убираем фильтр, возвращаем отфильтрованные строки в контрол
        for (auto &it : m_filteredItems)
        {
            BaseCtrl::InsertString(it.first, it.second.c_str());
        }
        m_filteredItems.clear();

        return true;
    }
    else
        return false;
}

//----------------------------------------------------------------------------//
void ComboWithSearch::updateRealCurSelFromCurrent()
{
    auto currentSel = BaseCtrl::GetCurSel();
    // получаем текущее значение выделения
    m_selectedItemIndex = convertToRealIndex(currentSel);
}

//----------------------------------------------------------------------------//
int ComboWithSearch::convertToRealIndex(int index)
{
    if (index != -1 && !m_filteredItems.empty())
    {
        // корректируем индекс с учетом удаленных строк
        // будем увеличивать на каждую удаленную строку до искомого индекса
        for (const auto &it : m_filteredItems)
        {
            if (it.first <= index)
                ++index;
            else
                break;
        }
    }

    return index;
}

//----------------------------------------------------------------------------//
int ComboWithSearch::convertFromRealIndex(int index)
{
    if (index != -1 && !m_filteredItems.empty())
    {
        int currentElementsIndex = index;
        // ищем нде находится индекс с учетом удаленных строк
        // будем уменьшать на каждую удаленную строку до искомого индекса
        for (const auto &it : m_filteredItems)
        {
            if (it.first <= index)
                --currentElementsIndex;
            else
                break;
        }

        return currentElementsIndex;
    }

    return index;
}
