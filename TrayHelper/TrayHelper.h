#pragma once

#include <afxdialogex.h>
#include <functional>

////////////////////////////////////////////////////////////////////////////////
// Helper class for tray setup and notification display
class CTrayHelper : public CDialogEx
{
public:
    CTrayHelper();
    ~CTrayHelper();

    // Function for creating menu
    typedef std::function<HMENU()> CreateTrayMenu;
    // Function called to destroy created menu
    typedef std::function<void(HMENU)> DestroyTrayMenu;
    // Function for selecting menu item
    typedef std::function<void(UINT)> OnSelectTrayMenu;
    // Function called on notification click
    typedef std::function<void()> OnUserClick;
    // Function called on double-click
    typedef std::function<void()> OnUserDBLClick;
public:
    /// <summary>Add an icon to the tray.</summary>
    /// <param name="icon">Icon in the tray.</param>
    /// <param name="tipText">Tooltip in the tray.</param>
    /// <param name="createTrayMenu">Create menu on right-click/left-click on tray icon.</param>
    /// <param name="destroyTrayMenu">Destroy menu, if nullptr default ::DestroyMenu will be called.</param>
    /// <param name="onSelectTrayMenu">Function called on menu command click.</param>
    /// <param name="onDBLClick">Double-click on tray icon.</param>
    void addTrayIcon(const HICON& icon,
                     const CString& tipText,
                     const CreateTrayMenu& createTrayMenu = nullptr,
                     const DestroyTrayMenu& destroyTrayMenu = nullptr,
                     const OnSelectTrayMenu& onSelectTrayMenu = nullptr,
                     const OnUserDBLClick& onDBLClick = nullptr);
    /// <summary>Remove an icon from the tray.</summary>
    void removeTrayIcon();
    /// <summary>Show a Windows notification.</summary>
    /// <param name="title">Notification title.</param>
    /// <param name="descr">Notification description.</param>
    /// <param name="dwBubbleFlags">Flags to show the notification.</param>
    /// <param name="onUserClick">User click on the icon.</param>
    void showBubble(const CString& title,
                    const CString& descr,
                    const DWORD dwBubbleFlags = NIIF_WARNING,
                    const OnUserClick& onUserClick = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);

// Tray icon display parameters
private:
    // Tray icon existence flag
    bool m_bTrayIconExist = false;

    // Callbacks for tray control
    CreateTrayMenu   m_trayCreateMenu = nullptr;        // menu creation
    DestroyTrayMenu  m_trayDestroyMenu = nullptr;       // menu destruction
    OnSelectTrayMenu m_traySelectMenuItem = nullptr;    // menu item selection
    OnUserDBLClick   m_trayDBLUserClick = nullptr;      // double-click in menu

    // Notification parameters
private:
    OnUserClick      m_notificationClick = nullptr;   // notification click

private:
    // Structure with notification parameters
    NOTIFYICONDATA   m_niData;
};
