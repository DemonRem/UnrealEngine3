/*=============================================================================
	DecalComponent.cpp: Decal implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "ScenePrivate.h"
#include "UnDecalRenderData.h"
#include "UnTerrain.h"
#include "UnTerrainRender.h"

IMPLEMENT_CLASS(UDecalComponent);

DECLARE_STATS_GROUP(TEXT("DECALS"),STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT("Attach Time"),STAT_DecalAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT("  BSP Attach Time"),STAT_DecalBSPAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT("  Static Mesh Attach Time"),STAT_DecalStaticMeshAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT("  Terrain Attach Time"),STAT_DecalTerrainAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT("  Skeletal Mesh Attach Time"),STAT_DecalSkeletalMeshAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT(" HitComponent Attach Time"),STAT_DecalHitComponentAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT(" HitNode Attach Time"),STAT_DecalHitNodeAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT(" MultiComponent Attach Time"),STAT_DecalMultiComponentAttachTime,STATGROUP_Decals);
DECLARE_CYCLE_STAT(TEXT(" ReceiverImages Attach Time"),STAT_DecalReceiverImagesAttachTime,STATGROUP_Decals);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  FDecalSceneProxy
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !FINAL_RELEASE
class FDecalSceneProxy : public FPrimitiveSceneProxy
{
public:
	FDecalSceneProxy(const UDecalComponent* Component)
		:	FPrimitiveSceneProxy( Component )
		,	Owner( Component->GetOwner() )
		,	DepthPriorityGroup( Component->DepthPriorityGroup )
		,	bSelected( Component->IsOwnerSelected() )
		,	Bounds( Component->Bounds )
		,	Width( Component->Width )
		,	Height( Component->Height )
		,	Thickness( Component->Thickness )
		,	HitLocation( Component->HitLocation )
		,	HitNormal( Component->HitNormal )
		,	HitTangent( Component->HitTangent )
		,	HitBinormal( Component->HitBinormal )
	{
		Component->GenerateDecalFrustumVerts( Verts );
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		const EShowFlags ShowFlags = View->Family->ShowFlags;

		if ( ShowFlags & SHOW_Decals )
		{
			if ( ((ShowFlags & SHOW_Bounds) && (GIsGame || !Owner || bSelected)) ||
				((ShowFlags & SHOW_DecalInfo) && (GIsGame || bSelected)) )
			{
				Result.bDynamicRelevance = TRUE;
				Result.bForegroundDPG = TRUE;
				Result.SetDPG(SDPG_Foreground,TRUE);
			}
		}

		return Result;
	}

	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	* @param	DPGIndex - current depth priority 
	* @param	Flags - optional set of flags from EDrawDynamicElementFlags
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
	{
		if ( View->Family->ShowFlags & SHOW_Decals )
		{	
			if ( (View->Family->ShowFlags & SHOW_Bounds) && ((View->Family->ShowFlags & SHOW_Game) || !Owner || bSelected) )
			{
				//if ( DPGIndex == SDPG_Foreground )
				{
					// Draw the decal's bounding box and sphere.
					DrawWireBox(PDI,Bounds.GetBox(), FColor(72,72,255),SDPG_Foreground);
					DrawCircle(PDI,Bounds.Origin,FVector(1,0,0),FVector(0,1,0),FColor(255,255,0),Bounds.SphereRadius,32,SDPG_Foreground);
					DrawCircle(PDI,Bounds.Origin,FVector(1,0,0),FVector(0,0,1),FColor(255,255,0),Bounds.SphereRadius,32,SDPG_Foreground);
					DrawCircle(PDI,Bounds.Origin,FVector(0,1,0),FVector(0,0,1),FColor(255,255,0),Bounds.SphereRadius,32,SDPG_Foreground);
				}
			}
			if ( (View->Family->ShowFlags & SHOW_DecalInfo) && ((View->Family->ShowFlags & SHOW_Game) || !Owner || bSelected)  )
			{
				//if ( DPGIndex == SDPG_World )
				{
					const FColor White(255, 255, 255);
					const FColor Red(255,0,0);
					const FColor Green(0,255,0);
					const FColor Blue(0,0,255);

					// Upper box.
					PDI->DrawLine( Verts[0], Verts[1], White, SDPG_Foreground );
					PDI->DrawLine( Verts[1], Verts[2], White, SDPG_Foreground );
					PDI->DrawLine( Verts[2], Verts[3], White, SDPG_Foreground );
					PDI->DrawLine( Verts[3], Verts[0], White, SDPG_Foreground );

					// Lower box.
					PDI->DrawLine( Verts[4], Verts[5], White, SDPG_Foreground );
					PDI->DrawLine( Verts[5], Verts[6], White, SDPG_Foreground );
					PDI->DrawLine( Verts[6], Verts[7], White, SDPG_Foreground );
					PDI->DrawLine( Verts[7], Verts[4], White, SDPG_Foreground );

					// Vertical box pieces.
					PDI->DrawLine( Verts[0], Verts[4], White, SDPG_Foreground );
					PDI->DrawLine( Verts[1], Verts[5], White, SDPG_Foreground );
					PDI->DrawLine( Verts[2], Verts[6], White, SDPG_Foreground );
					PDI->DrawLine( Verts[3], Verts[7], White, SDPG_Foreground );

					// Normal, Tangent, Binormal.
					const FLOAT HalfWidth = Width/2.f;
					const FLOAT HalfHeight = Height/2.f;
					PDI->DrawLine( HitLocation, HitLocation + (HitNormal*Thickness), Red, SDPG_Foreground );
					PDI->DrawLine( HitLocation, HitLocation + (HitTangent*HalfWidth), Green, SDPG_Foreground );
					PDI->DrawLine( HitLocation, HitLocation + (HitBinormal*HalfHeight), Blue, SDPG_Foreground );
				}
			}
		}
	}

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
	AActor* Owner;

	BITFIELD DepthPriorityGroup : UCONST_SDPG_NumBits;
	BITFIELD bSelected : 1;

	FBoxSphereBounds Bounds;
	FVector Verts[8];

	FLOAT Width;
	FLOAT Height;
	FLOAT Thickness;

	FVector HitLocation;
	FVector HitNormal;
	FVector HitTangent;
	FVector HitBinormal;
};
#endif // !FINAL_RELEASE

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  UDecalComponent
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @return	TRUE if the decal is enabled as specified in scalability options, FALSE otherwise.
 */
UBOOL UDecalComponent::IsEnabled() const
{
	return (bStaticDecal && GSystemSettings.bAllowStaticDecals) || (!bStaticDecal && GSystemSettings.bAllowDynamicDecals);
}

/**
 * Builds orthogonal planes from HitLocation, HitNormal, Width, Height and Thickness.
 */
void UDecalComponent::UpdateOrthoPlanes()
{
	enum PlaneIndex
	{
		DP_LEFT,
		DP_RIGHT,
		DP_FRONT,
		DP_BACK,
		DP_NEAR,
		DP_FAR,
		DP_NUM
	};

	// Set the decal's backface direction based on the handedness of the owner frame if it's static decal.
	bFlipBackfaceDirection = bStaticDecal && Owner ? (Owner->DrawScale3D.X*Owner->DrawScale3D.Y*Owner->DrawScale3D.Z < 0.f ) : FALSE;

	const FVector Position = Location;
	const FVector Normal = -Orientation.Vector() * (bFlipBackfaceDirection ? -1.0f : 1.0f);
	const FRotationMatrix NewFrame( Orientation );
	const FLOAT OffsetRad = static_cast<FLOAT>( PI*DecalRotation/180. );
	const FLOAT CosT = appCos( OffsetRad );
	const FLOAT SinT = appSin( OffsetRad );

	const FMatrix OffsetMatrix(
		FPlane(1.0f,	0.0f,	0.0f,	0.0f),
		FPlane(0.0f,	CosT,	SinT,	0.0f),
		FPlane(0.0f,	-SinT,	CosT,	0.0f),
		FPlane(0.0f,	0.0f,	0.0f,	1.0f) );

	const FMatrix OffsetFrame( OffsetMatrix*NewFrame );

	const FVector Tangent = -OffsetFrame.GetAxis(1);
	const FVector Binormal = OffsetFrame.GetAxis(2);

	// Ensure the Planes array is correctly sized.
	if ( Planes.Num() != DP_NUM )
	{
		Planes.Empty( DP_NUM );
		Planes.Add( DP_NUM );
	}

	const FLOAT TDotP = Tangent | Position;
	const FLOAT BDotP = Binormal | Position;
	const FLOAT NDotP = Normal | Position;

	Planes(DP_LEFT)	= FPlane( -Tangent, Width/2.f - TDotP );
	Planes(DP_RIGHT)= FPlane( Tangent, Width/2.f + TDotP );

	Planes(DP_FRONT)= FPlane( -Binormal, Height/2.f - BDotP );
	Planes(DP_BACK)	= FPlane( Binormal, Height/2.f + BDotP );

	Planes(DP_NEAR)	= FPlane( Normal, -NearPlane + NDotP );
	Planes(DP_FAR)	= FPlane( -Normal, FarPlane - NDotP );

	HitLocation = Position;
	HitNormal = Normal;
	HitBinormal = Binormal;
	HitTangent = Tangent;
}

/**
 * @return		TRUE if both IsEnabled() and Super::IsValidComponent() return TRUE.
 */
UBOOL UDecalComponent::IsValidComponent() const
{
	return (GEngine ? IsEnabled() : TRUE) && Super::IsValidComponent();
}

void UDecalComponent::CheckForErrors()
{
	Super::CheckForErrors();

	// Get the decal owner's name.
	FString OwnerName(GNone);
	if ( Owner )
	{
		OwnerName = Owner->GetName();
	}

	if ( !DecalMaterial )
	{
		GWarn->MapCheck_Add(MCTYPE_WARNING, Owner, *FString::Printf(TEXT("%s::%s : Decal's material is NULL"), *GetName(), *OwnerName), MCACTION_NONE, TEXT("DecalMaterialNull"));
	}
	else
	{
		// Warn about direct or indirect references to non decal materials.
		const UMaterial* ReferencedMaterial = DecalMaterial->GetMaterial();
		if( ReferencedMaterial && !ReferencedMaterial->IsA( UDecalMaterial::StaticClass() ) )
		{
			GWarn->MapCheck_Add(MCTYPE_WARNING, Owner, *FString::Printf(TEXT("%s::%s : Decal's material is not a DecalMaterial"), *GetName(), *OwnerName), MCACTION_NONE, TEXT("DecalMaterialIsAMaterial"));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Scene attachment
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allocates a sort key to the decal if the decal is non-static.
 */
void UDecalComponent::AllocateSortKey()
{
	if ( !bStaticDecal )
	{
		static INT SDecalSortKey = 100;
		SortOrder = ++SDecalSortKey;
	}
}

void UDecalComponent::Attach()
{
	if ( !GIsUCC )
	{
		DetachFromReceivers();

		// Dynamic decals and static decals not in game attach here.
		// Static decals in game attach in UDecalComponent::BeginPlay().
		if ( !bStaticDecal || (bStaticDecal && !GIsGame) )
		{
			// All decals outside of game will fully compute receivers.
			if ( !GIsGame || StaticReceivers.Num() == 0 )
			{
				ComputeReceivers();
			}
			else
			{
				AttachToStaticReceivers();
			}
		}
	}
	Super::Attach();
}

void UDecalComponent::UpdateTransform()
{
	// Make sure the planes and hit info are up to date for bounds calculations.
	UpdateOrthoPlanes();
	Super::UpdateTransform();
}

#if STATS || LOOKING_FOR_PERF_ISSUES
namespace {

static DOUBLE GSumMsecsSpent = 0.;
static INT GNumSamples = 0;
static INT GSampleSize = 20;

class FScopedTimingBlock
{
	const UDecalComponent* Decal;
	const TCHAR* OutputString;
	DOUBLE StartTime;

public:
	FScopedTimingBlock(UDecalComponent* InDecal, const TCHAR* InString)
		:	Decal( InDecal )
		,	OutputString( InString )
	{
		StartTime = appSeconds();
	}

	~FScopedTimingBlock()
	{
		const DOUBLE MsecsSpent = (appSeconds() - StartTime)*1000.0;

		if ( !Decal->bStaticDecal )
		{
			GSumMsecsSpent += MsecsSpent;
			++GNumSamples;
			if ( GNumSamples == GSampleSize )
			{
				const DOUBLE MsecsAvg = GSumMsecsSpent / static_cast<DOUBLE>(GNumSamples);
				debugf( NAME_DevDecals, TEXT("Decal AVG: (%.3f)"), MsecsAvg );
				GSumMsecsSpent = 0.;
				GNumSamples = 0;
			}
		}

		//if ( MsecsSpent > 1.0 )
		{
			const FString DecalMaterial = Decal->DecalMaterial->GetMaterial()->GetName();
			if ( Decal->bStaticDecal && Decal->GetOwner() )
			{
				// Log the decal actor name if the decal is static (level-placed) and has an owner.
				debugf( NAME_DevDecals, TEXT("Decal %s(%i,%s) - %s(%.3f)"),
					*Decal->GetOwner()->GetName(), Decal->DecalReceivers.Num(), Decal->bNoClip ? TEXT("TRUE"):TEXT("FALSE"), OutputString, MsecsSpent );
			}
			else
			{
				// Log the decal material.
				debugf( NAME_DevDecals, TEXT("Decal %s(%i,%s) - %s(%.3f)"),
					*DecalMaterial, Decal->DecalReceivers.Num(), Decal->bNoClip ? TEXT("TRUE"):TEXT("FALSE"), OutputString, MsecsSpent );
			}
		}
	}
};
} // namespace
#endif

#define ATTACH_RECEIVER(PrimitiveComponent) \
	if( (PrimitiveComponent) && (PrimitiveComponent)->IsAttached() && (PrimitiveComponent)->GetScene() == GetScene() ) \
	{ \
		AttachReceiver( PrimitiveComponent ); \
	}

/**
 * Attaches to static receivers.
 */
void UDecalComponent::AttachToStaticReceivers()
{
	if ( !IsEnabled() )
	{
		return;
	}

	// Make sure the planes and hit info are up to date for bounds calculations.
	UpdateOrthoPlanes();

	if ( DecalMaterial )
	{
		// Is the decal material lit?
		const UMaterial* Material = DecalMaterial->GetMaterial();
		const UBOOL bLitDecalMaterial = Material && (Material->LightingModel != MLM_Unlit);

		// const FScopedTimingBlock TimingBlock( this, TEXT("StaticReceivers") );
		for ( INT ReceiverIndex = 0 ; ReceiverIndex < StaticReceivers.Num() ; ++ReceiverIndex )
		{
			FStaticReceiverData* StaticReceiver = StaticReceivers(ReceiverIndex);
			UPrimitiveComponent* Receiver = StaticReceiver->Component;
			if ( Receiver && Receiver->IsAttached() && Receiver->GetScene() == GetScene() )
			{
				FDecalRenderData* DecalRenderData = NULL;

				// @todo: Fully attach lit decals on static meshes until 1D lightmap serialization issue is fixed.
				if ( bLitDecalMaterial && Receiver->IsA(UStaticMeshComponent::StaticClass()) )
				{
					FDecalState DecalState;
					CaptureDecalState( &DecalState );

					// Generate decal geometry.
					DecalRenderData = Receiver->GenerateDecalRenderData( &DecalState );
					if ( DecalRenderData )
					{
						DecalRenderData->InitResources_GameThread();
						// Add the decal attachment to the receiver.
						Receiver->AttachDecal( this, DecalRenderData, &DecalState );
					}
				}
				else
				{
					DecalRenderData = new FDecalRenderData( NULL, TRUE, TRUE );
					CopyStaticReceiverDataToDecalRenderData( *DecalRenderData, *StaticReceiver );
					DecalRenderData->InitResources_GameThread();

					// Add the decal attachment to the receiver.
					Receiver->AttachDecal( this, DecalRenderData, NULL );
				}

				// Add the receiver to this decal.
				if ( DecalRenderData )
				{
					FDecalReceiver* NewDecalReceiver = new(DecalReceivers) FDecalReceiver;
					NewDecalReceiver->Component = Receiver;
					NewDecalReceiver->RenderData = DecalRenderData;
				}
			}
		}
	}
}

/**
 * Updates ortho planes, computes the receiver set, and connects to a decal manager.
 */
void UDecalComponent::ComputeReceivers()
{
	SCOPE_CYCLE_COUNTER(STAT_DecalAttachTime);

	if ( !IsEnabled() )
	{
		return;
	}

	// Make sure the planes and hit info are up to date for bounds calculations.
	UpdateOrthoPlanes();

	if ( DecalMaterial )
	{
		AllocateSortKey();

		// Hit component to attach to was specified.
		if ( HitComponent )
		{
			SCOPE_CYCLE_COUNTER(STAT_DecalHitComponentAttachTime);
			check( !HitComponent->IsA(UModelComponent::StaticClass()) );
			ATTACH_RECEIVER( HitComponent );
		}
		// Hit node index of BSP to attach to was specified.
		else if ( HitNodeIndex != INDEX_NONE )
		{
			SCOPE_CYCLE_COUNTER(STAT_DecalHitNodeAttachTime);
			check( HitLevelIndex != INDEX_NONE );
			// Retrieve model component associated with node and level and attach it.
			ULevel*				Level			= GWorld->Levels(HitLevelIndex);
			const FBspNode&		Node			= Level->Model->Nodes( HitNodeIndex );
			UModelComponent*	ModelComponent	= Level->ModelComponents( Node.ComponentIndex );
			ATTACH_RECEIVER( ModelComponent );
		}
		// Receivers were specified.
		else if ( ReceiverImages.Num() > 0 )
		{
			SCOPE_CYCLE_COUNTER(STAT_DecalReceiverImagesAttachTime);
			for ( INT ReceiverIndex = 0 ; ReceiverIndex < ReceiverImages.Num() ; ++ReceiverIndex )
			{
				UPrimitiveComponent* PotentialReceiver = ReceiverImages(ReceiverIndex);
				// Fix up bad content as we can't attach a model component via receiver images as it's lacking node information.
				if( PotentialReceiver && PotentialReceiver->IsA(UModelComponent::StaticClass()) )
				{
					warnf(TEXT("Trying to attach %s to %s via ReceiverImages. This is not valid, entry is being removed."),*GetFullName(),*PotentialReceiver->GetFullName());
					ReceiverImages.Remove(ReceiverIndex--);
				}
				ATTACH_RECEIVER( PotentialReceiver );
			}
		}
		// No information about hit, determine ourselves. HitLevelIndex might be set but we ignore it.
		else
		{
			SCOPE_CYCLE_COUNTER(STAT_DecalMultiComponentAttachTime);

			// If the decal is static, get its level as we only want to attach it to the same level.
			ULevel* StaticDecalLevel = NULL;
			if( bStaticDecal && GetOwner() )
			{
				StaticDecalLevel = GetOwner()->GetLevel();
			}

			// Update the decal bounds and query the collision hash for potential receivers.
			UpdateBounds();

#define E3_DECAL_HACK 1 // TTP #61067 to remove it after E3 and fix appropriately

#if E3_DECAL_HACK
			// Array of receiver components, used to make terrain decals to be mutually exclusive with others.
			TArray<UPrimitiveComponent*> TerrainReceiverComponents;
			TArray<UPrimitiveComponent*> NonTerrainReceiverComponents;
#endif

			// Handle actors, aka static meshes, skeletal meshes and terrain.
			const UBOOL bProjectOnNonBSP = bProjectOnStaticMeshes || bProjectOnSkeletalMeshes || bProjectOnTerrain;
			if( bProjectOnNonBSP )
			{
				FMemMark Mark(GMem);
				FCheckResult* ActorResult = GWorld->Hash->ActorOverlapCheck( GMem, NULL, Bounds.Origin, Bounds.SphereRadius, TRACE_AllComponents );

				// Attach to overlapping actors.
				for( FCheckResult* HitResult = ActorResult ; HitResult ; HitResult = HitResult->GetNext() )
				{
					UPrimitiveComponent* HitComponent = HitResult->Component;
					// Only project on actors that accept decals and also check for detail mode as detail mode leaves components in octree
					// for gameplay reasons and simply doesn't render them.
					if ( HitComponent && HitComponent->bAcceptsDecals && (HitComponent->DetailMode <= GSystemSettings.DetailMode) )
					{
						// If this is a static decal with a level, make sure the receiver is in the same level as the decal.
						if ( StaticDecalLevel )
						{
							const AActor* ReceiverOwner = HitComponent->GetOwner();
							if ( ReceiverOwner && !ReceiverOwner->IsInLevel(StaticDecalLevel) )
							{
								continue;
							}
						}

#if E3_DECAL_HACK
						// Add receiver to appropriate array.
						if( HitComponent->IsA(UTerrainComponent::StaticClass()) )
						{
							TerrainReceiverComponents.AddItem( HitComponent );
						}
						else
						{
							NonTerrainReceiverComponents.AddItem( HitComponent );
						}
#else
						// Attach the decal if the level matched, or if the decal was spawned in game.						
						ATTACH_RECEIVER( HitComponent );
#endif
					}
				}

				Mark.Pop();
			}

			// Handle BSP.
			if( bProjectOnBSP )
			{
				// Indices into levels component arrays nodes are associated with.
				TArray<INT> ModelComponentIndices;

				// Iterate over all levels, checking for overlap.
				for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num(); LevelIndex++ )
				{
					ULevel* Level = GWorld->Levels(LevelIndex);

					// If the decal is static, make sure the receiving BSP is in the same level as the decal.
					if( StaticDecalLevel && StaticDecalLevel != Level )
					{
						continue;
					}

					// Filter bounding box down the BSP tree, retrieving overlapping nodes.
					HitNodeIndices.Reset();
					ModelComponentIndices.Reset();
					Level->Model->GetBoxIntersectingNodesAndComponents( Bounds.GetBox(), HitNodeIndices, ModelComponentIndices );

					// Iterate over model components to attach.
					for( INT ModelComponentIndexIndex=0;  ModelComponentIndexIndex<ModelComponentIndices.Num(); ModelComponentIndexIndex++ )
					{
						INT ModelComponentIndex = ModelComponentIndices(ModelComponentIndexIndex);
						UModelComponent* ModelComponent = Level->ModelComponents(ModelComponentIndex);
#if E3_DECAL_HACK
						NonTerrainReceiverComponents.AddItem( ModelComponent );
#else
						ATTACH_RECEIVER( ModelComponent );
#endif
					}
				}
			}

#if E3_DECAL_HACK
			// Temporarily make static mesh and terrain decals mutually exclusive as passed in orientation doesn't suit terrain. Long term this needs to be
			// fixed cleanly by having code calc normal for each individual hit in the multi- attach case.
			if( NonTerrainReceiverComponents.Num() )
			{
				TerrainReceiverComponents.Empty();
			}

			// Attach receivers.
			for( INT ReceiverIndex=0; ReceiverIndex<TerrainReceiverComponents.Num(); ReceiverIndex++ )
			{
				ATTACH_RECEIVER( TerrainReceiverComponents(ReceiverIndex) );
			}
			for( INT ReceiverIndex=0; ReceiverIndex<NonTerrainReceiverComponents.Num(); ReceiverIndex++ )
			{
				ATTACH_RECEIVER( NonTerrainReceiverComponents(ReceiverIndex) );
			}
#endif
		}

		ConnectToManager();
	}
}

void UDecalComponent::AttachReceiver(UPrimitiveComponent* Receiver)
{
	// Invariant: Receiving component is not already in the decal's receiver list
	if ( Receiver && Receiver->bAcceptsDecals && (Receiver->bAcceptsDecalsDuringGameplay || !GWorld->HasBegunPlay()) )
	{
		const UBOOL bIsReceiverHidden = (Receiver->GetOwner() && Receiver->GetOwner()->bHidden) || Receiver->HiddenGame;
		if ( !bIsReceiverHidden || bProjectOnHidden )
		{
			if ( FilterComponent( Receiver ) )
			{
				FDecalState DecalState;
				CaptureDecalState( &DecalState );

				// Generate decal geometry.
				FDecalRenderData* DecalRenderData = Receiver->GenerateDecalRenderData( &DecalState );

				// Was any geometry created?
				if ( DecalRenderData )
				{
					DecalRenderData->InitResources_GameThread();

					// Add the decal attachment to the receiver.
					Receiver->AttachDecal( this, DecalRenderData, &DecalState );

					// Add the receiver to this decal.
					FDecalReceiver* NewDecalReceiver = new(DecalReceivers) FDecalReceiver;
					NewDecalReceiver->Component = Receiver;
					NewDecalReceiver->RenderData = DecalRenderData;
				}
			}
		}
	}
}

/**
 * Disconnects the decal from its manager and detaches receivers.
 */
void UDecalComponent::DetachFromReceivers()
{
	DisconnectFromManager();

	for ( INT ReceiverIndex = 0 ; ReceiverIndex < DecalReceivers.Num() ; ++ReceiverIndex )
	{
		FDecalReceiver& Receiver = DecalReceivers(ReceiverIndex);
		if ( Receiver.Component )
		{
			// Detach the decal from the primitive.
			Receiver.Component->DetachDecal( this );
			Receiver.Component = NULL;
		}
	}

	// Now that the receiver has been disassociated from the decal, clear its render data.
	ReleaseResources( GIsEditor );
}

void UDecalComponent::Detach()
{
	if ( !GIsUCC )
	{
		DetachFromReceivers();
	}
	Super::Detach();
}

/**
 * Enqueues decal render data deletion. Split into separate function to work around ICE with VS.NET 2003.
 *
 * @param DecalRenderData	Decal render data to enqueue deletion for
 */
static void FORCENOINLINE EnqueueDecalRenderDataDeletion( FDecalRenderData* DecalRenderData )
{
	// We have to clear the lightmap reference on the game thread because FLightMap1D::Cleanup enqueues
	// a rendering command.  Rendering commands cannot be enqueued from the rendering thread, which would
	// happen if we let FDecalRenderData's dtor clear the lightmap reference.
	DecalRenderData->LightMap1D = NULL;

	// Enqueue deletion of render data and releasing its resources.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( 
		DeleteDecalRenderDataCommand, 
		FDecalRenderData*,
		DecalRenderData,
		DecalRenderData,
		{
			delete DecalRenderData;
		} );
}

void UDecalComponent::ReleaseResources(UBOOL bBlockOnRelease)
{
	// Iterate over all receivers and enqueue their deletion.
	for ( INT ReceiverIndex = 0 ; ReceiverIndex < DecalReceivers.Num() ; ++ReceiverIndex )
	{
		FDecalReceiver& Receiver = DecalReceivers(ReceiverIndex);
		if ( Receiver.RenderData )
		{
			// Ensure the component has already been disassociated from the decal before clearing its render data.
			check( Receiver.Component == NULL );
			// Enqueue deletion of decal render data.
			EnqueueDecalRenderDataDeletion( Receiver.RenderData );
			// No longer safe to access, NULL out.
			Receiver.RenderData = NULL;
		}
	}

	// Empty DecalReceivers array now that resource deletion has been enqueued.
	DecalReceivers.Empty();

	// Create a fence for deletion of component in case there is a pending init e.g.
	if ( !ReleaseResourcesFence )
	{
		ReleaseResourcesFence = new FRenderCommandFence;
	}
	ReleaseResourcesFence->BeginFence();

	// Wait for fence in case we requested to block on release.
	if( bBlockOnRelease )
	{
		ReleaseResourcesFence->Wait();
	}
}

void UDecalComponent::BeginPlay()
{
	Super::BeginPlay();

	// Static decals in game attach here.
	// Dynamic decals and static decals not in game attach in UDecalComponent::Attach().
	if ( bStaticDecal && GIsGame && !GIsUCC )
	{
		if ( StaticReceivers.Num() == 0 )
		{
			ComputeReceivers();
		}
		else
		{
			AttachToStaticReceivers();
		}
	}
}

void UDecalComponent::BeginDestroy()
{
	Super::BeginDestroy();
	ReleaseResources( FALSE );
	FreeStaticReceivers();
}

/**
 * Frees any StaticReceivers data.
 */
void UDecalComponent::FreeStaticReceivers()
{
	// Free any static receiver information.
	for ( INT ReceiverIndex = 0 ; ReceiverIndex < StaticReceivers.Num() ; ++ReceiverIndex )
	{
		delete StaticReceivers(ReceiverIndex);
	}
	StaticReceivers.Empty();
}

UBOOL UDecalComponent::IsReadyForFinishDestroy()
{
	check(ReleaseResourcesFence);
	const UBOOL bDecalIsReadyForFinishDestroy = ReleaseResourcesFence->GetNumPendingFences() == 0;
	return bDecalIsReadyForFinishDestroy && Super::IsReadyForFinishDestroy();
}

void UDecalComponent::FinishDestroy()
{
	// Finish cleaning up any receiver render data.
	for ( INT ReceiverIndex = 0 ; ReceiverIndex < DecalReceivers.Num() ; ++ReceiverIndex )
	{
		FDecalReceiver& Receiver = DecalReceivers(ReceiverIndex);
		delete Receiver.RenderData;
	}
	DecalReceivers.Empty();

	// Delete any existing resource fence.
	delete ReleaseResourcesFence;
	ReleaseResourcesFence = NULL;

	Super::FinishDestroy();
}

void UDecalComponent::PreSave()
{
	Super::PreSave();

	// Mitigate receiver attachment cost for static decals by storing off receiver render data.
	// Don't save static receivers when cooking because intersection queries with potential receivers
	// may not function properly.  Instead, we require static receivers to have been computed when the
	// level was saved in the editor.
	if ( bStaticDecal && !GIsUCC )
	{
		FreeStaticReceivers();
		for ( INT ReceiverIndex = 0 ; ReceiverIndex < DecalReceivers.Num(); ++ReceiverIndex )
		{
			FDecalReceiver& DecalReceiver = DecalReceivers(ReceiverIndex);
			if ( DecalReceiver.Component && DecalReceiver.RenderData )
			{
				FStaticReceiverData* NewStaticReceiver	= new FStaticReceiverData;
				NewStaticReceiver->Component			= DecalReceiver.Component;
				CopyDecalRenderDataToStaticReceiverData( *NewStaticReceiver, *DecalReceiver.RenderData );

				StaticReceivers.AddItem( NewStaticReceiver );
			}
		}
		StaticReceivers.Shrink();
	}
}

/**
 * @return		TRUE if the application filter passes the specified component, FALSE otherwise.
 */
UBOOL UDecalComponent::FilterComponent(UPrimitiveComponent* Component) const
{
	UBOOL bResult = TRUE;

	const AActor* TheOwner = Component->GetOwner();
	if ( !TheOwner )
	{
		// Actors with no owners fail if the filter is an affect filter.
		if ( FilterMode == FM_Affect )
		{
			bResult = FALSE;
		}
	}
	else
	{
		// The actor has an owner; pass it through the filter.
		if ( FilterMode == FM_Ignore )
		{
			// Reject if the component is in the filter.
			bResult = !Filter.ContainsItem( const_cast<AActor*>(TheOwner) );
		}
		else if ( FilterMode == FM_Affect )
		{
			// Accept if the component is in the filter.
			bResult = Filter.ContainsItem( const_cast<AActor*>(TheOwner) );
		}
	}

	return bResult;
}

/**
 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
 *
 * @return The proxy object.
 */
FPrimitiveSceneProxy* UDecalComponent::CreateSceneProxy()
{
#if !FINAL_RELEASE
	return new FDecalSceneProxy( this );
#else
	return NULL;
#endif // !FINAL_RELEASE
}

/**
 * Sets the component's bounds based on the vertices of the decal frustum.
 */
void UDecalComponent::UpdateBounds()
{
	FVector Verts[8];
	GenerateDecalFrustumVerts( Verts );

	Bounds = FBoxSphereBounds( FBox( Verts, 8 ) );

	// Expand the bounds slightly to prevent false occlusion.
	static FLOAT s_fOffset	= 1.0f;
	static FLOAT s_fScale	= 1.1f;
	const FVector Value(Bounds.BoxExtent.X + s_fOffset, Bounds.BoxExtent.Y + s_fOffset, Bounds.BoxExtent.Z + s_fOffset);
	Bounds = FBoxSphereBounds(Bounds.Origin,(Bounds.BoxExtent + FVector(s_fOffset)) * s_fScale,(Bounds.SphereRadius + s_fOffset) * s_fScale);
}

/**
 * Updates all associated lightmaps with the new value for allowing directional lightmaps.
 * This is only called when switching lightmap modes in-game and should never be called on cooked builds.
 */
void UDecalComponent::UpdateLightmapType(UBOOL bAllowDirectionalLightMaps)
{
	for(INT RenderDataIndex = 0;RenderDataIndex < RenderData.Num();RenderDataIndex++)
	{
		if(RenderData(RenderDataIndex)->LightMap1D != NULL)
		{
			RenderData(RenderDataIndex)->LightMap1D->UpdateLightmapType(bAllowDirectionalLightMaps);
		}
	}
}

/**
 * Fills in the specified vertex list with the local-space decal frustum vertices.
 */
void UDecalComponent::GenerateDecalFrustumVerts(FVector Verts[8]) const
{
	const FLOAT HalfWidth = Width/2.f;
	const FLOAT HalfHeight = Height/2.f;
	Verts[0] = Location + (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	Verts[1] = Location + (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	Verts[2] = Location - (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	Verts[3] = Location - (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	Verts[4] = Location + (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	Verts[5] = Location + (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	Verts[6] = Location - (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	Verts[7] = Location - (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * FarPlane);
}

/**
 * Fills in the specified decal state object with this decal's state.
 */
void UDecalComponent::CaptureDecalState(FDecalState* DecalState) const
{
	DecalState->DecalComponent = this;

	// Capture the decal material, or the default material if no material was specified.
	DecalState->DecalMaterial = DecalMaterial;
	if ( !DecalState->DecalMaterial )
	{
		DecalState->DecalMaterial = GEngine->DefaultMaterial;
	}

	// Compute the decal state's material view relevance flags.
	DecalState->MaterialViewRelevance = DecalState->DecalMaterial->GetViewRelevance();

	DecalState->OrientationVector = Orientation.Vector();
	DecalState->HitLocation = HitLocation;
	DecalState->HitNormal = HitNormal;
	DecalState->HitTangent = HitTangent;
	DecalState->HitBinormal = HitBinormal;
	DecalState->OffsetX = OffsetX;
	DecalState->OffsetY = OffsetY;

	DecalState->Thickness = Thickness;
	DecalState->Width = Width;
	DecalState->Height = Height;

	DecalState->DepthBias = DepthBias;
	DecalState->SlopeScaleDepthBias = SlopeScaleDepthBias;
	DecalState->SortOrder = SortOrder;

	DecalState->Planes = Planes;
	DecalState->WorldTexCoordMtx = FMatrix( TileX*HitTangent/Width,
											TileY*HitBinormal/Height,
											HitNormal,
											FVector(0.f,0.f,0.f) ).Transpose();
	DecalState->DecalFrame = FMatrix( -HitNormal, HitTangent, HitBinormal, HitLocation );
	DecalState->WorldToDecal = DecalState->DecalFrame.Inverse();
	DecalState->HitBone = HitBone;
	DecalState->HitBoneIndex = INDEX_NONE;
	if( HitNodeIndex != INDEX_NONE )
	{
		DecalState->HitNodeIndices.Empty(1);
		DecalState->HitNodeIndices.AddItem(HitNodeIndex);
	}
	else
	{
		DecalState->HitNodeIndices = HitNodeIndices;
	}
	DecalState->HitLevelIndex = HitLevelIndex;
	DecalState->SampleRemapping.Empty();
	DecalState->DepthPriorityGroup = DepthPriorityGroup;
	DecalState->bNoClip = bNoClip;
	DecalState->bProjectOnBackfaces = bProjectOnBackfaces;
	DecalState->bFlipBackfaceDirection = bFlipBackfaceDirection;
	DecalState->bProjectOnBSP = bProjectOnBSP;
	DecalState->bProjectOnStaticMeshes = bProjectOnStaticMeshes;
	DecalState->bProjectOnSkeletalMeshes = bProjectOnSkeletalMeshes;
	DecalState->bProjectOnTerrain = bProjectOnTerrain;

	// Compute frustum verts.
	const FLOAT HalfWidth = Width/2.f;
	const FLOAT HalfHeight = Height/2.f;
	DecalState->FrustumVerts[0] = HitLocation + (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	DecalState->FrustumVerts[1] = HitLocation + (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	DecalState->FrustumVerts[2] = HitLocation - (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	DecalState->FrustumVerts[3] = HitLocation - (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * NearPlane);
	DecalState->FrustumVerts[4] = HitLocation + (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	DecalState->FrustumVerts[5] = HitLocation + (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	DecalState->FrustumVerts[6] = HitLocation - (HitBinormal * HalfHeight) - (HitTangent * HalfWidth) - (HitNormal * FarPlane);
	DecalState->FrustumVerts[7] = HitLocation - (HitBinormal * HalfHeight) + (HitTangent * HalfWidth) - (HitNormal * FarPlane);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Interaction with UDecalManager
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * return	The UDecalManager object appropriate for this decal.
 */
UDecalManager* UDecalComponent::GetManager()
{
	if ( !bStaticDecal )
	{
		// Use the scene's world rather than GWorld directly, so that
		// decals that are reattached while PIE is open are associated
		// with the manager of the correct world.
		FSceneInterface* CurrentScene = GetScene();
		if ( CurrentScene && CurrentScene->GetWorld() )
		{
			return CurrentScene->GetWorld()->DecalManager;
		}
	}

	return NULL;
}

/**
 * Connects this decal to the appropriate manager.  Called by eg ComputeReceivers.
 */
void UDecalComponent::ConnectToManager()
{
	UDecalManager* Manager = GetManager();
	if ( Manager )
	{
		Manager->Connect( this );
	}
}

/**
 * Disconnects the decal from its manager.  Called by eg DetachFromReceivers.
 */
void UDecalComponent::DisconnectFromManager()
{
	UDecalManager* Manager = GetManager();
	if ( Manager )
	{
		Manager->Disconnect( this );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Serialization
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UDecalComponent::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

	if ( Ar.Ver() >= VER_DECAL_STATIC_DECALS_SERIALIZED )
	{
		if ( Ar.IsLoading() )
		{
			/////////////////////////
			// Loading.

			// Load number of static receivers from archive.
			INT NumStaticReceivers = 0;
			Ar << NumStaticReceivers;
			// Free existing static receivers.
			FreeStaticReceivers();
			StaticReceivers.AddZeroed(NumStaticReceivers);
			for ( INT ReceiverIndex = 0 ; ReceiverIndex < NumStaticReceivers ; ++ReceiverIndex )
			{
				// Allocate new static receiver data.
				FStaticReceiverData* NewStaticReceiver = new FStaticReceiverData;
				// Fill in its members from the archive.
				Ar << *NewStaticReceiver;
				// Add it to the list of static receivers.
				StaticReceivers(ReceiverIndex) = NewStaticReceiver;
			}
		}
		else if ( Ar.IsSaving() )
		{
			/////////////////////////
			// Saving.

			// Save number of static receivers to archive.
			INT NumStaticReceivers = StaticReceivers.Num();
			Ar << NumStaticReceivers;

			// Write each receiver to the archive.
			for ( INT ReceiverIndex = 0 ; ReceiverIndex < NumStaticReceivers ; ++ReceiverIndex )
			{
				FStaticReceiverData* StaticReceiver = StaticReceivers(ReceiverIndex);
				Ar << *StaticReceiver;
			}
		}
		else if ( Ar.IsObjectReferenceCollector() )
		{
			// When collecting object references, be sure to include the components referenced via StaticReceivers.
			for ( INT ReceiverIndex = 0 ; ReceiverIndex < StaticReceivers.Num() ; ++ReceiverIndex )
			{
				FStaticReceiverData* StaticReceiver = StaticReceivers(ReceiverIndex);
				Ar << StaticReceiver->Component;
			}
		}
	}
	else
	{
		if ( Ar.Ver() >= VER_DECAL_REFACTOR )
		{
		}
		else if ( Ar.Ver() == VER_DECAL_RENDERDATA )
		{
			// Discard render data by serializing into a temp buffer.
			TArray<FDecalRenderData> TempRenderData;
			Ar << TempRenderData;
		}
		else if ( Ar.Ver() >= VER_DECAL_RENDERDATA_POINTER )
		{
			if ( Ar.IsLoading() )
			{
				INT NewNumRenderData;
				Ar << NewNumRenderData;

				// Allocate new render data objects and read their values from the archive
				for ( INT i = 0 ; i < NewNumRenderData ; ++i )
				{
					FDecalRenderData NewRenderData;
					Ar << NewRenderData;
				}
			}
		}
	}
}
