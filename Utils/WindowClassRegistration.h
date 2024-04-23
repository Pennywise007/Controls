#include <Windows.h>
#include <string>

struct WindowClassRegistrationLock {

    WindowClassRegistrationLock(WNDCLASSEX& wndClass)
        : m_hInstance(wndClass.hInstance)
        , className(wndClass.lpszClassName)
    {
        ENSURE(::RegisterClassEx(&wndClass));
    }

    ~WindowClassRegistrationLock()
    {
        ENSURE(::UnregisterClass(className.c_str(), m_hInstance));
    }

private:
    const HINSTANCE m_hInstance;
    const std::wstring className;
};
