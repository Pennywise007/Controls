#pragma once

#include <functional>
#include <map>

#include <afx.h>
#include <afxtempl.h>
#include <afxlayout.h>
#include <afxwin.h>

#include <optional>

/**************************************************************************************************
// Using:
// Add Button/Static on form, set CSplitter as object

// Anchor splitter
splitter.AttachSplitterToWindow(*this, CMFCDynamicLayout::MoveHorizontal(50), CMFCDynamicLayout::SizeVertical(100));
// Or use Layout
splitter.SetCallbacks(nullptr, [&](CSplitter* /*splitter)
        {
            Layout::AnchorRemove(splitter, *this, { AnchorSide::eLeft, AnchorSide::eRight });
            Layout::AnchorWindow(splitter, *this, { AnchorSide::eLeft, AnchorSide::eRight }, AnchorSide::eRight, 100);
        });
Layout::AnchorWindow(view, splitter, { AnchorSide::eLeft }, AnchorSide::eRight, 100);

// Restrict bounds
splitter.SetControlBounds(CSplitter::BoundsType::eOffsetFromParentBounds,
        CRect(300, CSplitter::kNotSpecified, rect.Width(), CSplitter::kNotSpecified));

// Anchor windows to splitter
Layout::AnchorWindow(view, splitter, { AnchorSide::eLeft }, AnchorSide::eRight, 100);
**************************************************************************************************/

class CSplitter : public CWnd
{
public:
    // orientation of control
    enum class Orientation
    {
        eHorizontal,  // --
        eVertical     // |
    };
    CSplitter(_In_opt_ Orientation controlOrientation = Orientation::eVertical);
public: // Attaching controls
    // Setting changing layout settings on resizing specified window
    void AttachSplitterToWindow(CWnd& wnd, CMFCDynamicLayout::MoveSettings moveSettings, CMFCDynamicLayout::SizeSettings sizeSettings);
public: // Settings control moving bounds
    static constexpr LONG kNotSpecified = -1;
    enum class BoundsType
    {
        eControlBounds,             // setting work area beyond which the control cannot go
        eOffsetFromParentBounds     // setting minimum offset from parent control bounds
    };
    // Setting bounds for moving control, if kNotSpecified then the border is not taken into account
    void SetControlBounds(_In_ BoundsType type, _In_opt_ CRect bounds = CRect(kNotSpecified, kNotSpecified, kNotSpecified, kNotSpecified));
public: // Settings control moving bounds
    // Setting callbacks for handling drug splitter events,
    // if onStartDruggingSplitter return false - cancel moving control
    void SetCallbacks(_In_opt_ const std::function<bool(CSplitter* splitter, CRect& newRect)>& onPosChanging = nullptr,
        _In_opt_ const std::function<void(CSplitter* splitter)>& onEndDruggingSplitter = nullptr);
    // Setting the cursor from resource file that will be displayed on the buttons when you hover over it
    void SetMovingCursor(_In_ UINT cursorResourceId);
protected:
    // Check and correct new control bounds, move window if necessary
    void ApplyNewRect(_In_ CRect& newRect, _In_opt_ std::optional<CRect> currentRect = std::nullopt);
    // Detaching from window proc to which the splitter is attached
    void UnhookFromAttachedWindow();
    // The callback received when the window is resized to which the splitter is attached
    void OnAttachedWindowSizeChanged(int cx, int cy);

    // changing layout settings for attached window
    struct AttachedWindowsSettings
    {
        AttachedWindowsSettings(CSplitter& splitter, CWnd& wndToAttach, CMFCDynamicLayout::MoveSettings&& moveSettings, CMFCDynamicLayout::SizeSettings&& sizeSettings);
        HWND m_hWndToAttach;
        CRect m_initialWindowRect;
        CRect m_initialSlitterRect;
        CMFCDynamicLayout::MoveSettings m_moveSettings;
        CMFCDynamicLayout::SizeSettings m_sizeSettings;
    };
    // apply layout rules to rect
    static void ApplyLayout(CRect& rect, const AttachedWindowsSettings& settings, const CSize& positionDiff);

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

protected:
    std::optional<CPoint> m_drugStartMousePoint;
    const Orientation m_orientation;
    // bounds for control position
    CRect m_bounds = { kNotSpecified, kNotSpecified, kNotSpecified, kNotSpecified };
    BoundsType m_boundsType = BoundsType::eControlBounds;
    // callbacks
    std::function<bool(CSplitter* splitter, CRect& newRect)> m_onPosChanging;
    std::function<void(CSplitter* splitter)> m_onEndDruggingSplitter;

private: // splitter layout settings
    std::optional<AttachedWindowsSettings> m_splitterLayoutSettings;
    bool m_movingWithParentBounds = false;

private:
    HCURSOR m_hCursor;
};
