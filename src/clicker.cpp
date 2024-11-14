#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <tlhelp32.h>
#include <thread>

class AutoClicker {
private:
    int clickCount = 10;        // 默认点击次数
    int clickInterval = 10;     // 点击间隔(毫秒)
    POINT clickPos;             // 记录点击位置
    bool isClicking = false;    // 点击状态
    HWND gameWindow = NULL;     // 游戏窗口句柄
    std::thread clickThread;    // 点击线程

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
                fileName++; // 跳过反斜杠
                _wcslwr_s(fileName, wcslen(fileName) + 1);
                result = (wcscmp(fileName, L"gamemd-spawn.exe") == 0);
            }
        }

        CloseHandle(hProcess);
        return result;
    }

    void PostClick(int x, int y, int count = 1) {
        HWND targetWindow = GetForegroundWindow();
        if (!IsTargetGameWindow(targetWindow)) return;

        POINT pt = { x, y };
        ScreenToClient(targetWindow, &pt);
        LPARAM lParam = MAKELPARAM(pt.x, pt.y);
        
        for (int i = 0; i < count && !stopClicking; i++) {
            PostMessage(targetWindow, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
            Sleep(5);  // 短暂延迟确保按下消息被处理
            PostMessage(targetWindow, WM_LBUTTONUP, MK_LBUTTON, lParam);
            Sleep(clickInterval);
        }
        
        isClicking = false;
    }

    void ShowClickCountTooltip() {
        char text[32];
        sprintf_s(text, "Clicks: %d", clickCount);
        
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
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
            
            // 使用线程延迟销毁提示窗口
            std::thread([hwnd]() {
                Sleep(1000);
                DestroyWindow(hwnd);
            }).detach();
        }
    }

public:
    bool stopClicking = false;

    bool IsGameActive() {
        return IsTargetGameWindow(GetForegroundWindow());
    }

    void StartClicking(int x, int y) {
        if (!IsGameActive()) return;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        if (x > (screenWidth - 160)) {
            stopClicking = false;
            isClicking = true;
            
            // 如果之前的线程还在运行，等待它结束
            if (clickThread.joinable()) {
                clickThread.join();
            }
            
            // 启动新的点击线程
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

    void AdjustClickCount(bool increase) {
        if (increase) {
            clickCount = (clickCount < 30) ? clickCount + 1 : 1;
        } else {
            clickCount = (clickCount > 1) ? clickCount - 1 : 30;
        }
        ShowClickCountTooltip();
    }

    ~AutoClicker() {
        StopClicking();
        if (clickThread.joinable()) {
            clickThread.join();
        }
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
    // 安装
