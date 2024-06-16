#pragma once

#include "afxcolorbar.h"
#include "afxcolorbutton.h"
#include "afxcolorpopupmenu.h"

// Extension for CMFCColorButton, allows to:
// Auto set automatic color value
// Auto get color
// Allow to open color palette as a parent window,by default it opens without parent window
// which can lead to problems if initial window is topmost.
class CMFCColorButtonEx : protected CMFCColorButton
{
	DECLARE_DYNAMIC(CMFCColorButtonEx)
public:
	CMFCColorButtonEx() = default;

	operator CWnd& () { return *this; }
	operator const CWnd& () const { return *this; }
	CWnd& operator()() { return *this; }
	const CWnd& operator()() const { return *this; }

	COLORREF GetColor() const { return m_Color == -1 ? GetAutomaticColor() : m_Color; }
	void SetColor(COLORREF color /* -1 - automatic*/)
	{
		if (color == -1 || color == GetAutomaticColor())
			CMFCColorButton::SetColor(-1);
		else
			CMFCColorButton::SetColor(color);
	}

	using CMFCColorButton::SetPalette;
	using CMFCColorButton::SetColumnsNumber;
	void EnableAutomaticButton(LPCTSTR lpszLabel, COLORREF colorAutomatic, BOOL bEnable = TRUE)
	{
		CMFCColorButton::EnableAutomaticButton(lpszLabel, colorAutomatic, bEnable);
		if (colorAutomatic == GetColor())
			SetColor(-1);
	}
	using CMFCColorButton::EnableOtherButton;
	using CMFCColorButton::SetDocumentColors;
	using CMFCColorButton::SetColorName;
	using CMFCColorButton::GetAutomaticColor;

	using CMFCColorButton::m_bEnabledInCustomizeMode;
	using CMFCColorButton::m_bAutoSetFocus;

protected:
	virtual void OnShowColorPopup() override;
};

// Extension for CMFCColorBar, allows to open CColorDialog and CMFCColorDialog(Other window) as child dialogs
class CMFCColorBarEx : public CMFCColorBar
{
	friend class CMFCColorPopupMenuEx;

	DECLARE_DYNAMIC(CMFCColorBarEx)
public:
	using CMFCColorBar::CMFCColorBar;
	using CMFCColorBar::InitColors;
	using CMFCColorBar::m_bInternal;

protected:
	virtual BOOL OpenColorDialog(const COLORREF colorDefault, COLORREF& colorRes) override;
};

// Extension for CMFCColorPopupMenu, replace menu bar(m_wndColorBar) on CMFCColorBarEx
class CMFCColorPopupMenuEx : public CMFCColorPopupMenu
{
	DECLARE_DYNAMIC(CMFCColorPopupMenuEx)
	DECLARE_MESSAGE_MAP()
public:
	CMFCColorPopupMenuEx(const CArray<COLORREF, COLORREF>& colors, COLORREF color, LPCTSTR lpszAutoColor, LPCTSTR lpszOtherColor, LPCTSTR lpszDocColors, CList<COLORREF, COLORREF>& lstDocColors,
		int nColumns, int nHorzDockRows, int nVertDockColumns, COLORREF colorAutomatic, UINT uiCommandID, BOOL bStdColorDlg = FALSE)
		: CMFCColorPopupMenu(colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, nHorzDockRows, nVertDockColumns, colorAutomatic, uiCommandID, bStdColorDlg)
		, m_wndColorBarEx(colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, nHorzDockRows, nVertDockColumns, colorAutomatic, uiCommandID, NULL)
	{
		m_wndColorBarEx.m_bStdColorDlg = bStdColorDlg;
	}

	CMFCColorPopupMenuEx(CMFCColorButton* pParentBtn, const CArray<COLORREF, COLORREF>& colors, COLORREF color, LPCTSTR lpszAutoColor, LPCTSTR lpszOtherColor,
		LPCTSTR lpszDocColors, CList<COLORREF, COLORREF>& lstDocColors, int nColumns, COLORREF colorAutomatic)
		: CMFCColorPopupMenu(pParentBtn, colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, colorAutomatic)
		, m_wndColorBarEx(colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, -1, -1, colorAutomatic, (UINT)-1, pParentBtn)
	{
	}

	CMFCColorPopupMenuEx(CMFCRibbonColorButton* pParentBtn, const CArray<COLORREF, COLORREF>& colors, COLORREF color, LPCTSTR lpszAutoColor, LPCTSTR lpszOtherColor,
		LPCTSTR lpszDocColors, CList<COLORREF, COLORREF>& lstDocColors, int nColumns, COLORREF colorAutomatic, UINT nID)
		: CMFCColorPopupMenu(pParentBtn, colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, colorAutomatic, nID)
		, m_wndColorBarEx(colors, color, lpszAutoColor, lpszOtherColor, lpszDocColors, lstDocColors, nColumns, colorAutomatic, nID, pParentBtn)
	{
	}

	using CMFCColorPopupMenu::m_bEnabledInCustomizeMode;
	virtual CMFCPopupMenuBar* GetMenuBar() override { return &m_wndColorBarEx; }

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

protected:
	CMFCColorBarEx m_wndColorBarEx;
};
