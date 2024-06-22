#include "utility"
#include "afxcontrolbarutil.h"
#include "memory"

#include "CheckComboBox.h"
#include "../../DefaultWindowProc.h"

BEGIN_MESSAGE_MAP(CCheckComboBox, CComboBox)
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
	ON_MESSAGE(WM_GETTEXT, OnGetText)
	ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

struct ControlData {
	bool checkState = false;
	DWORD_PTR customData = (DWORD_PTR)nullptr;
};

CCheckComboBox::CCheckComboBox()
	: m_hCheckboxTheme(OpenThemeData(m_hWnd, L"Button"))
{
}

CCheckComboBox::~CCheckComboBox()
{
	CloseThemeData(m_hCheckboxTheme);
}

BOOL CCheckComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	// Apply style for which this control is designed
	dwStyle &= ~(CBS_OWNERDRAWVARIABLE | CBS_SIMPLE | CBS_DROPDOWN);
	dwStyle |= CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;

	return CComboBox::Create(dwStyle, rect, pParentWnd, nID);
}

void CCheckComboBox::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();

	// Verify that control have all necessary styles
	const auto style = GetStyle();
	ASSERT(style & CBS_DROPDOWNLIST);
	ASSERT(style & CBS_OWNERDRAWFIXED);
	ASSERT(style & CBS_HASSTRINGS);
}

LRESULT CCheckComboBox::OnCtlColorListBox(WPARAM wParam, LPARAM lParam) 
{
	// subscribe to combo window messages
	if (!m_messagesSubscribed)
	{
		HWND hWnd = (HWND)lParam;
		if (hWnd != 0 && hWnd != m_hWnd)
		{
			m_messagesSubscribed = true;
			
			CWnd* wnd = CWnd::FromHandle(hWnd);
			ASSERT(wnd);
			DefaultWindowProc::OnWindowMessage(*wnd, LB_GETCURSEL, [](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				// Make the combobox always return -1 as the current selection. This
				// causes the lpDrawItemStruct->itemID in DrawItem() to be -1
				// when the always-visible-portion of the combo is drawn
				result = -1;
			}, this);
			DefaultWindowProc::OnWindowMessage(*wnd, WM_LBUTTONUP, [](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				// Don't do anything here. This causes the combobox popup
				// windows to remain open after a selection has been made
				result = 1;
			}, this);
			DefaultWindowProc::OnWindowMessage(*wnd, WM_KEYDOWN, [combobox = this](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				switch (wParam)
				{
				case VK_UP:
				case VK_LEFT:
				{
					LRESULT nIndex = DefaultWindowProc::CallDefaultWindowProc(hWnd, LB_GETCURSEL, wParam, lParam);
					if (nIndex == -1)
						nIndex = 0;
					else if (nIndex == 0)
						nIndex = combobox->GetCount() - 1;
					else
						nIndex--;

					DefaultWindowProc::CallDefaultWindowProc(hWnd, LB_SETCURSEL, nIndex, 0);
				}
				break;
				case VK_DOWN:
				case VK_RIGHT:
				{
					LRESULT nIndex = DefaultWindowProc::CallDefaultWindowProc(hWnd, LB_GETCURSEL, wParam, lParam);
					if (nIndex == -1)
						nIndex = combobox->GetCount() - 1;
					else if (nIndex == combobox->GetCount() - 1)
						nIndex = 0;
					else
						nIndex++;

					DefaultWindowProc::CallDefaultWindowProc(hWnd, LB_SETCURSEL, nIndex, 0);
				}
				break;
				default:
					return;
				}

				result = 1;
			}, this);
			DefaultWindowProc::OnWindowMessage(*wnd, WM_CHAR, [combobox = this](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				if (wParam != VK_SPACE)
					return;
					
				LRESULT nIndex = DefaultWindowProc::CallDefaultWindowProc(hWnd, LB_GETCURSEL, wParam, lParam);

				CRect rcItem;
				::SendMessage(hWnd, LB_GETITEMRECT, nIndex, (LPARAM)&rcItem);
				::InvalidateRect(hWnd, rcItem, FALSE);

				combobox->SetCheck((int)nIndex, !combobox->GetCheck((int)nIndex));

				// Notify that selection has changed
				combobox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(combobox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)combobox->m_hWnd);
					
				result = 1;
			}, this);
			auto lButtonClick = [combobox = this](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				CRect rcClient;
				::GetClientRect(hWnd, rcClient);

				CPoint pt;
				pt.x = LOWORD(lParam);
				pt.y = HIWORD(lParam);

				if (!::PtInRect(rcClient, pt))
					return;

				LRESULT nItemHeight = ::SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0);
				LRESULT nTopIndex = ::SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);

				LRESULT itemIndex = nTopIndex + pt.y / nItemHeight;

				CRect rcItem;
				::SendMessage(hWnd, LB_GETITEMRECT, itemIndex, (LPARAM)&rcItem);

				if (!::PtInRect(rcItem, pt))
					return;

				::InvalidateRect(hWnd, rcItem, FALSE);
				combobox->SetCheck((int)itemIndex, !combobox->GetCheck((int)itemIndex));

				// Notify that selection has changed
				combobox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(combobox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)combobox->m_hWnd);
			};
			DefaultWindowProc::OnWindowMessage(*wnd, WM_LBUTTONDOWN, lButtonClick, this);
			DefaultWindowProc::OnWindowMessage(*wnd, WM_LBUTTONDBLCLK, lButtonClick, this);
				
			auto rButtonClick = [combobox = this](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
				bool allChecked = true;
				for (int i = 0, count = combobox->GetCount(); allChecked && i < count; ++i)
				{
					allChecked &= combobox->GetCheck(i);
				}
				combobox->SelectAll(!allChecked);

				// Make sure to invalidate this window as well
				::InvalidateRect(hWnd, 0, FALSE);
				combobox->GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetWindowLong(combobox->m_hWnd, GWL_ID), CBN_SELCHANGE), (LPARAM)combobox->m_hWnd);
			};
			DefaultWindowProc::OnWindowMessage(*wnd, WM_RBUTTONDOWN, rButtonClick, this);
			DefaultWindowProc::OnWindowMessage(*wnd, WM_RBUTTONDBLCLK, rButtonClick, this);
		}
	}

	return DefWindowProc(WM_CTLCOLORLISTBOX, wParam, lParam);
}

void CCheckComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CMemDC memDC(*CDC::FromHandle(lpDrawItemStruct->hDC), lpDrawItemStruct->rcItem);
	CDC& dc = memDC.GetDC();

	CFont* pOldFont = dc.SelectObject(GetFont());
	dc.FillSolidRect(&lpDrawItemStruct->rcItem, GetBkColor(lpDrawItemStruct->hDC));

	auto oldBkColor	  = dc.SetBkColor(GetBkColor(lpDrawItemStruct->hDC));
	auto oldTextColor = dc.SetTextColor(GetTextColor(lpDrawItemStruct->hDC));

	CRect rcText = lpDrawItemStruct->rcItem;

	CString strText;
	if (lpDrawItemStruct->itemID == -1)
	{
		// Drawing edit area, not a combobox
		strText = m_windowText;
	}
	else
	{
		GetLBText(lpDrawItemStruct->itemID, strText);

		TEXTMETRIC metrics;
		GetTextMetrics(dc, &metrics);

		CRect rcCheckbox;
		rcCheckbox.left   = 0;
		rcCheckbox.right  = lpDrawItemStruct->rcItem.left + metrics.tmHeight + metrics.tmExternalLeading + 6;
		rcCheckbox.top    = lpDrawItemStruct->rcItem.top + 1;
		rcCheckbox.bottom = lpDrawItemStruct->rcItem.bottom - 1;

		rcText.left = rcCheckbox.right;

		int stateId = lpDrawItemStruct->itemState & ODS_SELECTED ? CBS_UNCHECKEDHOT : CBS_UNCHECKEDNORMAL;
		if (GetCheck(lpDrawItemStruct->itemID))
			stateId = lpDrawItemStruct->itemState & ODS_SELECTED ? CBS_CHECKEDHOT : CBS_CHECKEDNORMAL;

		DrawThemeBackground(m_hCheckboxTheme, dc, BP_CHECKBOX, stateId, &rcCheckbox, NULL);
	}

	if (lpDrawItemStruct->itemID == -1)
	{
		if (strText.IsEmpty())
		{
			// draw cue banner
			strText = GetCueBanner();
			SetBkColor(dc, GetSysColor(COLOR_WINDOW));
			SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
		}
		else
		{
			// draw edit text
			SetBkColor(dc, GetSysColor(COLOR_WINDOW));
			SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
		}
	}
	else if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		// draw selected item
		SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else
	{
		// draw item normally
		SetBkColor(dc, GetSysColor(COLOR_WINDOW));
		SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
	}

	// Draw text
	ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rcText, 0, 0, 0);
	DrawText(dc, L' ' + strText, strText.GetLength() + 1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	if ((lpDrawItemStruct->itemState & (ODS_FOCUS|ODS_SELECTED)) == (ODS_FOCUS|ODS_SELECTED))
		DrawFocusRect(dc, &rcText);

	SetBkColor(dc, oldBkColor);
	SetTextColor(dc, oldTextColor);
	dc.SelectObject(pOldFont);
}

void CCheckComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	CClientDC dc(this);
	CFont *pFont = dc.SelectObject(GetFont());

	TEXTMETRIC metrics;
	dc.GetTextMetrics(&metrics);

	constexpr auto extraGap = 2;
	lpMeasureItemStruct->itemHeight = metrics.tmHeight + metrics.tmExternalLeading + extraGap;

	// This is needed since the WM_MEASUREITEM message is sent before
	// MFC hooks everything up if used in i dialog. So adjust the
	// static portion of the combo box now
	if (!m_itemHeightSet) {
		m_itemHeightSet = true;
		SetItemHeight(-1, lpMeasureItemStruct->itemHeight);
	}

	dc.SelectObject(pFont);
}

LRESULT CCheckComboBox::OnGetText(WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0)
		return 0;

	lstrcpyn((LPWSTR)lParam, m_windowText, (int)wParam);
	return m_windowText.GetLength();
}

LRESULT CCheckComboBox::OnGetTextLength(WPARAM, LPARAM)
{
	return m_windowText.GetLength();
}

void CCheckComboBox::OnDestroy()
{
	for (int i = 0, count = GetCount(); i < count; ++i)
	{
		std::unique_ptr<ControlData> deleter((ControlData*)CComboBox::GetItemDataPtr(i));
	}

	CComboBox::OnDestroy();
}

void CCheckComboBox::SetCheck(int item, bool check)
{
	((ControlData*)CComboBox::GetItemDataPtr(item))->checkState = check;

	// Get the list separator
	wchar_t szBuffer[10] = { 0 };
	const int res = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLIST, szBuffer, 10);

	CString strSeparator = res == 0 ? L';' : CString(szBuffer, res - 1);
	strSeparator.TrimRight();
	strSeparator += L' ';

	m_windowText.Empty();
	CString strItem;
	for (int i = 0, count = GetCount(); i < count; ++i)
	{
		if (!GetCheck(i))
			continue;

		GetLBText(i, strItem);
		if (!m_windowText.IsEmpty())
			m_windowText += strSeparator;
		m_windowText += std::move(strItem);
	}

	Invalidate(FALSE);
}

bool CCheckComboBox::GetCheck(int nIndex)
{
	return ((ControlData*)CComboBox::GetItemDataPtr(nIndex))->checkState;
}

void CCheckComboBox::SelectAll(bool check)
{
	for (int i = 0, count = GetCount(); i < count; ++i)
		SetCheck(i, check);
}

DWORD_PTR CCheckComboBox::GetItemData(int nIndex) const
{
	if (nIndex < 0 || nIndex >= GetCount())
		return 0;

	return ((ControlData*)CComboBox::GetItemDataPtr(nIndex))->customData;
}

int CCheckComboBox::SetItemData(int nIndex, DWORD_PTR dwItemData)
{
	if (nIndex < 0 || nIndex >= GetCount())
		return CB_ERR;

	((ControlData*)CComboBox::GetItemDataPtr(nIndex))->customData = dwItemData;
	return 0;
}

void* CCheckComboBox::GetItemDataPtr(int nIndex) const
{
	if (nIndex < 0 || nIndex >= GetCount())
		return nullptr;

	return (void*)((ControlData*)CComboBox::GetItemDataPtr(nIndex))->customData;
}

int CCheckComboBox::SetItemDataPtr(int nIndex, void* dwItemData)
{
	if (nIndex < 0 || nIndex >= GetCount())
		return CB_ERR;

	((ControlData*)CComboBox::GetItemDataPtr(nIndex))->customData = (DWORD_PTR)dwItemData;
	return 0;
}

int CCheckComboBox::AddString(LPCTSTR lpszString)
{
	auto res = CComboBox::AddString(lpszString);
	CComboBox::SetItemDataPtr(res, new ControlData());
	return res;
}

int CCheckComboBox::InsertString(_In_ int nIndex, _In_z_ LPCTSTR lpszString)
{
	auto res = CComboBox::InsertString(nIndex, lpszString);
	CComboBox::SetItemDataPtr(res, new ControlData());
	return res;
}

int CCheckComboBox::DeleteString(UINT nIndex)
{
	std::unique_ptr<ControlData> deleter((ControlData*)CComboBox::GetItemDataPtr(nIndex));
	return CComboBox::DeleteString(nIndex);
}

void CCheckComboBox::ResetContent()
{
	for (int i = 0, count = GetCount(); i < count; ++i)
	{
		std::unique_ptr<ControlData> deleter((ControlData*)CComboBox::GetItemDataPtr(i));
	}
	CComboBox::ResetContent();
}
