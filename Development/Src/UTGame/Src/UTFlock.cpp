/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
#include "UTGame.h"
#include "EngineAnimClasses.h"

IMPLEMENT_CLASS(AFlockTestActor);
IMPLEMENT_CLASS(AFlockTest_Spawner);
IMPLEMENT_CLASS(AFlockAttractor);

static ANavigationPoint* FindNewTarget(ANavigationPoint* Current)
{
	TArray<ANavigationPoint*> Points;
	for( FActorIterator It; It; ++It )
	{
		ANavigationPoint* Point = Cast<ANavigationPoint>(*It);
		if(Point && Point != Current)
		{
			Points.AddItem(Point);
		}
	}

	if(Points.Num() > 0)
	{
		return Points(appRand() % Points.Num());
	}
	else
	{
		return NULL;
	}
}

static FVector ClosestPointOnLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point)
{
	// Solve to find alpha along line that is closest point
	// http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
	FLOAT A = (LineStart-Point) | (LineEnd - LineStart);
	FLOAT B = (LineEnd - LineStart).SizeSquared();
	FLOAT T = ::Clamp(-A/B, 0.f, 1.f);

	// Generate closest point
	FVector ClosestPoint = LineStart + (T * (LineEnd - LineStart));

	return ClosestPoint;
}

/** Utility that returns sees if we are off the path network, and if so, gives unit dir vector and distance to get back onto it. */
static UBOOL IsOffPath(const FVector& Position, FLOAT& OutDist, FVector& ToPath)
{
	ToPath = FVector(0,0,0);
	OutDist = BIG_NUMBER;

	// Find all nav objects with 1200
	TArray<FNavigationOctreeObject*> NavObjects;  
	GWorld->NavigationOctree->RadiusCheck(Position, 1200, NavObjects);

	for(INT i=0; i<NavObjects.Num(); i++)
	{
		// Only interested in reach specs (edges)
		UReachSpec* Spec = NavObjects(i)->GetOwner<UReachSpec>();
		if(Spec)
		{
			ANavigationPoint* StartNav = Spec->Start;
			ANavigationPoint* EndNav = Spec->End.Nav;
			if(StartNav && EndNav)
			{
				FVector ReachStart = StartNav->Location;
				FVector ReachEnd = EndNav->Location;
			
				// Find closest point on edge from given location
				FVector ClosePoint = ClosestPointOnLine(ReachStart, ReachEnd, Position);

				// See how far that is
				FVector ThisToPath = (ClosePoint - Position);
				FLOAT ThisDist = ThisToPath.Size() - 300.f;

				// If we are 'within' a path - return no vector - no need to move the agent
				if(ThisDist <= 0.f)
				{
					ToPath = (ReachStart - ReachEnd).SafeNormal();
					OutDist = 0.f;
					return FALSE;
				}
				// If not - remember how far away we were
				else if(ThisDist < OutDist)
				{
					ToPath = ThisToPath;
					OutDist = ThisDist;
				}
			}
		}
	}

	// Return vector to closest point
	ToPath.Normalize();
	return TRUE;
}

/** */
void AFlockTestActor::performPhysics(FLOAT DeltaTime)
{
	if(!Spawner)
	{
		return;
	}

	Spawner->UpdateAgent(this, DeltaTime);
}

void AFlockTestActor::SetAgentMoveState(BYTE NewState)
{
	AgentState = NewState;
}

void AFlockTestActor::DoAction(UBOOL bAtTarget, const FVector& TargetLoc)
{
	SetAgentMoveState(EAMS_Idle);

	// Choose from different set if a 'targetted' action
	if(bAtTarget)
	{
		INT AnimIndex = appRand() % Spawner->TargetActionAnimNames.Num();
		ActionSeqNode->SetAnim( Spawner->TargetActionAnimNames(AnimIndex) );

		// Rotate agent to target
		FVector ToTargetDir = (TargetLoc - Location);
		ToTargetRot = ToTargetDir.Rotation();
		ToTargetRot.Pitch = 0;
		bRotateToTargetRot = TRUE;
	}
	else
	{
		INT AnimIndex = appRand() % Spawner->ActionAnimNames.Num();
		ActionSeqNode->SetAnim( Spawner->ActionAnimNames(AnimIndex) );
		bRotateToTargetRot = FALSE;
	}

	ActionBlendNode->SetBlendTarget( 1.f, Spawner->ActionBlendTime );

	EndActionTime = GWorld->GetTimeSeconds() + Spawner->ActionDuration.GetValue(0.f, NULL);
}

INT FindDeltaYaw(const FRotator& A, const FRotator& B)
{
	INT DeltaYaw = A.Yaw - B.Yaw;
	if(DeltaYaw > 32768)
		DeltaYaw -= 65536;
	else if(DeltaYaw < -32768)
		DeltaYaw += 65536;

	return DeltaYaw;
}

/** */
void AFlockTest_Spawner::UpdateAgent(AFlockTestActor* Agent, FLOAT DeltaTime)
{
	FVector FlockCentroid(0,0,0);
	FVector FlockVel(0,0,0);
	INT FlockCount = 0;

	if(!Agent->TargetNav)
	{
		Agent->SetAgentMoveState(EAMS_Move);

		Agent->TargetNav = FindNewTarget(NULL);
		Agent->NextChangeTargetTime = GWorld->GetTimeSeconds() + (appFrand() * ChangeTargetInterval);
		Agent->NextActionTime = GWorld->GetTimeSeconds() + ActionInterval.GetValue(0.f,NULL);
	}

	if(Agent->TargetNav)
	{
		FVector ToTargetVec = Agent->TargetNav->Location - Agent->Location;
		FLOAT TargetDist = ToTargetVec.Size();
		if(TargetDist < 100.f)
		{
			Agent->TargetNav = FindNewTarget(Agent->TargetNav);
		}
	}

	// Change you mind from time to time.
	if(GWorld->GetTimeSeconds() > Agent->NextChangeTargetTime)
	{
		Agent->TargetNav = FindNewTarget(Agent->TargetNav);
		Agent->NextChangeTargetTime = GWorld->GetTimeSeconds() + (appFrand() * ChangeTargetInterval);
	}

	FVector DesireForce(0,0,0);
	FVector	InhibitForce(0,0,0);

	FVector NearestActionTarget(0,0,0);
	FLOAT NearestActionTargetDist = BIG_NUMBER;
	UBOOL bNearbyTarget = FALSE;

	FMemMark Mark(GMem);
	FCheckResult* Link = GWorld->Hash->ActorOverlapCheck(GMem, Agent, Agent->Location, AwareRadius);
	for( FCheckResult* result=Link; result; result=result->GetNext())
	{
		AFlockTestActor* FlockActor = Cast<AFlockTestActor>(result->Actor);
		if(FlockActor)
		{
			FlockCentroid += FlockActor->Location;
			FlockVel += FlockActor->Velocity;

			FVector ToFlockActor = FlockActor->Location - Agent->Location;
			FLOAT ToFlockActorMag = ToFlockActor.Size();
			FLOAT Overlap = AvoidOtherRadius - ToFlockActorMag;
			if(Overlap > 0.f)
			{
				// normalize
				ToFlockActor /= ToFlockActorMag;

				InhibitForce += ((Overlap/AvoidOtherRadius) * -ToFlockActor * AvoidOtherStrength);
			}

			FlockCount++;
		}

		// Look for nearby attractors
		AFlockAttractor* Attractor = Cast<AFlockAttractor>(result->Actor);
		if(Attractor)
		{
			FVector ToAttractor = Attractor->Location - Agent->Location;

			// If enabled, have it add force to agent
			if(Attractor->bAttractorEnabled)
			{
				const FLOAT Distance = ToAttractor.Size();
				FLOAT AttractRadius = Attractor->CylinderComponent->CollisionRadius;

				if(Distance <= AttractRadius)
				{
					// Normalize vector from location to damaged actor.
					ToAttractor = ToAttractor / Distance;

					FLOAT AttractMag = Attractor->Attraction;

					// If desired, do falloff
					if(Attractor->bAttractionFalloff)
					{
						AttractMag *= (1.f - (Distance / AttractRadius));
					}

					DesireForce += (ToAttractor * AttractMag);
				}

				// If its an action target, see if its the closest one so far and save location.
				if(Attractor->bActionAtThisAttractor)
				{
					if(Distance < NearestActionTargetDist)
					{
						NearestActionTarget = Attractor->Location;
						NearestActionTargetDist = Distance;
						bNearbyTarget = TRUE;
					}
				}
			}
		}
	}
	Mark.Pop();

	// Average location and velocity for nearby agents.
	if(FlockCount > 0)
	{
		FlockCentroid /= (FLOAT)FlockCount;
		FlockVel /= (FLOAT)FlockCount;
	}
	
	// Overall crowd acceleration
	DesireForce += CrowdAcc;

	// Move towards center
	FVector ToCentroid = FlockCentroid - Agent->Location;
	DesireForce += ToCentroid * ToCentroidStrength;

	// Match velocity
	DesireForce += (FlockVel - Agent->Velocity) * MatchVelStrength;

	// To target attraction
	FVector ToTarget(0,0,0); 
	if(Agent->TargetNav)
	{
		ToTarget = (Agent->TargetNav->Location - Agent->Location).SafeNormal();
		DesireForce += ToTarget * ToTargetStrength;
	}

	// To path attraction 
	FLOAT DistToPath;
	FVector DirToPath;
	if(IsOffPath(Agent->Location, DistToPath, DirToPath))
	{
		FVector PathForce = (DirToPath * ToPathStrength);
		//GWorld->LineBatcher->DrawLine(Agent->Location, Agent->Location + (PathForce * 50.f), FColor(0,255,255), SDPG_World);
		InhibitForce += (PathForce);
	}
	else
	{
		//if((Velocity | DirToPath) > 0.f)
		if((ToTarget | DirToPath) > 0.f)
		{
			DesireForce += DirToPath * FollowPathStrength;
		}
		else
		{
			DesireForce -= DirToPath * FollowPathStrength;
		}
	}

	// Velocity damping
	FLOAT VMag = Agent->Velocity.Size();
	InhibitForce -= (Agent->Velocity * VMag * Agent->VelDamping);

	// Ensure forces only in Z plane.
	DesireForce.Z = 0;
	InhibitForce.Z = 0;

	// If we just saw a target - reset 'next action' time to use target action interval (more frequent)
	if(bNearbyTarget && !Agent->bHadNearbyTarget)
	{
		Agent->NextActionTime = GWorld->GetTimeSeconds() + TargetActionInterval.GetValue(0.f, NULL);
	}

	// If currently doing action.
	if(Agent->AgentState == EAMS_Idle)
	{
		// If action just finished
		if(GWorld->GetTimeSeconds() > Agent->EndActionTime)
		{
			Agent->TargetNav = FindNewTarget(Agent->TargetNav);
			Agent->ActionBlendNode->SetBlendTarget( 0.f, ActionBlendTime );
			Agent->SetAgentMoveState(EAMS_Move);

			if(bNearbyTarget)
			{
				Agent->NextActionTime = GWorld->GetTimeSeconds() + TargetActionInterval.GetValue(0.f, NULL);
			}
			else
			{
				Agent->NextActionTime = GWorld->GetTimeSeconds() + ActionInterval.GetValue(0.f, NULL);
			}
		}
	}
	else
	{
		UBOOL bOkToAction = (GWorld->GetTimeSeconds() - Agent->EndActionTime > ReActionDelay);
		UBOOL bTimeToAction = (GWorld->GetTimeSeconds() > Agent->NextActionTime);

		// First see if we want to do an action
		if( bOkToAction && (bTimeToAction || VMag < WalkVelThresh) )
		{
			Agent->DoAction(bNearbyTarget, NearestActionTarget);
		}
		else
		{
			Agent->SetAgentMoveState(EAMS_Move);
		}
	}

	// Blend between running and walking anim
	FLOAT SpeedWeight = ((VMag - SpeedBlendStart)/(SpeedBlendEnd - SpeedBlendStart));
	SpeedWeight = ::Clamp<FLOAT>(SpeedWeight, 0.f, 1.f);
	if(Agent->SpeedBlendNode)
	{
		Agent->SpeedBlendNode->SetBlendTarget(SpeedWeight, 0.f);
	}

	// Change anim rate based on speed
	if(Agent->WalkSeqNode)
	{
		Agent->WalkSeqNode->Rate = VMag * AnimVelRate;
	}
		
	if(Agent->RunSeqNode)
	{
		Agent->RunSeqNode->Rate = VMag * AnimVelRate;
	}

	// Force velocity to zero while idling.
	if(Agent->AgentState == EAMS_Idle)
	{
		Agent->Velocity = FVector(0,0,0);
		DesireForce = FVector(0,0,0);
		InhibitForce = FVector(0,0,0);

		// If desired, rotate pawn to look at target when performing action.
		if(Agent->bRotateToTargetRot)
		{
			INT DeltaYaw = FindDeltaYaw(Agent->ToTargetRot, Agent->Rotation);
			FRotator NewRotation = Agent->Rotation;
			NewRotation.Yaw += appRound((FLOAT)DeltaYaw * RotateToTargetSpeed);
			Agent->SetRotation(NewRotation);
		}
	}
	else
	{
		// Integrate force to get new velocity
		Agent->Velocity += (DesireForce + InhibitForce) * DeltaTime;

		// Integrate velocity to get new position.
		FVector NewLocation = (Agent->Location + (Agent->Velocity * DeltaTime));

		// Point in direction of travel
		FRotator NewRotation = Agent->Velocity.Rotation();

		// Cap the maximum yaw rate
		INT DeltaYaw = FindDeltaYaw(NewRotation, Agent->Rotation);
		INT MaxYaw = appRound(DeltaTime * MaxYawRate);
		DeltaYaw = ::Clamp(DeltaYaw, -MaxYaw, MaxYaw);
		NewRotation.Yaw = Agent->Rotation.Yaw + DeltaYaw;

		// Actually 
		FCheckResult Hit(1.f);
		GWorld->MoveActor(Agent, NewLocation - Agent->Location, NewRotation, 0, Hit);
	}

	Agent->bHadNearbyTarget = bNearbyTarget;
}

static inline UBOOL AllComponentsEqual(const FVector& Vec, FLOAT Tolerance=KINDA_SMALL_NUMBER)
{
	return Abs( Vec.X - Vec.Y ) < Tolerance && Abs( Vec.X - Vec.Z ) < Tolerance && Abs( Vec.Y - Vec.Z ) < Tolerance;
}

void AFlockAttractor::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * 500.0f;

	if ( bCtrlDown )
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		CylinderComponent->CollisionHeight += ModifiedScale.X;
		CylinderComponent->CollisionHeight = Max( 0.0f, CylinderComponent->CollisionHeight );
	}
	else
	{
		CylinderComponent->CollisionRadius += ModifiedScale.X;
		CylinderComponent->CollisionRadius = Max( 0.0f, CylinderComponent->CollisionRadius );

		// If non-uniformly scaling, Z scale affects height and Y can affect radius too.
		if ( !AllComponentsEqual(ModifiedScale) )
		{
			CylinderComponent->CollisionHeight += -ModifiedScale.Z;
			CylinderComponent->CollisionHeight = Max( 0.0f, CylinderComponent->CollisionHeight );

			CylinderComponent->CollisionRadius += ModifiedScale.Y;
			CylinderComponent->CollisionRadius = Max( 0.0f, CylinderComponent->CollisionRadius );
		}
	}
}

