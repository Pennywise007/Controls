#pragma once

// Класс обёртка над CListBox, позволяет устанавливать цвета элементам и рисовать их в несколько строк
#include <afxwin.h>
#include <memory>
#include <optional>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// CListBoxEx
class CListBoxEx : public CListBox
{
public:
    CListBoxEx(bool multilineText = true);

    // Вставить элемент с заданием цвета
    // если nIndex == -1 делается AddString иначе - вставляется по указанному индексу
    int AddItem(const CString& itemText, COLORREF color, int nIndex = -1);
    int AddString(const CString& text);
    int InsertString(int index, const CString& text);

    void GetText(int index, CString& sLabel) const;

protected:
    DECLARE_MESSAGE_MAP()

    virtual void PreSubclassWindow() override;
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
    afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    BOOL OnEraseBkgnd(CDC* pDC);

private:
    const bool m_multilineText;
    bool m_internalInsertingText = false;
    struct LineInfo
    {
        explicit LineInfo(CString lineText) noexcept
            : text(std::move(lineText))
        {}

        std::optional<COLORREF> lineColor;
        // collecting all strings manually because we want to display text after /0 symbols
        const CString text;
    };

    std::vector<std::shared_ptr<LineInfo>> m_lineInfo;
};
