#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <tlhelp32.h>
#include <thread>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

// 确保 Common Controls 6.0 可用
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

class AutoClicker {
private:
    int clickCount = 10;
    int clickInterval = 10;
    POINT clickPos;
    bool isClicking = false;
    std::thread clickThread;
    HWND currentTooltip = NULL;    // 跟踪当前显示的tooltip
    bool wasGameActive = false;     // 跟踪游戏窗口的上一个状态

    // 清除现有的tooltip
    void ClearCurrentTooltip() {
        if (currentTooltip) {
            DestroyWindow(currentTooltip);
            currentTooltip = NULL;
        }
    }

    void ShowClickCountTooltip(int x, int y) {
        // 清除现有的tooltip
        ClearCurrentTooltip();

        HWND hwndTT = CreateWindowEx(
            WS_EX_TOPMOST,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwndTT) {
            currentTooltip = hwndTT;  // 保存当前tooltip句柄

            wchar_t text[32];
            swprintf_s(text, L"%d", clickCount);

            TOOLINFOW ti = { 0 };
            ti.cbSize = sizeof(TOOLINFOW);
            ti.uFlags = TTF_ABSOLUTE | TTF_TRACK;
            ti.hwnd = NULL;
            ti.hinst = GetModuleHandle(NULL);
            ti.lpszText = text;

            SendMessageW(hwndTT, TTM_ADDTOOLW, 0, (LPARAM)&ti);
            SendMessageW(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(x, y - 20));
            SendMessageW(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

            // 1秒后自动清除
            std::thread([this, hwndTT]() {
                Sleep(1000);
                if (currentTooltip == hwndTT) {  // 确保这是最新的tooltip
                    ClearCurrentTooltip();
                }
            }).detach();
        }
    }

    void ShowActivationMessage() {
        ClearCurrentTooltip();  // 清除可能存在的其他提示

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        HWND hwndMsg = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_LAYERED,
            L"STATIC",
            L"连点器已激活，团战一触即发！",
            WS_POPUP | SS_CENTER,
            screenWidth / 2 - 150,  // 居中显示
            screenHeight - 100,     // 底部显示
            300,                    // 宽度
            30,                     // 高度
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwndMsg) {
            currentTooltip = hwndMsg;

            // 设置半透明效果
            SetLayeredWindowAttributes(hwndMsg, 0, 200, LWA_ALPHA);  // 透明度为200
            ShowWindow(hwndMsg, SW_SHOW);

            // 1秒后自动消失
            std::thread([this, hwndMsg]() {
                Sleep(1000);
                if (currentTooltip == hwndMsg) {
                    ClearCurrentTooltip();
                }
            }).detach();
        }
    }

public:
    bool stopClicking = false;

    void CheckGameWindowStatus() {
        bool isActive = IsGameActive();
        if (isActive && !wasGameActive) {
            // 游戏窗口刚刚变为活动状态
            ShowActivationMessage();
        }
        wasGameActive = isActive;
    }

    // ... 其他方法保持不变 ...

    ~AutoClicker() {
        StopClicking();
        if (clickThread.joinable()) {
            clickThread.join();
        }
        ClearCurrentTooltip();
    }
};

// 全局实例
AutoClicker g_clicker;

// 鼠标处理函数
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_clicker.IsGameActive()) {
        MSLLHOOKSTRUCT* msStruct = (MSLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_LBUTTONDOWN && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            g_clicker.StartClicking(msStruct->pt.x, msStruct->pt.y);
        }
        else if (wParam == WM_MOUSEWHEEL && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            int wheelDelta = GET_WHEEL_DELTA_WPARAM(msStruct->mouseData);
            g_clicker.AdjustClickCount(wheelDelta > 0, msStruct->pt.x, msStruct->pt.y);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 键盘处理函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_clicker.IsGameActive()) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN) {
            if (kbStruct->vkCode == 'Q' || kbStruct->vkCode == 'W') {
                g_clicker.StopClicking();
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    // 初始化 Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // 安装钩子
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    // 创建定时器检查游戏窗口状态
    SetTimer(NULL, 0, 100, [](HWND, UINT, UINT_PTR, DWORD) {
        g_clicker.CheckGameWindowStatus();
    });

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
    
    return 0;
}
}
