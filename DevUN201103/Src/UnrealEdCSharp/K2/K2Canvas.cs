/*
 *	K2Canvas.cs : Canvas class used to draw widgets for graph
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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Wpf_K2
{
	public class K2Canvas : Canvas
	{
		public K2Editor								mEditor;


		private class Connection
		{
			public NodeOutput	Start;
			public NodeInput	End;

			public Path			ConnPath;

			public void	UpdatePathGeom(K2Canvas Canvas)
			{
				ConnPath.Data = CreatePathGeometry(Start, End, Canvas);
			}
		}

		// list of all created connection widgets
		private List<Connection>					mConnections			= new List<Connection>();

        private class FauxConnection
        {
            public Point        Start;
            public Path         DropPath;

            public void UpdatePathGeom( K2Canvas Canvas, Point MousePos )
            {
                DropPath.Data = CreatePathGeometry(Start, MousePos, Canvas);
            }
        }
        private FauxConnection mDropConn;

		// map from model data (K2UINode) to widget (NodeWidget)
		private Dictionary<K2UINode, NodeWidget> mNodeToWidgetMap = new Dictionary<K2UINode, NodeWidget>();

		static K2Canvas()
		{

		}

		public K2Canvas()
		{
			this.AllowDrop = true;
		}

		/**
		 * @param InStart The widget from which to draw the line
		 * @param InEnd The widget to which to draw the line
		 * @param InCanvas The canvas on which we will draw the line (used for computing relative positions); all coords need to be in InCanvas-space
		 */
		static private PathGeometry CreatePathGeometry(NodeOutput InStart, NodeInput InEnd, K2Canvas InCanvas)
		{
			PathFigure ConnectorCurve = new PathFigure();

			// Create the Connector Curve
			{
				Point CanvasScreenPosition = InCanvas.PointToScreen( new Point(0,0) );
				Vector CanvasScreenOffset = new Vector(CanvasScreenPosition.X, CanvasScreenPosition.Y);

				Point ConnectorStartPoint_ScreenSpace = InStart.PointToScreen(new Point(InStart.ActualWidth, InStart.ActualHeight * 0.5));
				Point ConnectorEndPoint_ScreenSpace = InEnd.PointToScreen(new Point(0, InEnd.ActualHeight * 0.5));

				Point ConnectorStartPoint_CanvasSpace = Point.Subtract(ConnectorStartPoint_ScreenSpace, CanvasScreenOffset);
				Point ConnectorEndPoint_CanvasSpace = Point.Subtract(ConnectorEndPoint_ScreenSpace, CanvasScreenOffset);

				ConnectorCurve.StartPoint = ConnectorStartPoint_CanvasSpace;

				double XSeparation = (ConnectorEndPoint_CanvasSpace.X - ConnectorStartPoint_CanvasSpace.X)*0.5;

				ConnectorCurve.Segments.Add(new BezierSegment(
					new Point(ConnectorEndPoint_CanvasSpace.X - XSeparation, ConnectorStartPoint_CanvasSpace.Y),
					new Point(ConnectorStartPoint_CanvasSpace.X + XSeparation, ConnectorEndPoint_CanvasSpace.Y),
					ConnectorEndPoint_CanvasSpace,
					true
				));
			}

			PathGeometry NewPathGeom = new PathGeometry();
			NewPathGeom.Figures.Add( ConnectorCurve );
			return NewPathGeom;
		}

        static public PathGeometry CreatePathGeometry( Point InStart, Point InMousePos, K2Canvas InCanvas )
        {
            PathFigure ConnectorCurve = new PathFigure();

            // Create the Connector Curve
            {
                Point CanvasScreenPosition = InCanvas.PointToScreen(new Point(0, 0));
                Vector CanvasScreenOffset = new Vector(CanvasScreenPosition.X, CanvasScreenPosition.Y);

                Point ConnectorStartPoint_ScreenSpace = InStart;
                Point ConnectorEndPoint_ScreenSpace = InMousePos;

                Point ConnectorStartPoint_CanvasSpace = Point.Subtract(ConnectorStartPoint_ScreenSpace, CanvasScreenOffset);
                Point ConnectorEndPoint_CanvasSpace = Point.Subtract(ConnectorEndPoint_ScreenSpace, CanvasScreenOffset);

                ConnectorCurve.StartPoint = ConnectorStartPoint_CanvasSpace;

                double XSeparation = (ConnectorEndPoint_CanvasSpace.X - ConnectorStartPoint_CanvasSpace.X) * 0.5;

                ConnectorCurve.Segments.Add(new BezierSegment(
                    new Point(ConnectorEndPoint_CanvasSpace.X - XSeparation, ConnectorStartPoint_CanvasSpace.Y),
                    new Point(ConnectorStartPoint_CanvasSpace.X + XSeparation, ConnectorEndPoint_CanvasSpace.Y),
                    ConnectorEndPoint_CanvasSpace,
                    true
                ));
            }

            PathGeometry NewPathGeom = new PathGeometry();
            NewPathGeom.Figures.Add(ConnectorCurve);
            return NewPathGeom;
        }

		protected override Size ArrangeOverride(Size arrangeBounds)
		{
			Size ArrangedSize = base.ArrangeOverride(arrangeBounds);

			AdjustConnectors();

			return ArrangedSize;
		}

		private void AdjustConnectors()
		{
			foreach( Connection SomeConnection in mConnections )
			{
				SomeConnection.UpdatePathGeom(this);
			}
		}


		class LinkMenuItem : MenuItem
		{
			public Connection	ClickedConnection;
		}


		private Connection CreateConnection( NodeOutput InStart, NodeInput InEnd )
		{
			Connection NewConnection = new Connection { Start = InStart, End = InEnd };

			Path NewPath = new Path();
			NewPath.Stroke = new SolidColorBrush(InStart.mOutput.GetColor());
			NewPath.StrokeThickness = 2;

			// Make context menu
			NewPath.ContextMenu = new ContextMenu();
			LinkMenuItem BreakLinkMenuItem = new LinkMenuItem();
			BreakLinkMenuItem.Header = "Break Link";
			BreakLinkMenuItem.ClickedConnection = NewConnection;
			BreakLinkMenuItem.Click += BreakLinkMenuItem_Click;
			NewPath.ContextMenu.Items.Add(BreakLinkMenuItem);

			NewConnection.ConnPath = NewPath;

			mConnections.Add(NewConnection);

			this.Children.Add(NewConnection.ConnPath);

			return NewConnection;
		}

        private FauxConnection CreateFauxConnection( Point InMousePos, Color InColor )
        {
            FauxConnection NewConn = new FauxConnection { Start = InMousePos };

            Path NewPath = new Path();
            NewPath.Stroke = new SolidColorBrush(InColor);
            NewPath.StrokeThickness = 2;

            NewConn.DropPath = NewPath;

            Children.Add(NewConn.DropPath);

            return NewConn;
        }

        public void BeginConnectorDrag( Point InMousePos )
        {
            if (mEditor.mDragFromConn != null && mDropConn == null)
            {
//              System.Console.WriteLine("BeginConnectorDrag "+InMousePos.ToString());

                mDropConn = CreateFauxConnection( InMousePos, mEditor.mDragFromConn.GetColor());
            }
        }
        public void UpdateConnectorDrag( Point InMousePos )
        {
            if (mDropConn != null)
            {
//              System.Console.WriteLine("UpdateConnectorDrag " + InMousePos.ToString());

                mDropConn.UpdatePathGeom(this, InMousePos);
            }
        }
        public void EndConnectorDrag()
        {
            if (mEditor.mDragFromConn == null && mDropConn != null)
            {
//              System.Console.WriteLine("EndConnectorDrag");

                Children.Remove(mDropConn.DropPath);
                mDropConn = null;
            }
        }
        public bool IsDragConnectorActive()
        {
            return (mDropConn != null);
        }

		void BreakLinkMenuItem_Click(object sender, RoutedEventArgs e)
		{
			LinkMenuItem Item = sender as LinkMenuItem;

			K2UIInput BreakInput = Item.ClickedConnection.End.mInput;
			mEditor.mBackend.BreakConnection(BreakInput.mOwningNode.mName, BreakInput.mConnName);

			mEditor.UpdateGraph();
		}	

		// Called to update properties of this widget to match the underlying data object
		private void UpdateWidgetForNode(K2UINode Node, NodeWidget NodeWidget)
		{
			// Set the location to where you clicked
			Canvas.SetLeft(NodeWidget, Node.mLocation.X);
			Canvas.SetTop(NodeWidget, Node.mLocation.Y);

			// See if we are selected
			bool bSelected = mEditor.mSelectedNodes.Contains(Node);

			// Yellow if it's selected, otherwise use value from node
			if(bSelected)
			{
				NodeWidget.MainBox.BorderBrush = Brushes.Yellow;
			}
			else
			{
				NodeWidget.MainBox.BorderBrush = new SolidColorBrush( Node.mBorderColor );
			}

            NodeWidget.TitleText.Text = Node.mDisplayName;

			foreach (UIElement Ele in NodeWidget.InStackPanel.Children)
			{
				NodeInput InputWidget = Ele as NodeInput;
				InputWidget.UpdateLabel();
			}
		}

		// Util to create a widget for a supplied node object
		private NodeWidget CreateWidgetForNode(K2UINode Node)
		{
			// Create widget
			NodeWidget NewNodeWidget = new NodeWidget(this, Node);

			// Assign title
			NewNodeWidget.TitleText.Text = Node.mDisplayName;
			// and tooltip
			NewNodeWidget.ToolTip = Node.mNodeTooltip;

			// iterate over inputs to make widget for each
			foreach (K2UIInput Input in Node.mInputs)
			{
				NodeInput InputWidget = new NodeInput(NewNodeWidget, Input);
				NewNodeWidget.InStackPanel.Children.Add(InputWidget);
			}

			// iterate over outputs to make widgets
			foreach (K2UIOutput Output in Node.mOutputs)
			{
				NodeOutput OutputWidget = new NodeOutput(NewNodeWidget, Output);
				NewNodeWidget.OutStackPanel.Children.Add(OutputWidget);
			}

			// Update other params
			UpdateWidgetForNode(Node, NewNodeWidget);

			// Add to map
			mNodeToWidgetMap.Add(Node, NewNodeWidget);

			// Add to the canvas
			Children.Add(NewNodeWidget);

			return NewNodeWidget;
		}

		// Function to update all UI to match the underlying model
		public void UpdateWidgets()
		{
			// First make list of all widgets, so we can look for ones we need to remove
			List<NodeWidget> WidgetsToDelete = new List<NodeWidget>();
			foreach(KeyValuePair<K2UINode, NodeWidget>Pair in mNodeToWidgetMap)
			{
				WidgetsToDelete.Add(Pair.Value);
			}

			// Also copy list of connections, so we can 
			List<Connection> ConnectionsToDelete = new List<Connection>(mConnections);

			// First pass, create/destroy any widgets that are needed
			foreach(K2UINode Node in mEditor.mNodeGraph.AllNodes)
			{
				NodeWidget NodeWidget;
			
				if( mNodeToWidgetMap.TryGetValue(Node, out NodeWidget) )
				{
					// We don't want to delete this one
					WidgetsToDelete.Remove(NodeWidget);

					// Update stuff
					UpdateWidgetForNode(Node, NodeWidget);
				}
				else
				{
					// No widget for this node - create one
					NodeWidget = CreateWidgetForNode(Node);
				}
			}

			// Second pass, create/destroy any connector widgets
			foreach(K2UINode Node in mEditor.mNodeGraph.AllNodes)
			{
				NodeWidget NodeWidget;
				mNodeToWidgetMap.TryGetValue(Node, out NodeWidget);
			

				// Iterate over each connection 
				foreach(K2UIOutput FromOutput in Node.mOutputs)
				{
					foreach (K2UIInput ToInput in FromOutput.mToInputs)
					{
						// If this is actually a connection
						if (ToInput != null)
						{
							// See if a widget exists for this connection
							Connection ConnWidget = mConnections.FindLast(C =>
							{
								return ((C.Start.mOutput == FromOutput) && (C.End.mInput == ToInput));
							});

							// If it does, just update its geom
							if (ConnWidget != null)
							{
								ConnWidget.UpdatePathGeom(this);

								// and remember we don't want to delete it
								ConnectionsToDelete.Remove(ConnWidget);
							}
							// If it does not, time to make a new connection
							else
							{
								// This is slightly tricky as we need to find which WIDGETS to connect

								// Output widget it easy, we have the 'from' node widget already
								NodeOutput OutputWidget = NodeWidget.FindOutputWidgetFor(FromOutput);

								// Find the 'to' node widget from the input object
								K2UINode ToNode = ToInput.mOwningNode;
								NodeWidget ToNodeWidget;
								bool bSuccess = mNodeToWidgetMap.TryGetValue(ToNode, out ToNodeWidget);

								// Then find the input widget
								NodeInput InputWidget = ToNodeWidget.FindInputWidgetFor(ToInput);

								// Finally create the connection
								ConnWidget = CreateConnection(OutputWidget, InputWidget);
							}
						}
					}
				}
			}

			// Now clean up any widgets that we don't need any more
			foreach(NodeWidget NodeWidget in WidgetsToDelete)
			{
				mNodeToWidgetMap.Remove(NodeWidget.mNode);
				Children.Remove(NodeWidget);
			}

			// Clean up all Connections in ConnectionsToDelete
			foreach(Connection BreakConn in ConnectionsToDelete)
			{
				Children.Remove(BreakConn.ConnPath);
				mConnections.Remove(BreakConn);
			}
		}
	}
}
