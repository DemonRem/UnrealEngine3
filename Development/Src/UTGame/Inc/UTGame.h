//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#ifndef _INC_ENGINE

#include "Engine.h"
#include "EngineAIClasses.h"
#include "UnPhysicalMaterial.h" //was: #include "EnginePhysicsClasses.h", but is included in PhysMat
#include "UnTerrain.h"
#include "EngineSequenceClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineParticleClasses.h"
#include "EngineUIPrivateClasses.h"
#include "VoiceInterface.h"

#endif // _INC_ENGINE

#include "GameFrameworkClasses.h"

#include "UTGameClasses.h"


#ifndef _UT_INTRINSIC_CLASSES
#define _UT_INTRINSIC_CLASSES

BEGIN_COMMANDLET(UTLevelCheck,UTGame)
	void StaticInitialize()
	{
		IsEditor = FALSE;
	}
END_COMMANDLET

BEGIN_COMMANDLET(UTReplaceActor,UTGame)
	void StaticInitialize()
	{
		IsEditor = TRUE;
	}
END_COMMANDLET

FLOAT static CalcRequiredAngle(FVector Start, FVector Hit, INT Length)
{
	FLOAT Dist = (Hit - Start).Size();
	FLOAT Angle = 90 - ( appAcos( Dist / FLOAT(Length) ) * 57.2957795);

	return Clamp<FLOAT>(Angle,0,90);
}

inline UBOOL operator !=(const FTakeHitInfo& A, const FTakeHitInfo& B)
{
	return (A.Damage != B.Damage || A.HitLocation != B.HitLocation || A.Momentum != B.Momentum || A.DamageType != B.DamageType || A.HitBone != B.HitBone);
}

#endif
