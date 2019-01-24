/*
 *	K2Editor.xaml.cs : Main tool window
 *	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */ 

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

using System.Runtime.InteropServices;
namespace CorrectCursorPos
{
    public static class MouseUtilities
    {
        public static Point CorrectGetPosition(Visual relativeTo)
        {
            Win32Point w32Mouse = new Win32Point();
            GetCursorPos(ref w32Mouse);
            return relativeTo.PointFromScreen(new Point(w32Mouse.X, w32Mouse.Y));
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Win32Point
        {
            public Int32 X;
            public Int32 Y;
        };

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetCursorPos(ref Win32Point pt);
    }
}

namespace Wpf_K2
{
	/// <summary>
	/// Interaction logic for K2Editor.xaml
	/// </summary>
	public partial class K2Editor : UserControl
	{
		// Used to record mouse location when bringing up context menu
		Point										mCanvasContext_MousePos;

		// Connector that we are starting a connection from
		public K2UIConnector						mDragFromConn;

		public List<K2UINode>						mSelectedNodes = new List<K2UINode>();

		public IK2BackendInterface					mBackend;

		// MODEL ////////////////////////////////////////////////////////////////////////

		public K2UINodeGraph						mNodeGraph;

		//////////////////////////////////////////////////////////////////////////

		class UE3MenuItem : MenuItem
		{
			public UE3MenuItem() : base()
			{
				this.Style = (Style)this.FindResource(typeof(MenuItem));
			}
		}

		// Class for 'new node' menu item
		class NewNodeMenuItem : UE3MenuItem
		{
			public K2NewNodeOption mOption = null;			
		}

		public K2Editor()
		{
			this.InitializeComponent();
			TheCanvas.mEditor = this;

			/////// CANVAS CONTEXT MENU
			ContextMenu CanvasContextMenu = new ContextMenu();

			CanvasContextMenu.Opened += CanvasContext_Opened;
			TheCanvas.ContextMenu = CanvasContextMenu;

			TheCanvas.MouseLeftButtonDown += Canvas_MouseLeftButtonDown;

            TheCanvas.DragOver += new DragEventHandler(Canvas_DragOver);
            TheCanvas.Drop += Canvas_Drop;
		}

		public void SetBackend(IK2BackendInterface InBackend)
		{
			if(InBackend != null)
			{
				mBackend = InBackend;
				mNodeGraph = mBackend.GetUIGraph();
			}
		}

		// Canvas left click
		void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
            if (Mouse.Captured == null)
            {
                SelectNode(null);
            }
		}

        void Canvas_DragOver(object sender, DragEventArgs e)
        {
            TheCanvas.UpdateConnectorDrag(CorrectCursorPos.MouseUtilities.CorrectGetPosition(TheCanvas));
        }

		void Canvas_Drop(object sender, DragEventArgs e)
		{
			TheCanvas.EndConnectorDrag();
            TheCanvas.ContextMenu.IsOpen = true;
			e.Handled = true;
		}

        void Canvas_QueryContinueDrag(object sender, QueryContinueDragEventArgs e)
        {

            TheCanvas.UpdateConnectorDrag(Mouse.GetPosition(null));
        }

		// Canvas right-click menu opened
		void CanvasContext_Opened(object sender, RoutedEventArgs e)
		{
			mCanvasContext_MousePos = Mouse.GetPosition(TheCanvas);

			ContextMenu CanvasContextMenu = (ContextMenu)sender;
			CanvasContextMenu.Items.Clear(); // Clear existing menu items

			// The class name we want options from
			String UseClassName = "";

			// If we want to filter nodes to only those with a specific input type
			K2UIConnectorType FilterType = K2UIConnectorType.CT_Exec;

			// DROP WIRE CONTEXT
			K2UIOutput FromOutput = (mDragFromConn as K2UIOutput);
			bool bFilterByInputType = (FromOutput != null);

			K2UIInput FromInput = (mDragFromConn as K2UIInput);
			bool bFilterByOutputType = (FromInput != null);

			if (bFilterByOutputType || bFilterByInputType)
			{
				FilterType = mDragFromConn.mConnType;

				// If we dragged from an object connector, show only functions you can call on that object
				if ((FromOutput != null) && (FromOutput.mConnType == K2UIConnectorType.CT_Object))
				{
					UseClassName = FromOutput.mObjClassName;
					FilterType = K2UIConnectorType.CT_Exec;
				}
			}

			mDragFromConn = null;

			List<K2NewNodeOption> NodeOptions = mBackend.GetNewNodeOptions(UseClassName);

			// TODO: Filter based on input

			// Set of category menus used to sort out entries
			List<MenuItem> CategoryMenus = new List<MenuItem>();

			// Iterate over each option we are given
			foreach (K2NewNodeOption Option in NodeOptions)
			{
				if(bFilterByInputType && !mBackend.NodeHasInputOfType(Option, FilterType))
				{
					continue;
				}

				if (bFilterByOutputType && !mBackend.NodeHasOutputOfType(Option, FilterType))
				{
					continue;
				}

				// This is the menu to which we will add the entry
				ItemsControl CategoryMenu = null;

				// If a category is specified, we need to find the menu
				if (Option.mCategoryName != "")
				{
					// Look for an existing category menu with that name
					foreach (MenuItem TestItem in CategoryMenus)
					{
						if ((String)TestItem.Header == Option.mCategoryName)
						{
							CategoryMenu = TestItem;
							break;
						}
					}

					// Did not find one, make a new one
					if (CategoryMenu == null)
					{
						MenuItem NewMenu = new MenuItem();
						NewMenu.Header = Option.mCategoryName;

						CategoryMenu = NewMenu;
						CanvasContextMenu.Items.Add(NewMenu);
						CategoryMenus.Add(NewMenu); // Add to list
					}
				}
				else
				{
					CategoryMenu = CanvasContextMenu;
				}

				NewNodeMenuItem Item = new NewNodeMenuItem();
				Item.Header = Option.mNodeName;
				Item.mOption = Option;
				Item.ToolTip = Option.mComment;
				Item.Click += NewNodeMenuItem_Click;
				CategoryMenu.Items.Add(Item);
			}
		}	

		public void UpdateGraph()
		{
			TheCanvas.UpdateWidgets();
		}

		// Canvas 'New Call Function' chosen
		void NewNodeMenuItem_Click(object sender, RoutedEventArgs e)
		{
			NewNodeMenuItem Item = (NewNodeMenuItem)sender;

			mBackend.CreateNewNode(Item.mOption, mCanvasContext_MousePos);

			UpdateGraph();
		}

		// Make a connection from one connector to another
		public void MakeConnection(K2UIOutput FromConn, K2UIInput ToConn)
		{
			mBackend.MakeConnection(FromConn.mOwningNode.mName, FromConn.mConnName, ToConn.mOwningNode.mName, ToConn.mConnName);

			UpdateGraph();
		}

		// Chose 'delete' from the menu
		internal void DeleteNode(K2UINode Node)
		{
			mBackend.DeleteNode(Node.mName);

			UpdateGraph();
		}

		public void SelectNode(K2UINode SelectedNode)
		{
			mSelectedNodes.Clear();

			if(SelectedNode != null)
			{
				mSelectedNodes.Add(SelectedNode);
			}

			// Make a list of new selected node names
			List<string> SelNodeNames = new List<string>();
			foreach(K2UINode Node in mSelectedNodes)
			{
				if(Node != null)
				{
					SelNodeNames.Add(Node.mName);
				}
			}

			// Tell our owner about selection changes
			mBackend.OnSelectionChanged(SelNodeNames);

			// Update UI to show
			UpdateGraph();		
		}
	}
}