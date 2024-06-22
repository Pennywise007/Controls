// Combobox with checkboxes, allows to select multiple items
// Should be created with CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS

#pragma once

#include "afxwin.h"

class CCheckComboBox : public CComboBox
{
public:
	CCheckComboBox();
	~CCheckComboBox();

	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) override;

	void SetCheck(int item, bool check);
	bool GetCheck(int item);
	// Selects/unselects all items in the list
	void SelectAll(bool check);

	DWORD_PTR GetItemData(int nIndex) const;
	int SetItemData(int nIndex, DWORD_PTR dwItemData);
	void* GetItemDataPtr(int nIndex) const;
	int SetItemDataPtr(int nIndex, void* pData);

	int AddString(LPCTSTR lpszString);
	int InsertString(_In_ int nIndex, _In_z_ LPCTSTR lpszString);
	int DeleteString(UINT nIndex);
	void ResetContent();

protected:
	virtual void PreSubclassWindow() override;
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) override;
	afx_msg LRESULT OnCtlColorListBox(WPARAM wParam, LPARAM lParam);
	// GetWindowText support
	afx_msg LRESULT OnGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

protected:
	// theme to draw checkboxes
	HTHEME m_hCheckboxTheme;
	// Current window text
	CString m_windowText;
	// internal flags to execute code only once
	bool m_itemHeightSet = false;
	bool m_messagesSubscribed = false;
};