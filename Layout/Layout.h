﻿#pragma once

#include <afx.h>
#include <afxext.h>
#include <afxtempl.h>
#include <afxlayout.h>
#include <afxwin.h>
#include <bitset>
#include <functional>
#include <list>
#include <map>
#include <optional>

enum class AnchorSide : size_t
{
    eLeft,
    eTop,
    eRight,
    eBottom,
    eCount
};

/*
* Layout manager, allow to manage windows position on resizing
*
* Usage:
    // Layout window

    Layout::AnchorWindow(m_movingButton, main_dlg(*this), { AnchorSide::eLeft, AnchorSide::eRight }, AnchorSide::eRight, 100);
    Layout::AnchorWindow(m_movingButton, *this, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);

    // Setting bounds

    CRect rect;
    m_movingButton.GetWindowRect(rect);
    ScreenToClient(rect);
    CRect dlgClientRect;
    GetClientRect(dlgClientRect);
    Layout::SetWindowBounds(m_movingButton, Layout::BoundsType::eOffsetFromParentBounds, rect.left, rect.top,
                            dlgClientRect.right - rect.right, dlgClientRect.bottom - rect.bottom);
*/
class Layout
{
public: // anchoring controls
    typedef std::bitset<static_cast<size_t>(AnchorSide::eCount)> AnchoredSides;

    static constexpr auto AllAnchorSides = { AnchorSide::eLeft, AnchorSide::eTop, AnchorSide::eRight, AnchorSide::eBottom };

    /// <summary>
    /// Layout window to target window side
    /// </summary>
    /// <param name="who">Window to add anchors</param>
    /// <param name="anchorTarget">Target for anchoring</param>
    /// <param name="anchoredSides">Sides of anchored window</param>
    /// <param name="anchorTargetSide">Target side for anchoring another window</param>
    /// <param name="ratio">1-100, aspect ratio when changing sides of anchoring window</param>
    static void AnchorWindow(const CWnd& who, const CWnd& anchorTarget, AnchoredSides&& anchoredSides, AnchorSide anchorTargetSide, int ratio);
    static void AnchorWindow(const CWnd& who, const CWnd& anchorTarget, const std::initializer_list<AnchorSide>& anchoredSides, AnchorSide anchorTargetSide, int ratio);
    static void AnchorRemove(const CWnd& who, const CWnd& anchorTarget, const std::initializer_list<AnchorSide>& removingAnchoredForSides = AllAnchorSides);

public: // setting control moving and sizing bounds
    enum class BoundsType
    {
        eControlBounds,             // setting work area beyond which the control cannot go
        eOffsetFromParentBounds     // setting minimum offset from parent control bounds
    };
    // Setting bounds for moving control, if kNotSpecified then the border is not taken into account
    static void SetWindowBounds(CWnd& wnd, BoundsType type, std::optional<LONG> left, std::optional<LONG> top, std::optional<LONG> right, std::optional<LONG> bottom);
    // Setting minimum size of window, don`t allow to resize less
    static void SetWindowMinimumSize(CWnd& wnd, std::optional<UINT> width, std::optional<UINT> height);
public: // callbacks
    // On size changed callback.If width changed - newWidth != empty, if height changed - newHeight != empty
    typedef std::function<void(CWnd& window, int newWidth, int newHeight)> OnSizeChangedCallback;
    // Setting on size changed callback.If width changed - newWidth != empty, if height changed - newHeight != empty
    static void OnSizeChanged(CWnd& wnd, const OnSizeChangedCallback& onSizeChanged);

private:
    void OnWindowPosChanged(HWND hWnd, WPARAM wParam, LPARAM lParam);
    void OnWindowPosChanging(HWND hWnd, WPARAM wParam, LPARAM lParam);
    void OnGetMinMaxInfo(HWND hWnd, WPARAM wParam, LPARAM lParam);

private:
    // allow creating only from instance
    Layout() = default;

    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;

    // get anchor manager instance
    static Layout& Instance();
    // calc window rect
    [[nodiscard]] CRect GetWindowRect(HWND hWnd, HWND hAttachingWindow = nullptr);
    // if we don`t need to handle def window proc but we do - detach
    void CheckNecessityToHandleOnSizeChangedEvent(const CWnd& window);

private:    // anchors
    struct AnchorTargetInfo
    {
        struct AnchoredWindowInfo
        {
            // bit set of anchors to target anchor side
            AnchoredSides anchoredSides;
            // side of target window for applying window anchors
            AnchorSide anchorSide;
            // ratio for moving sides on anchors
            int ratio;
            // initial window rect for anchored window
            CRect initialWindowRect;
            // initial rect of target window
            CRect initialTargetRect;
            // apply anchors to window rects, return true if rect changed
            void ApplyAnchors(const CRect& currentTargetRect,
                              std::optional<double>& deltaLeft, std::optional<double>& deltaTop,
                              std::optional<double>& deltaRight, std::optional<double>& deltaBottom) const;
        };
        // list of windows and their anchors to target window
        std::map<HWND, std::list<AnchoredWindowInfo>> anchoredWindows;
    };

    // information about target anchors
    std::map<HWND, AnchorTargetInfo> m_anchoredWindows;

private:    // bounds
    struct BoundInfo
    {
        BoundsType type;

        std::optional<LONG> left;
        std::optional<LONG> top;
        std::optional<LONG> right;
        std::optional<LONG> bottom;

        // apply bounds to window rect, return true if rect changed
        bool ApplyBoundsToRect(HWND window, CRect& newRect);
    };
    // list of windows and their bounds
    std::map<HWND, BoundInfo> m_boundedWindows;

private:    // Size restrictions
    struct MinimumSize
    {
        std::optional<UINT> width;
        std::optional<UINT> height;
    };
    // list of windows and their size restrictions
    std::map<HWND, MinimumSize> m_minimumSizeWindows;
private:
    std::map<HWND, OnSizeChangedCallback> m_onSizeChangedCallbacks;
private:
    // If control linked sides to different controls it can be in inconsistent state during resizing
    // For example: we linked left control side to the splitter and right to the dialog
    // when we exit from full screen mode we apply changes to the right side before applying changes to left size
    // our control width become <0 but when we try to move it - negative values will be ignored.
    // We will store current rect and apply it when we receive movements for the left side.
    std::unordered_map<HWND, CRect> m_windowsWithNegativeRects;
};

// Helper class, allow to apply layout from dialog resource files
struct LayoutLoader : private CMFCDynamicLayoutData // for access to m_listCtrls
{
    /// <summary>
    /// Load layout from resource file and apply it to Layout
    /// Must be called after CDialog::OnInitDialog or CFormView::OnInitDialog, example:
    /// LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
    /// </summary>
    /// <param name="lpszResourceName">name or MAKEINTRESOURCE, wnd.m_lpszTemplateName</param>
    static bool ApplyLayoutFromResource(CFormView& formView, LPCTSTR lpszResourceName);
    static bool ApplyLayoutFromResource(CDialog& dialog, LPCTSTR lpszResourceName);

private:
    // don`t allow to create outside
    LayoutLoader() = default;

    // Realization of ApplyLayoutFromResource
    static bool ApplyLayoutFromResourceForWindow(CWnd& wnd, LPCTSTR lpszResourceName);
};
