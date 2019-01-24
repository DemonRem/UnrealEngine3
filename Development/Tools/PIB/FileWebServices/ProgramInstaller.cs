using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;

namespace PIB.FileWebServices
{
	[RunInstaller( true )]
	public partial class ProgramInstaller : Installer
	{
		public ProgramInstaller()
		{
			InitializeComponent();
			PIBFileWebServicesInstaller.ServiceName = FileWebServices.TheServiceName;
		}
	}
}
