/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

namespace PackageXPI
{
	partial class PackageXPI
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
			this.MainTextBox = new System.Windows.Forms.RichTextBox();
			this.PackageXPIButton = new System.Windows.Forms.Button();
			this.GetCertificateButton = new System.Windows.Forms.Button();
			this.SignPackageButton = new System.Windows.Forms.Button();
			this.CreateDBButton = new System.Windows.Forms.Button();
			this.CreatePFXButton = new System.Windows.Forms.Button();
			this.FileBrowserDialog = new System.Windows.Forms.OpenFileDialog();
			this.ImportPFXButton = new System.Windows.Forms.Button();
			this.LabelItem1 = new System.Windows.Forms.Label();
			this.LabelItem2 = new System.Windows.Forms.Label();
			this.LabelItem3 = new System.Windows.Forms.Label();
			this.LabelItem4 = new System.Windows.Forms.Label();
			this.LabelItem5 = new System.Windows.Forms.Label();
			this.LabelItem6 = new System.Windows.Forms.Label();
			this.SettingsButton = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// MainTextBox
			// 
			this.MainTextBox.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom )
						| System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.MainTextBox.BackColor = System.Drawing.SystemColors.Window;
			this.MainTextBox.Font = new System.Drawing.Font( "Consolas", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.MainTextBox.Location = new System.Drawing.Point( 153, 9 );
			this.MainTextBox.Margin = new System.Windows.Forms.Padding( 0 );
			this.MainTextBox.Name = "MainTextBox";
			this.MainTextBox.ReadOnly = true;
			this.MainTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.Vertical;
			this.MainTextBox.Size = new System.Drawing.Size( 855, 459 );
			this.MainTextBox.TabIndex = 0;
			this.MainTextBox.Text = "";
			// 
			// PackageXPIButton
			// 
			this.PackageXPIButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.PackageXPIButton.Location = new System.Drawing.Point( 25, 157 );
			this.PackageXPIButton.Name = "PackageXPIButton";
			this.PackageXPIButton.Size = new System.Drawing.Size( 117, 23 );
			this.PackageXPIButton.TabIndex = 1;
			this.PackageXPIButton.Text = "Package XPI";
			this.PackageXPIButton.UseVisualStyleBackColor = true;
			this.PackageXPIButton.Click += new System.EventHandler( this.PackageXPIClick );
			// 
			// GetCertificateButton
			// 
			this.GetCertificateButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.GetCertificateButton.Location = new System.Drawing.Point( 25, 99 );
			this.GetCertificateButton.Name = "GetCertificateButton";
			this.GetCertificateButton.Size = new System.Drawing.Size( 117, 23 );
			this.GetCertificateButton.TabIndex = 2;
			this.GetCertificateButton.Text = "Get Certificate";
			this.GetCertificateButton.UseVisualStyleBackColor = true;
			this.GetCertificateButton.Click += new System.EventHandler( this.GetCertClick );
			// 
			// SignPackageButton
			// 
			this.SignPackageButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.SignPackageButton.Location = new System.Drawing.Point( 25, 128 );
			this.SignPackageButton.Name = "SignPackageButton";
			this.SignPackageButton.Size = new System.Drawing.Size( 117, 23 );
			this.SignPackageButton.TabIndex = 3;
			this.SignPackageButton.Text = "Sign Package";
			this.SignPackageButton.UseVisualStyleBackColor = true;
			this.SignPackageButton.Click += new System.EventHandler( this.SignPackageClick );
			// 
			// CreateDBButton
			// 
			this.CreateDBButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.CreateDBButton.Location = new System.Drawing.Point( 25, 12 );
			this.CreateDBButton.Name = "CreateDBButton";
			this.CreateDBButton.Size = new System.Drawing.Size( 117, 23 );
			this.CreateDBButton.TabIndex = 4;
			this.CreateDBButton.Text = "Create Cert DB";
			this.CreateDBButton.UseVisualStyleBackColor = true;
			this.CreateDBButton.Click += new System.EventHandler( this.CreateCertDBClick );
			// 
			// CreatePFXButton
			// 
			this.CreatePFXButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.CreatePFXButton.Location = new System.Drawing.Point( 25, 41 );
			this.CreatePFXButton.Name = "CreatePFXButton";
			this.CreatePFXButton.Size = new System.Drawing.Size( 117, 23 );
			this.CreatePFXButton.TabIndex = 5;
			this.CreatePFXButton.Text = "Create PFX";
			this.CreatePFXButton.UseVisualStyleBackColor = true;
			this.CreatePFXButton.Click += new System.EventHandler( this.CreatePFXClick );
			// 
			// FileBrowserDialog
			// 
			this.FileBrowserDialog.AddExtension = false;
			this.FileBrowserDialog.DefaultExt = "pvk";
			this.FileBrowserDialog.Filter = "Key Files|*.pvk;*.spc|All files|*.*";
			this.FileBrowserDialog.InitialDirectory = "\\\\epicgames.net\\root";
			this.FileBrowserDialog.SupportMultiDottedExtensions = true;
			this.FileBrowserDialog.Title = "Browse to location of private and public keys";
			// 
			// ImportPFXButton
			// 
			this.ImportPFXButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.ImportPFXButton.Location = new System.Drawing.Point( 25, 70 );
			this.ImportPFXButton.Name = "ImportPFXButton";
			this.ImportPFXButton.Size = new System.Drawing.Size( 117, 23 );
			this.ImportPFXButton.TabIndex = 6;
			this.ImportPFXButton.Text = "Import PFX";
			this.ImportPFXButton.UseVisualStyleBackColor = true;
			this.ImportPFXButton.Click += new System.EventHandler( this.ImportPFXClick );
			// 
			// LabelItem1
			// 
			this.LabelItem1.AutoSize = true;
			this.LabelItem1.Location = new System.Drawing.Point( 2, 17 );
			this.LabelItem1.Name = "LabelItem1";
			this.LabelItem1.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem1.TabIndex = 7;
			this.LabelItem1.Text = "1.";
			// 
			// LabelItem2
			// 
			this.LabelItem2.AutoSize = true;
			this.LabelItem2.Location = new System.Drawing.Point( 2, 46 );
			this.LabelItem2.Name = "LabelItem2";
			this.LabelItem2.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem2.TabIndex = 8;
			this.LabelItem2.Text = "2.";
			// 
			// LabelItem3
			// 
			this.LabelItem3.AutoSize = true;
			this.LabelItem3.Location = new System.Drawing.Point( 2, 75 );
			this.LabelItem3.Name = "LabelItem3";
			this.LabelItem3.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem3.TabIndex = 9;
			this.LabelItem3.Text = "3.";
			// 
			// LabelItem4
			// 
			this.LabelItem4.AutoSize = true;
			this.LabelItem4.Location = new System.Drawing.Point( 2, 104 );
			this.LabelItem4.Name = "LabelItem4";
			this.LabelItem4.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem4.TabIndex = 10;
			this.LabelItem4.Text = "4.";
			// 
			// LabelItem5
			// 
			this.LabelItem5.AutoSize = true;
			this.LabelItem5.Location = new System.Drawing.Point( 2, 133 );
			this.LabelItem5.Name = "LabelItem5";
			this.LabelItem5.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem5.TabIndex = 11;
			this.LabelItem5.Text = "5.";
			// 
			// LabelItem6
			// 
			this.LabelItem6.AutoSize = true;
			this.LabelItem6.Location = new System.Drawing.Point( 2, 162 );
			this.LabelItem6.Name = "LabelItem6";
			this.LabelItem6.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem6.TabIndex = 12;
			this.LabelItem6.Text = "6.";
			// 
			// SettingsButton
			// 
			this.SettingsButton.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left ) ) );
			this.SettingsButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.SettingsButton.Location = new System.Drawing.Point( 25, 436 );
			this.SettingsButton.Name = "SettingsButton";
			this.SettingsButton.Size = new System.Drawing.Size( 117, 23 );
			this.SettingsButton.TabIndex = 13;
			this.SettingsButton.Text = "Settings";
			this.SettingsButton.UseVisualStyleBackColor = true;
			this.SettingsButton.Click += new System.EventHandler( this.SettingsButtonClick );
			// 
			// PackageXPI
			// 
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
			this.ClientSize = new System.Drawing.Size( 1016, 481 );
			this.Controls.Add( this.SettingsButton );
			this.Controls.Add( this.LabelItem6 );
			this.Controls.Add( this.LabelItem5 );
			this.Controls.Add( this.LabelItem4 );
			this.Controls.Add( this.LabelItem3 );
			this.Controls.Add( this.LabelItem2 );
			this.Controls.Add( this.LabelItem1 );
			this.Controls.Add( this.ImportPFXButton );
			this.Controls.Add( this.CreatePFXButton );
			this.Controls.Add( this.CreateDBButton );
			this.Controls.Add( this.SignPackageButton );
			this.Controls.Add( this.GetCertificateButton );
			this.Controls.Add( this.PackageXPIButton );
			this.Controls.Add( this.MainTextBox );
			this.MinimumSize = new System.Drawing.Size( 400, 200 );
			this.Name = "PackageXPI";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "Package XPI - A helper application to package Netscape Plugins";
			this.FormClosed += new System.Windows.Forms.FormClosedEventHandler( this.PackageXPIFormClosed );
			this.ResumeLayout( false );
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.RichTextBox MainTextBox;
		private System.Windows.Forms.Button PackageXPIButton;
		private System.Windows.Forms.Button GetCertificateButton;
		private System.Windows.Forms.Button SignPackageButton;
		private System.Windows.Forms.Button CreateDBButton;
		private System.Windows.Forms.Button CreatePFXButton;
		private System.Windows.Forms.OpenFileDialog FileBrowserDialog;
		private System.Windows.Forms.Button ImportPFXButton;
		private System.Windows.Forms.Label LabelItem1;
		private System.Windows.Forms.Label LabelItem2;
		private System.Windows.Forms.Label LabelItem3;
		private System.Windows.Forms.Label LabelItem4;
		private System.Windows.Forms.Label LabelItem5;
		private System.Windows.Forms.Label LabelItem6;
		private System.Windows.Forms.Button SettingsButton;
	}
}

