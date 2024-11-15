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

// 新增：定义输入结构
struct InjectedInput {
    INPUT input;
    MOUSEINPUT mi;
};

// 新增：初始化注入输入结构
void InitInjectedInput(InjectedInput& input, int x, int y, DWORD flags) {
    ZeroMemory(&input, sizeof(InjectedInput));
    input.input.type = INPUT_MOUSE;
    input.input.mi.dx = x;
    input.input.mi.dy = y;
    input.input.mi.dwFlags = flags;
    input.input.mi.time = 0;
    input.input.mi.dwExtraInfo = 0;
}

class AutoClicker {
private:
    int clickCount = 10;
    int clickInterval = 1;  // 减小间隔提高速度
    POINT clickPos;
    bool isClicking = false;
    std::thread clickThread;
    HWND currentTooltip = NULL;
    bool wasGameActive = false;
    
    // 新增：注入用的输入结构
    InjectedInput mouseDown;
    InjectedInput mouseUp;

    // 新增：初始化注入结构的方法
    void InitMouseStructures() {
        InitInjectedInput(mouseDown, 0, 0, MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK);
        InitInjectedInput(mouseUp, 0, 0, MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK);
    }

    // 新增：坐标转换方法
    void ConvertToAbsoluteCoordinates(int& x, int& y) {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        x = (x * 65535) / screenWidth;
        y = (y * 65535) / screenHeight;
    }

    void PostClick(int x, int y, int count = 1) {
        HWND targetWindow = GetForegroundWindow();
        if (!IsTargetGameWindow(targetWindow)) return;

        // 获取目标窗口的线程ID
        DWORD targetThreadId = GetWindowThreadProcessId(targetWindow, NULL);
        DWORD currentThreadId = GetCurrentThreadId();

        // 附加到目标窗口的线程
        AttachThreadInput(currentThreadId, targetThreadId, TRUE);

        // 转换坐标为绝对坐标
        POINT pt = { x, y };
        ClientToScreen(targetWindow, &pt);
        ConvertToAbsoluteCoordinates(pt.x, pt.y);

        // 更新注入结构的坐标
        mouseDown.input.mi.dx = pt.x;
        mouseDown.input.mi.dy = pt.y;
        mouseUp.input.mi.dx = pt.x;
        mouseUp.input.mi.dy = pt.y;

        for (int i = 0; i < count && !stopClicking; i++) {
            // 检查停止条件
            if ((GetAsyncKeyState('E') & 0x8000) || (GetAsyncKeyState('R') & 0x8000)) {
                stopClicking = true;
                isClicking = false;
                break;
            }

            // 发送注入的鼠标事件
            SendInput(1, &mouseDown.input, sizeof(INPUT));
            Sleep(1);  // 极短延迟
            SendInput(1, &mouseUp.input, sizeof(INPUT));

            if (clickInterval > 0) {
                Sleep(clickInterval);
            }
        }

        // 分离线程输入
        AttachThreadInput(currentThreadId, targetThreadId, FALSE);
        isClicking = false;
    }
      
// 在 AutoClicker 类的 private 部分添加：
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

        HWND hwndTT = CreateWindowExW(
            WS_EX_TOPMOST,
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
            ti.lpszText = L"[xb] Auto-Clicker Ready! Let's Fight!";

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
    

public:
    bool stopClicking = false;

    AutoClicker() {
        InitMouseStructures();  // 初始化鼠标结构
    }

    // ... (IsGameActive, CheckGameWindowStatus 等方法保持不变) ...

    void StartClicking(int x, int y) {
        if (!IsGameActive()) return;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        if (x > (screenWidth - 160)) {
            stopClicking = false;
            isClicking = true;
            
            if (clickThread.joinable()) {
                clickThread.join();
            }
            
            // 使用 lambda 捕获 this 和参数
            clickThread = std::thread([this, x, y]() {
                try {
                    PostClick(x, y, clickCount);
                }
                catch (...) {
                    // 错误处理
                    isClicking = false;
                    stopClicking = true;
                }
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
    // 提升进程优先级以提高点击响应速度
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

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
