/*=============================================================================
	CascadeEmitterEd.cpp: 'Cascade' particle editor emitter editor
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "CurveEd.h"
#include "Cascade.h"

static const INT	EmitterWidth = 180;
static const INT	EmitterHeadHeight = 60;
static const INT	EmitterThumbBorder = 5;

static const INT	ModuleHeight = 40;

enum ECascadeParticleModuleSelected
{
	ECPMS_Unselected	= 0,
	ECPMS_Selected		= 1,
	ECPMS_MAX			= 2
};

static FColor	ModuleColors[EPMT_MAX][ECPMS_MAX] = 
{
	{	FColor(160, 160, 160), FColor(255, 100,   0)	},		//EPMT_General
    {	FColor(200, 200, 200), FColor(255, 200,   0)	},		//EPMT_TypeData
    {	FColor(200, 200, 255), FColor( 50,  50, 200)	},		//EPMT_Beam
	{	FColor( 50, 200, 255), FColor( 50, 100, 150)	},		//EPMT_Trail
};

static FColor EmptyBackgroundColor(112,112,112);
static FColor EmitterBackgroundColor(130, 130, 130);
static FColor EmitterSelectedColor(255, 130, 30);
static FColor EmitterUnselectedColor(180, 180, 180);

static const FColor RenderModeSelected(255,200,0);
static const FColor RenderModeUnselected(112,112,112);

static const FColor Module3DDrawModeEnabledColor(255,200,0);
static const FColor Module3DDrawModeDisabledColor(112,112,112);

#define INDEX_TYPEDATAMODULE	(INDEX_NONE - 1)

/*-----------------------------------------------------------------------------
FCascadePreviewViewportClient
-----------------------------------------------------------------------------*/

FCascadeEmitterEdViewportClient::FCascadeEmitterEdViewportClient(WxCascade* InCascade) :
	  Cascade(InCascade)
{
	CurrentMoveMode = CMMM_None;
	MouseHoldOffset = FIntPoint(0,0);
	MousePressPosition = FIntPoint(0,0);
	bMouseDragging = false;
	bMouseDown = false;
	bPanning = false;
#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
	bDrawModuleDump = InCascade->EditorOptions->bShowModuleDump;
#else	//#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
	bDrawModuleDump = FALSE;
#endif	//#if defined(_CASCADE_ENABLE_MODULE_DUMP_)

	DraggedModule = NULL;

	Origin2D = FIntPoint(0,0);
	OldMouseX = 0;
	OldMouseY = 0;

	ResetDragModIndex = INDEX_NONE;

	EmptyBackgroundColor							= Cascade->EditorOptions->Empty_Background;
	EmptyBackgroundColor.A							= 255;
	EmitterBackgroundColor							= Cascade->EditorOptions->Emitter_Background;
	EmitterBackgroundColor.A						= 255;
	EmitterSelectedColor							= Cascade->EditorOptions->Emitter_Unselected;
	EmitterSelectedColor.A							= 255;
	EmitterUnselectedColor							= Cascade->EditorOptions->Emitter_Selected;
	EmitterUnselectedColor.A						= 255;
	ModuleColors[EPMT_General][ECPMS_Unselected]	= Cascade->EditorOptions->ModuleColor_General_Unselected;
	ModuleColors[EPMT_General][ECPMS_Unselected].A	= 255;
	ModuleColors[EPMT_General][ECPMS_Selected]		= Cascade->EditorOptions->ModuleColor_General_Selected;
	ModuleColors[EPMT_General][ECPMS_Selected].A	= 255;
	ModuleColors[EPMT_TypeData][ECPMS_Unselected]	= Cascade->EditorOptions->ModuleColor_TypeData_Unselected;
	ModuleColors[EPMT_TypeData][ECPMS_Unselected].A	= 255;
	ModuleColors[EPMT_TypeData][ECPMS_Selected]		= Cascade->EditorOptions->ModuleColor_TypeData_Selected;
	ModuleColors[EPMT_TypeData][ECPMS_Selected].A	= 255;
	ModuleColors[EPMT_Beam][ECPMS_Unselected]		= Cascade->EditorOptions->ModuleColor_Beam_Unselected;
	ModuleColors[EPMT_Beam][ECPMS_Unselected].A		= 255;
	ModuleColors[EPMT_Beam][ECPMS_Selected]			= Cascade->EditorOptions->ModuleColor_Beam_Selected;
	ModuleColors[EPMT_Beam][ECPMS_Selected].A		= 255;
	ModuleColors[EPMT_Trail][ECPMS_Unselected]		= Cascade->EditorOptions->ModuleColor_Trail_Unselected;
	ModuleColors[EPMT_Trail][ECPMS_Unselected].A	= 255;
	ModuleColors[EPMT_Trail][ECPMS_Selected]		= Cascade->EditorOptions->ModuleColor_Trail_Selected;
	ModuleColors[EPMT_Trail][ECPMS_Selected].A		= 255;

	CreateIconMaterials();
}

FCascadeEmitterEdViewportClient::~FCascadeEmitterEdViewportClient()
{

}


void FCascadeEmitterEdViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	Canvas->PushAbsoluteTransform(FTranslationMatrix(FVector(Origin2D.X,Origin2D.Y,0)));

    // Clear the background to gray and set the 2D draw origin for the viewport
    if (Canvas->IsHitTesting() == FALSE)
	{
		Clear(Canvas,EmptyBackgroundColor);
	}
	else
	{
		Clear(Canvas,FLinearColor(1.0f,1.0f,1.0f,1.0f));
	}

	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();

	UParticleSystem* PartSys = Cascade->PartSys;

	INT XPos = 0;
	for(INT i=0; i<PartSys->Emitters.Num(); i++)
	{
		UParticleEmitter* Emitter = PartSys->Emitters(i);
		if (Emitter)
		{
			DrawEmitter(i, XPos, Emitter, Viewport, Canvas);
		}
		// Move X position on to next emitter.
		XPos += EmitterWidth;
		// Draw vertical line after last column
		DrawTile(Canvas,XPos - 1, 0, 1, ViewY - Origin2D.Y, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black);
	}

	// Draw line under emitter headers
	DrawTile(Canvas,0, EmitterHeadHeight-1, ViewX - Origin2D.X, 1, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black);

    // Draw the module dump, if it is enabled
    if (bDrawModuleDump)
        DrawModuleDump(Viewport, Canvas);

	// When dragging a module.
	if ((CurrentMoveMode != CMMM_None) && bMouseDragging)
	{
		if (DraggedModule)
			DrawDraggedModule(DraggedModule, Viewport, Canvas);
	}

	Canvas->PopTransform();
}

void FCascadeEmitterEdViewportClient::DrawEmitter(INT Index, INT XPos, UParticleEmitter* Emitter, FViewport* Viewport, FCanvas* Canvas)
{
	INT ViewY = Viewport->GetSizeY();

    // Draw background block

	// Draw header block
	DrawHeaderBlock(Index, XPos, Emitter, Viewport, Canvas);

	// Draw the type data module
	DrawTypeDataBlock(XPos, Emitter, Viewport, Canvas);

    // Draw each module
	INT YPos = EmitterHeadHeight + ModuleHeight;
    INT j;

    // Now, draw the remaining modules
	//@todo. Add appropriate editor support for LOD here!
	UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
	for(j = 0; j < LODLevel->Modules.Num(); j++)
	{
		UParticleModule* Module = LODLevel->Modules(j);
		check(Module);
        if (!Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
        {
            DrawModule(XPos, YPos, Emitter, Module, Viewport, Canvas);
            // Update Y position for next module.
		    YPos += ModuleHeight;
        }
	}
}

void FCascadeEmitterEdViewportClient::DrawHeaderBlock(INT Index, INT XPos, UParticleEmitter* Emitter, FViewport* Viewport,FCanvas* Canvas)
{
	INT ViewY = Viewport->GetSizeY();
	FColor HeadColor = (Emitter == Cascade->SelectedEmitter) ? EmitterSelectedColor : EmitterUnselectedColor;

	INT					CurrLODSetting	= Cascade->ToolBar->LODSlider->GetValue();
	UBOOL				bValidLevel		= Emitter->IsLODLevelValid(CurrLODSetting);
	UParticleLODLevel*	LODLevel		= Emitter->GetClosestLODLevel(CurrLODSetting);

	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(new HCascadeEmitterProxy(Emitter));
	if (LODLevel->RequiredModule->bEditable && bValidLevel)
	{
		DrawTile(Canvas,XPos, 0, EmitterWidth, EmitterHeadHeight, 0.f, 0.f, 1.f, 1.f, HeadColor);
	}
	else
	{
		FTexture* Tex = GetTextureDisabledBackground();
		DrawTile(Canvas,XPos, 0, EmitterWidth, EmitterHeadHeight, 0.f, 0.f, 1.f, 1.f, HeadColor, Tex);
	}

	if (!Canvas->IsHitTesting())
	{
		UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(Emitter);
		if (SpriteEmitter)
		{
			FString TempString;

			TempString = SpriteEmitter->GetEmitterName().ToString();
			DrawShadowedString(Canvas,XPos + 10, 5, *TempString, GEngine->SmallFont, FLinearColor::White);

			INT ThumbSize = EmitterHeadHeight - 2*EmitterThumbBorder;
			FIntPoint ThumbPos(XPos + EmitterWidth - ThumbSize - EmitterThumbBorder, EmitterThumbBorder);
			ThumbPos.X += Origin2D.X;
			ThumbPos.Y += Origin2D.Y;

			//@todo. Add appropriate editor support for LOD here!
			UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
			UParticleLODLevel* HighestLODLevel = Emitter->LODLevels(0);

			TempString = FString::Printf(TEXT("%4d"), HighestLODLevel->PeakActiveParticles);
			DrawShadowedString(Canvas,XPos + 90, 25, *TempString, GEngine->SmallFont, FLinearColor::White);

			// Draw sprite material thumbnail.
			check(LODLevel->RequiredModule);
			if (LODLevel->RequiredModule->Material && !Canvas->IsHitTesting())
			{
				// Get the rendering info for this object
				FThumbnailRenderingInfo* RenderInfo =
					GUnrealEd->GetThumbnailManager()->GetRenderingInfo(LODLevel->RequiredModule->Material);
				// If there is an object configured to handle it, draw the thumbnail
				if (RenderInfo != NULL && RenderInfo->Renderer != NULL)
				{
					RenderInfo->Renderer->Draw(LODLevel->RequiredModule->Material,TPT_Plane,
						ThumbPos.X,ThumbPos.Y,ThumbSize,ThumbSize,Viewport,Canvas,TBT_None);
				}
			}
			else
			{
				DrawTile(Canvas,ThumbPos.X - Origin2D.X, ThumbPos.Y - Origin2D.Y, ThumbSize, ThumbSize, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);		
			}
		}
	}

	// Draw column background
	DrawTile(Canvas,XPos, EmitterHeadHeight, EmitterWidth, ViewY - EmitterHeadHeight - Origin2D.Y, 0.f, 0.f, 1.f, 1.f, EmitterBackgroundColor);
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(NULL);

	//@todo.SAS. Need the DrawTile(..., MaterialInterface*) version of the function!

	// Draw enable/disable button
	FTexture* EnabledIconTxtr = NULL;
	if (LODLevel->RequiredModule->bEnabled)
	{
		EnabledIconTxtr	= GetIconTexture(CASC_Icon_Module_Enabled);
	}
	else
	{
		EnabledIconTxtr	= GetIconTexture(CASC_Icon_Module_Disabled);
	}
	check(EnabledIconTxtr);
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(new HCascadeEmitterEnableProxy(Emitter));
	DrawTile(Canvas,XPos + 12, 26, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, EnabledIconTxtr);
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(NULL);

	// Draw rendering mode button.
	FTexture* IconTxtr	= NULL;
	switch (LODLevel->RequiredModule->EmitterRenderMode)
	{
	case ERM_Normal:
		IconTxtr	= GetIconTexture(CASC_Icon_Render_Normal);
		break;
	case ERM_Point:
		IconTxtr	= GetIconTexture(CASC_Icon_Render_Point);
		break;
	case ERM_Cross:
		IconTxtr	= GetIconTexture(CASC_Icon_Render_Cross);
		break;
	case ERM_None:
		IconTxtr	= GetIconTexture(CASC_Icon_Render_None);
		break;
	}
	check(IconTxtr);

	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(new HCascadeDrawModeButtonProxy(Emitter, LODLevel->RequiredModule->EmitterRenderMode));
	DrawTile(Canvas,XPos + 32, 26, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, IconTxtr);
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(NULL);

	// Draw button for sending emitter properties to graph.
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(new HCascadeGraphButton(Emitter, NULL));
	DrawTile(Canvas,XPos + 52, 26, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_CurveEdit));
	if (Canvas->IsHitTesting())
        Canvas->SetHitProxy(NULL);

	DrawColorButton(XPos, Emitter, NULL, Canvas->IsHitTesting(), Canvas);
}

void FCascadeEmitterEdViewportClient::DrawTypeDataBlock(INT XPos, UParticleEmitter* Emitter, FViewport* Viewport, FCanvas* Canvas)
{
	UParticleLODLevel*	LODLevel	= Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
	UParticleModule*	Module		= LODLevel->TypeDataModule;
	if (Module)
	{
        check(Module->IsA(UParticleModuleTypeDataBase::StaticClass()));
        DrawModule(XPos, EmitterHeadHeight, Emitter, Module, Viewport, Canvas);
	}
}

void FCascadeEmitterEdViewportClient::DrawModule(INT XPos, INT YPos, UParticleEmitter* Emitter, UParticleModule* Module, FViewport* Viewport, FCanvas* Canvas)
{	
    // Hack to ensure no black modules...
	if (Module->ModuleEditorColor == FColor(0,0,0,0))
	{
		Module->ModuleEditorColor = FColor::MakeRandomColor();
	}

    // Grab the correct color to use
	FColor ModuleBkgColor;
	if (Module == Cascade->SelectedModule)
	{
		ModuleBkgColor	= ModuleColors[Module->GetModuleType()][ECPMS_Selected];
	}
	else
	{
		ModuleBkgColor	= ModuleColors[Module->GetModuleType()][ECPMS_Unselected];
	}

    // Offset the 2D draw origin
//	Canvas->PushRelativeTransform(FTranslationMatrix(FVector(XPos + Origin2D.X,YPos + Origin2D.Y,0)));
	Canvas->PushRelativeTransform(FTranslationMatrix(FVector(XPos,YPos,0)));

    // Draw the module box and it's proxy
	DrawModule(Canvas, Module, ModuleBkgColor, Emitter);
	if (Cascade->ModuleIsShared(Module) || Module->IsDisplayedInCurveEd(Cascade->CurveEd->EdSetup))
	{
		DrawColorButton(XPos, Emitter, Module, Canvas->IsHitTesting(), Canvas);
	}

	// Draw little 'send properties to graph' button.
	if (Module->ModuleHasCurves())
	{
		DrawCurveButton(Emitter, Module, Canvas->IsHitTesting(), Canvas);
	}

	// Draw button for 3DDrawMode.
	if (Module->bSupported3DDrawMode)
	{
        Draw3DDrawButton(Emitter, Module, Canvas->IsHitTesting(), Canvas);
	}

    DrawEnableButton(Emitter, Module, Canvas->IsHitTesting(), Canvas);

	Canvas->PopTransform();
}

void FCascadeEmitterEdViewportClient::DrawModule(FCanvas* Canvas, UParticleModule* Module, FColor ModuleBkgColor, UParticleEmitter* Emitter)
{
	UBOOL bValidLevel	= Emitter ? Emitter->IsLODLevelValid(Cascade->ToolBar->LODSlider->GetValue()) : TRUE;

	if (Canvas->IsHitTesting())
		Canvas->SetHitProxy(new HCascadeModuleProxy(Emitter, Module));
	DrawTile(Canvas,-1, -1, EmitterWidth+1, ModuleHeight+2, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black);
	if (Canvas->IsHitTesting())
	{
		Canvas->SetHitProxy(NULL);
		return;
	}

	if (Module->bEditable && bValidLevel)
	{
		DrawTile(Canvas,0, 0, EmitterWidth-1, ModuleHeight, 0.f, 0.f, 1.f, 1.f, ModuleBkgColor);
	}
	else
	{
		FTexture* Tex = GetTextureDisabledBackground();
		DrawTile(Canvas,0, 0, EmitterWidth-1, ModuleHeight, 0.f, 0.f, 1.f, 1.f, ModuleBkgColor, Tex);
	}

	INT XL, YL;
	FString ModuleName = Module->GetClass()->GetDescription();

	// Postfix name with '+' if shared.
	if (Cascade->ModuleIsShared(Module))
		ModuleName = ModuleName + FString(TEXT("+"));

	StringSize(GEngine->SmallFont, XL, YL, *(ModuleName));
	DrawShadowedString(Canvas,10, 3, *(ModuleName), GEngine->SmallFont, FLinearColor::White);
}

void FCascadeEmitterEdViewportClient::DrawDraggedModule(UParticleModule* Module, FViewport* Viewport, FCanvas* Canvas)
{
	FIntPoint MousePos = FIntPoint(Viewport->GetMouseX(), Viewport->GetMouseY()) + Origin2D;

    // Draw indicator for where we would insert this module.
	UParticleEmitter* TargetEmitter = NULL;
	INT TargetIndex = INDEX_NONE;
	FindDesiredModulePosition(MousePos, TargetEmitter, TargetIndex);

	//@todo. Fix the drawing of the 'helper' line!
#if 0
	if (TargetEmitter && TargetIndex != INDEX_NONE)
	{
		FIntPoint DropPos = FindModuleTopLeft(TargetEmitter, NULL, Viewport);
		DropPos.Y = EmitterHeadHeight + (TargetIndex + 1) * ModuleHeight;

		DrawTile(Canvas,DropPos.X, DropPos.Y - 2, EmitterWidth, 5, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
	}
#endif

	// When dragging, draw the module under the mouse cursor.
	FVector Translate(MousePos.X + MouseHoldOffset.X, MousePos.Y + MouseHoldOffset.Y, 0);
	Translate -= Origin2D;
	if (!Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
	{
		Translate.Y += ModuleHeight;
	}

	Canvas->PushRelativeTransform(FTranslationMatrix(Translate));
	DrawModule(Canvas, DraggedModule, EmitterSelectedColor, TargetEmitter);
	Canvas->PopTransform();
}

void FCascadeEmitterEdViewportClient::DrawCurveButton(UParticleEmitter* Emitter, UParticleModule* Module, UBOOL bHitTesting, FCanvas* Canvas)
{
	if (bHitTesting)
        Canvas->SetHitProxy(new HCascadeGraphButton(Emitter, Module));
	DrawTile(Canvas,EmitterWidth - 20, 2, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_CurveEdit));
	if (bHitTesting)
        Canvas->SetHitProxy(NULL);
}

void FCascadeEmitterEdViewportClient::DrawColorButton(INT XPos, UParticleEmitter* Emitter, UParticleModule* Module, UBOOL bHitTesting, FCanvas* Canvas)
{
	if (bHitTesting)
		Canvas->SetHitProxy(new HCascadeColorButtonProxy(Emitter, Module));
	if (Module)
	{
		DrawTile(Canvas,0, 0, 5, ModuleHeight, 0.f, 0.f, 1.f, 1.f, Module->ModuleEditorColor);
	}
	else
	{
		UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
		DrawTile(Canvas,XPos, 0, 5, EmitterHeadHeight, 0.f, 0.f, 1.f, 1.f, LODLevel->RequiredModule->EmitterEditorColor);
	}
	if (bHitTesting)
		Canvas->SetHitProxy(NULL);
}

void FCascadeEmitterEdViewportClient::Draw3DDrawButton(UParticleEmitter* Emitter, UParticleModule* Module, UBOOL bHitTesting, FCanvas* Canvas)
{
	if (bHitTesting)
		Canvas->SetHitProxy(new HCascade3DDrawModeButtonProxy(Emitter, Module));
	if (Module->b3DDrawMode)
	{
		DrawTile(Canvas,EmitterWidth - 40, 21, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_3DDraw_Enabled));
	}
	else
	{
		DrawTile(Canvas,EmitterWidth - 40, 21, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_3DDraw_Disabled));
	}
	if (bHitTesting)
		Canvas->SetHitProxy(NULL);

#if defined(_CASCADE_ALLOW_3DDRAWOPTIONS_)
	if (Module->b3DDrawMode)
    {
        if (bHitTesting)
			Canvas->SetHitProxy(new HCascade3DDrawModeOptionsButtonProxy(Emitter, Module));
		DrawTile(Canvas,10 + 20, 20 + 10, 8, 8, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
		DrawTile(Canvas,11 + 20, 21 + 10, 6, 6, 0.f, 0.f, 1.f, 1.f, FColor(100,200,100));
		if (bHitTesting)
			Canvas->SetHitProxy(NULL);
    }
#endif	//#if defined(_CASCADE_ALLOW_3DDRAWOPTIONS_)
}

void FCascadeEmitterEdViewportClient::DrawEnableButton(UParticleEmitter* Emitter, UParticleModule* Module, UBOOL bHitTesting, FCanvas* Canvas)
{
	if (bHitTesting)
		Canvas->SetHitProxy(new HCascadeEnableButtonProxy(Emitter, Module));
	if (Module->bEnabled)
	{
		DrawTile(Canvas,EmitterWidth - 20, 21, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_Module_Enabled));
	}
	else
	{
		DrawTile(Canvas,EmitterWidth - 20, 21, 16, 16, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, GetIconTexture(CASC_Icon_Module_Disabled));
	}
	if (bHitTesting)
		Canvas->SetHitProxy(NULL);
}

void FCascadeEmitterEdViewportClient::DrawModuleDump(FViewport* Viewport, FCanvas* Canvas)
{
#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();
	UBOOL bHitTesting = RI->IsHitTesting();
    INT XPos = ViewX - EmitterWidth - 1;
	FColor HeadColor = EmitterUnselectedColor;

	FIntPoint SaveOrigin2D = RI->Origin2D;
    RI->SetOrigin2D(0, SaveOrigin2D.Y);

	DrawTile(Canvas,XPos - 2, 0, XPos + 2, ViewY - Origin2D.Y, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
	DrawTile(Canvas,XPos, 0, EmitterWidth, EmitterHeadHeight, 0.f, 0.f, 1.f, 1.f, HeadColor);
    DrawTile(Canvas,XPos, 0, 5, EmitterHeadHeight, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);

    FString ModuleDumpTitle = *LocalizeUnrealEd("ModuleDump");
	
    DrawShadowedString(Canvas,XPos + 10, 5, *ModuleDumpTitle, GEngine->SmallFont, FLinearColor::White);

	// Draw column background
	DrawTile(Canvas,XPos, EmitterHeadHeight, EmitterWidth, ViewY - EmitterHeadHeight - Origin2D.Y, 0.f, 0.f, 1.f, 1.f, FColor(160, 160, 160));
    if (bHitTesting)
        Canvas->SetHitProxy(NULL);

	// Draw the dump module list...
	INT YPos = EmitterHeadHeight;

	FVector2D TempOrigin = Origin2D;
	Origin2D.X = 0;
	for(INT i = 0; i < Cascade->ModuleDumpList.Num(); i++)
	{
		UParticleModule* Module = Cascade->ModuleDumpList(i);
		check(Module);
        DrawModule(XPos, YPos, NULL, Module, Viewport, Canvas);
        // Update Y position for next module.
		YPos += ModuleHeight;
	}

	Origin2D.X = TempOrigin.X;
    RI->SetOrigin2D(SaveOrigin2D);
#endif	//#if defined(_CASCADE_ENABLE_MODULE_DUMP_)
}

void FCascadeEmitterEdViewportClient::SetCanvas(INT X, INT Y)
{
	Origin2D.X = X;
	Origin2D.X = Min(0, Origin2D.X);
	
	Origin2D.Y = Y;
	Origin2D.Y = Min(0, Origin2D.Y);

	Viewport->Invalidate();
	// Force it to draw so the view change is seen
	Viewport->Draw();
}

void FCascadeEmitterEdViewportClient::PanCanvas(INT DeltaX, INT DeltaY)
{
	Origin2D.X += DeltaX;
	Origin2D.X = Min(0, Origin2D.X);
	
	Origin2D.Y += DeltaY;
	Origin2D.Y = Min(0, Origin2D.Y);

	Cascade->EmitterEdWindow->UpdateScrollBar(Origin2D.X, Origin2D.Y);
	Viewport->Invalidate();
}

FMaterialRenderProxy* FCascadeEmitterEdViewportClient::GetIcon(Icons eIcon)
{
	check(!TEXT("Cascade: Invalid Icon Request!"));
	return NULL;
}

FTexture* FCascadeEmitterEdViewportClient::GetIconTexture(Icons eIcon)
{
	if ((eIcon >= 0) && (eIcon < CASC_Icon_COUNT))
	{
		UTexture2D* IconTexture = IconTex[eIcon];
		if (IconTexture)
		{
            return IconTexture->Resource;
		}
	}

	check(!TEXT("Cascade: Invalid Icon Request!"));
	return NULL;
}

FTexture* FCascadeEmitterEdViewportClient::GetTextureDisabledBackground()
{
	return TexModuleDisabledBackground->Resource;
}

void FCascadeEmitterEdViewportClient::CreateIconMaterials()
{
	IconTex[CASC_Icon_Render_Normal]	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_Normal"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_Render_Cross]		= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_Cross"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_Render_Point]		= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_Point"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_Render_None]		= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_None"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_CurveEdit]		= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_CurveEd"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_3DDraw_Enabled]	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_ModuleEnable"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_3DDraw_Disabled]	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_ModuleDisable"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_Module_Enabled]	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_ModuleEnable"),NULL,LOAD_None,NULL);
	IconTex[CASC_Icon_Module_Disabled]	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_ModuleDisable"),NULL,LOAD_None,NULL);

	check(IconTex[CASC_Icon_Render_Normal]);
	check(IconTex[CASC_Icon_Render_Cross]);
	check(IconTex[CASC_Icon_Render_Point]);
	check(IconTex[CASC_Icon_Render_None]);
	check(IconTex[CASC_Icon_CurveEdit]);
	check(IconTex[CASC_Icon_3DDraw_Enabled]);
	check(IconTex[CASC_Icon_3DDraw_Disabled]);
	check(IconTex[CASC_Icon_Module_Enabled]);
	check(IconTex[CASC_Icon_Module_Disabled]);

	TexModuleDisabledBackground	= (UTexture2D*)UObject::StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("EditorMaterials.Cascade.CASC_DisabledModule"), NULL, LOAD_None, NULL);
	check(TexModuleDisabledBackground);
}

UBOOL FCascadeEmitterEdViewportClient::InputKey(FViewport* Viewport, INT ControllerId, FName Key, EInputEvent Event,FLOAT /*AmountDepressed*/,UBOOL /*Gamepad*/)
{
	Viewport->LockMouseToWindow(Viewport->KeyState(KEY_LeftMouseButton) || Viewport->KeyState(KEY_MiddleMouseButton));

	UBOOL				bLODIsValid	= FALSE;
	UParticleSystem*	CascPartSys	= Cascade->PartSys;
	if (CascPartSys)
	{
		if (CascPartSys->Emitters.Num())
		{
			UParticleEmitter* Emitter = CascPartSys->Emitters(0);
			if (Emitter)
			{
				bLODIsValid	= Emitter->IsLODLevelValid(Cascade->ToolBar->LODSlider->GetValue());
			}
		}
	}

	UBOOL bCtrlDown = Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl);
	UBOOL bShiftDown = Viewport->KeyState(KEY_LeftShift) || Viewport->KeyState(KEY_RightShift);
	UBOOL bAltDown = Viewport->KeyState(KEY_LeftAlt) || Viewport->KeyState(KEY_RightAlt);

	INT HitX = Viewport->GetMouseX();
	INT HitY = Viewport->GetMouseY();
	FIntPoint MousePos = FIntPoint(HitX, HitY);

	if (Key == KEY_LeftMouseButton || Key == KEY_RightMouseButton)
	{
		if (Event == IE_Pressed)
		{
			// Ignore pressing other mouse buttons while panning around.
			if (bPanning)
				return FALSE;

			if (Key == KEY_LeftMouseButton)
			{
				MousePressPosition = MousePos;
				bMouseDown = true;
			}

			HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);
			wxMenu* Menu = NULL;

			//@todo.SAS. Investigate why clicking on the empty background is generating a hit proxy...
			// Short-term, performing a quick-out
			UBOOL bHandledHitProxy = TRUE;

			if (HitResult)
			{
				if (HitResult->IsA(HCascadeEmitterProxy::StaticGetType()))
				{
					UParticleEmitter* Emitter = ((HCascadeEmitterProxy*)HitResult)->Emitter;
					Cascade->SetSelectedEmitter(Emitter);

					if (Key == KEY_RightMouseButton)
					{
						Menu = new WxMBCascadeEmitterBkg(Cascade, WxMBCascadeEmitterBkg::EVERYTHING);
					}
				}
				else
				if (HitResult->IsA(HCascadeEmitterEnableProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleEmitter*	Emitter		= ((HCascadeDrawModeButtonProxy*)HitResult)->Emitter;

						INT LODLevelValue = Cascade->ToolBar->LODSlider->GetValue();
						if (Emitter->IsLODLevelValid(LODLevelValue))
						{
							UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(LODLevelValue);
							if (LODLevel->RequiredModule->bEditable == TRUE)
							{
								LODLevel->bEnabled	= !LODLevel->bEnabled;
								LODLevel->RequiredModule->bEnabled = LODLevel->bEnabled;
							}

//							Cascade->OnResetSystem(wxCommandEvent());
						}
					}
				}
				else
				if (HitResult->IsA(HCascadeDrawModeButtonProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleEmitter*	Emitter		= ((HCascadeDrawModeButtonProxy*)HitResult)->Emitter;
						EEmitterRenderMode	DrawMode	= (EEmitterRenderMode)((HCascadeDrawModeButtonProxy*)HitResult)->DrawMode;

						switch (DrawMode)
						{
						case ERM_Normal:	DrawMode	= ERM_Point;	break;
						case ERM_Point:		DrawMode	= ERM_Cross;	break;
						case ERM_Cross:		DrawMode	= ERM_None;		break;
						case ERM_None:		DrawMode	= ERM_Normal;	break;
						}
						Cascade->SetSelectedEmitter(Emitter);
						UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
//						if (LODLevel->RequiredModule->bEditable == TRUE)
						{
							LODLevel->RequiredModule->EmitterRenderMode	= DrawMode;
						}
					}
				}
				else
				if (HitResult->IsA(HCascadeColorButtonProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleEmitter* Emitter = ((HCascadeModuleProxy*)HitResult)->Emitter;
						UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;

						if (Module || Emitter)
						{
							wxColour wxColorIn;
							if (Module)
							{
								wxColorIn	= wxColour(Module->ModuleEditorColor.R, Module->ModuleEditorColor.G, Module->ModuleEditorColor.B);
							}
							else
							{
								check(Emitter);
								UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
								wxColorIn	= wxColour(LODLevel->RequiredModule->EmitterEditorColor.R, LODLevel->RequiredModule->EmitterEditorColor.G, LODLevel->RequiredModule->EmitterEditorColor.B);
							}

							// Let go of the mouse lock...
							Viewport->LockMouseToWindow(FALSE);
							// Get the color from the user
							wxColour wxColorOut = wxGetColourFromUser(Cascade, wxColorIn);
							if (wxColorOut.Ok())
							{
								// Assign it
								if (Module)
								{
									FColor	NewColor(wxColorOut.Red(), wxColorOut.Green(), wxColorOut.Blue());
									Module->ChangeEditorColor(NewColor, Cascade->CurveEd->EdSetup);
								}
								else
								{
									FColor	NewColor(wxColorOut.Red(), wxColorOut.Green(), wxColorOut.Blue());
									Emitter->ChangeEditorColor(NewColor, Cascade->CurveEd->EdSetup);
								}
							}
							Viewport->Invalidate();
							Cascade->CurveEd->CurveEdVC->Viewport->Invalidate();
						}
					}
				}
				else
				if (HitResult->IsA(HCascadeModuleProxy::StaticGetType()))
				{
					UParticleEmitter* Emitter = ((HCascadeModuleProxy*)HitResult)->Emitter;
					UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;

					Cascade->SetSelectedModule(Emitter, Module);

					if (Key == KEY_RightMouseButton)
					{
						if (bMouseDragging)// && (CurrentMoveMode != CMMM_None))
						{
							// Don't allow menu pop-up while moving modules...
						}
						else
						{
							Menu = new WxMBCascadeModule(Cascade);
						}
					}
					else
					{
						check(Cascade->SelectedModule);

						// We are starting to drag this module. Look at keys to see if we are moving/instancing
						if (bCtrlDown || bAltDown)
                        {
                        	Cascade->SetCopyModule(Emitter, Module);
                            CurrentMoveMode = CMMM_Copy;
                        }
                        else
                        if (bShiftDown)
                        {
							CurrentMoveMode = CMMM_Instance;
                        }
						else
                        {
							CurrentMoveMode = CMMM_Move;
                        }

						// Figure out and save the offset from mouse location to top-left of selected module.
						FIntPoint ModuleTopLeft = FindModuleTopLeft(Emitter, Module, Viewport);
						MouseHoldOffset = ModuleTopLeft - MousePressPosition;
					}
				}
				else
				if (HitResult->IsA(HCascadeGraphButton::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleEmitter* Emitter = ((HCascadeModuleProxy*)HitResult)->Emitter;
						UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;

						if (Module)
						{
							Cascade->SetSelectedModule(Emitter, Module);
						}
						else
						{
							Cascade->SetSelectedEmitter(Emitter);
						}

						Cascade->AddSelectedToGraph();
					}
				}
				else
				if (HitResult->IsA(HCascade3DDrawModeButtonProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;
						check(Module);
						Module->b3DDrawMode = !Module->b3DDrawMode;
					}
				}
                else
				if (HitResult->IsA(HCascade3DDrawModeOptionsButtonProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;
						check(Module);
						// Pop up an options dialog??
						appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_7"));
					}
				}
				else
				if (HitResult->IsA(HCascadeEnableButtonProxy::StaticGetType()))
				{
					if (bLODIsValid)
					{
						UParticleModule* Module = ((HCascadeModuleProxy*)HitResult)->Module;
						check(Module);
						Module->bEnabled = !Module->bEnabled;
					}
				}
				else
				{
					bHandledHitProxy = FALSE;
				}
			}
			else
			{
				bHandledHitProxy = FALSE;
			}

			if (bHandledHitProxy == FALSE)
			{
				Cascade->SetSelectedModule(NULL, NULL);

				if (Key == KEY_RightMouseButton)
					Menu = new WxMBCascadeBkg(Cascade);
			}

			if (Menu)
			{
				FTrackPopupMenu tpm(Cascade, Menu);
				tpm.Show();
				delete Menu;
			}
		}
		else 
		if (Event == IE_Released)
		{
			// If we were dragging a module, find where the mouse currently is, and move module there
			if ((CurrentMoveMode != CMMM_None) && bMouseDragging)
			{
				if (DraggedModule)
				{
					// Find where to move module to.
					UParticleEmitter* TargetEmitter = NULL;
					INT TargetIndex = INDEX_NONE;
					FindDesiredModulePosition(MousePos, TargetEmitter, TargetIndex);

					if (!TargetEmitter || TargetIndex == INDEX_NONE)
					{
						// If the target is the DumpModules area, add it to the list of dump modules
						if (bDrawModuleDump)
						{
							Cascade->ModuleDumpList.AddItem(DraggedModule);
							DraggedModule = NULL;
						}
						else
						// If target is invalid and we were moving it, put it back where it came from.
						if (CurrentMoveMode == CMMM_Move)
						{
							if ((ResetDragModIndex != INDEX_NONE) && Cascade->SelectedEmitter)
							{
								Cascade->InsertModule(DraggedModule, Cascade->SelectedEmitter, ResetDragModIndex);
								Cascade->SelectedEmitter->UpdateModuleLists();
								Cascade->RemoveModuleFromDump(DraggedModule);
							}
							else
							{
								Cascade->ModuleDumpList.AddItem(DraggedModule);
							}
						}
					}
					else
					{
						// Add dragged module in new location.
						if ((CurrentMoveMode == CMMM_Move) || (CurrentMoveMode == CMMM_Instance) ||
                            (CurrentMoveMode == CMMM_Copy))
						{
                            if (CurrentMoveMode == CMMM_Copy)
                            {
								Cascade->CopyModuleToEmitter(DraggedModule, TargetEmitter, Cascade->PartSys);
							    TargetEmitter->UpdateModuleLists();
							    Cascade->RemoveModuleFromDump(DraggedModule);
                            }
							else
                            {
							    Cascade->InsertModule(DraggedModule, TargetEmitter, TargetIndex);
							    TargetEmitter->UpdateModuleLists();
							    Cascade->RemoveModuleFromDump(DraggedModule);
                            }

							wxCommandEvent DummyEvent;
							Cascade->OnResetSystem(DummyEvent);
						}
					}
				}
			}

			bMouseDown = false;
			bMouseDragging = false;
			CurrentMoveMode = CMMM_None;
			DraggedModule = NULL;

			Viewport->Invalidate();
		}
	}
	else
	if (Key == KEY_MiddleMouseButton)
	{
		if (Event == IE_Pressed)
		{
			bPanning = true;

			OldMouseX = HitX;
			OldMouseY = HitY;
		}
		else
		if (Event == IE_Released)
		{
			bPanning = false;
		}
	}

	if (Event == IE_Pressed)
	{
		if (bMouseDragging && (CurrentMoveMode != CMMM_None))
		{
			// Don't allow deleting while moving modules...
		}
		else
		{
			if (Key == KEY_Delete)
			{
				if (Cascade->SelectedModule)
				{
					Cascade->DeleteSelectedModule();
				}
				else
				{
					Cascade->DeleteSelectedEmitter();
				}
			}
			else
			if (Key == KEY_Left)
			{
				Cascade->MoveSelectedEmitter(-1);
			}
			else
			if (Key == KEY_Right)
			{
				Cascade->MoveSelectedEmitter(1);
			}
			else
			if ((Key == KEY_Z) && bCtrlDown)
			{
				Cascade->CascadeUndo();
			}
			else
			if ((Key == KEY_Y) && bCtrlDown)
			{
				Cascade->CascadeRedo();
			}
#if defined(_CASCADE_SPACEBAR_RESET_IN_EMITTER_ED_)
			else
			if (Key == KEY_SpaceBar)
			{
				//
				if (Cascade->EditorOptions->bUseSpaceBarReset)
				{
					wxCommandEvent In;
					Cascade->OnResetSystem(In);
				}
			}
#endif	//#if defined(_CASCADE_SPACEBAR_RESET_IN_EMITTER_ED_)
		}
	}


	// Handle viewport screenshot.
	InputTakeScreenshot( Viewport, Key, Event );

	return TRUE;
}

void FCascadeEmitterEdViewportClient::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	if (bPanning)
	{
		INT DeltaX = X - OldMouseX;
		OldMouseX = X;

		INT DeltaY = Y - OldMouseY;
		OldMouseY = Y;

		PanCanvas(DeltaX, DeltaY);
		return;
	}

	// Update bMouseDragging.
	if (bMouseDown && !bMouseDragging)
	{
		// See how far mouse has moved since we pressed button.
		FIntPoint TotalMouseMove = FIntPoint(X,Y) - MousePressPosition;

		if (TotalMouseMove.Size() > 4)
		{
			bMouseDragging = TRUE;
		}

		if (Cascade->SelectedEmitter)
		{
			if (Cascade->ToolBar->LODSlider->GetValue() != 0)
			{
				debugf(TEXT("Mouse down on invalid LOD!"));
				MousePressPosition = FIntPoint(X,Y);
				bMouseDragging = FALSE;
			}
		}

		// If we are moving a module, here is where we remove it from its emitter.
		// Should not be able to change the CurrentMoveMode unless a module is selected.
		if (bMouseDragging && (CurrentMoveMode != CMMM_None))
		{
			if (Cascade->SelectedModule)
			{
				DraggedModule = Cascade->SelectedModule;

				// DraggedModules
				if (DraggedModules.Num() == 0)
				{
					if (Cascade->SelectedEmitter)
					{
						// We are pulling from an emitter...
						DraggedModules.Insert(0, Cascade->SelectedEmitter->LODLevels.Num());
						for (INT LODIndex = 0; LODIndex < Cascade->SelectedEmitter->LODLevels.Num(); LODIndex++)
						{
							UParticleLODLevel* LODLevel = Cascade->SelectedEmitter->LODLevels(LODIndex);
							if (LODLevel)
							{
								if (Cascade->SelectedModuleIndex >= 0)
								{
									DraggedModules(LODIndex)	= LODLevel->Modules(Cascade->SelectedModuleIndex);
								}
								else
								{
									//@todo. Implement code for type data modules!
									DraggedModules(LODIndex)	= LODLevel->TypeDataModule;
								}
							}
						}
					}
				}

				if (CurrentMoveMode == CMMM_Move)
				{
					// Remeber where to put this module back to if we abort the move.
					ResetDragModIndex = INDEX_NONE;
					if (Cascade->SelectedEmitter)
					{
						UParticleLODLevel* LODLevel = Cascade->SelectedEmitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
						for (INT i=0; i < LODLevel->Modules.Num(); i++)
						{
							if (LODLevel->Modules(i) == Cascade->SelectedModule)
								ResetDragModIndex = i;
						}
						if (ResetDragModIndex == INDEX_NONE)
						{
							if (Cascade->SelectedModule->IsA(UParticleModuleTypeDataBase::StaticClass()))
							{
								ResetDragModIndex = INDEX_TYPEDATAMODULE;
							}							
						}

						check(ResetDragModIndex != INDEX_NONE);
						Cascade->DeleteSelectedModule();
					}
					else
					{
						// Remove the module from the dump
						Cascade->RemoveModuleFromDump(Cascade->SelectedModule);
					}
				}
			}
		}
	}

	// If dragging a module around, update each frame.
	if (bMouseDragging && CurrentMoveMode != CMMM_None)
	{
		Viewport->Invalidate();
	}
}

UBOOL FCascadeEmitterEdViewportClient::InputAxis(FViewport* Viewport, INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad)
{
	return FALSE;
}

// Given a screen position, find which emitter/module index it corresponds to.
void FCascadeEmitterEdViewportClient::FindDesiredModulePosition(const FIntPoint& Pos, class UParticleEmitter* &OutEmitter, INT &OutIndex)
{
	INT EmitterIndex = (Pos.X - Origin2D.X) / EmitterWidth;

	// If invalid Emitter, return nothing.
	if (EmitterIndex < 0 || EmitterIndex > Cascade->PartSys->Emitters.Num()-1)
	{
		OutEmitter = NULL;
		OutIndex = INDEX_NONE;
		return;
	}

	OutEmitter = Cascade->PartSys->Emitters(EmitterIndex);
	UParticleLODLevel* LODLevel	= OutEmitter->LODLevels(0);
	OutIndex = Clamp<INT>(((Pos.Y - Origin2D.Y) - EmitterHeadHeight - ModuleHeight) / ModuleHeight, 0, LODLevel->Modules.Num());
}

FIntPoint FCascadeEmitterEdViewportClient::FindModuleTopLeft(class UParticleEmitter* Emitter, class UParticleModule* Module, FViewport* Viewport)
{
	INT i;

	INT EmitterIndex = -1;
	for(i = 0; i < Cascade->PartSys->Emitters.Num(); i++)
	{
		if (Cascade->PartSys->Emitters(i) == Emitter)
			EmitterIndex = i;
	}

	INT ModuleIndex = 0;

	if (EmitterIndex != -1)
	{
		if (Module && Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
		{
			return FIntPoint(EmitterIndex*EmitterWidth, EmitterHeadHeight);
		}
		else
		{
			UParticleLODLevel* LODLevel = Emitter->GetClosestLODLevel(Cascade->ToolBar->LODSlider->GetValue());
			for(i = 0; i < LODLevel->Modules.Num(); i++)
			{
				if (LODLevel->Modules(i) == Module)
				{
					ModuleIndex = i;
				}
			}
		}

		return FIntPoint(EmitterIndex*EmitterWidth, EmitterHeadHeight + ModuleIndex*ModuleHeight);
	}

	// Must be in the module dump...
	check(Cascade->ModuleDumpList.Num());
	for (i = 0; i < Cascade->ModuleDumpList.Num(); i++)
	{
		if (Cascade->ModuleDumpList(i) == Module)
		{
			INT OffsetHeight = 0;
			if (!Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
			{
				// When we grab from the dump, we need to account for no 'TypeData'
				OffsetHeight = ModuleHeight;
			}
			return FIntPoint(Viewport->GetSizeX() - EmitterWidth - Origin2D.X, EmitterHeadHeight - OffsetHeight + i * EmitterHeadHeight - Origin2D.Y);
		}
	}

	return FIntPoint(0.f, 0.f);
}

/*-----------------------------------------------------------------------------
	WxCascadeEmitterEd
-----------------------------------------------------------------------------*/

BEGIN_EVENT_TABLE(WxCascadeEmitterEd, wxWindow)
	EVT_SIZE(WxCascadeEmitterEd::OnSize)
	EVT_SCROLL(WxCascadeEmitterEd::OnScroll)
	EVT_MOUSEWHEEL(WxCascadeEmitterEd::OnMouseWheel)
END_EVENT_TABLE()

WxCascadeEmitterEd::WxCascadeEmitterEd(wxWindow* InParent, wxWindowID InID, class WxCascade* InCascade )
: wxWindow(InParent, InID)
{
	ScrollBar_Horz = new wxScrollBar(this, ID_CASCADE_HORZ_SCROLL_BAR, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
	ScrollBar_Vert = new wxScrollBar(this, ID_CASCADE_VERT_SCROLL_BAR, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

	wxRect rc = GetClientRect();
	wxRect rcSBH = ScrollBar_Horz->GetClientRect();
	wxRect rcSBV = ScrollBar_Vert->GetClientRect();
	ScrollBar_Horz->SetSize(rc.GetLeft(), rc.GetTop() + rc.GetHeight() - rcSBH.GetHeight(), rc.GetWidth() - rcSBV.GetWidth(), rcSBH.GetHeight());
	ScrollBar_Vert->SetSize(rc.GetLeft() + rc.GetWidth() - rcSBV.GetWidth(), rc.GetTop(), rcSBV.GetWidth(), rc.GetHeight());

	ScrollBar_Horz->SetThumbPosition(0);
    ScrollBar_Horz->SetScrollbar(0, EmitterWidth, EmitterWidth * 25, EmitterWidth - 1);
    ScrollBar_Vert->SetThumbPosition(0);
    ScrollBar_Vert->SetScrollbar(0, ModuleHeight, ModuleHeight * 25, ModuleHeight - 1);

	ThumbPos_Horz = 0;
	ThumbPos_Vert = 0;

	EmitterEdVC = new FCascadeEmitterEdViewportClient(InCascade);
	EmitterEdVC->Viewport = GEngine->Client->CreateWindowChildViewport(EmitterEdVC, (HWND)GetHandle());
	EmitterEdVC->Viewport->CaptureJoystickInput(false);

	EmitterEdVC->SetCanvas(-ThumbPos_Horz, -ThumbPos_Vert);
}

WxCascadeEmitterEd::~WxCascadeEmitterEd()
{
	GEngine->Client->CloseViewport(EmitterEdVC->Viewport);
	EmitterEdVC->Viewport = NULL;
	delete EmitterEdVC;
	delete ScrollBar_Horz;
	delete ScrollBar_Vert;
}

void WxCascadeEmitterEd::OnSize(wxSizeEvent& In)
{
	wxRect rc = GetClientRect();

	wxRect rcSBH = ScrollBar_Horz->GetClientRect();
	wxRect rcSBV = ScrollBar_Vert->GetClientRect();
	ScrollBar_Horz->SetSize(rc.GetLeft(), rc.GetTop() + rc.GetHeight() - rcSBH.GetHeight(), rc.GetWidth() - rcSBV.GetWidth(), rcSBH.GetHeight());
	ScrollBar_Vert->SetSize(rc.GetLeft() + rc.GetWidth() - rcSBV.GetWidth(), rc.GetTop(), rcSBV.GetWidth(), rc.GetHeight());

	INT	PosX	= 0;
	INT	PosY	= 0;
	INT	Width	= rc.GetWidth() - rcSBV.GetWidth();
	INT	Height	= rc.GetHeight() - rcSBH.GetHeight();

	::MoveWindow((HWND)EmitterEdVC->Viewport->GetWindow(), PosX, PosY, Width, Height, 1);

	Refresh();
	if (EmitterEdVC && EmitterEdVC->Viewport && EmitterEdVC->Cascade && EmitterEdVC->Cascade->ToolBar)
	{
        EmitterEdVC->Viewport->Invalidate();
		EmitterEdVC->Viewport->Draw();
	}
}

// Updates the scrollbars values
void WxCascadeEmitterEd::UpdateScrollBar(INT Horz, INT Vert)
{
	ThumbPos_Horz = -Horz;
	ThumbPos_Vert = -Vert;
	ScrollBar_Horz->SetThumbPosition(ThumbPos_Horz);
	ScrollBar_Vert->SetThumbPosition(ThumbPos_Vert);
}

void WxCascadeEmitterEd::OnScroll(wxScrollEvent& In)
{
    wxScrollBar* InScrollBar = wxDynamicCast(In.GetEventObject(), wxScrollBar);
    if (InScrollBar) 
	{
        if (InScrollBar->IsVertical())
		{
			ThumbPos_Vert = In.GetPosition();
			EmitterEdVC->SetCanvas(-ThumbPos_Horz, -ThumbPos_Vert);
		}
        else
		{
			ThumbPos_Horz = In.GetPosition();
			EmitterEdVC->SetCanvas(-ThumbPos_Horz, -ThumbPos_Vert);
		}
    }
}

void WxCascadeEmitterEd::OnMouseWheel(wxMouseEvent& In)
{
}
