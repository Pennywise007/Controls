#include <afxpriv.h>

#include "CMFCEditBrowseCtrlEx.h"

BEGIN_MESSAGE_MAP(CMFCEditBrowseCtrlEx, CMFCEditBrowseCtrl)
    ON_MESSAGE(WM_MFC_INITCTRL, &CMFCEditBrowseCtrlEx::OnInitControl)
END_MESSAGE_MAP()

LRESULT CMFCEditBrowseCtrlEx::OnInitControl(WPARAM wParam, LPARAM lParam)
{
    const auto res = __super::OnInitControl(wParam, lParam);
	if (FAILED(res))
		return res;

	// Fixing problem with small icon while using high DPI screens
	CDC mDC, *pDC = GetDC();
	const auto horScale = ::GetDeviceCaps(pDC->GetSafeHdc(), LOGPIXELSX) / 100.;
	const auto vertScale = ::GetDeviceCaps(pDC->GetSafeHdc(), LOGPIXELSY) / 100.;

	// Resizing defult image
	m_sizeImage.cx = LONG(m_sizeImage.cx * horScale);
	m_sizeImage.cy = LONG(m_sizeImage.cy * vertScale);

	// Creating new bitmap with bigger image
	CBitmap stratchedIconBitmap;
	stratchedIconBitmap.CreateCompatibleBitmap(pDC, m_sizeImage.cx, m_sizeImage.cy);
	mDC.CreateCompatibleDC(pDC);
	ReleaseDC(pDC);

	// Stratching current image on new bitmap
	CBitmap* pOldBitmap = mDC.SelectObject(&stratchedIconBitmap);
	mDC.FillSolidRect(0, 0, m_sizeImage.cx, m_sizeImage.cy, GetSysColor(COLOR_BTNFACE));
	m_ImageBrowse.DrawEx(&mDC, 1, POINT{}, m_sizeImage, CLR_NONE, CLR_NONE, ILD_SCALE);

	// Clean up
	mDC.SelectObject(pOldBitmap);
	mDC.DeleteDC();

	__super::SetBrowseButtonImage(stratchedIconBitmap);

	// Need resize controls
    OnChangeLayout();

    return res;
}

void CMFCEditBrowseCtrlEx::OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot)
{
	// Fixing icon flickering on mouse hover
	CMemDC dc(*pDC, rect);
	__super::OnDrawBrowseButton(&dc.GetDC(), rect, bIsButtonPressed, bIsButtonHot);
}