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
    int clickCount = 9;
    int clickInterval = 1;
    POINT clickPos;
    bool isClicking = false;
    std::thread clickThread;
    HWND currentTooltip = NULL;
    bool wasGameActive = false;

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

    // 首先定义原始的 UTF-8 字符串
    const char* utf8Text = "xb提示：V7连点器就绪！战斗模式开启！";
      
    
    
      
    
    
    // 计算需要的宽字符缓冲区大小
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Text, -1, NULL, 0);
    if (wideSize <= 0) return;

    // 分配宽字符缓冲区
    wchar_t* wideText = new wchar_t[wideSize];
    
    // 转换为宽字符
    if (MultiByteToWideChar(CP_UTF8, 0, utf8Text, -1, wideText, wideSize) <= 0) {
        delete[] wideText;
        return;
    }

    HWND hwndTT = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,  // 添加 WS_EX_LAYERED 样式
      
    
    
      
    
        TOOLTIPS_CLASSW,
      
    
    
      
    
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwndTT) {
        // 设置窗口透明度
        SetLayeredWindowAttributes(hwndTT, 0, 200, LWA_ALPHA);  // 200 是透明度值(0-255)
        
        // 设置文本颜色为暗绿色
        SendMessage(hwndTT, TTM_SETTIPTEXTCOLOR, RGB(0, 100, 0), 0);

        currentTooltip = hwndTT;
      
    
    
      
    

        HFONT hFont = CreateFontW(
            16,                     // 高度
            0,                      // 宽度
            0, 0,                   // 角度
            FW_NORMAL,              // 粗细
            FALSE,                  // 斜体
            FALSE,                  // 下划线
            FALSE,                  // 删除线
            DEFAULT_CHARSET,        // 使用默认字符集
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"SimSun"              // 宋体
        );

        if (hFont) {
            SendMessageW(hwndTT, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.uFlags = TTF_ABSOLUTE | TTF_TRACK;
        ti.hwnd = NULL;
        ti.hinst = GetModuleHandle(NULL);
        ti.lpszText = wideText;    // 使用转换后的宽字符文本

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SendMessageW(hwndTT, TTM_SETMAXTIPWIDTH, 0, 300);
        SendMessageW(hwndTT, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        SendMessageW(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG((screenWidth - 300) / 2, screenHeight - 100));
        SendMessageW(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

        // 注意：需要在线程中也使用 wideText
        std::thread([this, hwndTT, hFont, wideText]() {
            Sleep(1000);
            if (IsWindow(hwndTT)) {
                SendMessageW(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                DestroyWindow(hwndTT);
                if (hFont) {
                    DeleteObject(hFont);
                }
            }
            if (currentTooltip == hwndTT) {
                currentTooltip = NULL;
            }
            delete[] wideText;  // 清理内存
        }).detach();
    } else {
        delete[] wideText;  // 如果创建窗口失败，清理内存
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

            wchar_t text[32];
            swprintf_s(text, L"%d", clickCount+1);

            TOOLINFOW ti = { 0 };
            ti.cbSize = sizeof(TOOLINFOW);
            ti.uFlags = TTF_ABSOLUTE | TTF_TRACK;
            ti.hwnd = NULL;
            ti.hinst = GetModuleHandle(NULL);
            ti.lpszText = text;

            SendMessageW(hwndTT, TTM_ADDTOOLW, 0, (LPARAM)&ti);
            SendMessageW(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(x, y - 20));
            SendMessageW(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

            std::thread([this, hwndTT]() {
                Sleep(1000);
                if (IsWindow(hwndTT)) {
                    SendMessageW(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                    DestroyWindow(hwndTT);
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
      
    
    
      
    

            SendMessage(targetWindow, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
            Sleep(5);
            SendMessage(targetWindow, WM_LBUTTONUP, MK_LBUTTON, lParam);
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
                case 'Q':  // Q 键处理
                case 'W':  // W 键处理
                    if (g_clicker.IsClicking()) {
                        g_clicker.StopClicking();
                    }
                    // 返回 CallNextHookEx 让按键继续传递
                    return CallNextHookEx(NULL, nCode, wParam, lParam);

                case 'E':  // E 键只停止连点
                case 'R':  // R 键只停止连点
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


