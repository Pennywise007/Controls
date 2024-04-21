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

class CToolTip : public ::CToolTipCtrl
{
    DECLARE_MESSAGE_MAP()

public:
    CToolTip() = default;
    CToolTip(CWnd& wnd, const CString& text);

    void SetTooltip(CWnd& wnd, const CString& text);

private:
    CWnd* m_attachedWnd = nullptr;
};

// Adding tooltip without holding CToolTip object
// NOTE: don't call it for the same window
void SetTooltip(CWnd& wnd, const CString& text);

} // namespace controls