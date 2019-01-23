/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MK2PanelBase_h_
#define __MK2PanelBase_h_

/** Abstract base class that encapsulates the CLI work needed for a K2-based node-graph tool */
ref class MK2PanelBase abstract : public Wpf_K2::IK2BackendInterface 
{
public:
	/** Pointer to the C# control that is used for editing the node graph */
	Wpf_K2::K2Editor^		K2Control;

	/** Graph of nodes that is used by the WPF front-end to draw widgets etc */
	Wpf_K2::K2UINodeGraph^	UIGraph;

	UK2GraphBase*			NodeGraph;


	/** Interop window */
	Interop::HwndSource^	InteropWindow;

	/** Constructor */
	MK2PanelBase(wxWindow* InHost, UK2GraphBase* InNodeGraph);

	IntPtr MessageHookFunction( IntPtr HWnd, int Msg, IntPtr WParam, IntPtr LParam, bool% OutHandled );

	void Resize(int Width, int Height);

	void SetFocus();

	/** Util for getting a K2NodeBase given its name */
	K2UINode^ FindMNodeFromName(String^ InName);

	/** Util for getting a managed output object from a node name and connector name */
	K2UIOutput^ FindMOutputFromName(String^ NodeName, String^ OutputName);

	UK2NodeBase* FindUNodeFromName(const FString& InName);

	K2UIOutput^ FindMOutputFromUOutput(UK2Output* UOutput);

	K2UIInput^ FindMInputFromName(String^ NodeName, String^ InputName);

	K2UIInput^ FindMInputFromUInput(UK2Input* UInput);

	static K2UINode^ CreateMNodeForUNode(UK2NodeBase* UNode);

	static K2UIConnectorType UNodeTypeToMNodeType(BYTE InMType);

	void UpdateUINodeGraph();

	virtual UBOOL AllowMultipleConnectionsFrom(BYTE InUType) = 0;

	//// IK2BackendInterface

	virtual K2UINodeGraph^ GetUIGraph();
	virtual void MakeConnection(String^ FromNodeName, String^ FromConnectorName, String^ ToNodeName, String^ ToConnectorName);
	virtual void MoveNode(String^ NodeName, Point NewLocation);
	virtual void BreakConnection(String^ InputNodeName, String^ InputConnectorName);
	virtual void BreakAllConnections(String^ NodeName);
	virtual void DeleteNode(String^ NodeName);

	// Below are pure virtual, implemented by concrete subclass

	virtual List<K2NewNodeOption^>^ GetNewNodeOptions(String^ ClassName) = 0;
	virtual bool NodeHasInputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) = 0;
	virtual bool NodeHasOutputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) = 0;
	virtual void CreateNewNode(K2NewNodeOption^ NodeOption, Point Location) = 0;
	virtual void SetInputDefaultValue(String^ NodeName, String^ InputName, String^ NewValue) = 0;
	virtual void OnSelectionChanged(List<String^>^ NewSelectedNodes) = 0;
};

#endif	// __MK2PanelBase_h_
