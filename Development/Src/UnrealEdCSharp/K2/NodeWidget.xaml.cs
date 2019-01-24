/*
 *	NodeWidget.xaml.cs : Widget representing a node in the visual graph
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
	/// Interaction logic for NodeWidget.xaml
	/// </summary>
	public partial class NodeWidget : UserControl
	{
		// Main K2 tool window that owns this widget
		public	K2Canvas	mCanvas;

		// Pointer to the node object that this widget represents
		public	K2UINode	mNode;

		// When dragging this node, what is the offset from mouse 'grab' location to the widget origin
		Point				mDragOffset;

		public NodeWidget()
		{
			this.InitializeComponent();
		}

		public NodeWidget(K2Canvas InCanvas, K2UINode InNode) 
			: this()
		{
			mCanvas = InCanvas;
			mNode = InNode;

			this.MouseLeftButtonDown += NodeWidget_MouseLeftButtonDown;
			this.MouseLeftButtonUp += NodeWidget_MouseLeftButtonUp;
			this.MouseMove += NodeWidget_MouseMove;

			NodeDelete.Click += NodeDelete_Click;
			NodeBreakAllLinks.Click += NodeBreakAllLinks_Click;
		}

		// Handler for left clicking on node
		void NodeWidget_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
                Point MousePos = Mouse.GetPosition(mCanvas);

                mDragOffset.X = MousePos.X - Canvas.GetLeft(this);
                mDragOffset.Y = MousePos.Y - Canvas.GetTop(this);

                this.CaptureMouse();

                mCanvas.mEditor.SelectNode(mNode);

                e.Handled = true;
            }

		// Handler for releasing left mouse
		void NodeWidget_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
                this.ReleaseMouseCapture();
                e.Handled = true;
            }

		// Handler for moving mouse
		void NodeWidget_MouseMove(object sender, MouseEventArgs e)
		{
			// If captured, we are dragging, update location
			if(this.IsMouseCaptured)
            {
                Point MousePos = Mouse.GetPosition(mCanvas);

                Point NewWidgetPos = new Point();
                NewWidgetPos.X = MousePos.X - mDragOffset.X;
                NewWidgetPos.Y = MousePos.Y - mDragOffset.Y;

                mCanvas.mEditor.mBackend.MoveNode(mNode.mName, NewWidgetPos);

                mCanvas.mEditor.UpdateGraph();

                e.Handled = true;
            }
		}

		public NodeOutput FindOutputWidgetFor(K2UIOutput Output)
		{
			foreach( UIElement Ele in OutStackPanel.Children )
			{
				NodeOutput OutputWidget = Ele as NodeOutput;
				if(OutputWidget.mOutput == Output)
				{
					return OutputWidget;
				}
			}

			return null;
		}

		public NodeInput FindInputWidgetFor(K2UIInput Input)
		{
			foreach(UIElement Ele in InStackPanel.Children)
			{
				NodeInput InputWidget = Ele as NodeInput;
				if(InputWidget.mInput == Input)
				{
					return InputWidget;
				}
			}

			return null;
		}

		// When clicking delete in context menu
		void NodeDelete_Click(object sender, RoutedEventArgs e)
		{
			// TODO
			mCanvas.mEditor.mBackend.DeleteNode(mNode.mName);

			mCanvas.mEditor.UpdateGraph();

			e.Handled = true;
		}

		// When clicking 'break all links' in the context menu
		void NodeBreakAllLinks_Click(object sender, RoutedEventArgs e)
		{
			mCanvas.mEditor.mBackend.BreakAllConnections(mNode.mName);

			mCanvas.mEditor.UpdateGraph();

			e.Handled = true;
		}
	}
}