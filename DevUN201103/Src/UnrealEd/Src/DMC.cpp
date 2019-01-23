/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EnginePrefabClasses.h"
#include "UnCompileHelper.h"
#include "PropertyWindow.h"
#include "DMC.h"
#include "EngineK2Classes.h"
#include "K2.h"
#include "DlgGenericComboEntry.h"

#if WITH_MANAGED_CODE
#include "K2_EditorShared.h"
#endif

/** Used to generate unique class names, for this session */
static INT ClassRenameCounter = 0;

static TArray<UClass*>	ComponentClasses;

class WxDMCToolMenuBar : public wxMenuBar
{
public:
	WxDMCToolMenuBar()
	{
		wxMenu* VarMenu = new wxMenu();
		VarMenu->Append( IDDMC_ADDBOOLVAR, *LocalizeUnrealEd("AddBoolVar"), TEXT("") );
		VarMenu->Append( IDDMC_ADDINTVAR, *LocalizeUnrealEd("AddIntVar"), TEXT("") );
		VarMenu->Append( IDDMC_ADDFLOATVAR, *LocalizeUnrealEd("AddFloatVar"), TEXT("") );
		VarMenu->Append( IDDMC_ADDVECTORVAR, *LocalizeUnrealEd("AddVectorVar"), TEXT("") );

		VarMenu->AppendSeparator(); //---

		for(INT i=0; i<ComponentClasses.Num(); i++)
		{
			if( ComponentClasses(i) != NULL )
			{
				FString AddCompDesc = FString::Printf( TEXT("Add %s Variable"), *ComponentClasses(i)->GetName().ToUpper() );
				VarMenu->Append( IDDMC_ADDCOMPONENT_START+i, *AddCompDesc, TEXT("") );
			}
		}

		VarMenu->AppendSeparator(); //---

		VarMenu->Append( IDDMC_REMOVEVAR, *LocalizeUnrealEd("RemoveVar"), TEXT("") );
		Append( VarMenu, *LocalizeUnrealEd("VarMenu") );
	}
};

BEGIN_EVENT_TABLE( WxDMC, WxTrackableFrame )
	EVT_MENU( IDDMC_ADDBOOLVAR, WxDMC::OnAddBoolVar )
	EVT_MENU( IDDMC_ADDINTVAR, WxDMC::OnAddIntVar )
	EVT_MENU( IDDMC_ADDFLOATVAR, WxDMC::OnAddFloatVar )
	EVT_MENU( IDDMC_ADDVECTORVAR, WxDMC::OnAddVectorVar )
	EVT_MENU_RANGE( IDDMC_ADDCOMPONENT_START, IDDMC_ADDCOMPONENT_END, WxDMC::OnAddCompVar )
	EVT_MENU( IDDMC_REMOVEVAR, WxDMC::OnRemoveVar )
	EVT_BUTTON( IDDMC_RECOMPILE, WxDMC::OnRecompile )
END_EVENT_TABLE()



WxDMC::WxDMC(wxWindow* InParent, wxWindowID InID, UDMC_Prototype* InProto)
:	WxTrackableFrame( InParent, InID, *LocalizeUnrealEd("DMCTool"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR )
,	FDockingParent(this)
{
	EditProto = InProto;

	FWindowUtil::LoadPosSize( GetDockingParentName(), this, 64,64,800,650 );

	wxPanel* MainPanel = new wxPanel( this, -1 );
	wxBoxSizer* SizerV = new wxBoxSizer( wxVERTICAL );
	{
		// K2 graphical editing control
#if WITH_MANAGED_CODE
		K2Host = new WxK2HostPanel(MainPanel, EditProto, this);
		SizerV->Add( K2Host, 3, wxGROW|wxALL, 0 );
#endif

		// text box for code
		TextBox = new WxTextCtrl(MainPanel, wxID_ANY, TEXT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
		TextBox->SetValue(*InProto->FunctionCode);
		TextBox->SetSelection(0,0); // deselect text
		SizerV->Add( TextBox, 1, wxGROW|wxALL, 0 );


		// Button for recompiling
		wxButton* RecompileButton = new wxButton(MainPanel, IDDMC_RECOMPILE, TEXT("RECOMPILE"));
		SizerV->Add( RecompileButton, 0, wxEXPAND|wxALL, 0 );

	}
	MainPanel->SetSizer(SizerV);


	AddDockingWindow( MainPanel, FDockingParent::DH_None, TEXT("Main") );
	SetDockHostSize(FDockingParent::DH_Right, 400);

	// DefObj window
	DefObjProps = new WxPropertyWindowHost;
	DefObjProps->Create( this, NULL, -1);
	DefObjProps->SetObject( EditProto->GeneratedClass->GetDefaultObject(), EPropertyWindowFlags::ShouldShowCategories | EPropertyWindowFlags::ShowDMCOnlyProperties );
	AddDockingWindow(DefObjProps, FDockingParent::DH_Right, TEXT("Defaults"));

	bShowingClassDefs = TRUE;

	// Fill in component array if not already done.
	if(ComponentClasses.Num() == 0)
	{
		ComponentClasses.AddItem(UStaticMeshComponent::StaticClass());
		ComponentClasses.AddItem(USkeletalMeshComponent::StaticClass());
		ComponentClasses.AddItem(UPointLightComponent::StaticClass());
		ComponentClasses.AddItem(UCylinderComponent::StaticClass());
		ComponentClasses.AddItem(UParticleSystemComponent::StaticClass());
	}

	// Menu bar
	MenuBar = new WxDMCToolMenuBar();
	AppendWindowMenu(MenuBar);
	SetMenuBar( MenuBar );
}

WxDMC::~WxDMC()
{
	FWindowUtil::SavePosSize( GetDockingParentName(), this );

	SaveDockingLayout();
}

/** Util to find all Actors in level of class OldClass and  */
void WxDMC::ReplaceAllActorsOfClass(UClass* OldClass, UClass* NewClass)
{
	for( FActorIterator It; It; ++It )
	{
		AActor* Actor = *It;

		if( !Actor->IsTemplate() && !Actor->IsPendingKill() && (Actor->GetClass() == OldClass))
		{
			FVector Location = Actor->Location;
			FRotator Rotation = Actor->Rotation;

			UBOOL bSelected = Actor->IsSelected();

			if(bSelected)
			{
				GUnrealEd->SelectActor( Actor, FALSE, NULL, TRUE, TRUE );
			}

			GWorld->EditorDestroyActor(Actor, FALSE);

			AActor* NewActor = GWorld->SpawnActor(NewClass, NAME_None, Location, Rotation);
			NewActor->PostEditMove(TRUE); // Forces re-run of creation script

			if(bSelected)
			{
				GUnrealEd->SelectActor( NewActor, TRUE, NULL, TRUE, TRUE );
			}
		}
	}
}

void WxDMC::OnSelectionChanged(TArray<UK2NodeBase*> NewSelectedNodes)
{
	UActorComponent* EditArchetype = NULL;

	for(INT i=0; i<NewSelectedNodes.Num(); i++)
	{
		UK2NodeBase* Node = NewSelectedNodes(i);
		if(Node)
		{
			UK2Node_Func_NewComp* NewCompNode = Cast<UK2Node_Func_NewComp>(Node);
			if(NewCompNode && NewCompNode->ComponentTemplate)
			{
				EditArchetype = NewCompNode->ComponentTemplate;
				break;
			}
		}
	}

	// Change property window to edit archetype
	if(EditArchetype != NULL)
	{
		DefObjProps->SetObject( EditArchetype, EPropertyWindowFlags::ShouldShowCategories );
		bShowingClassDefs = FALSE;
	}
	else if((EditArchetype == NULL) && !bShowingClassDefs)
	{
		DefObjProps->SetObject( EditProto->GeneratedClass->GetDefaultObject(), EPropertyWindowFlags::ShouldShowCategories );
		bShowingClassDefs = TRUE;
	}
}

/** Update the DMC object based on the current state in the tool */
void WxDMC::UpdateDMCFromTool()
{
	FStringOutputDevice Ar;
	const FExportObjectInnerContext Context;

	// Iterate over all objects making sure they import/export flags are unset. 
	// These are used for ensuring we export each object only once etc.
	for( FObjectIterator It; It; ++It )
	{
		It->ClearFlags( RF_TagImp | RF_TagExp );
	}

	UObject::ExportProperties
		(
		&Context,
		Ar,
		EditProto->GeneratedClass,
		EditProto->GeneratedClass->GetDefaults(),
		3,
		EditProto->GeneratedClass->GetSuperClass(),
		EditProto->GeneratedClass->GetSuperClass() ? EditProto->GeneratedClass->GetSuperClass()->GetDefaults() : NULL,
		EditProto->GeneratedClass,
		PPF_ExportDefaultProperties
		);

	EditProto->DefaultPropText = Ar;

#if 0
	debugf(TEXT("-- DEFPROPTEXT -- BEGIN"));
	debugf(*EditProto->DefaultPropText); 
	debugf(TEXT("-- DEFPROPTEXT -- END"));
#endif
}

UBOOL WxDMC::GenerateCode()
{
	TArray<FK2CodeLine> OutCode;
	UBOOL bSuccess = TRUE;

	// Find each event
	for(INT i=0; i<EditProto->Nodes.Num(); i++)
	{
		UK2Node_Event* Event = Cast<UK2Node_Event>(EditProto->Nodes(i));
		if(Event)
		{
			FK2CodeGenContext Context;
			TArray<FK2CodeLine> EventCode;

			Event->GetEventText(Context, EventCode);

			if(Context.bGenFailure)
			{
				appMsgf(AMT_OK, TEXT("Generation failed: %s"), *Context.GenFailMessage);
				bSuccess = FALSE;
			}
			else
			{
				OutCode.Append( EventCode );
				OutCode.AddItem( FK2CodeLine(TEXT("")) );				
			}
		}
	}

	if(bSuccess)
	{
		// Put it all together into a big string with line returns
		FString CodeString;
		for(INT LineIdx=0; LineIdx<OutCode.Num(); LineIdx++)
		{
			CodeString += OutCode(LineIdx).GetString();
		}

		// Set as both the text entry box and the function code in the DMC object
		TextBox->SetValue(*CodeString);
		EditProto->FunctionCode = CodeString;
	}

	return bSuccess;
}

/** Regenerate the UClass, based on the DMC_Prototype */
UBOOL WxDMC::RecompileClass(const FString& ExtraPropText)
{
	UpdateDMCFromTool();

	UClass* NewClass = GenerateClassFromDMCProto(EditProto, ExtraPropText);

	// If a success..
	if(NewClass)
	{
		UClass* OldClass = EditProto->GeneratedClass;

		// Clear out the old one
		if(EditProto->GeneratedClass)
		{
			// rename to a temp name, move into transient package
			FString TempName = FString::Printf(TEXT("%s_OLD_%d"), *EditProto->GetName(), ++ClassRenameCounter);
			EditProto->GeneratedClass->Rename(*TempName, UObject::GetTransientPackage());
			EditProto->GeneratedClass->ClearFlags( RF_Standalone ); // Can get GC'd
			EditProto->GeneratedClass->ScriptText = NULL; // clear script to avoid trying to recompile 

			EditProto->GeneratedClass = NULL;
		}

		// rename the new one to '..._C', as this is now the 'newest' version of this class
		FString GoodName = FString::Printf(TEXT("%s_C"), *EditProto->GetName());
		NewClass->Rename(*GoodName, EditProto->GetOutermost());
		EditProto->GeneratedClass = NewClass;

		// Make sure MetaData doesn't have any entries to the class we just renamed out of package
		EditProto->GetOutermost()->GetMetaData()->RemoveMetaDataOutsidePackage();

		// update property window to new default object
		DefObjProps->SetObject( EditProto->GeneratedClass->GetDefaultObject(), EPropertyWindowFlags::NoFlags );

		// Replace any instances of the old class in current level
		if(OldClass != NULL)
		{
			ReplaceAllActorsOfClass(OldClass, NewClass);
		}

		GCallbackEvent->Send(CALLBACK_RefreshEditor_ActorBrowser);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/** Called to regenerate class/recompile code */
void WxDMC::OnRecompile( wxCommandEvent& In )
{
	UBOOL bSuccess = GenerateCode();

	if(bSuccess)
	{
		RecompileClass( TEXT("") );
	}
}


/** Add a new variable of the supplied type to the DMC */
void WxDMC::AddNewVar(FName InVarType)
{
	// Let user enter the name for the new variable
	WxDlgGenericStringEntry NameDlg;
	if( NameDlg.ShowModal( TEXT("NewVarName"), TEXT(""), TEXT("") ) == wxID_OK )
	{
		// Get name from dialog
		FString NewNameString = NameDlg.GetEnteredString();
		FName NewName = FName(*NewNameString);

		// TODO: Check for duplicates
		if(NewName != NAME_None)
		{
			INT NewVarIdx = EditProto->NewVars.AddZeroed();
			EditProto->NewVars(NewVarIdx).VarName = NewName;
			EditProto->NewVars(NewVarIdx).VarType = InVarType;

			RecompileClass(TEXT(""));
		}
	}
}

/** Adds a new bool member variable to the class */
void WxDMC::OnAddBoolVar( wxCommandEvent& In )
{
	AddNewVar(NAME_Bool);
}

/** Adds a new int member variable to the class */
void WxDMC::OnAddIntVar( wxCommandEvent& In )
{
	AddNewVar(NAME_Int);
}

/** Adds a new float member variable to the class */
void WxDMC::OnAddFloatVar( wxCommandEvent& In )
{
	AddNewVar(NAME_Float);
}

/** Adds a new vector member variable to the class */
void WxDMC::OnAddVectorVar( wxCommandEvent& In )
{
	AddNewVar(NAME_Vector);
}

void WxDMC::OnAddCompVar( wxCommandEvent& In )
{
	INT CompClassIndex = In.GetId() - IDDMC_ADDCOMPONENT_START;
	check( CompClassIndex >= 0 && CompClassIndex < ComponentClasses.Num() );

	UClass* CompClass = ComponentClasses(CompClassIndex);
	AddNewVar( CompClass->GetFName() );
}

void WxDMC::OnRemoveVar( wxCommandEvent& In )
{
	appMsgf(AMT_OK, TEXT("Sorry, not done yet!"));
}


/** This will create a new class using the supplied DMC_Prototype object. New class will have 'GEN' in the title */
UClass* WxDMC::GenerateClassFromDMCProto(UDMC_Prototype* InProto, const FString& ExtraPropText)
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
		else
		{
			appMsgf(AMT_OK, TEXT("Success!"));
		}
	}

	return NewClass;
}


const TCHAR* WxDMC::GetDockingParentName() const
{
	return TEXT("DMC");
}

const INT WxDMC::GetDockingParentVersion() const
{
	return 1;
}