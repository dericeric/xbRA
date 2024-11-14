using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace RedAlertClicker
{
    class Program
    {
        internal static int clickCount = 1;
        internal static int maxClickCount = 30;
        internal static bool clickingActive = false;
        internal static int clicksRemaining = 0;
        internal static bool stopClicking = false;
        internal static int clickInterval = 100; // 可根据需要调整
        internal static System.Windows.Forms.Timer clickTimer;

        [DllImport("user32.dll")]
        private static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, UIntPtr dwExtraInfo);
        private const uint MOUSEEVENTF_LEFTDOWN = 0x02;
        private const uint MOUSEEVENTF_LEFTUP = 0x04;

        static void Main(string[] args)
        {
            clickTimer = new System.Windows.Forms.Timer { Interval = clickInterval };
            clickTimer.Tick += DoFixedPositionClick;

            Application.Run(new KeyboardHookListener());
        }

        internal static void DoFixedPositionClick(object sender, EventArgs e)
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

        internal static void PerformClick(int x, int y)
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
        protected override void OnMouseWheel(MouseEventArgs e)
        {
            if (Control.ModifierKeys == Keys.Shift)
            {
                if (e.Delta > 0) // Wheel Up
                {
                    Program.AdjustClickCount(true);
                }
                else if (e.Delta < 0) // Wheel Down
                {
                    Program.AdjustClickCount(false);
                }
            }
            base.OnMouseWheel(e);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left && Control.ModifierKeys == Keys.Shift)
            {
                if (Cursor.Position.X > Screen.PrimaryScreen.Bounds.Width - 160 && !Program.clickingActive)
                {
                    Program.clicksRemaining = Program.clickCount;
                    Program.clickingActive = true;
                    Program.stopClicking = false;
                    Program.clickTimer.Start();
                }
                else
                {
                    Program.PerformClick(Cursor.Position.X, Cursor.Position.Y);
                }
            }
            base.OnMouseDown(e);
        }
    }
}
