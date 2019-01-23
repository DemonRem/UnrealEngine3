namespace PIB.FileWebServices
{
	partial class ProgramInstaller
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary> 
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose( bool disposing )
		{
			if( disposing && ( components != null ) )
			{
				components.Dispose();
			}
			base.Dispose( disposing );
		}

		#region Component Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.PIBFileWebServicesProcInstaller = new System.ServiceProcess.ServiceProcessInstaller();
			this.PIBFileWebServicesInstaller = new System.ServiceProcess.ServiceInstaller();
			// 
			// PIBFileWebServicesProcInstaller
			// 
			this.PIBFileWebServicesProcInstaller.Password = null;
			this.PIBFileWebServicesProcInstaller.Username = null;
			// 
			// PIBFileWebServicesInstaller
			// 
			this.PIBFileWebServicesInstaller.Description = "The web services required to support Play in Browser.";
			this.PIBFileWebServicesInstaller.DisplayName = "Play in Browser Web Services";
			this.PIBFileWebServicesInstaller.ServiceName = "PIBFileWebServices";
			this.PIBFileWebServicesInstaller.StartType = System.ServiceProcess.ServiceStartMode.Automatic;
			// 
			// ProgramInstaller
			// 
			this.Installers.AddRange( new System.Configuration.Install.Installer[] {
            this.PIBFileWebServicesProcInstaller,
            this.PIBFileWebServicesInstaller} );

		}

		#endregion

		private System.ServiceProcess.ServiceProcessInstaller PIBFileWebServicesProcInstaller;
		private System.ServiceProcess.ServiceInstaller PIBFileWebServicesInstaller;
	}
}