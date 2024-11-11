using System;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using Gma.System.MouseKeyHook;
using log4net;

namespace MouseClickerApp
{
    public class MouseClicker : Form
    {
        public static readonly ILog Log = LogManager.GetLogger("MC.INFO");

        private IKeyboardMouseEvents KMEvents;
        private bool clicking = false;
        private bool shiftDown = false;
        private int clickCounts = 10; // Default click counts
        private Timer autoDetectTimer;
        private bool autoDetectEnabled = false;

        [DllImport("user32.dll", EntryPoint = "FindWindow", SetLastError = true)]
        public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        public MouseClicker()
        {
            this.Text = "Mouse Clicker";
            this.FormClosing += OnFormClosing;
            InitializeEventHandlers();
        }

        private void InitializeEventHandlers()
        {
            KMEvents = Hook.GlobalEvents();
            KMEvents.KeyDown += OnKeyDown;
            KMEvents.KeyUp += OnKeyUp;
            KMEvents.MouseDown += OnMouseClick;
            KMEvents.MouseWheel += OnMouseWheel;

            SetupAutoDetect();
        }

        private void OnFormClosing(object sender, FormClosingEventArgs e)
        {
            UnsubscribeEvents();
        }

        private void UnsubscribeEvents()
        {
            KMEvents.KeyDown -= OnKeyDown;
            KMEvents.KeyUp -= OnKeyUp;
            KMEvents.MouseDown -= OnMouseClick;
            KMEvents.MouseWheel -= OnMouseWheel;
            KMEvents.Dispose();
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F7)
            {
                ShowDateTimeTooltip();
            }
            else if (e.KeyCode == Keys.F7 && e.Alt)
            {
                Application.Exit();
            }
            else if (e.KeyCode == Keys.ShiftKey)
            {
                shiftDown = true;
            }
        }

        private void OnKeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.ShiftKey)
            {
                shiftDown = false;
            }
        }

        private void OnMouseClick(object sender, MouseEventArgs e)
        {
            if (shiftDown && e.Button == MouseButtons.Left && !clicking)
            {
                PerformFixedPositionClick();
            }
        }

        private void OnMouseWheel(object sender, MouseEventArgs e)
        {
            if (shiftDown)
            {
                clickCounts = Math.Max(1, clickCounts + (e.Delta > 0 ? 1 : -1));
                Log.Info($"Click count updated: {clickCounts}");
            }
        }

        private void PerformFixedPositionClick()
        {
            clicking = true;
            int initialX = Cursor.Position.X;
            int initialY = Cursor.Position.Y;

            Task.Run(() =>
            {
                for (int i = 0; i < clickCounts; i++)
                {
                    MouseSimulator.Click(MouseButtons.Left, initialX, initialY);
                    Thread.Sleep(100); // Adjust interval as needed
                }
                clicking = false;
            });
        }

        private void ShowDateTimeTooltip()
        {
            var now = DateTime.Now;
            var tooltipText = $"现在是 {now:yyyy年MM月dd日，dddd，HH:mm}";
            ToolTip tt = new ToolTip();
            tt.Show(tooltipText, this, 2000);
        }

        private void SetupAutoDetect()
        {
            autoDetectTimer = new Timer((state) =>
            {
                if (GameProcessExists() && !clicking)
                {
                    Log.Info("Game process detected, starting clicker");
                    PerformFixedPositionClick();
                }
                else if (!GameProcessExists() && clicking)
                {
                    clicking = false;
                }
            }, null, 1000, 1000); // Adjust interval as needed
        }

        private bool GameProcessExists()
        {
            foreach (Process process in Process.GetProcesses())
            {
                if (process.ProcessName.Contains("game_process_name")) // Replace with actual game process name
                {
                    return true;
                }
            }
            return false;
        }

        [STAThread]
        public static void Main()
        {
            Application.Run(new MouseClicker());
        }
    }

    public static class MouseSimulator
    {
        [DllImport("user32.dll")]
        private static extern void mouse_event(int dwFlags, int dx, int dy, int dwData, UIntPtr dwExtraInfo);

        private const int MOUSEEVENTF_LEFTDOWN = 0x0002;
        private const int MOUSEEVENTF_LEFTUP = 0x0004;

        public static void Click(MouseButtons button, int x, int y)
        {
            Cursor.Position = new Point(x, y);
            mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, UIntPtr.Zero);
            mouse_event(MOUSEEVENTF_LEFTUP, x, y, 0, UIntPtr.Zero);
        }
    }
}
