#pragma once

#include <afxtoolbarimages.h>

#include "afxcmn.h"
#include "afxwin.h"

#include <memory>
//*************************************************************************************************
constexpr auto kUseImageSize = -1;
//*************************************************************************************************
// перечисление видов привязки иконки на контроле
enum Alignment
{
    LeftCenter = 0,	// слева по центру
    CenterTop,		// по центру сверху
    CenterCenter,	// по центру
    CenterBottom,	// по центру снизу
    RightCenter		// справа по центру
};
//*************************************************************************************************
class CIconButton :	public CButton
{
public:	//*****************************************************************************************
    CIconButton();
public:	//*****************************************************************************************
    // loading image for the button, Alignments - flag of image binding relative to the button
    // IconWidth - icon width, if USE_ICON_SIZE then icon width will be used
    // IconHeight - icon height, if USE_ICON_SIZE then icon height will be used
    HICON SetIcon(_In_ UINT uiImageID,	_In_opt_ Alignment Alignment = CenterTop,
                  _In_opt_ int IconWidth = kUseImageSize, _In_opt_ int IconHeight = kUseImageSize);
    HICON SetIcon(_In_ HICON hIcon,		_In_opt_ Alignment Alignment = CenterTop,
                  _In_opt_ int IconWidth = kUseImageSize, _In_opt_ int IconHeight = kUseImageSize);
    // loading image for the button, Alignments - flag of image binding relative to the button
    // ColorMask - mask loaded from BITMAP
    // bUseColorMask - mask usage flag
    // IconWidth - icon width, if USE_ICON_SIZE then icon width will be used
    // IconHeight - icon height, if USE_ICON_SIZE then icon height will be used
    void SetBitmap(_In_ UINT uiBitmapID, _In_opt_ Alignment Alignment = CenterTop,
                   _In_opt_ int BitmapWidth = kUseImageSize, _In_opt_ int BitmapHeight = kUseImageSize,
                   _In_opt_ bool useTransparentColor = false,
                   _In_opt_ COLORREF transparentColor = RGB(0, 0, 0));
    void SetBitmap(_In_ CBitmap& hBitmap, _In_opt_ Alignment Alignment = CenterTop,
                   _In_opt_ int BitmapWidth = kUseImageSize, _In_opt_ int BitmapHeight = kUseImageSize,
                   _In_opt_ bool useTransparentColor = false,
                   _In_opt_ COLORREF transparentColor = RGB(0, 0, 0));
    //*********************************************************************************************
    void SetImageOffset	(_In_ long lOffset);		// установка отступа изображения от границ кнопки
    void SetTextColor	(_In_ COLORREF Color);		// установка цвета текста
    void SetBkColor		(_In_ COLORREF BkColor);	// установка фона для кнопки
    // установка флага использования стандартного фона или заданного пользователем
    void UseDefaultBkColor(_In_opt_ bool bUseStandart = true);
    //*********************************************************************************************
    void SetTooltip(_In_ CString Tooltip);
protected://***************************************************************************************
    void RepositionItems();		// масштабируем рабочие области кнопки
protected://***************************************************************************************
    long m_lOffset;				// отступ от границ контрола при отрисовке иконки
    CSize m_IconSize;			// размер иконки
    CPoint m_IconPos;			// местоположение верхнего левого угла иконки
    Alignment m_Alignment;		// привязка кнопки
    CMFCToolBarImages m_image;  // иконка
    //*********************************************************************************************
    bool m_bUseCustomBkColor;	// флаг использования пользовательского фона для кнокпи
    COLORREF m_TextColor;		// цвет текста
    COLORREF m_BkColor;			// цвет фона
    CString m_ButtonText;		// текст на кнопке
    CRect m_TextRect;			// размер текста
    bool m_bNeedCalcTextPos;	// флаг необходимости пересчитать размеры области под текст
    //*********************************************************************************************
    std::unique_ptr<CToolTipCtrl> m_ptooltip;	// тултип
public:	//*****************************************************************************************
    DECLARE_MESSAGE_MAP()
    virtual void PreSubclassWindow();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};	//*********************************************************************************************