#include "IconButton.h"

#include <afxglobals.h>
#include <afxtoolbarimages.h>
#include <atlimage.h>

namespace {

HBRUSH CreateGradientBrush(COLORREF top, COLORREF bottom, LPNMCUSTOMDRAW item)
{
    HBRUSH Brush = NULL;
    HDC hdcmem = CreateCompatibleDC(item->hdc);
    HBITMAP hbitmap = CreateCompatibleBitmap(item->hdc, item->rc.right - item->rc.left, item->rc.bottom - item->rc.top);
    SelectObject(hdcmem, hbitmap);

    int r1 = GetRValue(top), r2 = GetRValue(bottom), g1 = GetGValue(top), g2 = GetGValue(bottom), b1 = GetBValue(top), b2 = GetBValue(bottom);
    for (int i = 0; i < item->rc.bottom - item->rc.top; ++i)
    {
        RECT temp;
        int r, g, b;
        r = int(r1 + double(i * (r2 - r1) / item->rc.bottom - item->rc.top));
        g = int(g1 + double(i * (g2 - g1) / item->rc.bottom - item->rc.top));
        b = int(b1 + double(i * (b2 - b1) / item->rc.bottom - item->rc.top));
        Brush = CreateSolidBrush(RGB(r, g, b));
        temp.left = 0;
        temp.top = i;
        temp.right = item->rc.right - item->rc.left;
        temp.bottom = i + 1;
        FillRect(hdcmem, &temp, Brush);
        DeleteObject(Brush);
    }
    HBRUSH pattern = CreatePatternBrush(hbitmap);

    DeleteDC(hdcmem);
    DeleteObject(hbitmap);
    if (Brush)
        DeleteObject(Brush);

    return pattern;
}

COLORREF Darker(COLORREF Color, int Percent)
{
    // уменьшениe €ркости
    int r = GetRValue(Color);
    int g = GetGValue(Color);
    int b = GetBValue(Color);

    r = r - MulDiv(r, Percent, 100);
    g = g - MulDiv(g, Percent, 100);
    b = b - MulDiv(b, Percent, 100);
    return RGB(r, g, b);
}

COLORREF Lighter(COLORREF Color, int Percent)
{
    // уменьшениe €ркости
    int r = GetRValue(Color);
    int g = GetGValue(Color);
    int b = GetBValue(Color);

    r = r + MulDiv(255 - r, Percent, 100);
    g = g + MulDiv(255 - g, Percent, 100);
    b = b + MulDiv(255 - b, Percent, 100);
    return RGB(r, g, b);
}
} // namespace

CIconButton::CIconButton()
    : m_Alignment			(Alignment::CenterCenter)
    , m_lOffset				(5)
    , m_bNeedCalcTextPos	(true)
    , m_bUseCustomBkColor	(false)
{
    m_TextColor = GetSysColor(COLOR_BTNTEXT);
    m_BkColor	= GetSysColor(COLOR_BTNFACE);
}

BEGIN_MESSAGE_MAP(CIconButton, CButton)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CIconButton::OnNMCustomdraw)
    ON_WM_SIZE()
END_MESSAGE_MAP()

HICON CIconButton::SetIcon(_In_ UINT uiImageID,
                            _In_opt_ Alignment Alignment /*= CenterTop*/,
                            _In_opt_ int IconWidth	/*= USE_ICON_SIZE*/,
                            _In_opt_ int IconHeight /*= USE_ICON_SIZE*/)
{
    return SetIcon(AfxGetApp()->LoadIcon(uiImageID), Alignment, IconWidth, IconHeight);
}

HICON CIconButton::SetIcon(_In_ HICON hIcon,
                            _In_opt_ Alignment Alignment /*= CenterTop*/,
                            _In_opt_ int IconWidth	/*= USE_ICON_SIZE*/,
                            _In_opt_ int IconHeight /*= USE_ICON_SIZE*/)
{
    if (m_image.GetCount())
        m_image.Clear();

    if (IconWidth == kUseImageSize || IconHeight == kUseImageSize)
    {
        ICONINFO info;
        if (GetIconInfo(hIcon, &info))
        {
            BITMAP bitmap;
            if (GetObject(info.hbmColor, sizeof(bitmap), &bitmap))
            {
                if (IconWidth  == kUseImageSize)
                    IconWidth  = bitmap.bmWidth;
                if (IconHeight == kUseImageSize)
                    IconHeight = bitmap.bmHeight;
            }
            DeleteObject(&bitmap);
        }
        DeleteObject(&info);
    }

    m_image.SetImageSize(SIZE{ IconWidth, IconHeight });
    m_image.SetTransparentColor(GetGlobalData()->clrBtnFace);
    m_image.AddIcon(hIcon, TRUE);
    m_Alignment = Alignment;
    RepositionItems();

    return hIcon;
}

void CIconButton::SetBitmap(_In_ UINT uiBitmapID,
                            _In_opt_ Alignment Alignment,
                            _In_opt_ int BitmapWidth,
                            _In_opt_ int BitmapHeight,
                            _In_opt_ bool useTransparentColor /*= false*/,
                            _In_opt_ COLORREF transparentColor /*= RGB(0, 0, 0)*/)
{
    CBitmap myBitMap;
    if (myBitMap.LoadBitmap(uiBitmapID) == FALSE)
    {
        const LPCTSTR lpszResourceName = MAKEINTRESOURCE(uiBitmapID);
        ENSURE(lpszResourceName != NULL);

        CPngImage pngImage;
        if (pngImage.Load(lpszResourceName) != FALSE)
        {
            myBitMap.Attach(pngImage.Detach());
        }
        else
        {
            const HINSTANCE hinstRes = AfxFindResourceHandle(lpszResourceName, RT_BITMAP);
            if (hinstRes == NULL)
            {
                ASSERT(FALSE);
                return;
            }

            const UINT uiLoadImageFlags = LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS;

            auto bitmap = ::LoadImage(hinstRes, lpszResourceName, IMAGE_BITMAP, 0, 0, uiLoadImageFlags);
            if (bitmap == NULL)
            {
                ASSERT(FALSE);
                return;
            }
            myBitMap.Attach(bitmap);
        }
    }
    SetBitmap(myBitMap, Alignment, BitmapWidth, BitmapHeight, useTransparentColor, transparentColor);
}

void CIconButton::SetBitmap(_In_ CBitmap& hBitmap,
                            _In_opt_ Alignment Alignment,
                            _In_opt_ int BitmapWidth,
                            _In_opt_ int BitmapHeight,
                            _In_opt_ bool useTransparentColor /*= false*/,
                            _In_opt_ COLORREF transparentColor /*= RGB(0, 0, 0)*/)
{
    if (m_image.GetCount())
        m_image.Clear();

    if (BitmapWidth == kUseImageSize || BitmapHeight == kUseImageSize)
    {
        BITMAP bitmap;
        if (hBitmap.GetBitmap(&bitmap))
        {
            if (BitmapWidth == kUseImageSize)
                BitmapWidth = bitmap.bmWidth;
            if (BitmapHeight == kUseImageSize)
                BitmapHeight = bitmap.bmHeight;
        }
    }

    m_image.SetImageSize(SIZE{ BitmapWidth, BitmapHeight });
    m_image.SetTransparentColor(useTransparentColor ? transparentColor : GetGlobalData()->clrBtnFace);
    m_image.AddImage((HBITMAP)hBitmap.Detach(), TRUE);
    m_Alignment = Alignment;
    RepositionItems();
}

void CIconButton::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

    CString Temp;
    CButton::GetWindowText(Temp);
    if (!Temp.IsEmpty())
    {
        m_ButtonText = Temp;
        RepositionItems();
        CButton::SetWindowText(CString());
    }

    if (m_bUseCustomBkColor || m_drawCustomButtonBackground)
    {
        HBRUSH TempBrush;
        HPEN TempPen;

        // Fill background
        TempBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
        FillRect(pNMCD->hdc, &pNMCD->rc, TempBrush);
        DeleteObject(TempBrush);

        if (GetCheck() == BST_CHECKED && GetStyle() & BS_PUSHLIKE)
        {
            // Pressed state
            TempPen = CreatePen(PS_SOLID, 1, RGB(0, 84, 153));
            TempBrush = m_bUseCustomBkColor ?
                CreateSolidBrush(Darker(m_BkColor, 20)) :
                CreateSolidBrush(RGB(204, 228, 247));
        }
        else if (pNMCD->uItemState & ODS_DISABLED)
        {
            // Disabled state
            TempPen = CreatePen(PS_SOLID, 1, RGB(233, 233, 233));
            TempBrush = CreateSolidBrush(RGB(249, 249, 249));
        }
        else if (pNMCD->uItemState & ODS_HOTLIGHT)
        {
            if (pNMCD->uItemState & ODS_SELECTED)
            {
                // Pressed state
                TempPen = CreatePen(PS_SOLID, 1, RGB(0, 84, 153));
                TempBrush = m_bUseCustomBkColor ? 
                    CreateSolidBrush(Darker(m_BkColor, 20)) :
                    CreateSolidBrush(RGB(204, 228, 247));
            }
            else
            {
                // Hovered state
                TempPen = CreatePen(PS_SOLID, 1, RGB(0, 120, 212));
                TempBrush = m_bUseCustomBkColor ?
                    CreateSolidBrush(Lighter(m_BkColor, 20)) :
                    CreateSolidBrush(RGB(224, 238, 249));
            }
        }
        else
        {
            // Normal state
            TempPen = ::GetFocus() == GetSafeHwnd() ?
                CreatePen(PS_SOLID, 1, RGB(0, 120, 212)) :
                CreatePen(PS_SOLID, 1, GetSysColor(COLOR_ACTIVEBORDER));
            TempBrush = m_bUseCustomBkColor ?
                CreateSolidBrush(m_BkColor) :
                CreateSolidBrush(RGB(253, 253, 253));
        }

        SelectObject(pNMCD->hdc, TempPen);
        SelectObject(pNMCD->hdc, TempBrush);
        SetBkMode(pNMCD->hdc, TRANSPARENT);
        RoundRect(pNMCD->hdc, pNMCD->rc.left, pNMCD->rc.top, pNMCD->rc.right, pNMCD->rc.bottom, 6, 6);

        DeleteObject(TempPen);
        DeleteObject(TempBrush);
    }

    CDC *pDC = CDC::FromHandle(pNMCD->hdc);

    //Drawing Bitmap
    if (m_image.GetCount())
    {
        CAfxDrawState ds;
        m_image.PrepareDrawImage(ds);
        m_image.Draw(pDC, m_IconPos.x, m_IconPos.y, 0, FALSE);
        m_image.EndDrawImage(ds);
    }

    pDC->SetBkMode(TRANSPARENT);

    if (m_bNeedCalcTextPos)
    {
        // получаем размер текста который нужно отобразить чтобы расчитать как его центрировать
        CRect tempRect(m_TextRect);
        pDC->DrawText(m_ButtonText, tempRect, DT_EDITCONTROL | DT_WORDBREAK | DT_CALCRECT | DT_NOCLIP);
        m_TextRect.top = m_TextRect.CenterPoint().y - tempRect.Height() / 2;
        m_TextRect.bottom = m_TextRect.top + tempRect.Height();
        m_bNeedCalcTextPos = false;
    }

    pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));

    if (pNMCD->uItemState & ODS_DISABLED)
    {
        pDC->SetTextColor(GetSysColor(COLOR_WINDOW));
        pDC->DrawText(m_ButtonText, m_TextRect, DT_EDITCONTROL | DT_WORDBREAK | DT_END_ELLIPSIS | DT_CENTER);

        pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
        pDC->DrawText(m_ButtonText, m_TextRect, DT_EDITCONTROL | DT_WORDBREAK | DT_END_ELLIPSIS | DT_CENTER);
    }
    else
    {
        pDC->SetTextColor(m_TextColor);
        pDC->DrawText(m_ButtonText, m_TextRect, DT_EDITCONTROL | DT_WORDBREAK | DT_CENTER);
    }

    *pResult = 0;
}

void CIconButton::RepositionItems()
{
    CRect ControlRect;
    GetClientRect(ControlRect);

    if (m_image.GetCount())
    {
        switch (m_Alignment)
        {
            case LeftCenter:
                m_IconPos.SetPoint(m_lOffset, ControlRect.CenterPoint().y - m_image.GetImageSize().cy / 2);

                m_TextRect.top = ControlRect.top + m_lOffset;
                m_TextRect.bottom = ControlRect.bottom - m_lOffset;
                m_TextRect.left = m_IconPos.x + m_image.GetImageSize().cx + m_lOffset;
                m_TextRect.right = ControlRect.right - m_lOffset;
                break;
            case CenterTop:
                m_IconPos.SetPoint(ControlRect.CenterPoint().x - m_image.GetImageSize().cx / 2, m_lOffset);
                m_TextRect.top = m_image.GetImageSize().cy + m_lOffset;
                m_TextRect.bottom = ControlRect.bottom - m_lOffset;
                m_TextRect.left = ControlRect.left + m_lOffset;
                m_TextRect.right = ControlRect.right - m_lOffset;
                break;
            case CenterBottom:
                m_IconPos.SetPoint(ControlRect.CenterPoint().x - m_image.GetImageSize().cx / 2, ControlRect.Height() - m_image.GetImageSize().cy);
                m_TextRect.top = ControlRect.top + m_lOffset;
                m_TextRect.bottom = ControlRect.bottom - m_image.GetImageSize().cy - m_lOffset;
                m_TextRect.left = ControlRect.left + m_lOffset;
                m_TextRect.right = ControlRect.right - m_lOffset;
                break;
            case RightCenter:
                m_IconPos.SetPoint(ControlRect.Width() - m_image.GetImageSize().cx - m_lOffset, ControlRect.CenterPoint().y - m_image.GetImageSize().cy / 2);
                m_TextRect.top = ControlRect.top + m_lOffset;
                m_TextRect.bottom = ControlRect.bottom - m_lOffset;
                m_TextRect.left = ControlRect.left + m_lOffset;
                m_TextRect.right = ControlRect.right - m_image.GetImageSize().cx - m_lOffset;
                break;
            case CenterCenter:
                m_IconPos.SetPoint(ControlRect.CenterPoint().x - m_image.GetImageSize().cx / 2, ControlRect.CenterPoint().y - m_image.GetImageSize().cy / 2);
                m_TextRect.SetRectEmpty();
                break;
            default:
                m_TextRect = ControlRect;
                break;
        }
    }
    else
        m_TextRect = ControlRect;

    m_bNeedCalcTextPos = true;
    Invalidate();
}

void CIconButton::PreSubclassWindow()
{
    GetWindowText(m_ButtonText);

    CButton::SetWindowText(CString());

    GetClientRect(m_TextRect);

    CButton::PreSubclassWindow();
}

void CIconButton::OnSize(UINT nType, int cx, int cy)
{
    CButton::OnSize(nType, cx, cy);
    RepositionItems();
}

void CIconButton::SetImageOffset(_In_ long lOffset)
{
    m_lOffset = lOffset;
    RepositionItems();
}

void CIconButton::SetTextColor(_In_ COLORREF Color)
{
    m_TextColor = Color;
    Invalidate();
}

void CIconButton::SetBkColor(_In_ COLORREF BkColor)
{
    m_bUseCustomBkColor = true;
    m_BkColor = BkColor;
    Invalidate();
}

void CIconButton::UseDefaultBkColor(_In_opt_ bool bUseStandart /*= true*/)
{
    m_bUseCustomBkColor = !bUseStandart;
    Invalidate();
}

void CIconButton::UseCustomBackgroundDraw(bool use)
{
    m_drawCustomButtonBackground = use;
    Invalidate();
}
