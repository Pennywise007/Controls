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
        if (!instance.TryAttachToWindowProc(targetWindow))
            return;

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
            if (windowInfoIt->second.callbacksByMessages.empty() && windowInfoIt->second.anyMessagesCallbacks.empty())
            {
                // restore default window proc
                ::SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)windowInfoIt->second.defaultWindowProc);
                instance.m_windowsInfo.erase(windowInfoIt);
            }
        }
    }

    /// Callback for handling message from window, set result if you want to interrupt handling message by other handlers
    typedef std::function<void(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)> AnyMessageCallback;
    /// Add callback on window message
    static void OnAnyWindowMessage(const CWnd& targetWindow, AnyMessageCallback&& callback, const LPVOID handler)
    {
        ENSURE(::IsWindow(targetWindow));
        ENSURE(!!handler);

        DefaultWindowProc& instance = Instance();
        instance.TryAttachToWindowProc(targetWindow);

        auto windowInfoIt = instance.m_windowsInfo.find(static_cast<HWND>(targetWindow));
        ASSERT(windowInfoIt != instance.m_windowsInfo.end());
        windowInfoIt->second.anyMessagesCallbacks.emplace(handler, std::move(callback));
    }
    /// Add callback on window message
    static void RemoveAnyMessageCallback(const CWnd& targetWindow, const LPVOID handler)
    {
        DefaultWindowProc& instance = Instance();

        auto windowInfoIt = instance.m_windowsInfo.find(static_cast<HWND>(targetWindow));
        if (windowInfoIt == instance.m_windowsInfo.end())
        {
            // no callbacks for target window
            return;
        }

        auto handlerCallbackIt = windowInfoIt->second.anyMessagesCallbacks.find(handler);
        if (handlerCallbackIt == windowInfoIt->second.anyMessagesCallbacks.end())
        {
            // this handler hasn`t subscribed to events yet
            ASSERT(FALSE);
            return;
        }

        // clean callbacks
        windowInfoIt->second.anyMessagesCallbacks.erase(handlerCallbackIt);
        if (windowInfoIt->second.callbacksByMessages.empty() && windowInfoIt->second.anyMessagesCallbacks.empty())
        {
            // restore default window proc
            ::SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)windowInfoIt->second.defaultWindowProc);
            instance.m_windowsInfo.erase(windowInfoIt);
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
    bool TryAttachToWindowProc(HWND hWnd)
    {
        if (m_windowsInfo.find(hWnd) == m_windowsInfo.end())
        {
            auto windowPros = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
            if (windowPros == nullptr)
            {
                ASSERT(!"Failed to change window proc");
                return false;
            }
            m_windowsInfo.emplace(hWnd, WindowInfo(windowPros));
        }
        return true;
    }
    // if we don`t need to handle def window proc but we do - detach
    void CheckNecessityToHandleDefProc(HWND hWnd)
    {
        if (const auto it = m_windowsInfo.find(hWnd); it != m_windowsInfo.end() &&
            it->second.callbacksByMessages.empty() && it->second.anyMessagesCallbacks.empty())
        {
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)it->second.defaultWindowProc);
            m_windowsInfo.erase(it);
        }
    }
    // proxy window proc
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DefaultWindowProc& instance = Instance();
        auto windowInfoIt = instance.m_windowsInfo.find(hWnd);
        if (windowInfoIt == instance.m_windowsInfo.end())
        {
            ASSERT(FALSE);
            return S_FALSE;
        }

#ifdef DEBUG // debugging helper
        CString str;
        if (uMsg != WM_GETTEXT && uMsg != WM_GETTEXTLENGTH)
        {
            if (::IsWindow(hWnd))
                CWnd::FromHandle(hWnd)->GetWindowText(str);
            else
                ASSERT(FALSE);
        }
#endif
        WNDPROC defaultWindowProc = windowInfoIt->second.defaultWindowProc;

        LRESULT result = 0;
        for (auto&& [handler, callback] : windowInfoIt->second.anyMessagesCallbacks)
        {
            callback(hWnd, uMsg, wParam, lParam, result);

            // callback might unsubscribe itself
            windowInfoIt = instance.m_windowsInfo.find(hWnd);
            if (windowInfoIt == instance.m_windowsInfo.end())
            {
                if (result || !::IsWindow(hWnd))
                    return 1;
                return ::CallWindowProc(defaultWindowProc, hWnd, uMsg, wParam, lParam);
            }

            if (result)
            {
                if (uMsg == WM_NCDESTROY)
                    break;

                return result;
            }
        }

        if (!result)
        {
            const CallbacksForMessage& callbacks = windowInfoIt->second.callbacksByMessages;
            const auto messageCallbackIt = callbacks.find(uMsg);
            if (messageCallbackIt != callbacks.end())
            {
                for (auto&& [handler, callback] : messageCallbackIt->second)
                {
                    callback(hWnd, wParam, lParam, result);

                    // callback might unsubscribe itself
                    windowInfoIt = instance.m_windowsInfo.find(hWnd);
                    if (windowInfoIt == instance.m_windowsInfo.end())
                    {
                        if (result || !::IsWindow(hWnd))
                            return 1;
                        return ::CallWindowProc(defaultWindowProc, hWnd, uMsg, wParam, lParam);
                    }

                    if (result)
                    {
                        if (uMsg == WM_NCDESTROY)
                            break;
                        return result;
                    }
                }
            }
        }

        switch (uMsg)
        {
        case WM_NCDESTROY:
            // restore def window proc
            ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)defaultWindowProc);
            // don't use 
            instance.m_windowsInfo.erase(windowInfoIt);
            break;
        default:
            break;
        }

        return ::CallWindowProc(defaultWindowProc, hWnd, uMsg, wParam, lParam);
    }

private:
    typedef std::unordered_map<LPVOID, AnyMessageCallback> AnyMessageCallbackByHandler;
    typedef std::unordered_map<LPVOID, Callback> CallbackByHandler;
    typedef std::unordered_map<UINT, CallbackByHandler> CallbacksForMessage;
    struct WindowInfo
    {
        explicit WindowInfo(WNDPROC proc) noexcept : defaultWindowProc(proc) {}

        WNDPROC defaultWindowProc;

        AnyMessageCallbackByHandler anyMessagesCallbacks;
        CallbacksForMessage callbacksByMessages;
    };

    // list of handling windows and their information
    std::unordered_map<HWND, WindowInfo> m_windowsInfo;
};
