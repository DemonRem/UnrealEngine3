using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace PackageCAB
{
	static class Program
	{
		static int ProcessArguments( string[] Arguments )
		{
			foreach( string Argument in Arguments )
			{
				if( Argument.ToLower().StartsWith( "-" ) )
				{
					return ( 1 );
				}
				if( Argument.ToLower().StartsWith( "/" ) )
				{
					return ( 2 );
				}
			}

			return ( 2 );
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault( false );

			Application.CurrentCulture = System.Globalization.CultureInfo.InvariantCulture;

			// Create the window
			PackageCAB MainWindow = new PackageCAB();
			MainWindow.Init();

			while( MainWindow.Ticking )
			{
				// Tick the OS
				Application.DoEvents();

				// Tick the main build loop
				MainWindow.Run();

				// Yield a little time to the system
				System.Threading.Thread.Sleep( 50 );
			}

			MainWindow.Destroy();
		}
	}
}
