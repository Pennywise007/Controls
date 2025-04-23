﻿#pragma once

#include <afxtoolbarimages.h>
#include <bitset>
#include <cassert>

#include "SubItemsControls.h"

#include "Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h"

namespace controls::list::widgets {

template <typename CBaseList>
struct SubItemsControls<CBaseList>::Control
{
    explicit Control(int iItem, int iSubItem, HWND hWnd, LPCWSTR themeClassList) noexcept
        : m_iItem(iItem)
        , m_iSubItem(iSubItem)
    {
        VERIFY(hWnd && themeClassList);
        m_hTheme = ::OpenThemeData(hWnd, themeClassList);
        VERIFY(!!m_hTheme);
    }
    explicit Control(int iItem, int iSubItem) noexcept
        : m_iItem(iItem)
        , m_iSubItem(iSubItem)
    {}

    virtual ~Control() noexcept
    {
        if (m_hTheme)
            VERIFY(SUCCEEDED(::CloseThemeData(m_hTheme)));
    }

    void OnMouseHover() noexcept { ASSERT(!m_state.test(State::eHovered)); m_state.set(State::eHovered); }
    void OnMouseLeave()
    {
        ASSERT(m_state.test(State::eHovered));
        m_state.set(State::eHovered, false);
    }
    void OnLButtonDown() noexcept { ASSERT(!m_state.test(State::eLButtonDown)); m_state.set(State::eLButtonDown); }
    /// <summary>Notify about button up </summary>
    /// <returns>Item index</returns>
    virtual void OnLButtonUp(int iItem, int iSubItem, CallbacksHolder& callbacks) noexcept
    {
        ASSERT(m_state.test(State::eLButtonDown));
        m_state.set(State::eLButtonDown, false);
    }
    virtual void Draw(LPNMLVCUSTOMDRAW pNMCD, const CString& text) noexcept = 0;

    int m_iItem;
    int m_iSubItem;
protected:
    HTHEME m_hTheme = nullptr;

    enum State : size_t
    {
        eHovered,
        eLButtonDown,

        Count
    };
    std::bitset<State::Count> m_state;
};

template <typename CBaseList>
struct SubItemsControls<CBaseList>::ICheckbox : Control
{
    using Control::Control;

    virtual void SetState(bool state) = 0;
    [[nodiscard]] virtual bool GetState() const = 0;
};

BEGIN_TEMPLATE_MESSAGE_MAP(SubItemsControls, CBaseList, CBaseList)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT_EX(NM_CUSTOMDRAW, &SubItemsControls::OnNMCustomdraw)
END_MESSAGE_MAP()

template<typename CBaseList>
inline SubItemsControls<CBaseList>::SubItemsControls()
{
    m_controls.OnNewPos([](std::shared_ptr<Control>& control, int newItemIndex, int newSubItemIndex)
                        {
                            control->m_iItem = newItemIndex;
                            control->m_iSubItem = newSubItemIndex;
                        });
}

template <typename CBaseList>
void SubItemsControls<CBaseList>::
SetButton(int index, int iSubItem)
{
    struct ButtonControl final : Control
    {
        explicit ButtonControl(HWND hWnd, int iItem, int iSubItem) noexcept
            : Control(iItem, iSubItem, hWnd, L"BUTTON")
        {}

    private: // Control
        void Draw(LPNMLVCUSTOMDRAW pNMCD, const CString& text) noexcept override
        {
            const BUTTONPARTS buttonType = BP_PUSHBUTTON;
            PUSHBUTTONSTATES buttonState = PBS_NORMAL;

            if (pNMCD->nmcd.uItemState & ODS_DISABLED)
                buttonState = PBS_DISABLED;
            else if (Control::m_state.test(Control::State::eLButtonDown))
                buttonState = PBS_PRESSED;
            else if (pNMCD->nmcd.uItemState & ODS_HOTLIGHT ||
                     Control::m_state.test(Control::State::eHovered))
                buttonState = PBS_HOT;

            // Рисуем фон хидера и получаем внутренний размер
            VERIFY(SUCCEEDED(::DrawThemeBackground(Control::m_hTheme, pNMCD->nmcd.hdc, buttonType,
                                                   buttonState, &pNMCD->nmcd.rc, 0)));
            VERIFY(SUCCEEDED(::DrawThemeText(Control::m_hTheme, pNMCD->nmcd.hdc, buttonType,
                                             buttonState, text.GetString(), text.GetLength(),
                                             DT_CENTER | DT_VCENTER, 0, &pNMCD->nmcd.rc)));
        }
        void OnLButtonUp(int iItem, int iSubItem, CallbacksHolder& callbacks) noexcept override
        {
            if (iItem == Control::m_iItem && iSubItem == Control::m_iSubItem)
            {
                if (callbacks.buttonPressedCallback)
                    callbacks.buttonPressedCallback(Control::m_iItem, Control::m_iSubItem);
                else
                    ASSERT(false);
            }
            Control::OnLButtonUp(iItem, iSubItem, callbacks);
        }
    };

    m_controls.Add(index, iSubItem, std::make_shared<ButtonControl>(CBaseList::m_hWnd, index, iSubItem));
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
SetCheckbox(int index, int iSubItem, bool enabled)
{
    struct CheckboxControl final : ICheckbox
    {
        explicit CheckboxControl(HWND hWnd, int iItem, int iSubItem, bool state) noexcept
            : ICheckbox(iItem, iSubItem, hWnd, L"BUTTON")
            , m_state(state) {}

    private: // Control
        void Draw(LPNMLVCUSTOMDRAW pNMCD, const CString& /*text*/) noexcept override
        {
            const BUTTONPARTS buttonType = BP_CHECKBOX;
            CHECKBOXSTATES buttonState = m_state ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;

            if (pNMCD->nmcd.uItemState & ODS_DISABLED)
                buttonState = m_state ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
            else if (Control::m_state.test(Control::State::eLButtonDown))
                buttonState = m_state ? CBS_CHECKEDPRESSED : CBS_UNCHECKEDPRESSED;
            else if (pNMCD->nmcd.uItemState & ODS_HOTLIGHT ||
                     Control::m_state.test(Control::State::eHovered))
                buttonState = m_state ? CBS_CHECKEDHOT : CBS_UNCHECKEDHOT;

            auto rect = pNMCD->nmcd.rc;
            --rect.bottom; // fix for checkbox size
            VERIFY(SUCCEEDED(::DrawThemeBackground(Control::m_hTheme, pNMCD->nmcd.hdc, buttonType,
                                                   buttonState, &rect, 0)));
        }
        void OnLButtonUp(int iItem, int iSubItem, CallbacksHolder& callbacks) noexcept override
        {
            if (iItem == Control::m_iItem && iSubItem == Control::m_iSubItem)
            {
                m_state = !m_state;
                if (callbacks.checkboxStateChangedCallback)
                    callbacks.checkboxStateChangedCallback(Control::m_iItem, Control::m_iSubItem, m_state);
                else
                    ASSERT(false);
            }
            Control::OnLButtonUp(iItem, iSubItem, callbacks);
        }
    private: // ICheckbox
        void SetState(bool state) override { m_state = state; }
        [[nodiscard]] bool GetState() const override { return m_state; }

    private:
        bool m_state;
    };

    if (enabled)
    {
        const auto it = m_checkboxColumns.find(iSubItem);
        ENSURE(it != m_checkboxColumns.end());
        ++it->second;
    }
    m_controls.Add(index, iSubItem, std::make_shared<CheckboxControl>(CBaseList::m_hWnd, index, iSubItem, enabled));
    CBaseList::SetItemText(index, iSubItem, enabled ? L"1" : L"0");  // for sorting
    CheckCheckboxColumnState(iSubItem);
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
SetImage(int index, int iSubItem, CBitmap& hBitmap, LONG imageWidth, LONG imageHeight)
{
    struct ImageControl final : Control
    {
        explicit ImageControl(int iItem, int iSubItem, CBitmap& hBitmap,
                              LONG imageWidth = kStratched, LONG imageHeight = kStratched) noexcept
            : Control(iItem, iSubItem)
            , m_imageSize(imageWidth, imageHeight)
        {
            m_image.AddImage((HBITMAP)hBitmap.Detach(), TRUE);
        }

    private: // Control
        void Draw(LPNMLVCUSTOMDRAW pNMCD, const CString& /*text*/) noexcept override
        {
            LISTITEMSTATES state = LISTITEMSTATES::LISS_NORMAL;

            if (pNMCD->nmcd.uItemState & ODS_DISABLED)
                state = LISTITEMSTATES::LISS_DISABLED;
            else if (Control::m_state.test(Control::State::eLButtonDown))
                state = LISTITEMSTATES::LISS_HOTSELECTED;
            else if (pNMCD->nmcd.uItemState & ODS_HOTLIGHT ||
                     Control::m_state.test(Control::State::eHovered))
                state = LISTITEMSTATES::LISS_HOT;

            auto* pDC = CDC::FromHandle(pNMCD->nmcd.hdc);
            pDC->FillSolidRect(&pNMCD->nmcd.rc, pDC->GetBkColor());

            const CRect drawRect(pNMCD->nmcd.rc);
            const CSize imageSize(m_imageSize.cx != kStratched ? m_imageSize.cx : drawRect.Width(),
                                  m_imageSize.cy != kStratched ? m_imageSize.cy : drawRect.Height());
            const CPoint drawPoint = imageSize == drawRect.Size() ?
                                     drawRect.TopLeft() :
                                     (drawRect.CenterPoint() - CPoint(m_imageSize.cx / 2, m_imageSize.cy / 2));

            CAfxDrawState ds;
            m_image.PrepareDrawImage(ds, imageSize);
            m_image.Draw(pDC, drawPoint.x, drawPoint.y, 0, FALSE, pNMCD->nmcd.uItemState & ODS_DISABLED);
            m_image.EndDrawImage(ds);
        }
        void OnLButtonUp(int iItem, int iSubItem, CallbacksHolder& callbacks) noexcept override
        {
            if (iItem == Control::m_iItem && iSubItem == Control::m_iSubItem)
            {
                if (callbacks.buttonPressedCallback)
                    callbacks.buttonPressedCallback(Control::m_iItem, Control::m_iSubItem);
                else
                    ASSERT(false);
            }
            Control::OnLButtonUp(iItem, iSubItem, callbacks);
        }

    private:
        CSize m_imageSize;
        CMFCToolBarImages m_image;
    };
    m_controls.Add(index, iSubItem, std::make_shared<ImageControl>(index, iSubItem, hBitmap,
                                                                   imageWidth, imageHeight));
}

template <typename CBaseList>
void SubItemsControls<CBaseList>::SetCheckboxColumn(int iColumnIndex)
{
    CBaseList::ModifyExtendedStyle(0, LVS_EX_CHECKBOXES);
    // fix checkboxes lag during control resize
    CBaseList::ModifyStyle(0, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

    CHeaderCtrl* header = CBaseList::GetHeaderCtrl();
    HDITEM hdi = { 0 };
    hdi.mask = HDI_FORMAT;
    Header_GetItem(*header, iColumnIndex, &hdi);
    if (!(hdi.fmt & HDF_CHECKBOX))
    {
        hdi.fmt |= HDF_CHECKBOX;
        Header_SetItem(*header, iColumnIndex, &hdi);
    }

    for (int i = 0, count = CBaseList::GetItemCount(); i < count; ++i)
    {
        SetCheckbox(i, iColumnIndex, false);
    }
    VERIFY(m_checkboxColumns.try_emplace(iColumnIndex, 0).second);
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
RemoveControl(int index, int iSubItem)
{
    const auto lineIt = m_controls.find(index);
    if (lineIt == m_controls.end())
        return;
    const auto columnIt = lineIt->second->find(iSubItem);
    if (columnIt == lineIt->second.end())
        return;
    const bool checkBox = !!std::dynamic_pointer_cast<ICheckbox>(columnIt->second);
    lineIt->erase(columnIt);
    if (lineIt->empty())
        m_controls.erase(lineIt);

    if (checkBox)
        CBaseList::SetItemText(index, iSubItem, L"");
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
SetButtonPressedCallback(const SubItemsControls::OnButtonPressed& onPress)
{
    m_callbacksHolder.buttonPressedCallback = onPress;
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
SetCheckboxChangedCallback(const SubItemsControls::OnCheckboxStateChanged& onChanged)
{
    m_callbacksHolder.checkboxStateChangedCallback = onChanged;
}

template<typename CBaseList>
std::pair<int, int> SubItemsControls<CBaseList>::
GetSubItemUnderPoint(const CPoint& point)
{
    // CMT: Select the item the user clicked on.
    UINT uFlags;
    const auto currentItem = CBaseList::HitTest(point, &uFlags);
    int currentSubItem = -1;
    // CMT: If the click has been made on some valid item
    if (-1 != currentItem && uFlags & LVHT_ONITEMLABEL)
    {
        LVHITTESTINFO hti;
        hti.pt = point;
        hti.flags = LVM_SUBITEMHITTEST;
        CBaseList::SubItemHitTest(&hti);

        currentSubItem = hti.iSubItem;
    }

    return std::make_pair(currentItem, currentSubItem);
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::CheckCheckboxColumnState(int column)
{
    if (const auto it = m_checkboxColumns.find(column); it != m_checkboxColumns.end())
    {
        const bool allChecked = it->second == CBaseList::GetItemCount();

        CHeaderCtrl* header = CBaseList::GetHeaderCtrl();
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT;
        Header_GetItem(*header, column, &hdi);
        const auto prevFormat = hdi.fmt;
        if (allChecked)
            hdi.fmt |= HDF_CHECKED;
        else
            hdi.fmt &= ~HDF_CHECKED;
        if (prevFormat != hdi.fmt)
            Header_SetItem(*header, column, &hdi);
    }
    else
        ASSERT(false);
}

template<typename CBaseList>
inline int SubItemsControls<CBaseList>::GetRealItemIndex(int item) const
{
    if constexpr (std::is_base_of_v<CListGroupCtrl, CBaseList>)
    {
        return CListGroupCtrl::GetDefaultItemIndex(item);
    }
    else
    {
        return CBaseList::GetItemData(item);
    }
}

template<typename CBaseList>
inline void SubItemsControls<CBaseList>::OnLButtonPress(const CPoint& point)
{
    const auto&& [iItem, iSubItem] = GetSubItemUnderPoint(point);
    if (iItem == -1)
        return;

    const auto* control = m_controls.Get(GetRealItemIndex(iItem), iSubItem);
    if (!control)
        return;

    CBaseList::SetCapture();
    m_buttonDownControl = *control;
    m_buttonDownControl->OnLButtonDown();

    if (!(GetKeyState(VK_SHIFT) & 0x8000))
    {
        // remove current selection
        if (const UINT selCount = CBaseList::GetSelectedCount(); selCount != 0)
        {
            POSITION pos = CBaseList::GetFirstSelectedItemPosition();
            for (UINT index = 0; index < selCount; ++index)
            {
                CBaseList::SetItemState(CBaseList::GetNextSelectedItem(pos), 0, LVIS_SELECTED);
            }
        }
    }

    CBaseList::SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
    CBaseList::EnsureVisible(iItem, TRUE);
}

template<typename CBaseList>
LRESULT SubItemsControls<CBaseList>::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    auto processControls = [&]()
    {
        m_controls.ProcessWindowProc(message, wParam, lParam, [&](int i) { return GetRealItemIndex(i); });
    };

    switch (message)
    {
    case LVM_INSERTITEM:
        {
            const auto* pItem = reinterpret_cast<const LVITEM*>(lParam);

            for (auto&& [subItem, count] : m_checkboxColumns)
            {
                SetCheckbox(pItem->iItem, subItem, false);
            }

            const auto res = CBaseList::WindowProc(message, wParam, lParam);
            if constexpr (!std::is_base_of_v<CListGroupCtrl, CBaseList>)
            {
                CBaseList::SetItemData(pItem->iItem, pItem->iItem);
            }
            processControls();
            return res;
        }
    case LVM_DELETEITEM:
        {
            const int iItem = GetRealItemIndex(wParam);
            for (auto&& [subItem, count] : m_checkboxColumns)
            {
                const auto* control = m_controls.Get(iItem, subItem);
                VERIFY(!!control);
                const auto checkbox = std::dynamic_pointer_cast<ICheckbox>(*control);
                if (!!checkbox && checkbox->GetState())
                {
                    --count;
                }
                CheckCheckboxColumnState(subItem);
            }

            if (m_buttonDownControl && m_buttonDownControl->m_iItem == iItem)
                m_buttonDownControl = nullptr;
        }
        break;
    case LVM_DELETEALLITEMS:
        {
            m_hoveredSubItem.reset();
            m_buttonDownControl.reset();

            for (auto&& [subItem, count] : m_checkboxColumns)
            {
                count = 0;
            }
        }
        break;
    case LVM_INSERTCOLUMN:
        {
            const int iSubItem = wParam;

            if (!m_checkboxColumns.empty())
            {
                HDITEM hdi = { 0 };
                hdi.mask = HDI_FORMAT;
                CHeaderCtrl* header = CBaseList::GetHeaderCtrl();
                for (auto checkBoxColumn = std::prev(m_checkboxColumns.end());
                     checkBoxColumn != m_checkboxColumns.end() && checkBoxColumn->first >= iSubItem;
                     --checkBoxColumn)
                {
                    m_checkboxColumns[checkBoxColumn->first + 1] = checkBoxColumn->second;

                    Header_GetItem(*header, checkBoxColumn->first, &hdi);
                    hdi.fmt &= ~HDF_CHECKBOX;
                    Header_SetItem(*header, checkBoxColumn->first, &hdi);

                    Header_GetItem(*header, checkBoxColumn->first + 1, &hdi);
                    hdi.fmt |= HDF_CHECKBOX;
                    Header_SetItem(*header, checkBoxColumn->first + 1, &hdi);

                    checkBoxColumn = m_checkboxColumns.erase(checkBoxColumn);
                }
            }
        }
        break;
    case LVM_DELETECOLUMN:
        {
            const int iSubItem = wParam;
            if (m_buttonDownControl && m_buttonDownControl->m_iSubItem == iSubItem)
                m_buttonDownControl = nullptr;
            if (const auto it = m_checkboxColumns.find(iSubItem); it != m_checkboxColumns.end())
                m_checkboxColumns.erase(it);
        }
        break;
    case WM_NOTIFY:
        {
            const auto* pNMHDR = reinterpret_cast<NMHDR*>(lParam);

            if (pNMHDR->code == HDN_ITEMSTATEICONCLICK)
            {
                const auto* pNMHeader = reinterpret_cast<LPNMHEADER>(lParam);
                if (pNMHeader->pitem->mask & HDI_FORMAT && pNMHeader->pitem->fmt & HDF_CHECKBOX)
                {
                    const auto checkBoxColumn = m_checkboxColumns.find(pNMHeader->iItem);
                    if (checkBoxColumn != m_checkboxColumns.end())
                    {
                        const bool bChecked = (pNMHeader->pitem->fmt & HDF_CHECKED) == FALSE;
                        for (int item = CBaseList::GetItemCount() - 1; item >= 0; --item)
                        {
                            const auto index = GetRealItemIndex(item);
                            const auto* control = m_controls.Get(index, pNMHeader->iItem);
                            if (!!control)
                            {
                                auto checkbox = std::dynamic_pointer_cast<ICheckbox>(*control);
                                VERIFY(!!checkbox);
                                checkbox->SetState(bChecked);
                                CBaseList::SetItemText(item, pNMHeader->iItem, bChecked ? L"1" : L"0");  // for sorting
                            }
                            else
                                ASSERT(false);
                        }

                        checkBoxColumn->second = bChecked ? CBaseList::GetItemCount() : 0;
                        CBaseList::RedrawWindow();
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    processControls();
    return CBaseList::WindowProc(message, wParam, lParam);
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::OnMouseMove(UINT nFlags, CPoint point)
{
    CBaseList::OnMouseMove(nFlags, point);

    const auto removeHover = [&]()
    {
        if (!m_hoveredSubItem.has_value())
            return;

        const auto* control = m_controls.Get(GetRealItemIndex(m_hoveredSubItem.value().first),
                                             m_hoveredSubItem.value().second);
        if (!control)
        {
            m_hoveredSubItem.reset();
            return;
        }

        const int itemIndex = m_hoveredSubItem.value().first;
        control->get()->OnMouseLeave();
        CBaseList::RedrawItems(itemIndex, itemIndex);
        m_hoveredSubItem.reset();
    };

    auto itemPositions = GetSubItemUnderPoint(point);
    if (itemPositions.second != -1)
    {
        ASSERT(itemPositions.first != -1);
        if (!m_hoveredSubItem.has_value() || itemPositions != m_hoveredSubItem.value())
        {
            removeHover();
            m_hoveredSubItem = std::move(itemPositions);

            const auto* control = m_controls.Get(GetRealItemIndex(m_hoveredSubItem.value().first),
                                                 m_hoveredSubItem.value().second);
            if (!control)
                return;

            control->get()->OnMouseHover();
            CBaseList::RedrawItems(m_hoveredSubItem.value().first, m_hoveredSubItem.value().first);
        }
    }
    else if (m_hoveredSubItem.has_value())
        removeHover();
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::OnMouseLeave()
{
    CBaseList::OnMouseLeave();

    if (!m_hoveredSubItem.has_value())
        return;
    const auto* control = m_controls.Get(GetRealItemIndex(m_hoveredSubItem.value().first),
                                         m_hoveredSubItem.value().second);
    if (!control)
    {
        m_hoveredSubItem.reset();
        return;
    }

    control->get()->OnMouseLeave();
    const auto controlIndex = m_hoveredSubItem.value().second;
    CBaseList::RedrawItems(controlIndex, controlIndex);
    m_hoveredSubItem.reset();
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
OnLButtonDown(UINT nFlags, CPoint point)
{
    ASSERT(!m_buttonDownControl);
    OnLButtonPress(point);

    if (!m_buttonDownControl)
        CBaseList::OnLButtonDown(nFlags, point);
}

template<typename CBaseList>
void SubItemsControls<CBaseList>::
OnLButtonUp(UINT nFlags, CPoint point)
{
    if (!m_buttonDownControl)
        return CBaseList::OnLButtonUp(nFlags, point);

    const auto&& [iItem, iSubItem] = GetSubItemUnderPoint(point);

    VERIFY(::ReleaseCapture());
    ASSERT(m_buttonDownControl);

    const std::shared_ptr<ICheckbox> checkbox = std::dynamic_pointer_cast<ICheckbox>(m_buttonDownControl);
    const auto prevState = checkbox && checkbox->GetState();

    m_buttonDownControl->OnLButtonUp(iItem != -1 ? GetRealItemIndex(iItem) : -1 , iSubItem, m_callbacksHolder);
    const int itemIndex = m_buttonDownControl->m_iItem;
    m_buttonDownControl.reset();

    if (const auto it = m_checkboxColumns.find(iSubItem); it != m_checkboxColumns.end())
    {
        if (prevState != (checkbox && checkbox->GetState()))
        {
            if (prevState)
            {
                ASSERT(it->second > 0);
                --it->second;
            }
            else
                ++it->second;

            checkbox->SetState(!prevState);
            CBaseList::SetItemText(itemIndex, iSubItem, prevState ? L"1" : L"0");  // for sorting

            CheckCheckboxColumnState(iSubItem);
        }
    }

    CBaseList::RedrawItems(itemIndex, itemIndex);
}

template<typename CBaseList>
inline void SubItemsControls<CBaseList>::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    auto&& [iItem, iSubItem] = GetSubItemUnderPoint(point);
    iItem = GetRealItemIndex(iItem);
    if (!!m_controls.Get(iItem, iSubItem))
    {
        // When we dbl click on checkbox we want to switch 2 times.
        ASSERT(!m_buttonDownControl);
        OnLButtonPress(point);
        return;
    }

    CBaseList::OnLButtonDblClk(nFlags, point);
}

template<typename CBaseList>
inline BOOL SubItemsControls<CBaseList>::
OnChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            auto* pNMHDR = reinterpret_cast<NMHDR*>(lParam);

            switch (pNMHDR->code)
            {
            case NM_CUSTOMDRAW:
                if (OnNMCustomdraw(pNMHDR, pResult) == TRUE)
                    return TRUE;
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }

    return CBaseList::OnChildNotify(uMsg, wParam, lParam, pResult);
}

template<typename CBaseList>
BOOL SubItemsControls<CBaseList>::
OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

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
            const int iItem = pNMCD->nmcd.dwItemSpec;
            const int iSubItem = pNMCD->iSubItem;
            const auto* control = m_controls.Get(GetRealItemIndex(iItem), iSubItem);
            if (!control)
                break;
            control->get()->Draw(pNMCD, CBaseList::GetItemText(iItem, iSubItem));

            *pResult = CDRF_SKIPDEFAULT;
        }
        return TRUE;
    default:
        break;
    }
    // Будем выполнять стандартную обработку для всех сообщений по умолчанию
    *pResult = CDRF_DODEFAULT;
    return FALSE;
}

} // namespace controls::list::widgets