namespace PackageCAB
{
	partial class SettingsDialog
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

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.SettingsPropertyGrid = new System.Windows.Forms.PropertyGrid();
			this.OKUIButton = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// SettingsPropertyGrid
			// 
			this.SettingsPropertyGrid.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom )
						| System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.SettingsPropertyGrid.Location = new System.Drawing.Point( 0, 0 );
			this.SettingsPropertyGrid.Name = "SettingsPropertyGrid";
			this.SettingsPropertyGrid.Size = new System.Drawing.Size( 691, 244 );
			this.SettingsPropertyGrid.TabIndex = 0;
			// 
			// OKUIButton
			// 
			this.OKUIButton.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right ) ) );
			this.OKUIButton.Location = new System.Drawing.Point( 605, 258 );
			this.OKUIButton.Name = "OKUIButton";
			this.OKUIButton.Size = new System.Drawing.Size( 75, 23 );
			this.OKUIButton.TabIndex = 2;
			this.OKUIButton.Text = "OK";
			this.OKUIButton.UseVisualStyleBackColor = true;
			this.OKUIButton.Click += new System.EventHandler( this.UIOKButtonClick );
			// 
			// SettingsDialog
			// 
			this.AcceptButton = this.OKUIButton;
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
			this.ClientSize = new System.Drawing.Size( 692, 293 );
			this.Controls.Add( this.OKUIButton );
			this.Controls.Add( this.SettingsPropertyGrid );
			this.Name = "SettingsDialog";
			this.Text = "PackageCAB Settings";
			this.ResumeLayout( false );

		}

		#endregion

		public System.Windows.Forms.PropertyGrid SettingsPropertyGrid;
		private System.Windows.Forms.Button OKUIButton;


	}
}