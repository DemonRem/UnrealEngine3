namespace Controller
{
    partial class Main
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.TextBox_Log = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // TextBox_Log
            // 
            this.TextBox_Log.Location = new System.Drawing.Point( 0, 0 );
            this.TextBox_Log.Name = "TextBox_Log";
            this.TextBox_Log.Size = new System.Drawing.Size( 897, 421 );
            this.TextBox_Log.TabIndex = 0;
            this.TextBox_Log.Text = "";
            this.TextBox_Log.WordWrap = false;
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF( 7F, 14F );
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size( 1432, 478 );
            this.Controls.Add( this.TextBox_Log );
            this.Font = new System.Drawing.Font( "Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ( ( byte )( 0 ) ) );
            this.MinimumSize = new System.Drawing.Size( 320, 240 );
            this.Name = "Main";
            this.Text = "Build Controller";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler( this.Main_FormClosed );
            this.SizeChanged += new System.EventHandler( this.MainWindow_SizeChanged );
            this.ResumeLayout( false );

        }

        #endregion

        private System.Windows.Forms.RichTextBox TextBox_Log;


    }
}

