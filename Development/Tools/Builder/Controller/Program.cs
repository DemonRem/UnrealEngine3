using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;

namespace Controller
{
    public class Controller
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault( false );

            // Move to run from the root folder
            string CWD = Environment.CurrentDirectory;
            if( !CWD.ToLower().EndsWith( "\\binaries" ) )
            {
                MessageBox.Show( "Controller must be run from the \'Binaries\' folder!", "Controller Fatal Error", MessageBoxButtons.OK, MessageBoxIcon.Error );
                return;
            }
            Environment.CurrentDirectory = CWD.Substring( 0, CWD.Length - "\\Binaries".Length );

            // Create the window
            Main MainWindow = new Main();
            MainWindow.Init();

            while( MainWindow.Ticking )
            {
                Application.DoEvents();
                MainWindow.Run();

                // Yield a little time to the system
                System.Threading.Thread.Sleep( 100 );
            }

            MainWindow.Destroy();
        }
    }
}

