//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameVehicleClasses.h"
#include "EngineAnimClasses.h"
//#include "UTGameAnimationClasses.h"
//#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "UnPath.h"


IMPLEMENT_CLASS(AUTOnslaughtObjective);
IMPLEMENT_CLASS(AUTOnslaughtPanelNode);
IMPLEMENT_CLASS(AUTOnslaughtPowernode);
IMPLEMENT_CLASS(AUTOnslaughtPowerCore);
IMPLEMENT_CLASS(AUTOnslaughtNodeObjective);
IMPLEMENT_CLASS(AUTVehicleFactory);
IMPLEMENT_CLASS(AUTOnslaughtFlag);
IMPLEMENT_CLASS(AUTOnslaughtGodBeam);
IMPLEMENT_CLASS(ULinkRenderingComponent);
IMPLEMENT_CLASS(UUTOnslaughtMapInfo);


void AUTOnslaughtFlag::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	// update god beam position
	if ( MyGodBeam )
	{
		AActor *DesiredBase = (Base && Base->GetAPawn()) ? Base : this;
		if ( MyGodBeam->Base != DesiredBase )
		{
			GWorld->FarMoveActor(MyGodBeam,Location,0,1,0);
			MyGodBeam->SetBase(Base);
			MyGodBeam->NodeBeamEffect->SetHiddenGame( Base && Base->GetAPawn() && Base->GetAPawn()->IsHumanControlled() && Base->GetAPawn()->IsLocallyControlled() );
		}
		MyGodBeam->SetRotation(FRotator(0,0,0));
	}

	// make attachment to pawn springy
	if (WorldInfo->NetMode != NM_DedicatedServer && Mesh && (WorldInfo->TimeSeconds - Mesh->LastRenderTime < 0.2f) )
	{
		Mesh->Scale = NormalOrbScale;
		if ( Base && Base->GetAPawn() )
		{
			AVehicle* HolderVehicle = Base->GetAPawn()->DrivenVehicle;
			if ( HolderVehicle && HolderVehicle->IsHumanControlled() && HolderVehicle->IsLocallyControlled() )
			{
				// make smaller if holder is on hoverboard
				Mesh->Scale = HoverboardOrbScale;
			}

			// figure out where the mesh should be now
			FVector OldMeshLocation = Mesh->LocalToWorld.GetOrigin() - Location + OldLocation;
			FVector Diff = Location - OldMeshLocation;
			FVector NewMeshLocation = OldMeshLocation + Diff.SafeNormal() * Diff.Size() * DeltaSeconds;
			// determine the translation needed to get it there
			Mesh->Translation = WorldToLocal().TransformFVector(NewMeshLocation);
			// if the translation is too large, clamp it
			if (Mesh->Translation.Size() > MaxSpringDistance)
			{
				Mesh->Translation = MaxSpringDistance * Mesh->Translation.SafeNormal();
			}

			if (TetherEffect != NULL)
			{
				if (TetherEffect->HiddenGame)
				{
					TetherEffect->SetHiddenGame(FALSE);
					TetherEffect->ActivateSystem();
				}
				TetherEffect->SetVectorParameter(TetherEffectEndPointName, Base->Location - Mesh->Translation);
				TetherEffect->Translation = Mesh->Translation;
				TetherEffect->BeginDeferredReattach();
			}
		}
		else
		{
			if (!Mesh->Translation.IsZero())
			{
				// quickly translate back to zero
				if (Mesh->Translation.Size() < MaxSpringDistance * DeltaSeconds)
				{
					Mesh->Translation = FVector(0.f, 0.f, 0.f);
				}
				else
				{
					Mesh->Translation -= Mesh->Translation.UnsafeNormal() * (MaxSpringDistance * DeltaSeconds);
				}
			}

			if (TetherEffect != NULL && !TetherEffect->HiddenGame)
			{
				TetherEffect->DeactivateSystem();
				TetherEffect->SetHiddenGame(TRUE);
			}
		}

		// apply Mesh transforms to effect as well
		if (FlagEffectComp != NULL && (FlagEffectComp->Translation != Mesh->Translation || FlagEffectComp->Scale != Mesh->Scale))
		{
			FlagEffectComp->Translation = Mesh->Translation;
			FlagEffectComp->Scale = Mesh->Scale;
			FlagEffectComp->BeginDeferredUpdateTransform();
		}

		// Rotate
		Mesh->Rotation += RotationRate * DeltaSeconds;
		Mesh->BeginDeferredUpdateTransform();
	}
	OldLocation = Location;
}

/** @return the optimal location to fire weapons at this actor */
FVector AUTOnslaughtPowernode::GetTargetLocation(AActor* RequestedBy)
{
	return (EnergySphereCollision != NULL) ? EnergySphereCollision->LocalToWorld.GetOrigin() : Location;
}

UBOOL AUTOnslaughtNodeObjective::ProscribePathTo(ANavigationPoint* Nav, AScout* Scout)
{
	// auto-proscribe paths to nearby nodes that are never active simultaneously in a link setup
	AUTOnslaughtNodeObjective* OtherNode = Cast<AUTOnslaughtNodeObjective>(Nav);
	if (OtherNode != NULL && (OtherNode->Location - Location).Size() < Scout->GetSize(FName(TEXT("Human"),FNAME_Find)).X * 2.0f)
	{
		UUTOnslaughtMapInfo* MapInfo = Cast<UUTOnslaughtMapInfo>(WorldInfo->GetMapInfo());
		if (MapInfo != NULL && MapInfo->LinkSetups.Num() > 0)
		{
			UBOOL bBothNodesAreActive = FALSE;
			for (INT i = 0; i < MapInfo->LinkSetups.Num(); i++)
			{
				UBOOL bThisNodeIsActive = FALSE, bOtherNodeIsActive = FALSE;
				// check node links and standalone nodes for presence of the two nodes we're testing
				for (INT j = 0; j < MapInfo->LinkSetups(i).NodeLinks.Num(); j++)
				{
					if (MapInfo->LinkSetups(i).NodeLinks(j).FromNode == this || MapInfo->LinkSetups(i).NodeLinks(j).ToNode == this)
					{
						bThisNodeIsActive = TRUE;
					}
					else if (MapInfo->LinkSetups(i).NodeLinks(j).FromNode == OtherNode || MapInfo->LinkSetups(i).NodeLinks(j).ToNode == OtherNode)
					{
						bOtherNodeIsActive = TRUE;
					}
				}
				if (MapInfo->LinkSetups(i).StandaloneNodes.ContainsItem(this))
				{
					bThisNodeIsActive = TRUE;
				}
				if (MapInfo->LinkSetups(i).StandaloneNodes.ContainsItem(OtherNode))
				{
					bOtherNodeIsActive = TRUE;
				}

				bBothNodesAreActive = (bThisNodeIsActive && bOtherNodeIsActive);
				if (bBothNodesAreActive)
				{
					break;
				}
			}
			if (!bBothNodesAreActive)
			{
				// proscribe the path
				UReachSpec* NewSpec = ConstructObject<UReachSpec>(UProscribedReachSpec::StaticClass(),GetOuter(),NAME_None);
				NewSpec->Start = this;
				NewSpec->End = Nav;
				NewSpec->Distance = appTrunc((Location - Nav->Location).Size());
				PathList.AddItem(NewSpec);
				return TRUE;
			}
		}
	}

	return Super::ProscribePathTo(Nav, Scout);
}

void AUTOnslaughtPowernode::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if (WorldInfo->NetMode != NM_DedicatedServer && GWorld->GetWorldInfo()->TimeSeconds - LastRenderTime < 0.2f)
	{
		// rotate energy sphere and spinner mesh
		const INT RotationChange = Max<INT>(1, appTrunc(DeltaSeconds * YawRotationRate));
		if (EnergySphere != NULL && !EnergySphere->HiddenGame)
		{
			EnergySphere->Rotation.Yaw += RotationChange;
			// this is just visual, so we don't need to update it immediately
			EnergySphere->BeginDeferredReattach();
		}
		if (NodeBaseSpinner != NULL)
		{
			NodeBaseSpinner->Rotation.Yaw += RotationChange;
			NodeBaseSpinner->BeginDeferredReattach();
		}
		if(FlagLinkEffect)
		{
			const INT StraightYaw = (Location - FlagLinkEffect->Location).Rotation().Yaw;
			const INT DeltaYaw = StraightYaw - FlagLinkEffect->Rotation.Yaw;
			/* RIGHT CURVE */
			if(DeltaYaw != 8192)
			{
				FRotator NewRot = FlagLinkEffect->Rotation;
				NewRot.Yaw = StraightYaw + 8192;
				FlagLinkEffect->ParticleSystemComponent->Rotation = NewRot;
				FlagLinkEffect->ParticleSystemComponent->BeginDeferredUpdateTransform();
			}			
			/* Dead Straight*/ // Left commented for testing if needed since straight lines are nice and easy to see visually.
			/*
			if(DeltaYaw != 0)
			{
				FRotator NewRot = FlagLinkEffect->Rotation;
				NewRot.Yaw = StraightYaw;
				FlagLinkEffect->ParticleSystemComponent->Rotation = NewRot;
				FlagLinkEffect->ParticleSystemComponent->BeginDeferredUpdateTransform();
			}
			*/
		}

// 		// here we are setting the various particle system parameters on the necrification of the node
// 		if( bBeingCapturedByNecris == TRUE )
// 		{
// 			const FLOAT PercentOfHealth = Health / DamageCapacity;
// 
// 			//warnf( TEXT( " %f %f %f %f"), PercentOfHealth, Health, ShieldDamageCounter, DamageCapacity );
// 
// // 			if( PercentOfHealth >= 1.0f )
// // 			{
// // 				FLOAT Opacity = 0.0f;;
// // 				PSC_NecrisGooPuddle->GetFloatParameter( TEXT("Nec_PuddleOpacity"), Opacity );
// // 				Opacity += DeltaSeconds;
// // 				PSC_NecrisGooPuddle->SetFloatParameter( TEXT("Nec_PuddleOpacity"), Opacity );
// // 			}
// // 			else if( PercentOfHealth >= 0.5f )
// // 			{
// // 				const FLOAT Opacity = (PercentOfHealth - 0.5f) * 1.25;
// // 				//warnf( TEXT( " %f %f "), PercentOfHealth, Opacity);
// // 				PSC_NecrisGooPuddle->SetFloatParameter( TEXT("Nec_PuddleOpacity"), Opacity );
// // 			}
// 		}
// 		else
// 		{
// 
// 		}
	}
}


void AUTOnslaughtPowerCore::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if (WorldInfo->NetMode != NM_DedicatedServer && BaseMesh != NULL)
	{
		// only rotate if not rendered if local player is nearby
		UBOOL bAlwaysAnimate = FALSE;
		for( INT iPlayers=0; iPlayers<GEngine->GamePlayers.Num(); iPlayers++ )
		{
			if ( GEngine->GamePlayers(iPlayers) && GEngine->GamePlayers(iPlayers)->Actor && GEngine->GamePlayers(iPlayers)->Actor->ViewTarget 
				&& ((GEngine->GamePlayers(iPlayers)->Actor->ViewTarget->Location - Location).SizeSquared() < 9000000.f) )
			{
				bAlwaysAnimate = TRUE;
				break;
			}
		}
		BaseMesh->bUpdateSkelWhenNotRendered = bAlwaysAnimate;
		BaseMesh->bIgnoreControllersWhenNotRendered = !bAlwaysAnimate;

		FLOAT MaxEnergyEffectDistSquared = Square(MaxEnergyEffectDist);
		for (INT i = 0; i < ARRAY_COUNT(EnergyEffects); i++)
		{
			if (EnergyEffects[i].Effect != NULL && !EnergyEffects[i].Effect->HiddenGame)
			{
				for (INT j = 0; j < 2; j++)
				{
					if (EnergyEffects[i].CurrentEndPointBones[j] != NAME_None)
					{
						FVector BoneLoc = BaseMesh->GetBoneLocation(EnergyEffects[i].CurrentEndPointBones[j]);
						if ((BoneLoc - EnergyEffects[i].Effect->GetOrigin()).SizeSquared2D() > MaxEnergyEffectDistSquared)
						{
							// too far away, clear it
							EnergyEffects[i].CurrentEndPointBones[j] = NAME_None;
						}
						else
						{
							// update endpoint location
							EnergyEffects[i].Effect->SetVectorParameter(EnergyEndPointParameterNames[j], BoneLoc);
						}
					}
					if (EnergyEffects[i].CurrentEndPointBones[j] == NAME_None)
					{
						// choose a new bone for the endpoint by picking a random location on the powercore sphere that's close to the effect start location
						FVector BoneLoc;
						FVector TestLocation = EnergyEffects[i].Effect->GetOrigin() + (EnergyEffects[i].Effect->LocalToWorld.GetAxis(0) * -50.f) +
												(EnergyEffects[i].Effect->LocalToWorld.GetAxis(1) * (MaxEnergyEffectDist * appFrand())) +
												FVector(0.f, 0.f, (-MaxEnergyEffectDist * 0.5f) + (1.5f * MaxEnergyEffectDist * appFrand()));
						EnergyEffects[i].CurrentEndPointBones[j] = BaseMesh->FindClosestBone(TestLocation, &BoneLoc);
						// don't let both beams target the same bone and don't let a beam target a bone that doesn't have the correct prefix
						if ( EnergyEffects[i].CurrentEndPointBones[j] == EnergyEffects[i].CurrentEndPointBones[1 - j] ||
							(EnergyEndPointBonePrefix.Len() > 0 && !EnergyEffects[i].CurrentEndPointBones[j].ToString().StartsWith(EnergyEndPointBonePrefix)) )
						{
							EnergyEffects[i].CurrentEndPointBones[j] = NAME_None;
						}
						else if (EnergyEffects[i].CurrentEndPointBones[j] != NAME_None)
						{
							EnergyEffects[i].Effect->SetVectorParameter(EnergyEndPointParameterNames[j], BoneLoc);
						}
					}
				}
			}
		}
	}
}
						

void ULinkRenderingComponent::UpdateBounds()
{
	FBox BoundingBox(0);

	AUTOnslaughtNodeObjective *node = Cast<AUTOnslaughtNodeObjective>(Owner);
	if ( node )
	{
		BoundingBox += node->Location;

		for (INT i=0; i<UCONST_MAXNUMLINKS; i++)
		{
			if ( node->LinkedNodes[i] )
				BoundingBox += node->LinkedNodes[i]->Location;
		}
	}
	Bounds = FBoxSphereBounds(BoundingBox);
}

static void DrawLineArrow(FPrimitiveDrawInterface* PDI,const FVector &start,const FVector &end,const FColor &color,FLOAT mag)
{
	// draw a pretty arrow
	FVector dir = end - start;
	FLOAT dirMag = dir.Size();
	dir /= dirMag;
	FMatrix arrowTM = FMatrix::Identity;
	arrowTM.SetOrigin(start);
	FVector YAxis, ZAxis;
	dir.FindBestAxisVectors(YAxis,ZAxis);
	arrowTM.SetAxis(0,dir);
	arrowTM.SetAxis(1,YAxis);
	arrowTM.SetAxis(2,ZAxis);
	DrawDirectionalArrow(PDI,arrowTM,color,dirMag,mag,SDPG_World);
}

void ULinkRenderingComponent::Render(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
#if GEMINI_TODO
	if (Context.View &&	!(View->Family->ShowFlags & SHOW_Paths) )
#endif
	{
		// draw node links - only if not rendering paths
		AUTOnslaughtNodeObjective *node = Cast<AUTOnslaughtNodeObjective>(Owner);
		if ( node )
		{
			for (INT i=0; i<UCONST_MAXNUMLINKS; i++)
			{
				if ( node->LinkedNodes[i] != NULL )
					DrawLineArrow(PDI,node->Location,node->LinkedNodes[i]->Location,FColor(255,0,255,0),8.f);
			}
		}
	}
}

void UUTOnslaughtMapInfo::PostEditChange(UProperty* PropertyThatChanged)
{
	// make sure no two setups are named the same
	for (INT i = 0; i < LinkSetups.Num(); i++)
	{
		for (INT j = i + 1; j < LinkSetups.Num(); j++)
		{
			if (LinkSetups(i).SetupName == LinkSetups(j).SetupName)
			{
				LinkSetups(j).SetupName = FName(*(LinkSetups(j).SetupName.ToString() + TEXT("_Dup")));
			}
		}
	}

	// find the current preview link setup index
	INT SetupIndex = INDEX_NONE;
	for (INT i = 0; i < LinkSetups.Num(); i++)
	{
		if (LinkSetups(i).SetupName == EditorPreviewSetup)
		{
			SetupIndex = i;
			break;
		}
	}

	for (FActorIterator It; It; ++It)
	{
		if (!bEnableEditorPreview || SetupIndex == INDEX_NONE)
		{
			// if we're not previewing a link setup, don't hide anything
			It->bHiddenEdCustom = FALSE;
		}
		else if (LinkSetups(SetupIndex).DeactivatedActors.ContainsItem(*It) || LinkSetups(SetupIndex).DeactivatedGroups.ContainsItem(It->Group))
		{
			// this actor is deactivated, so hide it
			It->bHiddenEdCustom = TRUE;
		}
		else if (It->IsA(AUTOnslaughtNodeObjective::StaticClass()))
		{
			// hide the node if not standalone and no links reference it
			It->bHiddenEdCustom = TRUE;
			if (LinkSetups(SetupIndex).StandaloneNodes.ContainsItem((AUTOnslaughtNodeObjective*)*It))
			{
				It->bHiddenEdCustom = FALSE;
			}
			else
			{
				for (INT i = 0; i < LinkSetups(SetupIndex).NodeLinks.Num(); i++)
				{
					if (LinkSetups(SetupIndex).NodeLinks(i).FromNode == *It || LinkSetups(SetupIndex).NodeLinks(i).ToNode == *It)
					{
						It->bHiddenEdCustom = FALSE;
						break;
					}
				}
			}
		}
		else
		{
			// this actor is active in this link setup
			It->bHiddenEdCustom = FALSE;
		}
	}

	// update all components so the changes to bHiddenEdCustom take effect
	GWorld->UpdateComponents(FALSE);

	Super::PostEditChange(PropertyThatChanged);
}

void UUTOnslaughtMapInfo::CheckForErrors()
{
	Super::CheckForErrors();

	// verify that the link setups affect valid actors
	for (INT i = 0; i < LinkSetups.Num(); i++)
	{
		for (INT j = 0; j < LinkSetups(i).DeactivatedActors.Num(); j++)
		{
			if (LinkSetups(i).DeactivatedActors(j) != NULL && LinkSetups(i).DeactivatedActors(j)->bStatic)
			{
				GWarn->MapCheck_Add(MCTYPE_ERROR, GetOuter(), *FString::Printf(TEXT("Link Setup '%s' contains static actor '%s' in its DeactivatedActors list"), *LinkSetups(i).SetupName.ToString(), *LinkSetups(i).DeactivatedActors(j)->GetName()));
			}
		}

		for (INT j = 0; j < LinkSetups(i).ActivatedActors.Num(); j++)
		{
			if (LinkSetups(i).ActivatedActors(j) != NULL && LinkSetups(i).ActivatedActors(j)->bStatic)
			{
				GWarn->MapCheck_Add(MCTYPE_ERROR, GetOuter(), *FString::Printf(TEXT("Link Setup '%s' contains static actor '%s' in its ActivatedActors list"), *LinkSetups(i).SetupName.ToString(), *LinkSetups(i).ActivatedActors(j)->GetName()));
			}
		}
	}
}
