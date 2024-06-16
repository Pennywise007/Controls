#include "pch.h" // CPopertyPage compilation problems without this line

#include "afxcolordialog.h"
#include "afxcolormenubutton.h"

#include "CMFCColorSelection.h"

IMPLEMENT_DYNAMIC(CMFCColorButtonEx, CMFCColorButton)

void CMFCColorButtonEx::OnShowColorPopup()
{
	// Full copy of the CMFCColorButton::OnShowColorPopup() but with CMFCColorPopupMenuEx instead of CMFCColorPopupMenu

	if (m_pPopup != NULL)
	{
		m_pPopup->SendMessage(WM_CLOSE);
		m_pPopup = NULL;
		return;
	}

	if (m_Colors.GetSize() == 0)
	{
		// Use default pallete:
		CMFCColorBarEx::InitColors(NULL, m_Colors);
	}

	CMFCColorPopupMenuEx* menu = new CMFCColorPopupMenuEx(this, m_Colors, m_Color, m_strAutoColorText, m_strOtherText, m_strDocColorsText, m_lstDocColors, m_nColumns, m_ColorAutomatic);
	menu->m_bEnabledInCustomizeMode = m_bEnabledInCustomizeMode;

	CRect rectWindow;
	GetWindowRect(rectWindow);

	if (!menu->Create(this, rectWindow.left, rectWindow.bottom, NULL, m_bEnabledInCustomizeMode))
	{
		ASSERT(FALSE);
		menu = NULL;

		TRACE(_T("Color menu can't be used in the customization mode. You need to set CMFCColorButton::m_bEnabledInCustomizeMode\n"));
	}
	else
	{
		if (m_bEnabledInCustomizeMode)
		{
			CMFCColorBarEx* pColorBar = DYNAMIC_DOWNCAST(CMFCColorBarEx, menu->GetMenuBar());

			if (pColorBar != NULL)
			{
				ASSERT_VALID(pColorBar);
				pColorBar->m_bInternal = TRUE;
			}
		}

		CRect rect;
		menu->GetWindowRect(&rect);
		menu->UpdateShadow(&rect);

		if (m_bAutoSetFocus)
		{
			menu->GetMenuBar()->SetFocus();
		}
	}
	m_pPopup = menu;

	if (m_bCaptured)
	{
		ReleaseCapture();
		m_bCaptured = FALSE;
	}
}

IMPLEMENT_DYNAMIC(CMFCColorBarEx, CMFCColorBar)

BOOL CMFCColorBarEx::OpenColorDialog(const COLORREF colorDefault, COLORREF& colorRes)
{
	// Full copy of the  CMFCColorBar::OpenColorDialog but we set parent to CColorDialog and CMFCColorDialog
	CMFCColorMenuButton* pColorMenuButton = NULL;

	CMFCPopupMenu* pParentMenu = DYNAMIC_DOWNCAST(CMFCPopupMenu, GetParent());
	if (pParentMenu != NULL)
	{
		pColorMenuButton = DYNAMIC_DOWNCAST(CMFCColorMenuButton, pParentMenu->GetParentButton());
		if (pColorMenuButton != NULL)
		{
			return pColorMenuButton->OpenColorDialog(colorDefault, colorRes);
		}
	}

	BOOL bResult = FALSE;

	if (m_bStdColorDlg)
	{
		CColorDialog dlg(colorDefault, CC_FULLOPEN | CC_ANYCOLOR, this);
		if (dlg.DoModal() == IDOK)
		{
			colorRes = dlg.GetColor();
			bResult = TRUE;
		}
	}
	else
	{
		CMFCColorDialog dlg(colorDefault, 0, this);
		if (dlg.DoModal() == IDOK)
		{
			colorRes = dlg.GetColor();
			bResult = TRUE;
		}
	}

	return bResult;
}

IMPLEMENT_DYNAMIC(CMFCColorPopupMenuEx, CMFCColorPopupMenu)

BEGIN_MESSAGE_MAP(CMFCColorPopupMenuEx, CMFCColorPopupMenu)
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CMFCColorPopupMenuEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Copy of the CMFCColorPopupMenu::OnCreate but with creating m_wndColorBarEx instead of m_wndColorBar
	DWORD toolbarStyle = AFX_DEFAULT_TOOLBAR_STYLE;
	if (GetAnimationType() != NO_ANIMATION && !CMFCToolBar::IsCustomizeMode())
	{
		toolbarStyle &= ~WS_VISIBLE;
	}

	if (!m_wndColorBarEx.Create(this, toolbarStyle | CBRS_TOOLTIPS | CBRS_FLYBY, 1))
	{
		TRACE(_T("Can't create popup menu bar\n"));
		return -1;
	}

	CWnd* pWndParent = GetParent();
	ASSERT_VALID(pWndParent);

	m_wndColorBarEx.SetOwner(pWndParent);
	m_wndColorBarEx.SetPaneStyle(m_wndColorBarEx.GetPaneStyle() | CBRS_TOOLTIPS);

	auto res = CMFCColorPopupMenu::OnCreate(lpCreateStruct);
	// Destroying created in base class m_wndColorBar
	m_wndColorBar.DestroyWindow();
	return res;
}