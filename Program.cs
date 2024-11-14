using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace RedAlertClicker
{
    class Program
    {
        static int clickCount = 1;
        static int maxClickCount = 30;
        static bool clickingActive = false;
        static int clicksRemaining = 0;
        static bool stopClicking = false;
        static int clickInterval = 100; // 可根据需要调整
        static Timer clickTimer;

        [DllImport("user32.dll")]
        private static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, UIntPtr dwExtraInfo);
        private const uint MOUSEEVENTF_LEFTDOWN = 0x02;
        private const uint MOUSEEVENTF_LEFTUP = 0x04;

        static void Main(string[] args)
        {
            clickTimer = new Timer { Interval = clickInterval };
            clickTimer.Tick += DoFixedPositionClick;

            Application.Run(new KeyboardHookListener());
        }

        private static void DoFixedPositionClick(object sender, EventArgs e)
        {
            if (clicksRemaining > 0 && !stopClicking)
            {
                PerformClick(Screen.PrimaryScreen.Bounds.Width - 160, Screen.PrimaryScreen.Bounds.Height / 2);
                clicksRemaining--;
            }
            else
            {
                clickingActive = false;
                clickTimer.Stop();
            }
        }

        private static void PerformClick(int x, int y)
        {
            Cursor.Position = new Point(x, y);
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, x, y, 0, UIntPtr.Zero);
        }

        public static void AdjustClickCount(bool increase)
        {
            if (increase)
            {
                clickCount = (clickCount < maxClickCount) ? clickCount + 1 : 1;
            }
            else
            {
                clickCount = (clickCount > 1) ? clickCount - 1 : maxClickCount;
            }
            ShowToolTip($"Click count: {clickCount}");
        }

        private static void ShowToolTip(string text)
        {
            ToolTip tooltip = new ToolTip();
            tooltip.Show(text, null, Screen.PrimaryScreen.Bounds.Width / 2, Screen.PrimaryScreen.Bounds.Height - 80, 1000);
        }
    }

    public class KeyboardHookListener : Form
    {
        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e.KeyCode == Keys.LButton && e.Shift && Cursor.Position.X > Screen.PrimaryScreen.Bounds.Width - 160)
            {
                if (!Program.clickingActive)
                {
                    Program.clicksRemaining = Program.clickCount;
                    Program.clickingActive = true;
                    Program.stopClicking = false;
                    Program.clickTimer.Start();
                }
            }
            else if (e.Delta > 0 && e.Shift) // Wheel Up
            {
                Program.AdjustClickCount(true);
            }
            else if (e.Delta < 0 && e.Shift) // Wheel Down
            {
                Program.AdjustClickCount(false);
            }
            else if (!Program.clickingActive)
            {
                // Simulate left click
                Program.PerformClick(Cursor.Position.X, Cursor.Position.Y);
            }
        }
    }
}
