using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.ServiceProcess;

namespace PIB.FileWebServices
{
	class FileWebServices : ServiceBase
	{
		public const string TheServiceName = "PIBFileWebServices";

		GenericWebService PIBFileWebServices;

		protected override void OnStart( string[] args )
		{
			// run the webservice on port 1805 (IIS is grabbing all 80 requests)
			PIBFileWebServices = new GenericWebService( 1805, TheServiceName );

			// add all the service providers we know
			PIBFileWebServices.AddServiceProvider<GetFileListService>();

			// start the webservice
			PIBFileWebServices.Start();
		}

		protected override void OnStop()
		{
			PIBFileWebServices.Dispose();
			PIBFileWebServices = null;
		}

		public FileWebServices()
		{
			this.ServiceName = TheServiceName;
			this.AutoLog = true;
		}

		static void Main( string[] Args )
		{
			if( Args.Contains( "/manual" ) )
			{
				FileWebServices Svc = new FileWebServices();
				Svc.OnStart( Args );
				Console.WriteLine( "Press enter to quit..." );
				Console.ReadLine();
				Svc.OnStop();
			}
			else
			{
				ServiceBase.Run( new FileWebServices() );
			}
		}

		private void InitializeComponent()
		{
			// 
			// FileWebServices
			// 
			this.CanStop = false;
			this.ServiceName = "PIBFileListWebService";
		}
	}
}
