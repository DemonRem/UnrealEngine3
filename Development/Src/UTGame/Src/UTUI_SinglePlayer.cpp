//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "EngineAnimClasses.h"


IMPLEMENT_CLASS(UUTUIScene_Campaign);
IMPLEMENT_CLASS(UUTUIScene_CMissionSelection);
IMPLEMENT_CLASS(UUTSeqObj_SPMission);
IMPLEMENT_CLASS(UUTSeqObj_SPRootMission);
IMPLEMENT_CLASS(UUTSeqAct_ServerTravel);
IMPLEMENT_CLASS(AUTMissionGRI);
IMPLEMENT_CLASS(AUTMissionPlayerController);
IMPLEMENT_CLASS(UUTSeqAct_TutorialMsg);

/*=========================================================================================
  UTSeqObj_SPMission - This is the main object that holds information about the mission
  ========================================================================================= */

/**
 * Figure out the best index for this mission and set it.
 */

void UUTSeqObj_SPMission::OnCreated()
{
	Super::OnCreated();
	FindInitialIndex();
}

void UUTSeqObj_SPMission::OnPasted()
{
	FindInitialIndex();
}

void UUTSeqObj_SPMission::FindInitialIndex()
{
	USequence* FirstSequence = GWorld->GetGameSequence(0);
	INT NewIndex = 0;
	if ( FirstSequence )
	{
		TArray<USequenceObject*> Seqs;
		FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);

		for (INT i=0; i<Seqs.Num(); i++)
		{
			UUTSeqObj_SPMission* Mission = Cast<UUTSeqObj_SPMission>(Seqs(i));
			if ( Mission && Mission != this )
			{
				if ( Mission->MissionInfo.MissionIndex >= NewIndex )
				{
					NewIndex = Mission->MissionInfo.MissionIndex + 1;
				}
			}
		}
	}
	MissionInfo.MissionIndex = NewIndex;

	ObjComment = FString::Printf(TEXT("Mission %i - %s"),MissionInfo.MissionIndex, *MissionInfo.MissionTitle);	
}

/**
 * Store off the index for later so once we know the new index we can adjust it.
 *
 * @param PropertyThatWillChange	The UProperty that is about to change
 */
void UUTSeqObj_SPMission::PreEditChange(UProperty* PropertyThatWillChange)
{
	if (PropertyThatWillChange->GetName() == TEXT("MissionIndex") )
	{
		OldIndex = MissionInfo.MissionIndex;
	}
	Super::PreEditChange(PropertyThatWillChange);
}

/**
 * Find all other Missions and adjust their indices based on our change
 */
void UUTSeqObj_SPMission::AdjustIndex()
{
	USequence* FirstSequence = GWorld->GetGameSequence(0);
	if ( FirstSequence )
	{
		TArray<USequenceObject*> Seqs;
		FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);
		
		INT NewIndex = MissionInfo.MissionIndex;

		for (INT i=0; i<Seqs.Num(); i++)
		{
			UUTSeqObj_SPMission* Mission = Cast<UUTSeqObj_SPMission>(Seqs(i));
			if ( Mission && Mission != this )
			{
				INT MI = Mission->MissionInfo.MissionIndex;
				if ( OldIndex > NewIndex )
				{
					if ( MI >= NewIndex && MI < OldIndex )
					{
						Mission->MissionInfo.MissionIndex++;
					}
				}
				else
				{
					if ( MI <= NewIndex && MI > OldIndex )
					{
						Mission->MissionInfo.MissionIndex--;
					}
				}
			}
		}
	}
}

/**
 * Walk all missions and turn off their bFirstMission flags
 */
void UUTSeqObj_SPMission::SetAsFirstMission()
{
	if ( bFirstMission )
	{
		// We are the new first mission, clear everyone else and set our index to 0

		USequence* FirstSequence = GWorld->GetGameSequence(0);
		if ( FirstSequence )
		{
			TArray<USequenceObject*> Seqs;
			FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);
			
			INT NewIndex = MissionInfo.MissionIndex;

			for (INT i=0; i<Seqs.Num(); i++)
			{
				UUTSeqObj_SPMission* Mission = Cast<UUTSeqObj_SPMission>(Seqs(i));
				if ( Mission && Mission != this )
				{
					Mission->bFirstMission = false;
				}
			}
		}

		MissionInfo.MissionIndex = 0;
		AdjustIndex();
	}
	else
	{
		// If we have a mission index of 0, force bFirstMission to be true
		if ( MissionInfo.MissionIndex == 0 )
		{
			bFirstMission = true;
		}
		else
		{

			// Otherwise, find the sequence with index = 0
			USequence* FirstSequence = GWorld->GetGameSequence(0);
			if ( FirstSequence )
			{
				TArray<USequenceObject*> Seqs;
				FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);
				
				INT NewIndex = MissionInfo.MissionIndex;

				for (INT i=0; i<Seqs.Num(); i++)
				{
					UUTSeqObj_SPMission* Mission = Cast<UUTSeqObj_SPMission>(Seqs(i));
					if ( Mission && Mission != this )
					{
						if ( Mission->MissionInfo.MissionIndex == 0 )
						{
							Mission->bFirstMission = true;
							break;
						}
					}
				}
			}
		}
	}
}


/**
 * Manage data as it changes.
 *
 * @param PropertyThatChanged	The UProperty that changed.
 */
void UUTSeqObj_SPMission::PostEditChange( class FEditPropertyChain& PropertyThatChanged )
{

	UProperty* Prop = PropertyThatChanged.GetActiveNode()->GetValue();
	UProperty* ParentProp = PropertyThatChanged.GetActiveMemberNode()->GetValue();


	if ( Prop->GetName() == TEXT("MissionIndex") )	// Adjust all of the indicies
	{
		// Only the root can be 0

		if (MissionInfo.MissionIndex == 99999)
		{
			
			USequence* FirstSequence = GWorld->GetGameSequence(0);
			if ( FirstSequence )
			{
				TArray<USequenceObject*> Seqs;
				FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);

				INT SeqIndex = 1;
				for (INT i=0;i<Seqs.Num();i++)
				{
					UUTSeqObj_SPMission* Mission = Cast<UUTSeqObj_SPMission>(Seqs(i));
					if ( Mission )
					{
						UUTSeqObj_SPRootMission* ROOT = Cast<UUTSeqObj_SPRootMission>(Mission);
						if ( ROOT )
						{
							Mission->MissionInfo.MissionIndex = 0;
						}
						else
						{
							Mission->MissionInfo.MissionIndex = SeqIndex;
							SeqIndex++;
						}

					}
				}
			}
			return;
		}

		if (MissionInfo.MissionIndex < 1)
		{
			MissionInfo.MissionIndex = 1;
		}

		if ( MissionInfo.MissionIndex != OldIndex )
		{
			AdjustIndex();
		}
	}
	else if (Prop->GetName() == TEXT("bFirstMission") )	
	{
		// Only the root can be flagged as such
		bFirstMission = false;
	}

	// If something in the MissionInfo changed, update the various fields.
	else if ( ParentProp->GetName() == TEXT("MissionInfo") )
	{
		// Remove mission links that are no longer needed.
		if ( OutputLinks.Num() > MissionInfo.MissionProgression.Num() )
		{
			OutputLinks.Remove(MissionInfo.MissionProgression.Num(), OutputLinks.Num() - MissionInfo.MissionProgression.Num());
		}

		if ( OutputLinks.Num() < MissionInfo.MissionProgression.Num() )
		{
			for (INT i=OutputLinks.Num(); i<MissionInfo.MissionProgression.Num(); i++)
			{
				OutputLinks.AddZeroed();
			}
		}

		for (INT i=0;i<MissionInfo.MissionProgression.Num(); i++)
		{
			switch( MissionInfo.MissionProgression(i).MissionResult)
			{
				case EMResult_Any : OutputLinks(i).LinkDesc = FString::Printf(TEXT("Any")); break;
				case EMResult_Won : OutputLinks(i).LinkDesc = FString::Printf(TEXT("Win")); break;
				case EMResult_Lost: OutputLinks(i).LinkDesc = FString::Printf(TEXT("Loss")); break;
			}
		}
	}
}

/** 
 * Custom Draw Code
 */
void UUTSeqObj_SPMission::DrawSeqObj(FCanvas* Canvas, UBOOL bSelected, UBOOL bMouseOver, INT MouseOverConnType, INT MouseOverConnIndex, FLOAT MouseOverTime)
{
	Super::DrawSeqObj(Canvas, bSelected, bMouseOver, MouseOverConnType, MouseOverConnIndex, MouseOverTime);

	if (bMouseOver)
	{
		FLOAT X = ObjPosX+1;
		FLOAT Y = ObjPosY + DrawHeight+3;
		FLOAT XL = 318;
		FLOAT YL = 0.0;

		FString MissionDescription = GetDescription();

		WrappedPrint( Canvas, false, X, Y, XL, YL, GEngine->SmallFont, *MissionDescription, FLinearColor::White);
		DrawTile(Canvas, X-1, Y-1, XL+2, YL+2, 0.0f,0.0f,0.0f,0.0f, FColor(255,255,64) );
		DrawTile(Canvas, X, Y, XL, YL, 0.0f,0.0f,0.0f,0.0f, FColor(64,64,64) );
		WrappedPrint( Canvas, true, X, Y, XL, YL, GEngine->SmallFont, *MissionDescription, FLinearColor::White);
	}

}

FString UUTSeqObj_SPMission::GetDescription()
{
	FString MissionTitle;
	FString MissionDescription;
	UUIDataProvider* FieldProvider = NULL;

	FString FieldName;
	FUIProviderFieldValue FieldValue(EC_EventParm);

	MissionTitle = MissionInfo.MissionTitle;

	// resolve the markup string to get the name of the data field and the provider that contains that data
	if ( MissionInfo.MissionTitle != TEXT("") && UUIRoot::ResolveDataStoreMarkup(MissionInfo.MissionTitle, NULL, NULL, FieldProvider, FieldName ) )
	{
		if ( FieldProvider->GetDataStoreValue(FieldName, FieldValue) )
		{
			MissionTitle = FieldValue.StringValue;
		}
	}

	MissionDescription = MissionInfo.MissionDescription;
	// resolve the markup string to get the name of the data field and the provider that contains that data
	if ( MissionInfo.MissionDescription != TEXT("") && UUIRoot::ResolveDataStoreMarkup(MissionInfo.MissionDescription, NULL, NULL, FieldProvider, FieldName ) )
	{
		if ( !FieldProvider->GetDataStoreValue(FieldName, FieldValue) )
		{
			MissionDescription = FieldValue.StringValue;
		}
	}

	return FString::Printf(TEXT("-[Title]-\n\n%s\n\n-[Desc]-\n\n%s"),*MissionTitle?*MissionTitle:TEXT("None"),*MissionDescription?*MissionDescription:TEXT("None"));
}

/* This is Wrapped String */
void UUTSeqObj_SPMission::WrappedPrint( FCanvas* Canvas, UBOOL Draw, FLOAT CurX, FLOAT CurY, FLOAT& XL, FLOAT& YL, UFont* Font, const TCHAR* Text, FLinearColor DrawColor )
{
	if (!Font)
	{
		return;
	}

	FLOAT Width = XL;
	FLOAT ClipX = CurX + XL;
	do
	{
		INT iCleanWordEnd=0, iTestWord;
		FLOAT TestXL = 0, CleanXL=0;
		FLOAT TestYL = 0, CleanYL=0;
		UBOOL GotWord = FALSE;
		for( iTestWord = 0 ; Text[iTestWord] != 0 && Text[iTestWord] != '\n'; )
		{
			FLOAT ChW, ChH;
			Font->GetCharSize(Text[iTestWord], ChW, ChH);
			TestXL += ChW ; 
			TestYL = Max(TestYL, ChH );

			// If we are past the width, break here

			if( TestXL > Width )
			{
				break;
			}
			
			iTestWord++;

			UBOOL WordBreak = Text[iTestWord]==' ' || Text[iTestWord]=='\n' || Text[iTestWord]==0;

			if ( WordBreak || !GotWord )
			{
				iCleanWordEnd = iTestWord;
				CleanXL       = TestXL;
				CleanYL       = TestYL;
				GotWord       = GotWord || WordBreak;
			}

			if ( Text[iTestWord] =='\n' && Text[iTestWord+1] == '\n' )
			{
				CleanYL *= 2;
			}
		}

		if( iCleanWordEnd == 0 )
		{
			break;
		}

		// Sucessfully split this line, now draw it.
		if ( Draw && iCleanWordEnd>0 )
		{
			FString TextLine(Text);
			FLOAT LineX = CurX;
			LineX += DrawString(Canvas,LineX, CurY,*(TextLine.Left(iCleanWordEnd)),Font,DrawColor);
		}

		// Update position.
		CurY += CleanYL;
		YL   += CleanYL;
		XL   = Max( XL,CleanXL);
		Text += iCleanWordEnd;

		// Skip whitespace after word wrap.
		while( *Text==' ' || *Text=='\n' )
		{
			Text++;
		}
	}
	while( *Text );

}

/*=========================================================================================
	AUTMissionGRI - The GRI for the mission game type
  ========================================================================================= */

void AUTMissionGRI::FillMissionList()
{
	USequence* FirstSequence = GWorld->GetGameSequence(0);
	if ( FirstSequence )
	{
		FullMissionList.Empty();
		TArray<USequenceObject*> Seqs;
		FirstSequence->FindSeqObjectsByClass(UUTSeqObj_SPMission::StaticClass(), Seqs, true);
		
		for (INT i=0; i<Seqs.Num(); i++)
		{
			UUTSeqObj_SPMission* Mission= Cast<UUTSeqObj_SPMission>(Seqs(i));
			if ( Mission )
			{
				FullMissionList.AddItem(Mission);
			}
		}
	}
}

/*=========================================================================================
	UTSeqObj_SPRootMission - This is the first mission
  ========================================================================================= */

void UUTSeqObj_SPRootMission::OnCreated()
{
	Super::OnCreated();
	MissionInfo.MissionIndex = 0;
	ObjComment = FString::Printf(TEXT("From Menus"));	

}

/**
 * Manage data as it changes.
 *
 * @param PropertyThatChanged	The UProperty that changed.
 */
void UUTSeqObj_SPRootMission::PostEditChange( class FEditPropertyChain& PropertyThatChanged )
{

	UProperty* Prop = PropertyThatChanged.GetActiveNode()->GetValue();

	if ( Prop->GetName() == TEXT("MissionIndex") )	// Adjust all of the indicies
	{
		MissionInfo.MissionIndex = 0;
	}
	else if (Prop->GetName() == TEXT("bFirstMission") )	// Set this mission as the first
	{
		bFirstMission = true;
	}

	Super::PostEditChange(PropertyThatChanged);

}

/*=========================================================================================
	UTSeqAct_ServerTravel - Allow Kismet to perform a server travel
  ========================================================================================= */

/**
 * Performs a server travel to a new map.
 */
void UUTSeqAct_ServerTravel::OnReceivedImpulse( class USequenceOp* ActivatorOp, INT InputLinkIndex )
{

	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();

	if (WorldInfo && WorldInfo->NetMode < NM_Client && WorldInfo->Game)
	{
		AUTGame* GI = Cast<AUTGame>(WorldInfo->Game);
		if ( GI )
		{
			// Make sure to pass along the Single player Index if there is any
			FString FinalURL = FString::Printf(TEXT("%s?SPIndex=%i?Listen"),*TravelURL,GI->SinglePlayerMissionIndex);

			if ( GI->SinglePlayerMissionIndex >=0 )
			{
				WorldInfo->eventServerTravel(FinalURL);
				return;
			}
		}
		WorldInfo->eventServerTravel(TravelURL);
	}
}

/*=========================================================================================
	UTMissionPlayerController - The PC
  ========================================================================================= */


UBOOL AUTMissionPlayerController::Tick( FLOAT DeltaTime, enum ELevelTick TickType )
{

	if (CameraTransitions[1].DestGlobe)
	{
		FVector StartLoc = CameraTransitions[0].DestGlobe->SkeletalMeshComponent->GetBoneLocation(CameraTransitions[0].BoneName);
		FVector StartTan = (StartLoc - CameraTransitions[0].DestGlobe->Location).SafeNormal() * 32;
		StartLoc += StartTan.SafeNormal() * CameraTransitions[0].MapDist;
		FVector StartLook = StartTan * - 1;

		FVector DestLoc = CameraTransitions[1].DestGlobe->SkeletalMeshComponent->GetBoneLocation(CameraTransitions[1].BoneName);
		FVector DestTan = (DestLoc - CameraTransitions[1].DestGlobe->Location).SafeNormal() * 32;
		DestLoc += DestTan.SafeNormal() * CameraTransitions[1].MapDist;
		FVector DestLook = DestTan * - 1;

		FLOAT Position = Clamp<FLOAT>( (1.0 - (CameraTransitionTime / CameraTransitionDuration)), 0.f, 1.f);

		CameraLocation = CubicInterp(StartLoc, StartTan, DestLoc, DestTan, Position);
		
		FVector VCameraLookDir;
		VCameraLookDir = CubicInterp(StartLook, FVector(0,0,0), DestLook, FVector(0,0,0),Position);
		CameraLook =  VCameraLookDir.Rotation();
		if ( CameraTransitionTime > 0.f )
		{
			CameraTransitionTime -= DeltaTime;
		}
	}
	return Super::Tick(DeltaTime, TickType);
}

/*=========================================================================================
	UTSeqAct_ServerTravel - Allow Kismet to perform a server travel
  ========================================================================================= */

UBOOL UUTSeqAct_TutorialMsg::UpdateOp(FLOAT deltaTime)
{
	return bFinished;
}


/*=========================================================================================
	UTUIScene_CMissionSelection
  ========================================================================================= */

void UUTUIScene_CMissionSelection::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TeaserIndex >= 0)
	{
		TeaserDuration -= DeltaTime;
		if ( TeaserDuration <= 0.0f && AudioPlayer->bFinished )
		{
			eventPlayNextTeaser();
		}
	}

	// Update the Player Lists

	INT LblIndex=0;

	AWorldInfo* WI = GWorld->GetWorldInfo();
	if (WI && WI->GRI)
	{
		for ( INT i=0; i<WI->GRI->PRIArray.Num() && LblIndex <4 ;i++)
		{
			APlayerReplicationInfo* PRI = WI->GRI->PRIArray(i);
			if (PRI && !PRI->bOnlySpectator && !PRI->bBot)
			{
				PlayerLabels[LblIndex]->SetValue(PRI->PlayerName);
				if ( !PlayerLabels[LblIndex]->IsVisible() )
				{
					PlayerLabels[LblIndex]->eventSetVisibility(true);
				}
				LblIndex++;
			}
		}
	}

	if (GIsGame)
	{
		for (INT i=LblIndex; i<4 ;i++)
		{
			if ( PlayerLabels[LblIndex]->IsVisible() )
			{
				PlayerLabels[LblIndex]->eventSetVisibility(false);
			}
		}
	}

}
