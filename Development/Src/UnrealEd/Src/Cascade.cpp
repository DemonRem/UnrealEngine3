/*=============================================================================
	Cascade.cpp: 'Cascade' particle editor
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "Cascade.h"
#include "CurveEd.h"
#include "EngineMaterialClasses.h"
#include "Properties.h"

IMPLEMENT_CLASS(UCascadeOptions);
IMPLEMENT_CLASS(UCascadePreviewComponent);

/*-----------------------------------------------------------------------------
	UCascadeParticleSystemComponent
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UCascadeParticleSystemComponent);

UBOOL UCascadeParticleSystemComponent::SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, DWORD TraceFlags, const FVector& Extent)
{
	if (CascadePreviewViewportPtr && CascadePreviewViewportPtr->FloorComponent && (CascadePreviewViewportPtr->FloorComponent->HiddenEditor == FALSE))
	{
		Hit = FCheckResult(1.f);

		const UBOOL bHit = !CascadePreviewViewportPtr->FloorComponent->LineCheck(Hit, End, Start, Extent, TraceFlags);
		if (bHit)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*-----------------------------------------------------------------------------
	FCascadeNotifyHook
-----------------------------------------------------------------------------*/
void FCascadeNotifyHook::NotifyDestroy(void* Src)
{
	if (WindowOfInterest == Src)
	{
		if (Cascade->PreviewVC && Cascade->PreviewVC->FloorComponent)
		{
			// Save the information in the particle system
			Cascade->PartSys->FloorPosition = Cascade->PreviewVC->FloorComponent->Translation;
			Cascade->PartSys->FloorRotation = Cascade->PreviewVC->FloorComponent->Rotation;
			Cascade->PartSys->FloorScale = Cascade->PreviewVC->FloorComponent->Scale;
			Cascade->PartSys->FloorScale3D = Cascade->PreviewVC->FloorComponent->Scale3D;

			if (Cascade->PreviewVC->FloorComponent->StaticMesh)
			{
				Cascade->PartSys->FloorMesh = Cascade->PreviewVC->FloorComponent->StaticMesh->GetPathName();
			}
			else
			{
				warnf(TEXT("Unable to locate Cascade floor mesh outer..."));
				Cascade->PartSys->FloorMesh = TEXT("");
			}
			Cascade->PartSys->MarkPackageDirty();
		}
	}
}

/*-----------------------------------------------------------------------------
	WxCascade
-----------------------------------------------------------------------------*/
/**
 * Versioning Info for the Docking Parent layout file.
 */
namespace
{
	static const TCHAR* DockingParent_Name = TEXT("Cascade");
	static const INT DockingParent_Version = 0;		//Needs to be incremented every time a new dock window is added or removed from this docking parent.
}


UBOOL				WxCascade::bParticleModuleClassesInitialized = FALSE;
TArray<UClass*>		WxCascade::ParticleModuleClasses;
TArray<UClass*>		WxCascade::ParticleModuleBaseClasses;

// On init, find all particle module classes. Will use later on to generate menus.
void WxCascade::InitParticleModuleClasses()
{
	if (bParticleModuleClassesInitialized)
		return;

	for(TObjectIterator<UClass> It; It; ++It)
	{
		// Find all ParticleModule classes (ignoring abstract or ParticleTrailModule classes
		if (It->IsChildOf(UParticleModule::StaticClass()))
		{
			if (!(It->ClassFlags & CLASS_Abstract))
			{
					
				if (
					// Ver 190 - Removed TypeDataSubUV
					!(*It == UParticleModuleTypeDataSubUV::StaticClass()) &&
					// Do not want to put the LocationPrimitiveBase class in either...
					!(*It == UParticleModuleLocationPrimitiveBase::StaticClass()) &&
					// Removing the old beam and trail type data modules
					!(*It == UParticleModuleTypeDataBeam::StaticClass()) &&
					!(*It == UParticleModuleTypeDataTrail::StaticClass()) &&
					!(It->IsChildOf(UParticleModuleUberBase::StaticClass()))
					)
				{
					ParticleModuleClasses.AddItem(*It);
				}
			}
			else
			{
				if (!(*It == UParticleModuleUberBase::StaticClass()))
				{
					ParticleModuleBaseClasses.AddItem(*It);
				}
			}
		}
	}

	bParticleModuleClassesInitialized = TRUE;
}

UBOOL				WxCascade::bParticleEmitterClassesInitialized = FALSE;
TArray<UClass*>		WxCascade::ParticleEmitterClasses;

// On init, find all particle module classes. Will use later on to generate menus.
void WxCascade::InitParticleEmitterClasses()
{
	if (bParticleEmitterClassesInitialized)
		return;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		// Find all ParticleModule classes (ignoring abstract classes)
		if (It->IsChildOf(UParticleEmitter::StaticClass()) && !(It->ClassFlags & CLASS_Abstract))
		{
			ParticleEmitterClasses.AddItem(*It);
		}
	}

	bParticleEmitterClassesInitialized = TRUE;
}

bool WxCascade::DuplicateEmitter(UParticleEmitter* SourceEmitter, UParticleSystem* DestSystem, UBOOL bShare)
{
	// Find desired class of new module.
    UClass* NewEmitClass = SourceEmitter->GetClass();
	if (NewEmitClass == UParticleSpriteEmitter::StaticClass())
	{
		// Construct it
		UParticleEmitter* NewEmitter = ConstructObject<UParticleEmitter>(NewEmitClass, DestSystem, NAME_None, RF_Transactional);

		check(NewEmitter);

		FString	NewName = SourceEmitter->GetEmitterName().ToString();
		NewName += TEXT("_Dup");

		NewEmitter->SetEmitterName(FName(*NewName));

		// UParticleEmitter members...
		if (NewEmitClass == UParticleSpriteEmitter::StaticClass())
		{
			UParticleSpriteEmitter* NewSpriteEmitter = (UParticleSpriteEmitter*)NewEmitter;
			UParticleSpriteEmitter* SelectedSpriteEmitter = (UParticleSpriteEmitter*)SourceEmitter;

            // Copy the settings, and then copy each module from the source to the destination
            NewSpriteEmitter->Material			= SelectedSpriteEmitter->Material;
            NewSpriteEmitter->ScreenAlignment	= SelectedSpriteEmitter->ScreenAlignment;
		}

		//	'Private' data - not required by the editor
		UObject*			DupObject;
		UParticleLODLevel*	SourceLODLevel;
		UParticleLODLevel*	NewLODLevel;

		NewEmitter->LODLevels.InsertZeroed(0, SourceEmitter->LODLevels.Num());
		for (INT LODIndex = 0; LODIndex < SourceEmitter->LODLevels.Num(); LODIndex++)
		{
			SourceLODLevel	= SourceEmitter->LODLevels(LODIndex);
			NewLODLevel		= ConstructObject<UParticleLODLevel>(UParticleLODLevel::StaticClass(), NewEmitter, NAME_None, RF_Transactional);
			check(NewLODLevel);

			NewLODLevel->Level					= SourceLODLevel->Level;
			NewLODLevel->LevelSetting			= SourceLODLevel->LevelSetting;

			NewLODLevel->bEnabled				= SourceLODLevel->bEnabled;

			// The RequiredModule
			DupObject = GEditor->StaticDuplicateObject(SourceLODLevel->RequiredModule, SourceLODLevel->RequiredModule, DestSystem, TEXT("None"));
			check(DupObject);
			NewLODLevel->RequiredModule						= Cast<UParticleModuleRequired>(DupObject);
			NewLODLevel->RequiredModule->ModuleEditorColor	= FColor::MakeRandomColor();

			// Copy each module
			NewLODLevel->Modules.InsertZeroed(0, SourceLODLevel->Modules.Num());
			for (INT ModuleIndex = 0; ModuleIndex < SourceLODLevel->Modules.Num(); ModuleIndex++)
			{
				UParticleModule* SourceModule = SourceLODLevel->Modules(ModuleIndex);
				if (bShare)
				{
					NewLODLevel->Modules(ModuleIndex) = SourceModule;
				}
				else
				{
					DupObject = GEditor->StaticDuplicateObject(SourceModule, SourceModule, DestSystem, TEXT("None"));
					if (DupObject)
					{
						UParticleModule* Module				= Cast<UParticleModule>(DupObject);
						Module->ModuleEditorColor			= FColor::MakeRandomColor();
						NewLODLevel->Modules(ModuleIndex)	= Module;
					}
				}
			}

			// TypeData module as well...
			if (SourceLODLevel->TypeDataModule)
			{
				if (bShare)
				{
					NewLODLevel->TypeDataModule = SourceLODLevel->TypeDataModule;
				}
				else
				{
					DupObject = GEditor->StaticDuplicateObject(SourceLODLevel->TypeDataModule, SourceLODLevel->TypeDataModule, DestSystem, TEXT("None"));
					if (DupObject)
					{
						UParticleModule* Module		= Cast<UParticleModule>(DupObject);
						Module->ModuleEditorColor	= FColor::MakeRandomColor();
						NewLODLevel->TypeDataModule	= Module;
					}
				}
			}
			NewLODLevel->ConvertedModules		= TRUE;
			NewLODLevel->PeakActiveParticles	= SourceLODLevel->PeakActiveParticles;

			NewEmitter->LODLevels(LODIndex)		= NewLODLevel;
		}

		//@todo. Compare against the destination system, and insert appropriate LOD levels where necessary
		// Generate all the levels that are present in other emitters...
		// NOTE: Big assumptions - the highest and lowest are 0,100 respectively and they MUST exist.
		if (DestSystem->Emitters.Num() > 0)
		{
			UParticleEmitter* DestEmitter = DestSystem->Emitters(0);

			debugf(TEXT("Generating existing LOD levels..."));

			UParticleEmitter* OuterEmitter = DestEmitter;
			UParticleEmitter* InnerEmitter = NewEmitter;

			UINT OuterLODIndex = 0;
			UINT OuterLastLODIndex = OuterEmitter->LODLevels.Num();
			UINT InnerLODIndex = 0;
			UINT InnerLastLODIndex = InnerEmitter->LODLevels.Num();

			UParticleLODLevel* OuterLOD;
			UParticleLODLevel* NextOuterLOD;
			UParticleLODLevel* InnerLOD;
			UParticleLODLevel* NextInnerLOD;

			// Run through the destination emitter...
			// If we find an LOD level that does NOT exist in the NewEmitter, add it
			for (OuterLODIndex = 0; OuterLODIndex < OuterLastLODIndex - 1; OuterLODIndex++)
			{
				OuterLOD = OuterEmitter->LODLevels(OuterLODIndex);
				NextOuterLOD = OuterEmitter->LODLevels(OuterLODIndex + 1);

				InnerLastLODIndex = InnerEmitter->LODLevels.Num();
				for (InnerLODIndex = 0; InnerLODIndex < InnerLastLODIndex - 1; InnerLODIndex++)
				{
					InnerLOD = DestEmitter->LODLevels(InnerLODIndex);
					NextInnerLOD = DestEmitter->LODLevels(InnerLODIndex + 1);

					if (InnerLOD->LevelSetting == OuterLOD->LevelSetting)
					{
						// Level already exists... skip to the next one
						debugf(TEXT("NewEmitter - LOD %3d exists  in DestEmitter!"), OuterLOD->LevelSetting);
						continue;
					}

					if ((OuterLOD->LevelSetting < NextInnerLOD->LevelSetting) &&
						(OuterLOD->LevelSetting > InnerLOD->LevelSetting))
					{
						// Generate this level in the new emitter...
						debugf(TEXT("NewEmitter - LOD %3d missing in DestEmitter!"), OuterLOD->LevelSetting);
					}
				}
			}

			// Now, run through the new emitter. The problem is, each emitter in the destination system 
			// will have to be updated if required.
		}

        NewEmitter->UpdateModuleLists();

		// Add to selected emitter
        DestSystem->Emitters.AddItem(NewEmitter);
	}
	else
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_4"), *NewEmitClass->GetDesc());
		return FALSE;
	}

	return TRUE;
}

// Undo/Redo support
bool WxCascade::BeginTransaction(const TCHAR* pcTransaction)
{
	if (TransactionInProgress())
	{
		FString kError(*LocalizeUnrealEd("Error_FailedToBeginTransaction"));
		kError += kTransactionName;
		check(!*kError);
		return FALSE;
	}

	GEditor->Trans->Begin(pcTransaction);
	kTransactionName = FString(pcTransaction);
	bTransactionInProgress = TRUE;

	return TRUE;
}

bool WxCascade::EndTransaction(const TCHAR* pcTransaction)
{
	if (!TransactionInProgress())
	{
		FString kError(*LocalizeUnrealEd("Error_FailedToEndTransaction"));
		kError += kTransactionName;
		check(!*kError);
		return FALSE;
	}

	if (appStrcmp(*kTransactionName, pcTransaction) != 0)
	{
		debugf(TEXT("Cascade -   EndTransaction = %s --- Curr = %s"), 
			pcTransaction, *kTransactionName);
		return FALSE;
	}

	GEditor->Trans->End();

	kTransactionName = TEXT("");
	bTransactionInProgress = FALSE;

	return TRUE;
}

bool WxCascade::TransactionInProgress()
{
	return bTransactionInProgress;
}

void WxCascade::ModifySelectedObjects()
{
	if (SelectedEmitter)
	{
		ModifyEmitter(SelectedEmitter);
	}
	if (SelectedModule)
	{
		SelectedModule->Modify();
	}
}

void WxCascade::ModifyParticleSystem()
{
	PartSys->Modify();
	PartSysComp->Modify();
}

void WxCascade::ModifyEmitter(UParticleEmitter* Emitter)
{
	if (Emitter)
	{
		Emitter->Modify();
		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
			if (LODLevel)
			{
				LODLevel->Modify();
			}
		}
	}
}

void WxCascade::CascadeUndo()
{
	GEditor->Trans->Undo();

	CascadeTouch();
}

void WxCascade::CascadeRedo()
{
	GEditor->Trans->Redo();

	CascadeTouch();
}

void WxCascade::CascadeTouch()
{
	// Touch the module lists in each emitter.
	for (INT ii = 0; ii < PartSys->Emitters.Num(); ii++)
	{
		UParticleEmitter* pkEmitter = PartSys->Emitters(ii);
		pkEmitter->UpdateModuleLists();
	}
	PartSysComp->ResetParticles();
	PartSysComp->InitializeSystem();
	// 'Refresh' the viewport
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::FillLODCombo()
{
	if (!ToolBar)
	{
		return;
	}

	ToolBar->LODCombo->Clear();

	// Add the strings and data for each LOD level
	ToolBar->LODCombo->Append(wxString(*LocalizeUnrealEd("LODUnset")));

	if (PartSys && (PartSys->Emitters.Num() > 0) && (PartSys->Emitters(0) != NULL))
	{
		// Grab emitter 0.
		UParticleEmitter* Emitter = PartSys->Emitters(0);
		check(Emitter);

		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
			if (LODLevel)
			{
				if (LODLevel->LevelSetting == 0)
				{
					ToolBar->LODCombo->Append(wxString(*LocalizeUnrealEd("LODHighest")));
				}
				else
				if (LODLevel->LevelSetting == 100)
				{
					ToolBar->LODCombo->Append(wxString(*LocalizeUnrealEd("LODLowest")));
				}
				else
				{
					FString	ValueString	= FString::Printf(TEXT("%s %d"), 
						*LocalizeUnrealEd("CascadeLODLabel"), 
						LODLevel->LevelSetting);
					ToolBar->LODCombo->Append(wxString(*ValueString));
				}
			}
		}
	}
	else
	{
		ToolBar->LODCombo->Append(wxString(*LocalizeUnrealEd("LODHighest")));
		ToolBar->LODCombo->Append(wxString(*LocalizeUnrealEd("LODLowest")));
	}

	UpdateLODCombo();
}

void WxCascade::UpdateLODCombo()
{
	if (!ToolBar)
	{
		return;
	}

	// Grab the settings
	INT		Value	= ToolBar->LODSlider->GetValue();
	FString	Compare	= FString::Printf(TEXT("%s %d"), *LocalizeUnrealEd("CascadeLODLabel"), Value);

	INT SetIndex	= -1;
	// Set the combo
	for (INT Index = 0; Index < ToolBar->LODCombo->GetCount(); Index++)
	{
		FString	ComboString	= FString(ToolBar->LODCombo->GetString(Index));
		if (Value == 100)
		{
			if (appStrcmp(*ComboString, *LocalizeUnrealEd("LODLowest")) == 0)
			{
				SetIndex	= Index;
				break;
			}
		}
		else
		if (Value == 0)
		{
			if (appStrcmp(*ComboString, *LocalizeUnrealEd("LODHighest")) == 0)
			{
				SetIndex	= Index;
				break;
			}
		}
		else
		{
			if (appStrcmp(*ComboString, *Compare) == 0)
			{
				SetIndex	= Index;
				break;
			}
		}
	}

	if (SetIndex == -1)
	{
		ToolBar->LODCombo->SetSelection(0);
	}
	else
	{
		ToolBar->LODCombo->SetSelection(SetIndex);
	}
	
	// Set the LOD level on the particle system
	if (PartSysComp)
	{
		PartSysComp->SetEditorLODLevel(Value);
	}
}

// PostProces
/**
 *	Update the post process chain according to the show options
 */
void WxCascade::UpdatePostProcessChain()
{
	if (DefaultPostProcess && PreviewVC)
	{
		UPostProcessEffect* BloomEffect = NULL;
		UPostProcessEffect* DOFEffect = NULL;
		UPostProcessEffect* MotionBlurEffect = NULL;
		UPostProcessEffect* PPVolumeEffect = NULL;

		for (INT EffectIndex = 0; EffectIndex < DefaultPostProcess->Effects.Num(); EffectIndex++)
		{
			UPostProcessEffect* Effect = DefaultPostProcess->Effects(EffectIndex);
			if (Effect)
			{
				if (Effect->EffectName.ToString() == FString(TEXT("CascadeDOFAndBloom")))
				{
					BloomEffect = Effect;
					DOFEffect = Effect;
				}
				else
				if (Effect->EffectName.ToString() == FString(TEXT("CascadeMotionBlur")))
				{
					MotionBlurEffect = Effect;
				}
				else
				if (Effect->EffectName.ToString() == FString(TEXT("CascadePPVolumeMaterial")))
				{
					PPVolumeEffect = Effect;
				}
			}
		}

		if (BloomEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_BLOOM)
			{
				BloomEffect->bShowInEditor = TRUE;
				BloomEffect->bShowInGame = TRUE;
			}
			else
			{
				BloomEffect->bShowInEditor = FALSE;
				BloomEffect->bShowInGame = FALSE;
			}
		}
		if (DOFEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_DOF)
			{
				DOFEffect->bShowInEditor = TRUE;
				DOFEffect->bShowInGame = TRUE;
			}
			else
			{
				DOFEffect->bShowInEditor = FALSE;
				DOFEffect->bShowInGame = FALSE;
			}
		}
		if (MotionBlurEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_MOTIONBLUR)
			{
				MotionBlurEffect->bShowInEditor = TRUE;
				MotionBlurEffect->bShowInGame = TRUE;
			}
			else
			{
				MotionBlurEffect->bShowInEditor = FALSE;
				MotionBlurEffect->bShowInGame = FALSE;
			}
		}
		if (PPVolumeEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_PPVOLUME)
			{
				PPVolumeEffect->bShowInEditor = TRUE;
				PPVolumeEffect->bShowInGame = TRUE;
			}
			else
			{
				PPVolumeEffect->bShowInEditor = FALSE;
				PPVolumeEffect->bShowInGame = FALSE;
			}
		}
	}
	else
	{
		warnf(TEXT("CASCADE::UpdatePostProcessChain> Nno post process chain."));
	}
}

/**
 */
void WxCascade::SetLODValue(INT LODSetting)
{
	if (ToolBar)
	{
		FString	ValueStr = FString::Printf(TEXT("%d"), LODSetting);
		ToolBar->LODSetting->SetValue(*ValueStr);
		ToolBar->LODSlider->SetValue(LODSetting);
	}

	if (PartSys)
	{
		check(LODSetting >= 0);
		PartSys->EditorLODSetting	= LODSetting;
	}
}

BEGIN_EVENT_TABLE(WxCascade, wxFrame)
	EVT_SIZE(WxCascade::OnSize)
	EVT_MENU(IDM_CASCADE_RENAME_EMITTER, WxCascade::OnRenameEmitter)
//	EVT_MENU_RANGE( IDM_CASCADE_NEW_SPRITEEMITTER, IDM_CASCADE_NEW_TRAILEMITTER, WxCascade::OnNewEmitter )
	EVT_MENU_RANGE(	IDM_CASCADE_NEW_EMITTER_START, IDM_CASCADE_NEW_EMITTER_END, WxCascade::OnNewEmitter )
	EVT_MENU(IDM_CASCADE_SELECT_PARTICLESYSTEM, WxCascade::OnSelectParticleSystem)
	EVT_MENU(IDM_CASCADE_PSYS_NEW_EMITTER_BEFORE, WxCascade::OnNewEmitterBefore)
	EVT_MENU(IDM_CASCADE_PSYS_NEW_EMITTER_AFTER, WxCascade::OnNewEmitterAfter) 
	EVT_MENU_RANGE( IDM_CASCADE_NEW_MODULE_START, IDM_CASCADE_NEW_MODULE_END, WxCascade::OnNewModule )
	EVT_MENU(IDM_CASCADE_ADD_SELECTED_MODULE, WxCascade::OnAddSelectedModule)
	EVT_MENU(IDM_CASCADE_COPY_MODULE, WxCascade::OnCopyModule)
	EVT_MENU(IDM_CASCADE_PASTE_MODULE, WxCascade::OnPasteModule)
	EVT_MENU( IDM_CASCADE_DELETE_MODULE, WxCascade::OnDeleteModule )
	EVT_MENU( IDM_CASCADE_ENABLE_MODULE, WxCascade::OnEnableModule )
	EVT_MENU( IDM_CASCADE_RESET_MODULE, WxCascade::OnResetModule )
	EVT_MENU(IDM_CASCADE_DUPLICATE_EMITTER, WxCascade::OnDuplicateEmitter)
	EVT_MENU(IDM_CASCADE_DUPLICATE_SHARE_EMITTER, WxCascade::OnDuplicateEmitter)
	EVT_MENU( IDM_CASCADE_DELETE_EMITTER, WxCascade::OnDeleteEmitter )
	EVT_MENU(IDM_CASCADE_EXPORT_EMITTER, WxCascade::OnExportEmitter)
	EVT_MENU(IDM_CASCADE_CONVERT_RAIN_EMITTER, WxCascade::OnConvertRainEmitter)
	EVT_MENU_RANGE( IDM_CASCADE_SIM_PAUSE, IDM_CASCADE_SIM_NORMAL, WxCascade::OnMenuSimSpeed )
	EVT_MENU( IDM_CASCADE_SAVECAM, WxCascade::OnSaveCam )
#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
	EVT_MENU( IDM_CASCADE_VIEW_DUMP, WxCascade::OnViewModuleDump)
#endif	//#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
	EVT_MENU( IDM_CASCADE_RESETSYSTEM, WxCascade::OnResetSystem )
	EVT_MENU( IDM_CASCADE_RESETINLEVEL, WxCascade::OnResetInLevel )
	EVT_TOOL( IDM_CASCADE_ORBITMODE, WxCascade::OnOrbitMode )
	EVT_TOOL(IDM_CASCADE_WIREFRAME, WxCascade::OnWireframe)
	EVT_TOOL(IDM_CASCADE_BOUNDS, WxCascade::OnBounds)
	EVT_TOOL(IDM_CASCADE_POSTPROCESS, WxCascade::OnPostProcess)
	EVT_TOOL(IDM_CASCADE_TOGGLEGRID, WxCascade::OnToggleGrid)
	EVT_TOOL(IDM_CASCADE_PLAY, WxCascade::OnPlay)
	EVT_TOOL(IDM_CASCADE_PAUSE, WxCascade::OnPause)
	EVT_TOOL(IDM_CASCADE_SPEED_100,	WxCascade::OnSpeed)
	EVT_TOOL(IDM_CASCADE_SPEED_50, WxCascade::OnSpeed)
	EVT_TOOL(IDM_CASCADE_SPEED_25, WxCascade::OnSpeed)
	EVT_TOOL(IDM_CASCADE_SPEED_10, WxCascade::OnSpeed)
	EVT_TOOL(IDM_CASCADE_SPEED_1, WxCascade::OnSpeed)
	EVT_TOOL(IDM_CASCADE_LOOPSYSTEM, WxCascade::OnLoopSystem)
	EVT_TOOL(IDM_CASCADE_REALTIME, WxCascade::OnRealtime)
	EVT_TOOL(IDM_CASCADE_BACKGROUND_COLOR, WxCascade::OnBackgroundColor)
	EVT_TOOL(IDM_CASCADE_TOGGLE_WIRE_SPHERE, WxCascade::OnToggleWireSphere)
	EVT_TOOL(IDM_CASCADE_UNDO, WxCascade::OnUndo)
	EVT_TOOL(IDM_CASCADE_REDO, WxCascade::OnRedo)
	EVT_TOOL(IDM_CASCADE_PERFORMANCE_CHECK, WxCascade::OnPerformanceCheck)
	EVT_COMMAND_SCROLL_ENDSCROLL(IDM_CASCADE_LOD_SLIDER, WxCascade::OnLODSlider)
	EVT_TEXT(IDM_CASCADE_LOD_SETTING, WxCascade::OnLODSetting)
	EVT_COMBOBOX(IDM_CASCADE_LOD_COMBO, WxCascade::OnLODCombo)
	EVT_TOOL(IDM_CASCADE_LOD_LOW, WxCascade::OnLODLow)
	EVT_TOOL(IDM_CASCADE_LOD_LOWER, WxCascade::OnLODLower)
	EVT_TOOL(IDM_CASCADE_LOD_ADD, WxCascade::OnLODAdd)
	EVT_TOOL(IDM_CASCADE_LOD_HIGHER, WxCascade::OnLODHigher)
	EVT_TOOL(IDM_CASCADE_LOD_HIGH, WxCascade::OnLODHigh)
	EVT_TOOL(IDM_CASCADE_LOD_DELETE, WxCascade::OnLODDelete)
	EVT_TOOL(IDM_CASCADE_LOD_REGEN, WxCascade::OnRegenerateLowestLOD)
	EVT_TOOL(IDM_CASCADE_LOD_REGENDUP, WxCascade::OnRegenerateLowestLODDuplicateHighest)
	EVT_MENU( IDM_CASCADE_VIEW_AXES, WxCascade::OnViewAxes )
	EVT_MENU(IDM_CASCADE_VIEW_COUNTS, WxCascade::OnViewCounts)
	EVT_MENU(IDM_CASCADE_VIEW_TIMES, WxCascade::OnViewTimes)
	EVT_MENU(IDM_CASCADE_VIEW_DISTANCE, WxCascade::OnViewDistance)
	EVT_MENU(IDM_CASCADE_VIEW_GEOMETRY, WxCascade::OnViewGeometry)
	EVT_MENU(IDM_CASCADE_VIEW_GEOMETRY_PROPERTIES, WxCascade::OnViewGeometryProperties)
	EVT_MENU(IDM_CASCADE_RESET_PEAK_COUNTS, WxCascade::OnResetPeakCounts)
	EVT_MENU(IDM_CASCADE_CONVERT_TO_UBER, WxCascade::OnUberConvert)
	EVT_MENU(IDM_CASCADE_REGENERATE_LOWESTLOD, WxCascade::OnRegenerateLowestLOD)
	EVT_MENU(IDM_CASCADE_SAVE_PACKAGE, WxCascade::OnSavePackage)
	EVT_MENU(IDM_CASCADE_SIM_RESTARTONFINISH, WxCascade::OnLoopSimulation )
	EVT_MENU(IDM_CASC_SHOWPP_BLOOM, WxCascade::OnShowPPBloom)
	EVT_MENU(IDM_CASC_SHOWPP_DOF, WxCascade::OnShowPPDOF)
	EVT_MENU(IDM_CASC_SHOWPP_MOTIONBLUR, WxCascade::OnShowPPMotionBlur)
	EVT_MENU(IDM_CASC_SHOWPP_PPVOLUME, WxCascade::OnShowPPVolumeMaterial)
END_EVENT_TABLE()


#define CASCADE_NUM_SASHES		4

WxCascade::WxCascade(wxWindow* InParent, wxWindowID InID, UParticleSystem* InPartSys) : 
	wxFrame(InParent, InID, TEXT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR, 
		InPartSys ? *(InPartSys->GetPathName()) : TEXT("EMPTY")),
	FDockingParent(this),
	MenuBar(NULL),
	ToolBar(NULL),
	PreviewVC(NULL)
{
	check(InPartSys);
	InPartSys->EditorLODSetting	= 0;

	DefaultPostProcessName = TEXT("");
    DefaultPostProcess = NULL;

	EditorOptions = ConstructObject<UCascadeOptions>(UCascadeOptions::StaticClass());
	check(EditorOptions);

	// Load the desired window position from .ini file
	FWindowUtil::LoadPosSize(TEXT("CascadeEditor"), this, 256, 256, 1024, 768);
	
	// Make sure we have a list of available particle modules
	WxCascade::InitParticleModuleClasses();
	WxCascade::InitParticleEmitterClasses();

	// Set up pointers to interp objects
	PartSys = InPartSys;

	// Set up for undo/redo!
	PartSys->SetFlags(RF_Transactional);

	for (INT ii = 0; ii < PartSys->Emitters.Num(); ii++)
	{
		UParticleEmitter* Emitter = PartSys->Emitters(ii);
		if (Emitter)
		{
			Emitter->SetFlags(RF_Transactional);
			for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* LODLevel = Emitter->GetLODLevel(LODIndex);
				if (LODLevel)
				{
					LODLevel->SetFlags(RF_Transactional);
					check(LODLevel->RequiredModule);
					LODLevel->RequiredModule->SetFlags(RF_Transactional);
					for (INT jj = 0; jj < LODLevel->Modules.Num(); jj++)
					{
						UParticleModule* pkModule = LODLevel->Modules(jj);
						pkModule->SetFlags(RF_Transactional);
					}
				}
			}
		}
	}

	// Nothing selected initially
	SelectedEmitter = NULL;
	SelectedModule = NULL;

	CopyModule = NULL;
	CopyEmitter = NULL;

	CurveToReplace = NULL;

	bResetOnFinish = TRUE;
	bPendingReset = FALSE;
	bOrbitMode = TRUE;
	bWireframe = FALSE;
	bBounds = FALSE;
	SimSpeed = 1.0f;

	bTransactionInProgress = FALSE;

	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create(this, this);

	// Create particle system preview
	WxCascadePreview* PreviewWindow = new WxCascadePreview( this, -1, this );
	PreviewVC = PreviewWindow->CascadePreviewVC;
	PreviewVC->SetPreviewCamera(PartSys->ThumbnailAngle, PartSys->ThumbnailDistance);
	PreviewVC->SetViewLocationForOrbiting( FVector(0.f,0.f,0.f) );
	if (EditorOptions->bShowGrid == TRUE)
	{
		PreviewVC->ShowFlags |= SHOW_Grid;
	}
	else
	{
		PreviewVC->ShowFlags &= ~SHOW_Grid;
	}

	PreviewVC->bDrawParticleCounts = EditorOptions->bShowParticleCounts;
	PreviewVC->bDrawParticleTimes = EditorOptions->bShowParticleTimes;
	PreviewVC->bDrawSystemDistance = EditorOptions->bShowParticleDistance;

	UpdatePostProcessChain();

	// Create new curve editor setup if not already done
	if (!PartSys->CurveEdSetup)
	{
		PartSys->CurveEdSetup = ConstructObject<UInterpCurveEdSetup>( UInterpCurveEdSetup::StaticClass(), PartSys, NAME_None, RF_NotForClient | RF_NotForServer | RF_Transactional );
	}

	// Create graph editor to work on systems CurveEd setup.
	CurveEd = new WxCurveEditor( this, -1, PartSys->CurveEdSetup );
	// Register this window with the Curve editor so we will be notified of various things.
	CurveEd->SetNotifyObject(this);

	// Create emitter editor
	EmitterEdWindow = new WxCascadeEmitterEd(this, -1, this);
	EmitterEdVC = EmitterEdWindow->EmitterEdVC;

	// Create Docking Windows
	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Bottom, *FString::Printf( *LocalizeUnrealEd("PropertiesCaption_F"), *PartSys->GetName() ), *LocalizeUnrealEd("Properties") );
		AddDockingWindow(CurveEd, FDockingParent::DH_Bottom, *FString::Printf( *LocalizeUnrealEd("CurveEditorCaption_F"), *PartSys->GetName() ), *LocalizeUnrealEd("CurveEditor") );
		AddDockingWindow(PreviewWindow, FDockingParent::DH_Left, *FString::Printf( *LocalizeUnrealEd("PreviewCaption_F"), *PartSys->GetName() ), *LocalizeUnrealEd("Preview") );
		
		SetDockHostSize(FDockingParent::DH_Left, 500);

		wxPane* PreviewPane = new wxPane( this );
		{
			PreviewPane->ShowHeader(false);
			PreviewPane->ShowCloseButton( false );
			PreviewPane->SetClient(EmitterEdWindow);
		}
		LayoutManager->SetLayout( wxDWF_SPLITTER_BORDERS, PreviewPane );

		// Try to load a existing layout for the docking windows.
		LoadDockingLayout();
	}

	// Create menu bar
	MenuBar = new WxCascadeMenuBar(this);
	AppendDockingMenu(MenuBar);
	SetMenuBar( MenuBar );

	// Create tool bar
	ToolBar	= NULL;
	ToolBar = new WxCascadeToolBar( this, -1 );
	SetToolBar( ToolBar );
	FillLODCombo();

	// Set window title to particle system we are editing.
	SetTitle( *FString::Printf( *LocalizeUnrealEd("CascadeCaption_F"), *PartSys->GetName() ) );

	// Set emitter to use the particle system we are editing.
	PartSysComp->SetTemplate(PartSys);

	SetSelectedModule(NULL, NULL);

	PreviewVC->BackgroundColor = EditorOptions->BackgroundColor;

	// Setup the accelerator table
	TArray<wxAcceleratorEntry> Entries;
	// Allow derived classes an opportunity to register accelerator keys.
	// Bind SPACE to reset.
	Entries.AddItem(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_SPACE, IDM_CASCADE_RESETSYSTEM));
	// Create the new table with these.
	SetAcceleratorTable(wxAcceleratorTable(Entries.Num(),Entries.GetTypedData()));

	PartSysComp->InitializeSystem();
	PartSysComp->ActivateSystem();
}

WxCascade::~WxCascade()
{
	if (PartSys)
	{
		UPackage* Package = PartSys->GetOutermost();
		if (Package && (Package->IsDirty() == TRUE))
		{
			for (INT EmitterIndex = 0; EmitterIndex < PartSys->Emitters.Num(); EmitterIndex++)
			{
				UParticleEmitter* Emitter = PartSys->Emitters(EmitterIndex);
				if (Emitter)
				{
					// Check the LOD levels to see if they are unedited.
					// If so, re-generate the lowest.
					if (Emitter->LODLevels.Num() == 2)
					{
						// There are only two levels, so it is likely unedited.
						UBOOL bRegenerate = TRUE;
						UParticleLODLevel* LODLevel = Emitter->LODLevels(1);
						if (LODLevel)
						{
							if (LODLevel->RequiredModule->bEditable == TRUE)
							{
								bRegenerate = FALSE;
							}
							else
							{
								for (INT ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++)
								{
									UParticleModule* Module = LODLevel->Modules(ModuleIndex);
									if (Module)
									{
										if (Module->bEditable == TRUE)
										{
											bRegenerate = FALSE;
											break;
										}
									}
								}
							}

							if (bRegenerate == TRUE)
							{
								Emitter->LODLevels.Remove(1);
								// The re-generation will happen below...
								if (Emitter->AutogenerateLowestLODLevel(PartSys->bRegenerateLODDuplicate) == TRUE)
								{
									Emitter->UpdateModuleLists();
								}

								wxCommandEvent DummyEvent;
								OnResetInLevel(DummyEvent);
							}
						}
					}
				}
			}
		}
	}

	GEditor->Trans->Reset(TEXT("QuitCascade"));

	// Save the desired window position to the .ini file
	FWindowUtil::SavePosSize(TEXT("CascadeEditor"), this);
	
	SaveDockingLayout();

	// Destroy preview viewport before we destroy the level.
	GEngine->Client->CloseViewport(PreviewVC->Viewport);
	PreviewVC->Viewport = NULL;

	if (PreviewVC->FloorComponent)
	{
		EditorOptions->FloorPosition = PreviewVC->FloorComponent->Translation;
		EditorOptions->FloorRotation = PreviewVC->FloorComponent->Rotation;
		EditorOptions->FloorScale = PreviewVC->FloorComponent->Scale;
		EditorOptions->FloorScale3D = PreviewVC->FloorComponent->Scale3D;

		if (PreviewVC->FloorComponent->StaticMesh)
		{
			if (PreviewVC->FloorComponent->StaticMesh->GetOuter())
			{
				EditorOptions->FloorMesh = PreviewVC->FloorComponent->StaticMesh->GetOuter()->GetName();
				EditorOptions->FloorMesh += TEXT(".");
			}
			else
			{
				warnf(TEXT("Unable to locate Cascade floor mesh outer..."));
				EditorOptions->FloorMesh = TEXT("");
			}

			EditorOptions->FloorMesh += PreviewVC->FloorComponent->StaticMesh->GetName();
		}
		else
		{
			EditorOptions->FloorMesh += FString::Printf(TEXT("EditorMeshes.AnimTreeEd_PreviewFloor"));
		}

		FString	Name;

		Name = EditorOptions->FloorMesh;
		debugf(TEXT("StaticMesh       = %s"), *Name);

		EditorOptions->SaveConfig();
	}

	delete PreviewVC;
	PreviewVC = NULL;

	delete PropertyWindow;
}

wxToolBar* WxCascade::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
{
	if (name == TEXT("Cascade"))
		return new WxCascadeToolBar(this, -1);

	wxToolBar*	ReturnToolBar = OnCreateToolBar(style, id, name);
	if (ReturnToolBar)
	{
		FillLODCombo();
	}

	return ReturnToolBar;
}

void WxCascade::Serialize(FArchive& Ar)
{
	PreviewVC->Serialize(Ar);

	Ar << EditorOptions;
}

/**
 * Pure virtual that must be overloaded by the inheriting class. It will
 * be called from within UnLevTick.cpp after ticking all actors.
 *
 * @param DeltaTime	Game time passed since the last call.
 */
static const DOUBLE ResetInterval = 0.5f;
void WxCascade::Tick( FLOAT DeltaTime )
{
	// Don't bother ticking at all if paused.
	if (PreviewVC->TimeScale > KINDA_SMALL_NUMBER)
	{
		const FLOAT fSaveUpdateDelta = PartSys->UpdateTime_Delta;
		if (PreviewVC->TimeScale < 1.0f)
		{
			PartSys->UpdateTime_Delta *= PreviewVC->TimeScale;
		}

		PartSysComp->Tick(PreviewVC->TimeScale * DeltaTime);

		PreviewVC->TotalTime += PreviewVC->TimeScale * DeltaTime;

		PartSys->UpdateTime_Delta = fSaveUpdateDelta;
	}

	// If we are doing auto-reset
	if(bResetOnFinish)
	{
		UParticleSystemComponent* PartComp = PartSysComp;

		// If system has finish, pause for a bit before resetting.
		if(bPendingReset)
		{
			if(PreviewVC->TotalTime > ResetTime)
			{
				PartComp->ResetParticles();
				PartComp->ActivateSystem();

				bPendingReset = FALSE;
			}
		}
		else
		{
			if( PartComp->HasCompleted() )
			{
				bPendingReset = TRUE;
				ResetTime = PreviewVC->TotalTime + ResetInterval;
			}
		}
	}
}

// FCurveEdNotifyInterface
/**
 *	PreEditCurve
 *	Called by the curve editor when N curves are about to change
 *
 *	@param	CurvesAboutToChange		An array of the curves about to change
 */
void WxCascade::PreEditCurve(TArray<UObject*> CurvesAboutToChange)
{
	debugf(TEXT("CASCADE: PreEditCurve - %2d curves"), CurvesAboutToChange.Num());

//	InterpEdTrans->BeginSpecial(*LocalizeUnrealEd("CurveEdit"));
	BeginTransaction(*LocalizeUnrealEd("CurveEdit"));

	// Call Modify on all tracks with keys selected
	for (INT i = 0; i < CurvesAboutToChange.Num(); i++)
	{
		// If this keypoint is from a distribution, call Modify on it to back up its state.
		UDistributionFloat* DistFloat = Cast<UDistributionFloat>(CurvesAboutToChange(i));
		if (DistFloat)
		{
			DistFloat->Modify();
		}
		UDistributionVector* DistVector = Cast<UDistributionVector>(CurvesAboutToChange(i));
		if (DistVector)
		{
			DistVector->Modify();
		}
	}
}

/**
 *	PostEditCurve
 *	Called by the curve editor when the edit has completed.
 */
void WxCascade::PostEditCurve()
{
	debugf(TEXT("CASCADE: PostEditCurve"));
//	InterpEdTrans->EndSpecial();
	this->EndTransaction(*LocalizeUnrealEd("CurveEdit"));
}

/**
 *	MovedKey
 *	Called by the curve editor when a key has been moved.
 */
void WxCascade::MovedKey()
{
}

/**
 *	DesireUndo
 *	Called by the curve editor when an Undo is requested.
 */
void WxCascade::DesireUndo()
{
	debugf(TEXT("CASCADE: DesireUndo"));
//	InterpEdUndo();
	CascadeUndo();
}

/**
 *	DesireRedo
 *	Called by the curve editor when an Redo is requested.
 */
void WxCascade::DesireRedo()
{
	debugf(TEXT("CASCADE: DesireRedo"));
//	InterpEdRedo();
	CascadeRedo();
}

void WxCascade::OnSize( wxSizeEvent& In )
{
	In.Skip();
	Refresh();
}

///////////////////////////////////////////////////////////////////////////////////////
// Menu Callbacks

void WxCascade::OnRenameEmitter(wxCommandEvent& In)
{
	if (!SelectedEmitter)
		return;

	BeginTransaction(TEXT("EmitterRename"));

	PartSys->PreEditChange(NULL);
	PartSysComp->PreEditChange(NULL);

	FName& CurrentName = SelectedEmitter->GetEmitterName();

	WxDlgGenericStringEntry dlg;
	if (dlg.ShowModal(TEXT("RenameEmitter"), TEXT("Name"), *CurrentName.ToString()) == wxID_OK)
	{
//		element->Modify();
		FName newName = FName(*(dlg.GetEnteredString()));
		SelectedEmitter->SetEmitterName(newName);
//		element->Rename(*newElementName,Container);
	}

	PartSysComp->PostEditChange(NULL);
	PartSys->PostEditChange(NULL);

	EndTransaction(TEXT("EmitterRename"));

	// Refresh viewport
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnNewEmitter(wxCommandEvent& In)
{
	BeginTransaction(TEXT("NewEmitter"));
	PartSys->PreEditChange(NULL);
	PartSysComp->PreEditChange(NULL);

	// Find desired class of new module.
	INT NewEmitClassIndex = In.GetId() - IDM_CASCADE_NEW_EMITTER_START;
	check( NewEmitClassIndex >= 0 && NewEmitClassIndex < ParticleEmitterClasses.Num() );

	UClass* NewEmitClass = ParticleEmitterClasses(NewEmitClassIndex);
	if (NewEmitClass == UParticleSpriteEmitter::StaticClass())
	{
		check( NewEmitClass->IsChildOf(UParticleEmitter::StaticClass()) );

		// Construct it
		UParticleEmitter* NewEmitter = ConstructObject<UParticleEmitter>(NewEmitClass, PartSys, NAME_None, RF_Transactional);
		UParticleLODLevel* LODLevel	= NewEmitter->GetLODLevel(0);
		if (LODLevel == NULL)
		{
			// Generate the HighLOD level, and the default lowest level
			INT Index = NewEmitter->CreateLODLevel(0);
			LODLevel = NewEmitter->GetLODLevel(0);
		}

		check(LODLevel);

		LODLevel->RequiredModule->EmitterEditorColor	= FColor::MakeRandomColor();
		
        // Set to sensible default values
		NewEmitter->SetToSensibleDefaults();

        // Handle special cases...
		if (NewEmitClass == UParticleSpriteEmitter::StaticClass())
		{
			// For handyness- use currently selected material for new emitter (or default if none selected)
			UParticleSpriteEmitter* NewSpriteEmitter = (UParticleSpriteEmitter*)NewEmitter;
			UMaterialInterface* CurrentMaterial = GEditor->GetSelectedObjects()->GetTop<UMaterialInterface>();
			if (CurrentMaterial)
			{
				LODLevel->RequiredModule->Material = CurrentMaterial;
			}
			else
			{
				LODLevel->RequiredModule->Material = LoadObject<UMaterialInterface>(NULL, TEXT("EngineMaterials.DefaultParticle"), NULL, LOAD_None, NULL);
			}
		}

		if (NewEmitter->AutogenerateLowestLODLevel(PartSys->bRegenerateLODDuplicate) == FALSE)
		{
			warnf(TEXT("Failed to autogenerate lowest LOD level!"));
		}

		// Generate all the levels that are present in other emitters...
		if (PartSys->Emitters.Num() > 0)
		{
			UParticleEmitter* ExistingEmitter = PartSys->Emitters(0);
			if (ExistingEmitter->LODLevels.Num() > 2)
			{
				debugf(TEXT("Generating existing LOD levels..."));

				// Walk the LOD levels of the existing emitter...
				UParticleLODLevel* ExistingLOD;
				UParticleLODLevel* NewLOD_Prev = NewEmitter->LODLevels(0);
				UParticleLODLevel* NewLOD_Next = NewEmitter->LODLevels(1);

				check(NewLOD_Prev && (NewLOD_Prev->LevelSetting == 0));
				check(NewLOD_Next && (NewLOD_Next->LevelSetting == 100));

				for (INT LODIndex = 1; LODIndex < ExistingEmitter->LODLevels.Num() - 1; LODIndex++)
				{
					ExistingLOD = ExistingEmitter->LODLevels(LODIndex);

					// Add this one
					INT ResultIndex = NewEmitter->CreateLODLevel(ExistingLOD->LevelSetting, TRUE);

					UParticleLODLevel* NewLODLevel	= NewEmitter->LODLevels(ResultIndex);
					check(NewLODLevel);
					NewLODLevel->UpdateModuleLists();
				}
			}
		}

		NewEmitter->UpdateModuleLists();

		NewEmitter->PostEditChange(NULL);
		if (NewEmitter)
		{
			NewEmitter->SetFlags(RF_Transactional);
			for (INT LODIndex = 0; LODIndex < NewEmitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* LODLevel = NewEmitter->GetLODLevel(LODIndex);
				if (LODLevel)
				{
					LODLevel->SetFlags(RF_Transactional);
					check(LODLevel->RequiredModule);
					LODLevel->RequiredModule->SetFlags(RF_Transactional);
					for (INT jj = 0; jj < LODLevel->Modules.Num(); jj++)
					{
						UParticleModule* pkModule = LODLevel->Modules(jj);
						pkModule->SetFlags(RF_Transactional);
					}
				}
			}
		}

        // Add to selected emitter
        PartSys->Emitters.AddItem(NewEmitter);

		// Setup the LOD distances
		if (PartSys->LODDistances.Num() == 0)
		{
			UParticleEmitter* Emitter = PartSys->Emitters(0);
			if (Emitter)
			{
				PartSys->LODDistances.Add(Emitter->LODLevels.Num());
				for (INT LODIndex = 0; LODIndex < PartSys->LODDistances.Num(); LODIndex++)
				{
					PartSys->LODDistances(LODIndex) = LODIndex * 2500.0f;
				}
			}
		}
	}
	else
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_4"), *NewEmitClass->GetDesc());
	}

	PartSysComp->PostEditChange(NULL);
	PartSys->PostEditChange(NULL);

	EndTransaction(TEXT("NewEmitter"));

	// Refresh viewport
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnSelectParticleSystem( wxCommandEvent& In )
{
	SetSelectedEmitter(NULL);
}

void WxCascade::OnNewEmitterBefore( wxCommandEvent& In )
{
	if ((SelectedEmitter != NULL) && (PartSys != NULL))
	{
		INT EmitterCount = PartSys->Emitters.Num();
		INT EmitterIndex = -1;
		for (INT Index = 0; Index < EmitterCount; Index++)
		{
			UParticleEmitter* CheckEmitter = PartSys->Emitters(Index);
			if (SelectedEmitter == CheckEmitter)
			{
				EmitterIndex = Index;
				break;
			}
		}

		if (EmitterIndex != -1)
		{
			debugf(TEXT("Insert New Emitter Before %d"), EmitterIndex);

			// Fake create it at the end
			wxCommandEvent DummyIn;
			DummyIn.SetId(IDM_CASCADE_NEW_EMITTER_START + 1);
			OnNewEmitter(DummyIn);

			if (EmitterCount + 1 == PartSys->Emitters.Num())
			{
				UParticleEmitter* NewEmitter = PartSys->Emitters(EmitterCount);
				SetSelectedEmitter(NewEmitter);
				MoveSelectedEmitter(EmitterIndex - EmitterCount);
			}
		}
	}
}

void WxCascade::OnNewEmitterAfter( wxCommandEvent& In )
{
	if ((SelectedEmitter != NULL) && (PartSys != NULL))
	{
		INT EmitterCount = PartSys->Emitters.Num();
		INT EmitterIndex = -1;
		for (INT Index = 0; Index < EmitterCount; Index++)
		{
			UParticleEmitter* CheckEmitter = PartSys->Emitters(Index);
			if (SelectedEmitter == CheckEmitter)
			{
				EmitterIndex = Index;
				break;
			}
		}

		if (EmitterIndex != -1)
		{
			debugf(TEXT("Insert New Emitter After  %d"), EmitterIndex);

			// Fake create it at the end
			wxCommandEvent DummyIn;
			DummyIn.SetId(IDM_CASCADE_NEW_EMITTER_START + 1);
			OnNewEmitter(DummyIn);

			if (EmitterCount + 1 == PartSys->Emitters.Num())
			{
				UParticleEmitter* NewEmitter = PartSys->Emitters(EmitterCount);
				SetSelectedEmitter(NewEmitter);
				if (EmitterIndex + 1 < EmitterCount)
				{
					MoveSelectedEmitter(EmitterIndex - EmitterCount + 1);
				}
			}
		}
	}
}

void WxCascade::OnNewModule(wxCommandEvent& In)
{
	if (!SelectedEmitter)
		return;

	// Find desired class of new module.
	INT NewModClassIndex = In.GetId() - IDM_CASCADE_NEW_MODULE_START;
	check( NewModClassIndex >= 0 && NewModClassIndex < ParticleModuleClasses.Num() );

	CreateNewModule(NewModClassIndex);
}

void WxCascade::OnDuplicateEmitter(wxCommandEvent& In)
{
	// Make sure there is a selected emitter
	if (!SelectedEmitter)
		return;

	UBOOL bShare = FALSE;
	if (In.GetId() == IDM_CASCADE_DUPLICATE_SHARE_EMITTER)
	{
		bShare = TRUE;
	}

	BeginTransaction(TEXT("EmitterDuplicate"));

	PartSys->PreEditChange(NULL);
	PartSysComp->PreEditChange(NULL);

	if (!DuplicateEmitter(SelectedEmitter, PartSys, bShare))
	{
	}
	PartSysComp->PostEditChange(NULL);
	PartSys->PostEditChange(NULL);

	EndTransaction(TEXT("EmitterDuplicate"));

	// Refresh viewport
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnDeleteEmitter(wxCommandEvent& In)
{
	DeleteSelectedEmitter();
}

void WxCascade::OnExportEmitter(wxCommandEvent& In)
{
	ExportSelectedEmitter();
}

void WxCascade::OnConvertRainEmitter(wxCommandEvent& In)
{
	check(GIsEpicInternal);

	if (!SelectedEmitter)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_MustSelectEmitter"));
		return;
	}

	if (appMsgf(AMT_YesNo, *LocalizeUnrealEd("UberModuleConvertConfirm")))
	{
		BeginTransaction(TEXT("EmitterUberRainConvert"));

		// Determine which Rain module is suitable
		UParticleModuleUberRainDrops*	RainDrops	= ConstructObject<UParticleModuleUberRainDrops>(UParticleModuleUberRainDrops::StaticClass(), SelectedEmitter->GetOuter(), NAME_None, RF_Transactional);
		check(RainDrops);
		UParticleModuleUberRainImpacts* RainImpacts	= ConstructObject<UParticleModuleUberRainImpacts>(UParticleModuleUberRainImpacts::StaticClass(), SelectedEmitter->GetOuter(), NAME_None, RF_Transactional);
		check(RainImpacts);
		UParticleModuleUberRainSplashA* RainSplashA	= ConstructObject<UParticleModuleUberRainSplashA>(UParticleModuleUberRainSplashA::StaticClass(), SelectedEmitter->GetOuter(), NAME_None, RF_Transactional);
		check(RainSplashA);
		UParticleModuleUberRainSplashB* RainSplashB	= ConstructObject<UParticleModuleUberRainSplashB>(UParticleModuleUberRainSplashB::StaticClass(), SelectedEmitter->GetOuter(), NAME_None, RF_Transactional);
		check(RainSplashB);

		UParticleModuleUberBase* RainBase	= NULL;

		if (RainBase == NULL && RainDrops->IsCompatible(SelectedEmitter))
		{
			RainBase = Cast<UParticleModuleUberBase>(RainDrops);
		}

		if (RainBase == NULL && RainImpacts->IsCompatible(SelectedEmitter))
		{
			RainBase = Cast<UParticleModuleUberBase>(RainImpacts);
		}
	
		if (RainBase == NULL && RainSplashA->IsCompatible(SelectedEmitter))
		{
			RainBase = Cast<UParticleModuleUberBase>(RainSplashA);
		}
	
		if (RainBase == NULL && RainSplashB->IsCompatible(SelectedEmitter))
		{
			RainBase = Cast<UParticleModuleUberBase>(RainSplashB);
		}
		
		if (RainBase)
		{
			// First, duplicate the emitter to have a back-up...
			PartSys->PreEditChange(NULL);
			PartSysComp->PreEditChange(NULL);

			UParticleLODLevel* LODLevel = SelectedEmitter->LODLevels(0);
			UBOOL bEnabledSave	= LODLevel->bEnabled;
			LODLevel->bEnabled = FALSE;
//			SelectedEmitter->bEnabled = FALSE;
			if (!DuplicateEmitter(SelectedEmitter, PartSys))
			{
				check(!TEXT("BAD DUPLICATE IN UBER-RAIN CONVERSION!"));
			}
			LODLevel->bEnabled = bEnabledSave;
//			SelectedEmitter->bEnabled	= LODLevel->bEnabled;

			// Convert it
			if (RainBase->ConvertToUberModule(SelectedEmitter) == TRUE)
			{
				// DID IT!
			}
			else
			{
				appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedToConverToUberModule"));
			}

			PartSysComp->PostEditChange(NULL);
			PartSys->PostEditChange(NULL);
		}
		else
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedToFindUberModule"));
		}

		EndTransaction(TEXT("EmitterUberRainConvert"));

		wxCommandEvent DummyEvent;
		OnRegenerateLowestLOD( DummyEvent );

		// Mark package as dirty
		SelectedEmitter->MarkPackageDirty();

		// Redraw the module window
		EmitterEdVC->Viewport->Invalidate();
	}
}

void WxCascade::OnAddSelectedModule(wxCommandEvent& In)
{
}

void WxCascade::OnCopyModule(wxCommandEvent& In)
{
	if (SelectedModule)
		SetCopyModule(SelectedEmitter, SelectedModule);
}

void WxCascade::OnPasteModule(wxCommandEvent& In)
{
	if (!CopyModule)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_5"));
		return;
	}

	if (SelectedEmitter && CopyEmitter && (SelectedEmitter == CopyEmitter))
	{
		// Can't copy onto ourselves... Or can we
		appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_6"));
		return;
	}

	PasteCurrentModule();
}

void WxCascade::OnDeleteModule(wxCommandEvent& In)
{
	DeleteSelectedModule();
}

void WxCascade::OnEnableModule(wxCommandEvent& In)
{
	EnableSelectedModule();
}

void WxCascade::OnResetModule(wxCommandEvent& In)
{
	ResetSelectedModule();
}

void WxCascade::OnMenuSimSpeed(wxCommandEvent& In)
{
	INT Id = In.GetId();

	if (Id == IDM_CASCADE_SIM_PAUSE)
	{
		PreviewVC->TimeScale = 0.f;
		ToolBar->ToggleTool(IDM_CASCADE_PLAY, FALSE);
		ToolBar->ToggleTool(IDM_CASCADE_PAUSE, TRUE);
	}
	else
	{
		if ((Id == IDM_CASCADE_SIM_1PERCENT) || 
			(Id == IDM_CASCADE_SIM_10PERCENT) || 
			(Id == IDM_CASCADE_SIM_25PERCENT) || 
			(Id == IDM_CASCADE_SIM_50PERCENT) || 
			(Id == IDM_CASCADE_SIM_NORMAL))
		{
			ToolBar->ToggleTool(IDM_CASCADE_PLAY, TRUE);
			ToolBar->ToggleTool(IDM_CASCADE_PAUSE, FALSE);
		}

		if (Id == IDM_CASCADE_SIM_1PERCENT)
		{
			PreviewVC->TimeScale = 0.01f;
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_1, TRUE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_10, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_25, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_50, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_100, FALSE);
		}
		else if (Id == IDM_CASCADE_SIM_10PERCENT)
		{
			PreviewVC->TimeScale = 0.1f;
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_1, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_10, TRUE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_25, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_50, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_100, FALSE);
		}
		else if (Id == IDM_CASCADE_SIM_25PERCENT)
		{
			PreviewVC->TimeScale = 0.25f;
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_1, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_10, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_25, TRUE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_50, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_100, FALSE);
		}
		else if (Id == IDM_CASCADE_SIM_50PERCENT)
		{
			PreviewVC->TimeScale = 0.5f;
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_1, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_10, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_25, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_50, TRUE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_100, FALSE);
		}
		else if (Id == IDM_CASCADE_SIM_NORMAL)
		{
			PreviewVC->TimeScale = 1.f;
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_1, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_10, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_25, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_50, FALSE);
			ToolBar->ToggleTool(IDM_CASCADE_SPEED_100, TRUE);
		}
	}
}

void WxCascade::OnSaveCam(wxCommandEvent& In)
{
	PartSys->ThumbnailAngle = PreviewVC->PreviewAngle;
	PartSys->ThumbnailDistance = PreviewVC->PreviewDistance;
	PartSys->PreviewComponent = NULL;

	PreviewVC->bCaptureScreenShot = TRUE;
}

void WxCascade::OnResetSystem(wxCommandEvent& In)
{
	if (PartSysComp)
	{
		PartSysComp->ResetParticles();
		PartSysComp->ActivateSystem();
		PartSysComp->Template->bShouldResetPeakCounts = TRUE;
		if (PreviewVC)
		{
			PreviewVC->PreviewScene.RemoveComponent(PartSysComp);
			PreviewVC->PreviewScene.AddComponent(PartSysComp, FMatrix::Identity);
		}
	}
}

void WxCascade::OnResetInLevel(wxCommandEvent& In)
{
	OnResetSystem(In);
	for (TObjectIterator<UParticleSystemComponent> It;It;++It)
	{
		if (It->Template == PartSysComp->Template)
		{
			UParticleSystemComponent* PSysComp = *It;

			PSysComp->ResetParticles();
			PSysComp->ActivateSystem();
			PSysComp->Template->bShouldResetPeakCounts = TRUE;

			FSceneInterface* Scene = PSysComp->GetScene();
			if (Scene)
			{
				Scene->RemovePrimitive(PSysComp);
				Scene->AddPrimitive(PSysComp);
			}
		}
	}
}

void WxCascade::OnResetPeakCounts(wxCommandEvent& In)
{
	PartSysComp->ResetParticles();
/***
	for (INT i = 0; i < PartSysComp->Template->Emitters.Num(); i++)
	{
		UParticleEmitter* Emitter = PartSysComp->Template->Emitters(i);
		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
			LODLevel->PeakActiveParticles = 0;
		}
	}
***/
	PartSysComp->Template->bShouldResetPeakCounts = TRUE;
	PartSysComp->InitParticles();
}

void WxCascade::OnUberConvert(wxCommandEvent& In)
{
	if (!SelectedEmitter)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_MustSelectEmitter"));
		return;
	}

	if (appMsgf(AMT_YesNo, *LocalizeUnrealEd("UberModuleConvertConfirm")))
	{
		BeginTransaction(TEXT("EmitterUberConvert"));

		// Find the best uber module
		UParticleModuleUberBase* UberModule	= Cast<UParticleModuleUberBase>(
			UParticleModuleUberBase::DetermineBestUberModule(SelectedEmitter));
		if (!UberModule)
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedToFindUberModule"));
			EndTransaction(TEXT("EmitterUberConvert"));
			return;
		}

		// Convert it
		if (UberModule->ConvertToUberModule(SelectedEmitter) == FALSE)
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedToConverToUberModule"));
			EndTransaction(TEXT("EmitterUberConvert"));
			return;
		}

		EndTransaction(TEXT("EmitterUberConvert"));

		// Mark package as dirty
		SelectedEmitter->MarkPackageDirty();

		// Redraw the module window
		EmitterEdVC->Viewport->Invalidate();
	}
}

/**
 *	OnRegenerateLowestLOD
 *	This function is supplied to remove all LOD levels and regenerate the lowest.
 *	It is intended for use once the artists/designers decide on a suitable baseline
 *	for the lowest LOD generation parameters.
 */
void WxCascade::OnRegenerateLowestLOD(wxCommandEvent& In)
{
	if ((PartSys == NULL) || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	PartSys->bRegenerateLODDuplicate = FALSE;

	FString	WarningMessage(TEXT(""));

	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine1");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine2");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine3");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine4");

	if (appMsgf(AMT_YesNo, *WarningMessage) == TRUE)
	{
		debugf(TEXT("Regenerate Lowest LOD levels!"));

		BeginTransaction(TEXT("CascadeRegenerateLowestLOD"));
		ModifyParticleSystem();

		// Delete all LOD levels from each emitter...
		for (INT EmitterIndex = 0; EmitterIndex < PartSys->Emitters.Num(); EmitterIndex++)
		{
			UParticleEmitter*	Emitter	= PartSys->Emitters(EmitterIndex);
			if (Emitter)
			{
				for (INT LODIndex = Emitter->LODLevels.Num() - 1; LODIndex > 0; LODIndex--)
				{
					Emitter->LODLevels.Remove(LODIndex);
				}
				if (Emitter->AutogenerateLowestLODLevel(PartSys->bRegenerateLODDuplicate) == FALSE)
				{
					warnf(TEXT("Failed to autogenerate lowest LOD level!"));
				}

				Emitter->UpdateModuleLists();
			}
		}

		// Reset the LOD distances
		PartSys->LODDistances.Empty();
		UParticleEmitter* SourceEmitter = PartSys->Emitters(0);
		if (SourceEmitter)
		{
			PartSys->LODDistances.Add(SourceEmitter->LODLevels.Num());
			for (INT LODIndex = 0; LODIndex < PartSys->LODDistances.Num(); LODIndex++)
			{
				PartSys->LODDistances(LODIndex) = LODIndex * 2500.0f;
			}
		}

		wxCommandEvent DummyEvent;
		OnResetInLevel(DummyEvent);

		check(TransactionInProgress());
		EndTransaction(TEXT("CascadeRegenerateLowestLOD"));

		// Re-fill the LODCombo so that deleted LODLevels are removed.
		FillLODCombo();
		EmitterEdVC->Viewport->Invalidate();
		PropertyWindow->Rebuild();
		if (PartSysComp)
		{
			PartSysComp->ResetParticles();
			PartSysComp->InitializeSystem();
		}
	}
	else
	{
		debugf(TEXT("CANCELLED Regenerate Lowest LOD levels!"));
	}
}

/**
 *	OnRegenerateLowestLODDuplicateHighest
 *	This function is supplied to remove all LOD levels and regenerate the lowest.
 *	It is intended for use once the artists/designers decide on a suitable baseline
 *	for the lowest LOD generation parameters.
 *	It will duplicate the highest LOD as the lowest.
 */
void WxCascade::OnRegenerateLowestLODDuplicateHighest(wxCommandEvent& In)
{
	if ((PartSys == NULL) || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	PartSys->bRegenerateLODDuplicate = TRUE;

	FString	WarningMessage(TEXT(""));

	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine1");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine2");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine3");
	WarningMessage += TEXT("\n");
	WarningMessage += *LocalizeUnrealEd("CascadeRegenLowLODWarningLine4");

	if (appMsgf(AMT_YesNo, *WarningMessage) == TRUE)
	{
		debugf(TEXT("Regenerate Lowest LOD levels!"));

		BeginTransaction(TEXT("CascadeRegenerateLowestLOD"));
		ModifyParticleSystem();

		// Delete all LOD levels from each emitter...
		for (INT EmitterIndex = 0; EmitterIndex < PartSys->Emitters.Num(); EmitterIndex++)
		{
			UParticleEmitter*	Emitter	= PartSys->Emitters(EmitterIndex);
			if (Emitter)
			{
				for (INT LODIndex = Emitter->LODLevels.Num() - 1; LODIndex > 0; LODIndex--)
				{
					Emitter->LODLevels.Remove(LODIndex);
				}
				if (Emitter->AutogenerateLowestLODLevel(PartSys->bRegenerateLODDuplicate) == FALSE)
				{
					warnf(TEXT("Failed to autogenerate lowest LOD level!"));
				}

				Emitter->UpdateModuleLists();
			}
		}

		// Reset the LOD distances
		PartSys->LODDistances.Empty();
		UParticleEmitter* SourceEmitter = PartSys->Emitters(0);
		if (SourceEmitter)
		{
			PartSys->LODDistances.Add(SourceEmitter->LODLevels.Num());
			for (INT LODIndex = 0; LODIndex < PartSys->LODDistances.Num(); LODIndex++)
			{
				PartSys->LODDistances(LODIndex) = LODIndex * 2500.0f;
			}
		}

		wxCommandEvent DummyEvent;
		OnResetInLevel(DummyEvent);

		check(TransactionInProgress());
		EndTransaction(TEXT("CascadeRegenerateLowestLOD"));

		// Re-fill the LODCombo so that deleted LODLevels are removed.
		FillLODCombo();
		EmitterEdVC->Viewport->Invalidate();
		PropertyWindow->Rebuild();
		if (PartSysComp)
		{
			PartSysComp->ResetParticles();
			PartSysComp->InitializeSystem();
		}
	}
	else
	{
		debugf(TEXT("CANCELLED Regenerate Lowest LOD levels!"));
	}
}

void WxCascade::OnSavePackage(wxCommandEvent& In)
{
	debugf(TEXT("SAVE PACKAGE"));
	if (!PartSys)
	{
		appMsgf(AMT_OK, TEXT("No particle system active..."));
		return;
	}

	UPackage* Package = Cast<UPackage>(PartSys->GetOutermost());
	if (Package)
	{
		debugf(TEXT("Have a package!"));

		FString FileTypes( TEXT("All Files|*.*") );
		
		for (INT i=0; i<GSys->Extensions.Num(); i++)
		{
			FileTypes += FString::Printf( TEXT("|(*.%s)|*.%s"), *GSys->Extensions(i), *GSys->Extensions(i) );
		}

		if (FindObject<UWorld>(Package, TEXT("TheWorld")))
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_CantSaveMapViaCascade"), *Package->GetName());
		}
		else
		{
			FString ExistingFile, File, Directory;
			FString PackageName = Package->GetName();

			if (GPackageFileCache->FindPackageFile( *PackageName, NULL, ExistingFile ))
			{
				FString Filename, Extension;
				GPackageFileCache->SplitPath( *ExistingFile, Directory, Filename, Extension );
				File = FString::Printf( TEXT("%s.%s"), *Filename, *Extension );
			}
			else
			{
				Directory = TEXT("");
				File = FString::Printf( TEXT("%s.upk"), *PackageName );
			}

			WxFileDialog SaveFileDialog( this, 
				*LocalizeUnrealEd("SavePackage"), 
				*Directory,
				*File,
				*FileTypes,
				wxSAVE,
				wxDefaultPosition);

			FString SaveFileName;

			if ( SaveFileDialog.ShowModal() == wxID_OK )
			{
				SaveFileName = FString( SaveFileDialog.GetPath() );

				if ( GFileManager->IsReadOnly( *SaveFileName ) || !GUnrealEd->Exec( *FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""), *PackageName, *SaveFileName) ) )
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("Error_CouldntSavePackage") );
				}
			}
		}

		if (PartSys)
		{
			PartSys->PostEditChange(NULL);
		}
	}
}

void WxCascade::OnOrbitMode(wxCommandEvent& In)
{
	bOrbitMode = In.IsChecked();

	//@todo. actually handle this...
	if (bOrbitMode)
	{
		PreviewVC->SetPreviewCamera(PreviewVC->PreviewAngle, PreviewVC->PreviewDistance);
	}
}

void WxCascade::OnWireframe(wxCommandEvent& In)
{
	bWireframe = In.IsChecked();
	PreviewVC->bWireframe = bWireframe;
}

void WxCascade::OnBounds(wxCommandEvent& In)
{
	bBounds = In.IsChecked();
	PreviewVC->bBounds = bBounds;
}

/**
 *	Handler for turning post processing on and off.
 *
 *	@param	In	wxCommandEvent
 */
void WxCascade::OnPostProcess(wxCommandEvent& In)
{
	WxCascadePostProcessMenu* menu = new WxCascadePostProcessMenu(this);
	if (menu)
	{
		FTrackPopupMenu tpm(this, menu);
		tpm.Show();
		delete menu;
	}
}

/**
 *	Handler for turning the grid on and off.
 *
 *	@param	In	wxCommandEvent
 */
void WxCascade::OnToggleGrid(wxCommandEvent& In)
{
	bool bShowGrid = In.IsChecked();

	if (PreviewVC)
	{
		// Toggle the grid and worldbox.
		EditorOptions->bShowGrid = bShowGrid;
		EditorOptions->SaveConfig();
		PreviewVC->DrawHelper.bDrawGrid = bShowGrid;
		if (bShowGrid)
		{
			PreviewVC->ShowFlags |= SHOW_Grid;
		}
		else
		{
			PreviewVC->ShowFlags &= ~SHOW_Grid;
		}
	}
}

void WxCascade::OnViewAxes(wxCommandEvent& In)
{
	PreviewVC->bDrawOriginAxes = In.IsChecked();
}

void WxCascade::OnViewCounts(wxCommandEvent& In)
{
	PreviewVC->bDrawParticleCounts = In.IsChecked();
	EditorOptions->bShowParticleCounts = PreviewVC->bDrawParticleCounts;
	EditorOptions->SaveConfig();
}

void WxCascade::OnViewTimes(wxCommandEvent& In)
{
	PreviewVC->bDrawParticleTimes = In.IsChecked();
	EditorOptions->bShowParticleTimes = PreviewVC->bDrawParticleTimes;
	EditorOptions->SaveConfig();
}

void WxCascade::OnViewDistance(wxCommandEvent& In)
{
	PreviewVC->bDrawSystemDistance = In.IsChecked();
	EditorOptions->bShowParticleDistance = PreviewVC->bDrawSystemDistance;
	EditorOptions->SaveConfig();
}

void WxCascade::OnViewGeometry(wxCommandEvent& In)
{
	if (PreviewVC->FloorComponent)
	{
		PreviewVC->FloorComponent->HiddenEditor = !In.IsChecked();
		PreviewVC->FloorComponent->HiddenGame = PreviewVC->FloorComponent->HiddenEditor;

		EditorOptions->bShowFloor = !PreviewVC->FloorComponent->HiddenEditor;
		EditorOptions->SaveConfig();

		PreviewVC->PreviewScene.RemoveComponent(PreviewVC->FloorComponent);
		PreviewVC->PreviewScene.AddComponent(PreviewVC->FloorComponent, FMatrix::Identity);
	}
}

void WxCascade::OnViewGeometryProperties(wxCommandEvent& In)
{
	if (PreviewVC->FloorComponent)
	{
		WxPropertyWindowFrame* Properties = new WxPropertyWindowFrame;
		Properties->Create(this, -1, 0, &PropWindowNotifyHook);
		Properties->AllowClose();
		Properties->SetObject(PreviewVC->FloorComponent,0,1,1);
		Properties->SetTitle(*FString::Printf(TEXT("Properties: %s"), *PreviewVC->FloorComponent->GetPathName()));
		Properties->Show();
		PropWindowNotifyHook.Cascade = this;
		PropWindowNotifyHook.WindowOfInterest = (void*)(Properties->GetPropertyWindow());
	}
}

void WxCascade::OnLoopSimulation(wxCommandEvent& In)
{
	bResetOnFinish = In.IsChecked();

	if (!bResetOnFinish)
		bPendingReset = FALSE;
}

void WxCascade::OnShowPPBloom( wxCommandEvent& In )
{
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_BLOOM;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_BLOOM;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
}

void WxCascade::OnShowPPDOF( wxCommandEvent& In )
{
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_DOF;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_DOF;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
}

void WxCascade::OnShowPPMotionBlur( wxCommandEvent& In )
{
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_MOTIONBLUR;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_MOTIONBLUR;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
}

void WxCascade::OnShowPPVolumeMaterial( wxCommandEvent& In )
{
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_PPVOLUME;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_PPVOLUME;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
}

#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
void WxCascade::OnViewModuleDump(wxCommandEvent& In)
{
	EmitterEdVC->bDrawModuleDump = !EmitterEdVC->bDrawModuleDump;
	EditorOptions->bShowModuleDump = EmitterEdVC->bDrawModuleDump;
	EditorOptions->SaveConfig();

	EmitterEdVC->Viewport->Invalidate();
}
#endif	//#if defined(_CASCADE_ENABLE_MODULE_DUMP_)

void WxCascade::OnPlay(wxCommandEvent& In)
{
	PreviewVC->TimeScale = SimSpeed;
}

void WxCascade::OnPause(wxCommandEvent& In)
{
	PreviewVC->TimeScale = 0.f;
}

void WxCascade::OnSpeed(wxCommandEvent& In)
{
	INT Id = In.GetId();

	FLOAT NewSimSpeed = 0.0f;
	INT SimID;

	switch (Id)
	{
	case IDM_CASCADE_SPEED_1:
		NewSimSpeed = 0.01f;
		SimID = IDM_CASCADE_SIM_1PERCENT;
		break;
	case IDM_CASCADE_SPEED_10:
		NewSimSpeed = 0.1f;
		SimID = IDM_CASCADE_SIM_10PERCENT;
		break;
	case IDM_CASCADE_SPEED_25:
		NewSimSpeed = 0.25f;
		SimID = IDM_CASCADE_SIM_25PERCENT;
		break;
	case IDM_CASCADE_SPEED_50:
		NewSimSpeed = 0.5f;
		SimID = IDM_CASCADE_SIM_50PERCENT;
		break;
	case IDM_CASCADE_SPEED_100:
		NewSimSpeed = 1.0f;
		SimID = IDM_CASCADE_SIM_NORMAL;
		break;
	}

	if (NewSimSpeed != 0.0f)
	{
		SimSpeed = NewSimSpeed;
		if (PreviewVC->TimeScale != 0.0f)
		{
			PreviewVC->TimeScale = SimSpeed;
		}
	}
}

void WxCascade::OnLoopSystem(wxCommandEvent& In)
{
	OnLoopSimulation(In);
}

void WxCascade::OnRealtime(wxCommandEvent& In)
{
	PreviewVC->SetRealtime(In.IsChecked());
}

void WxCascade::OnBackgroundColor(wxCommandEvent& In)
{
	wxColour wxColorIn(PreviewVC->BackgroundColor.R, PreviewVC->BackgroundColor.G, PreviewVC->BackgroundColor.B);

	wxColour wxColorOut = wxGetColourFromUser(this, wxColorIn);
	if (wxColorOut.Ok())
	{
		PreviewVC->BackgroundColor.R = wxColorOut.Red();
		PreviewVC->BackgroundColor.G = wxColorOut.Green();
		PreviewVC->BackgroundColor.B = wxColorOut.Blue();
	}

	EditorOptions->BackgroundColor = PreviewVC->BackgroundColor;
	EditorOptions->SaveConfig();
}

void WxCascade::OnToggleWireSphere(wxCommandEvent& In)
{
	PreviewVC->bDrawWireSphere = !PreviewVC->bDrawWireSphere;
	if (PreviewVC->bDrawWireSphere)
	{
		// display a dialog box asking fort the radius of the sphere
		WxDlgGenericStringEntry Dialog;
		INT Result = Dialog.ShowModal(TEXT("CascadeToggleWireSphere"), TEXT("SphereRadius"), *FString::Printf(TEXT("%f"), PreviewVC->WireSphereRadius));
		if (Result != wxID_OK)
		{
			// dialog was canceled
			PreviewVC->bDrawWireSphere = FALSE;
			ToolBar->ToggleTool(IDM_CASCADE_TOGGLE_WIRE_SPHERE, FALSE);
		}
		else
		{
			FLOAT NewRadius = appAtof(*Dialog.GetEnteredString());
			// if an invalid number was entered, cancel
			if (NewRadius < KINDA_SMALL_NUMBER)
			{
				PreviewVC->bDrawWireSphere = FALSE;
				ToolBar->ToggleTool(IDM_CASCADE_TOGGLE_WIRE_SPHERE, FALSE);
			}
			else
			{
				PreviewVC->WireSphereRadius = NewRadius;
			}
		}
	}
}

void WxCascade::OnUndo(wxCommandEvent& In)
{
	CascadeUndo();
}

void WxCascade::OnRedo(wxCommandEvent& In)
{
	CascadeRedo();
}

void WxCascade::OnPerformanceCheck(wxCommandEvent& In)
{
	debugf(TEXT("PERFORMANCE CHECK!"));
}

void WxCascade::OnLODSlider(wxScrollEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	//
	INT	Value	= ToolBar->LODSlider->GetValue();

	FString	ValueStr = FString::Printf(TEXT("%d"), Value);
	ToolBar->LODSetting->SetValue(*ValueStr);

	UpdateLODCombo();

	this->PartSys->EditorLODSetting	= Value;
	SetSelectedModule(SelectedEmitter, SelectedModule);
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnLODSetting(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	// Get the setting
	INT	Value	= appAtoi(In.GetString());
	// Verify
	if (Value < 0)
	{
		ToolBar->LODSetting->SetValue(TEXT("0"));
		Value	= 0;
	}
	else
	if (Value > 100)
	{
		ToolBar->LODSetting->SetValue(TEXT("100"));
		Value	= 100;
	}

	ToolBar->LODSlider->SetValue(Value);

	UpdateLODCombo();

	this->PartSys->EditorLODSetting	= Value;
	SetSelectedModule(SelectedEmitter, SelectedModule);
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnLODCombo(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	INT	Value	= -1;
	INT	Index	= ToolBar->LODCombo->GetSelection();
	if (Index > 0)
	{
		FString	ComboString	= FString(ToolBar->LODCombo->GetString(Index));
		if (appStrcmp(*ComboString, TEXT("LOD Unset")) == 0)
		{
			// Leave the value where it is...
		}
		else
		if (appStrcmp(*ComboString, *LocalizeUnrealEd("LODLowest")) == 0)
		{
			Value	= 100;
		}
		else
		if (appStrcmp(*ComboString, *LocalizeUnrealEd("LODHighest")) == 0)
		{
			Value	= 0;
		}
		else
		{
			// Parse off the LOD level index. Grab the information from the particle system component.
			FString	LODString;
			FString	ValueString;

			ComboString.Split(*LocalizeUnrealEd("CascadeLODLabel"), &LODString, &ValueString);
			Value	= appAtoi(*ValueString);
		}
	}

	if (Value != -1)
	{
		FString	ValueStr = FString::Printf(TEXT("%d"), Value);
		ToolBar->LODSetting->SetValue(*ValueStr);
		ToolBar->LODSlider->SetValue(Value);

		PartSys->EditorLODSetting	= Value;
		SetSelectedModule(SelectedEmitter, SelectedModule);
	}

	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnLODLow(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	INT	Value = 100;

	SetLODValue(Value);
/***
	FString	ValueStr = FString::Printf(TEXT("%d"), Value);
	ToolBar->LODSetting->SetValue(*ValueStr);
	ToolBar->LODSlider->SetValue(Value);

	this->PartSys->EditorLODSetting	= Value;
***/
	SetSelectedModule(SelectedEmitter, SelectedModule);
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnLODLower(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	INT	LODValue	= ToolBar->LODSlider->GetValue();

	// Find the next lower LOD...
	// We can use any emitter, since they will all have the same number of LOD levels
	UParticleEmitter* Emitter	= PartSys->Emitters(0);
	if (Emitter)
	{
		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel	= Emitter->LODLevels(LODIndex);
			if (LODLevel)
			{
				if (LODLevel->LevelSetting > LODValue)
				{
					SetLODValue(LODLevel->LevelSetting);
					SetSelectedModule(SelectedEmitter, SelectedModule);
					EmitterEdVC->Viewport->Invalidate();
					break;
				}
			}
		}
	}
}

void WxCascade::OnLODAdd(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	// Get the LOD setting
	INT	LODValue	= ToolBar->LODSlider->GetValue();

	INT InsertIndex	= -1;

	// See if there is already a LOD level for this value...
	if (PartSys->Emitters.Num() > 0)
	{
		UParticleEmitter* CheckEmitter	= PartSys->Emitters(0);
		if (CheckEmitter)
		{
			for (INT LODIndex = 0; LODIndex < CheckEmitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* CheckLODLevel = CheckEmitter->LODLevels(LODIndex);
				if (CheckLODLevel)
				{
					if (CheckLODLevel->LevelSetting == LODValue)
					{
						// Already exists...
						return;
					}
					else
					if (CheckLODLevel->LevelSetting < LODValue)
					{
						InsertIndex	= LODIndex + 1;
					}
				}
			}
		}
	}

	// Add it
	if (InsertIndex != -1)
	{
		debugf(TEXT("Inserting LOD level %d at %d"), LODValue, InsertIndex);

		BeginTransaction(TEXT("CascadeInsertLOD"));
		ModifyParticleSystem();

		// Insert the LOD into the distance array
		PartSys->LODDistances.Insert(InsertIndex, 1);
		FLOAT	LowerDistance	= PartSys->LODDistances(InsertIndex - 1);
		FLOAT	HigherDistance	= PartSys->LODDistances(InsertIndex + 1);
		
		PartSys->LODDistances(InsertIndex)	= LowerDistance + (HigherDistance - LowerDistance) / 2;

		// Update all the emitters
		for (INT EmitterIndex = 0; EmitterIndex < PartSys->Emitters.Num(); EmitterIndex++)
		{
			UParticleEmitter* Emitter	= PartSys->Emitters(EmitterIndex);
			if (Emitter)
			{
				// 
				INT ResultIndex = Emitter->CreateLODLevel(LODValue, FALSE);
				if (ResultIndex != InsertIndex)
				{
					warnf(TEXT("LODAdd: WTF Happened???"));
				}
				else
				{
					UParticleLODLevel* NewLODLevel	= Emitter->LODLevels(ResultIndex);
					check(NewLODLevel);
					NewLODLevel->UpdateModuleLists();

					// <<<TESTING>>>
					if ((ResultIndex > 0) && (ResultIndex < (Emitter->LODLevels.Num() - 1)))
					{
						UParticleLODLevel*	HighLevel	= Emitter->LODLevels(ResultIndex - 1);
						UParticleLODLevel*	LowLevel	= Emitter->LODLevels(ResultIndex + 1);

						UParticleModule* TargetModule;
						UParticleModule* HighModule;
						UParticleModule* LowModule;

						TargetModule	= NewLODLevel->RequiredModule;
						HighModule		= HighLevel->RequiredModule;
						LowModule		= LowLevel->RequiredModule;

						FLOAT	Percentage	= (
							(FLOAT)(LowLevel->LevelSetting - NewLODLevel->LevelSetting) / 
							(FLOAT)(LowLevel->LevelSetting - HighLevel->LevelSetting));
						GenerateLODModuleValues(TargetModule, HighModule, LowModule, Percentage);

						for (INT ModuleIndex = 0; ModuleIndex < NewLODLevel->Modules.Num(); ModuleIndex++)
						{
							TargetModule	= NewLODLevel->Modules(ModuleIndex);
							HighModule		= HighLevel->Modules(ModuleIndex);
							LowModule		= LowLevel->Modules(ModuleIndex);
							GenerateLODModuleValues(TargetModule, HighModule, LowModule, Percentage);
						}
					}
				}
			}
		}

		wxCommandEvent DummyEvent;
		OnResetInLevel(DummyEvent);

		check(TransactionInProgress());
		EndTransaction(TEXT("CascadeInsertLOD"));

		// Add it to the LOD combo...
		FillLODCombo();
		SetSelectedModule(SelectedEmitter, SelectedModule);
		EmitterEdVC->Viewport->Invalidate();
		if (PartSysComp)
		{
			PartSysComp->ResetParticles();
			PartSysComp->InitializeSystem();
		}
	}

	PropertyWindow->Refresh();
}

void WxCascade::OnLODHigher(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	INT	LODValue	= ToolBar->LODSlider->GetValue();

	// Find the next higher LOD...
	// We can use any emitter, since they will all have the same number of LOD levels
	UParticleEmitter* Emitter	= PartSys->Emitters(0);
	if (Emitter)
	{
		// Go from the low to the high...
		for (INT LODIndex = Emitter->LODLevels.Num() - 1; LODIndex >= 0; LODIndex--)
		{
			UParticleLODLevel* LODLevel	= Emitter->LODLevels(LODIndex);
			if (LODLevel)
			{
				if (LODLevel->LevelSetting < LODValue)
				{
					SetLODValue(LODLevel->LevelSetting);
					SetSelectedModule(SelectedEmitter, SelectedModule);
					EmitterEdVC->Viewport->Invalidate();
					break;
				}
			}
		}
	}
}

void WxCascade::OnLODHigh(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys || (PartSys->Emitters.Num() == 0))
	{
		return;
	}

	INT	Value = 0;

	SetLODValue(Value);
	SetSelectedModule(SelectedEmitter, SelectedModule);
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::OnLODDelete(wxCommandEvent& In)
{
	if (!ToolBar || !PartSys)
	{
		return;
	}

	INT	Selection	= ToolBar->LODCombo->GetSelection();
	FString	ComboSetting	= FString(ToolBar->LODCombo->GetString(Selection));

	if (appStrcmp(*ComboSetting, TEXT("LOD Highest")) == 0)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("CascadeCantDeleteLOD"), TEXT("LOD 0"));
		return;
	}
	else
	if (appStrcmp(*ComboSetting, TEXT("LOD Lowest")) == 0)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("CascadeCantDeleteLOD"), TEXT("LOD Lowest"));
		return;
	}
	else
	if (appStrcmp(*ComboSetting, TEXT("LOD Unset")) == 0)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("CascadeCantDeleteLOD"), TEXT("LOD Unset"));
		return;
	}

	// Delete the setting...
	INT	LODSetting	= ToolBar->LODSlider->GetValue();

	BeginTransaction(TEXT("CascadeDeleteLOD"));
	ModifyParticleSystem();

	// Remove the LOD entry from the distance array
	UParticleEmitter* Emitter = PartSys->Emitters(0);
	if (Emitter)
	{
		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel	= Emitter->LODLevels(LODIndex);
			if (LODLevel)
			{
				if (LODLevel->LevelSetting == LODSetting)
				{
					PartSys->LODDistances.Remove(LODLevel->Level);
					break;
				}
			}
		}
	}

	// Remove the level from each emitter in the system
	for (INT EmitterIndex = 0; EmitterIndex < PartSys->Emitters.Num(); EmitterIndex++)
	{
		Emitter = PartSys->Emitters(EmitterIndex);
		if (Emitter)
		{
			for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* LODLevel	= Emitter->LODLevels(LODIndex);
				if (LODLevel)
				{
					if (LODLevel->LevelSetting == LODSetting)
					{
						// Delete it and shift all down
						Emitter->LODLevels.Remove(LODIndex);

						for (; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
						{
							UParticleLODLevel* RemapLODLevel	= Emitter->LODLevels(LODIndex);
							if (RemapLODLevel)
							{
								RemapLODLevel->Level--;
							}
						}
						break;
					}
				}
			}
		}
	}

	wxCommandEvent DummyEvent;
	OnResetInLevel(DummyEvent);

	check(TransactionInProgress());
	EndTransaction(TEXT("CascadeDeleteLOD"));

	FillLODCombo();
	EmitterEdVC->Viewport->Invalidate();
	PropertyWindow->Rebuild();
}

///////////////////////////////////////////////////////////////////////////////////////
// Properties window NotifyHook stuff

void WxCascade::NotifyDestroy( void* Src )
{

}

void WxCascade::NotifyPreChange( FEditPropertyChain* PropertyChain )
{
	BeginTransaction(TEXT("CascadePropertyChange"));
	ModifyParticleSystem();

	CurveToReplace = NULL;

	// get the property that is being edited
	UObjectProperty* ObjProp = Cast<UObjectProperty>(PropertyChain->GetActiveNode()->GetValue());
	if (ObjProp && 
		(ObjProp->PropertyClass->IsChildOf(UDistributionFloat::StaticClass()) || 
		 ObjProp->PropertyClass->IsChildOf(UDistributionVector::StaticClass()))
		 )
	{
		UObject* EditedObject = NULL;
		if (SelectedModule)
		{
			EditedObject = SelectedModule;
		}
		else
//		if (SelectedEmitter)
		{
			EditedObject = SelectedEmitter;
		}

		// calculate offset from object to property being edited
		DWORD Offset = 0;
		UObject* BaseObject = EditedObject;
		for (FEditPropertyChain::TIterator It(PropertyChain->GetHead()); It; ++It )
		{
			Offset += It->Offset;

			// don't go past the active property
			if (*It == ObjProp)
			{
				break;
			}

			// If it is an object property, then reset our base pointer/offset
			if (It->IsA(UObjectProperty::StaticClass()))
			{
				BaseObject = *(UObject**)((BYTE*)BaseObject + Offset);
				Offset = 0;
			}
		}

		BYTE* CurvePointer = (BYTE*)BaseObject + Offset;
		UObject** ObjPtrPtr = (UObject**)CurvePointer;
		UObject* ObjPtr = *(ObjPtrPtr);
		CurveToReplace = ObjPtr;

		check(CurveToReplace); // These properties are 'noclear', so should always have a curve here!
	}
}

void WxCascade::NotifyPostChange( FEditPropertyChain* PropertyChain )
{
	if (CurveToReplace)
	{
		// This should be the same property we just got in NotifyPreChange!
		UObjectProperty* ObjProp = Cast<UObjectProperty>(PropertyChain->GetActiveNode()->GetValue());
		check(ObjProp);
		check(ObjProp->PropertyClass->IsChildOf(UDistributionFloat::StaticClass()) || ObjProp->PropertyClass->IsChildOf(UDistributionVector::StaticClass()));

		UObject* EditedObject = NULL;
		if (SelectedModule)
		{
			EditedObject = SelectedModule;
		}
		else
//		if (SelectedEmitter)
		{
			EditedObject = SelectedEmitter;
		}

		// calculate offset from object to property being edited
		DWORD Offset = 0;
		UObject* BaseObject = EditedObject;
		for (FEditPropertyChain::TIterator It(PropertyChain->GetHead()); It; ++It )
		{
			Offset += It->Offset;

			// don't go past the active property
			if (*It == ObjProp)
			{
				break;
			}

			// If it is an object property, then reset our base pointer/offset
			if (It->IsA(UObjectProperty::StaticClass()))
			{
				BaseObject = *(UObject**)((BYTE*)BaseObject + Offset);
				Offset = 0;
			}
		}

		BYTE* CurvePointer = (BYTE*)BaseObject + Offset;
		UObject** ObjPtrPtr = (UObject**)CurvePointer;
		UObject* NewCurve = *(ObjPtrPtr);
		
		if (NewCurve)
		{
			PartSys->CurveEdSetup->ReplaceCurve(CurveToReplace, NewCurve);
			CurveEd->CurveChanged();
		}
	}

	if (SelectedModule || SelectedEmitter)
	{
		PartSys->PostEditChange(PropertyChain->GetActiveNode()->GetValue());

		if (SelectedModule)
		{
			if (SelectedModule->IsDisplayedInCurveEd(CurveEd->EdSetup))
			{
				TArray<FParticleCurvePair> Curves;
				SelectedModule->GetCurveObjects(Curves);

				for (INT i=0; i<Curves.Num(); i++)
				{
					CurveEd->EdSetup->ChangeCurveColor(Curves(i).CurveObject, SelectedModule->ModuleEditorColor);
				}
			}
		}

		if (SelectedEmitter)
		{
			for (INT LODIndex = 0; LODIndex < SelectedEmitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* LODLevel = SelectedEmitter->GetLODLevel(LODIndex);
				CurveEd->EdSetup->ChangeCurveColor(LODLevel->RequiredModule->SpawnRate.Distribution, LODLevel->RequiredModule->EmitterEditorColor);
			}
		}
	}

	PartSys->ThumbnailImageOutOfDate = TRUE;

	CurveEd->CurveChanged();
	EmitterEdVC->Viewport->Invalidate();

	check(TransactionInProgress());
	EndTransaction(TEXT("CascadePropertyChange"));
}

void WxCascade::NotifyExec( void* Src, const TCHAR* Cmd )
{
	GUnrealEd->NotifyExec(Src, Cmd);
}

///////////////////////////////////////////////////////////////////////////////////////
// Utils
void WxCascade::CreateNewModule(INT ModClassIndex)
{
	if (SelectedEmitter == NULL)
	{
		return;
	}

	if (ToolBar->LODSlider->GetValue() != 0)
	{
		// Don't allow creating modules if not at highest LOD
		warnf(TEXT("Cascade: Attempting to create module while not on highest LOD (0)"));
		return;
	}

	UClass* NewModClass = ParticleModuleClasses(ModClassIndex);
	check(NewModClass->IsChildOf(UParticleModule::StaticClass()));

	UBOOL bTypeData = FALSE;

	if (NewModClass->IsChildOf(UParticleModuleTypeDataBase::StaticClass()))
	{
		// Make sure there isn't already a TypeData module applied!
		UParticleLODLevel* LODLevel = SelectedEmitter->GetLODLevel(0);
		if (LODLevel->TypeDataModule != 0)
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_TypeDataModuleAlreadyPresent"));
			return;
		}
		bTypeData = TRUE;
	}

	BeginTransaction(TEXT("CreateNewModule"));
	ModifyParticleSystem();
	ModifySelectedObjects();

	PartSys->PreEditChange(NULL);
	PartSysComp->PreEditChange(NULL);

	// Construct it and add to selected emitter.
	for (INT LODIndex = 0; LODIndex < SelectedEmitter->LODLevels.Num(); LODIndex++)
	{
		UParticleModule* NewModule = ConstructObject<UParticleModule>(NewModClass, PartSys, NAME_None, RF_Transactional);
		NewModule->ModuleEditorColor = FColor::MakeRandomColor();

		UParticleLODLevel* LODLevel	= SelectedEmitter->GetLODLevel(LODIndex);
		if (LODLevel->LevelSetting != 0)
		{
			NewModule->bEditable	= FALSE;
		}

		//@todo.SAS. Add a SetToSensibleDefaults function for ALL modules that takes the emitter pointer.
		// This will allow for setting (for example) the Size to appropriate values when it is a sprite vs. mesh emitter.
		NewModule->SetToSensibleDefaults(SelectedEmitter);

		LODLevel->Modules.AddItem(NewModule);
	}

	SelectedEmitter->UpdateModuleLists();

	PartSysComp->PostEditChange(NULL);
	PartSys->PostEditChange(NULL);

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("CreateNewModule"));

	// Refresh viewport
	EmitterEdVC->Viewport->Invalidate();
}

void WxCascade::PasteCurrentModule()
{
	if (!SelectedEmitter)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_MustSelectEmitter"));
		return;
	}

	if (ToolBar->LODSlider->GetValue() != 0)
	{
		// Don't allow pasting modules if not at highest LOD
		warnf(TEXT("Cascade: Attempting to paste module while not on highest LOD (0)"));
		return;
	}

	check(CopyModule);

	UObject* pkDupObject = 
		GEditor->StaticDuplicateObject(CopyModule, CopyModule, PartSys, TEXT("None"));
	if (pkDupObject)
	{
		UParticleModule* Module	= Cast<UParticleModule>(pkDupObject);
		Module->ModuleEditorColor = FColor::MakeRandomColor();
		UParticleLODLevel* LODLevel	= SelectedEmitter->GetLODLevel(0);
		InsertModule(Module, SelectedEmitter, LODLevel->Modules.Num());
	}
}

void WxCascade::CopyModuleToEmitter(UParticleModule* pkSourceModule, UParticleEmitter* pkTargetEmitter, UParticleSystem* pkTargetSystem)
{
    check(pkSourceModule);
    check(pkTargetEmitter);
	check(pkTargetSystem);

	if (ToolBar->LODSlider->GetValue() != 0)
	{
		// Don't allow copying modules if not at highest LOD
		warnf(TEXT("Cascade: Attempting to copy module while not on highest LOD (0)"));
		return;
	}

	UObject* DupObject = GEditor->StaticDuplicateObject(pkSourceModule, pkSourceModule, pkTargetSystem, TEXT("None"));
	if (DupObject)
	{
		UParticleModule* Module	= Cast<UParticleModule>(DupObject);
		Module->ModuleEditorColor = FColor::MakeRandomColor();

		UParticleLODLevel* LODLevel;

		if (EmitterEdVC->DraggedModule == pkSourceModule)
		{
			EmitterEdVC->DraggedModules(0) = Module;
			// If we are dragging, we need to copy all the LOD modules
			for (INT LODIndex = 1; LODIndex < pkTargetEmitter->LODLevels.Num(); LODIndex++)
			{
				LODLevel	= pkTargetEmitter->GetLODLevel(LODIndex);

				UParticleModule* CopySource = EmitterEdVC->DraggedModules(LODIndex);
				if (CopySource)
				{
					DupObject = GEditor->StaticDuplicateObject(CopySource, CopySource, pkTargetSystem, TEXT("None"));
					if (DupObject)
					{
						UParticleModule* NewModule	= Cast<UParticleModule>(DupObject);
						NewModule->ModuleEditorColor = Module->ModuleEditorColor;
						EmitterEdVC->DraggedModules(LODIndex) = NewModule;
					}
				}
				else
				{
					warnf(TEXT("Missing dragged module!"));
				}
			}
		}

		LODLevel	= pkTargetEmitter->GetLODLevel(0);
		InsertModule(Module, pkTargetEmitter, LODLevel->Modules.Num(), FALSE);
	}
}

void WxCascade::SetSelectedEmitter( UParticleEmitter* NewSelectedEmitter )
{
	SetSelectedModule(NewSelectedEmitter, NULL);
}

void WxCascade::SetSelectedModule( UParticleEmitter* NewSelectedEmitter, UParticleModule* NewSelectedModule )
{
	SelectedEmitter = NewSelectedEmitter;

	INT	SliderValue	= ToolBar->LODSlider->GetValue();

	UParticleLODLevel* LODLevel = NULL;
	// Make sure it's the correct LOD level...
	if (SelectedEmitter)
	{
		LODLevel = SelectedEmitter->GetClosestLODLevel(SliderValue);
		SelectedEmitter->RequiredModule	= LODLevel->RequiredModule;

		if (NewSelectedModule)
		{
			INT	ModuleIndex	= -1;
			for (INT LODLevelCheck = 0; LODLevelCheck < SelectedEmitter->LODLevels.Num(); LODLevelCheck++)
			{
				UParticleLODLevel* CheckLODLevel	= SelectedEmitter->LODLevels(LODLevelCheck);
				if (LODLevel)
				{
					for (INT ModuleCheck = 0; ModuleCheck < CheckLODLevel->Modules.Num(); ModuleCheck++)
					{
						if (CheckLODLevel->Modules(ModuleCheck) == NewSelectedModule)
						{
							ModuleIndex = ModuleCheck;
							break;
						}
					}
				}

				if (ModuleIndex != -1)
				{
					break;
				}
			}

			if (ModuleIndex != -1)
			{
				NewSelectedModule	= LODLevel->Modules(ModuleIndex);
			}
			SelectedModuleIndex	= ModuleIndex;
		}
	}

	SelectedModule = NewSelectedModule;

	if (SelectedModule && SelectedEmitter)
	{
		if (LODLevel && LODLevel->LevelSetting != SliderValue)
		{
			PropertyWindow->SetReadOnly(TRUE);
		}
		else
		{
			PropertyWindow->SetReadOnly(!SelectedModule->bEditable);
		}
		PropertyWindow->SetObject(SelectedModule, 1, 0, 1);
	}
	else
	if (SelectedEmitter)
	{
		if (LODLevel && LODLevel->LevelSetting != SliderValue)
		{
			PropertyWindow->SetReadOnly(TRUE);
		}
		else
		{
			PropertyWindow->SetReadOnly(!SelectedEmitter->RequiredModule->bEditable);
		}
		PropertyWindow->SetObject(SelectedEmitter, 1, 0, 1);
	}
	else
	{
		PropertyWindow->SetReadOnly(FALSE);
		PropertyWindow->SetObject(PartSys, 1, 0, 1);
	}

	SetSelectedInCurveEditor();
	EmitterEdVC->Viewport->Invalidate();
}

// Insert the selected module into the target emitter at the desired location.
void WxCascade::InsertModule(UParticleModule* Module, UParticleEmitter* TargetEmitter, INT TargetIndex, UBOOL bSetSelected)
{
	if (!Module || !TargetEmitter || TargetIndex == INDEX_NONE)
		return;

	if (ToolBar->LODSlider->GetValue() != 0)
	{
		// Don't allow moving modules if not at highest LOD
		warnf(TEXT("Cascade: Attempting to move module while not on highest LOD (0)"));
		return;
	}

	// Cannot insert the same module more than once into the same emitter.
	UParticleLODLevel* LODLevel	= TargetEmitter->GetLODLevel(0);
	for(INT i = 0; i < LODLevel->Modules.Num(); i++)
	{
		if (LODLevel->Modules(i) == Module)
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_ModuleCanOnlyBeUsedInEmitterOnce"));
			return;
		}
	}

	BeginTransaction(TEXT("InsertModule"));
	ModifyEmitter(TargetEmitter);
	ModifyParticleSystem();

	// Insert in desired location in new Emitter
	PartSys->PreEditChange(NULL);

	if (Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
	{
		if (LODLevel->TypeDataModule)
		{
		}
		LODLevel->TypeDataModule = Module;

		if (EmitterEdVC->DraggedModules.Num() > 0)
		{
			// Swap the modules in all the LOD levels
			for (INT LODIndex = 1; LODIndex < TargetEmitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel*	LODLevel	= TargetEmitter->GetLODLevel(LODIndex);
				UParticleModule*	Module		= EmitterEdVC->DraggedModules(LODIndex);

				LODLevel->TypeDataModule	= Module;
			}

			EmitterEdVC->DraggedModules.Empty();
		}
	}
	else
	{
		INT NewModuleIndex = Clamp<INT>(TargetIndex, 0, LODLevel->Modules.Num());
		LODLevel->Modules.Insert(NewModuleIndex);
		LODLevel->Modules(NewModuleIndex) = Module;

		if (EmitterEdVC->DraggedModules.Num() > 0)
		{
			// Swap the modules in all the LOD levels
			for (INT LODIndex = 1; LODIndex < TargetEmitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel*	LODLevel	= TargetEmitter->GetLODLevel(LODIndex);
				UParticleModule*	Module		= EmitterEdVC->DraggedModules(LODIndex);

				LODLevel->Modules.Insert(NewModuleIndex);
				LODLevel->Modules(NewModuleIndex)	= Module;
			}

			EmitterEdVC->DraggedModules.Empty();
		}
	}

	TargetEmitter->UpdateModuleLists();

	PartSys->PostEditChange(NULL);

	// Update selection
    if (bSetSelected)
    {
        SetSelectedModule(TargetEmitter, Module);
    }

	EmitterEdVC->Viewport->Invalidate();

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("InsertModule"));
}

// Delete entire Emitter from System
// Garbage collection will clear up any unused modules.
void WxCascade::DeleteSelectedEmitter()
{
	if (!SelectedEmitter)
		return;

	check(PartSys->Emitters.ContainsItem(SelectedEmitter));

	BeginTransaction(TEXT("DeleteSelectedEmitter"));
	ModifyParticleSystem();

	PartSys->PreEditChange(NULL);

	SelectedEmitter->RemoveEmitterCurvesFromEditor(CurveEd->EdSetup);

	PartSys->Emitters.RemoveItem(SelectedEmitter);

	PartSys->PostEditChange(NULL);

	SetSelectedEmitter(NULL);

	EmitterEdVC->Viewport->Invalidate();

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("DeleteSelectedEmitter"));
}

// Move the selected amitter by MoveAmount in the array of Emitters.
void WxCascade::MoveSelectedEmitter(INT MoveAmount)
{
	if (!SelectedEmitter)
		return;

	BeginTransaction(TEXT("MoveSelectedEmitter"));
	ModifyParticleSystem();

	INT CurrentEmitterIndex = PartSys->Emitters.FindItemIndex(SelectedEmitter);
	check(CurrentEmitterIndex != INDEX_NONE);

	INT NewEmitterIndex = Clamp<INT>(CurrentEmitterIndex + MoveAmount, 0, PartSys->Emitters.Num() - 1);

	if (NewEmitterIndex != CurrentEmitterIndex)
	{
		PartSys->PreEditChange(NULL);

		PartSys->Emitters.RemoveItem(SelectedEmitter);
		PartSys->Emitters.InsertZeroed(NewEmitterIndex);
		PartSys->Emitters(NewEmitterIndex) = SelectedEmitter;

		PartSys->PostEditChange(NULL);

		EmitterEdVC->Viewport->Invalidate();
	}

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("MoveSelectedEmitter"));
}

// Export the selected emitter for importing into another particle system
void WxCascade::ExportSelectedEmitter()
{
	if (!SelectedEmitter)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_NoEmitterSelectedForExport"));
		return;
	}

	for ( USelection::TObjectIterator Itor = GEditor->GetSelectedObjects()->ObjectItor() ; Itor ; ++Itor )
	{
		UParticleSystem* DestPartSys = Cast<UParticleSystem>(*Itor);
		if (DestPartSys && (DestPartSys != PartSys))
		{
			UBOOL bSafeToExport = TRUE;

			if (DestPartSys->Emitters.Num() > 0)
			{
				UParticleEmitter* DestEmitter0 = DestPartSys->Emitters(0);

				//@todo. Properly handle these situations...
				if (DestEmitter0->LODLevels.Num() != SelectedEmitter->LODLevels.Num())
				{
					bSafeToExport = FALSE;
				}
				else
				{
					// Make sure each level matches up
					for (INT LODIndex = 0; LODIndex < DestEmitter0->LODLevels.Num(); LODIndex++)
					{
						UParticleLODLevel* DestLODLevel	= DestEmitter0->LODLevels(LODIndex);
						UParticleLODLevel* SelLODLevel	= SelectedEmitter->LODLevels(LODIndex);

						if (DestLODLevel->LevelSetting != SelLODLevel->LevelSetting)
						{
							bSafeToExport = FALSE;
							break;
						}
					}
				}

				// Make sure that each LOD level of the source emitter exists in the destination
				for (INT SourceLODIndex = 0; SourceLODIndex < SelectedEmitter->LODLevels.Num(); SourceLODIndex++)
				{
					UParticleLODLevel* SourceLOD = SelectedEmitter->LODLevels(SourceLODIndex);
					INT SourceLODLevel = SourceLOD->LevelSetting;

					UBOOL bSourceInDest = FALSE;
					for (INT DestLODIndex = 0; DestLODIndex < DestEmitter0->LODLevels.Num(); DestLODIndex++)
					{
						UParticleLODLevel* DestLOD = DestEmitter0->LODLevels(DestLODIndex);
						INT DestLODLevel = DestLOD->LevelSetting;

						if (DestLODLevel == SourceLODLevel)
						{
							bSourceInDest = TRUE;
							break;
						}
					}

					if (bSourceInDest == FALSE)
					{
						// Create it in each emitter
						for (INT EmitterIndex = 0; EmitterIndex < DestPartSys->Emitters.Num(); EmitterIndex++)
						{
							UParticleEmitter* DestEmitter = DestPartSys->Emitters(EmitterIndex);
							if (DestEmitter)
							{
								// 
								INT ResultIndex = DestEmitter->CreateLODLevel(SourceLODLevel, FALSE);
								if (ResultIndex == -1)
								{
									warnf(TEXT("ExportEmitter: WTF Happened???"));
								}
								else
								{
									UParticleLODLevel* NewLODLevel	= DestEmitter->LODLevels(ResultIndex);
									check(NewLODLevel);
									NewLODLevel->UpdateModuleLists();

									// <<<TESTING>>>
									if ((ResultIndex > 0) && (ResultIndex < (DestEmitter->LODLevels.Num() - 1)))
									{
										UParticleLODLevel*	HighLevel	= DestEmitter->LODLevels(ResultIndex - 1);
										UParticleLODLevel*	LowLevel	= DestEmitter->LODLevels(ResultIndex + 1);

										UParticleModule* TargetModule;
										UParticleModule* HighModule;
										UParticleModule* LowModule;

										TargetModule	= NewLODLevel->RequiredModule;
										HighModule		= HighLevel->RequiredModule;
										LowModule		= LowLevel->RequiredModule;

										FLOAT	Percentage	= (
											(FLOAT)(LowLevel->LevelSetting - NewLODLevel->LevelSetting) / 
											(FLOAT)(LowLevel->LevelSetting - HighLevel->LevelSetting));
										GenerateLODModuleValues(TargetModule, HighModule, LowModule, Percentage);

										for (INT ModuleIndex = 0; ModuleIndex < NewLODLevel->Modules.Num(); ModuleIndex++)
										{
											TargetModule	= NewLODLevel->Modules(ModuleIndex);
											HighModule		= HighLevel->Modules(ModuleIndex);
											LowModule		= LowLevel->Modules(ModuleIndex);
											GenerateLODModuleValues(TargetModule, HighModule, LowModule, Percentage);
										}
									}
								}
							}
						}

						bSafeToExport = TRUE;
					}
				}
			}
/***
			if (bSafeToExport == FALSE)
			{
				appMsgf(AMT_OK, *LocalizeUnrealEd("Error_ExportEmitterLODDiscrepancy"));
				continue;
			}
***/
			if (!DuplicateEmitter(SelectedEmitter, DestPartSys))
			{
				appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedToCopy"), 
					*SelectedEmitter->GetEmitterName().ToString(),
					*DestPartSys->GetName());
			}

			DestPartSys->MarkPackageDirty();
		}
	}
}

// Delete selected module from selected emitter.
// Module may be used in other Emitters, so we don't destroy it or anything - garbage collection will handle that.
void WxCascade::DeleteSelectedModule()
{
	if (!SelectedModule || !SelectedEmitter)
	{
		return;
	}

	if (ToolBar->LODSlider->GetValue() != 0)
	{
		// Don't allow deleting modules if not at highest LOD
		warnf(TEXT("Cascade: Attempting to delete module while not on highest LOD (0)"));
		return;
	}

	BeginTransaction(TEXT("DeleteSelectedModule"));
	ModifySelectedObjects();
	ModifyParticleSystem();

	PartSys->PreEditChange(NULL);

	// Find the module index...
	INT	DeleteModuleIndex	= -1;

	UParticleLODLevel* HighLODLevel	= SelectedEmitter->GetLODLevel(0);
	check(HighLODLevel);
	for (INT ModuleIndex = 0; ModuleIndex < HighLODLevel->Modules.Num(); ModuleIndex++)
	{
		UParticleModule* CheckModule = HighLODLevel->Modules(ModuleIndex);
		if (CheckModule == SelectedModule)
		{
			DeleteModuleIndex = ModuleIndex;
			break;
		}
	}

	if (SelectedModule->IsDisplayedInCurveEd(CurveEd->EdSetup) && !ModuleIsShared(SelectedModule))
	{
		// Remove it from the curve editor!
		SelectedModule->RemoveModuleCurvesFromEditor(CurveEd->EdSetup);
	}

	// Check all the others...
	for (INT LODIndex = 1; LODIndex < SelectedEmitter->LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel	= SelectedEmitter->GetLODLevel(LODIndex);
		if (LODLevel)
		{
			UParticleModule* Module;

			if (DeleteModuleIndex >= 0)
			{
				Module = LODLevel->Modules(DeleteModuleIndex);
			}
			else
			{
				Module	= LODLevel->TypeDataModule;
			}

			if (Module)
			{
				Module->RemoveModuleCurvesFromEditor(CurveEd->EdSetup);
			}
		}
			
	}
	CurveEd->CurveEdVC->Viewport->Invalidate();

	if (SelectedEmitter)
	{
		UBOOL bNeedsListUpdated = FALSE;

		for (INT LODIndex = 0; LODIndex < SelectedEmitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel	= SelectedEmitter->GetLODLevel(LODIndex);

			// See if it is in this LODs level...
			UParticleModule* CheckModule;

			if (DeleteModuleIndex >= 0)
			{
				CheckModule = LODLevel->Modules(DeleteModuleIndex);
			}
			else
			{
				CheckModule = LODLevel->TypeDataModule;
			}

			if (CheckModule)
			{
				if (CheckModule->IsA(UParticleModuleTypeDataBase::StaticClass()))
				{
					check(LODLevel->TypeDataModule == CheckModule);
					LODLevel->TypeDataModule = NULL;
				}
				LODLevel->Modules.RemoveItem(CheckModule);
				bNeedsListUpdated = TRUE;
			}
		}

		if (bNeedsListUpdated)
		{
			SelectedEmitter->UpdateModuleLists();
		}
	}
	else
	{
		// Assume that it's in the module dump...
		ModuleDumpList.RemoveItem(SelectedModule);
	}

	PartSys->PostEditChange(NULL);

	SetSelectedEmitter(SelectedEmitter);

	EmitterEdVC->Viewport->Invalidate();

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("DeleteSelectedModule"));
}

void WxCascade::EnableSelectedModule()
{
	if (!SelectedModule && !SelectedEmitter)
	{
		return;
	}

	INT	CurrLODSetting	= ToolBar->LODSlider->GetValue();
	if (SelectedEmitter->IsLODLevelValid(CurrLODSetting) == FALSE)
	{
		return;
	}

	BeginTransaction(TEXT("EnableSelectedModule"));
	ModifySelectedObjects();
	ModifyParticleSystem();

	PartSys->PreEditChange(NULL);

	if (SelectedModule)
	{
		SelectedModule->bEditable = TRUE;
	}
	else
	if (SelectedEmitter)
	{
		UParticleLODLevel* LODLevel = SelectedEmitter->GetClosestLODLevel(CurrLODSetting);
		LODLevel->RequiredModule->bEditable	= TRUE;
	}

	PartSys->PostEditChange(NULL);

	SetSelectedEmitter(SelectedEmitter);

	EmitterEdVC->Viewport->Invalidate();

	PartSys->MarkPackageDirty();

	EndTransaction(TEXT("EnableSelectedModule"));
}

void WxCascade::ResetSelectedModule()
{
}

UBOOL WxCascade::ModuleIsShared(UParticleModule* InModule)
{
	INT FindCount = 0;

	for (INT i = 0; i < PartSys->Emitters.Num(); i++)
	{
		UParticleEmitter* Emitter = PartSys->Emitters(i);
		for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel = Emitter->GetLODLevel(LODIndex);
			for (INT j = 0; j < LODLevel->Modules.Num(); j++)
			{
				if (LODLevel->Modules(j) == InModule)
				{
					FindCount++;
					if (FindCount == 2)
					{
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

void WxCascade::AddSelectedToGraph()
{
	if (!SelectedEmitter)
		return;

	INT	CurrLODSetting	= ToolBar->LODSlider->GetValue();
	if (SelectedEmitter->IsLODLevelValid(CurrLODSetting) == FALSE)
	{
		return;
	}

	if (SelectedModule)
	{
		if (SelectedModule->bEditable)
		{
			SelectedModule->AddModuleCurvesToEditor( PartSys->CurveEdSetup );
		}
	}
	else
	{
		UParticleLODLevel* LODLevel	= SelectedEmitter->GetClosestLODLevel(CurrLODSetting);
		check(LODLevel);
		if (LODLevel->RequiredModule->bEditable)
		{
			LODLevel->RequiredModule->AddModuleCurvesToEditor( PartSys->CurveEdSetup );
		}
	}

	SetSelectedInCurveEditor();
	CurveEd->CurveEdVC->Viewport->Invalidate();
}

void WxCascade::SetSelectedInCurveEditor()
{
	CurveEd->ClearAllSelectedCurves();
	if (SelectedModule)
	{
		TArray<FParticleCurvePair> Curves;
		SelectedModule->GetCurveObjects(Curves);
		for (INT CurveIndex = 0; CurveIndex < Curves.Num(); CurveIndex++)
		{
			UObject* Distribution = Curves(CurveIndex).CurveObject;
			if (Distribution)
			{
				CurveEd->SetCurveSelected(Distribution, TRUE);
			}
		}
	}
	else
	if (SelectedEmitter)
	{
		UParticleLODLevel* LODLevel = SelectedEmitter->GetClosestLODLevel(ToolBar->LODSlider->GetValue());
		CurveEd->SetCurveSelected(LODLevel->RequiredModule->SpawnRate.Distribution, TRUE);
	}

	CurveEd->CurveEdVC->Viewport->Invalidate();
}

void WxCascade::SetCopyEmitter(UParticleEmitter* NewEmitter)
{
	CopyEmitter = NewEmitter;
}

void WxCascade::SetCopyModule(UParticleEmitter* NewEmitter, UParticleModule* NewModule)
{
	CopyEmitter = NewEmitter;
	CopyModule = NewModule;
}

void WxCascade::RemoveModuleFromDump(UParticleModule* Module)
{
	for (INT i = 0; i < ModuleDumpList.Num(); i++)
	{
		if (ModuleDumpList(i) == Module)
		{
			ModuleDumpList.Remove(i);
			break;
		}
	}
}

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxCascade::GetDockingParentName() const
{
	return DockingParent_Name;
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxCascade::GetDockingParentVersion() const
{
	return DockingParent_Version;
}


//
// UCascadeOptions
// 
