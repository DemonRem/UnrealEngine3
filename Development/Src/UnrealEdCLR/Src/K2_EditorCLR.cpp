/*=============================================================================
	PerforceSourceControl.cpp: Perforce specific source control API
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEdCLR.h"
#include "K2_EditorShared.h"
#include "EngineK2Classes.h"
#include "K2.h"
#include "EnginePrefabClasses.h"
#include "K2OwnerInterface.h"

#ifdef __cplusplus_cli

using namespace Wpf_K2;

#include "ManagedCodeSupportCLR.h"
#include "MK2PanelBase.h"
#include "WPFFrameCLR.h"




/** Util to see if the function supplied has an input of the desired type */
static UBOOL FunctionHasParamOfType(UFunction* InFunction, K2UIConnectorType InType, UBOOL bWantOutput)
{
	// Iterate over all params of function
	for (TFieldIterator<UProperty> PropIt(InFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParam = *PropIt;

		// See if its input of output
		UBOOL bOutput = (FuncParam->PropertyFlags & CPF_OutParm);
		// See if this is the type of param we want
		if((bOutput && bWantOutput) || (!bOutput && !bWantOutput))
		{
			// Get the enum type of this param 
			UClass* ConnObjClass = NULL;
			EK2ConnectorType UConnType = GetTypeFromProperty(FuncParam, ConnObjClass);
			// Convert to managed enum
			K2UIConnectorType MConnType = MK2PanelBase::UNodeTypeToMNodeType(UConnType);
			// If the same, yay, we do!
			if(MConnType == InType)
			{
				return TRUE;
			}
		}
	}

	// Boo, no input of that type
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// MK2Panel

ref class MK2Panel : public MK2PanelBase
{
public:

	UDMC_Prototype*			EditDMC;

	FK2OwnerInterface*		OwnerInterface;

	MK2Panel::MK2Panel(wxWindow* InHost, UDMC_Prototype* InDMC, FK2OwnerInterface* InOwnerInt)
		:  MK2PanelBase(InHost, InDMC)
	{
		EditDMC = InDMC;
		OwnerInterface = InOwnerInt;
	}

	virtual UBOOL AllowMultipleConnectionsFrom(BYTE InUType) override
	{
		return (InUType != K2CT_Exec);
	}

	/** Find the unreal function with this name */
	UFunction* FindFunctionByName(UClass* InClass, const FString& FuncName, DWORD FuncFlag)
	{
		// If no class specified, default to DMC class.
		if(InClass == NULL)
		{
			InClass = EditDMC->GeneratedClass;
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

	/** Find the unreal class with this name */
	UClass* FindClassByName(const FString& ClassName)
	{
		// Default to finding function within the DMC class
		UClass* OutClass = EditDMC->GeneratedClass;

		// If name specified, find that class and search in it
		if(ClassName.Len() > 0)
		{
			OutClass = (UClass*)UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *ClassName );
			if(OutClass == NULL)
			{
				debugf(TEXT("GetCallFunctions: Failed to find class '%s'"), *ClassName);
			}
		}

		return OutClass;
	}

	/** Find the unreal member variable with this name */
	UProperty* FindVariableByName(const FString& VarName)
	{
		UProperty* Var = NULL;
		for( TFieldIterator<UProperty> Props(EditDMC->GeneratedClass, TRUE); Props; ++Props )
		{
			UProperty* Prop = *Props;
			UClass* OutClass = NULL;
			if( (Prop->GetName() == VarName) && Prop->HasAllPropertyFlags(CPF_Edit) && (GetTypeFromProperty(Prop, OutClass) != K2CT_Unsupported) )
			{
				Var = Prop;
				break;
			}
		}

		return Var;
	}

	/** Create a new node for a function call */
	void CreateNewCallFuncNode(String^ ClassName, String^ FunctionName, Point Location)
	{
		UClass* Class = FindClassByName( CLRTools::ToFString(ClassName) );

		FString FuncName = CLRTools::ToFString(FunctionName);
		UFunction* Function = FindFunctionByName(Class, FuncName, FUNC_K2Call);

		// If we found the function, create a node for it
		if(Function != NULL)
		{
			// Find correct node class
			UClass* NewNodeClass = NULL;
			if(Function->HasAllFunctionFlags(FUNC_K2Pure))
			{
				NewNodeClass = UK2Node_FuncPure::StaticClass();
			}
			else
			{
				NewNodeClass = UK2Node_Func::StaticClass();
			}

			UK2Node_FuncBase* NewFuncNode = ConstructObject<UK2Node_FuncBase>(NewNodeClass, EditDMC);
			
			NewFuncNode->Function = Function;
			NewFuncNode->NodePosX = Location.X;
			NewFuncNode->NodePosY = Location.Y;

			NewFuncNode->CreateAutoConnectors();
			NewFuncNode->CreateConnectorsFromFunction(Function);

			NodeGraph->Nodes.AddItem(NewFuncNode);
			UpdateUINodeGraph();
		}
		else
		{
			debugf(TEXT("CreateNewCallFuncNode: Function (%s) not found!"), *FuncName);
		}
	}

	/** Create a new component node of the supplied type */
	void CreateNewComponentNode(String^ ComponentType, Point Location)
	{
		UClass* NewCompClass = NULL;

		if(ComponentType == "StaticMesh")
		{
			NewCompClass = UStaticMeshComponent::StaticClass();
		}
		else if(ComponentType == "SkeletalMesh")
		{
			NewCompClass = USkeletalMeshComponent::StaticClass();
		}
		else if(ComponentType == "PointLight")
		{
			NewCompClass = UPointLightComponent::StaticClass();
		}
		else if(ComponentType == "Cylinder")
		{
			NewCompClass = UCylinderComponent::StaticClass();
		}
		else if(ComponentType == "Particle")
		{
			NewCompClass = UParticleSystemComponent::StaticClass();
		}
		else
		{
			debugf(TEXT("CreateNewComponentNode: Unknown type '%s'"), *CLRTools::ToFString(ComponentType));
			return;
		}

		UFunction* Function = FindFunctionByName(NULL, TEXT("AddComponent"), FUNC_AllFlags);
		if(Function != NULL)
		{
			UK2Node_Func_NewComp* NewCompFuncNode = ConstructObject<UK2Node_Func_NewComp>(UK2Node_Func_NewComp::StaticClass(), EditDMC);

			NewCompFuncNode->Function = Function;
			NewCompFuncNode->NodePosX = Location.X;
			NewCompFuncNode->NodePosY = Location.Y;

			NewCompFuncNode->CreateAutoConnectors();
			NewCompFuncNode->CreateConnectorsFromFunction(Function, NewCompClass);


			UObject* TemplateOuter = EditDMC->GetOutermost();
			FName TemplateOuterName = UObject::MakeUniqueObjectName(TemplateOuter, NewCompClass, FName(TEXT("K2CompTemplate")));
			UActorComponent* NewTemplate = (UActorComponent*)UObject::StaticConstructObject(NewCompClass, TemplateOuter, TemplateOuterName);

			// Make sure selected asset in content browser is loaded, to GetTop can get them
			GCallbackEvent->Send( CALLBACK_LoadSelectedAssetsIfNeeded );

			// Special handling for static mesh component
			UStaticMeshComponent* NewStaticMeshTemp = Cast<UStaticMeshComponent>(NewTemplate);
			if(NewStaticMeshTemp != NULL)
			{
				UStaticMesh* SelectedStaticMesh = GEditor->GetSelectedObjects()->GetTop<UStaticMesh>();
				NewStaticMeshTemp->StaticMesh = SelectedStaticMesh;
			}

			// Special handling for skel mesh component
			USkeletalMeshComponent* NewSkelMeshTemp = Cast<USkeletalMeshComponent>(NewTemplate);
			if(NewSkelMeshTemp != NULL)
			{
				USkeletalMesh* SelectedSkelMesh = GEditor->GetSelectedObjects()->GetTop<USkeletalMesh>();
				NewSkelMeshTemp->SkeletalMesh = SelectedSkelMesh;
			}

			// Special handling for particle component
			UParticleSystemComponent* NewParticleTemp = Cast<UParticleSystemComponent>(NewTemplate);
			if(NewParticleTemp != NULL)
			{
				UParticleSystem* SelectedPSys = GEditor->GetSelectedObjects()->GetTop<UParticleSystem>();
				NewParticleTemp->Template = SelectedPSys;
			}

			NewCompFuncNode->ComponentTemplate = NewTemplate;

			NodeGraph->Nodes.AddItem(NewCompFuncNode);
			UpdateUINodeGraph();
		}
		else
		{
			debugf(TEXT("CreateNewComponentNode: Function not found!"));
		}
	}

	/** Create a new node for implementing a function */
	void CreateNewImplementFuncNode(String^ FunctionName, Point Location)
	{
		FString FuncName = CLRTools::ToFString(FunctionName);
		UFunction* Function = FindFunctionByName(NULL, FuncName, FUNC_K2Override);
		if(Function != NULL)
		{
			UK2Node_Event* NewEventNode = ConstructObject<UK2Node_Event>(UK2Node_Event::StaticClass(), EditDMC);
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

	/** Create a new node for accessing a variable */
	void CreateNewVarNode(String^ VarName, Point Location)
	{
		FString VariableName = CLRTools::ToFString(VarName);
		UProperty* Var = FindVariableByName(VariableName);
		if(Var != NULL)
		{
			UClass* OutClass = NULL;

			UK2Node_MemberVar* NewVarNode = ConstructObject<UK2Node_MemberVar>(UK2Node_MemberVar::StaticClass(), EditDMC);
			NewVarNode->VarName = VariableName;
			NewVarNode->VarType = GetTypeFromProperty(Var, OutClass);
			NewVarNode->NodePosX = Location.X;
			NewVarNode->NodePosY = Location.Y;

			NewVarNode->CreateAutoConnectors();
			NewVarNode->CreateConnectorsFromVariable(Var);

			NodeGraph->Nodes.AddItem(NewVarNode);
			UpdateUINodeGraph();
		}
		else
		{
			debugf(TEXT("Var (%s) not found!"), *VariableName);
		}
	}

	/** Create a new if/else node */
	void CreateNewIfElseNode(Point Location)
	{
		UK2Node_IfElse* NewIfNode = ConstructObject<UK2Node_IfElse>(UK2Node_IfElse::StaticClass(), EditDMC);
		NewIfNode->NodePosX = Location.X;
		NewIfNode->NodePosY = Location.Y;

		NewIfNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem(NewIfNode);
		UpdateUINodeGraph();
	}

	/** Create a new for loop node */
	void CreateNewForLoopNode(Point Location)
	{
		UK2Node_ForLoop* NewForNode = ConstructObject<UK2Node_ForLoop>(UK2Node_ForLoop::StaticClass(), EditDMC);
		NewForNode->NodePosX = Location.X;
		NewForNode->NodePosY = Location.Y;

		NewForNode->CreateAutoConnectors();

		NodeGraph->Nodes.AddItem(NewForNode);
		UpdateUINodeGraph();
	}

	//////////////////////////////////////////////////////////////////////////
	// IK2BackendInterface


	/** Get list of function that K2 can call on the supplied class */
	virtual List<K2NewNodeOption^>^ GetNewNodeOptions(String^ ClassName) override
	{
		List<K2NewNodeOption^>^ AllOptions = gcnew List<K2NewNodeOption^>();

		// Default to adding function on 'this' class
		UClass* FindInClass = EditDMC->GeneratedClass;

		// If name specified, find that class and search in it
		if((ClassName != nullptr) && (ClassName->Length > 0))
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

		//////////
		// NEW COMPONENTS

		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Component"), TEXT("StaticMesh")) );
		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Component"), TEXT("SkeletalMesh")) );
		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Component"), TEXT("PointLight")) );
		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Component"), TEXT("Cylinder")) );
		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Component"), TEXT("Particle")) );

		//////////
		// NEW CONSTRUCTS

		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Construct"), TEXT("For Loop")) );
		AllOptions->Add( gcnew K2NewNodeOption(TEXT("New Construct"), TEXT("If/Else")) );

		//////////
		// FUNCTIONS WE CAN IMPLEMENT

		for( TFieldIterator<UFunction> Functions(EditDMC->GeneratedClass, TRUE); Functions; ++Functions )
		{
			UFunction* Function = *Functions;
			if( Function->HasAnyFunctionFlags(FUNC_K2Override) )
			{
				K2NewNodeOption^ NewOption = gcnew K2NewNodeOption( TEXT("New Implement"), CLRTools::ToString(Function->GetName()) );
				AllOptions->Add(NewOption);
			}
		}

		//////////
		// MEMBER VARS

		for( TFieldIterator<UProperty> Props(EditDMC->GeneratedClass, TRUE); Props; ++Props )
		{
			UProperty* Prop = *Props;
			UClass* OutClass = NULL;
			if( Prop->HasAllPropertyFlags(CPF_Edit) && (GetTypeFromProperty(Prop, OutClass) != K2CT_Unsupported) )
			{
				K2NewNodeOption^ NewOption = gcnew K2NewNodeOption( TEXT("New Variable"), CLRTools::ToString(Prop->GetName()) );
				AllOptions->Add(NewOption);
			}
		}

		return AllOptions;
	}

	virtual bool NodeHasInputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		// Only check function options...
		bool bIsPure = (NodeOption->mCategoryName == TEXT("New Pure"));
		bool bIsImp = (NodeOption->mCategoryName == TEXT("New Function"));

		if(bIsPure || bIsImp)
		{
			// Imperative functions always have an event and an object input
			if(bIsImp && (Type == K2UIConnectorType::CT_Exec || Type == K2UIConnectorType::CT_Object))
			{
				return true;
			}
			
			// Find the function
			UClass* Class = FindClassByName( CLRTools::ToFString(NodeOption->mClassName) );

			FString FuncName = CLRTools::ToFString(NodeOption->mNodeName);
			UFunction* Function = FindFunctionByName(Class, FuncName, FUNC_K2Call);

			if((Function != NULL) && Function->HasAnyFunctionFlags(FUNC_K2Call))
			{
				// See if if it has an input of that type
				return (FunctionHasParamOfType(Function, Type, FALSE) != FALSE);
			}
		}

		return false;
	}
	
	virtual bool NodeHasOutputOfType(K2NewNodeOption^ NodeOption, K2UIConnectorType Type) override
	{
		// Only check function options...
		bool bIsPure = (NodeOption->mCategoryName == TEXT("New Pure"));
		bool bIsImp = (NodeOption->mCategoryName == TEXT("New Function"));

		if(bIsPure || bIsImp)
		{
			// Imperative functions always have an event output
			if(bIsImp && (Type == K2UIConnectorType::CT_Exec))
			{
				return true;
			}

			// Find the function
			UClass* Class = FindClassByName( CLRTools::ToFString(NodeOption->mClassName) );

			FString FuncName = CLRTools::ToFString(NodeOption->mNodeName);
			UFunction* Function = FindFunctionByName(Class, FuncName, FUNC_K2Call);

			if((Function != NULL) && Function->HasAnyFunctionFlags(FUNC_K2Call))
			{
				// See if if it has an output of that type
				return (FunctionHasParamOfType(Function, Type, TRUE) != FALSE);
			}
		}

		return false;
	}

	/** Create a new node for a function call */
	virtual void CreateNewNode(K2NewNodeOption^ NodeOption, Point Location) override
	{
		if(NodeOption->mCategoryName == TEXT("New Pure") || NodeOption->mCategoryName == TEXT("New Function"))
		{
			CreateNewCallFuncNode(NodeOption->mClassName, NodeOption->mNodeName, Location);
		}
		else if(NodeOption->mCategoryName == TEXT("New Component"))
		{
			CreateNewComponentNode(NodeOption->mNodeName, Location);
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
			CreateNewVarNode(NodeOption->mNodeName, Location);
		}
	}
	

	/** Set the 'default' value for an input, as a string */
	virtual void SetInputDefaultValue(String^ NodeName, String^ InputName, String^ NewValue) override
	{
		UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(NodeName) );
		if(UNode == NULL)
		{
			debugf(TEXT("SetInputDefaultValue: Node not found."));
			return;
		}

		UK2Input* UInput = UNode->GetInputFromName( CLRTools::ToFString(InputName) );
		if(UInput == NULL)
		{
			debugf(TEXT("SetInputDefaultValue: Connector not found."));
			return;
		}

		UInput->SetDefaultFromString( CLRTools::ToFString(NewValue) );

		UpdateUINodeGraph();
	}

	/** Tell the backend that the selection has changed */
	virtual void OnSelectionChanged(List<String^>^ NewSelectedNodes) override
	{
		TArray<UK2NodeBase*> USelNodes;

		for(INT i=0; i<NewSelectedNodes->Count; i++)
		{
			UK2NodeBase* UNode = FindUNodeFromName( CLRTools::ToFString(NewSelectedNodes[i]) );
			if(UNode != NULL)
			{
				USelNodes.AddItem(UNode);
			}
		}

		OwnerInterface->OnSelectionChanged(USelNodes);
	}
};



//////////////////////////////////////////////////////////////////////////
// WxK2HostPanel

BEGIN_EVENT_TABLE( WxK2HostPanel, wxPanel )
	EVT_SIZE( WxK2HostPanel::OnSize )
	EVT_SET_FOCUS( WxK2HostPanel::OnReceiveFocus )
END_EVENT_TABLE()

WxK2HostPanel::WxK2HostPanel(wxWindow* Parent, UDMC_Prototype* DMC, FK2OwnerInterface* OwnerInt) :
	wxPanel(Parent, wxID_ANY)
{
	K2Panel = gcnew MK2Panel(this, DMC, OwnerInt);
}


WxK2HostPanel::~WxK2HostPanel()
{
	MK2Panel^ MyK2Panel = K2Panel;
	if( MyK2Panel != nullptr )
	{
		delete MyK2Panel;
		K2Panel = NULL;
	}
}

void WxK2HostPanel::OnSize( wxSizeEvent& In )
{
	//debugf(TEXT("Wx; WM_SIZE: (%d, %d)"), In.GetSize().GetWidth() , In.GetSize().GetHeight());

	if( static_cast<MK2Panel^>(K2Panel) != nullptr )
	{
		K2Panel->Resize(In.GetSize().GetWidth(), In.GetSize().GetHeight());
	}
}

void WxK2HostPanel::OnReceiveFocus( wxFocusEvent& Event )
{
	if ( static_cast<MK2Panel^>(K2Panel) != nullptr )
	{
		K2Panel->SetFocus();
	}
}

#endif // __cplusplus_cli