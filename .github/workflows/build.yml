#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <tlhelp32.h>
#include <thread>           // 添加线程支持
#include <commctrl.h>       // 添加 Common Controls 支持
#pragma comment(lib, "comctl32.lib")

// 如果没有定义这些宏，手动定义它们
#ifndef TTM_TRACKPOSITION
#define TTM_TRACKPOSITION (WM_USER + 18)
#endif

#ifndef TTM_TRACKACTIVATE
#define TTM_TRACKACTIVATE (WM_USER + 17)
#endif

#ifndef TTM_ADDTOOL
#define TTM_ADDTOOL (WM_USER + 4)
#endif

class AutoClicker {
private:
    int clickCount = 10;        // 默认点击次数
    int clickInterval = 10;     // 点击间隔(毫秒)
    POINT clickPos;             // 记录点击位置
    bool isClicking = false;    // 点击状态
    HWND gameWindow = NULL;     // 游戏窗口句柄

    // 使用简单的消息框显示点击次数
    void ShowClickCountTooltip() {
        char text[32];
        sprintf_s(text, "Click Count: %d", clickCount);
        
        // 获取屏幕尺寸
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        // 创建一个临时窗口来显示提示
        HWND hwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"STATIC",
            NULL,
            WS_POPUP | SS_CENTER,
            screenWidth / 2 - 50,
            screenHeight - 100,
            100,
            20,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwnd) {
            SetWindowTextA(hwnd, text);
            ShowWindow(hwnd, SW_SHOW);
            
            // 1秒后销毁窗口
            std::thread([hwnd]() {
                Sleep(1000);
                DestroyWindow(hwnd);
            }).detach();
        }
    }

    void PostClick(int x, int y, int count = 1) {
        if (!gameWindow) return;

        POINT pt = { x, y };
        ScreenToClient(gameWindow, &pt);
        LPARAM lParam = MAKELPARAM(pt.x, pt.y);
        
        for (int i = 0; i < count && !stopClicking; i++) {
            PostMessage(gameWindow, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
            PostMessage(gameWindow, WM_LBUTTONUP, MK_LBUTTON, lParam);
            Sleep(clickInterval);
        }
        
        isClicking = false;
    }

    bool IsWindowFromProcess(HWND hwnd, const wchar_t* processName) {
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(pe32);

        bool found = false;
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                if (processId == pe32.th32ProcessID) {
                    found = (_wcsicmp(pe32.szExeFile, processName) == 0);
                    break;
                }
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return found;
    }

public:
    bool stopClicking = false;

    AutoClicker() {
        gameWindow = FindWindowW(NULL, L"gamemd-spawn.exe");
    }

    bool IsGameActive() {
        return (gameWindow != NULL && gameWindow == GetForegroundWindow());
    }

    void StartClicking(int x, int y) {
        if (!IsGameActive()) return;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        if (x > (screenWidth - 160)) {
            stopClicking = false;
            isClicking = true;
            
            std::thread([this, x, y]() {
                PostClick(x, y, clickCount);
            }).detach();
        }
    }

    void StopClicking() {
        stopClicking = true;
        isClicking = false;
    }

    void AdjustClickCount(bool increase) {
        if (increase) {
            clickCount = (clickCount < 30) ? clickCount + 1 : 1;
        } else {
            clickCount = (clickCount > 1) ? clickCount - 1 : 30;
        }
        ShowClickCountTooltip();
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
            g_clicker.AdjustClickCount(wheelDelta > 0);
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
    // 安装键盘钩子
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    // 安装鼠标钩子
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 卸载钩子
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
    
    return 0;
}
