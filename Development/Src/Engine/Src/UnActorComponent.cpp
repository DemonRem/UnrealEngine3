/*=============================================================================
	UnActorComponent.cpp: Actor component implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UActorComponent);
IMPLEMENT_CLASS(UAudioComponent);

IMPLEMENT_CLASS(UWindDirectionalSourceComponent);

IMPLEMENT_CLASS(UPrimitiveComponentFactory);
IMPLEMENT_CLASS(UMeshComponentFactory);
IMPLEMENT_CLASS(UStaticMeshComponentFactory);


/**
 * Constructor, removing primitive from scene and caching it if we need to readd .
 */
FPrimitiveSceneAttachmentContext::FPrimitiveSceneAttachmentContext(UPrimitiveComponent* InPrimitive )
:	Scene(NULL)
{
	check(InPrimitive);
	checkf(!InPrimitive->HasAnyFlags(RF_Unreachable), TEXT("%s"), *InPrimitive->GetFullName());
	if( (InPrimitive->IsAttached() || !InPrimitive->IsValidComponent()) && InPrimitive->GetScene() )
	{
		Primitive	= InPrimitive;
		Scene		= Primitive->GetScene();
		Scene->RemovePrimitive( Primitive );
	}
	else
	{
		Primitive = NULL;
	}
}

/**
 * Destructor, adding primitive to scene again if needed. 
 */
FPrimitiveSceneAttachmentContext::~FPrimitiveSceneAttachmentContext()
{
	if( Primitive && Primitive->IsValidComponent() )
	{
		const UBOOL bShowInEditor				= !(Primitive->HiddenEditor);
		const UBOOL bShowInGame					= !(Primitive->HiddenGame);
		const UBOOL bDetailModeAllowsRendering	= Primitive->DetailMode <= GSystemSettings.DetailMode;
		if ( bDetailModeAllowsRendering && ((GIsGame && bShowInGame) || (!GIsGame && bShowInEditor)) )
		{
			Scene->AddPrimitive( Primitive );
		}
	}
}


void AActor::ClearComponents()
{
	for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
	{
		UActorComponent* Component = Components(ComponentIndex); 
		if( Component )
		{
			Component->ConditionalDetach();
		}
	}
}

/**
 * Works through the component arrays marking entries as pending kill so references to them
 * will be NULL'ed.
 */
void AActor::MarkComponentsAsPendingKill(void)
{
	//@todo garbage collection: discuss repercussions of marking attached vs owned components as pending kill
	for (INT Index = 0; Index < Components.Num(); Index++)
	{
		UActorComponent* Component = Components(Index);
		if( Component != NULL )
		{
			if( GIsEditor )
			{
				Component->Modify();
			}
			Component->MarkPendingKill();
		}
	}
	for (INT Index = 0; Index < AllComponents.Num(); Index++)
	{
		UActorComponent* Component = AllComponents(Index);
		if( Component != NULL )
		{
			if( GIsEditor )
			{
				Component->Modify();
			}
			Component->MarkPendingKill();
		}
	}
}

/**
 * Flags all components as dirty so that they will be guaranteed an update from
 * AActor::Tick(), and also be conditionally reattached by AActor::ConditionalUpdateComponents().
 * @param	bTransformOnly	- True if only the transform has changed.
 */
void AActor::MarkComponentsAsDirty(UBOOL bTransformOnly)
{
	// Make a copy of the AllComponents array, since BeginDeferredReattach may change the order by reattaching the component.
	TArray<UActorComponent*> LocalAllComponents = AllComponents;

	for (INT Idx = 0; Idx < LocalAllComponents.Num(); Idx++)
	{
		if (LocalAllComponents(Idx) != NULL)
		{
			if (bStatic)
			{
				LocalAllComponents(Idx)->ConditionalDetach();
			}
			else
			{
				if(bTransformOnly)
				{
					LocalAllComponents(Idx)->BeginDeferredUpdateTransform();
				}
				else
				{
					LocalAllComponents(Idx)->BeginDeferredReattach();
				}
			}
		}
	}

	if (bStatic  && !IsPendingKill())
	{
		ConditionalUpdateComponents(FALSE);
	}
}

/**
 * Verifies that neither this actor nor any of its components are RF_Unreachable and therefore pending
 * deletion via the GC.
 *
 * @return TRUE if no unreachable actors are referenced, FALSE otherwise
 */
UBOOL AActor::VerifyNoUnreachableReferences()
{
	UBOOL bHasUnreachableReferences = FALSE;

	// Check object itself.
	if( HasAnyFlags(RF_Unreachable) )
	{
		bHasUnreachableReferences = TRUE;
	}
	// Check components in Components array.
	for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
	{
		UActorComponent* Component = Components(ComponentIndex); 
		if( Component && Component->HasAnyFlags(RF_Unreachable) )
		{
			bHasUnreachableReferences = TRUE;
		}
	}
	// Check components in AllComponents array.
	for(INT ComponentIndex = 0;ComponentIndex < AllComponents.Num();ComponentIndex++)
	{
		UActorComponent* Component = AllComponents(ComponentIndex); 
		if( Component && Component->HasAnyFlags(RF_Unreachable) )
		{
			bHasUnreachableReferences = TRUE;
		}
	}

	// Detailed logging of culprit.
	if( bHasUnreachableReferences )
	{
		debugf(TEXT("Actor '%s' has references to unreachable objects."), *GetFullName());
		debugf(TEXT("%s  Actor             Flags: 0x%016I64X  %s"), HasAnyFlags(RF_Unreachable) ? TEXT("X") : TEXT(" "), GetFlags(), *GetFullName());

		// Components array.
		for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
		{
			UActorComponent* Component = Components(ComponentIndex); 
			if( Component )
			{
				debugf(TEXT("%s  Components    %2i  Flags: 0x%016I64X  %s"), 
					Component->HasAnyFlags(RF_Unreachable) ? TEXT("X") : TEXT(" "), 
					ComponentIndex,
					Component->GetFlags(), 
					*Component->GetFullName());
			}
			else
			{
				debugf(TEXT("%s  Components    %2i"), TEXT(" "), ComponentIndex);
			}
		}
		// AllComponents array.
		for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
		{
			UActorComponent* Component = Components(ComponentIndex); 
			if( Component )
			{
				debugf(TEXT("%s  AllComponents %2i  Flags: 0x%016I64X  %s"), 
					Component->HasAnyFlags(RF_Unreachable) ? TEXT("X") : TEXT(" "), 
					ComponentIndex,
					Component->GetFlags(), 
					*Component->GetFullName());
			}
			else
			{
				debugf(TEXT("%s  AllComponents %2i"), TEXT(" "), ComponentIndex);
			}
		}
	}

	return !bHasUnreachableReferences;
}

void AActor::ConditionalUpdateComponents(UBOOL bCollisionUpdate)
{
#if DO_GUARD_SLOW
	// Verify that actor has no references to unreachable components.
	VerifyNoUnreachableReferences();
#endif

	SCOPE_CYCLE_COUNTER(STAT_UpdateComponentsTime);

	// Don't update components on destroyed actors and default objects/ archetypes.
	if( !ActorIsPendingKill()
	&&	!HasAnyFlags(RF_ArchetypeObject|RF_ClassDefaultObject) )
	{
		UpdateComponentsInternal( bCollisionUpdate );
	}
}

void AActor::UpdateComponentsInternal(UBOOL bCollisionUpdate)
{
	checkf(!HasAnyFlags(RF_Unreachable), TEXT("%s"), *GetFullName());
	checkf(!HasAnyFlags(RF_ArchetypeObject|RF_ClassDefaultObject), TEXT("%s"), *GetFullName());
	checkf(!ActorIsPendingKill(), TEXT("%s"), *GetFullName());

	const FMatrix&	ActorToWorld = LocalToWorld();

	// if it's a collision only update
	if (bCollisionUpdate)
	{
		// then only bother with the collision component
		if (CollisionComponent != NULL)
		{
			CollisionComponent->UpdateComponent(GWorld->Scene,this,ActorToWorld);
		}
	}
	else
	{
		// Look for components which should be directly attached to the actor, but aren't yet.
		for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
		{
			UActorComponent* Component = Components(ComponentIndex); 
			if( Component )
			{
				Component->UpdateComponent(GWorld->Scene,this,ActorToWorld);
			}
		}
	}
}

/**
 * Flags all components as dirty and then calls UpdateComponents().
 *
 * @param	bCollisionUpdate	[opt] As per UpdateComponents; defaults to FALSE.
	 * @param	bTransformOnly		[opt] TRUE to update only the component transforms, FALSE to update the entire component.
 */
void AActor::ForceUpdateComponents(UBOOL bCollisionUpdate,UBOOL bTransformOnly)
{
	MarkComponentsAsDirty(bTransformOnly);
	ConditionalUpdateComponents( bCollisionUpdate );
}

/**
 * Flags all components as dirty if in the editor, and then calls UpdateComponents().
 *
 * @param	bCollisionUpdate	[opt] As per UpdateComponents; defaults to FALSE.
	 * @param	bTransformOnly		[opt] TRUE to update only the component transforms, FALSE to update the entire component.
 */
void AActor::ConditionalForceUpdateComponents(UBOOL bCollisionUpdate,UBOOL bTransformOnly)
{
	if ( GIsEditor )
	{
		MarkComponentsAsDirty(bTransformOnly);
	}
	ConditionalUpdateComponents( bCollisionUpdate );
}

void AActor::InvalidateLightingCache()
{
	for(INT ComponentIndex = 0;ComponentIndex < AllComponents.Num();ComponentIndex++)
	{
		if(AllComponents(ComponentIndex))
		{
			AllComponents(ComponentIndex)->InvalidateLightingCache();
		}
	}
}

UBOOL AActor::ActorLineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags)
{
	UBOOL Hit = 0;
	for(INT ComponentIndex = 0;ComponentIndex < Components.Num();ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Components(ComponentIndex));
		if(Primitive && !Primitive->LineCheck(Result,End,Start,Extent,TraceFlags))
		{
			Hit = 1;
		}
	}
	return !Hit;
}

///////////////////////////////////////////////////////////////////////////////
// ACTOR COMPONENT
///////////////////////////////////////////////////////////////////////////////

void UActorComponent::CheckForErrors()
{
	if (Owner != NULL && GetClass()->ClassFlags & CLASS_Deprecated)
	{
		GWarn->MapCheck_Add(MCTYPE_WARNING, Owner, *FString::Printf(TEXT("%s::%s is obsolete and must be removed!!!"), *GetName(), *Owner->GetName()), MCACTION_DELETE);
	}
}

UBOOL UActorComponent::IsOwnerSelected() const
{
	return Owner && Owner->IsSelected();
}

void UActorComponent::BeginDestroy()
{
	ConditionalDetach();
	Super::BeginDestroy();
}

/** Do not load this component if our Outer (the Actor) is not needed. */
UBOOL UActorComponent::NeedsLoadForClient() const
{
	check(GetOuter());
	return (GetOuter()->NeedsLoadForClient() && Super::NeedsLoadForClient());
}

/** Do not load this component if our Outer (the Actor) is not needed. */
UBOOL UActorComponent::NeedsLoadForServer() const
{
	check(GetOuter());
	return (GetOuter()->NeedsLoadForServer() && Super::NeedsLoadForServer());
}

/** FComponentReattachContexts for components which have had PreEditChange called but not PostEditChange. */
static TMap<UActorComponent*,FComponentReattachContext*> EditReattachContexts;

void UActorComponent::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if(IsAttached())
	{
		// The component or its outer could be pending kill when calling PreEditChange when applying a transaction.
		// Don't do do a full recreate in this situation, and instead simply detach.
		if( !IsPendingKill() )
		{
			check(!EditReattachContexts.Find(this));
			EditReattachContexts.Set(this,new FComponentReattachContext(this));
		}
		else
		{
			ConditionalDetach();
		}
	}

	// Flush rendering commands to ensure the rendering thread processes the component detachment before it is modified.
	FlushRenderingCommands();
}

void UActorComponent::PostEditChange(UProperty* PropertyThatChanged)
{
	// The component or its outer could be pending kill when calling PostEditChange when applying a transaction.
	// Don't do do a full recreate in this situation, and instead simply detach.
	if( !IsPendingKill() )
	{
		FComponentReattachContext* ReattachContext = EditReattachContexts.FindRef(this);
		if(ReattachContext)
		{
			delete ReattachContext;
			EditReattachContexts.Remove(this);
		}
	}
	else
	{
		ConditionalDetach();
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Returns whether the component is valid to be attached.
 * This should be implemented in subclasses to validate parameters.
 * If this function returns false, then Attach won't be called.
 */
UBOOL UActorComponent::IsValidComponent() const 
{ 
	return IsPendingKill() == FALSE; 
}

/**
 * Attaches the component to a ParentToWorld transform, owner and scene.
 * Requires IsValidComponent() == true.
 */
void UActorComponent::Attach()
{
	checkf(!HasAnyFlags(RF_Unreachable), TEXT("%s"), *GetFullName());
	checkf(!GetOuter()->IsTemplate(), TEXT("'%s' (%s)"), *GetOuter()->GetFullName(), *GetFullName());
	checkf(!IsTemplate(), TEXT("'%s' (%s)"), *GetOuter()->GetFullName(), *GetFullName() );
	check(Scene);
	check(IsValidComponent());
	check(!IsAttached());
	check(!IsPendingKill());

	bAttached = TRUE;

	if(Owner)
	{
		check(!Owner->IsPendingKill());
		// Add the component to the owner's list of all owned components.
		Owner->AllComponents.AddItem(this);
	}
}

/**
 * Updates state dependent on the ParentToWorld transform.
 * Requires bAttached == true
 */
void UActorComponent::UpdateTransform()
{
	check(bAttached);
}

/**
 * Detaches the component from the scene it is in.
 * Requires bAttached == true
 */
void UActorComponent::Detach()
{
	check(IsAttached());

	bAttached = FALSE;

	if(Owner)
	{
		// Remove the component from the owner's list of all owned components.
		Owner->AllComponents.RemoveItem(this);
	}
}

/**
 * Starts gameplay for this component.
 * Requires bAttached == true.
 */
void UActorComponent::BeginPlay()
{
	check(bAttached);
}

/**
 * Updates time dependent state for this component.
 * Requires bAttached == true.
 * @param DeltaTime - The time since the last tick.
 */
void UActorComponent::Tick(FLOAT DeltaTime)
{
	check(bAttached);
}

/**
 * Conditionally calls Attach if IsValidComponent() == true.
 * @param InScene - The scene to attach the component to.
 * @param InOwner - The actor which the component is directly or indirectly attached to.
 * @param ParentToWorld - The ParentToWorld transform the component is attached to.
 */
void UActorComponent::ConditionalAttach(FSceneInterface* InScene,AActor* InOwner,const FMatrix& ParentToWorld)
{
	bNeedsReattach = FALSE;
	bNeedsUpdateTransform = FALSE;

	// If the component was already attached, detach it before reattaching.
	if(IsAttached())
	{
		DetachFromAny();
	}

	Scene = InScene;
	Owner = InOwner;
	SetParentToWorld(ParentToWorld);
	if(IsValidComponent())
	{
		Attach();
	}
}

/**
 * Conditionally calls UpdateTransform if bAttached == true.
 * @param ParentToWorld - The ParentToWorld transform the component is attached to.
 */
void UActorComponent::ConditionalUpdateTransform(const FMatrix& ParentToWorld)
{
	bNeedsUpdateTransform = FALSE;

	SetParentToWorld(ParentToWorld);
	if(bAttached)
	{
		UpdateTransform();
	}
}

/**
 * Conditionally calls UpdateTransform if bAttached == true.
 */
void UActorComponent::ConditionalUpdateTransform()
{
	if(bAttached)
	{
		UpdateTransform();
	}
}

/**
 * Conditionally calls Detach if bAttached == true.
 */
void UActorComponent::ConditionalDetach()
{
	if(bAttached)
	{
		Detach();
	}
	Scene = NULL;
	Owner = NULL;
}

/**
 * Conditionally calls BeginPlay if bAttached == true.
 */
void UActorComponent::ConditionalBeginPlay()
{
	if(bAttached)
	{
		BeginPlay();
	}
}

/**
 * Conditionally calls Tick if bAttached == true.
 * @param DeltaTime - The time since the last tick.
 */
void UActorComponent::ConditionalTick(FLOAT DeltaTime)
{
	if(bAttached)
	{
		Tick(DeltaTime);
	}
}

/**
 * Sets the ticking group for this component
 *
 * @param NewTickGroup the new group to assign
 */
void UActorComponent::SetTickGroup(BYTE NewTickGroup)
{
	TickGroup = NewTickGroup;
}

/**
 * Marks the appropriate world as lighting requiring a rebuild.
 */
void UActorComponent::MarkLightingRequiringRebuild()
{
	// Whether this component is contributing to static lighting.
	UBOOL bIsContributingToStaticLighting	= FALSE;

	// Only primitive components with static shadowing contribute to static lighting.
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(this);
	bIsContributingToStaticLighting			= bIsContributingToStaticLighting || (PrimitiveComponent && PrimitiveComponent->HasStaticShadowing());

	// Only light components with static shadowing contribute to static lighting.
	ULightComponent* LightComponent			= Cast<ULightComponent>(this);
	bIsContributingToStaticLighting			= bIsContributingToStaticLighting || (LightComponent && LightComponent->HasStaticShadowing());

	// No need to mark lighting as requiring to be rebuilt if component doesn't contribute to static lighting.
	if( bIsContributingToStaticLighting )
	{
		// Find out whether the component resides in a PIE package, in which case we don't have to mark anything
		// as requiring lighting to be rebuilt.
		UBOOL bIsInPIEPackage = (GetOutermost()->PackageFlags & PKG_PlayInEditor) ? TRUE : FALSE;
		if( !bIsInPIEPackage )
		{
			// Find out whether the component is part of loaded level/ world.
			UBOOL		bHasWorldInOuterChain	= FALSE;
			UObject*	Object					= GetOuter();
			while( Object )
			{
				if( Object->IsA( UWorld::StaticClass() ) )
				{
					bHasWorldInOuterChain = TRUE;
					break;
				}
				Object = Object->GetOuter();
			}

			// Mark the world as requiring lighting to be rebuilt.
			if( bHasWorldInOuterChain )
			{
				GWorld->GetWorldInfo()->SetMapNeedsLightingFullyRebuilt( TRUE );
			}
		}
	}
}

/** used by DetachFromAny() - recurses over SkeletalMeshComponent attachments until the component is detached or there are no more SkeletalMeshComponents
 * @return whether Component was successfully detached
 */
static UBOOL DetachComponentFromMesh(UActorComponent* Component, USkeletalMeshComponent* Mesh)
{
	Mesh->DetachComponent(Component);
	if (!Component->IsAttached())
	{
		// successfully removed from Mesh
		return TRUE;
	}
	else
	{
		// iterate over attachments and recurse over any SkeletalMeshComponents found
		for (INT i = 0; i < Mesh->Attachments.Num(); i++)
		{
			USkeletalMeshComponent* AttachedMesh = Cast<USkeletalMeshComponent>(Mesh->Attachments(i).Component);
			if (AttachedMesh != NULL && DetachComponentFromMesh(Component, AttachedMesh))
			{
				return TRUE;
			}
		}

		return FALSE;
	}
}

/** if the component is attached, finds out what it's attached to and detaches it from that thing
 * slower, so use only when you don't have an easy way to find out where it's attached
 */
void UActorComponent::DetachFromAny()
{
	if (IsAttached())
	{
		// if there is no owner, just call Detach()
		if (Owner == NULL)
		{
			ConditionalDetach();
		}
		else
		{
			// try to detach from the actor directly
			Owner->DetachComponent(this);
			if (IsAttached())
			{
				// check if the owner has any SkeletalMeshComponents, and if so make sure this component is not attached to them
				// we could optimize this by adding a pointer to ActorComponent for the SkeletalMeshComponent it's attached to
				for (INT i = 0; i < Owner->AllComponents.Num(); i++)
				{
					USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Owner->AllComponents(i));
					if (Mesh != NULL && DetachComponentFromMesh(this, Mesh))
					{
						break;
					}
				}
			}
		}
		check(!IsAttached());
	}
}

void UActorComponent::UpdateComponent(FSceneInterface* InScene,AActor* InOwner,const FMatrix& InLocalToWorld)
{
	if( !IsAttached() )
	{
		// initialize the component if it hasn't already been
		ConditionalAttach(InScene,InOwner,InLocalToWorld);
	}
	else if(bNeedsReattach)
	{
		// Reattach the component if it has changed since it was last updated.
		ConditionalDetach();
		ConditionalAttach(InScene,InOwner,InLocalToWorld);
	}
	else if(bNeedsUpdateTransform)
	{
		// Update the component's transform if the actor has been moved since it was last updated.
		ConditionalUpdateTransform(InLocalToWorld);
	}

	// Update the components attached indirectly via this component.
	UpdateChildComponents();
}

void UActorComponent::BeginDeferredReattach()
{
	bNeedsReattach = TRUE;

	if(Owner)
	{
		// If the component has a static owner, reattach it directly.
		// If it has a dynamic owner, it will be reattached at the end of the tick.
		if(Owner->bStatic)
		{
			Owner->ConditionalUpdateComponents(FALSE);
		}
	}
	else
	{
		// If the component doesn't have an owner, reattach it directly using its existing transform.
		FComponentReattachContext(this);
	}
}

void UActorComponent::BeginDeferredUpdateTransform()
{
	bNeedsUpdateTransform = TRUE;

	if(Owner)
	{
		// If the component has a static owner, update its transform directly.
		// If it has a dynamic owner, it will be updated at the end of the tick.
		if(Owner->bStatic)
		{
			Owner->ConditionalUpdateComponents(FALSE);
		}
	}
	else
	{
		// If the component doesn't have an owner, just call UpdateTransform without actually applying a new transform.
		ConditionalUpdateTransform();
	}
}

/** 
 * Create the bounding box/sphere for this primitive component
 */
void UDrawQuadComponent::UpdateBounds()
{
	const FLOAT MinThick=16.f;
	Bounds = FBoxSphereBounds( 
		LocalToWorld.TransformFVector(FVector(0,0,0)), 
		FVector(MinThick,Width,Height), 
		Max<FLOAT>(Width,Height) );
}


static FLOAT Fade(FLOAT T)
{
	return T * T * T * (T * (T * 6 - 15) + 10);
}

static FLOAT Noise1D(FLOAT X)
{
	enum { NumNoiseSamples = 64 };
	static FLOAT NoiseTable[NumNoiseSamples];
	static UBOOL bFirstRun = TRUE;
	if(bFirstRun)
	{
		FRandomStream RandomStream(0);
		for(INT NoiseIndex = 0;NoiseIndex < NumNoiseSamples;NoiseIndex++)
		{
			NoiseTable[NoiseIndex] = RandomStream.GetFraction();
		}
		bFirstRun = FALSE;
	}

	INT IntX = appFloor(X);
	FLOAT FracX = X - IntX;

	FLOAT NoiseA = NoiseTable[(IntX + 0) & (NumNoiseSamples - 1)];
	FLOAT NoiseB = NoiseTable[(IntX + 1) & (NumNoiseSamples - 1)];

	return Lerp(NoiseA,NoiseB,FracX);
}

FVector FWindSourceSceneProxy::GetWindSkew(const FVector& Location,FLOAT CurrentWorldTime) const
{
	const FLOAT LocalDistance = Max<FLOAT>(0.0f,(Location | Direction) + WORLD_MAX);
	const FLOAT LocalPhase = (Phase + CurrentWorldTime) * Frequency - LocalDistance * InvSpeed;
	return Direction * (Noise1D(LocalPhase) + 0.1f * GMath.SinFloat(LocalPhase * 10.0f)) * Strength;
}

void UWindDirectionalSourceComponent::Attach()
{
	Super::Attach();
	Scene->AddWindSource(this);
}

void UWindDirectionalSourceComponent::Detach()
{
	Super::Detach();
	Scene->RemoveWindSource(this);
}

//
//	UWindDirectionalSourceComponent::GetRenderData
//

FWindSourceSceneProxy* UWindDirectionalSourceComponent::CreateSceneProxy() const
{
	return new FWindSourceSceneProxy(
		Owner->LocalToWorld().TransformNormal(FVector(1,0,0)).SafeNormal(),//LocalToWorld.TransformNormal(FVector(1,0,0)).SafeNormal(),
		Strength,
		Phase,
		Frequency,
		Speed
		);
}

//
//	UStaticMeshComponentFactory::CreatePrimitiveComponent
//

UPrimitiveComponent* UStaticMeshComponentFactory::CreatePrimitiveComponent(UObject* InOuter)
{
	UStaticMeshComponent*	Component = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass(),InOuter);

	Component->CollideActors = CollideActors;
	Component->BlockActors = BlockActors;
	Component->BlockZeroExtent = BlockZeroExtent;
	Component->BlockNonZeroExtent = BlockNonZeroExtent;
	Component->BlockRigidBody = BlockRigidBody;
	Component->HiddenGame = HiddenGame;
	Component->HiddenEditor = HiddenEditor;
	Component->CastShadow = CastShadow;
	Component->Materials = Materials;
	Component->StaticMesh = StaticMesh;

	return Component;
}
