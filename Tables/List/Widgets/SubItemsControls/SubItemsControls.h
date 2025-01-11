#pragma once

#include "afxcmn.h"

#include <afxwin.h>
#include <memory>
#include <optional>
#include <functional>
#include <map>

#include "Controls/Tables/List/SubItemsInfo.h"

namespace controls::list::widgets {

/*
   This class allows you to add controls to the table cells, in particular, a button, a checkbox and an image.

   Important: GetItemData will contain the index of the item when it was added
*/
template <typename CBaseList = CListCtrl>
class SubItemsControls : public CBaseList
{
public:
    SubItemsControls();

    void SetButton(int index, int iSubItem);
    void SetCheckbox(int index, int iSubItem, bool enabled);
    inline static LONG kStratched = -1;
    void SetImage(int index, int iSubItem, CBitmap& hBitmap, LONG imageWidth = kStratched, LONG imageHeight = kStratched);

    void SetCheckboxColumn(int iColumnIndex);

    void RemoveControl(int index, int iSubItem);

    typedef std::function<void(int iItem, int iSubItem)> OnButtonPressed;
    typedef std::function<void(int iItem, int iSubItem, bool newState)> OnCheckboxStateChanged;

    void SetButtonPressedCallback(const OnButtonPressed& onPress);
    void SetCheckboxChangedCallback(const OnCheckboxStateChanged& onChanged);

private:
    [[nodiscard]] std::pair<int, int> GetSubItemUnderPoint(const CPoint& point);
    void CheckCheckboxColumnState(int column);
    int GetRealItemIndex(int item) const;

protected://********************************************************************
    DECLARE_MESSAGE_MAP()

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
    virtual BOOL OnChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

    // дабл клик с созданием контрола редактирования если необходимо
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);

private:
    struct CallbacksHolder
    {
        OnButtonPressed buttonPressedCallback;
        OnCheckboxStateChanged checkboxStateChangedCallback;
    };

    struct Control;

    SubItemsInfo<std::shared_ptr<Control>> m_controls;

    CallbacksHolder m_callbacksHolder;
    // on left button pressed control
    std::shared_ptr<Control> m_buttonDownControl;
    // current hovered sub item indexes, if exists
    std::optional<std::pair<int, int>> m_hoveredSubItem;

    struct ICheckbox;
    std::map<int, size_t> m_checkboxColumns;
};

} // namespace controls::list::widgets

// т.к класс шаблонный прячем реализацию в другой хедер
#include "SubItemsControlsImpl.h"