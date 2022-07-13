#pragma once

#include <afx.h>
#include <functional>
#include <list>
#include <unordered_map>

/*
* DefaultWindowProc manager, allow to add callbacks on window messages
*
* Usage:

    DefaultWindowProc::OnWindowMessage(button, WM_NCDESTROY, [](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        OutputDebugString(L"On destroy button");
    }, this);
*/
class DefaultWindowProc
{
public:
    /// Callback for handling message from window, set result if you want to interrupt handling message by other handlers
    typedef std::function<void(HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result)> Callback;

    /// Add callback on window message
    static void OnWindowMessage(const CWnd& targetWindow, UINT message, Callback&& callback, const LPVOID handler)
    {
        ENSURE(::IsWindow(targetWindow));
        ENSURE(!!handler);

        DefaultWindowProc& instance = Instance();
        instance.TryAttachToWindowProc(targetWindow);

        auto windowInfoIt = instance.m_windowsInfo.find(static_cast<HWND>(targetWindow));
        ASSERT(windowInfoIt != instance.m_windowsInfo.end());
        windowInfoIt->second.callbacksByMessages[message].emplace(handler, std::move(callback));
    }

    static void RemoveCallback(const CWnd& targetWindow, UINT message, const LPVOID handler)
    {
        DefaultWindowProc& instance = Instance();

        auto windowInfoIt = instance.m_windowsInfo.find(static_cast<HWND>(targetWindow));
        if (windowInfoIt == instance.m_windowsInfo.end())
        {
            // no callbacks for target window
            return;
        }

        auto messagesCallbacksIt = windowInfoIt->second.callbacksByMessages.find(message);
        if (messagesCallbacksIt == windowInfoIt->second.callbacksByMessages.end())
        {
            // no callbacks for this message
            ASSERT(FALSE);
            return;
        }

        auto handlerCallbackIt = messagesCallbacksIt->second.find(handler);
        if (handlerCallbackIt == messagesCallbacksIt->second.end())
        {
            // this handler hasn`t subscribed to events yet
            ASSERT(FALSE);
            return;
        }

        // clean callbacks
        messagesCallbacksIt->second.erase(handlerCallbackIt);
        if (messagesCallbacksIt->second.empty())
        {
            windowInfoIt->second.callbacksByMessages.erase(messagesCallbacksIt);
            if (windowInfoIt->second.callbacksByMessages.empty())
            {
                // restore default window proc
                ::SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)windowInfoIt->second.defaultWindowProc);
                instance.m_windowsInfo.erase(windowInfoIt);
            }
        }
    }
private:
    // allow creating only from instance
    DefaultWindowProc() = default;
    ~DefaultWindowProc()
    {
        ASSERT(m_windowsInfo.empty());

        for (auto&&[hWnd, windowInfo] : m_windowsInfo)
        {
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)windowInfo.defaultWindowProc);
        }
    }

    DefaultWindowProc(const DefaultWindowProc&) = delete;
    DefaultWindowProc& operator=(const DefaultWindowProc&) = delete;

private:
    // get anchor manager instance
    static DefaultWindowProc& Instance()
    {
        static DefaultWindowProc s;
        return s;
    }
    // checking if we already attached to window def window proc, if no - attach
    void TryAttachToWindowProc(HWND hWnd)
    {
        if (m_windowsInfo.find(hWnd) == m_windowsInfo.end())
            m_windowsInfo.emplace(hWnd, WindowInfo((WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc)));
    }
    // if we don`t need to handle def window proc but we do - detach
    void CheckNecessityToHandleDefProc(HWND hWnd)
    {
        if (const auto it = m_windowsInfo.find(hWnd); it != m_windowsInfo.end() && it->second.callbacksByMessages.empty())
        {
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)it->second.defaultWindowProc);
            m_windowsInfo.erase(it);
        }
    }
    // proxy window proc
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DefaultWindowProc& instance = Instance();
        const auto windowInfoIt = instance.m_windowsInfo.find(hWnd);
        if (windowInfoIt == instance.m_windowsInfo.end())
        {
            ASSERT(FALSE);
            return S_FALSE;
        }

        const CallbacksForMessage& callbacks = windowInfoIt->second.callbacksByMessages;
        const auto messageCallbackIt = callbacks.find(uMsg);
        if (messageCallbackIt != callbacks.end())
        {
#ifdef DEBUG
            CString str;
            if (::IsWindow(hWnd))
                CWnd::FromHandle(hWnd)->GetWindowText(str);
            else
                ASSERT(FALSE);
#endif

            LRESULT result = 0;
            for (auto&& [handler, callback] : messageCallbackIt->second)
            {
                callback(hWnd, wParam, lParam, result);
                if (result)
                {
                    if (uMsg == WM_NCDESTROY)
                        break;
                    return result;
                }
            }
        }

        WNDPROC defaultWindowProc = windowInfoIt->second.defaultWindowProc;
        switch (uMsg)
        {
        case WM_NCDESTROY:
            // restore def window proc
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)defaultWindowProc);
            instance.m_windowsInfo.erase(windowInfoIt);
            break;
        default:
            break;
        }

        return ::CallWindowProc(defaultWindowProc, hWnd, uMsg, wParam, lParam);
    }

private:
    typedef std::unordered_map<LPVOID, Callback> CallbackByHandler;
    typedef std::unordered_map<UINT, CallbackByHandler> CallbacksForMessage;
    struct WindowInfo
    {
        explicit WindowInfo(WNDPROC proc) noexcept : defaultWindowProc(proc) {}

        WNDPROC defaultWindowProc;
        CallbacksForMessage callbacksByMessages;
    };

    // list of handling windows and their information
    std::unordered_map<HWND, WindowInfo> m_windowsInfo;
};
