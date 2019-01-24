/*=============================================================================
	PerforceSourceControl.cpp: Perforce specific source control API
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEdCLR.h"
#include "EngineAIClasses.h"
#include "EngineK2Classes.h"
#include "K2.h"
#include "EnginePrefabClasses.h"
#include "GameFrameworkClasses.h"
#include "K2_AITreeEditorShared.h"
#include "UnCompileHelper.h"

#ifdef __cplusplus_cli

using namespace Wpf_K2;

#include "ManagedCodeSupportCLR.h"
#include "MK2PanelBase.h"
#include "WPFFrameCLR.h"

/** Used to generate unique class names, for this session */
static INT ClassRenameCounter = 0;

//////////////////////////////////////////////////////////////////////////
// MK2AITreeUtilityPanel

ref class MK2AITreeUtilityPanel : public MK2PanelBase
{
public:
	/** Pointer to the AITree being edited */
	UAITree*				Tree;
	UAICommandNodeBase*		CommandNode;

	MK2AITreeUtilityPanel::MK2AITreeUtilityPanel(wxWindow* InHost, UAITree* InTree )
		:  MK2PanelBase(InHost, NULL)
	{
		Tree = InTree;
		CommandNode = NULL;
	}

	virtual UBOOL AllowMultipleConnectionsFrom(BYTE InUType) override
	{
		return TRUE;
	}

	virtual void SetCommandNode( UAICommandNodeBase* InNode )
	{
		CommandNode = InNode;

		if( CommandNode != NULL )
		{
			NodeGraph = CommandNode->UtilityDMC;
		}
		else
		{
			NodeGraph = NULL;
		}

		UpdateUINodeGraph();
		K2Control->UpdateGraph();
	}

	/** Find the unreal function with this name */
	UFunction* FindFunctionByName(UClass* InClass, const FString& FuncName, DWORD FuncFlag)
	{
		// If no class specified, default to DMC class.
		if(InClass == NULL)
		{
			InClass = CommandNode->UtilityDMC->GeneratedClass;
		}

		UFunction* Function = NULL;
		for( TFieldIterator<UFunction> Functions(InClass, TRUE); Functions; ++Functions )
		{
			UFunction* TestFunc = *Functions;
			if( TestFunc->HasAnyFunctionFlags(FuncFlag) && (TestFunc->GetName() == FuncName) )
			{
				Function = TestFunc;
				break;
			}
		}

		return Function;
	}


	virtual void CreateDMC()
	{
		FString PackageName = Tree->GetOuter()->GetName();
		PackageName = PackageName + TEXT(".Utility");

		UPackage* Package = UObject::CreatePackage( NULL, *PackageName );
		check(Package);

		FString ClassName = Tree->GetName() + TEXT("_") + CommandNode->GetName();

		// Create new DMC_Prototype object
		CommandNode->UtilityDMC = ConstructObject<UDMC_Prototype>(UDMC_Prototype::StaticClass(), Package, *ClassName, RF_Public|RF_Standalone|RF_Transactional);
		CommandNode->UtilityDMC->ParentClass = UAITree_DMC_Base::StaticClass();

		// Create initial UClass
		UClass* NewClass = GenerateClassFromDMCProto( CommandNode->UtilityDMC,  FString(TEXT("")) );
		if(NewClass != NULL)
		{
			// rename the new one to '..._C', as this is now the 'newest' version of this class
			FString GoodName = FString::Printf(TEXT("%s_C"), *CommandNode->UtilityDMC->GetName());
			NewClass->Rename(*GoodName, CommandNode->UtilityDMC->GetOutermost());
			CommandNode->UtilityDMC->GeneratedClass = NewClass;
		}

		NodeGraph = CommandNode->UtilityDMC;
		UpdateUINodeGraph();
	}


	/** This will create a new class using the supplied DMC_Prototype object. New class will have 'GEN' in the title */
	UClass* GenerateClassFromDMCProto(UDMC_Prototype* InProto, const FString& ExtraPropText)
	{
		if(!InProto->ParentClass)
		{
			debugf(TEXT("UDMC_Prototype::GenerateClass : No ParentClass"));
			return NULL;
		}

		FString NewClassName = FString::Printf( TEXT("%s_GEN_%d"), *InProto->GetName(), ++ClassRenameCounter );

		// Currently create class in the same package as the DMC. 
		// TODO: This means that the DMC will not be loaded in game (which it would be if the outer was the DMC), but
		// means that if you move(rename) the DMC to a different package, it will leave the class where it was
		UClass* NewClass = new( InProto->GetOutermost(), *NewClassName, RF_Public|RF_Standalone|RF_Transactional )UClass( NULL );

		if(NewClass != NULL)
		{
			NewClass->SuperStruct = InProto->ParentClass;
			NewClass->ClassCastFlags |= NewClass->GetSuperClass()->ClassCastFlags;

			// Build text for class description

			// Class header
			FString ScriptText = FString::Printf(TEXT("class %s extends %s;\n\n"), *NewClassName, *NewClass->GetSuperClass()->GetName() );

			// New member variable entries
			for(INT VarIdx=0; VarIdx<InProto->NewVars.Num(); VarIdx++)
			{		
				ScriptText += FString::Printf( TEXT("%s\n"), *InProto->NewVars(VarIdx).ToCodeString(InProto->GetName()) );
			}

			// Function code
			ScriptText += FString::Printf(TEXT("\n\n%s\n\n"), *InProto->FunctionCode);

			//debugf(*ScriptText); // TEMP: Log resulting info

			// Assign to new class
			NewClass->ScriptText = new(NewClass) UTextBuffer(*ScriptText);

			// Assign default properties text to new class
			NewClass->DefaultPropText = FString(TEXT("linenumber=0\r\n")) + InProto->DefaultPropText + TEXT("\n\n") + ExtraPropText;

			if(GScriptHelper == NULL)
			{
				GScriptHelper = new FCompilerMetadataManager();
			}

			GIsUCCMake = TRUE;
			UBOOL bSuccess = GEditor->MakeScripts( NewClass, GWarn, 0, FALSE, TRUE, NewClass->GetOutermost(), FALSE );
			GIsUCCMake = FALSE;

			if(!bSuccess)
			{
				// Didn't work, allow GC to clear it up
				NewClass->ClearFlags( RF_Standalone ); // Can get GC'd

				// clear script to avoid trying to recompile 
				NewClass->ScriptText = NULL; 
				NewClass->DefaultPropText = NULL;
				NewClass = NULL;

				appMsgf(AMT_OK, TEXT("Failure!\n%s"), *GUnrealEd->Results->Text);
			}
		}

		return NewClass;
	}


	//////////////////////////////////////////////////////////////////////////
	// IK2BackendInterface

	/** Get list of function that K2 can call on the supplied class */
	virtual List<K2NewNodeOption^>^ GetNewNodeOptions(String^ ClassName) override
	{
		List<K2NewNodeOption^>^ AllOptions = gcnew List<K2NewNodeOption^>();

		if( CommandNode != NULL )
		{
			if( CommandNode->UtilityDMC == NULL )
			{
				CreateDMC();
			}
			check(CommandNode->UtilityDMC);

			// Default to adding function on 'this' class
			UClass* FindInClass = CommandNode->UtilityDMC->GeneratedClass;

			// If name specified, find that class and search in it
			if( ClassName != nullptr && ClassName->Length > 0 )
			{
				FString ClassNameStr = CLRTools::ToFString(ClassName);
				FindInClass = (UClass*)UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *ClassNameStr );
				if(FindInClass == NULL)
				{
					debugf(TEXT("GetCallFunctions: Failed to find class '%s'"), *ClassNameStr);
				}
			}

			//////////
			// FUNCTIONS WE CAN CALL
/*
			for( TFieldIterator<UFunction> Functions(FindInClass, TRUE); Functions; ++Functions )
			{
				UFunction* Function = *Functions;
				if( Function->HasAnyFunctionFlags(FUNC_K2Call) )
				{
					// See if this is the kind of function we are looking for
					bool bIsPureFunc = (Function->HasAnyFunctionFlags(FUNC_K2Pure) != FALSE);

					K2NewNodeOption^ NewOption = gcnew K2NewNodeOption( bIsPureFunc ? TEXT("New Pure") : TEXT("New Function"), CLRTools::ToString(Function->GetName()) );
					NewOption->mClassName = CLRTools::ToString(FindInClass->GetName());
					//NewOption->mComment = TEXT("Func Comment"); // TODOTOOLTIP

					AllOptions->Add(NewOption);
				}
			}
*/

			//////////
			// NEW CONSTRUCTS

			AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Construct"), TEXT("For Loop")) );
			AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Construct"), TEXT("If/Else")) );

			//////////
			// FUNCTIONS WE CAN IMPLEMENT

			for( TFieldIterator<UFunction> Functions(CommandNode->UtilityDMC->GeneratedClass, TRUE); Functions; ++Functions )
			{
				UFunction* Function = *Functions;
				if( Function->HasAnyFunctionFlags(FUNC_K2Override) )
				{
					K2NewNodeOption^ NewOption = gcnew K2NewNodeOption( TEXT("New Implement"), CLRTools::ToString(Function->GetName()) );
					AllOptions->Add(NewOption);
				}
			}
		}


		
		return AllOptions;
	}

	virtual bool NodeHasInputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		// Not used
		return false;
	}

	virtual bool NodeHasOutputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		return false;
	}


	/** Create a new node for a function call */
	virtual void CreateNewNode(K2NewNodeOption^ NodeOption, Point Location) override
	{
		if( CommandNode->UtilityDMC == NULL )
		{
			CreateDMC();
		}

		if(NodeOption->mCategoryName == TEXT("New Pure") || NodeOption->mCategoryName == TEXT("New Function"))
		{
//			CreateNewCallFuncNode(NodeOption->mClassName, NodeOption->mNodeName, Location);
		}
		else if(NodeOption->mCategoryName == TEXT("New Construct"))
		{
			if(NodeOption->mNodeName == TEXT("For Loop"))
			{
				CreateNewForLoopNode(Location);
			}
			else if(NodeOption->mNodeName == TEXT("If/Else"))
			{
				CreateNewIfElseNode(Location);
			}
		}
		else if(NodeOption->mCategoryName == TEXT("New Implement"))
		{
			CreateNewImplementFuncNode(NodeOption->mNodeName, Location);
		}
		else if(NodeOption->mCategoryName == TEXT("New Variable"))
		{
//			CreateNewVarNode(NodeOption->mNodeName, Location);
		}
	}

	/** Create a new node for implementing a function */
	void CreateNewImplementFuncNode(String^ FunctionName, Point Location)
	{
		FString FuncName = CLRTools::ToFString(FunctionName);
		UFunction* Function = FindFunctionByName(NULL, FuncName, FUNC_K2Override);
		if(Function != NULL)
		{
			UK2Node_Event* NewEventNode = ConstructObject<UK2Node_Event>(UK2Node_Event::StaticClass(), CommandNode->UtilityDMC);
			NewEventNode->EventName = CLRTools::ToFString(FunctionName);
			NewEventNode->EventName = Function->GetName();
			NewEventNode->Function = Function;
			NewEventNode->NodePosX = Location.X;
			NewEventNode->NodePosY = Location.Y;

			NewEventNode->CreateAutoConnectors();
			NewEventNode->CreateConnectorsFromFunction(Function);

			NodeGraph->Nodes.AddItem(NewEventNode);
			UpdateUINodeGraph();
		}
		else
		{
			debugf(TEXT("Function (%s) not found!"), *FuncName);
		}
	}

	/** Create a new if/else node */
	void CreateNewIfElseNode(Point Location)
	{
		UK2Node_IfElse* NewIfNode = ConstructObject<UK2Node_IfElse>(UK2Node_IfElse::StaticClass(), CommandNode->UtilityDMC);
		NewIfNode->NodePosX = Location.X;
		NewIfNode->NodePosY = Location.Y;

		NewIfNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem(NewIfNode);
		UpdateUINodeGraph();
	}

	/** Create a new for loop node */
	void CreateNewForLoopNode(Point Location)
	{
		UK2Node_ForLoop* NewForNode = ConstructObject<UK2Node_ForLoop>(UK2Node_ForLoop::StaticClass(), CommandNode->UtilityDMC);
		NewForNode->NodePosX = Location.X;
		NewForNode->NodePosY = Location.Y;

		NewForNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem(NewForNode);
		UpdateUINodeGraph();
	}

	/** Set the 'default' value for an input, as a string */
	virtual void SetInputDefaultValue(String^ NodeName, String^ InputName, String^ NewValue) override
	{
		// Not used 
	}	

	/** Tell the backend that the selection has changed */
	virtual void OnSelectionChanged(List<String^>^ NewSelectedNodes) override
	{
		// Do we care?
	}
};

//////////////////////////////////////////////////////////////////////////
// MK2AITreeGatherPanel

ref class MK2AITreeGatherPanel : public MK2PanelBase
{
public:
	UAITree*		Tree;
	
	MK2AITreeGatherPanel::MK2AITreeGatherPanel(wxWindow* InHost, UAITree* InTree)
		:  MK2PanelBase(InHost, InTree->GatherList)
	{
		Tree = InTree;
	}

	virtual UBOOL AllowMultipleConnectionsFrom(BYTE InUType) override
	{
		return TRUE;
	}

	//////////////////////////////////////////////////////////////////////////
	// IK2BackendInterface

	/** Get list of function that K2 can call on the supplied class */
	virtual List<K2NewNodeOption^>^ GetNewNodeOptions(String^ ClassName) override
	{
		List<K2NewNodeOption^>^ AllOptions = gcnew List<K2NewNodeOption^>();


		for( FObjectIterator It; It; ++It )
		{
			UClass* Cls = Cast<UClass>(*It);
			if( Cls == NULL || Cls->HasAnyClassFlags( CLASS_Abstract ) )
			{
				continue;
			}

			if( Cls->IsChildOf( UAIGatherNodeBase::StaticClass() ) )
			{
				FString ClsName = Cls->GetName();
				K2NewNodeOption^ NewOption = gcnew K2NewNodeOption(TEXT("Nodes"), CLRTools::ToString(Cls->GetDesc()));
				NewOption->mClassName = CLRTools::ToString(ClsName);
				AllOptions->Add( NewOption );
			}
		}

		return AllOptions;
	}

	virtual bool NodeHasInputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		// Not used
		return false;
	}

	virtual bool NodeHasOutputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		return false;
	}

	/** Create a new node for a function call */
	virtual void CreateNewNode(K2NewNodeOption^ NodeOption, Point Location) override
	{
		UClass* NewNodeClass = UAIGatherNodeBase::StaticClass();

		UAIGatherNodeBase* NewGatherNode = ConstructObject<UAIGatherNodeBase>(NewNodeClass, Tree);
		NewGatherNode->NodePosX = Location.X;
		NewGatherNode->NodePosY = Location.Y;

		NewGatherNode->CreateAutoConnectors();

		if( Tree->GatherList == NULL )
		{
			Tree->GatherList = ConstructObject<UK2GraphBase>( UK2GraphBase::StaticClass(), Tree );
			NodeGraph = Tree->GatherList;
		}

		Tree->GatherList->Nodes.AddItem( NewGatherNode );
		UpdateUINodeGraph();

	}

	/** Set the 'default' value for an input, as a string */
	virtual void SetInputDefaultValue(String^ NodeName, String^ InputName, String^ NewValue) override
	{
		// Not used 
	}	

	/** Tell the backend that the selection has changed */
	virtual void OnSelectionChanged(List<String^>^ NewSelectedNodes) override
	{
		// Do we care?
	}
};

//////////////////////////////////////////////////////////////////////////
// MK2Panel

ref class MK2AITreePanel : public MK2PanelBase
{
public:
	/** Pointer to the AITree being edited */
	UAITree*				Tree;
	MK2AITreeUtilityPanel^	UtilityPanel;
	MK2AITreeGatherPanel^	GatherPanel;

	MK2AITreePanel::MK2AITreePanel(wxWindow* InHost, UAITree* InTree)
		:  MK2PanelBase(InHost, InTree)
	{
		Tree = CastChecked<UAITree>(InTree);
	}


	/** Create a new node for a function call */
	void CreateNewCommandNode(String^ ClassName, String^ FunctionName, Point Location)
	{
		UClass* NewNodeClass = UAICommandNodeBase::StaticClass();

		UAICommandNodeBase* NewCmdNode = ConstructObject<UAICommandNodeBase>(NewNodeClass, Tree);
		NewCmdNode->NodePosX = Location.X;
		NewCmdNode->NodePosY = Location.Y;
		NewCmdNode->CommandClass = ((ClassName != nullptr) ? Cast<UClass>(UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *CLRTools::ToFString(ClassName) )) : NULL);

		NewCmdNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem( NewCmdNode );
		UpdateUINodeGraph();
	}

	void CreateNewActionNode(String^ ClassName, String^ FunctionName, Point Location)
	{
		UClass* NewNodeClass = UAICommandNodeBase::StaticClass();

		UAICommandNodeBase* NewCmdNode = ConstructObject<UAICommandNodeBase>(NewNodeClass, Tree);
		NewCmdNode->NodePosX = Location.X;
		NewCmdNode->NodePosY = Location.Y;
		NewCmdNode->CommandClass = ((ClassName != nullptr) ? Cast<UClass>(UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *CLRTools::ToFString(ClassName) )) : NULL);

		NewCmdNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem( NewCmdNode );
		UpdateUINodeGraph();
	}	

	void CreateNewRootNode(String^ ClassName, String^ FunctionName, Point Location)
	{
		UClass* NewNodeClass = UAICommandNodeRoot::StaticClass();

		UAICommandNodeRoot* NewRootNode = ConstructObject<UAICommandNodeRoot>(NewNodeClass, Tree);
		NewRootNode->NodePosX = Location.X;
		NewRootNode->NodePosY = Location.Y;
		NewRootNode->CommandClass = NULL;

		NewRootNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem( NewRootNode );
		UpdateUINodeGraph();
	}

	virtual UBOOL AllowMultipleConnectionsFrom(BYTE InUType) override
	{
		return TRUE;
	}

	//////////////////////////////////////////////////////////////////////////
	// IK2BackendInterface


	/** Get list of function that K2 can call on the supplied class */
	virtual List<K2NewNodeOption^>^ GetNewNodeOptions(String^ ClassName) override
	{
		List<K2NewNodeOption^>^ AllOptions = gcnew List<K2NewNodeOption^>();

		K2NewNodeOption^ NewRootOption = gcnew K2NewNodeOption(TEXT("Root"), CLRTools::ToString(UAICommandNodeRoot::StaticClass()->GetName()));
		AllOptions->Add( NewRootOption );

		for( FObjectIterator It; It; ++It )
		{
			UClass* Cls = Cast<UClass>(*It);
			if( Cls == NULL || Cls->HasAnyClassFlags( CLASS_Abstract ) )
			{
				continue;
			}

			if( Cls->IsChildOf( UGameAICommand::StaticClass() ) )
			{
				FString ClsName = Cls->GetName();
				if( appStristr( *ClsName, TEXT("NanoAIAction") ) == NULL )
				{
					K2NewNodeOption^ NewOption = gcnew K2NewNodeOption(TEXT("Command"), CLRTools::ToString(Cls->GetDesc()));
					NewOption->mClassName = CLRTools::ToString(ClsName);
					AllOptions->Add( NewOption );
				}
				else
				{
					K2NewNodeOption^ NewOption = gcnew K2NewNodeOption(TEXT("Action"), CLRTools::ToString(Cls->GetDesc()));
					NewOption->mClassName = CLRTools::ToString(ClsName);
					AllOptions->Add( NewOption );
				}
			}
		}

		return AllOptions;
	}

	virtual bool NodeHasInputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		// Not used
		return false;
	}
	
	virtual bool NodeHasOutputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		return false;
	}

	/** Create a new node for a function call */
	virtual void CreateNewNode(K2NewNodeOption^ NodeOption, Point Location) override
	{
		if( NodeOption->mCategoryName == TEXT("Root") )
		{
			CreateNewRootNode( NodeOption->mClassName, NodeOption->mNodeName, Location );
		}
		else if( NodeOption->mCategoryName == TEXT("Command") )
		{
			CreateNewCommandNode( NodeOption->mClassName, NodeOption->mNodeName, Location );
		}
		else if( NodeOption->mCategoryName == TEXT("Action") )
		{
			CreateNewActionNode( NodeOption->mClassName, NodeOption->mNodeName, Location );
		}
	}
	

	/** Set the 'default' value for an input, as a string */
	virtual void SetInputDefaultValue(String^ NodeName, String^ InputName, String^ NewValue) override
	{
		// Not used 
	}	

	/** Tell the backend that the selection has changed */
	virtual void OnSelectionChanged(List<String^>^ NewSelectedNodes) override
	{
		UAICommandNodeBase* CommandNode = NULL;
		if( NewSelectedNodes->Count == 1 )
		{
			UK2NodeBase* NodeBase = FindUNodeFromName(CLRTools::ToFString(NewSelectedNodes[0]));
			CommandNode = Cast<UAICommandNodeBase>(NodeBase);
		}

//PASS BY STRING NAME??!
		if( CommandNode != NULL )
		{
			INT j = 0;
			j++;
		}

		UtilityPanel->SetCommandNode(CommandNode);
	}
};


//////////////////////////////////////////////////////////////////////////
// WxK2AITree_HostPanel

BEGIN_EVENT_TABLE( WxK2AITree_HostPanel, wxPanel )
	EVT_SIZE( WxK2AITree_HostPanel::OnSize )
	EVT_SET_FOCUS( WxK2AITree_HostPanel::OnReceiveFocus )
END_EVENT_TABLE()

WxK2AITree_HostPanel::WxK2AITree_HostPanel(wxWindow* Parent, UAITree* InTree) :
	wxPanel(Parent, wxID_ANY)
{
	K2Panel = gcnew MK2AITreePanel(this, InTree);
}


WxK2AITree_HostPanel::~WxK2AITree_HostPanel()
{
	MK2AITreePanel^ MyK2Panel = K2Panel;
	if( MyK2Panel != nullptr )
	{
		delete MyK2Panel;
		K2Panel = NULL;
	}
}

void WxK2AITree_HostPanel::OnSize( wxSizeEvent& In )
{
	if( static_cast<MK2AITreePanel^>(K2Panel) != nullptr )
	{
		K2Panel->Resize(In.GetSize().GetWidth(), In.GetSize().GetHeight());
	}
}

void WxK2AITree_HostPanel::OnReceiveFocus( wxFocusEvent& Event )
{
	if ( static_cast<MK2AITreePanel^>(K2Panel) != nullptr )
	{
		K2Panel->SetFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
// WxK2AITree_UtilityPanel
BEGIN_EVENT_TABLE( WxK2AITree_UtilityPanel, wxPanel )
	EVT_SIZE( WxK2AITree_UtilityPanel::OnSize )
	EVT_SET_FOCUS( WxK2AITree_UtilityPanel::OnReceiveFocus )
END_EVENT_TABLE()


WxK2AITree_UtilityPanel::WxK2AITree_UtilityPanel( wxWindow* Parent, WxK2AITree_HostPanel* InHostPanel, UAITree* InTree ) : 
	wxPanel(Parent, wxID_ANY)
{
	HostPanel = InHostPanel;
	
	K2UtilityPanel = gcnew MK2AITreeUtilityPanel(this, InTree);

	HostPanel->UtilityPanel = this;
	HostPanel->K2Panel->UtilityPanel = K2UtilityPanel;
}

WxK2AITree_UtilityPanel::~WxK2AITree_UtilityPanel()
{
	MK2AITreeUtilityPanel^ MyK2UtilityPanel = K2UtilityPanel;
	if( MyK2UtilityPanel != nullptr )
	{
		delete MyK2UtilityPanel;
		K2UtilityPanel = NULL;
	}
}

void WxK2AITree_UtilityPanel::OnSize( wxSizeEvent& In )
{
	if( static_cast<MK2AITreeUtilityPanel^>(K2UtilityPanel) != nullptr )
	{
		K2UtilityPanel->Resize(In.GetSize().GetWidth(), In.GetSize().GetHeight());
	}
}

void WxK2AITree_UtilityPanel::OnReceiveFocus( wxFocusEvent& Event )
{
	if ( static_cast<MK2AITreeUtilityPanel^>(K2UtilityPanel) != nullptr )
	{
		K2UtilityPanel->SetFocus();
	}
}


//////////////////////////////////////////////////////////////////////////
// WxK2AITree_GatherPanel
BEGIN_EVENT_TABLE( WxK2AITree_GatherPanel, wxPanel )
	EVT_SIZE( WxK2AITree_GatherPanel::OnSize )
	EVT_SET_FOCUS( WxK2AITree_GatherPanel::OnReceiveFocus )
END_EVENT_TABLE()

WxK2AITree_GatherPanel::WxK2AITree_GatherPanel( wxWindow* Parent, WxK2AITree_HostPanel* InHostPanel, UAITree* InTree ) : 
	wxPanel(Parent, wxID_ANY)
{
	HostPanel = InHostPanel;
	K2GatherPanel = gcnew MK2AITreeGatherPanel(this, InTree);

	HostPanel->GatherPanel = this;
	HostPanel->K2Panel->GatherPanel = K2GatherPanel;
}

WxK2AITree_GatherPanel::~WxK2AITree_GatherPanel()
{
	MK2AITreeGatherPanel^ MyK2GatherPanel = K2GatherPanel;
	if( MyK2GatherPanel != nullptr )
	{
		delete MyK2GatherPanel;
		K2GatherPanel = NULL;
	}
}

void WxK2AITree_GatherPanel::OnSize( wxSizeEvent& In )
{
	if( static_cast<MK2AITreeGatherPanel^>(K2GatherPanel) != nullptr )
	{
		K2GatherPanel->Resize(In.GetSize().GetWidth(), In.GetSize().GetHeight());
	}
}

void WxK2AITree_GatherPanel::OnReceiveFocus( wxFocusEvent& Event )
{
	if ( static_cast<MK2AITreeGatherPanel^>(K2GatherPanel) != nullptr )
	{
		K2GatherPanel->SetFocus();
	}
}

#endif // __cplusplus_cli