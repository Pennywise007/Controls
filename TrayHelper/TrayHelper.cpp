#include <strsafe.h>
#include <typeinfo>

#include "TrayHelper.h"

#include "../Utils/WindowClassRegistration.h"

////////////////////////////////////////////////////////////////////////////////
// Tray icon messages
#define WM_TRAY_ICON        (WM_APP + 100)

namespace {
// Tray icon identifier, should not be 0 to display balloons
constexpr auto kTrayId = 1;
} // namespace

//----------------------------------------------------------------------------//
BEGIN_MESSAGE_MAP(CTrayHelper, CDialogEx)
    ON_MESSAGE(WM_TRAY_ICON, &CTrayHelper::OnTrayIcon)
END_MESSAGE_MAP()

//----------------------------------------------------------------------------//
CTrayHelper::CTrayHelper()
{
    HINSTANCE instance = AfxGetInstanceHandle();
    const CString className(typeid(*this).name());

    // Register our class
    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, className, &wndClass))
    {
        // Register window class used for cell editing
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = AfxGetInstanceHandle();
        wndClass.lpszClassName = className;

        static WindowClassRegistrationLock rigistration(wndClass);
    }

    // Create an invisible window to handle tray events
    if (CDialogEx::CreateEx(WS_EX_TOOLWINDOW, className, L"",
                            0,
                            0, 0, 0, 0,
                            NULL, nullptr, nullptr) == FALSE)
        ASSERT(!"Failed to create taskbar control window!");

    // Initialize notification data structure
    ::ZeroMemory(&m_niData, sizeof(NOTIFYICONDATA));

    m_niData.cbSize = sizeof(NOTIFYICONDATA);
    m_niData.hWnd = m_hWnd;
    m_niData.uID = kTrayId;
    m_niData.uCallbackMessage = WM_TRAY_ICON;
}

//----------------------------------------------------------------------------//
CTrayHelper::~CTrayHelper()
{
    removeTrayIcon();

    CDialogEx::DestroyWindow();
}

//----------------------------------------------------------------------------//
CTrayHelper& CTrayHelper::Instance()
{
    static CTrayHelper s;
    return s;
}

//----------------------------------------------------------------------------//
void CTrayHelper::addTrayIcon(const HICON& icon,
                              const CString& tipText,
                              const CreateTrayMenu& onCreateTrayMenu /*= nullptr*/,
                              const DestroyTrayMenu& onDestroyTrayMenu /*= nullptr*/,
                              const OnSelectTrayMenu& onSelectTrayMenu /*= nullptr*/,
                              const OnUserDBLClick& onDBLClick /*= nullptr*/)
{
    m_trayCreateMenu = onCreateTrayMenu;
    m_trayDestroyMenu = onDestroyTrayMenu;
    m_traySelectMenuItem = onSelectTrayMenu;
    m_trayDBLUserClick = onDBLClick;

    // Create tray icon
    m_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_niData.hIcon = icon;

    StringCchCopy(m_niData.szTip, ARRAYSIZE(m_niData.szTip), tipText);

    ::Shell_NotifyIcon(m_bTrayIconExist ? NIM_MODIFY : NIM_ADD, &m_niData);

    m_bTrayIconExist = true;
}

//----------------------------------------------------------------------------//
void CTrayHelper::removeTrayIcon()
{
    if (!m_bTrayIconExist)
        return;

    // Remove icon from tray
    ::Shell_NotifyIcon(NIM_DELETE, &m_niData);

    m_bTrayIconExist = false;
    m_trayCreateMenu = nullptr;
    m_trayDestroyMenu = nullptr;
    m_traySelectMenuItem = nullptr;
    m_trayDBLUserClick = nullptr;
}

//----------------------------------------------------------------------------//
void CTrayHelper::showBubble(const CString& title,
                             const CString& descr,
                             DWORD dwBubbleFlags /*= NIIF_WARNING*/,
                             const OnUserClick& onUserClick /*= nullptr*/)
{
    m_notificationClick = onUserClick;

    m_niData.uFlags |= NIF_INFO;
    // Enable message processing if notification click is provided
    if (m_notificationClick)
        m_niData.uFlags |= NIF_MESSAGE;

    m_niData.dwInfoFlags = dwBubbleFlags;

    StringCchCopy(m_niData.szInfo, ARRAYSIZE(m_niData.szInfo), descr);
    StringCchCopy(m_niData.szInfoTitle, ARRAYSIZE(m_niData.szInfoTitle), title);

    // Remove old and make new, this prevents "queue" of balloons
    ::Shell_NotifyIcon(NIM_DELETE, &m_niData);
    ::Shell_NotifyIcon(NIM_ADD, &m_niData);
}

//----------------------------------------------------------------------------//
BOOL CTrayHelper::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // Notify about menu selection
    if (m_traySelectMenuItem)
        m_traySelectMenuItem(LOWORD(wParam));

    return CDialogEx::OnCommand(wParam, lParam);
}

//----------------------------------------------------------------------------//
afx_msg LRESULT CTrayHelper::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        if (!m_trayCreateMenu)
            break;

        // Create menu
        HMENU hMenu = m_trayCreateMenu();
        if (hMenu)
        {
            POINT pt;
            ::GetCursorPos(&pt);
            // Window should be foreground to avoid double-clicking
            // See documentation for TrackPopupMenu
            ::SetForegroundWindow(m_hWnd);

            ::TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, nullptr);

            if (m_trayDestroyMenu)
                m_trayDestroyMenu(hMenu);
            else
                ::DestroyMenu(hMenu);
        }
    }
    break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        // Notify about double-click
        if (m_trayDBLUserClick)
            m_trayDBLUserClick();

        break;
    case NIN_BALLOONUSERCLICK:  // Notification click
        if (m_notificationClick)
            m_notificationClick();
        break;
    }

    return 0;
}
