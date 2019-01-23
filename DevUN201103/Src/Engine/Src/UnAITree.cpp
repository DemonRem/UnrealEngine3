/*=============================================================================
	UnAITree.cpp: Behavior tree implementation
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAIClasses.h"

IMPLEMENT_CLASS(UAITree);
IMPLEMENT_CLASS(UAICommandNodeBase);
IMPLEMENT_CLASS(UAICommandNodeRoot);
IMPLEMENT_CLASS(UAIGatherNodeBase);
IMPLEMENT_CLASS(UAITree_DMC_Base);


//////////////////////////////////////////////////////////////////////////
// UAITree_DMC_Base
void UAITree_DMC_Base::RegenDMC()
{
	// Only regenerate if an instance, not a class default
	if(!IsTemplate())
	{
		ClearDMC();
	}
}


void UAITree_DMC_Base::ClearDMC()
{
}

void UAITree_DMC_Base::ProcessEvent( UFunction* Function, void* Parms, void* Result )
{
	UObject::ProcessEvent(Function, Parms, Result);
}

void UAITree_DMC_Base::PostLoad()
{
	Super::PostLoad();
}

void UAITree_DMC_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RegenDMC();
}

//////////////////////////////// FAITreeHandle ////////////////////////////////
UBOOL FAITreeHandle::IsNodeDisabled( UAICommandNodeBase* Node )
{
	for( INT i = 0; i < DisabledNodes.Num(); i++ )
	{
		if( DisabledNodes(i) == Node )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//////////////////////////////// UAICommandNodeBase ////////////////////////////////
#if WITH_EDITOR
FString UAICommandNodeBase::GetDisplayName()
{
	if(CommandClass != NULL)
	{
		return CommandClass->GetName();
	}
	else
	{
		return TEXT("NONE");
	}
}

void UAICommandNodeBase::CreateAutoConnectors()
{
	CreateConnector(K2CD_Input, K2CT_Exec, TEXT("Input"));

	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Output"));
}
#endif // WITH_EDITOR

UAICommandNodeBase* UAICommandNodeBase::SelectBestChild( AAIController* InAI, FAITreeHandle& Handle )
{
	FLOAT				BestChildRating = 0.f;
	UAICommandNodeBase* BestChildNode	= NULL;

	for( INT OutputIdx = 0; OutputIdx < Outputs.Num(); OutputIdx++ )
	{
		UK2Output* Output = Outputs(OutputIdx);
		if( Output != NULL )
		{
			for( INT InputIdx = 0; InputIdx < Output->ToInputs.Num(); InputIdx++ )
			{
				UK2Input* Input = Output->ToInputs(InputIdx);
				if( Input != NULL )
				{
					UAICommandNodeBase* ChildNode = Cast<UAICommandNodeBase>(Input->OwningNode);
					if( ChildNode != NULL && ChildNode->CommandClass != NULL )
					{
						FLOAT ChildRating = -1.f;
						if( !Handle.IsNodeDisabled( ChildNode ) )
						{
							if( GIsGame )
							{
								UAICommandBase* DefaultCmd = Cast<UAICommandBase>(ChildNode->CommandClass->GetDefaultObject());
								ChildRating = DefaultCmd->eventGetUtility( InAI );
							}

							if( BestChildNode == NULL || ChildRating > BestChildRating )
							{
								BestChildRating = ChildRating;
								BestChildNode	= ChildNode;
							}
						}
#if !FINAL_RELEASE
						FAITreeUtilityInfo Info;
						Info.CommandClass  = ChildNode->CommandClass;
						Info.UtilityRating = ChildRating;
						Handle.LastUtilityRatingList.AddItem( Info );
#endif
					}
				}
			}
		}
	}

	return BestChildNode;
}

//////////////////////////////// UAICommandNodeRoot ////////////////////////////////
#if WITH_EDITOR
FString UAICommandNodeRoot::GetDisplayName()
{
	return RootName.ToString();
}


void UAICommandNodeRoot::CreateAutoConnectors()
{
	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Output"));
}
#endif // WITH_EDITOR


//////////////////////////////// UAICommandNodeRoot ////////////////////////////////
#if WITH_EDITOR
FString UAIGatherNodeBase::GetDisplayName()
{
	return TEXT("GATHER NODE");
}

void UAIGatherNodeBase::CreateAutoConnectors()
{
}
#endif // WITH_EDITOR


//////////////////////////////// UAITree ////////////////////////////////
void UAITree::PostLoad()
{
	Super::PostLoad();

	FillRootList();
}

void UAITree::FillRootList()
{
	RootList.Empty();
	for( INT NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++ )
	{
		UAICommandNodeRoot* ChkRoot = Cast<UAICommandNodeRoot>(Nodes(NodeIdx));
		if( ChkRoot != NULL )
		{
			RootList.AddUniqueItem( ChkRoot );
		}
	}
}

UBOOL UAITree::SetActiveRoot( FName InName, FAITreeHandle& Handle )
{
	if( RootList.Num() == 0 )
	{
		FillRootList();
	}

	for( INT RootIdx = 0; RootIdx < RootList.Num(); RootIdx++ )
	{
		UAICommandNodeRoot* ChkRoot = RootList(RootIdx);
		if( ChkRoot != NULL )
		{
			if( InName == NAME_None || InName == ChkRoot->RootName )
			{
				Handle.ActiveRootName = ChkRoot->RootName;
				Handle.ActiveRoot = ChkRoot;
				return TRUE;
			}
		}
	}

	//debug
	debugf(TEXT("UAITree::SetActiveRoot - Failed to find root named %s"), *InName.ToString());

	return FALSE;
}

TArray<UClass*> UAITree::EvaluateTree( AAIController* InAI, FAITreeHandle& Handle )
{
	TArray<UClass*> Results;

	if( Handle.ActiveRoot == NULL )
	{
		SetActiveRoot( NAME_None, Handle );
	}

	if( GatherList != NULL && GIsGame )
	{
		AWorldInfo* WorldInfo = InAI->WorldInfo;
		for( INT GatherIdx = 0; GatherIdx < GatherList->Nodes.Num(); GatherIdx++ )
		{
			UAIGatherNodeBase* Node = Cast<UAIGatherNodeBase>(GatherList->Nodes(GatherIdx));
			if( Node != NULL && WorldInfo->TimeSeconds > Node->LastUpdateTime )
			{
				Node->UpdateNodeValue();
			}			
		}		
	}

#if !FINAL_RELEASE
	Handle.LastUtilityRatingList.Empty();
#endif
	if( Handle.ActiveRoot != NULL )
	{
		UAICommandNodeBase* BestChild = Handle.ActiveRoot;
		while( BestChild != NULL )
		{
			if( BestChild->CommandClass != NULL )
			{
				Results.AddItem( BestChild->CommandClass );
			}
			BestChild = BestChild->SelectBestChild( InAI, Handle );
		}
	}

	return Results;
}


