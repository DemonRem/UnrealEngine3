/*=============================================================================
	RouteRenderingComponent.cpp: Unreal pathnode placement

	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.

=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAIClasses.h"
#include "DebugRenderSceneProxy.h"

IMPLEMENT_CLASS(URouteRenderingComponent);

/** Represents a RouteRenderingComponent to the scene manager. */
class FRouteRenderingSceneProxy : public FDebugRenderSceneProxy
{
public:

	FRouteRenderingSceneProxy(const URouteRenderingComponent* InComponent):
	  FDebugRenderSceneProxy(InComponent)
	  {
		  // draw reach specs
		  check(InComponent);
		  ARoute *Route = Cast<ARoute>(InComponent->GetOwner());
		  if( Route->NavList.Num() )
		  {
			  if( Route->IsSelected() )
			  {
				  for( INT Idx = 1; Idx < Route->NavList.Num(); Idx++ )
				  {
					  AActor* A = ~Route->NavList(Idx-1);
					  AActor* B = ~Route->NavList(Idx);
					  if( A && B )
					  {
						  new(ArrowLines) FArrowLine(A->Location, B->Location + FVector(0,0,16), FColor(0,0,255));
					  }

					  // If this is the last one
					  if( Route->RouteType == ERT_Circle && Idx == (Route->NavList.Num() - 1) )
					  {
						  A = ~Route->NavList(0);
						  B = ~Route->NavList(Idx);
						  if( A && B )
						  {
							  new(ArrowLines) FArrowLine(A->Location, B->Location + FVector(0,0,16), FColor(0,0,255));
						  }
					  }						
				  }
			  }

			  AActor* Start = ~Route->NavList(0);
			  AActor* End   = ~Route->NavList(Route->NavList.Num()-1);
			  if( Start )
			  {
				  new(DashedLines) FDashedLine( Route->Location, Start->Location, FColor(0,255,0), 16.f );
			  }

			  if( End )
			  {
				  new(DashedLines) FDashedLine( Route->Location, End->Location, FColor(255,0,0), 16.f );
			  }
		  }
	  }

  	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDynamicRelevance = IsShown(View);
		Result.SetDPG(SDPG_World,TRUE);
		if (IsShadowCast(View))
		{
			Result.bShadowRelevance = TRUE;
		}
		return Result;
	}

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const { return( FDebugRenderSceneProxy::GetAllocatedSize() ); }
};


void URouteRenderingComponent::UpdateBounds()
{
	FBox BoundingBox(0);

	ARoute *Route = Cast<ARoute>(Owner);
	if( Route && Route->NavList.Num() )
	{
		BoundingBox += Route->Location;

		for( INT Idx = 0; Idx < Route->NavList.Num(); Idx++ )
		{
			AActor* A = ~Route->NavList(Idx);
			if( A )
			{
				BoundingBox += A->Location;
			}
		}
	}

	Bounds = FBoxSphereBounds( BoundingBox );
}


FPrimitiveSceneProxy* URouteRenderingComponent::CreateSceneProxy()
{
	return new FRouteRenderingSceneProxy(this);
}
