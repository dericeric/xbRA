#include <windows.h>
#include <iostream>

int clickCount = 10;          // 需要执行的点击次数
int clickInterval = 20;       // 每次点击之间的间隔（毫秒）
int clicksRemaining = 0;      // 剩余点击次数 
int clickX = 0;               // 点击的X坐标                              
int clickY = 0;               // 点击的Y坐标 
bool stopClicking = false;    // 控制是否停止点击的标志
bool clickingActive = false;  // 检查定时器是否已在运行

void SetTooltip(const std::string& message) {
    // 显示工具提示（替代 AutoHotkey 的 ToolTip 功能）
    std::cout << "Tooltip: " << message << std::endl;
}

void DoFixedPositionClick(HWND hwnd) {
    if (clicksRemaining <= 0 || stopClicking) {
        clickingActive = false; // 停止标志
        KillTimer(hwnd, 1);     // 停止定时器
        return;
    }

    // 模拟鼠标点击（ControlClick 替代）
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = clickX;
    input.mi.dy = clickY;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));

    clicksRemaining--;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            switch (wParam) {
            case VK_LBUTTON: // Shift + Left Click
                if (clickingActive == false) {
                    POINT pt;
                    GetCursorPos(&pt);
                    clickX = pt.x;
                    clickY = pt.y;
                    clicksRemaining = clickCount;
                    stopClicking = false;
                    clickingActive = true;
                    SetTimer(hwnd, 1, clickInterval, NULL);
                }
                break;

            case VK_UP: // Shift + WheelUp
                if (clickCount < 30) clickCount++;
                else clickCount = 1;
                SetTooltip("Click count: " + std::to_string(clickCount));
                break;

            case VK_DOWN: // Shift + WheelDown
                if (clickCount > 1) clickCount--;
                else clickCount = 30;
                SetTooltip("Click count: " + std::to_string(clickCount));
                break;
            }
        }
        break;

    case WM_TIMER:
        if (wParam == 1) {
            DoFixedPositionClick(hwnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main() {
    const char CLASS_NAME[] = "ClickerWindow";
    
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Auto Clicker", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, SW_HIDE);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
