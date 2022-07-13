#pragma once

#include <afxwin.h>

class ScrollWnd : public CWnd
{
public:
    /// <summary> Set scroll on new position </summary>
    /// <param name="nBar"> SB_HORZ or SB_VERT </param>
    /// <param name="newPos"></param>
    void SetScrollPosition(int nBar, int newPos);

    // Set full control size
    void SetTotalSize(CSize sizeTotal);
    // Get full control size
    const CSize& GetTotalSize() const;

    void GetVisibleClientArea(CRect& rect) const;

public:
    DECLARE_MESSAGE_MAP()

    // Calling on paint, dc already prepared and moved on scroll positions
    virtual void OnDraw(CDC& dc) = 0;
    virtual void OnVerticalVisibleAreaChanged() {}

private:
    void CalculateScrolls();

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnPaint();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
    bool m_scrollCalculating = false;
    CSize m_totalSize = { 100, 100 };
};
