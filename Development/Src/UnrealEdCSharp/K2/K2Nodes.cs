/*
 *	K2Nodes.cs : Node classes used to represent graph
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
	public enum K2UIConnectorType
	{
		CT_Bool,
		CT_Int,
		CT_Float,
		CT_Vector,
		CT_Rotator,
		CT_String,
		CT_Object,
		CT_Exec
	}

	// Structure defining a single connector on a node
	public abstract class K2UIConnector
	{
		public K2UINode				mOwningNode;
		public string				mConnName;
		public K2UIConnectorType	mConnType;

		// Comment that should appear when mousing over this connector
		public string			mConnTooptip;

		public K2UIConnector(K2UIConnectorType InType, string InName, K2UINode InNode)
		{
			mConnType = InType;
			mConnName = InName;
			mOwningNode = InNode;
		}

		public virtual Color GetColor()
		{
			switch(mConnType)
			{
				case K2UIConnectorType.CT_Bool:
					return Color.FromRgb(255, 42, 0);

				case K2UIConnectorType.CT_Int:
					return Color.FromRgb(0, 201, 89);

				case K2UIConnectorType.CT_Float:
					return Color.FromRgb(5, 159, 255);

				case K2UIConnectorType.CT_Vector:
					return Color.FromRgb(255, 216, 0);

				case K2UIConnectorType.CT_Rotator:
					return Color.FromRgb(255, 106, 0);

				case K2UIConnectorType.CT_String:
					return Color.FromRgb(45, 77, 255);

				case K2UIConnectorType.CT_Object:
					return Color.FromRgb(252, 109, 255);

				case K2UIConnectorType.CT_Exec:
					return Color.FromRgb(0, 0, 0);

				default:
					return Color.FromRgb(255, 255, 255);
			}
		}

		public virtual string GetConnectorLabel()
		{
			return mConnName;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// INPUT

	// Structure defining a single input for a node
	public class K2UIInput : K2UIConnector
	{
		public K2UIInput(K2UIConnectorType InType, string InName, string InValue, K2UINode InNode)
			: base(InType, InName, InNode)
		{
			mValueString = InName;
			mbEditable = true;
		}

		/** Pointer to output that we are connected to */
		public K2UIOutput mFromOutput;

		/** String used to show the default value at this input */
		public string mValueString;

		/** If this input can be typed in to to set a default value */
		public bool mbEditable;


		public override string GetConnectorLabel()
		{
			if (!mbEditable || (mFromOutput != null))
			{
				return mConnName;
			}
			else
			{
				return mConnName + " (" + mValueString + ")";
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// OUTPUT

	// Structure defining a single output for a node
	public class K2UIOutput : K2UIConnector
	{
		public K2UIOutput(K2UIConnectorType InType, string InName, K2UINode InNode)
			: base(InType, InName, InNode)
		{}

		// List of inputs that this output is connected to
		public List<K2UIInput> mToInputs = new List<K2UIInput>();

		// If an object connector, this is the name of the class
		public string mObjClassName;
	}

	//////////////////////////////////////////////////////////////////////////
	// NODE

	// Class that indicates a new node 
	public class K2UINode
	{
		// Used to identify this node
		public string mName;

		// 'Friendly' name for this node
		public string mDisplayName;

		// List of input connectors
		public List<K2UIInput> mInputs = new List<K2UIInput>();

		// List of output connectors
		public List<K2UIOutput> mOutputs = new List<K2UIOutput>();

		// 2D location for node in the K2 tool
		public Point mLocation;

		// Color of the border of this node
		public Color mBorderColor;

		// Comment that should appear when mousing over this node
		public string mNodeTooltip;

		/** Util to find & return an input connector by name */
		public K2UIInput GetInputFromName(string InName)
		{
			foreach (K2UIInput Input in mInputs)
			{
				if(Input.mConnName == InName)
				{
					return Input;
				}
			}

			return null;
		}

		/** Util to find & return an output connector by name */
		public K2UIOutput GetOutputFromName(string InName)
		{
			foreach (K2UIOutput Output in mOutputs)
			{
				if (Output.mConnName == InName)
				{
					return Output;
				}
			}

			return null;
		}
	}
}