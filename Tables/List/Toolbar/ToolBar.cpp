#include <cassert>

#include "ToolBar.h"
#include "../../../DefaultWindowProc.h"

namespace controls {

namespace {
constexpr auto kButtonsGap = 1;
constexpr auto kSelectionChangedTimerId = 0;

constexpr UINT ButtonId(toolbar::ButtonType type)
{
    return static_cast<UINT>(type) + 1;
}

} // namespace

BEGIN_MESSAGE_MAP(CToolbar, CWnd)
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_BN_CLICKED(ButtonId(toolbar::ButtonType::eAddButton),        &CToolbar::OnBnClickedAdd)
    ON_BN_CLICKED(ButtonId(toolbar::ButtonType::eDeleteButton),     &CToolbar::OnBnClickedDelete)
    ON_BN_CLICKED(ButtonId(toolbar::ButtonType::eMoveUpButton),     &CToolbar::OnBnClickedUp)
    ON_BN_CLICKED(ButtonId(toolbar::ButtonType::eMoveDownButton),   &CToolbar::OnBnClickedDown)
END_MESSAGE_MAP()

CToolbar::CToolbar(CListCtrl* connectedList) noexcept
    : m_list(connectedList)
{
    ASSERT(m_list);
}

void CToolbar::InitButtons(const toolbar::ButtonInfos& buttonInfos) noexcept
{
    ENSURE(m_list);

    CRect rect;
    CWnd::GetClientRect(rect);

    const DWORD style = WS_CHILD | (CWnd::GetStyle() & WS_VISIBLE ? WS_VISIBLE : 0);

    size_t index = 0;
    // remove current extra buttons
    for (const auto& info : buttonInfos)
    {
        auto& button = m_buttons[index++];

        if (::IsWindow(button))
            button.DestroyWindow();

        if (!info.visible)
            continue;

        CRect rectButton = rect;
        rectButton.right = rect.left + info.initialWidth;
        button.Create(info.text.c_str(), style, rectButton, this, index);
        button.SetFont(CWnd::GetFont(), FALSE);

        if (info.imageId.has_value())
            button.SetImage(info.imageId.value());

        rect.left = rectButton.right + kButtonsGap;

        // add extra gap before move buttons
        if (index == static_cast<size_t>(toolbar::ButtonType::eMoveUpButton))
            rect.left += 10;

        Layout::AnchorWindow(button, *this, { AnchorSide::eLeft  }, AnchorSide::eRight, info.movingRatio);
        Layout::AnchorWindow(button, *this, { AnchorSide::eRight }, AnchorSide::eRight, info.movingRatio + info.sizingRatio);
    }

    DefaultWindowProc::OnWindowMessage(*m_list, WM_ENABLE, [&](auto, auto, auto, auto)
                                       {
                                           EnsureButtonsEnabled();
                                       }, this);

    DefaultWindowProc::OnWindowMessage(*m_list->GetParent(), WM_NOTIFY,
        [&, listId = m_list->GetDlgCtrlID()](auto, auto, LPARAM lParam, auto)
        {
            const auto* params = reinterpret_cast<LPNMHDR>(lParam);
            if (params->idFrom != static_cast<UINT>(listId))
                return;

            switch (params->code)
            {
            case LVN_ITEMCHANGED:
                {
                    const auto* listViewItem = reinterpret_cast<LPNMLISTVIEW>(lParam);
                    if (listViewItem->uChanged != LVIF_STATE)
                        break;

                    if ((listViewItem->uOldState ^ listViewItem->uNewState) & LVIS_SELECTED)
                    {
                        KillTimer(kSelectionChangedTimerId);
                        SetTimer(kSelectionChangedTimerId, 50, nullptr);
                    }
                }
                break;
            default:
                break;
            }
        }, this);

    EnsureButtonsEnabled();
}

void CToolbar::PreSubclassWindow()
{
    CWnd::PreSubclassWindow();
    CWnd::ModifyStyle(0, WS_CLIPCHILDREN, WS_CLIPCHILDREN);
    CWnd::SetWindowTextW(L"");

    InitButtons();
}

void CToolbar::OnBnClickedAdd()
{
    ENSURE(m_list);
    if (m_addCallback)
    {
        m_addCallback(m_list);
        return;
    }
    const auto selectedLines = ListSelectedItems();
    for (const auto& item : selectedLines)
        m_list->SetItemState(item, 0, LVIS_SELECTED);
    ListItemSelect(m_list->InsertItem(selectedLines.empty() ? m_list->GetItemCount() : (selectedLines.back() + 1), L""));
}

void CToolbar::OnBnClickedDelete()
{
    ENSURE(m_list);

    const std::list<int>& selectedLines = ListSelectedItems();
    for (auto item : selectedLines)
    {
        if (m_removeCallback)
            m_removeCallback(m_list, item);
        m_list->DeleteItem(item);
    }
    if (selectedLines.empty() || m_list->GetItemCount() == 0)
        return;
    ListItemSelect(selectedLines.front() == 0 ? 0 : selectedLines.front() - 1);
}

void CToolbar::OnBnClickedUp()
{
    ENSURE(m_list);

    const std::list<int>& selectedLines = ListSelectedItems();
    auto it = selectedLines.begin(), end = selectedLines.end();
    if (*it == 0)
    {
        // can`t move, try move next lines
        int nNextItem = 0;
        while (nNextItem == *it)
        {
            ++it;
            ++nNextItem;

            if (it == end)
                return;
        }
    }
    for (; it != end; ++it)
    {
        ASSERT(*it > 0);
        ListItemsSwap(*it, *it - 1);
    }
    if (!selectedLines.empty())
        m_list->EnsureVisible(selectedLines.front(), FALSE);
}

void CToolbar::OnBnClickedDown()
{
    ENSURE(m_list);

    const std::list<int>& selectedLines = ListSelectedItems();
    auto it = selectedLines.rbegin(), end = selectedLines.rend();
    const auto itemsCount = m_list->GetItemCount() - 1;
    {
        if (*it == itemsCount)
        {
            // can`t move, try move prev lines
            int nPrevItem = itemsCount;
            while (nPrevItem == *it)
            {
                --nPrevItem;
                ++it;

                if (it == end)
                    return;
            }
        }
    }

    for (; it != end; ++it)
    {
        ASSERT(*it < itemsCount);
        ListItemsSwap(*it, *it + 1);
    }
    if (!selectedLines.empty())
        m_list->EnsureVisible(selectedLines.back(), FALSE);
}

void CToolbar::ListItemSelect(const int index)
{
    ENSURE(m_list);
    m_list->EnsureVisible(index, FALSE);
    m_list->SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

void CToolbar::ListItemsSwap(int source, int destination)
{
    ENSURE(m_list);

    for (int subItem = 0, size = m_list->GetHeaderCtrl()->GetItemCount(); subItem < size; ++subItem)
    {
        auto&& sourceText = m_list->GetItemText(source, subItem);
        auto&& destinationText = m_list->GetItemText(destination, subItem);

        m_list->SetItemText(source, subItem, destinationText);
        m_list->SetItemText(destination, subItem, sourceText);
    }
    const auto sourceState = m_list->GetItemState(source, 0xffff);
    const auto destinationState = m_list->GetItemState(destination, 0xffff);

    m_list->SetItemState(source, destinationState, 0xffff);
    m_list->SetItemState(destination, sourceState, 0xffff);

    const auto sourceData = m_list->GetItemData(source);
    const auto destinationData = m_list->GetItemData(destination);
    m_list->SetItemData(source, destinationData);
    m_list->SetItemData(destination, sourceData);

    if (m_changePositionCallback)
        m_changePositionCallback(m_list, source, destination);
}

std::list<int> CToolbar::ListSelectedItems() const
{
    std::list<int> selectedLines;

    POSITION pos = m_list->GetFirstSelectedItemPosition();
    while (pos)
        selectedLines.emplace_back(m_list->GetNextSelectedItem(pos));
    return selectedLines;
}

void CToolbar::EnsureButtonsEnabled()
{
    const auto controlEnabled = m_list->IsWindowEnabled();

    const auto addButtonInd = static_cast<size_t>(toolbar::ButtonType::eAddButton);
    auto& addButton = m_buttons[addButtonInd];
    if (::IsWindow(addButton))
        addButton.EnableWindow(controlEnabled);

    const auto selectionExist = controlEnabled && (m_list->GetSelectedCount() != 0);
    for (auto& button : m_buttons)
    {
        if (button != addButton && ::IsWindow(button))
            button.EnableWindow(selectionExist);
    }
}

void CToolbar::OnDestroy()
{
    CWnd::OnDestroy();

    for (auto& button : m_buttons)
    {
        if (::IsWindow(button))
            button.DestroyWindow();
    }
}

void CToolbar::OnTimer(UINT_PTR nIDEvent)
{
    ASSERT(nIDEvent == kSelectionChangedTimerId);
    KillTimer(nIDEvent);

    EnsureButtonsEnabled();

    CWnd::OnTimer(nIDEvent);
}

} // namespace controls
