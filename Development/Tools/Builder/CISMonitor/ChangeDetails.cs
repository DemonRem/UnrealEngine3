/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace CISMonitor
{
    public partial class ChangeDetails : Form
    {
        public ChangeDetails( SettableOptions Options, string[] Output )
        {
            InitializeComponent();
            ChangeDetailsTextBox.Focus();

            foreach( string Line in Output )
            {
                if( Line.StartsWith( "Changelist:" ) || Line.StartsWith( "'" ) )
                {
                    ChangeDetailsTextBox.SelectionLength = 0;
                    ChangeDetailsTextBox.SelectionFont = new Font( "Consolas", 12, FontStyle.Bold );
                }

                ChangeDetailsTextBox.AppendText( Line );

                if( Line.StartsWith( "Changelist:" ) || Line.StartsWith( "'" ) )
                {
                    ChangeDetailsTextBox.SelectionFont = new Font( "Consolas", 10, FontStyle.Regular );
                }
            }

            Show();
        }
    }
}