#pragma once

#include <afx.h>
#include <ShlObj_core.h>
#include <afxeditbrowsectrl.h>

class CMFCEditBrowseCtrlEx : public CMFCEditBrowseCtrl
{
protected:
    DECLARE_MESSAGE_MAP()

    virtual void OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot) override;
};
