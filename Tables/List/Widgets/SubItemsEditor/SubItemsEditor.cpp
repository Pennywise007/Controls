﻿#include <afxdialogex.h>
#include <cassert>

#include "SubItemsEditor.h"

#include "../../../../Utils/WindowClassRegistrator.h"

namespace controls::list::widgets {

////////////////////////////////////////////////////////////////////////////////
// окно для редактирвоания ячейки списка
class CSubItemEditorWindow : public IEditSubItemWindow
{
public:
    CSubItemEditorWindow(const CRect& rect, const LVSubItemParams::Ptr& subitemParams);

// IEditSubItemWindow
public:
    // устанавливаем внутренний контрол использующийся для редактирования
    void setInternalControl(std::shared_ptr<CWnd> pControlWnd) override;
    // получаем внутренний контрол использующийся для редактирования
    CWnd* getInternalControl() override;
    // получаем информацию об редактируемой ячейке
    LVSubItemParams* getSubItemParams() override;

protected:
    DECLARE_MESSAGE_MAP()

    virtual void OnOK();
    virtual void OnCancel();
protected:
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnDestroy();

private:
    // создаем окно
    void createWindow(const CRect& rect);

private:
    // вставляемый в окно контрол
    std::shared_ptr<CWnd> m_internalControl;
    LVSubItemParams::Ptr m_subItemParams;
};

////////////////////////////////////////////////////////////////////////////////
// создаем окно для редактирвоания ячейки списка
std::unique_ptr<IEditSubItemWindow> IEditSubItemWindow::createWindow(
    const CRect& rect, const LVSubItemParams::Ptr& subItemParams)
{
    return std::make_unique<CSubItemEditorWindow>(rect, subItemParams);
}

BEGIN_MESSAGE_MAP(CSubItemEditorWindow, CDialogEx)
    ON_WM_ACTIVATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
CSubItemEditorWindow::CSubItemEditorWindow(const CRect& rect, const LVSubItemParams::Ptr& subItemParams)
    : m_subItemParams(subItemParams)
{
    HINSTANCE instance = AfxGetInstanceHandle();
    const CString editLabelClassName(typeid(*this).name());

    // регистрируем наш клас
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, editLabelClassName, &wndClass))
    {
        // Регистрация класса окна которое используется для редактирования ячеек
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = editLabelClassName;

        static WindowsClassRegistrationLock lock(wndClass);
    }

    // создаем окно
    createWindow(rect);
}

//----------------------------------------------------------------------------//
void CSubItemEditorWindow::createWindow(const CRect& rect)
{
    // создаем окно
    if (CDialogEx::CreateEx(0, CString(typeid(*this).name()), L"",
                            0,
                            0, 0, 0, 0,
                            NULL, nullptr, nullptr) == FALSE)
    {
        assert(false);
        return;
    }

    // убираем заголовок
    CDialogEx::ModifyStyle(WS_CAPTION, 0, SWP_FRAMECHANGED);

    // после убирания заголовка проставляем новое положение окна
    CDialogEx::MoveWindow(rect);
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
void CSubItemEditorWindow::setInternalControl(std::shared_ptr<CWnd> pControlWnd)
{
    m_internalControl = pControlWnd;

    ::SetFocus(m_internalControl->m_hWnd);
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
CWnd* CSubItemEditorWindow::getInternalControl()
{
    return m_internalControl.get();
}

//----------------------------------------------------------------------------//
// IEditSubItemWindow
LVSubItemParams* CSubItemEditorWindow::getSubItemParams()
{
    return m_subItemParams.get();
}

//----------------------------------------------------------------------------//
void CSubItemEditorWindow::OnOK()
{
    CWnd* ownerWnd = CDialogEx::GetOwner();
    if (ownerWnd && ::IsWindow(ownerWnd->m_hWnd))
        ::SendMessage(ownerWnd->m_hWnd, WM_END_EDIT_SUB_ITEM, IDOK, 0);

    if (::IsWindow(m_hWnd))
        CDialogEx::OnOK();
}

//----------------------------------------------------------------------------//
void CSubItemEditorWindow::OnCancel()
{
    CWnd* ownerWnd = CDialogEx::GetOwner();
    if (ownerWnd && ::IsWindow(ownerWnd->m_hWnd))
        ::SendMessage(ownerWnd->m_hWnd, WM_END_EDIT_SUB_ITEM, IDCANCEL, 0);

    if (::IsWindow(m_hWnd))
        CDialogEx::OnCancel();
}

//----------------------------------------------------------------------------//
void CSubItemEditorWindow::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CDialogEx::OnActivate(nState, pWndOther, bMinimized);

    // если окно потеряло активность - закрываемся
    if (nState == 0)
        OnOK();
}

//----------------------------------------------------------------------------//
void CSubItemEditorWindow::OnDestroy()
{
    CDialogEx::OnDestroy();

    if (m_internalControl && ::IsWindow(m_internalControl->m_hWnd))
        m_internalControl->DestroyWindow();
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<CWnd>
SubItemEditorControllerBase::createEditorControl(CListCtrl* pList,
                                                 CWnd* parentWindow,
                                                 const LVSubItemParams* pParams)
{
    CEdit* editWnd = new CEdit;
    editWnd->Create(getStandartEditorWndStyle() | ES_AUTOHSCROLL,
                    CRect(), parentWindow, 0);
    // ставим клиентские границы чтобы был бордер и отступы
    SetWindowLongPtr(editWnd->m_hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE);

    // ставим текст
    editWnd->SetWindowTextW(pList->GetItemText(pParams->iItem, pParams->iSubItem));

    return std::shared_ptr<CWnd>(editWnd);
}

//----------------------------------------------------------------------------//
void SubItemEditorControllerBase::onEndEditSubItem(CListCtrl* pList, CWnd* editorControl,
                                                   const LVSubItemParams* pParams,
                                                   bool bAcceptResult)
{
    if (!bAcceptResult)
        return;

    CString currentEditorText;
    editorControl->GetWindowTextW(currentEditorText);

    pList->SetItemText(pParams->iItem, pParams->iSubItem, currentEditorText);
}

} // namespace controls::list::widgets