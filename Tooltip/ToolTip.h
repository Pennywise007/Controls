#pragma once

#include <afxcmn.h>

/*
* Allows to add tooltip to window, usage:
* controls::CToolTip crosshairTooltip;
* crosshairTooltip.SetTooltip(&m_crosshairControl, L"Text...);
* 
* or you can just call
* controls::SetTooltip(&m_crosshairControl, L"Text...);
*/

namespace controls {

// Adding tooltip without holding CToolTip object
void SetTooltip(CWnd& wnd, const CString& text);

class CToolTip : public ::CToolTipCtrl
{
    DECLARE_MESSAGE_MAP()

public:
    CToolTip() = default;
    CToolTip(CWnd& wnd, const CString& text);

    void SetTooltip(CWnd& wnd, const CString& text);

private:
    friend void SetTooltip(CWnd& wnd, const CString& text);

    CWnd* m_attachedWnd = nullptr;
};

} // namespace controls