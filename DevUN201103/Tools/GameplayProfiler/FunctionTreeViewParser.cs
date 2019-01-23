using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using System.Text;

namespace GameplayProfiler
{
	class FunctionTreeViewParser
	{
		/**
		 * Payload for each node in the tree.	
		 */
		public class NodePayload
		{
			/** Inclusive time. */
			public float InclusiveTime = 0;
			/** Number of calls. */
			public float CallCount = 0;
			/** Name of function. */
			public string FunctionName;

			/** Constructor, initializing all members. */
			public NodePayload( string InFunctionName )
			{
				FunctionName = InFunctionName;
			}

			/**
			 * @return	Time string.
			 */
			private string GetTimeString()
			{
				return String.Format("{0:F2}", InclusiveTime).PadLeft(7) + " ";
			}

			/**
			 * Returns the display string for the associated node. It is a mix of time and description.
			 * 
			 * @return	Display string for associated node in "Inclusive Time    Description" format
			 */
			public string GetDisplayString()
			{
				return GetTimeString() + CallCount.ToString().PadLeft(6) + "  " + FunctionName;
			}
		}


		/**
		 * Parses a frame of the passed in stream into the function tree view taking into account display thresholds.
		 */
		public static void Parse(TreeView FunctionTreeView, ProfilerStream ProfilerStream, int FrameIndex, bool bOnlyParseSingleFrame, float TimeThreshold)
		{
			FunctionTreeView.BeginUpdate();
			FunctionTreeView.Nodes.Clear();
			
			// Create temporary root node so we always have a parent. We'll simply remove it as a post process step.
			var RootNode = new TreeNode( "TempRoot" );
			var CurrentNode = RootNode;
			var ActiveObjects = new Stack<TokenObject>();

			// Iterate over tokens starting at passed in frame. We either iterate till the frame or end of stream
			// marker based on passed in option.
			bool bStopParsing = false;
			for (int TokenIndex = ProfilerStream.Frames[FrameIndex].StartIndex; true; TokenIndex++)
			{
				var Token = ProfilerStream.Tokens[TokenIndex];
				var Object = Token as TokenObject;
				switch (Token.TokenType)
				{
					case ETokenTypes.Function:
						CurrentNode = AddFunction( CurrentNode, (TokenFunction) Object );
						ActiveObjects.Push(Object);
						break;
					case ETokenTypes.Actor:
					case ETokenTypes.Component:
						ActiveObjects.Push(Object);
						break;
					case ETokenTypes.EndOfScope:
						var CurrentObject = ActiveObjects.Pop();
						var Function = CurrentObject as TokenFunction;
						if( Function != null )
						{
							CurrentNode = CurrentNode.Parent;
						}
						break;
					case ETokenTypes.Frame:
						if( bOnlyParseSingleFrame )
						{
							bStopParsing = true;
						}
						break;
					case ETokenTypes.EndOfStream:
						bStopParsing = true;
						break;
					default:
						throw new InvalidDataException();
				}

				if (bStopParsing)
				{
					break;
				}
			}

			// Recursively updates node text with final time information.
			RecursivelyUpdateNodeText( RootNode );

			// Add root nodes's node to tree.
			foreach( TreeNode Node in RootNode.Nodes )
			{
				FunctionTreeView.Nodes.Add( Node );
			}

			FunctionTreeView.TreeViewNodeSorter = new NodeTimeSorter();
			FunctionTreeView.EndUpdate();
		}

		/**
		 * Adds the passed in function to the passed in node or updates if already a child.
		 */
		private static TreeNode AddFunction( TreeNode ParentNode, TokenFunction Function )
		{
			string FunctionName = Function.GetFunctionName();
			TreeNode FunctionNode = null;
			NodePayload FunctionPayload = null;
			
			// Look whether parent already has a node for this function.
			foreach( TreeNode ChildNode in ParentNode.Nodes )
			{
				var Payload = (NodePayload) ChildNode.Tag;
				if( Payload.FunctionName == FunctionName )
				{
					FunctionNode = ChildNode;
					FunctionPayload = Payload;
					break;
				}
			}

			// Create node if parent doesn't already have one.
			if( FunctionNode == null )
			{
				FunctionNode = new TreeNode(FunctionName);
				FunctionPayload = new NodePayload( FunctionName );
				FunctionNode.Tag = FunctionPayload;
				ParentNode.Nodes.Add(FunctionNode);
			}

			// Update information.
			FunctionPayload.InclusiveTime += Function.InclusiveTime;
			FunctionPayload.CallCount++;

			return FunctionNode;
		}

		private static void RecursivelyUpdateNodeText( TreeNode ParentNode )
		{
			foreach( TreeNode Node in ParentNode.Nodes )
			{
				RecursivelyUpdateNodeText( Node );
				Node.Text = ((NodePayload) Node.Tag).GetDisplayString();
			}
		}

		/**
		 * Node sorter that implements the IComparer interface. Nodes are sorted by time in descending order.
		 */ 
		public class NodeTimeSorter : System.Collections.IComparer
		{
			public int Compare(object ObjectA, object ObjectB)
			{
				NodePayload PayloadA = (NodePayload)((TreeNode)ObjectA).Tag;
				NodePayload PayloadB = (NodePayload)((TreeNode)ObjectB).Tag;;
				// Sort by size, descending.
				return Math.Sign( PayloadB.InclusiveTime - PayloadA.InclusiveTime );
			}
		}
	}
}
