/*
 *	NodeInput.xaml.cs : Input connector widget
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
	/// Interaction logic for NodeInput.xaml
	/// </summary>
	public partial class NodeInput : UserControl
	{
		// Node widget that owns this node
		public NodeWidget	mOwningWidget;

		// Node widget that owns this node
		public K2UIInput		mInput;


		public NodeInput()
		{
			this.InitializeComponent();

			InputConnector.AllowDrop = true;
			InputConnector.Drop += Connector_Drop;

            InputConnector.MouseDown += Connector_MouseDown;
            InputConnector.MouseUp += Connector_MouseUp;
            InputConnector.MouseMove += Connector_MouseMove;

			InputTextBlock.MouseDown += InputText_MouseDown;

			InputTextBox.KeyDown += InputText_KeyDown;
			InputTextBox.LostFocus += InputText_LostFocus;
		}

        public NodeInput(NodeWidget InNodeWidget, K2UIInput InInput)
            : this()
        {
            mOwningWidget = InNodeWidget;
            mInput = InInput;

            ToolTip = InInput.mConnTooptip;

            InputConnector.Fill = new SolidColorBrush(InInput.GetColor());
        }

		void InputText_LostFocus(object sender, RoutedEventArgs e)
		{
			CompleteTextBoxEntry();
			e.Handled = true;
		}

		void InputText_MouseDown(object sender, MouseButtonEventArgs e)
		{
			if (mInput.mbEditable)
			{
				InputTextBox.Visibility = Visibility.Visible;
				InputTextBlock.Visibility = Visibility.Collapsed;

				InputTextBox.Text = mInput.mValueString;

				Keyboard.Focus(InputTextBox);
				InputTextBox.SelectAll();

				e.Handled = true;
			}
		}

		void CompleteTextBoxEntry()
		{
			InputTextBox.Visibility = Visibility.Collapsed;
			InputTextBlock.Visibility = Visibility.Visible;

			mOwningWidget.mCanvas.mEditor.mBackend.SetInputDefaultValue(mOwningWidget.mNode.mName, mInput.mConnName, InputTextBox.Text);
			
			mOwningWidget.mCanvas.UpdateWidgets();
		}

		void InputText_KeyDown(object sender, KeyEventArgs e)
		{
			if(e.Key == Key.Enter)
			{
				CompleteTextBoxEntry();

				e.Handled = true;
			}
		}

		public void UpdateLabel()
		{
			InputTextBlock.Text = mInput.GetConnectorLabel();
		}

        void Connector_MouseDown(object sender, MouseButtonEventArgs e)
        {
            InputConnector.CaptureMouse();
            
            // Set the 'drag from connector' in the main tool
            mOwningWidget.mCanvas.mEditor.mDragFromConn = mInput;
            mOwningWidget.mCanvas.BeginConnectorDrag(CorrectCursorPos.MouseUtilities.CorrectGetPosition(mOwningWidget.mCanvas));

            e.Handled = true;
        }

        void Connector_MouseUp(object sender, MouseButtonEventArgs e)
        {
            InputConnector.ReleaseMouseCapture();
            e.Handled = true;
        }

        void Connector_MouseMove(object sender, MouseEventArgs e)
        {
            if (Mouse.Captured == InputConnector)
            {
                InputConnector.ReleaseMouseCapture();
                DragDrop.DoDragDrop(this, "test", DragDropEffects.All);
            }
        }

		void Connector_Drop(object sender, DragEventArgs e)
		{
			// Tell the main tool to make a connection between these two connectors
			mOwningWidget.mCanvas.mEditor.MakeConnection(mOwningWidget.mCanvas.mEditor.mDragFromConn as K2UIOutput, mInput);
			mOwningWidget.mCanvas.mEditor.mDragFromConn = null;
            mOwningWidget.mCanvas.EndConnectorDrag();

			e.Handled = true;
		}

	}
}