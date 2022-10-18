#include "window.h"
#include <iostream>

static_assert(std::numeric_limits<SimT>::is_iec559);

inline void CHECK_HR(const HRESULT hr) 
{
    if(FAILED(hr)) 
    {
        std::cout << std::hex << "0x" << hr << std::endl; 
        system("pause"); 
        abort();
    }
}

CAppModule module_;

int main()
{
    HRESULT hr = S_OK;

    // initialize COM
    CHECK_HR(hr = CoInitialize(nullptr));
    AtlInitCommonControls(ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES);

    // create window
    CHECK_HR(hr = module_.Init(nullptr, nullptr));
    {
        CMessageLoop msgloop;
        module_.AddMessageLoop(&msgloop);

        MainWnd mainWnd;
        if(mainWnd.Create(nullptr, CWindow::rcDefault, windowTitle) == nullptr)
            CHECK_HR(hr = E_UNEXPECTED);
        mainWnd.ShowWindow(SW_SHOW);
        const int ret = msgloop.Run(); ret;
        module_.RemoveMessageLoop();
    }

    CoUninitialize();

    return 0;
}