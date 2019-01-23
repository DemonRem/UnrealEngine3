/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"

IMPLEMENT_CLASS(UDecalManager);
IMPLEMENT_CLASS(UDecalLifetime);
IMPLEMENT_CLASS(UDecalLifetimeAge);
IMPLEMENT_CLASS(UDecalLifetimeData);
IMPLEMENT_CLASS(UDecalLifetimeDataAge);

/**
 * Initializes the LifetimePolicies array with instances of all classes that derive from UDecalLifetime.
 */
void UDecalManager::Init()
{
	LifetimePolicies.Empty();

	for ( TObjectIterator<UClass> It ; It ; ++It )
	{
		UClass* TheClass = *It;
		if ( TheClass->IsChildOf( UDecalLifetime::StaticClass() ) && !(TheClass->ClassFlags & CLASS_Abstract) )
		{
			LifetimePolicies.AddItem( (UDecalLifetime*)StaticConstructObject( TheClass, GetOuter() ) );
		}
	}
}

/**
 * Updates the manager's lifetime policies array with any policies that were added since the manager was first created.
 */
void UDecalManager::UpdatePolicies()
{
	for ( TObjectIterator<UClass> It ; It ; ++It )
	{
		UClass* TheClass = *It;
		if ( TheClass->IsChildOf( UDecalLifetime::StaticClass() ) && !(TheClass->ClassFlags & CLASS_Abstract) )
		{
			// Determine if this class is already present.
			UBOOL bPolicyExists = FALSE;
			for( INT LifetimePolicyIndex = 0 ; LifetimePolicyIndex < LifetimePolicies.Num() ; ++LifetimePolicyIndex )
			{
				const UDecalLifetime* LifetimePolicy = LifetimePolicies( LifetimePolicyIndex );
				if( LifetimePolicy->GetClass() == TheClass )
				{
					bPolicyExists = TRUE;
					break;
				}
			}
			// If not present, add it.
			if ( !bPolicyExists )
			{
				LifetimePolicies.AddItem( (UDecalLifetime*)StaticConstructObject( TheClass, GetOuter() ) );
			}
		}
	}
}

/**
 * Returns the named lifetime policy, or NULL if no lifetime policy with that name exists in LifetimePolicies.
 */
UDecalLifetime* UDecalManager::GetPolicy(const FString& PolicyName) const
{
	for ( INT LifetimePolicyIndex = 0 ; LifetimePolicyIndex < LifetimePolicies.Num() ; ++LifetimePolicyIndex )
	{
		UDecalLifetime* LifetimePolicy = LifetimePolicies( LifetimePolicyIndex );
		if ( LifetimePolicy->PolicyName == PolicyName )
		{
			return LifetimePolicy;
		}
	}
	return NULL;
}

/**
 * Connects the specified decal component to the lifetime policy specified by the decal's LifetimePolicyName property.
 */
void UDecalManager::Connect(UDecalComponent* InDecalComponent)
{
	check( InDecalComponent );
	check( !InDecalComponent->bStaticDecal );

	if ( InDecalComponent->LifetimeData )
	{
		UDecalLifetime* LifetimePolicy = GetPolicy( InDecalComponent->LifetimeData->LifetimePolicyName );
		if ( LifetimePolicy )
		{
			LifetimePolicy->AddDecal( InDecalComponent );
		}
	}
}

/**
 * Disconnects the specified decal component from the lifetime policy specified by the decal's LifetimePolicyName property.
 */
void UDecalManager::Disconnect(UDecalComponent* InDecalComponent)
{
	check( InDecalComponent );
	check( !InDecalComponent->bStaticDecal );

	if ( InDecalComponent->LifetimeData )
	{
		UDecalLifetime* LifetimePolicy = GetPolicy( InDecalComponent->LifetimeData->LifetimePolicyName );
		if ( LifetimePolicy )
		{
			LifetimePolicy->RemoveDecal( InDecalComponent );
		}
	}
}

/**
 * Ticks all policies in the LifetimePolicies array.
 */
void UDecalManager::Tick(FLOAT DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_DecalTime);
	for ( INT LifetimePolicyIndex = 0 ; LifetimePolicyIndex < LifetimePolicies.Num() ; ++LifetimePolicyIndex )
	{
		UDecalLifetime* LifetimePolicy = LifetimePolicies( LifetimePolicyIndex );
		LifetimePolicy->Tick( DeltaSeconds );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UDecalLifetime
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void UDecalLifetime::Tick(FLOAT DeltaSeconds)
{
	appErrorf( TEXT("UDecalLifetime::Tick not implemented by derived class") );
}

/**
 * Helper function for derived classes that encapsulates decal detachment, owner components array clearing, etc.
 */
void UDecalLifetime::EliminateDecal(UDecalComponent* InDecalComponent)
{
	// Cache the decal's owner, as ConditionalDetach will clear the component's owner pointer.
	AActor* Owner = InDecalComponent->GetOwner();

	// Detach the component.
	InDecalComponent->ConditionalDetach();

	// Remove the decal from its owner's components array.
	if ( Owner )
	{
		UBOOL bFoundComponent = FALSE;
		TArrayNoInit<UActorComponent*>& Components = Owner->Components;
		for( INT ComponentIndex = 0 ; ComponentIndex < Components.Num() ; ++ComponentIndex )
		{
			UActorComponent* Component = Components(ComponentIndex); 
			if( Component == InDecalComponent )
			{
				Components.Remove( ComponentIndex );
				bFoundComponent = TRUE;
				break;
			}
		}

		// Ensure the decal component was found.
		check( bFoundComponent );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UDecalLifetimeAge
//
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Called by UDecalManager::Tick.
 */
void UDecalLifetimeAge::Tick(FLOAT DeltaSeconds)
{
	// Accumulate a list of decals to kill.
	TArray<UDecalComponent*> KilledDecals;

	for ( INT DecalIndex = ManagedDecals.Num() - 1 ; DecalIndex >= 0 ; --DecalIndex )
	{
		UDecalComponent* Decal = ManagedDecals( DecalIndex );
		if( Decal )
		{
			// All decals managed by the decal manager should have a non-NULL LifetimeData object.
			check( Decal->LifetimeData );

			UDecalLifetimeDataAge*	LifetimeData = Cast<UDecalLifetimeDataAge>( Decal->LifetimeData );

			// Advance the decal in age.
			LifetimeData->Age += DeltaSeconds;

			// Kill off the decal if it older than its lifespan.
			const UBOOL bDecalAlive = LifetimeData->LifeSpan > LifetimeData->Age;
			if ( !bDecalAlive )
			{
				ManagedDecals.Remove( DecalIndex );
				KilledDecals.AddItem( Decal );
			}
		}
		else
		{
			ManagedDecals.Remove( DecalIndex );
		}
	}

	// Eliminate decals marked for killing.
	for ( INT DecalIndex = 0 ; DecalIndex < KilledDecals.Num() ; ++DecalIndex )
	{
		EliminateDecal( KilledDecals( DecalIndex ) );
	}
}

