/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace MemoryProfiler2
{
	/**
	 * Class parsing snapshot into a tree from a given root node.
	 */
	public static class FCallGraphTreeViewParser
	{
		public static void ParseSnapshot( TreeView CallGraphTreeView, List<FCallStackAllocationInfo> CallStackList, MainWindow MainMProfWindow, bool bShouldSortBySize, int ParentFunctionIndex, string FilterText )
		{
			MainMProfWindow.UpdateStatus("Updating call graph for " + MainMProfWindow.CurrentFilename);

			CallGraphTreeView.BeginUpdate();

			// Clear out existing nodes and add two root nodes. One for regular call stacks and one for truncated ones.
			CallGraphTreeView.Nodes.Clear();
			TreeNode RegularNode = new TreeNode("Full Callstacks");
			TreeNode TruncatedNode = new TreeNode("Truncated Callstacks");
			CallGraphTreeView.Nodes.Add(RegularNode);
			CallGraphTreeView.Nodes.Add(TruncatedNode);

			// Iterate over all call graph paths and add them to the graph.
			foreach( FCallStackAllocationInfo AllocationInfo in CallStackList )
			{
				// Add this call graph to the tree view.
				FCallStack CallStack = FStreamInfo.GlobalInstance.CallStackArray[AllocationInfo.CallStackIndex];
				// Split the tree into full and truncated callstacks.
				TreeNode RootNode = CallStack.bIsTruncated ? TruncatedNode : RegularNode;
                // Don't bother with callstacks that don't have a contribution.
                if( ((AllocationInfo.Count != 0) || (AllocationInfo.Size != 0))
				// Apply filter based on text representation of address.
					&& CallStack.RunFilters( FilterText, MainMProfWindow.Options.ClassGroups, MainMProfWindow.FilterTypeSplitButton.Text == "Filter In" ) )
                {
                    // Add callstack to proper part of graph.
				    AddCallStackToGraph( RootNode, CallStack, AllocationInfo, ParentFunctionIndex );
                }
			}

			// Update the node text by prepending memory usage and allocation count.
			UpdateNodeText( RegularNode );
			UpdateNodeText( TruncatedNode );

			// Last but not least, set the node sorter property to sort nodes.
			if( bShouldSortBySize )
			{
				CallGraphTreeView.TreeViewNodeSorter = new FNodeSizeSorter();
			}
			else
			{
				CallGraphTreeView.TreeViewNodeSorter = new FNodeCountSorter();
			}

			CallGraphTreeView.EndUpdate();
		}

		private static void AddCallStackToGraph( TreeNode RootNode, FCallStack CallStack, FCallStackAllocationInfo AllocationInfo, int ParentFunctionIndex )
		{
			// Used to determine whether it is okay to add address to the graph. An index of -1 means we don't care about parenting.
			bool bAllowNodeUpdate = ParentFunctionIndex == -1;
			// Iterate over each address and add it to the tree view.
			TreeNode CurrentNode = RootNode;
			foreach( int AddressIndex in CallStack.AddressIndices )
			{
				// Filter based on function if wanted. This means we only include callstacks that contain the function
				// and we also reparent the callstack to start at the first occurence of the function.
				if( ParentFunctionIndex != -1 && !bAllowNodeUpdate )
				{
					 bAllowNodeUpdate = FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndex].FunctionIndex == ParentFunctionIndex;
				}
	
				if( bAllowNodeUpdate )
				{
					// Update the node for this address. The return value of the function will be the new parent node.
					CurrentNode = UpdateNodeAndPayload( CurrentNode, AddressIndex, AllocationInfo );
				}
			}
		}

		private static TreeNode UpdateNodeAndPayload( TreeNode ParentNode, int AddressIndex, FCallStackAllocationInfo AllocationInfo )
		{
			// Iterate over existing nodes to see whether there is a match.
			foreach( TreeNode Node in ParentNode.Nodes )
			{
				FNodePayload Payload = (FNodePayload) Node.Tag;
				// If there is a match, update the allocation size and return the current node.
				if( FStreamInfo.GlobalInstance.CallStackAddressArray[Payload.AddressIndex].FunctionIndex == FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndex].FunctionIndex )
				{
					Payload.AllocationSize += AllocationInfo.Size;
					Payload.AllocationCount += AllocationInfo.Count;
					// Return current node as parent for next iteration.
					return Node;
				}
			}

			// If we made it here it means that we need to add a new node.
			string NodeName = FStreamInfo.GlobalInstance.NameArray[FStreamInfo.GlobalInstance.CallStackAddressArray[AddressIndex].FunctionIndex];
			TreeNode NewNode = new TreeNode( NodeName );
			
			// Create payload for the node and associate it.
			FNodePayload NewPayload = new FNodePayload(AddressIndex,AllocationInfo.Size,AllocationInfo.Count);
			NewNode.Tag = NewPayload;
			
			// Add to parent node and return new node as subsequent parent.
			ParentNode.Nodes.Add( NewNode );
			return NewNode;
		}

		private static void UpdateNodeText( TreeNode RootNode )
		{
			// Compile a list of all nodes in the graph without recursion.
			List<TreeNode> TreeNodes = new List<TreeNode>();
			TreeNodes.Add(RootNode);
			int NodeIndex = 0;
			while( NodeIndex < TreeNodes.Count )
			{
				foreach( TreeNode Node in TreeNodes[NodeIndex].Nodes )
				{
					TreeNodes.Add( Node );
				}
				NodeIndex++;
			}

			// Iterate over all nodes and prepend size in KByte.
			foreach( TreeNode Node in TreeNodes )
			{
				// Some nodes like root node won't have a tag.
				if( Node.Tag != null )
				{
					FNodePayload Payload = Node.Tag as FNodePayload;
					Node.Text = (Payload.AllocationSize / 1024) + " KByte  " + Payload.AllocationCount + " Allocations  " + Node.Text;
				}
				// Count down work remaining.
				NodeIndex--;
			}
		}
	};

	/**
	 * TreeNode payload containing raw node stats about address and allocation size.
	 */
	public class FNodePayload
	{
		/** Index into stream info addresses array associated with node. */
		public int AddressIndex;
		/** Cummulative size for this point in the graph. */
		public long AllocationSize;
		/** Number of allocations at this point in the graph. */
		public int AllocationCount;

		/** Constructor, initializing all members with passed in values. */
		public FNodePayload( int InAddressIndex, long InAllocationSize, int InAllocationCount )
		{
			AddressIndex = InAddressIndex;
			AllocationSize = InAllocationSize;
			AllocationCount = InAllocationCount;
		}
	}

	/**
	 * Node sorter that implements the IComparer interface. Nodes are sorted by size.
	 */ 
	public class FNodeSizeSorter : System.Collections.IComparer
	{
	    public int Compare(object ObjectA, object ObjectB)
	    {
			// We sort by size, which requires payload.
			TreeNode NodeA = ObjectA as TreeNode;
			TreeNode NodeB = ObjectB as TreeNode;
			FNodePayload PayloadA = NodeA.Tag as FNodePayload;
			FNodePayload PayloadB = NodeB.Tag as FNodePayload;

			// Can only sort if there is payload.
			if( PayloadA != null && PayloadB != null )
			{
				// Sort by size, descending.
				return Math.Sign( PayloadB.AllocationSize - PayloadA.AllocationSize );
			}
			// Treat missing payload as unsorted
			else
			{
				return 0;
			}
		}
	};

	/**
	 * Node sorter that implements the IComparer interface. Nodes are sorted by count.
	 */ 
	public class FNodeCountSorter : System.Collections.IComparer
	{
	    public int Compare(object ObjectA, object ObjectB)
	    {
			// We sort by size, which requires payload.
			TreeNode NodeA = ObjectA as TreeNode;
			TreeNode NodeB = ObjectB as TreeNode;
			FNodePayload PayloadA = NodeA.Tag as FNodePayload;
			FNodePayload PayloadB = NodeB.Tag as FNodePayload;

			// Can only sort if there is payload.
			if( PayloadA != null && PayloadB != null )
			{
				// Sort by size, descending.
				return Math.Sign( PayloadB.AllocationCount - PayloadA.AllocationCount );
			}
			// Treat missing payload as unsorted
			else
			{
				return 0;
			}
		}
	};
}