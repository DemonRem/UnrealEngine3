/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
namespace UnSetup
{
	partial class InstallOptions
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
			this.InstallLocationTextbox = new System.Windows.Forms.TextBox();
			this.ChooseInstallLocationButton = new System.Windows.Forms.Button();
			this.InstallOptionsTitleLabel = new System.Windows.Forms.Label();
			this.ChooseInstallLocationBrowser = new System.Windows.Forms.FolderBrowserDialog();
			this.InstallCancelButton = new System.Windows.Forms.Button();
			this.InstallButton = new System.Windows.Forms.Button();
			this.InstallLocationGroupBox = new System.Windows.Forms.GroupBox();
			this.InstallOptionsHeaderLine = new System.Windows.Forms.Label();
			this.InstallFooterLine = new System.Windows.Forms.Label();
			this.InstallOptionsFooterLineLabel = new System.Windows.Forms.Label();
			this.EmailGroupBox = new System.Windows.Forms.GroupBox();
			this.PrivacyPolicyTextBox = new System.Windows.Forms.RichTextBox();
			this.InvalidEmailLabel = new System.Windows.Forms.Label();
			this.EmailTextBox = new System.Windows.Forms.TextBox();
			this.EmailLabel = new System.Windows.Forms.Label();
			this.OptionalEmailLabel = new System.Windows.Forms.Label();
			this.InstallLocationGroupBox.SuspendLayout();
			this.EmailGroupBox.SuspendLayout();
			this.SuspendLayout();
			// 
			// InstallLocationTextbox
			// 
			this.InstallLocationTextbox.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.InstallLocationTextbox.Location = new System.Drawing.Point( 11, 23 );
			this.InstallLocationTextbox.Margin = new System.Windows.Forms.Padding( 8, 4, 4, 4 );
			this.InstallLocationTextbox.Name = "InstallLocationTextbox";
			this.InstallLocationTextbox.Size = new System.Drawing.Size( 654, 23 );
			this.InstallLocationTextbox.TabIndex = 0;
			// 
			// ChooseInstallLocationButton
			// 
			this.ChooseInstallLocationButton.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.ChooseInstallLocationButton.Location = new System.Drawing.Point( 673, 23 );
			this.ChooseInstallLocationButton.Margin = new System.Windows.Forms.Padding( 4, 4, 8, 4 );
			this.ChooseInstallLocationButton.Name = "ChooseInstallLocationButton";
			this.ChooseInstallLocationButton.Size = new System.Drawing.Size( 80, 23 );
			this.ChooseInstallLocationButton.TabIndex = 1;
			this.ChooseInstallLocationButton.Text = "Browse...";
			this.ChooseInstallLocationButton.UseVisualStyleBackColor = true;
			this.ChooseInstallLocationButton.MouseClick += new System.Windows.Forms.MouseEventHandler( this.ChooseInstallLocationClick );
			// 
			// InstallOptionsTitleLabel
			// 
			this.InstallOptionsTitleLabel.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallOptionsTitleLabel.BackColor = System.Drawing.Color.White;
			this.InstallOptionsTitleLabel.Font = new System.Drawing.Font( "Tahoma", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.InstallOptionsTitleLabel.Image = global::UnSetup.Properties.Resources.BannerImage;
			this.InstallOptionsTitleLabel.Location = new System.Drawing.Point( -3, 0 );
			this.InstallOptionsTitleLabel.Name = "InstallOptionsTitleLabel";
			this.InstallOptionsTitleLabel.Size = new System.Drawing.Size( 800, 68 );
			this.InstallOptionsTitleLabel.TabIndex = 9;
			this.InstallOptionsTitleLabel.Text = "Title";
			this.InstallOptionsTitleLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// ChooseInstallLocationBrowser
			// 
			this.ChooseInstallLocationBrowser.Description = "Where do you wish to install the Unreal Development Kit?";
			this.ChooseInstallLocationBrowser.RootFolder = System.Environment.SpecialFolder.MyComputer;
			// 
			// InstallCancelButton
			// 
			this.InstallCancelButton.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.InstallCancelButton.Location = new System.Drawing.Point( 682, 285 );
			this.InstallCancelButton.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.InstallCancelButton.Name = "InstallCancelButton";
			this.InstallCancelButton.Size = new System.Drawing.Size( 100, 32 );
			this.InstallCancelButton.TabIndex = 4;
			this.InstallCancelButton.Text = "Cancel";
			this.InstallCancelButton.UseVisualStyleBackColor = true;
			this.InstallCancelButton.Click += new System.EventHandler( this.CancelButtonClick );
			// 
			// InstallButton
			// 
			this.InstallButton.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallButton.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.InstallButton.Location = new System.Drawing.Point( 576, 285 );
			this.InstallButton.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.InstallButton.Name = "InstallButton";
			this.InstallButton.Size = new System.Drawing.Size( 100, 32 );
			this.InstallButton.TabIndex = 3;
			this.InstallButton.Text = "Install";
			this.InstallButton.UseVisualStyleBackColor = true;
			this.InstallButton.Click += new System.EventHandler( this.InstallButtonClick );
			// 
			// InstallLocationGroupBox
			// 
			this.InstallLocationGroupBox.Controls.Add( this.InstallLocationTextbox );
			this.InstallLocationGroupBox.Controls.Add( this.ChooseInstallLocationButton );
			this.InstallLocationGroupBox.Location = new System.Drawing.Point( 18, 87 );
			this.InstallLocationGroupBox.Name = "InstallLocationGroupBox";
			this.InstallLocationGroupBox.Size = new System.Drawing.Size( 764, 61 );
			this.InstallLocationGroupBox.TabIndex = 0;
			this.InstallLocationGroupBox.TabStop = false;
			this.InstallLocationGroupBox.Text = "Install Location";
			// 
			// InstallOptionsHeaderLine
			// 
			this.InstallOptionsHeaderLine.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallOptionsHeaderLine.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.InstallOptionsHeaderLine.Location = new System.Drawing.Point( -3, 68 );
			this.InstallOptionsHeaderLine.Name = "InstallOptionsHeaderLine";
			this.InstallOptionsHeaderLine.Size = new System.Drawing.Size( 800, 2 );
			this.InstallOptionsHeaderLine.TabIndex = 23;
			// 
			// InstallFooterLine
			// 
			this.InstallFooterLine.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallFooterLine.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.InstallFooterLine.Location = new System.Drawing.Point( -3, 276 );
			this.InstallFooterLine.Margin = new System.Windows.Forms.Padding( 3 );
			this.InstallFooterLine.Name = "InstallFooterLine";
			this.InstallFooterLine.Size = new System.Drawing.Size( 800, 2 );
			this.InstallFooterLine.TabIndex = 24;
			// 
			// InstallOptionsFooterLineLabel
			// 
			this.InstallOptionsFooterLineLabel.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.InstallOptionsFooterLineLabel.AutoSize = true;
			this.InstallOptionsFooterLineLabel.Enabled = false;
			this.InstallOptionsFooterLineLabel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.InstallOptionsFooterLineLabel.Location = new System.Drawing.Point( 12, 268 );
			this.InstallOptionsFooterLineLabel.Name = "InstallOptionsFooterLineLabel";
			this.InstallOptionsFooterLineLabel.Size = new System.Drawing.Size( 87, 16 );
			this.InstallOptionsFooterLineLabel.TabIndex = 25;
			this.InstallOptionsFooterLineLabel.Text = " UDK-2009-09";
			// 
			// EmailGroupBox
			// 
			this.EmailGroupBox.Controls.Add( this.PrivacyPolicyTextBox );
			this.EmailGroupBox.Controls.Add( this.InvalidEmailLabel );
			this.EmailGroupBox.Controls.Add( this.EmailTextBox );
			this.EmailGroupBox.Controls.Add( this.EmailLabel );
			this.EmailGroupBox.Controls.Add( this.OptionalEmailLabel );
			this.EmailGroupBox.Location = new System.Drawing.Point( 18, 162 );
			this.EmailGroupBox.Name = "EmailGroupBox";
			this.EmailGroupBox.Size = new System.Drawing.Size( 764, 100 );
			this.EmailGroupBox.TabIndex = 2;
			this.EmailGroupBox.TabStop = false;
			// 
			// PrivacyPolicyTextBox
			// 
			this.PrivacyPolicyTextBox.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.PrivacyPolicyTextBox.BackColor = System.Drawing.SystemColors.Control;
			this.PrivacyPolicyTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.PrivacyPolicyTextBox.Font = new System.Drawing.Font( "Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.PrivacyPolicyTextBox.Location = new System.Drawing.Point( 35, 73 );
			this.PrivacyPolicyTextBox.Name = "PrivacyPolicyTextBox";
			this.PrivacyPolicyTextBox.ReadOnly = true;
			this.PrivacyPolicyTextBox.Size = new System.Drawing.Size( 676, 22 );
			this.PrivacyPolicyTextBox.TabIndex = 27;
			this.PrivacyPolicyTextBox.TabStop = false;
			this.PrivacyPolicyTextBox.Text = "Read our Privacy Policy: http://udk.com/privacy";
			this.PrivacyPolicyTextBox.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler( this.PrivacyLinkClicked );
			// 
			// InvalidEmailLabel
			// 
			this.InvalidEmailLabel.AutoSize = true;
			this.InvalidEmailLabel.ForeColor = System.Drawing.Color.Firebrick;
			this.InvalidEmailLabel.Image = global::UnSetup.Properties.Resources.red_arrow;
			this.InvalidEmailLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.InvalidEmailLabel.Location = new System.Drawing.Point( 434, 47 );
			this.InvalidEmailLabel.Name = "InvalidEmailLabel";
			this.InvalidEmailLabel.Size = new System.Drawing.Size( 141, 16 );
			this.InvalidEmailLabel.TabIndex = 28;
			this.InvalidEmailLabel.Text = "   Invalid email address";
			// 
			// EmailTextBox
			// 
			this.EmailTextBox.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.EmailTextBox.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.EmailTextBox.Location = new System.Drawing.Point( 138, 43 );
			this.EmailTextBox.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.EmailTextBox.Name = "EmailTextBox";
			this.EmailTextBox.Size = new System.Drawing.Size( 290, 23 );
			this.EmailTextBox.TabIndex = 28;
			this.EmailTextBox.TextChanged += new System.EventHandler( this.EmailAddressTextChanged );
			// 
			// EmailLabel
			// 
			this.EmailLabel.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.EmailLabel.AutoSize = true;
			this.EmailLabel.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.EmailLabel.Location = new System.Drawing.Point( 32, 46 );
			this.EmailLabel.Name = "EmailLabel";
			this.EmailLabel.Size = new System.Drawing.Size( 89, 16 );
			this.EmailLabel.TabIndex = 28;
			this.EmailLabel.Text = "Email Address";
			// 
			// OptionalEmailLabel
			// 
			this.OptionalEmailLabel.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.OptionalEmailLabel.Location = new System.Drawing.Point( 8, 17 );
			this.OptionalEmailLabel.Name = "OptionalEmailLabel";
			this.OptionalEmailLabel.Size = new System.Drawing.Size( 577, 22 );
			this.OptionalEmailLabel.TabIndex = 29;
			this.OptionalEmailLabel.Text = "[Optional] Sign up for UDK updates and news.";
			// 
			// InstallOptions
			// 
			this.AcceptButton = this.InstallButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF( 7F, 16F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.InstallCancelButton;
			this.ClientSize = new System.Drawing.Size( 794, 330 );
			this.Controls.Add( this.InstallOptionsFooterLineLabel );
			this.Controls.Add( this.InstallFooterLine );
			this.Controls.Add( this.InstallOptionsHeaderLine );
			this.Controls.Add( this.InstallButton );
			this.Controls.Add( this.InstallCancelButton );
			this.Controls.Add( this.InstallOptionsTitleLabel );
			this.Controls.Add( this.InstallLocationGroupBox );
			this.Controls.Add( this.EmailGroupBox );
			this.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Icon = global::UnSetup.Properties.Resources.UDKIcon;
			this.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "InstallOptions";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Title";
			this.Load += new System.EventHandler( this.OnLoad );
			this.InstallLocationGroupBox.ResumeLayout( false );
			this.InstallLocationGroupBox.PerformLayout();
			this.EmailGroupBox.ResumeLayout( false );
			this.EmailGroupBox.PerformLayout();
			this.ResumeLayout( false );
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox InstallLocationTextbox;
		private System.Windows.Forms.Button ChooseInstallLocationButton;
		private System.Windows.Forms.Label InstallOptionsTitleLabel;
		private System.Windows.Forms.FolderBrowserDialog ChooseInstallLocationBrowser;
		private System.Windows.Forms.Button InstallCancelButton;
		private System.Windows.Forms.Button InstallButton;
		private System.Windows.Forms.GroupBox InstallLocationGroupBox;
		private System.Windows.Forms.Label InstallOptionsHeaderLine;
		private System.Windows.Forms.Label InstallFooterLine;
		private System.Windows.Forms.Label InstallOptionsFooterLineLabel;
		private System.Windows.Forms.GroupBox EmailGroupBox;
		private System.Windows.Forms.RichTextBox PrivacyPolicyTextBox;
		private System.Windows.Forms.Label InvalidEmailLabel;
		private System.Windows.Forms.TextBox EmailTextBox;
		private System.Windows.Forms.Label EmailLabel;
		private System.Windows.Forms.Label OptionalEmailLabel;
	}
}