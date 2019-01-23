//=============================================================================
//	BackendInterface.cs: Content browser backend interface
//	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
//=============================================================================


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
using System.Windows.Controls.Primitives;
using System.IO;
using System.Collections.ObjectModel;
using System.ComponentModel;
using UnrealEd;

namespace Wpf_K2
{

	public class K2UINodeGraph
	{
		// List of all current model nodes 
		public List<K2UINode> AllNodes = new List<K2UINode>();
	}

	public class K2NewNodeOption
	{
		public string mCategoryName;

		public string mNodeName;

		public string mClassName;

		public string mComment;

		public K2NewNodeOption(string CategoryName, string NodeName)
		{
			mCategoryName = CategoryName;
			mNodeName = NodeName;
		}
	}


	/**
     * An Interface to be implemented by the C++/CLI portion of the code.
     * This exists so that C# code can call C++/CLI code. 
     */
	public interface IK2BackendInterface
	{
		/** Return a pointer to the shared K2UINodeGraph object, that is the working C# view of the code graph */
		K2UINodeGraph GetUIGraph();

		/** Get list of function that K2 can call on the supplied class */
		List<K2NewNodeOption> GetNewNodeOptions(string ClassName);

		/**  */
		bool NodeHasInputOfType(K2NewNodeOption NodeOption, K2UIConnectorType Type);

		/**  */
		bool NodeHasOutputOfType(K2NewNodeOption NodeOption, K2UIConnectorType Type);

		/** Create a new node for a function call */
		void CreateNewNode(K2NewNodeOption NodeOption, Point Location);

		/** Set the 'default' value for an input, as a string */
		void SetInputDefaultValue(string NodeName, string InputName, string NewValue);

		/** Create a connection between an output and input connector */
		void MakeConnection(string FromNodeName, string FromConnectorName, string ToNodeName, string ToConnectorName);

		/** Delete an existing node */
		void DeleteNode(string NodeName);

		/** Move an existing node */
		void MoveNode(string NodeName, Point NewLocation);

		/** Break a connection */
		void BreakConnection(string InputNodeName, string InputConnectorName);

		/** Break all connections on a specified node */
		void BreakAllConnections(string NodeName);

		/** Tell the backend that the selection has changed */
		void OnSelectionChanged(List<string> NewSelectedNodes);
	}
}