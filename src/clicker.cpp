#include <windows.h>
#include <iostream>
      
    
    
      
    
#include <psapi.h>
#include <tlhelp32.h>

class AutoClicker {
private:
    int clickCount = 10;        // 默认点击次数
    int clickInterval = 20;     // 点击间隔(毫秒)
    POINT clickPos;             // 记录点击位置
    bool isClicking = false;    // 点击状态
    HWND gameWindow = NULL;     // 游戏窗口句柄

    // 新增：检查窗口是否属于指定进程
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

    // 新增：查找游戏窗口
    HWND FindGameWindow() {
        // 存储找到的窗口句柄
        HWND result = NULL;
        
        // 枚举所有顶层窗口
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            AutoClicker* self = reinterpret_cast<AutoClicker*>(lParam);
            
            // 检查窗口是否可见
            if (!IsWindowVisible(hwnd)) return TRUE;
            
            // 检查是否是游戏进程的窗口
            if (self->IsWindowFromProcess(hwnd, L"gamemd-spawn.exe")) {
      
    
    
      
    
                self->gameWindow = hwnd;
                return FALSE; // 停止枚举
            }
            return TRUE; // 继续枚举
        }, reinterpret_cast<LPARAM>(this));

        return gameWindow;
    }

public:
    AutoClicker() {
        // 查找游戏窗口
        gameWindow = FindGameWindow();
        if (gameWindow) {
            std::cout << "Game window found!" << std::endl;
        } else {
            std::cout << "Game window not found!" << std::endl;
        }
    }

    bool IsGameActive() {
        // 检查游戏窗口是否存在且为活动窗口
        return (gameWindow != NULL && gameWindow == GetForegroundWindow());
    }

    void SimulateClick() {
        // 只在游戏窗口激活时执行点击
        if (!IsGameActive()) return;

        // 发送鼠标点击消息到游戏窗口
        PostMessage(gameWindow, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(clickPos.x, clickPos.y));
        Sleep(10);
        PostMessage(gameWindow, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(clickPos.x, clickPos.y));
    }

    void StartClicking() {
        // 获取当前鼠标位置
        GetCursorPos(&clickPos);
        
        // 检查是否在屏幕右侧
        RECT screenRect;
        GetWindowRect(GetDesktopWindow(), &screenRect);
        if (clickPos.x < (screenRect.right - 160)) {
            SimulateClick();
            return;
        }

        isClicking = true;
        for (int i = 0; i < clickCount && isClicking; i++) {
            SimulateClick();
            Sleep(clickInterval);
        }
        isClicking = false;
    }

    void StopClicking() {
        isClicking = false;
    }

    // 调整点击次数
    void AdjustClickCount(bool increase) {
        if (increase) {
            clickCount = (clickCount < 30) ? clickCount + 1 : 1;
        } else {
            clickCount = (clickCount > 1) ? clickCount - 1 : 30;
        }
        std::cout << "Click count: " << clickCount << std::endl;
    }
};

// 热键处理函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static AutoClicker clicker;
    
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN) {
            // 检查是否按下Q或W键停止点击
            if (kbStruct->vkCode == 'Q' || kbStruct->vkCode == 'W') {
                clicker.StopClicking();
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 修改鼠标处理函数
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static AutoClicker clicker;
    
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* msStruct = (MSLLHOOKSTRUCT*)lParam;
        
        // 只在游戏窗口激活时处理点击
        if (clicker.IsGameActive()) {
            if (wParam == WM_LBUTTONDOWN && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                clicker.StartClicking();
            }
            else if (wParam == WM_MOUSEWHEEL && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                int wheelDelta = GET_WHEEL_DELTA_WPARAM(msStruct->mouseData);
                clicker.AdjustClickCount(wheelDelta > 0);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
      
    

// ... 原有代码保持不变 ...

// 在main函数开始添加控制台窗口
int main() {
    // 创建控制台窗口以显示点击次数
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);

    // ... 原有的main函数代码 ...

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
