#include "CMFCEditBrowseCtrlEx.h"

BEGIN_MESSAGE_MAP(CMFCEditBrowseCtrlEx, CMFCEditBrowseCtrl)
END_MESSAGE_MAP()

void CMFCEditBrowseCtrlEx::OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot)
{
    CMemDC dc(*pDC, rect);
    __super::OnDrawBrowseButton(&dc.GetDC(), rect, bIsButtonPressed, bIsButtonHot);
}