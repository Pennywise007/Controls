#include "stdafx.h"
#include "ListCustomizing.h"

BEGIN_MESSAGE_MAP(CHeaderCtrlEx, CHeaderCtrl)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CHeaderCtrlEx::OnNMCustomdraw)
    ON_WM_MEASUREITEM()
END_MESSAGE_MAP()


void CHeaderCtrlEx::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    //LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

    switch(pNMCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;          // ask for subitem notifications.
        break;

    case CDDS_ITEMPREPAINT:
        {
            CDC* pDC = CDC::FromHandle(pNMCD->nmcd.hdc);

            // �� ����� ��������� ������ ����������� �����
            pDC->SelectObject(GetStockObject(DEFAULT_GUI_FONT));

            CRect rcContent;
            // ����� ������������ ���� ������ � �������� ���������
            // �� https://docs.microsoft.com/ru-ru/windows/win32/controls/parts-and-states?redirectedfrom=MSDN
            HTHEME hTheme = OpenThemeData(m_hWnd, L"HEADER");

            // ������ ��� ������ � �������� ���������� ������
            HRESULT hr = DrawThemeBackground(hTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                                             HIS_NORMAL, &pNMCD->nmcd.rc, 0);
            if (SUCCEEDED(hr))
            {
                hr = GetThemeBackgroundContentRect(hTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                                                   HIS_NORMAL, &pNMCD->nmcd.rc, rcContent);
            }

            // ������ �����
            if (SUCCEEDED(hr))
            {
                HDITEM hdi = { 0 };
                TCHAR  lpBuffer[256];

                hdi.mask = HDI_TEXT;
                hdi.pszText = lpBuffer;
                hdi.cchTextMax = sizeof(lpBuffer) - 1;

                GetItem(pNMCD->nmcd.dwItemSpec, &hdi);

                // ����� ��� ���������
                DWORD drawTextStyle = DT_WORD_ELLIPSIS | DT_CENTER;

                CRect rcText = rcContent;
                // �.�. ������� ������������ ����� ������������ ��� �� �����
                // ������ DrawThemeTextEx �.�. DrawThemeText �� ������������ DT_CALCRECT
                DTTOPTS dttopts = { sizeof(DTTOPTS) };
                dttopts.dwFlags = DTT_CALCRECT;
                DrawThemeTextEx(hTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                                HIS_NORMAL, lpBuffer, wcslen(lpBuffer),
                                DT_CALCRECT | drawTextStyle,
                                rcText, &dttopts);

                // ���������� ���� ������������ �������
                rcText.OffsetRect(rcContent.CenterPoint() - rcText.CenterPoint());

                // �� ���� ����� �� �������
                if (rcText.left < rcContent.left)
                    rcText.OffsetRect(rcContent.left - rcText.left, 0);
                if (rcText.top < rcContent.top)
                    rcText.OffsetRect(0, rcContent.top - rcText.top);

                DrawThemeText(hTheme, pNMCD->nmcd.hdc, HP_HEADERITEM,
                              HIS_NORMAL, lpBuffer, wcslen(lpBuffer),
                              drawTextStyle, 0, rcText);
            }

            // ��������� ����
            if (hTheme)
                CloseThemeData(hTheme);

            pDC->SelectStockObject(SYSTEM_FONT);

            *pResult = CDRF_SKIPDEFAULT;
            break;
        }

    default:// it wasn't a notification that was interesting to us.
        *pResult = CDRF_DODEFAULT;
        break;
    }
}


void CHeaderCtrlEx::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    // TODO: Add your message handler code here and/or call default

    CHeaderCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
