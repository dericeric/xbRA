#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <tlhelp32.h>

class AutoClicker {
private:
    int clickCount = 10;        // 默认点击次数
    int clickInterval = 10;     // 点击间隔(毫秒)
    POINT clickPos;             // 记录点击位置
    bool isClicking = false;    // 点击状态
    HWND gameWindow = NULL;     // 游戏窗口句柄

    // 使用PostMessage模拟点击
    void PostClick(int x, int y, int count = 1) {
        if (!gameWindow) return;

        // 将屏幕坐标转换为客户区坐标
        POINT pt = { x, y };
        ScreenToClient(gameWindow, &pt);
        
        // 构造消息参数
        LPARAM lParam = MAKELPARAM(pt.x, pt.y);
        
        // 执行指定次数的点击
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
        // 查找游戏窗口
        gameWindow = FindWindowW(NULL, L"Command & Conquer Red Alert 2");
        if (!gameWindow) {
            gameWindow = FindWindowW(NULL, L"gamemd-spawn.exe");
        }
    }

    bool IsGameActive() {
        return (gameWindow != NULL && gameWindow == GetForegroundWindow());
    }

    void StartClicking(int x, int y) {
        if (!IsGameActive()) return;

        // 检查是否在屏幕右侧160像素范围内
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        if (x > (screenWidth - 160)) {
            stopClicking = false;
            isClicking = true;
            
            // 创建新线程执行点击，避免阻塞主线程
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
        
        // 显示提示
        ShowClickCountTooltip();
    }

private:
    void ShowClickCountTooltip() {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        // 创建提示文本
        char tooltipText[32];
        sprintf_s(tooltipText, "%d", clickCount);
        
        // 显示提示
        HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, GetModuleHandle(NULL), NULL);

        TOOLINFO ti = { 0 };
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_ABSOLUTE | TTF_TRACK;
        ti.hwnd = NULL;
        ti.hinst = GetModuleHandle(NULL);
        ti.lpszText = tooltipText;
        
        SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(hwndTT, TTM_TRACKPOSITION, 0, 
            MAKELONG(screenWidth / 2, screenHeight - 80));
        SendMessage(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

        // 1秒后销毁提示
        std::thread([hwndTT]() {
            Sleep(1000);
            DestroyWindow(hwndTT);
        }).detach();
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
