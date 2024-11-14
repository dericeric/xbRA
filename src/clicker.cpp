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

    void ClearCurrentTooltip() {
        if (currentTooltip) {
            DestroyWindow(currentTooltip);
            currentTooltip = NULL;
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

            std::thread([this, hwndTT]() {
                Sleep(1000);
                if (currentTooltip == hwndTT) {
                    ClearCurrentTooltip();
                }
            }).detach();
        }
    }

    void ShowActivationMessage() {
        ClearCurrentTooltip();

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        HWND hwndMsg = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_LAYERED,
            L"STATIC",
            L"连点器已激活，团战一触即发！",
            WS_POPUP | SS_CENTER,
            screenWidth / 2 - 150,
            screenHeight - 100,
            300,
            30,
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (hwndMsg) {
            currentTooltip = hwndMsg;
            SetLayeredWindowAttributes(hwndMsg, 0, 200, LWA_ALPHA);
            ShowWindow(hwndMsg, SW_SHOW);

            std::thread([this, hwndMsg]() {
                Sleep(1000);
                if (currentTooltip == hwndMsg) {
                    ClearCurrentTooltip();
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
            if (kbStruct->vkCode == 'Q' || kbStruct->vkCode == 'W') {
                g_clicker.StopClicking();
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
