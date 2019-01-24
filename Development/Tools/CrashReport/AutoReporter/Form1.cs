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

namespace AutoReporter
{
    public partial class Form1 : Form
    {
        public string crashDesc;
        public string summary;

        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            crashDesc = textBox1.Text;
            summary = textBox2.Text;
            Close();
        }

        public void SetCallStack(string CallStack)
        {
            textBox3.Text = CallStack;

            int NewLabelHeight = (CallStack.Length / 80) * 15;
			NewLabelHeight = System.Math.Max( textBox3.Height, NewLabelHeight );
			NewLabelHeight = System.Math.Min( textBox3.Height + 200, NewLabelHeight );
			int LabelHeightIncrease = NewLabelHeight - textBox3.Height;
            textBox3.Height += LabelHeightIncrease;

            Height += LabelHeightIncrease;
        }

        public void SetServiceError(string ErrorMsg)
        {
            textBox1.Visible = false;
            label4.Visible = false;
            textBox2.Visible = false;

            label1.Text = "An error occurred connecting to the Report Service: \n" + ErrorMsg;
            label1.Height += 20;
        }


		public void ShowBalloon( string Msg, int BalloonTimeInMs )
		{
			AppErrorNotifyInfo.BalloonTipText = Msg;
			AppErrorNotifyInfo.Visible = true;
			AppErrorNotifyInfo.ShowBalloonTip( BalloonTimeInMs );
		}


		public void HideBalloon()
		{
			AppErrorNotifyInfo.Visible = false;
		}
	}
}