/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEdCLR.h"
#include "EngineK2Classes.h"
#include "K2.h"

#ifdef __cplusplus_cli

using namespace Wpf_K2;

#include "ManagedCodeSupportCLR.h"
#include "MK2PanelBase.h"





/** Constructor, do basic window interop setup */
MK2PanelBase::MK2PanelBase(wxWindow* InHost, UK2GraphBase* InNodeGraph)		
{
	// Setup WPF window to be created as a child of the browser window
	Interop::HwndSourceParameters sourceParams( "K2Host" );
	sourceParams.PositionX = 0;
	sourceParams.PositionY = 0;
	sourceParams.ParentWindow = (IntPtr)InHost->GetHandle();
	sourceParams.WindowStyle = (WS_VISIBLE | WS_CHILD);

	InteropWindow = gcnew Interop::HwndSource(sourceParams);
	InteropWindow->SizeToContent = SizeToContent::Manual;

	// Need to make sure any faulty WPF methods are hooked as soon as possible after a WPF window
	// has been created (it loads DLLs internally, which can't be hooked until after creation)
#if WITH_EASYHOOK
	WxUnrealEdApp::InstallHooksWPF();
#endif

	// Save pointer to node graph
	NodeGraph = InNodeGraph;

	// Create actual K2 editor wpf control
	K2Control = gcnew Wpf_K2::K2Editor();

	// Tell interop window it controls this control
	InteropWindow->RootVisual = K2Control;

	InteropWindow->AddHook(gcnew Interop::HwndSourceHook( this, &MK2PanelBase::MessageHookFunction ) );

	// Create an empty node graph (subclasses will actually fill this in in their constructors)
	UIGraph = gcnew Wpf_K2::K2UINodeGraph();

	// Tell K2 editor control about the backend interface to use (ie us)
	K2Control->SetBackend(this);

	// Do an initial update to make the UI match the data
	UpdateUINodeGraph();
	K2Control->UpdateGraph();

	// Show the window!
	const HWND WPFWindowHandle = ( HWND )InteropWindow->Handle.ToPointer();
	::ShowWindow( WPFWindowHandle, SW_SHOW );
}


/** Called when the HwndSource receives a windows message */
IntPtr MK2PanelBase::MessageHookFunction( IntPtr HWnd, int Msg, IntPtr WParam, IntPtr LParam, bool% OutHandled )
{
	IntPtr Result = (IntPtr)0;
	OutHandled = false;

	if( Msg == WM_GETDLGCODE )
	{
		OutHandled = true;

		// This tells Windows (and Wx) that we'll need keyboard events for this control
		Result = IntPtr( DLGC_WANTALLKEYS );
	}

	// Tablet PC software (e.g. Tablet PC Input Panel) sends WM_GETOBJECT events which
	// trigger a massive performance bug in WPF (http://wpf.codeplex.com/Thread/View.aspx?ThreadId=41964)
	// We'll intercept this message and skip it.
	if( Msg == WM_GETOBJECT )
	{
		OutHandled = true;
		Result = (IntPtr)0;
	}

	return Result;
}

/** Route resize messages to WPF interop */
void MK2PanelBase::Resize(int Width, int Height)
{
	SetWindowPos(static_cast<HWND>(InteropWindow->Handle.ToPointer()), NULL, 0, 0, Width, Height, SWP_NOMOVE | SWP_NOZORDER);
}

/** Route focus changes to WPF interop */
void MK2PanelBase::SetFocus()
{
	if ( InteropWindow != nullptr )
	{
		K2Control->Focus();
	}
}

/** Util for getting a K2UINode given its name */
K2UINode^ MK2PanelBase::FindMNodeFromName(String^ InName)
{
	if(UIGraph != nullptr)
	{
		for(INT i=0; i<UIGraph->AllNodes->Count; i++)
		{
			K2UINode^ MNode = UIGraph->AllNodes[i];

			if((MNode != nullptr) && (MNode->mName == InName))
			{
				return MNode;
			}
		}
	}

	return nullptr;
}

/** Util for getting a managed output object from a node name and connector name */
K2UIOutput^ MK2PanelBase::FindMOutputFromName(String^ NodeName, String^ OutputName)
{
	K2UINode^ MNode = FindMNodeFromName(NodeName);
	if(MNode == nullptr)
	{
		return nullptr;
	}

	return MNode->GetOutputFromName(OutputName);
}


K2UINode^ MK2PanelBase::CreateMNodeForUNode(UK2NodeBase* UNode)
{
	K2UINode^ MNode = gcnew K2UINode();
	MNode->mName = CLRTools::ToString( UNode->GetName() );
	MNode->mDisplayName = CLRTools::ToString( UNode->GetDisplayName() );
	MNode->mBorderColor = CLRTools::ToColor( UNode->GetBorderColor() );

	// See if this node is a member var node
	UBOOL bInputsAreEditable = UNode->InputDefaultsAreEditable();

	// Create input connectors
	for(INT i=0; i<UNode->Inputs.Num(); i++)
	{
		UK2Input* UInput = UNode->Inputs(i);
		if(UInput)
		{
			K2UIInput^ MInput = gcnew K2UIInput(UNodeTypeToMNodeType(UInput->Type), CLRTools::ToString(UInput->ConnName), CLRTools::ToString(UInput->GetValueString()), MNode);

			// Don't let this input be editable if it is an exec ior object input, or if it is an input on a member var
			if( (UInput->Type == K2CT_Object) || (UInput->Type == K2CT_Exec) || !bInputsAreEditable )
			{
				MInput->mbEditable = false;
			}

			MInput->mConnTooptip = CLRTools::ToString( UInput->GetTypeAsCodeString() );

			MNode->mInputs->Add(MInput);
		}
	}

	// Create output connectors
	for(INT i=0; i<UNode->Outputs.Num(); i++)
	{
		UK2Output* UOutput = UNode->Outputs(i);
		if(UOutput)
		{
			K2UIOutput^ MOutput = gcnew K2UIOutput(UNodeTypeToMNodeType(UOutput->Type), CLRTools::ToString(UOutput->ConnName), MNode);

			// If an object connector, fill in the class name
			UK2Output_Object* ObjOutput = Cast<UK2Output_Object>(UOutput);
			if(ObjOutput != NULL)
			{
				MOutput->mObjClassName = CLRTools::ToString( ObjOutput->ObjClass->GetPathName() );
				MOutput->mConnTooptip = MOutput->mObjClassName;
			}
			else
			{
				MOutput->mConnTooptip = CLRTools::ToString( UOutput->GetTypeAsCodeString() );
			}

			MNode->mOutputs->Add(MOutput);
		}
	}

	return MNode;
}

K2UIConnectorType MK2PanelBase::UNodeTypeToMNodeType(BYTE InMType)
{
	switch(InMType)
	{
	case K2CT_Bool:
		return K2UIConnectorType::CT_Bool;
	case K2CT_Float:
		return K2UIConnectorType::CT_Float;
	case K2CT_Vector:
		return K2UIConnectorType::CT_Vector;
	case K2CT_Rotator:
		return K2UIConnectorType::CT_Rotator;
	case K2CT_String:
		return K2UIConnectorType::CT_String;
	case K2CT_Int:
		return K2UIConnectorType::CT_Int;
	case K2CT_Object:
		return K2UIConnectorType::CT_Object;
	case K2CT_Exec:
	default:
		return K2UIConnectorType::CT_Exec;
	}
};

/** Updates the NodeGraph based on the current state of the graph inside the unreal DMC object */
void MK2PanelBase::UpdateUINodeGraph()
{
	if( NodeGraph != NULL )
	{
		for(INT i=0; i<NodeGraph->Nodes.Num(); i++)
		{
			UK2NodeBase* UNode = NodeGraph->Nodes(i);

			// If managed node not found, create now
			K2UINode^ MNode = FindMNodeFromName( CLRTools::ToString(UNode->GetName()) );
			if(MNode == nullptr)
			{				
				MNode = CreateMNodeForUNode(UNode);

				UIGraph->AllNodes->Add(MNode);
			}

			// Always update position
			MNode->mLocation.X = UNode->NodePosX;
			MNode->mLocation.Y = UNode->NodePosY;
		}

		// Now look for managed nodes that have no unreal node
		for(INT i=0; i<UIGraph->AllNodes->Count; i++)
		{
			K2UINode^ MNode = UIGraph->AllNodes[i];

			UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(MNode->mName) );
			if(UNode == NULL)
			{
				UIGraph->AllNodes->Remove(MNode);
			}
		}

		// Now all the correct nodes exist, update connections
		for(INT i=0; i<NodeGraph->Nodes.Num(); i++)
		{
			UK2NodeBase* UNode = NodeGraph->Nodes(i);
			K2UINode^ MNode = FindMNodeFromName( CLRTools::ToString(UNode->GetName()) );

			// Fix all input pointers
			for(INT i=0; i<UNode->Inputs.Num(); i++)
			{
				UK2Input* UInput = UNode->Inputs(i);
				if(UInput)
				{
					K2UIInput^ MInput = MNode->GetInputFromName( CLRTools::ToString(UInput->ConnName) );
					if(MInput)
					{
						// Update default value
						MInput->mValueString = CLRTools::ToString(UInput->GetValueString());

						// If unreal pointer is null, clear on managed object
						if(UInput->FromOutput == NULL)
						{
							MInput->mFromOutput = nullptr;
						}
						// if not, look up corresponding managed output
						else
						{
							MInput->mFromOutput = FindMOutputFromUOutput(UInput->FromOutput);
						}
					}
				}
			}

			// Fix all output pointers
			for(INT i=0; i<UNode->Outputs.Num(); i++)
			{
				UK2Output* UOutput = UNode->Outputs(i);
				if(UOutput)
				{
					K2UIOutput^ MOutput = MNode->GetOutputFromName( CLRTools::ToString(UOutput->ConnName) );
					if(MOutput)
					{
						// Clear existing 'to inputs' array
						MOutput->mToInputs->Clear();

						// Then add an entry for each one in the unreal connector
						for(INT ConnIdx=0; ConnIdx<UOutput->ToInputs.Num(); ConnIdx++)
						{
							K2UIInput^ MInput = FindMInputFromUInput( UOutput->ToInputs(ConnIdx) );
							if(MInput != nullptr)
							{
								MOutput->mToInputs->Add(MInput);
							}
						}
					}
				}
			}

		}
	}
	else
	{
		UIGraph->AllNodes->Clear();
	}
}

/** Util for getting a UK2NodeBase given its name */
UK2NodeBase* MK2PanelBase::FindUNodeFromName(const FString& InName)
{
	if( NodeGraph != NULL )
	{
		for(INT i=0; i<NodeGraph->Nodes.Num(); i++)
		{
			UK2NodeBase* Node = NodeGraph->Nodes(i);
			if((Node != NULL) && (Node->GetName() == InName))
			{
				return Node;
			}
		}
	}

	return NULL;
}

/** Find a managed output object from an Unreal output */
K2UIOutput^ MK2PanelBase::FindMOutputFromUOutput(UK2Output* UOutput)
{
	check(UOutput != NULL);
	check(UOutput->OwningNode != NULL);
	FString NodeName = UOutput->OwningNode->GetName();
	FString ConnName = UOutput->ConnName;

	return FindMOutputFromName( CLRTools::ToString(NodeName), CLRTools::ToString(ConnName) );
}

/** Util for getting a managed input object from a node name and connector name */
K2UIInput^ MK2PanelBase::FindMInputFromName(String^ NodeName, String^ InputName)
{
	K2UINode^ MNode = FindMNodeFromName(NodeName);
	if(MNode == nullptr)
	{
		return nullptr;
	}

	return MNode->GetInputFromName(InputName);
}

/** Find a managed input object from an Unreal input */
K2UIInput^ MK2PanelBase::FindMInputFromUInput(UK2Input* UInput)
{
	check(UInput != NULL);
	check(UInput->OwningNode != NULL);
	FString NodeName = UInput->OwningNode->GetName();
	FString ConnName = UInput->ConnName;

	return FindMInputFromName( CLRTools::ToString(NodeName), CLRTools::ToString(ConnName) );
}


//////////////////////////////////////////////////////////////////////////
// IK2BackendInterface

/** Return a pointer to the shared K2UINodeGraph object, that is the working C# view of the code graph */
Wpf_K2::K2UINodeGraph^ MK2PanelBase::GetUIGraph()
{
	return UIGraph;
}

/** Create a connection between an output and input connector */
void MK2PanelBase::MakeConnection(String^ FromNodeName, String^ FromConnectorName, String^ ToNodeName, String^ ToConnectorName)
{
	UK2NodeBase* FromUNode = FindUNodeFromName( CLRTools::ToFString(FromNodeName) );
	UK2NodeBase* ToUNode = FindUNodeFromName( CLRTools::ToFString(ToNodeName) );
	if((FromUNode == NULL) || (ToUNode == NULL))
	{
		debugf(TEXT("MakeConnection: Node not found."));
		return;
	}


	UK2Output* FromUConn = FromUNode->GetOutputFromName( CLRTools::ToFString(FromConnectorName) );
	UK2Input* ToUConn = ToUNode->GetInputFromName( CLRTools::ToFString(ToConnectorName) );
	if((FromUConn == NULL) || (ToUConn == NULL))
	{
		debugf(TEXT("MakeConnection: Connector not found."));
		return;
	}

	if(FromUConn->Type != ToUConn->Type)
	{
		debugf(TEXT("MakeConnection: Incorrect types."));
		return;
	}

	// See if we allow multiple connections from the supplied type in this tool
	if( !AllowMultipleConnectionsFrom(FromUConn->Type) )
	{
		FromUConn->BreakAllConnections();
	}

	// Then set new pointer
	FromUConn->ToInputs.AddItem(ToUConn);

	// No input is allowed to have more than one connection, so break it before making a new one
	ToUConn->Break();

	// Then set new pointer
	ToUConn->FromOutput = FromUConn;

	UpdateUINodeGraph();
}

/** Delete an existing node */
void MK2PanelBase::DeleteNode(String^ NodeName)
{
	UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(NodeName) );
	if(UNode != NULL)
	{
		UNode->BreakAllConnections();

		NodeGraph->Nodes.RemoveItem(UNode);

		UpdateUINodeGraph();
	}
}

/** Move an existing node */
void MK2PanelBase::MoveNode(String^ NodeName, Point NewLocation)
{
	UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(NodeName) );
	if(UNode != NULL)
	{
		UNode->NodePosX = NewLocation.X;
		UNode->NodePosY = NewLocation.Y;

		UpdateUINodeGraph();
	}
}

/** Break a connection */
void MK2PanelBase::BreakConnection(String^ InputNodeName, String^ InputConnectorName)
{
	UK2NodeBase* ToUNode = FindUNodeFromName( CLRTools::ToFString(InputNodeName) );
	if(ToUNode == NULL)
	{
		debugf(TEXT("BreakConnection: Node not found."));
		return;
	}

	UK2Input* ToUConn = ToUNode->GetInputFromName( CLRTools::ToFString(InputConnectorName) );
	if(ToUConn == NULL)
	{
		debugf(TEXT("BreakConnection: Connector not found."));
		return;
	}

	ToUConn->Break();

	UpdateUINodeGraph();
}

/** Break all connections on a specified node */
void MK2PanelBase::BreakAllConnections(String^ NodeName)
{
	UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(NodeName) );
	if(UNode == NULL)
	{
		debugf(TEXT("BreakAllConnections: Node not found."));
		return;
	}

	UNode->BreakAllConnections();

	UpdateUINodeGraph();
}

#endif // __cplusplus_cli
