#pragma once
#include "afxcmn.h"
#include <list>
//*************************************************************************************************
/*	Combo list with icons
* 
//*************************************************************************************************
!!! Control window needs to be recreated and RecreateCtrl() automatically if it was not created dynamically !!!

	Ideal style for combobox is:
	WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED
	CBS_OWNERDRAWFIXED will be added and CBS_OWNERDRAWVARIABLE | CBS_HASSTRINGS will be removed for displaying icons
//*************************************************************************************************
Example:
	std::list<HICON> icons = { AfxGetApp()->LoadStandardIcon(IDI_QUESTION) };
!!! Don't forget to call ::DestroyIcon on any created icons

	iconCombobox.RecreateCtrl(100);

	iconCombobox.SetIconList(icons);
	iconCombobox.InsertItem(0, L"Item1", 0);
	iconCombobox.SetCurSel(0);

*/
//*************************************************************************************************
class CIconComboBox : public CComboBoxEx
{
	static constexpr int kUseImageIndex = -1;
public://******************************************************************************************
	CIconComboBox() = default;
	~CIconComboBox();
public://******************************************************************************************
	// Control window rectreation, will be called automatically if control was not created dynamically
	virtual void RecreateCtrl(_In_opt_ int dropdownMaxHeight = 100);
	//*********************************************************************************************
	// Setting images list
	virtual void SetIconList(_In_ std::list<HICON> iconList, _In_opt_ CSize iconSizes = CSize(15, 15));
	virtual void SetBitmapsList(_In_ std::list<CBitmap*> bitmaps, _In_opt_ CSize bitmapSizes = CSize(15, 15));
	//*********************************************************************************************
	// Adding line in control
	virtual int InsertItem(_In_ int iItemIndex, 
						   _In_ LPCTSTR pszText,
						   _In_ int iImageIndex,
						   _In_opt_ int iSelectedImageIndex	= kUseImageIndex,
						   _In_opt_ int iIndent	= 0);
	virtual int InsertItem(_In_ COMBOBOXEXITEM *citem);
	// Get current window text
	virtual void GetWindowText(CString& text);
public://******************************************************************************************
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) override;
protected://*****************************************************************************************
	void ReleaseResources();
protected://***************************************************************************************
	CImageList m_imageList;
	std::list<HICON> m_tempIcons;
	bool m_bNeedRecreate = true;
};	//*********************************************************************************************