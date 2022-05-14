#pragma once

#include <afxcmn.h>
#include <xstring>

class TextProgressCtrl : public CProgressCtrl
{
    DECLARE_DYNAMIC(TextProgressCtrl)

public:
    TextProgressCtrl() = default;
    virtual ~TextProgressCtrl();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

public:
    void SetZeroRange( short range );
    void SetPosition( int pos );
    void SetIndeterminate( BOOL bInf = TRUE ) const;
    void Pause() const;
    void Error() const;

    // Output string for output progress, example: L"Search progress %d/%",
    // set empty format to output text from SetWindowText
    void SetOutputFormat(const std::wstring& format);

private:
    // if not empty - output text on paint with given format
    std::wstring m_outputFormat;
};
