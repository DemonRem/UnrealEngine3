namespace PackageCRX
{
	partial class PackageCRX
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
			this.SettingsButton = new System.Windows.Forms.Button();
			this.PackageCRXButton = new System.Windows.Forms.Button();
			this.LabelItem1 = new System.Windows.Forms.Label();
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
			this.MainTextBox.TabIndex = 2;
			this.MainTextBox.Text = "";
			// 
			// SettingsButton
			// 
			this.SettingsButton.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left ) ) );
			this.SettingsButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.SettingsButton.Location = new System.Drawing.Point( 25, 445 );
			this.SettingsButton.Name = "SettingsButton";
			this.SettingsButton.Size = new System.Drawing.Size( 117, 23 );
			this.SettingsButton.TabIndex = 15;
			this.SettingsButton.Text = "Settings";
			this.SettingsButton.UseVisualStyleBackColor = true;
			this.SettingsButton.Click += new System.EventHandler( this.SettingsButtonClick );
			// 
			// PackageCRXButton
			// 
			this.PackageCRXButton.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.PackageCRXButton.Location = new System.Drawing.Point( 25, 12 );
			this.PackageCRXButton.Name = "PackageCRXButton";
			this.PackageCRXButton.Size = new System.Drawing.Size( 117, 23 );
			this.PackageCRXButton.TabIndex = 16;
			this.PackageCRXButton.Text = "Package CRX";
			this.PackageCRXButton.UseVisualStyleBackColor = true;
			this.PackageCRXButton.Click += new System.EventHandler( this.PackageCRXButtonClick );
			// 
			// LabelItem1
			// 
			this.LabelItem1.AutoSize = true;
			this.LabelItem1.Location = new System.Drawing.Point( 2, 17 );
			this.LabelItem1.Name = "LabelItem1";
			this.LabelItem1.Size = new System.Drawing.Size( 16, 13 );
			this.LabelItem1.TabIndex = 18;
			this.LabelItem1.Text = "1.";
			// 
			// PackageCRX
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size( 1016, 481 );
			this.Controls.Add( this.LabelItem1 );
			this.Controls.Add( this.PackageCRXButton );
			this.Controls.Add( this.SettingsButton );
			this.Controls.Add( this.MainTextBox );
			this.Name = "PackageCRX";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "Package CRX - A helper application to package plugins for Google Chrome";
			this.FormClosed += new System.Windows.Forms.FormClosedEventHandler( this.PackageCRXFormClosed );
			this.ResumeLayout( false );
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.RichTextBox MainTextBox;
		private System.Windows.Forms.Button SettingsButton;
		private System.Windows.Forms.Button PackageCRXButton;
		private System.Windows.Forms.Label LabelItem1;
	}
}

