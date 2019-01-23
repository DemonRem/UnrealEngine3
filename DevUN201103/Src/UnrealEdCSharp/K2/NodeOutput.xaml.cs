/*
 *	NodeOutput.xaml.cs : Output connector widget
 *	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */ 


using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Wpf_K2
{
	/// <summary>
	/// Interaction logic for NodeOutput.xaml
	/// </summary>
	public partial class NodeOutput : UserControl
	{
		// Node widget that owns this output
		public NodeWidget	mOwningWidget;

		// index of this output
		public	K2UIOutput	mOutput;

		public NodeOutput()
		{
			this.InitializeComponent();

            OutputConnector.AllowDrop = true;
            OutputConnector.Drop += Connector_Drop;

			OutputConnector.MouseDown += Connector_MouseDown;
			OutputConnector.MouseUp += Connector_MouseUp;
			OutputConnector.MouseMove += Connector_MouseMove;
		}

		public NodeOutput(NodeWidget InNodeWidget, K2UIOutput InOutput)
			: this()
		{
			mOwningWidget = InNodeWidget;
			mOutput = InOutput;

			ToolTip = InOutput.mConnTooptip;

			OutputConnector.Fill = new SolidColorBrush(InOutput.GetColor());

			OutputTextBlock.Text = InOutput.mConnName;
		}

        void Connector_MouseDown(object sender, MouseButtonEventArgs e)
		{
			OutputConnector.CaptureMouse();
            
            // Set the 'drag from connector' in the main tool
            mOwningWidget.mCanvas.mEditor.mDragFromConn = mOutput;
            mOwningWidget.mCanvas.BeginConnectorDrag(CorrectCursorPos.MouseUtilities.CorrectGetPosition(mOwningWidget.mCanvas));

            e.Handled = true;
		}

		void Connector_MouseUp(object sender, MouseButtonEventArgs e)
		{
			OutputConnector.ReleaseMouseCapture();
			e.Handled = true;
		}

		void Connector_MouseMove(object sender, MouseEventArgs e)
		{
			if (Mouse.Captured == OutputConnector)
			{
				OutputConnector.ReleaseMouseCapture();
				DragDrop.DoDragDrop(this, "test", DragDropEffects.All);
                e.Handled = true;
			}
		}

        void Connector_Drop(object sender, DragEventArgs e)
        {
            // Tell the main tool to make a connection between these two connectors
            mOwningWidget.mCanvas.mEditor.MakeConnection(mOutput, mOwningWidget.mCanvas.mEditor.mDragFromConn as K2UIInput);
            mOwningWidget.mCanvas.mEditor.mDragFromConn = null;
            mOwningWidget.mCanvas.EndConnectorDrag();
            
            e.Handled = true;
        }
	}
}