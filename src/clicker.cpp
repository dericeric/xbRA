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
    int clickInterval = 1;  // 减小间隔提高速度
    POINT clickPos;
    bool isClicking = false;
    bool stopClicking = false;
    HWND currentTooltip = NULL;
    bool wasGameActive = false;

    // 预定义鼠标输入结构
    INPUT mouseDown = { 0 };
    INPUT mouseUp = { 0 };

    // 初始化鼠标输入结构
    void InitMouseInput() {
        mouseDown.type = INPUT_MOUSE;
        mouseDown.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        
        mouseUp.type = INPUT_MOUSE;
        mouseUp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    }

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

    void ClearCurrentTooltip() {
        if (currentTooltip && IsWindow(currentTooltip)) {
            DestroyWindow(currentTooltip);
            currentTooltip = NULL;
        }
    }

    void ShowActivationMessage() {
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
            currentTooltip = hwndTT;

            HFONT hFont = CreateFontW(
                16, 0, 0, 0,
                FW_NORMAL,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                L"微软雅黑"
            );
            SendMessageW(hwndTT, WM_SETFONT, (WPARAM)hFont, TRUE);

            TOOLINFOW ti = { 0 };
            ti.cbSize = sizeof(TOOLINFOW);
            ti.uFlags = TTF_ABSOLUTE | TTF_TRACK;
            ti.hwnd = NULL;
            ti.hinst = GetModuleHandle(NULL);
            ti.lpszText = L"[XB Alert] Auto-Clicker Online! Battle Mode Engaged!";

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            SendMessageW(hwndTT, TTM_ADDTOOLW, 0, (LPARAM)&ti);
            SendMessageW(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG((screenWidth - 300) / 2, screenHeight - 100));
            SendMessageW(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

            std::thread([this, hwndTT, hFont]() {
                Sleep(1000);
                if (IsWindow(hwndTT)) {
                    SendMessageW(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                    DestroyWindow(hwndTT);
                    DeleteObject(hFont);
                }
                if (currentTooltip == hwndTT) {
                    currentTooltip = NULL;
                }
      
    
    
      
    
            }).detach();
        }
    }

    void ShowClickCountTooltip(int x, int y) {
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
            currentTooltip = hwndTT;

            HFONT hFont = CreateFontW(
                16, 0, 0, 0,
                FW_NORMAL,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                L"微软雅黑"
            );
            SendMessageW(hwndTT, WM_SETFONT, (WPARAM)hFont, TRUE);

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

            std::thread([this, hwndTT, hFont]() {
                Sleep(1000);
                if (IsWindow(hwndTT)) {
                    SendMessageW(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                    DestroyWindow(hwndTT);
                    DeleteObject(hFont);
                }
                if (currentTooltip == hwndTT) {
                    currentTooltip = NULL;
                }
            }).detach();
        }
    }

    void PostClick(int x, int y, int count = 1) {
        HWND targetWindow = GetForegroundWindow();
        if (!IsTargetGameWindow(targetWindow)) return;

        // 创建独立线程进行连点
        std::thread([this, targetWindow, x, y, count]() {
            POINT pt = { x, y };
            ScreenToClient(targetWindow, &pt);
            
            // 设置鼠标位置
            SetCursorPos(x, y);
            
            for (int i = 0; i < count && !stopClicking; i++) {
                // 检查停止条件
                if ((GetAsyncKeyState('E') & 0x8000) || (GetAsyncKeyState('R') & 0x8000)) {
                    stopClicking = true;
                    isClicking = false;
                    break;
                }

                // 使用 SendInput 发送鼠标事件
                SendInput(1, &mouseDown, sizeof(INPUT));
                Sleep(1);
                SendInput(1, &mouseUp, sizeof(INPUT));
                
                if (clickInterval > 0) {
                    Sleep(clickInterval);
                }
            }
            
            isClicking = false;
        }).detach();
    }

public:
    AutoClicker() {
        InitMouseInput();  // 初始化鼠标输入结构
    }

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
            PostClick(x, y, clickCount);
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
                case 'E':
                case 'R':
                    if (g_clicker.IsClicking()) {
                        g_clicker.StopClicking();
                    }
                    // 直接返回，让按键继续传递
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
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
