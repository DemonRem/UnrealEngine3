//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameVehicleClasses.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(UUTUIScene_Hud);
IMPLEMENT_CLASS(UUTUIScene_Pawns);
IMPLEMENT_CLASS(UUTUIScene_UTPawn);
IMPLEMENT_CLASS(UUTUIScene_WeaponHud);
IMPLEMENT_CLASS(UUTUI_HudWidget);
IMPLEMENT_CLASS(UHudWidget_PawnDoll);
IMPLEMENT_CLASS(UHudWidget_WeaponGroup);
IMPLEMENT_CLASS(UHudWidget_DeployableGroup);
IMPLEMENT_CLASS(UHudWidget_WeaponBar);
IMPLEMENT_CLASS(UHudWidget_TeamScore);
IMPLEMENT_CLASS(UHudWidget_TeamBar);
IMPLEMENT_CLASS(UHudWidget_LocalMessages);

IMPLEMENT_CLASS(UUTUIScene_Scoreboard);

IMPLEMENT_CLASS(UWeapHud_RocketLauncher);

IMPLEMENT_CLASS(UUTUIScene_WeaponQuickPick)
IMPLEMENT_CLASS(UHudWidget_QuickPickCell);

IMPLEMENT_CLASS(UMiniMapImage);
IMPLEMENT_CLASS(UHudWidget_Map);

IMPLEMENT_CLASS(UUTUIScene_MOTD);

IMPLEMENT_CLASS(UUIState_UTObjStates);
	IMPLEMENT_CLASS(UUIState_UTObjActive);
	IMPLEMENT_CLASS(UUIState_UTObjBuilding);
	IMPLEMENT_CLASS(UUIState_UTObjCritical);
	IMPLEMENT_CLASS(UUIState_UTObjInactive);
	IMPLEMENT_CLASS(UUIState_UTObjNeutral);
	IMPLEMENT_CLASS(UUIState_UTObjProtected);
	IMPLEMENT_CLASS(UUIState_UTObjUnderAttack);


IMPLEMENT_CLASS(UUTUIScene_MapVote);

/*=========================================================================================
	THis is the acutal hud actor
========================================================================================= */


/**
 * Look at all Transient properties of the hud that are children of HudWidget and
 * auto-link them to objects in the scene
 */
void AUTHUD::LinkToHudScene()
{
/*
	if ( HudScene )	
	{
		for( TFieldIterator<UObjectProperty,CLASS_IsAUObjectProperty> It( GetClass() ); It; ++It)
		{
			UObjectProperty* P = *It;
			DWORD PropertyFlagMask = (P->PropertyFlags & CPF_Transient);
			if ( PropertyFlagMask != 0 )
			{
				for (INT Idx=0; Idx < It->ArrayDim; Idx++)
				{
					// Find the location of the data
					BYTE* ObjLoc = (BYTE*)this + It->Offset + (Idx * It->ElementSize);
					UObject* ObjPtr = *((UObject**)( ObjLoc ));
					if ( !ObjPtr )
					{
						// TypeCheck!!
						UUIObject* SourceObj = HudScene->FindChild( FName( *P->GetName() ), TRUE);
						if (SourceObj && SourceObj->GetClass()->IsChildOf(P->PropertyClass ) )
						{
							// Copy the value in to place
							P->CopyCompleteValue( ObjLoc, &SourceObj);
						}
					}
				}
			}
		}
	}
*/
}

void AUTHUD::DrawGlowText(const FString &Text, FLOAT X, FLOAT Y, FLOAT MaxHeightInPixels, FLOAT PulseScale, UBOOL bRightJustified)
{
	if ( Canvas )
	{
		INT XL,YL;
		FLOAT Scale;

		Scale = 1.0f;

		Canvas->CurX = 0;
		Canvas->CurY = 0;

		// Size the string
		Canvas->WrappedStrLenf(GlowFonts[0], Scale, Scale, XL, YL, *Text);

		FLOAT Width = FLOAT(XL);
		FLOAT Height = FLOAT(YL);

		// Figure out the scaling

		if (YL > MaxHeightInPixels)
		{
			Scale = MaxHeightInPixels / YL;
		}

		if ( PulseScale > 1.0f )
		{
			FLOAT CX = X + (FLOAT(XL) * 0.5);
			FLOAT CY = Y + (FLOAT(YL) * 0.5);

			Width = (FLOAT(XL) * PulseScale * Scale);
			Height = (FLOAT(YL) * PulseScale * Scale);

			X = CX - ( Width * 0.5);
			Y = CY - ( Height * 0.5);
		}
		else
		{
			Width *= Scale;
			Height *= Scale;
		}

		Scale *= PulseScale;

		Canvas->CurX = bRightJustified ? X - Width : X;
		Canvas->CurY = Y;

		Canvas->WrappedPrint(true, XL, YL, GlowFonts[0], Scale, Scale, false, *Text);

		Canvas->CurX = bRightJustified ? X - Width : X;
		Canvas->CurY = Y;

		Canvas->WrappedPrint(true, XL, YL, GlowFonts[1], Scale, Scale, false, *Text);
	}
}


/*=========================================================================================
  UTUIScene_Hud - Hud Scenes
 have a speical 1 on 1 releationship with a viewport
  ========================================================================================= */
/**
 * @Returns the UTHUD associated with this scene
 */

AUTHUD* UUTUIScene_Hud::GetPlayerHud()
{
	AUTPlayerController* PC = GetUTPlayerOwner();
	if ( PC )
	{
		return Cast<AUTHUD>( PC->myHUD );
	}
	return NULL;
}

/*=========================================================================================
  UTUI_HudWidget - UTUI_HudWidgets are collections of other widgets that share game logic.
  ========================================================================================= */

/** 
 * Cache a reference to the scene owner
 * @see UUIObject.Intiailize
 */

void UUTUI_HudWidget::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
	UTHudSceneOwner = Cast<UUTUIScene_Hud>(inOwnerScene);
	OpacityTarget = Opacity;
}


void UUTUI_HudWidget::Tick_Widget(FLOAT DeltaTime)
{
	AGameReplicationInfo* GRI = GWorld->GetWorldInfo()->GRI;

	// Auto-Manage visibility

	if ( !bManualVisibility )
	{
		if ( GRI && GIsGame )
		{
			UBOOL bNewHidden;
			if ( GRI->bMatchHasBegun )
			{
				if ( GRI->bMatchIsOver )
				{
					bNewHidden = !bVisibleAfterMatch;
				}
				else
				{
					bNewHidden = !bVisibleDuringMatch;
				}
			}
			else
			{
				bNewHidden = !bVisibleBeforeMatch;
			}

			if ( eventPlayerOwnerIsSpectating() && !bVisibleWhileSpectating )
			{
				bNewHidden = TRUE;
			}

			const UBOOL bCurrentlyHidden = IsHidden();
			if ( bNewHidden != bCurrentlyHidden )
			{
				eventSetVisibility(!bNewHidden);
			}
		}
		else
		{
			eventSetVisibility(TRUE);
		}
	}

	Super::Tick_Widget(DeltaTime);
	eventWidgetTick(DeltaTime);

	// Update Any Animations

	UpdateAnimations(DeltaTime);
}

void UUTUI_HudWidget::UpdateAnimations(FLOAT DeltaTime)
{
	for (INT i=0; i<Animations.Num(); i++)
	{
		if ( Animations(i).bIsPlaying )
		{

			INT SeqIndex = Animations(i).SeqIndex;
			FWidgetAnimSequence Seq = Animations(i).Sequences(SeqIndex);
			
			FLOAT A = ( Seq.EndValue - Animations(i).Value ) ;
			FLOAT B = ( DeltaTime / Animations(i).Time );

			Animations(i).Value += A * B;
			Animations(i).Time -= DeltaTime;
			
			if ( Animations(i).Time < 0.0f )
			{
				Animations(i).bIsPlaying = false;
				Animations(i).Value = Seq.EndValue;

				if ( Animations(i).bNotifyWhenFinished )
				{
					UTSceneOwner->eventOnAnimationFinished(this, Animations(i).Tag, Seq.Tag);
				}
			}
			SetPosition( Animations(i).Value, (Animations(i).Property == EAPT_PositionLeft) ? UIFACE_Left : UIFACE_Top, EVALPOS_PercentageOwner);
		}
	}
}

UBOOL UUTUI_HudWidget::PlayAnimation(FName AnimTag, FName SeqTag, UBOOL bForceBeginning)
{
	UBOOL bResults = false;
	for (INT i=0; i<Animations.Num(); i++)
	{
		if ( Animations(i).Tag == AnimTag && Animations(i).Property != EAPT_None )
		{
			for ( INT SeqIndex = 0; SeqIndex < Animations(i).Sequences.Num(); SeqIndex++ )
			{
				FWidgetAnimSequence Seq = Animations(i).Sequences(SeqIndex);
				if (Seq.Tag == SeqTag)
				{
					Animations(i).SeqIndex = SeqIndex;
					if (!Animations(i).bIsPlaying || bForceBeginning)
					{
						Animations(i).Value = Seq.StartValue;
						Animations(i).Time = Seq.Rate;
					}
					else
					{
						FLOAT Pos = Abs(Seq.EndValue - Animations(i).Value);
						FLOAT Max = Abs(Seq.EndValue - Seq.StartValue);
						FLOAT Perc =  Pos / Max;
						Animations(i).Time = Seq.Rate * Perc;
					}
					
					Animations(i).bIsPlaying = true;
					bResults = true;
					break;
				}
			}
			break;
		}
	}
	return bResults;
}

void UUTUI_HudWidget::StopAnimation(FName AnimTag, UBOOL bForceEnd)
{
	for (INT i=0; i<Animations.Num(); i++)
	{
		if ( Animations(i).Tag == AnimTag )
		{
			Animations(i).bIsPlaying = false;
			if ( bForceEnd )
			{
				FWidgetAnimSequence Seq = Animations(i).Sequences(Animations(i).SeqIndex);
				Animations(i).Value = Seq.EndValue;
				SetPosition( Animations(i).Value, (Animations(i).Property == EAPT_PositionLeft) ? UIFACE_Left : UIFACE_Top, EVALPOS_PercentageOwner);
			}
			break;
		}
	}
}

/*=========================================================================================
  HudWidget_WeaponGroup - This is a complex widget that manges what weapons are available in
  a given group.
  ========================================================================================= */

void UHudWidget_WeaponGroup::Tick_Widget(FLOAT DeltaTime)
{
	eventSetVisibility(FALSE);
}

/** 
 * Changes the state of all it's children.  
 *
 * @Param	bIsActive		If true, it will add the activate state, otherwise it will remove it
 */
void UHudWidget_WeaponGroup::ChangeState(UBOOL bIsActive)
{
	for (INT ChildIdx = 0 ; ChildIdx < Children.Num(); ChildIdx++)
	{
		if ( bIsActive )
		{
			Children(ChildIdx)->ActivateStateByClass(UUIState_Active::StaticClass(),0);
		}
		else
		{
			Children(ChildIdx)->DeactivateStateByClass(UUIState_Active::StaticClass(),0);
		}
	}
}

void UHudWidget_WeaponGroup::FadeIn(UUIImage* Image, FLOAT FadeRate)
{
	if ( Image )
	{
		FLOAT CurOpacity = Image->ImageComponent->StyleCustomization.Opacity;
		FLOAT ActualRate = FadeRate * ( 1.0 -  CurOpacity );

		Image->ImageComponent->FadeType = EFT_Fading;
		Image->ImageComponent->FadeAlpha = CurOpacity;
		Image->ImageComponent->FadeTarget = 1.0;
		Image->ImageComponent->FadeTime = ActualRate;
	}

}

void UHudWidget_WeaponGroup::FadeOut(UUIImage* Image, FLOAT FadeRate)
{
	if ( Image )
	{
		FLOAT CurOpacity = Image->ImageComponent->StyleCustomization.Opacity;
		FLOAT ActualRate = FadeRate * CurOpacity;

		Image->ImageComponent->FadeType = EFT_Fading;
		Image->ImageComponent->FadeAlpha = CurOpacity;
		Image->ImageComponent->FadeTarget = 0.0;
		Image->ImageComponent->FadeTime = ActualRate;
	}

}


void UHudWidget_WeaponGroup::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
	Overlay->ImageComponent->SetOpacity(0.0f);
	PendingOverlay->ImageComponent->SetOpacity(0.0f);

	GroupLabel->SetValue( FString::Printf(TEXT("%i"),(AssociatedWeaponGroup < 10 ? AssociatedWeaponGroup : 0) ) );
}

/*=========================================================================================
  HudWidget_DeployableGroup - A child of WeaponGroup this manages deployables
  ========================================================================================= */

void UHudWidget_DeployableGroup::Tick_Widget(FLOAT DeltaTime)
{
	eventSetVisibility(FALSE);
}



/*=========================================================================================
  HudWidget_WeaponBar - This is the container class for the WeaponGroups
  ========================================================================================= */

INT UHudWidget_WeaponBar::InsertChild(class UUIObject* NewChild,INT InsertIndex,UBOOL bRenameExisting)
{
	INT InsertedIndex = Super::InsertChild(NewChild, InsertIndex, bRenameExisting);
	if ( InsertedIndex != INDEX_NONE )
	{
		UHudWidget_WeaponGroup* ChildGrp = Cast<UHudWidget_WeaponGroup>(NewChild);		
		InitializeGroup(ChildGrp);
	}
	return InsertedIndex;
}


void UHudWidget_WeaponBar::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);

	if ( GIsGame )
	{
		if (GroupWidgets.Num() <= 0)
		{
			if ( GroupTemplate )
			{
				for (INT GroupIdx=1; GroupIdx<=NumberOfGroups; GroupIdx++)
				{
					UUIPrefabInstance* PrefabInstance = GroupTemplate->InstancePrefab(this, *FString::Printf(TEXT("Container-%i"),GroupIdx));
//					UUIPrefab* Prefab = Cast<UUIPrefab> ( CreateWidget(this, UUIPrefab::StaticClass(), GroupTemplate) );
//					if ( Prefab && Prefab->Rename(*FString::Printf(TEXT("Container-%i"),GroupIdx), NULL, REN_ForceNoResetLoaders) )
					if ( PrefabInstance != NULL )
					{
						if ( InsertChild(PrefabInstance) != INDEX_NONE )
						{
							TArray<UUIObject*> Kids = PrefabInstance->GetChildren(true);
							UHudWidget_WeaponGroup* WeapGroup = NULL;
							Kids.FindItemByClass<UHudWidget_WeaponGroup>(&WeapGroup);
							if ( WeapGroup )
							{
								INT Index = GroupWidgets.Num();
								GroupWidgets.Add(1);
								GroupWidgets(Index).Group = WeapGroup;
								GroupWidgets(Index).Scaler = 1.0f;
								GroupWidgets(Index).bOversized = false;
								GroupWidgets(Index).OverSizeScaler = 0.0f;
								WeapGroup->WeaponBar = this;
								WeapGroup->AssociatedWeaponGroup = BYTE(GroupIdx);
							}
							else
							{
								GWarn->Logf(TEXT("Weapon bar [%s] Cound not cast Weapon Group"),*this->GetFullName());
							}
						}
						else
						{
							GWarn->Logf(TEXT("Weapon bar [%s] Could not insert prefab [%s]"),*this->GetFullName(),*GroupTemplate->GetFullName());
						}
					}

				}
			}
			else
			{
				GWarn->Logf(TEXT("Weapon bar [%s] lacks a Group Template"),*this->GetFullName());
			}
		}
	}
}

void UHudWidget_WeaponBar::InitializeGroup(UHudWidget_WeaponGroup* ChildGrp)
{
	if ( ChildGrp )
	{
		// Insertion sort it in to the array

		ChildGrp->WeaponBar = this;

		INT SrtIdx;
		for (SrtIdx=0 ; SrtIdx < GroupWidgets.Num(); SrtIdx++)
		{
			// Check to see if this group is already in the List

			if ( GroupWidgets(SrtIdx).Group == ChildGrp )
			{
				return;
			}

			if ( GroupWidgets(SrtIdx).Group->AssociatedWeaponGroup > ChildGrp->AssociatedWeaponGroup )
			{
				break;
			}
		}

		FWeaponGroupData GD;
		GD.Group = ChildGrp;
		GD.Scaler = 1.0f;
		GD.bOversized = false;
		GD.OverSizeScaler = 0.0f;
		GroupWidgets.InsertItem( GD, SrtIdx );
	}
}

void UHudWidget_WeaponBar::SetGroupAsActive(UHudWidget_WeaponGroup* ActiveGroup)
{
	for (INT i=0; i<GroupWidgets.Num(); i++)
	{
		if (GroupWidgets(i).Group == ActiveGroup )
		{
			ActiveGroupIndex = i;
			GroupWidgets(i).bOversized = true;
			GroupWidgets(i).OverSizeScaler = 0.0f;
			return;
		}
	}
	GWarn->Logf(TEXT("Error: Attempting to make a group active that isn't in the WeaponBar"));
}

void UHudWidget_WeaponBar::Tick_Widget(FLOAT DeltaTime)
{
	Super::Tick_Widget(DeltaTime);

	// Figure out the Height to Width ratio of the basic WeaponGroup.  FIX ME:
	// I've had to hard code it because GetBounds() doesn't like EVALPOS_PercentageOwner

	FLOAT HeightToWidthRatio = 69.0f / 49.0f; 

	// We only need to perform this logic if we have groups

	if ( GroupWidgets.Num() > 0 )
	{

		// Calculate the size of an inactive group given the current width
		// of the Bar.  GroupScaleModifier is the modifier needed to scale
		// an inactive group.  We will use the GroupWidgets Scaler parameter 
		// to scale the active group (or transitional groups).

		FLOAT OSM = 0.0;
		for (INT i=0;i<GroupWidgets.Num();i++)
		{
			OSM += GroupWidgets(i).OverSizeScaler;
		}


		FLOAT Width = ( (GroupWidgets.Num() - 1) * InactiveScale ) + 1.0f + OSM;
		FLOAT GroupScaleModifier = InactiveScale * (1.0f / Width);
		
		// FIXME: GetBounds is not very compatible with EVALPOS_PercentageOwner.  Using bounds instead.
		// We have to calculate the Width to Height ratio of the bar so later we can maintain the 
		// proper ratio of when sizing the groups

		FLOAT WW = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT WH = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];
		FLOAT WidgetRatio =  WW / WH;

		
		// XPerc is where we should begin drawing, MaxScaler is precalced here and holds the 
		// maxium additional scaling that can be applied (for smooth expanding/contracting

		FLOAT XPerc = 0.0f;		
		FLOAT ActualWidth = 0.0f;

		for ( INT GrpIdx = 0 ; GrpIdx < GroupWidgets.Num(); GrpIdx++ )
		{
			ActualWidth += GroupScaleModifier * (GroupWidgets(GrpIdx).Scaler + GroupWidgets(GrpIdx).OverSizeScaler);
		}

		XPerc = 0.5 - (ActualWidth * 0.5);

		FLOAT MaxScaler = 1.0f / InactiveScale;

		// Itterate over all Groups in the list and size/position them.  OPTIMIZE: We can probably
		// optimize this so it doesn't occur every frame.

		for ( INT GrpIdx = 0 ; GrpIdx < GroupWidgets.Num(); GrpIdx++ )
		{
			// Calculate the size and alignment
			FLOAT Modifier =  GroupScaleModifier * (GroupWidgets(GrpIdx).Scaler + GroupWidgets(GrpIdx).OverSizeScaler);
			FLOAT Left = XPerc;
			FLOAT Right = Modifier;
			
			FLOAT Height = Modifier / HeightToWidthRatio * WidgetRatio;
			
			FLOAT Top = 1.0f - Height; 
			FLOAT Bottom = Height;

			// Reposition the widget
			GroupWidgets(GrpIdx).Group->SetPosition(Left, Top, Right, Bottom, EVALPOS_PercentageOwner, false, false);
			// Setup the next position
			XPerc += Modifier;

			// Adjust the scaler.

			FLOAT NewScaler = GroupWidgets(GrpIdx).Scaler;
			FLOAT NewOpacity = GroupWidgets(GrpIdx).Group->Opacity;
			FLOAT MinOpacity = OpacityList[ INT(GroupWidgets(GrpIdx).Group->GroupStatus) ];
			
			if (ActiveGroupIndex == GrpIdx)
			{
				if ( NewScaler < (MaxScaler) )
				{
					NewScaler += (DeltaTime * TransitionRate);
				}
				
				if ( NewOpacity < ActiveGroupOpacity )
				{
					NewOpacity += (DeltaTime * TransitionRate);
				}

			}
			else
			{

				if ( NewScaler > 1.0f )
				{
					NewScaler -= (DeltaTime * TransitionRate);
				}

				if ( Abs( ActiveGroupIndex - GrpIdx ) > 1 )
				{
					if ( NewOpacity > MinOpacity )
					{
						NewOpacity -= (DeltaTime * TransitionRate);
						if (NewOpacity < MinOpacity)
						{
							NewOpacity = MinOpacity;
						}
					}
					else if ( NewOpacity < MinOpacity )
					{
						NewOpacity += (DeltaTime * TransitionRate);
						if (NewOpacity > MinOpacity)
						{
							NewOpacity = MinOpacity;
						}
					}
						
				}
				else 
				{
					NewOpacity = 0.5;
				}
			}

			FLOAT NewOverSizeScaler = GroupWidgets(GrpIdx).OverSizeScaler;
			if ( GroupWidgets(GrpIdx).bOversized )
			{
				if ( GroupWidgets(GrpIdx).Scaler >= 1.0f )
				{
					NewOverSizeScaler += (DeltaTime * TransitionRate);
					if ( NewOverSizeScaler > 0.5 )
					{
						GroupWidgets(GrpIdx).bOversized = false;
					}
				}
			}
			else
			{
				if ( NewOverSizeScaler > 0.0f )
				{
					NewOverSizeScaler -= (DeltaTime * TransitionRate);
				}
			}

			GroupWidgets(GrpIdx).OverSizeScaler = Clamp<FLOAT>(NewOverSizeScaler,0.0,0.5);
			GroupWidgets(GrpIdx).Scaler = Clamp<FLOAT>(NewScaler, 1.0 , MaxScaler);
			GroupWidgets(GrpIdx).Group->Opacity = Clamp<FLOAT>(NewOpacity, 0.15, 1.0);
		}
	}
}

void UHudWidget_WeaponBar::Render_Widget(FCanvas* Canvas)
{
	Super::Render_Widget(Canvas);

	if ( !GIsGame )
	{
		// Render the bounds 
		FLOAT X  = RenderBounds[UIFACE_Left];
		FLOAT Y  = RenderBounds[UIFACE_Top];
		FLOAT tW = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT tH = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		DrawTile(Canvas, X,Y, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
		DrawTile(Canvas, X,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));

		DrawTile(Canvas, X,Y+tH, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
		DrawTile(Canvas, X+tW,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
	}
}

/*=========================================================================================
  HudWidget_TeamBar - Used to position and manage the team widgets around a separator
  ========================================================================================= */

INT UHudWidget_TeamBar::InsertChild(class UUIObject* NewChild,INT InsertIndex,UBOOL bRenameExisting)
{
	INT InsertedIndex = Super::InsertChild(NewChild, InsertIndex, bRenameExisting);
	if ( InsertedIndex != INDEX_NONE )
	{
		UHudWidget_TeamScore* Child = Cast<UHudWidget_TeamScore>(NewChild);		
		InitializeChild(Child);
	}
	return InsertedIndex;
}


void UHudWidget_TeamBar::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);

	for (INT ChildIdx=0; ChildIdx < Children.Num(); ChildIdx++)
	{
		UHudWidget_TeamScore* Child = Cast<UHudWidget_TeamScore>(Children(ChildIdx));
		InitializeChild(Child);
	}
}

void UHudWidget_TeamBar::InitializeChild(UHudWidget_TeamScore* Child)
{
	if ( Child && Child->TeamIndex >= 0 && Child->TeamIndex < 2 )
	{
		TeamWidgets[ Child->TeamIndex ] = Child;
		TeamScale[ Child->TeamIndex ] = 0.0f;
	}
}


void UHudWidget_TeamBar::Tick_Widget(FLOAT DeltaTime)
{
	Super::Tick_Widget(DeltaTime);

	// First, position the Separator

	FLOAT SepWidth = Separator->GetPosition(UIFACE_Right, EVALPOS_PercentageOwner);
	FLOAT SepLeft = 0.5f - ( SepWidth / 2.0f );

	Separator->SetPosition(SepLeft, UIFACE_Left, EVALPOS_PercentageOwner);

	// Now, figure out the actual widgets

	AUTPlayerReplicationInfo* PRI = UTHudSceneOwner->GetPRIOwner();
	INT PlayerTeam = -1;
	if ( PRI && PRI->Team )
	{
		PlayerTeam = Clamp<INT>(PRI->Team->TeamIndex,0,1);
	}
	else
	{
		PlayerTeam = appTrunc(TestTeamIndex);
	}

	AUTPlayerController* UTPC = UTHudSceneOwner->GetUTPlayerOwner();

	for ( INT i=0; i<2; i++ )
	{
		if (TeamWidgets[i] )
		{
			FLOAT DestScale = PlayerTeam == TeamWidgets[i]->TeamIndex ? TeamScaleModifier : 1.0;

			FLOAT ScaleModifier = 0.0f;
			if ( UTPC && UTPC->PlayerReplicationInfo && UTPC->PlayerReplicationInfo->Team )
			{
				if ( UTPC->bPulseTeamColor && UTPC->PlayerReplicationInfo->Team->TeamIndex == TeamWidgets[i]->TeamIndex )
				{
					ScaleModifier = (Abs( appSin(GWorld->GetWorldInfo()->TimeSeconds * 5) ) * 0.25);
				}
			}

			if ( TeamScale[i] != DestScale || ScaleModifier > 0 )
			{
				FLOAT Rate = DeltaTime * TeamScaleTransitionRate * ( ( TeamScale[i] > DestScale ) ? -1.0 : 1.0);

				TeamScale[i] = Clamp<FLOAT>( (TeamScale[i] + Rate), 1.0, TeamScaleModifier );

				// Reposition the widget
				
				FLOAT Left  = (i<1) ? SepLeft - (WidgetDefaultWidth * (TeamScale[i] + ScaleModifier)) : SepLeft + SepWidth;
				FLOAT Right = WidgetDefaultWidth * (TeamScale[i] + ScaleModifier);
				FLOAT Top = 0;
				FLOAT Bottom = WidgetDefaultHeight * (TeamScale[i] + ScaleModifier);

				TeamWidgets[i]->SetPosition(Left, Top, Right, Bottom, EVALPOS_PercentageOwner, false, false);
			}
		}
	}
}

/*=========================================================================================
  Rocket Launcher Hud - Look at the current firing mode of the RL and 
  ========================================================================================= */

void UWeapHud_RocketLauncher::Tick(FLOAT DeltaTime)
{
	if ( GIsGame || bEditorRealTimePreview )
	{
		eventTickScene(DeltaTime * GWorld->GetWorldInfo()->TimeDilation);
	}

	Super::Tick(DeltaTime);
}

/*=========================================================================================
  The base Scoreboard Class
  ========================================================================================= */

void UUTUIScene_Scoreboard::Tick(FLOAT DeltaTime)
{
	if ( GIsGame || bEditorRealTimePreview )
	{
		eventTickScene(DeltaTime);
	}

	Super::Tick(DeltaTime);
}


/*=========================================================================================
  QuickPickCell
  ========================================================================================= */

void UHudWidget_QuickPickCell::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
	Overlay->ImageComponent->SetOpacity(0.0);
	WeaponIcon->eventSetVisibility(FALSE);
}

void UHudWidget_QuickPickCell::AssociatedWithWeapon(AUTWeapon* Weapon)
{
	if ( Weapon  )
	{
		WeaponIcon->eventSetVisibility(TRUE);
		WeaponIcon->ImageComponent->SetCoordinates(Weapon->IconCoordinates);
		eventSetVisibility(TRUE);

	}
	else
	{
		WeaponIcon->eventSetVisibility(FALSE);
		eventSetVisibility(FALSE);
	}
	MyWeapon = Weapon;

}

/*=========================================================================================
  UTUIScene_WeaponQuickPick
  ========================================================================================= */

/** 
 * Set up everything and select the current weapon
 */
void UUTUIScene_WeaponQuickPick::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
	RefreshCells();
}


void UUTUIScene_WeaponQuickPick::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GIsGame)
	{
		return;
	}

	RefreshTimer -= DeltaTime;

	if (RefreshTimer <= 0)
	{
		RefreshCells();
	}
}

void UUTUIScene_WeaponQuickPick::RefreshCells()
{

	if (GIsGame)
	{
		// Get a list of affected inventory

		for (INT i=0; i<8; i++)
		{
			if ( Cells[i] )
			{
				Cells[i]->AssociatedWithWeapon(NULL);
			}
		}
			
		AUTPawn* PawnOwner = Cast<AUTPawn>( GetPawnOwner() );
		if ( PawnOwner && PawnOwner->InvManager )
		{
			AInventory* Inv = PawnOwner->InvManager->InventoryChain;
			while ( Inv )
			{
				AUTWeapon* Weap = Cast<AUTWeapon>(Inv);
				if ( Weap && Weap->AmmoCount > 0 )
				{
					// Find the group it belongs in

					for (INT i=0;i<8;i++)
					{
						if (Cells[i] )
						{
							if ( Weap->QuickPickGroup == i && ( !Cells[i]->MyWeapon || 
										(Cells[i]->MyWeapon != Weap && Weap->QuickPickWeight > Cells[i]->MyWeapon->QuickPickWeight ) ) )
							{
								Cells[i]->AssociatedWithWeapon( Weap );
							}
						}
					}
				}
				Inv = Inv->Inventory;
			}
		}
	}

	RefreshTimer = RefreshFrequency;
}

/*=========================================================================================
  UMiniMapImage
  ========================================================================================= */

void UMiniMapImage::SetStyle(FName NewStyleTag)
{
	TArray<FStyleReferenceId> StyleReferences;
	GetStyleReferenceProperties(StyleReferences, ImageComponent);
	SetWidgetStyle( GetActiveSkin()->FindStyle( NewStyleTag ), StyleReferences(0) );
	if ( ImageComponent )
	{
		ImageComponent->RefreshAppliedStyleData();
	}
}
		
void UMiniMapImage::Render_Widget(FCanvas* Canvas)
{
	Super::Render_Widget(Canvas);
}

/*=========================================================================================
  UHudWidget_Map
  ========================================================================================= */

void UHudWidget_Map::Tick_Widget(FLOAT DeltaTime)
{
	
	if ( UTMapPanel )
	{
		const FLOAT X2 = GetPosition(UIFACE_Right, EVALPOS_PercentageOwner);
		SetPosition(1.0 - X2, UIFACE_Left, EVALPOS_PercentageOwner, TRUE);
	}

	Super::Tick_Widget(DeltaTime);
}

/*=========================================================================================
  UHudWidget_LocalMessages
  ========================================================================================= */

void UHudWidget_LocalMessages::SizeZone(INT ZoneIndex, INT FontIndex)
{
	FLOAT FontSizeX, FontSizeY;
	FVector2D ViewportSize;
	GetViewportSize(ViewportSize);

	FontPool[FontIndex]->GetCharSize(TEXT('Q'),FontSizeX, FontSizeY, FontPool[FontIndex]->GetResolutionPageIndex(ViewportSize.Y));

	const FLOAT OwnerHeight = Owner->GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport);
	MessageZones[ZoneIndex]->Position.SetPositionValue( MessageZones[ZoneIndex], (FontSizeY / OwnerHeight), UIFACE_Bottom, EVALPOS_PercentageOwner,TRUE);
}

/*=========================================================================================
  UTUIScene_MOTD
  ========================================================================================= */

void UUTUIScene_MOTD::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GIsGame && TimeRemainingOnScreen > 0.0)
	{
		TimeRemainingOnScreen -= DeltaTime;
		if (TimeRemainingOnScreen <= 0.0)
		{
			eventFinish();
		}
	}
}


void UUTUIScene_MapVote::Tick(FLOAT DeltaTime)
{
	FLOAT FullSeconds = 0.0;
	INT Secs, Mins, Hours;
	if (TimeRemaining)
	{
		AUTGameReplicationInfo* GRI = Cast<AUTGameReplicationInfo>(GWorld->GetWorldInfo()->GRI);
		if ( GRI )
		{
			FullSeconds = GRI->MapVoteTimeRemaining;
		}

		Hours = appTrunc(FullSeconds / 3600.0);
		FullSeconds -= (Hours * 3600);
		Mins = appTrunc(FullSeconds / 60.0);
		FullSeconds -= (Mins * 60);
		Secs = appTrunc(FullSeconds);

		Hours = Clamp<INT>(Hours,0,99);
		Mins = Clamp<INT>(Mins,0,59);
		Secs = Clamp<INT>(Secs,0,59);

		FString TimeStr = FString::Printf(TEXT("%02i:%02i:%02i"),Hours,Mins,Secs);
		TimeRemaining->SetValue(TimeStr);
	}

	Super::Tick(DeltaTime);

}
