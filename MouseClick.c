#include <windows.h>
#include <stdio.h>
#include <time.h>

#define CLICK_COUNT 10  // 点击次数
#define CLICK_INTERVAL 20  // 点击间隔 (毫秒)

// 用于模拟鼠标点击
void PerformClick() {
    // 鼠标左键按下
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    // 鼠标左键松开
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

// 显示当前时间的提示框
void ShowTooltip() {
    // 获取当前时间
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "红警xb提示：现在是 %Y年%m月%d日，%A，%H:%M", timeinfo);

    // 显示提示框
    MessageBoxA(NULL, buffer, "提示", MB_OK);
}

// 主程序
int main() {
    int clickCount = CLICK_COUNT;
    int isClicking = 0;

    while (1) {
        // 如果按下 F7 键
        if (GetAsyncKeyState(VK_F7) & 0x8000) {
      
    
    
      
    
            // 显示时间提示框
            ShowTooltip();
            Sleep(3000); // 等待 3 秒避免频繁弹窗
        }

        // 如果按下 Alt+F7 键
        if ((GetAsyncKeyState(VK_F7) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            // 退出程序
            break;
        }

        // 如果按住 Shift 键并点击鼠标左键
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            // 开始模拟点击
            if (!isClicking) {
                isClicking = 1;
                for (int i = 0; i < clickCount; i++) {
                    PerformClick();
                    Sleep(CLICK_INTERVAL); // 点击间隔
                }
                isClicking = 0;
            }
        }

        // 小睡一会，避免CPU占用过高
        Sleep(10);
    }

    return 0;
}
