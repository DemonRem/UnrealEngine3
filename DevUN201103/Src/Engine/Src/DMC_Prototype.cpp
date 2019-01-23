/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EnginePrefabClasses.h"

IMPLEMENT_CLASS(UDMC_Prototype);
IMPLEMENT_CLASS(ADMC_Base);

FString FDMCNewVar::ToCodeString(const FString VarSect)
{
	return FString::Printf(TEXT("var(%s)  %s  %s;"), *VarSect, *VarType.ToString(), *VarName.ToString());
}

//////////////////////////////////////////////////////////////////////////
// UDMC_Prototype

void UDMC_Prototype::PostLoad()
{
	Super::PostLoad();

	if(GCallbackEvent != NULL)
	{
		GCallbackEvent->Send(CALLBACK_DMCLoaded, this);
	}
}

//////////////////////////////////////////////////////////////////////////
// ADMC_Base


void ADMC_Base::RegenDMC()
{
	// Only regenerate if an instance, not a class default
	if(!IsTemplate())
	{
		ClearDMC();

		eventDMCCreate();
	}
}


void ADMC_Base::ClearDMC()
{
	for(INT i=0; i<CreatedComponents.Num(); i++)
	{
		UActorComponent* Comp = CreatedComponents(i);
		if(Comp)
		{
			DetachComponent(Comp);
		}
	}
	CreatedComponents.Empty();
}

void ADMC_Base::ProcessEvent( UFunction* Function, void* Parms, void* Result )
{
	UObject::ProcessEvent(Function, Parms, Result);
}

void ADMC_Base::PostLoad()
{
	for(INT i=0; i<CreatedComponents.Num(); i++)
	{
		UActorComponent* Comp = CreatedComponents(i);
		if(Comp)
		{
			Components.AddItem(Comp);
		}
	}

	Super::PostLoad();
}

void ADMC_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RegenDMC();
}

void ADMC_Base::PostEditMove(UBOOL bFinished)
{
	Super::PostEditMove(bFinished);

	if(bFinished)
	{
		RegenDMC();
	}
}


UActorComponent* ADMC_Base::AddComponent(UActorComponent* Template)
{
	check(Template);

	UActorComponent* NewComp = (UActorComponent*)StaticDuplicateObject(Template, Template, this, TEXT(""));

	CreatedComponents.AddItem(NewComp);

	AttachComponent(NewComp);

	return NewComp;
}
