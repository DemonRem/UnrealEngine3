/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Deployment.Application;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace Monitor
{
	static class Monitor
	{
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
			catch( Exception )
			{
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

			if( CheckForUpdates() )
			{
				Application.Restart();
			}

			// Create the window
			Main MainWindow = new Main();
			MainWindow.Init();

			while( MainWindow.Ticking )
			{
				// Process the application message loop
				Application.DoEvents();
				MainWindow.Run();

				if( CheckForUpdates() )
				{
					MainWindow.Ticking = false;
					MainWindow.Restart = true;
				}

				// Yield a little time to the system
				Thread.Sleep( 100 );
			}

			MainWindow.Destroy();

			// Restart the process if it's been requested
			if( MainWindow.Restart )
			{
				Application.Restart();
			}
		}
	}
}