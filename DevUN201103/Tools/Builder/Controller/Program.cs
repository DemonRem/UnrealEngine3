/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Deployment.Application;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using System.Windows.Forms;

namespace Controller
{
    public class Controller
    {
		[Flags]
		public enum ErrorModes : uint
		{
			SYSTEM_DEFAULT = 0x0000,
			SEM_FAILCRITICALERRORS = 0x0001,
			SEM_NOALIGNMENTFAULTEXCEPT = 0x0004,
			SEM_NOGPFAULTERRORBOX = 0x0002,
			SEM_NOOPENFILEERRORBOX = 0x8000
		}

		[DllImport( "kernel32.dll" )]
		static extern ErrorModes SetErrorMode( ErrorModes Mode );

		private static DateTime LastUpdateCheck = DateTime.UtcNow;

		static bool CheckForUpdates()
		{
			try
			{
				if( DateTime.UtcNow - LastUpdateCheck > new TimeSpan( 0, 1, 0 ) )
				{
					LastUpdateCheck = DateTime.UtcNow;

					if( ApplicationDeployment.IsNetworkDeployed )
					{
						ApplicationDeployment Current = ApplicationDeployment.CurrentDeployment;

						// If there are any updates available, install them now
						if( Current.CheckForUpdate() )
						{
							return ( Current.Update() );
						}
					}
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Deployment Exception: " + Ex.ToString() );
			}

			return ( false );
		}

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main( string[] Arguments )
        {
            Application.EnableVisualStyles();
		    Application.SetCompatibleTextRenderingDefault( false );

			Application.CurrentCulture = System.Globalization.CultureInfo.InvariantCulture;

			// Tell Windows to not pop up any crash dialogs (propagated to child processes)
			SetErrorMode( ErrorModes.SEM_FAILCRITICALERRORS );

			if( CheckForUpdates() )
			{
				Application.Restart();
			}

            // Create the window
            Main MainWindow = new Main();
            MainWindow.Init();

            while( MainWindow.Ticking )
            {
                Application.DoEvents();
				
				// Tick the main build loop
				if( !MainWindow.Run() )
				{
					// Check for updates if no build running
					if( CheckForUpdates() )
					{
						MainWindow.Ticking = false;
						MainWindow.Restart = true;
					}
				}

                // Yield a little time to the system
				System.Threading.Thread.Sleep( 100 );
            }

            MainWindow.Destroy();

            // Restart the process if requested
            if( MainWindow.Restart )
            {
				Application.Restart();
            }
        }
    }
}

