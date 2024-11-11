using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Threading;
using System.Drawing;

namespace AutoClicker
{
    public partial class AutoClickerForm : Form
    {
        private int clickCount = 10;  // 默认点击次数
        private int clickInterval = 20; // 点击间隔
        private Timer clickTimer;
        private bool isClicking = false;

        public AutoClickerForm()
        {
            InitializeComponent();
            this.Text = "Auto Clicker";
            this.WindowState = FormWindowState.Minimized;
            this.ShowInTaskbar = false;

            clickTimer = new Timer { Interval = clickInterval };
            clickTimer.Tick += (s, e) => PerformClick();

            this.KeyPreview = true;
            this.KeyDown += OnKeyDown;
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F7 && e.Modifiers == Keys.None)
            {
                ShowTooltip();
            }
            else if (e.KeyCode == Keys.F7 && e.Modifiers == Keys.Alt)
            {
                Application.Exit();
            }
            else if (e.KeyCode == Keys.LButton && Control.ModifierKeys == Keys.Shift)
            {
                if (!isClicking)
                {
                    StartClicking();
                }
            }
        }

        private void StartClicking()
        {
            isClicking = true;
            clickTimer.Start();
        }

        private void PerformClick()
        {
            if (clickCount <= 0)
            {
      
    
    
      
    
                clickTimer.Stop();
                isClicking = false;
                return;
            }

            MouseSimulator.Click(MouseButtons.Left);
            clickCount--;
        }

        private void ShowTooltip()
        {
            string message = $"红警xb提示：现在是{DateTime.Now:yyyy年MM月dd日，dddd，HH:mm}";
            ToolTip tooltip = new ToolTip();
            tooltip.Show(message, this, Cursor.Position, 3000);
        }
    }

    public static class MouseSimulator
    {
        [DllImport("user32.dll")]
        static extern void mouse_event(int flags, int dX, int dY, int buttons, int extraInfo);

        const int MOUSEEVENTF_LEFTDOWN = 0x2;
        const int MOUSEEVENTF_LEFTUP = 0x4;

        public static void Click(MouseButtons button)
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        }
    }
}
