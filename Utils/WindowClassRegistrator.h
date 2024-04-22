#include <Windows.h>
#include <string>

struct WindowsClassRegistrationLock {

    WindowsClassRegistrationLock(WNDCLASSEX& wndClass)
        : m_hInstance(wndClass.hInstance)
        , className(wndClass.lpszClassName)
    {
        ENSURE(::RegisterClassEx(&wndClass));
    }

    ~WindowsClassRegistrationLock()
    {
        ENSURE(::UnregisterClass(className.c_str(), m_hInstance));
    }

private:
    const HINSTANCE m_hInstance;
    const std::wstring className;
};
