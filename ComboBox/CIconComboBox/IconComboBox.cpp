#include "IconComboBox.h"

CIconComboBox::~CIconComboBox()
{
	ReleaseResources();
}

void CIconComboBox::RecreateCtrl(_In_opt_ int dropdownMaxHeight /*= 100*/)
{
	CRect rect;
	CComboBoxEx::GetWindowRect(rect);
	CComboBoxEx::GetParent()->ScreenToClient(rect);
	rect.bottom += dropdownMaxHeight;

	UINT id = CComboBoxEx::GetDlgCtrlID();
	UINT style = CComboBoxEx::GetStyle();
	UINT styleEx = CComboBoxEx::GetExStyle();
	CWnd* pParrent = CComboBoxEx::GetParent();

	DestroyWindow();
	CreateEx(styleEx, style, rect, pParrent, id);
}

BOOL CIconComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	m_bNeedRecreate = false;

	// Ideal style for combobox is
	// WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_NOINTEGRALHEIGHT
	dwStyle &= ~(CBS_OWNERDRAWVARIABLE | CBS_HASSTRINGS);
	dwStyle |= CBS_OWNERDRAWFIXED;

	return CComboBoxEx::Create(dwStyle, rect, pParentWnd, nID);
}

void CIconComboBox::ReleaseResources()
{
	m_imageList.DeleteImageList();

	for (auto icon : m_tempIcons)
        ::DestroyIcon(icon);
	m_tempIcons.clear();
}

void CIconComboBox::SetIconList(_In_ std::list<HICON> iconList, _In_opt_ CSize iconSizes /*= CSize(15, 15)*/)
{
	if (m_bNeedRecreate)
	{
		RecreateCtrl();
	}

	ReleaseResources();
	m_imageList.Create(iconSizes.cx, iconSizes.cy, ILC_COLOR32, 0, (int)iconList.size());

	for (auto& it : iconList)
		m_imageList.Add(it);

	IMAGEINFO info = { 0 };
	m_imageList.GetImageInfo(0, &info);

	CComboBoxEx::SetImageList(&m_imageList);
}

void CIconComboBox::SetBitmapsList(_In_ std::list<CBitmap*> bitmaps, _In_opt_ CSize bitmapSizes /*= CSize(15, 15)*/)
{
	if (m_bNeedRecreate)
		RecreateCtrl();

	ReleaseResources();
	// we use ILC_COLOR32 because we will draw icons, otherwise use ILC_COLOR24 | ILC_MASK 
	m_imageList.Create(bitmapSizes.cx, bitmapSizes.cy, ILC_COLOR32, 0, int(bitmaps.size()));
	
	for (auto it : bitmaps)
	{
		// Convert bitmap to icon because bitmap got problems with transparency
		BITMAP bmp;
		it->GetBitmap(&bmp);

		HBITMAP hbmMask = ::CreateCompatibleBitmap(::GetDC(NULL), bmp.bmWidth, bmp.bmHeight);

		ICONINFO ii = { 0 };
		ii.fIcon = TRUE;
		ii.hbmColor = *it;
		ii.hbmMask = hbmMask;

		m_imageList.Add(m_tempIcons.emplace_back(::CreateIconIndirect(&ii)));

		::DeleteObject(hbmMask);
	}

	CComboBoxEx::SetImageList(&m_imageList);
}

int CIconComboBox::InsertItem(_In_ int iItemIndex,
							  _In_ LPCTSTR pszText,
							  _In_ int iImageIndex,
							  _In_opt_ int iSelectedImage /*= kUseImageIndex*/,
							  _In_opt_ int iIndent		  /*= 0*/)
{
	COMBOBOXEXITEM cbei;

	cbei.iItem = iItemIndex;
	cbei.pszText = (LPTSTR)pszText;
	cbei.cchTextMax = int(wcslen(pszText) * sizeof(WCHAR));
	cbei.iImage = iImageIndex;
	cbei.iSelectedImage = iSelectedImage != kUseImageIndex ? iSelectedImage : iImageIndex;
	cbei.iIndent = iIndent;

	// Set the mask common to all items.
	cbei.mask = CBEIF_TEXT | CBEIF_INDENT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;

	return InsertItem(&cbei);
}

int CIconComboBox::InsertItem(_In_ COMBOBOXEXITEM* citem)
{
	if (m_bNeedRecreate)
        RecreateCtrl();

	auto item = CComboBoxEx::InsertItem(citem);
	ASSERT(item != -1);

	return item;
}

void CIconComboBox::GetWindowText(CString& text)
{
	auto iItem = GetCurSel();
	if (iItem == -1)
		return CComboBoxEx::GetWindowText(text);

	COMBOBOXEXITEM item = { 0 };
	item.iItem = iItem;
	item.pszText = text.GetBufferSetLength(MAX_PATH);
	item.cchTextMax = MAX_PATH;
	item.mask = CBEIF_TEXT;
	GetItem(&item);

	text.ReleaseBuffer();
}
