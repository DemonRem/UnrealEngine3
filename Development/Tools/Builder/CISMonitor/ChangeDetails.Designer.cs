/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
namespace CISMonitor
{
    partial class ChangeDetails
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager( typeof( ChangeDetails ) );
			this.ChangeDetailsTextBox = new System.Windows.Forms.RichTextBox();
			this.SuspendLayout();
			// 
			// ChangeDetailsTextBox
			// 
			this.ChangeDetailsTextBox.Anchor = ( ( System.Windows.Forms.AnchorStyles )( ( ( ( System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom )
						| System.Windows.Forms.AnchorStyles.Left )
						| System.Windows.Forms.AnchorStyles.Right ) ) );
			this.ChangeDetailsTextBox.Font = new System.Drawing.Font( "Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
			this.ChangeDetailsTextBox.Location = new System.Drawing.Point( 1, 1 );
			this.ChangeDetailsTextBox.Name = "ChangeDetailsTextBox";
			this.ChangeDetailsTextBox.Size = new System.Drawing.Size( 952, 807 );
			this.ChangeDetailsTextBox.TabIndex = 0;
			this.ChangeDetailsTextBox.Text = "";
			// 
			// ChangeDetails
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size( 956, 810 );
			this.Controls.Add( this.ChangeDetailsTextBox );
			this.Icon = ( ( System.Drawing.Icon )( resources.GetObject( "$this.Icon" ) ) );
			this.Name = "ChangeDetails";
			this.Text = "Detailed description of potentially bad changelists";
			this.ResumeLayout( false );

        }

        #endregion

        private System.Windows.Forms.RichTextBox ChangeDetailsTextBox;
    }
}