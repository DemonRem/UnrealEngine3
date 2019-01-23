/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
namespace UnSetup
{
	partial class ManifestDialog
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
			this.OptionsPropertyGrid = new System.Windows.Forms.PropertyGrid();
			this.SuspendLayout();
			// 
			// OptionsPropertyGrid
			// 
			this.OptionsPropertyGrid.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom )
						| System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.OptionsPropertyGrid.Location = new System.Drawing.Point( 14, 15 );
			this.OptionsPropertyGrid.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.OptionsPropertyGrid.Name = "OptionsPropertyGrid";
			this.OptionsPropertyGrid.Size = new System.Drawing.Size( 546, 580 );
			this.OptionsPropertyGrid.TabIndex = 0;
			// 
			// ManifestDialog
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF( 7F, 16F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size( 574, 609 );
			this.Controls.Add( this.OptionsPropertyGrid );
			this.Font = new System.Drawing.Font( "Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.Icon = global::UnSetup.Properties.Resources.UDKIcon;
			this.Margin = new System.Windows.Forms.Padding( 3, 4, 3, 4 );
			this.MinimumSize = new System.Drawing.Size( 349, 240 );
			this.Name = "ManifestDialog";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Update Options";
			this.Load += new System.EventHandler( this.OnLoad );
			this.ResumeLayout( false );

		}

		#endregion

		private System.Windows.Forms.PropertyGrid OptionsPropertyGrid;
	}
}