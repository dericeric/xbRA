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
    HWND currentTooltip = NULL;
    bool wasGameActive = false;

    // 检查窗口是否属于目标游戏进程
    bool IsTargetGameWindow(HWND hwnd) {
        if (!hwnd) return false;
        
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (!hProcess) return false;

        wchar_t processName[MAX_PATH];
        DWORD size = MAX_PATH;
        bool result = false;

        if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
            wchar_t* fileName = wcsrchr(processName, L'\\');
            if (fileName) {
                fileName++;
                _wcslwr_s(fileName, wcslen(fileName) + 1);
                result = (wcscmp(fileName, L"gamemd-spawn.exe") == 0);
            }
        }

        CloseHandle(hProcess);
        return result;
    }

    // ... 其他成员保持不变 ...

    void ShowActivationMessage() {
        ClearCurrentTooltip();

        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ActivationMessageClass";
            RegisterClassExW(&wc);
            registered = true;
        }

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"ActivationMessageClass",
            L"连点器已激活，团战一触即发！",
            WS_POPUP,
            (screenWidth - 300) / 2,
            screenHeight - 100,
            300,
            30,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwnd) {
            currentTooltip = hwnd;
            SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);

            // 创建线程在1秒后销毁窗口
            std::thread([this, hwnd]() {
                Sleep(1000);
                if (IsWindow(hwnd)) {
                    DestroyWindow(hwnd);
                }
                if (currentTooltip == hwnd) {
                    currentTooltip = NULL;
                }
            }).detach();
        }
    }

    void ShowClickCountTooltip(int x, int y) {
        ClearCurrentTooltip();

        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ClickCountClass";
            RegisterClassExW(&wc);
            registered = true;
        }

        wchar_t text[32];
        swprintf_s(text, L"%d", clickCount);

        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"ClickCountClass",
            text,
            WS_POPUP,
            x - 15,
            y - 30,
            30,
            20,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwnd) {
            currentTooltip = hwnd;
            SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);

            // 创建线程在1秒后销毁窗口
            std::thread([this, hwnd]() {
                Sleep(1000);
                if (IsWindow(hwnd)) {
                    DestroyWindow(hwnd);
                }
                if (currentTooltip == hwnd) {
                    currentTooltip = NULL;
                }
            }).detach();
        }
    }

    void ClearCurrentTooltip() {
        if (currentTooltip && IsWindow(currentTooltip)) {
            DestroyWindow(currentTooltip);
            currentTooltip = NULL;
        }
    }

    // ... 其他方法保持不变 ...
};


    void PostClick(int x, int y, int count = 1) {
        HWND targetWindow = GetForegroundWindow();
        if (!IsTargetGameWindow(targetWindow)) return;

        POINT pt = { x, y };
        ScreenToClient(targetWindow, &pt);
        LPARAM lParam = MAKELPARAM(pt.x, pt.y);
        
        for (int i = 0; i < count && !stopClicking; i++) {
            // 检查 E 和 R 键的状态
            if ((GetAsyncKeyState('E') & 0x8000) || (GetAsyncKeyState('R') & 0x8000)) {
                stopClicking = true;
                isClicking = false;
                break;
            }

            PostMessage(targetWindow, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
            Sleep(5);
            PostMessage(targetWindow, WM_LBUTTONUP, MK_LBUTTON, lParam);
            Sleep(clickInterval);
        }
        
        isClicking = false;
    }

public:
    bool stopClicking = false;

    bool IsGameActive() {
        return IsTargetGameWindow(GetForegroundWindow());
    }

    void CheckGameWindowStatus() {
        bool isActive = IsGameActive();
        if (isActive && !wasGameActive) {
            ShowActivationMessage();
        }
        wasGameActive = isActive;
    }

    void StartClicking(int x, int y) {
        if (!IsGameActive()) return;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        if (x > (screenWidth - 160)) {
            stopClicking = false;
            isClicking = true;
            
            if (clickThread.joinable()) {
                clickThread.join();
            }
            
            clickThread = std::thread([this, x, y]() {
                PostClick(x, y, clickCount);
            });
            clickThread.detach();
        }
    }

    void StopClicking() {
        stopClicking = true;
        isClicking = false;
    }

    bool IsClicking() const {
        return isClicking;
    }

    void AdjustClickCount(bool increase, int mouseX, int mouseY) {
        if (increase) {
            clickCount = (clickCount < 30) ? clickCount + 1 : 1;
        } else {
            clickCount = (clickCount > 1) ? clickCount - 1 : 30;
        }
        ShowClickCountTooltip(mouseX, mouseY);
    }

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
            switch (kbStruct->vkCode) {
                case 'Q':
                case 'W':
                case 'E':  // 添加 E 键检测
                case 'R':  // 添加 R 键检测
                    if (g_clicker.IsClicking()) {
                        g_clicker.StopClicking();
                    }
                    break;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    SetTimer(NULL, 0, 100, [](HWND, UINT, UINT_PTR, DWORD) {
        g_clicker.CheckGameWindowStatus();
    });

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
    
    return 0;
}
