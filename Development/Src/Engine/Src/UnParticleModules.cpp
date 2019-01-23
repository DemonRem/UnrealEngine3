/*=============================================================================
	UnParticleModules.cpp: Particle module implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnNovodexSupport.h"

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UParticleModuleAccelerationBase);
IMPLEMENT_CLASS(UParticleModuleAttractorBase);
IMPLEMENT_CLASS(UParticleModuleColorBase);
IMPLEMENT_CLASS(UParticleModuleKillBase);
IMPLEMENT_CLASS(UParticleModuleLifetimeBase);
IMPLEMENT_CLASS(UParticleModuleLocationBase);
IMPLEMENT_CLASS(UParticleModuleOrientationBase);
IMPLEMENT_CLASS(UParticleModuleRotationBase);
IMPLEMENT_CLASS(UParticleModuleRotationRateBase);
IMPLEMENT_CLASS(UParticleModuleSizeBase);
IMPLEMENT_CLASS(UParticleModuleSpawnBase);
IMPLEMENT_CLASS(UParticleModuleSubUVBase);
IMPLEMENT_CLASS(UParticleModuleTypeDataBase);
IMPLEMENT_CLASS(UParticleModuleUberBase);
IMPLEMENT_CLASS(UParticleModuleVelocityBase);

IMPLEMENT_CLASS(UDistributionFloatParticleParameter);
IMPLEMENT_CLASS(UDistributionVectorParticleParameter);

/*-----------------------------------------------------------------------------
	Helper functions.
-----------------------------------------------------------------------------*/

static FLinearColor ColorFromVector(const FVector& ColorVec, const FLOAT Alpha)
{
	FLinearColor Color;
	Color.R = ColorVec.X;
	Color.G = ColorVec.Y;
	Color.B = ColorVec.Z;
	Color.A = Alpha;

	return Color;
}


/*-----------------------------------------------------------------------------
	UParticleModule implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModule);

void UParticleModule::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
}

void UParticleModule::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
}

UINT UParticleModule::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return 0;
}

UINT UParticleModule::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return 0;
}

void UParticleModule::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModule::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModule::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	// The default implementation does nothing...
}

/** Fill an array with each Object property that fulfills the FCurveEdInterface interface. */
void UParticleModule::GetCurveObjects(TArray<FParticleCurvePair>& OutCurves)
{
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		// attempt to get a distribution from a random struct property
		UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(*It, (BYTE*)this);
		if (Distribution)
		{
			FParticleCurvePair* NewCurve = new(OutCurves)FParticleCurvePair;
			check(NewCurve);
			NewCurve->CurveObject = Distribution;
			NewCurve->CurveName = It->GetName();
		}
	}
}

/** Add all curve-editable Objects within this module to the curve. */
void UParticleModule::AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup)
{
	TArray<FParticleCurvePair> OutCurves;
	GetCurveObjects(OutCurves);
	for (INT CurveIndex = 0; CurveIndex < OutCurves.Num(); CurveIndex++)
	{
		UObject* Distribution = OutCurves(CurveIndex).CurveObject;
		if (Distribution)
		{
			EdSetup->AddCurveToCurrentTab(Distribution, OutCurves(CurveIndex).CurveName, ModuleEditorColor, bCurvesAsColor);
		}
	}
}

/** Remove all curve-editable Objects within this module from the curve. */
void UParticleModule::RemoveModuleCurvesFromEditor(UInterpCurveEdSetup* EdSetup)
{
	TArray<FParticleCurvePair> OutCurves;
	GetCurveObjects(OutCurves);
	for (INT CurveIndex = 0; CurveIndex < OutCurves.Num(); CurveIndex++)
	{
		UObject* Distribution = OutCurves(CurveIndex).CurveObject;
		if (Distribution)
		{
			EdSetup->RemoveCurve(Distribution);
		}
	}
}

/** Returns true if this Module has any curves that can be pushed into the curve editor. */
UBOOL UParticleModule::ModuleHasCurves()
{
	TArray<FParticleCurvePair> Curves;
	GetCurveObjects(Curves);

	return (Curves.Num() > 0);
}

/** Returns whether any property of this module is displayed in the supplied CurveEd setup. */
UBOOL UParticleModule::IsDisplayedInCurveEd(UInterpCurveEdSetup* EdSetup)
{
	TArray<FParticleCurvePair> Curves;
	GetCurveObjects(Curves);

	for(INT i=0; i<Curves.Num(); i++)
	{
		if(EdSetup->ShowingCurve(Curves(i).CurveObject))
		{
			return true;
		}	
	}

	return false;
}

void UParticleModule::ChangeEditorColor(FColor& Color, UInterpCurveEdSetup* EdSetup)
{
	ModuleEditorColor	= Color;

	TArray<FParticleCurvePair> Curves;
	GetCurveObjects(Curves);

	for (INT TabIndex = 0; TabIndex < EdSetup->Tabs.Num(); TabIndex++)
	{
		FCurveEdTab*	Tab = &(EdSetup->Tabs(TabIndex));
		for (INT CurveIndex = 0; CurveIndex < Tab->Curves.Num(); CurveIndex++)
		{
			FCurveEdEntry* Entry	= &(Tab->Curves(CurveIndex));
			for (INT MyCurveIndex = 0; MyCurveIndex < Curves.Num(); MyCurveIndex++)
			{
				if (Curves(MyCurveIndex).CurveObject == Entry->CurveObject)
				{
					Entry->CurveColor	= Color;
				}
			}
		}
	}
}

void UParticleModule::AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp)
{
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		// attempt to get a distribution from a random struct property
		UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(*It, (BYTE*)this);
		if (Distribution)
		{
		    BYTE	ParamType	= PSPT_None;

			// only handle particle param types
			if (Distribution->IsA(UDistributionFloatParticleParameter::StaticClass()))
			{
			    ParamType	= PSPT_Scalar;
			}
			else 
			if (Distribution->IsA(UDistributionVectorParticleParameter::StaticClass()))
			{
				ParamType	= PSPT_Vector;
			}

			if (ParamType != PSPT_None)
			{
				FName ParamName	= Distribution->GetFName();

				UBOOL	bFound	= FALSE;
				for (INT i = 0; i < PSysComp->InstanceParameters.Num(); i++)
				{
					FParticleSysParam* Param = &(PSysComp->InstanceParameters(i));
					
					if (Param->Name == ParamName)
					{
						bFound	=	TRUE;
						break;
					}
				}

				if (!bFound)
				{
					INT NewParamIndex = PSysComp->InstanceParameters.AddZeroed();
					PSysComp->InstanceParameters(NewParamIndex).Name		= ParamName;
					PSysComp->InstanceParameters(NewParamIndex).ParamType	= ParamType;
					PSysComp->InstanceParameters(NewParamIndex).Actor		= NULL;
				}
			}
		}
	}
}

/**
 *	GenerateLODModuleValues
 *	Default implementation.
 *	Function is intended to generate the required values by multiplying the source module
 *	values by the given percentage.
 *	
 *	@param	SourceModule		The module to use as the source for values
 *	@param	Percentage			The percentage of the source values to set
 *	@param	LODLevel			The LOD level being generated
 *
 *	@return	TRUE	if successful
 *			FALSE	if failed
 */
UBOOL UParticleModule::GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel)
{
	return TRUE;
}

UBOOL UParticleModule::ConvertFloatDistribution(UDistributionFloat* FloatDist, UDistributionFloat* SourceFloatDist, FLOAT Percentage)
{
	FLOAT	Multiplier	= Percentage / 100.0f;

	UDistributionFloatConstant*				DistConstant		= Cast<UDistributionFloatConstant>(FloatDist);
	UDistributionFloatConstantCurve*		DistConstantCurve	= Cast<UDistributionFloatConstantCurve>(FloatDist);
	UDistributionFloatUniform*				DistUniform			= Cast<UDistributionFloatUniform>(FloatDist);
	UDistributionFloatUniformCurve*			DistUniformCurve	= Cast<UDistributionFloatUniformCurve>(FloatDist);
	UDistributionFloatParticleParameter*	DistParticleParam	= Cast<UDistributionFloatParticleParameter>(FloatDist);

	if (DistConstant)
	{
		UDistributionFloatConstant*	SourceConstant	= Cast<UDistributionFloatConstant>(SourceFloatDist);
		check(SourceConstant);
		DistConstant->SetKeyOut(0, 0, SourceConstant->Constant * Multiplier);
	}
	else
	if (DistConstantCurve)
	{
		UDistributionFloatConstantCurve* SourceConstantCurve	= Cast<UDistributionFloatConstantCurve>(SourceFloatDist);
		check(SourceConstantCurve);

		for (INT KeyIndex = 0; KeyIndex < SourceConstantCurve->GetNumKeys(); KeyIndex++)
		{
			DistConstantCurve->CreateNewKey(SourceConstantCurve->GetKeyIn(KeyIndex));
			for (INT SubIndex = 0; SubIndex < SourceConstantCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= SourceConstantCurve->GetKeyOut(SubIndex, KeyIndex);
				DistConstantCurve->SetKeyOut(SubIndex, KeyIndex, Value * Multiplier);
			}
		}
	}
	else
	if (DistUniform)
	{
		DistUniform->SetKeyOut(0, 0, DistUniform->Min * Multiplier);
		DistUniform->SetKeyOut(1, 0, DistUniform->Max * Multiplier);
	}
	else
	if (DistUniformCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistUniformCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistUniformCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistUniformCurve->GetKeyOut(SubIndex, KeyIndex);
				DistUniformCurve->SetKeyOut(SubIndex, KeyIndex, Value * Multiplier);
			}
		}
	}
	else
	if (DistParticleParam)
	{
		DistParticleParam->MinOutput	*= Multiplier;
		DistParticleParam->MaxOutput	*= Multiplier;
	}
	else
	{
		debugf(TEXT("UParticleModule::ConvertFloatDistribution> Invalid distribution?"));
		return FALSE;
	}

	// Safety catch to ensure that the distribution lookup tables get rebuilt...
	FloatDist->bIsDirty = TRUE;
	return TRUE;
}

UBOOL UParticleModule::ConvertVectorDistribution(UDistributionVector* VectorDist, UDistributionVector* SourceVectorDist, FLOAT Percentage)
{
	FLOAT	Multiplier	= Percentage / 100.0f;

	UDistributionVectorConstant*			DistConstant		= Cast<UDistributionVectorConstant>(VectorDist);
	UDistributionVectorConstantCurve*		DistConstantCurve	= Cast<UDistributionVectorConstantCurve>(VectorDist);
	UDistributionVectorUniform*				DistUniform			= Cast<UDistributionVectorUniform>(VectorDist);
	UDistributionVectorUniformCurve*		DistUniformCurve	= Cast<UDistributionVectorUniformCurve>(VectorDist);
	UDistributionVectorParticleParameter*	DistParticleParam	= Cast<UDistributionVectorParticleParameter>(VectorDist);

	if (DistConstant)
	{
		DistConstant->Constant.X *= Multiplier;
		DistConstant->Constant.Y *= Multiplier;
		DistConstant->Constant.Z *= Multiplier;
	}
	else
	if (DistConstantCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistConstantCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistConstantCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistConstantCurve->GetKeyOut(SubIndex, KeyIndex);
				DistConstantCurve->SetKeyOut(SubIndex, KeyIndex, Value * Multiplier);
			}
		}
	}
	else
	if (DistUniform)
	{
		DistUniform->Min.X	*= Multiplier;
		DistUniform->Min.Y	*= Multiplier;
		DistUniform->Min.Z	*= Multiplier;
		DistUniform->Max.X	*= Multiplier;
		DistUniform->Max.Y	*= Multiplier;
		DistUniform->Max.Z	*= Multiplier;
	}
	else
	if (DistUniformCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistUniformCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistUniformCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistUniformCurve->GetKeyOut(SubIndex, KeyIndex);
				DistUniformCurve->SetKeyOut(SubIndex, KeyIndex, Value * Multiplier);
			}
		}
	}
	else
	if (DistParticleParam)
	{
		DistParticleParam->MinOutput.X	*= Multiplier;
		DistParticleParam->MinOutput.Y	*= Multiplier;
		DistParticleParam->MinOutput.Z	*= Multiplier;
		DistParticleParam->MaxOutput.X	*= Multiplier;
		DistParticleParam->MaxOutput.Y	*= Multiplier;
		DistParticleParam->MaxOutput.Z	*= Multiplier;
	}
	else
	{
		debugf(TEXT("UParticleModule::ConvertVectorDistribution> Invalid distribution?"));
		return FALSE;
	}

	// Safety catch to ensure that the distribution lookup tables get rebuilt...
	VectorDist->bIsDirty = TRUE;
	return TRUE;
}

UBOOL UParticleModule::ConvertColorFloatDistribution(UDistributionFloat* FloatDist)
{
	UDistributionFloatConstant*				DistConstant		= Cast<UDistributionFloatConstant>(FloatDist);
	UDistributionFloatConstantCurve*		DistConstantCurve	= Cast<UDistributionFloatConstantCurve>(FloatDist);
	UDistributionFloatUniform*				DistUniform			= Cast<UDistributionFloatUniform>(FloatDist);
	UDistributionFloatUniformCurve*			DistUniformCurve	= Cast<UDistributionFloatUniformCurve>(FloatDist);
	UDistributionFloatParticleParameter*	DistParticleParam	= Cast<UDistributionFloatParticleParameter>(FloatDist);

	if (DistConstant)
	{
		// We need to 'skip' the default case here...
		if (DistConstant->Constant != 1.0f)
		{
			DistConstant->Constant /= 255.9f;
			Clamp<FLOAT>(DistConstant->Constant, 0.0f, 1.0f);
		}
	}
	else
	if (DistConstantCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistConstantCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistConstantCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistConstantCurve->GetKeyOut(SubIndex, KeyIndex);
				Value /= 255.9f;
				Clamp<FLOAT>(Value, 0.0f, 1.0f);
				DistConstantCurve->SetKeyOut(SubIndex, KeyIndex, Value);

				FLOAT ArriveTangent, LeaveTangent;

				DistConstantCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);
				DistConstantCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent / 255.9f, LeaveTangent / 255.9f);
			}
		}

		if (DistConstantCurve->GetNumKeys() > 0)
		{
			EInterpCurveMode InterpMode = (EInterpCurveMode)(DistConstantCurve->GetKeyInterpMode(0));
			switch (InterpMode)
			{
			case CIM_Linear:				break;
			case CIM_CurveAuto:				break;
			case CIM_Constant:				break;
			case CIM_CurveUser:				break;
			case CIM_CurveBreak:			break;
			case CIM_Unknown:				break;
			}
			DistConstantCurve->ConstantCurve.AutoSetTangents();
		}
	}
	else
	if (DistUniform)
	{
		DistUniform->Min /= 255.9f;
		Clamp<FLOAT>(DistUniform->Min, 0.0f, 1.0f);
		DistUniform->Max /= 255.9f;
		Clamp<FLOAT>(DistUniform->Max, 0.0f, 1.0f);
	}
	else
	if (DistUniformCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistUniformCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistUniformCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistUniformCurve->GetKeyOut(SubIndex, KeyIndex);
				Value /= 255.9f;
				Clamp<FLOAT>(Value, 0.0f, 1.0f);
				DistUniformCurve->SetKeyOut(SubIndex, KeyIndex, Value);

				FLOAT ArriveTangent, LeaveTangent;

				DistUniformCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);
				DistUniformCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent / 255.9f, LeaveTangent / 255.9f);
			}
		}

		if (DistUniformCurve->GetNumKeys() > 0)
		{
			EInterpCurveMode InterpMode = (EInterpCurveMode)(DistUniformCurve->GetKeyInterpMode(0));
			switch (InterpMode)
			{
			case CIM_Linear:				break;
			case CIM_CurveAuto:				break;
			case CIM_Constant:				break;
			case CIM_CurveUser:				break;
			case CIM_CurveBreak:			break;
			case CIM_Unknown:				break;
			}
			DistUniformCurve->ConstantCurve.AutoSetTangents();
		}
	}
	else
	if (DistParticleParam)
	{
		DistParticleParam->MinOutput /= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MinOutput, 0.0f, 1.0f);
		DistParticleParam->MaxOutput /= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MaxOutput, 0.0f, 1.0f);
	}
	else
	{
		debugf(TEXT("UParticleModule::ConvertColorFloatDistribution> Invalid distribution?"));
		return FALSE;
	}

	// Safety catch to ensure that the distribution lookup tables get rebuilt...
	FloatDist->bIsDirty = TRUE;
	return TRUE;
}

UBOOL UParticleModule::ConvertColorVectorDistribution(UDistributionVector* VectorDist)
{
	UDistributionVectorConstant*			DistConstant		= Cast<UDistributionVectorConstant>(VectorDist);
	UDistributionVectorConstantCurve*		DistConstantCurve	= Cast<UDistributionVectorConstantCurve>(VectorDist);
	UDistributionVectorUniform*				DistUniform			= Cast<UDistributionVectorUniform>(VectorDist);
	UDistributionVectorUniformCurve*		DistUniformCurve	= Cast<UDistributionVectorUniformCurve>(VectorDist);
	UDistributionVectorParticleParameter*	DistParticleParam	= Cast<UDistributionVectorParticleParameter>(VectorDist);

	if (DistConstant)
	{
		DistConstant->Constant.X /= 255.9f;
		Clamp<FLOAT>(DistConstant->Constant.X, 0.0f, 1.0f);
		DistConstant->Constant.Y /= 255.9f;
		Clamp<FLOAT>(DistConstant->Constant.Y, 0.0f, 1.0f);
		DistConstant->Constant.Z /= 255.9f;
		Clamp<FLOAT>(DistConstant->Constant.Z, 0.0f, 1.0f);
	}
	else
	if (DistConstantCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistConstantCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistConstantCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistConstantCurve->GetKeyOut(SubIndex, KeyIndex);
				Value /= 255.9f;
				Clamp<FLOAT>(Value, 0.0f, 1.0f);
				DistConstantCurve->SetKeyOut(SubIndex, KeyIndex, Value);

				FLOAT ArriveTangent, LeaveTangent;

				DistConstantCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);
				DistConstantCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent / 255.9f, LeaveTangent / 255.9f);
			}
		}

		if (DistConstantCurve->GetNumKeys() > 0)
		{
			EInterpCurveMode InterpMode = (EInterpCurveMode)(DistConstantCurve->GetKeyInterpMode(0));
			switch (InterpMode)
			{
			case CIM_Linear:				break;
			case CIM_CurveAuto:				break;
			case CIM_Constant:				break;
			case CIM_CurveUser:				break;
			case CIM_CurveBreak:			break;
			case CIM_Unknown:				break;
			}
			DistConstantCurve->ConstantCurve.AutoSetTangents();
		}
	}
	else
	if (DistUniform)
	{
		DistUniform->Min.X	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Min.X, 0.0f, 1.0f);
		DistUniform->Min.Y	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Min.Y, 0.0f, 1.0f);
		DistUniform->Min.Z	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Min.Z, 0.0f, 1.0f);
		DistUniform->Max.X	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Max.X, 0.0f, 1.0f);
		DistUniform->Max.Y	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Max.Y, 0.0f, 1.0f);
		DistUniform->Max.Z	/= 255.9f;
		Clamp<FLOAT>(DistUniform->Max.Z, 0.0f, 1.0f);
	}
	else
	if (DistUniformCurve)
	{
		for (INT KeyIndex = 0; KeyIndex < DistUniformCurve->GetNumKeys(); KeyIndex++)
		{
			for (INT SubIndex = 0; SubIndex < DistUniformCurve->GetNumSubCurves(); SubIndex++)
			{
				FLOAT	Value	= DistUniformCurve->GetKeyOut(SubIndex, KeyIndex);
				Value /= 255.9f;
				Clamp<FLOAT>(Value, 0.0f, 1.0f);
				DistUniformCurve->SetKeyOut(SubIndex, KeyIndex, Value);

				FLOAT ArriveTangent, LeaveTangent;

				DistUniformCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);
				DistUniformCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent / 255.9f, LeaveTangent / 255.9f);
			}
		}

		if (DistUniformCurve->GetNumKeys() > 0)
		{
			EInterpCurveMode InterpMode = (EInterpCurveMode)(DistUniformCurve->GetKeyInterpMode(0));
			switch (InterpMode)
			{
			case CIM_Linear:				break;
			case CIM_CurveAuto:				break;
			case CIM_Constant:				break;
			case CIM_CurveUser:				break;
			case CIM_CurveBreak:			break;
			case CIM_Unknown:				break;
			}
			DistUniformCurve->ConstantCurve.AutoSetTangents();
		}
	}
	else
	if (DistParticleParam)
	{
		DistParticleParam->MinOutput.X	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MinOutput.X, 0.0f, 1.0f);
		DistParticleParam->MinOutput.Y	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MinOutput.Y, 0.0f, 1.0f);
		DistParticleParam->MinOutput.Z	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MinOutput.Z, 0.0f, 1.0f);
		DistParticleParam->MaxOutput.X	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MaxOutput.X, 0.0f, 1.0f);
		DistParticleParam->MaxOutput.Y	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MaxOutput.Y, 0.0f, 1.0f);
		DistParticleParam->MaxOutput.Z	/= 255.9f;
		Clamp<FLOAT>(DistParticleParam->MaxOutput.Z, 0.0f, 1.0f);
	}
	else
	{
		debugf(TEXT("UParticleModule::ConvertColorVectorDistribution> Invalid distribution?"));
		return FALSE;
	}

	// Safety catch to ensure that the distribution lookup tables get rebuilt...
	VectorDist->bIsDirty = TRUE;
	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocation implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocation);

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle.Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
		Particle.Location += StartLoc;
	}
}

void UParticleModuleLocation::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	SPAWN_INIT;
	UParticleModuleLocation* LowerModule	= Cast<UParticleModuleLocation>(LowerLODModule);
	FVector	StartA	= StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector	StartB	= LowerModule->StartLocation.GetValue(Owner->EmitterTime, Owner->Component);

	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		StartA = Owner->Component->LocalToWorld.TransformNormal(StartA);
		StartB = Owner->Component->LocalToWorld.TransformNormal(StartB);
	}
	Particle.Location += (StartA * Multiplier) + (StartB * (1.0f - Multiplier));
}

void UParticleModuleLocation::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Draw the location as a wire star
//    FVector Position = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Position = FVector(0.0f);

	// @GEMINI_TODO: Pull this and other editor type code from consoles
	if (StartLocation.Distribution)
	{
		// Nothing else to do if it is constant...
		if (StartLocation.Distribution->IsA(UDistributionVectorUniform::StaticClass()))
		{
			// Draw a box showing the min/max extents
			UDistributionVectorUniform* Uniform = CastChecked<UDistributionVectorUniform>(StartLocation.Distribution);
			
			Position = (Uniform->GetMaxValue() + Uniform->GetMinValue()) / 2.0f;

			DrawWireBox(PDI,FBox(Uniform->GetMinValue(), Uniform->GetMaxValue()), ModuleEditorColor, SDPG_World);
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorConstantCurve::StaticClass()))
		{
			// Draw a box showing the min/max extents
			UDistributionVectorConstantCurve* Curve = CastChecked<UDistributionVectorConstantCurve>(StartLocation.Distribution);

			//Curve->
			Position = StartLocation.GetValue(0.0f, Owner->Component);
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorConstant::StaticClass()))
		{
			Position = StartLocation.GetValue(0.0f, Owner->Component);
		}
	}

	DrawWireStar(PDI,Position, 10.0f, ModuleEditorColor, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationDirect implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationDirect);

void UParticleModuleLocationDirect::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle.Location = Location.GetValue(Particle.RelativeTime, Owner->Component);
	}
	else
	{
		FVector StartLoc	= Location.GetValue(Particle.RelativeTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformFVector(StartLoc);
		Particle.Location	= StartLoc;
	}

	PARTICLE_ELEMENT(FVector, LocOffset);
	LocOffset	= LocationOffset.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Location += LocOffset;
}

void UParticleModuleLocationDirect::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FVector	NewLoc;
		UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
		check(LODLevel);
		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			NewLoc = Location.GetValue(Particle.RelativeTime, Owner->Component);
		}
		else
		{
			FVector Loc			= Location.GetValue(Particle.RelativeTime, Owner->Component);
			Loc = Owner->Component->LocalToWorld.TransformFVector(Loc);
			NewLoc	= Loc;
		}

		FVector	Scale	= ScaleFactor.GetValue(Particle.RelativeTime, Owner->Component);

		PARTICLE_ELEMENT(FVector, LocOffset);
		NewLoc += LocOffset;

		FVector	Diff		 = (NewLoc - Particle.Location);
		FVector	ScaleDiffA	 = Diff * Scale.X;
		FVector	ScaleDiffB	 = Diff * (1.0f - Scale.X);
		Particle.Velocity	 = ScaleDiffA / DeltaTime;
		Particle.Location	+= ScaleDiffB;
	END_UPDATE_LOOP;
}

UINT UParticleModuleLocationDirect::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FVector);
}

void UParticleModuleLocationDirect::SetToSensibleDefaults()
{
}

void UParticleModuleLocationDirect::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange( PropertyThatChanged );
}

void UParticleModuleLocationDirect::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	SPAWN_INIT;

	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleLocationDirect*	LowModule	= Cast<UParticleModuleLocationDirect>(LowerLODModule);

	FVector	LocationA;
	FVector	LocationB;

	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		LocationA	= Location.GetValue(Particle.RelativeTime, Owner->Component);
		LocationB	= LowModule->Location.GetValue(Particle.RelativeTime, Owner->Component);
	}
	else
	{
		LocationA	= Location.GetValue(Particle.RelativeTime, Owner->Component);
		LocationB	= LowModule->Location.GetValue(Particle.RelativeTime, Owner->Component);
		LocationA	= Owner->Component->LocalToWorld.TransformFVector(LocationA);
		LocationB	= Owner->Component->LocalToWorld.TransformFVector(LocationB);
	}

	Particle.Location = (LocationA * Multiplier) + (LocationB * (1.0f - Multiplier));

	PARTICLE_ELEMENT(FVector, LocOffset);
	LocationA	= LocationOffset.GetValue(Owner->EmitterTime, Owner->Component);
	LocationB	= LowModule->LocationOffset.GetValue(Owner->EmitterTime, Owner->Component);
	LocOffset	= (LocationA * Multiplier) + (LocationB * (1.0f - Multiplier));
	Particle.Location += LocOffset;
}

void UParticleModuleLocationDirect::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleModuleLocationDirect*	LowModule	= Cast<UParticleModuleLocationDirect>(LowerLODModule);
	BEGIN_UPDATE_LOOP;
		FVector	LocationA;
		FVector	LocationB;

		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			LocationA	= Location.GetValue(Particle.RelativeTime, Owner->Component);
			LocationB	= LowModule->Location.GetValue(Particle.RelativeTime, Owner->Component);
		}
		else
		{
			LocationA	= Location.GetValue(Particle.RelativeTime, Owner->Component);
			LocationB	= LowModule->Location.GetValue(Particle.RelativeTime, Owner->Component);
			LocationA	= Owner->Component->LocalToWorld.TransformFVector(LocationA);
			LocationB	= Owner->Component->LocalToWorld.TransformFVector(LocationB);
		}
		FVector	NewLoc	= (LocationA * Multiplier) + (LocationB * (1.0f - Multiplier));

		FVector	ScaleA	= ScaleFactor.GetValue(Particle.RelativeTime, Owner->Component);
		FVector	ScaleB	= LowModule->ScaleFactor.GetValue(Particle.RelativeTime, Owner->Component);
		FVector	Scale	= (ScaleA * Multiplier) + (ScaleB * (1.0f - Multiplier));

		PARTICLE_ELEMENT(FVector, LocOffset);
		NewLoc += LocOffset;

		FVector	Diff		 = (NewLoc - Particle.Location);
		FVector	ScaleDiffA	 = Diff * Scale.X;
		FVector	ScaleDiffB	 = Diff * (1.0f - Scale.X);
		Particle.Velocity	 = ScaleDiffA / DeltaTime;
		Particle.Location	+= ScaleDiffB;
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationEmitter implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationEmitter);

//    class UParticleEmitter* LocationEmitter;
void UParticleModuleLocationEmitter::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* LocationEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				LocationEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (LocationEmitterInst == NULL)
	{
		// No source emitter, so we don't spawn??
		return;
	}

	SPAWN_INIT;
		{
			INT Index = 0;

			switch (SelectionMethod)
			{
			case ELESM_Random:
				{
					Index = appTrunc(appSRand() * LocationEmitterInst->ActiveParticles);
					if (Index >= LocationEmitterInst->ActiveParticles)
					{
						Index = LocationEmitterInst->ActiveParticles - 1;
					}
				}
				break;
			case ELESM_Sequential:
				{
					FLocationEmitterInstancePayload* Payload = 
						(FLocationEmitterInstancePayload*)(Owner->GetModuleInstanceData(this));
					if (Payload != NULL)
					{
						Index = ++(Payload->LastSelectedIndex);
						if (Index >= LocationEmitterInst->ActiveParticles)
						{
							Index = 0;
							Payload->LastSelectedIndex = Index;
						}
					}
					else
					{
						// There was an error...
						//@todo.SAS. How to resolve this situation??
					}
				}
				break;
			}
					
			// Grab a particle from the location emitter instance
			FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(Index);
			if (pkParticle)
			{
				if ((pkParticle->RelativeTime == 0.0f) && (pkParticle->Location == FVector(0.0f)))
				{
					Particle.Location = LocationEmitterInst->Component->LocalToWorld.GetOrigin();
				}
				else
				{
					Particle.Location = pkParticle->Location;
				}
				if (InheritSourceVelocity)
				{
					Particle.BaseVelocity	+= pkParticle->Velocity * InheritSourceVelocityScale;
					Particle.Velocity		+= pkParticle->Velocity * InheritSourceVelocityScale;
				}

				if (bInheritSourceRotation)
				{
					// If the ScreenAlignment of the source emitter is PSA_Velocity, 
					// and that of the local is not, then the rotation will NOT be correct!
					Particle.Rotation		+= pkParticle->Rotation * InheritSourceRotationScale;
				}
			}
		}
} 

UINT UParticleModuleLocationEmitter::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return sizeof(FLocationEmitterInstancePayload);
}

void UParticleModuleLocationEmitter::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// Don't need to do anything special for LocationEmitter SpawnEditor.
	Spawn(Owner, Offset, SpawnTime);
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationEmitterDirect implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationEmitterDirect);

void UParticleModuleLocationEmitterDirect::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* LocationEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				LocationEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (LocationEmitterInst == NULL)
	{
		// No source emitter, so we don't spawn??
		return;
	}

	SPAWN_INIT;
		INT Index = Owner->ActiveParticles;

		// Grab a particle from the location emitter instance
		FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(Index);
		if (pkParticle)
		{
			Particle.Location		= pkParticle->Location;
			Particle.OldLocation	= pkParticle->OldLocation;
			Particle.Velocity		= pkParticle->Velocity;
			Particle.RelativeTime	= pkParticle->RelativeTime;
		}
} 

void UParticleModuleLocationEmitterDirect::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* LocationEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				LocationEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (LocationEmitterInst == NULL)
	{
		// No source emitter, so we don't spawn??
		return;
	}

	BEGIN_UPDATE_LOOP;
		{
			// Grab a particle from the location emitter instance
			FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(i);
			if (pkParticle)
			{
				Particle.Location		= pkParticle->Location;
				Particle.OldLocation	= pkParticle->OldLocation;
				Particle.Velocity		= pkParticle->Velocity;
				Particle.RelativeTime	= pkParticle->RelativeTime;
			}
		}
	END_UPDATE_LOOP;
} 

void UParticleModuleLocationEmitterDirect::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// Don't need to do anything special for LocationEmitter SpawnEditor.
	Spawn(Owner, Offset, SpawnTime);
}

void UParticleModuleLocationEmitterDirect::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// Don't need to do anything special for LocationEmitter SpawnEditor.
	Update(Owner, Offset, DeltaTime);
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationPrimitiveBase implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationPrimitiveBase);

void UParticleModuleLocationPrimitiveBase::DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& vUnitDir)
{
	FVector vRand;

	// Grab 3 random numbers for the axes
	vRand.X	= appSRand();
	vRand.Y = appSRand();
	vRand.Z = appSRand();

	// Set the unit dir
	if (Positive_X && Negative_X)
	{
		vUnitDir.X = vRand.X * 2 - 1;
	}
	else
	if (Positive_X)
	{
		vUnitDir.X = vRand.X;
	}
	else
	if (Negative_X)
	{
		vUnitDir.X = -vRand.X;
	}
	else
	{
		vUnitDir.X = 0.0f;
	}

	if (Positive_Y && Negative_Y)
	{
		vUnitDir.Y = vRand.Y * 2 - 1;
	}
	else
	if (Positive_Y)
	{
		vUnitDir.Y = vRand.Y;
	}
	else
	if (Negative_Y)
	{
		vUnitDir.Y = -vRand.Y;
	}
	else
	{
		vUnitDir.Y = 0.0f;
	}

	if (Positive_Z && Negative_Z)
	{
		vUnitDir.Z = vRand.Z * 2 - 1;
	}
	else
	if (Positive_Z)
	{
		vUnitDir.Z = vRand.Z;
	}
	else
	if (Negative_Z)
	{
		vUnitDir.Z = -vRand.Z;
	}
	else
	{
		vUnitDir.Z = 0.0f;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationPrimitiveCylinder implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationPrimitiveCylinder);

void UParticleModuleLocationPrimitiveCylinder::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	INT	RadialIndex0	= 0;	//X
	INT	RadialIndex1	= 1;	//Y
	INT	HeightIndex		= 2;	//Z

	switch (HeightAxis)
	{
	case PMLPC_HEIGHTAXIS_X:
		RadialIndex0	= 1;	//Y
		RadialIndex1	= 2;	//Z
		HeightIndex		= 0;	//X
		break;
	case PMLPC_HEIGHTAXIS_Y:
		RadialIndex0	= 0;	//X
		RadialIndex1	= 2;	//Z
		HeightIndex		= 1;	//Y
		break;
	case PMLPC_HEIGHTAXIS_Z:
		break;
	}

	// Determine the start location for the sphere
	FVector vStartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);

	// Determine the unit direction
	FVector vUnitDir, vUnitDirTemp;
	DetermineUnitDirection(Owner, vUnitDirTemp);
	vUnitDir[RadialIndex0]	= vUnitDirTemp[RadialIndex0];
	vUnitDir[RadialIndex1]	= vUnitDirTemp[RadialIndex1];
	vUnitDir[HeightIndex]	= vUnitDirTemp[HeightIndex];

	FVector vNormalizedDir = vUnitDir;
	vNormalizedDir.Normalize();

	FVector2D vUnitDir2D(vUnitDir[RadialIndex0], vUnitDir[RadialIndex1]);
	FVector2D vNormalizedDir2D = vUnitDir2D.SafeNormal();

	// Determine the position
	FVector vOffset(0.0f);
	FLOAT	fStartRadius	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	fStartHeight	= StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;

	// Always want Z in the [-Height, Height] range
	vOffset[HeightIndex] = vUnitDir[HeightIndex] * fStartHeight;

	vNormalizedDir[RadialIndex0] = vNormalizedDir2D.X;
	vNormalizedDir[RadialIndex1] = vNormalizedDir2D.Y;

	if (SurfaceOnly)
	{
		// Clamp the X,Y to the outer edge...

		if (Abs(vOffset[HeightIndex]) == fStartHeight)
		{
			// On the caps, it can be anywhere within the 'circle'
			vOffset[RadialIndex0] = vUnitDir[RadialIndex0] * fStartRadius;
			vOffset[RadialIndex1] = vUnitDir[RadialIndex1] * fStartRadius;
		}
		else
		{
			// On the sides, it must be on the 'circle'
			vOffset[RadialIndex0] = vNormalizedDir[RadialIndex0] * fStartRadius;
			vOffset[RadialIndex1] = vNormalizedDir[RadialIndex1] * fStartRadius;
		}
	}
	else
	{
		vOffset[RadialIndex0] = vUnitDir[RadialIndex0] * fStartRadius;
		vOffset[RadialIndex1] = vUnitDir[RadialIndex1] * fStartRadius;
	}

	// Clamp to the radius...
	FVector	vMax;
	
	vMax[RadialIndex0]	= Abs(vNormalizedDir[RadialIndex0]) * fStartRadius;
	vMax[RadialIndex1]	= Abs(vNormalizedDir[RadialIndex1]) * fStartRadius;
	vMax[HeightIndex]	= fStartHeight;

	vOffset[RadialIndex0]	= Clamp<FLOAT>(vOffset[RadialIndex0], -vMax[RadialIndex0], vMax[RadialIndex0]);
	vOffset[RadialIndex1]	= Clamp<FLOAT>(vOffset[RadialIndex1], -vMax[RadialIndex1], vMax[RadialIndex1]);
	vOffset[HeightIndex]	= Clamp<FLOAT>(vOffset[HeightIndex],  -vMax[HeightIndex],  vMax[HeightIndex]);

	// Add in the start location
	vOffset[RadialIndex0]	+= vStartLoc[RadialIndex0];
	vOffset[RadialIndex1]	+= vStartLoc[RadialIndex1];
	vOffset[HeightIndex]	+= vStartLoc[HeightIndex];

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		vOffset = Owner->Component->LocalToWorld.TransformNormal(vOffset);
	}
	Particle.Location += vOffset;

	if (Velocity)
	{
		FVector vVelocity;
		vVelocity[RadialIndex0]	= vOffset[RadialIndex0]	- vStartLoc[RadialIndex0];
		vVelocity[RadialIndex1]	= vOffset[RadialIndex1]	- vStartLoc[RadialIndex1];
		vVelocity[HeightIndex]	= vOffset[HeightIndex]	- vStartLoc[HeightIndex];

		if (RadialVelocity)
		{
			vVelocity[HeightIndex]	= 0.0f;
		}
		vVelocity	*= VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);

		Particle.Velocity		+= vVelocity;
		Particle.BaseVelocity	+= vVelocity;
	}
}

void UParticleModuleLocationPrimitiveCylinder::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*							LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleLocationPrimitiveCylinder*	LowModule	= Cast<UParticleModuleLocationPrimitiveCylinder>(LowerLODModule);

	SPAWN_INIT;

	INT	RadialIndex0	= 0;	//X
	INT	RadialIndex1	= 1;	//Y
	INT	HeightIndex		= 2;	//Z

	check(HeightAxis == LowModule->HeightAxis);

	switch (HeightAxis)
	{
	case PMLPC_HEIGHTAXIS_X:
		RadialIndex0	= 1;	//Y
		RadialIndex1	= 2;	//Z
		HeightIndex		= 0;	//X
		break;
	case PMLPC_HEIGHTAXIS_Y:
		RadialIndex0	= 0;	//X
		RadialIndex1	= 2;	//Z
		HeightIndex		= 1;	//Y
		break;
	case PMLPC_HEIGHTAXIS_Z:
		break;
	}

	// Determine the start location for the sphere
	FVector StartLocA = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector StartLocB = LowModule->StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector StartLoc  = (StartLocA * Multiplier) + (StartLocB * (1.0f - Multiplier));

	// Determine the unit direction
	FVector UnitDir, UnitDirTemp;
	DetermineUnitDirection(Owner, UnitDirTemp);
	UnitDir[RadialIndex0]	= UnitDirTemp[RadialIndex0];
	UnitDir[RadialIndex1]	= UnitDirTemp[RadialIndex1];
	UnitDir[HeightIndex]	= UnitDirTemp[HeightIndex];

	FVector NormalizedDir = UnitDir;
	NormalizedDir.Normalize();

	FVector2D UnitDir2D(UnitDir[RadialIndex0], UnitDir[RadialIndex1]);
	FVector2D NormalizedDir2D = UnitDir2D.SafeNormal();

	// Determine the position
	FVector PosOffset(0.0f);

	FLOAT	StartRadiusA	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	StartRadiusB	= LowModule->StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	StartRadius		= (StartRadiusA	* Multiplier) + (StartRadiusB * (1.0f - Multiplier));

	FLOAT	StartHeightA	= StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;
	FLOAT	StartHeightB	= LowModule->StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;
	FLOAT	StartHeight		= (StartHeightA	* Multiplier) + (StartHeightB * (1.0f - Multiplier));

	// Always want Z in the [-Height, Height] range
	PosOffset[HeightIndex] = UnitDir[HeightIndex] * StartHeight;

	NormalizedDir[RadialIndex0] = NormalizedDir2D.X;
	NormalizedDir[RadialIndex1] = NormalizedDir2D.Y;

	if (SurfaceOnly)
	{
		// Clamp the X,Y to the outer edge...

		if (Abs(PosOffset[HeightIndex]) == StartHeight)
		{
			// On the caps, it can be anywhere within the 'circle'
			PosOffset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
			PosOffset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
		}
		else
		{
			// On the sides, it must be on the 'circle'
			PosOffset[RadialIndex0] = NormalizedDir[RadialIndex0] * StartRadius;
			PosOffset[RadialIndex1] = NormalizedDir[RadialIndex1] * StartRadius;
		}
	}
	else
	{
		PosOffset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
		PosOffset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
	}

	// Clamp to the radius...
	FVector	Max;
	
	Max[RadialIndex0]	= Abs(NormalizedDir[RadialIndex0]) * StartRadius;
	Max[RadialIndex1]	= Abs(NormalizedDir[RadialIndex1]) * StartRadius;
	Max[HeightIndex]	= StartHeight;

	PosOffset[RadialIndex0]	= Clamp<FLOAT>(PosOffset[RadialIndex0], -Max[RadialIndex0], Max[RadialIndex0]);
	PosOffset[RadialIndex1]	= Clamp<FLOAT>(PosOffset[RadialIndex1], -Max[RadialIndex1], Max[RadialIndex1]);
	PosOffset[HeightIndex]	= Clamp<FLOAT>(PosOffset[HeightIndex],  -Max[HeightIndex],  Max[HeightIndex]);

	// Add in the start location
	PosOffset[RadialIndex0]	+= StartLoc[RadialIndex0];
	PosOffset[RadialIndex1]	+= StartLoc[RadialIndex1];
	PosOffset[HeightIndex]	+= StartLoc[HeightIndex];

	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		PosOffset = Owner->Component->LocalToWorld.TransformNormal(PosOffset);
	}
	Particle.Location += PosOffset;

	if (Velocity)
	{
		FLOAT	VelocityA;
		FLOAT	VelocityB;
		FVector	Velocity;
		Velocity[RadialIndex0]	= PosOffset[RadialIndex0]	- StartLoc[RadialIndex0];
		Velocity[RadialIndex1]	= PosOffset[RadialIndex1]	- StartLoc[RadialIndex1];
		Velocity[HeightIndex]	= PosOffset[HeightIndex]	- StartLoc[HeightIndex];

		if (RadialVelocity)
		{
			Velocity[HeightIndex]	= 0.0f;
		}

		VelocityA	 = VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
		VelocityB	 = LowModule->VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
		Velocity	*= ((VelocityA * Multiplier) + (VelocityB * (1.0f - Multiplier)));

		Particle.Velocity		+= Velocity;
		Particle.BaseVelocity	+= Velocity;
	}
}

void UParticleModuleLocationPrimitiveCylinder::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Draw the location as a wire star
	DrawWireStar(PDI,Owner->Component->LocalToWorld.GetOrigin(), 10.0f, ModuleEditorColor, SDPG_World);

	FVector Position = FVector(0.0f);

	if (StartLocation.Distribution)
	{
		if (StartLocation.Distribution->IsA(UDistributionVectorConstant::StaticClass()))
		{
			UDistributionVectorConstant* pkConstant = CastChecked<UDistributionVectorConstant>(StartLocation.Distribution);
			Position = pkConstant->Constant;
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorUniform::StaticClass()))
		{
			// Draw at the avg. of the min/max extents
			UDistributionVectorUniform* pkUniform = CastChecked<UDistributionVectorUniform>(StartLocation.Distribution);
			Position = (pkUniform->GetMaxValue() + pkUniform->GetMinValue()) / 2.0f;
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorConstantCurve::StaticClass()))
		{
			// Draw at the avg. of the min/max extents
			UDistributionVectorConstantCurve* pkCurve = CastChecked<UDistributionVectorConstantCurve>(StartLocation.Distribution);

			//pkCurve->
			Position = StartLocation.GetValue(0.0f, Owner->Component);
		}
	}

	// Draw a wire start at the center position
	DrawWireStar(PDI,Position, 10.0f, ModuleEditorColor, SDPG_World);

	FLOAT	fStartRadius	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	fStartHeight	= StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;

	FVector	Axis[3];

	switch (HeightAxis)
	{
	case PMLPC_HEIGHTAXIS_X:
		Axis[0]	= FVector(0, 1, 0);		//Y
		Axis[1]	= FVector(0, 0, 1);		//Z
		Axis[2]	= FVector(1, 0, 0);		//X
		break;
	case PMLPC_HEIGHTAXIS_Y:
		Axis[0]	= FVector(1, 0, 0);		//X
		Axis[1]	= FVector(0, 0, 1);		//Z
		Axis[2]	= FVector(0, 1, 0);		//Y
		break;
	case PMLPC_HEIGHTAXIS_Z:
		Axis[0]	= FVector(1, 0, 0);		//X
		Axis[1]	= FVector(0, 1, 0);		//Y
		Axis[2]	= FVector(0, 0, 1);		//Z
		break;
	}
	DrawWireCylinder(PDI,Position, Axis[0], Axis[1], Axis[2], 
		ModuleEditorColor, fStartRadius, fStartHeight, 16, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleLocationPrimitiveSphere implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLocationPrimitiveSphere);

void UParticleModuleLocationPrimitiveSphere::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	// Determine the start location for the sphere
	FVector vStartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);

	// Determine the unit direction
	FVector vUnitDir;
	DetermineUnitDirection(Owner, vUnitDir);

	FVector vNormalizedDir = vUnitDir;
	vNormalizedDir.Normalize();

	// If we want to cover just the surface of the sphere...
	if (SurfaceOnly)
	{
		vUnitDir.Normalize();
	}

	// Determine the position
	FLOAT	fStartRadius	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FVector vStartRadius	= FVector(fStartRadius);
	FVector vOffset			= vUnitDir * vStartRadius;

	// Clamp to the radius...
	FVector	vMax;
	
	vMax.X	= Abs(vNormalizedDir.X) * fStartRadius;
	vMax.Y	= Abs(vNormalizedDir.Y) * fStartRadius;
	vMax.Z	= Abs(vNormalizedDir.Z) * fStartRadius;

	if (Positive_X || Negative_X)
	{
		vOffset.X = Clamp<FLOAT>(vOffset.X, -vMax.X, vMax.X);
	}
	else
	{
		vOffset.X = 0.0f;
	}
	if (Positive_Y || Negative_Y)
	{
		vOffset.Y = Clamp<FLOAT>(vOffset.Y, -vMax.Y, vMax.Y);
	}
	else
	{
		vOffset.Y = 0.0f;
	}
	if (Positive_Z || Negative_Z)
	{
		vOffset.Z = Clamp<FLOAT>(vOffset.Z, -vMax.Z, vMax.Z);
	}
	else
	{
		vOffset.Z = 0.0f;
	}

	vOffset += vStartLoc;

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		vOffset = Owner->Component->LocalToWorld.TransformNormal(vOffset);
	}
	Particle.Location += vOffset;

	if (Velocity)
	{
		FVector vVelocity		 = (vOffset - vStartLoc) * VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
		Particle.Velocity		+= vVelocity;
		Particle.BaseVelocity	+= vVelocity;
	}
}

void UParticleModuleLocationPrimitiveSphere::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleLocationPrimitiveSphere*	LowModule	= Cast<UParticleModuleLocationPrimitiveSphere>(LowerLODModule);

	SPAWN_INIT;

	// Determine the start location for the sphere
	FVector StartLocA	= StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector StartLocB	= LowModule->StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	FVector StartLoc	= (StartLocA * Multiplier) + (StartLocB * (1.0f - Multiplier));

	// Determine the unit direction
	FVector UnitDir;
	DetermineUnitDirection(Owner, UnitDir);

	FVector NormalizedDir = UnitDir;
	NormalizedDir.Normalize();

	// If we want to cover just the surface of the sphere...
	if (SurfaceOnly)
	{
		UnitDir.Normalize();
	}

	// Determine the position
	FLOAT	StartRadiusA	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	StartRadiusB	= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	StartRadius		= (StartRadiusA * Multiplier) + (StartRadiusB * (1.0f - Multiplier));

	FVector LocOffset		= UnitDir * StartRadius;

	// Clamp to the radius...
	FVector	Max;
	
	Max.X	= Abs(NormalizedDir.X) * StartRadius;
	Max.Y	= Abs(NormalizedDir.Y) * StartRadius;
	Max.Z	= Abs(NormalizedDir.Z) * StartRadius;

	if (Positive_X || Negative_X)
	{
		LocOffset.X = Clamp<FLOAT>(LocOffset.X, -Max.X, Max.X);
	}
	else
	{
		LocOffset.X = 0.0f;
	}
	if (Positive_Y || Negative_Y)
	{
		LocOffset.Y = Clamp<FLOAT>(LocOffset.Y, -Max.Y, Max.Y);
	}
	else
	{
		LocOffset.Y = 0.0f;
	}
	if (Positive_Z || Negative_Z)
	{
		LocOffset.Z = Clamp<FLOAT>(LocOffset.Z, -Max.Z, Max.Z);
	}
	else
	{
		LocOffset.Z = 0.0f;
	}

	LocOffset += StartLoc;

	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		LocOffset = Owner->Component->LocalToWorld.TransformNormal(LocOffset);
	}
	Particle.Location += LocOffset;

	if (Velocity)
	{
		FLOAT	VelocityScaleA	 = VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	VelocityScaleB	 = LowModule->VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
		FVector Velocity		 = (LocOffset - StartLoc) * ((VelocityScaleA * Multiplier) + (VelocityScaleB * (1.0f - Multiplier)));

		Particle.Velocity		+= Velocity;
		Particle.BaseVelocity	+= Velocity;
	}
}

void UParticleModuleLocationPrimitiveSphere::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Draw the location as a wire star
	DrawWireStar(PDI,Owner->Component->LocalToWorld.GetOrigin(), 10.0f, ModuleEditorColor, SDPG_World);

	FVector Position = FVector(0.0f);

	if (StartLocation.Distribution)
	{
		if (StartLocation.Distribution->IsA(UDistributionVectorConstant::StaticClass()))
		{
			UDistributionVectorConstant* pkConstant = CastChecked<UDistributionVectorConstant>(StartLocation.Distribution);
			Position = pkConstant->Constant;
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorUniform::StaticClass()))
		{
			// Draw at the avg. of the min/max extents
			UDistributionVectorUniform* pkUniform = CastChecked<UDistributionVectorUniform>(StartLocation.Distribution);
			Position = (pkUniform->GetMaxValue() + pkUniform->GetMinValue()) / 2.0f;
		}
		else
		if (StartLocation.Distribution->IsA(UDistributionVectorConstantCurve::StaticClass()))
		{
			// Draw at the avg. of the min/max extents
			UDistributionVectorConstantCurve* pkCurve = CastChecked<UDistributionVectorConstantCurve>(StartLocation.Distribution);

			//pkCurve->
			Position = StartLocation.GetValue(0.0f, Owner->Component);
		}
	}

	// Draw a wire start at the center position
	DrawWireStar(PDI,Position, 10.0f, ModuleEditorColor, SDPG_World);

	FLOAT	fRadius		= StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
	INT		iNumSides	= 32;
	FVector	vAxis[3];

	vAxis[0]	= Owner->Component->LocalToWorld.GetAxis(0);
	vAxis[1]	= Owner->Component->LocalToWorld.GetAxis(1);
	vAxis[2]	= Owner->Component->LocalToWorld.GetAxis(2);

	DrawCircle(PDI,Position, vAxis[0], vAxis[1], ModuleEditorColor, fRadius, iNumSides, SDPG_World);
	DrawCircle(PDI,Position, vAxis[0], vAxis[2], ModuleEditorColor, fRadius, iNumSides, SDPG_World);
	DrawCircle(PDI,Position, vAxis[1], vAxis[2], ModuleEditorColor, fRadius, iNumSides, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleOrientationAxisLock implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleOrientationAxisLock);

//    BYTE LockAxisFlags;
//    FVector LockAxis;
void UParticleModuleOrientationAxisLock::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
}

void UParticleModuleOrientationAxisLock::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
}

void UParticleModuleOrientationAxisLock::PostEditChange(UProperty* PropertyThatChanged)
{
	UObject* OuterObj = GetOuter();
	check(OuterObj);
	UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
	if (LODLevel)
	{
		// The outer is incorrect - warn the user and handle it
		warnf(NAME_Warning, TEXT("UParticleModuleOrientationAxisLock has an incorrect outer... run FixupEmitters on package %s"),
			*(OuterObj->GetOutermost()->GetPathName()));
		OuterObj = LODLevel->GetOuter();
		UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
		check(Emitter);
		OuterObj = Emitter->GetOuter();
	}
	UParticleSystem* PartSys = PartSys = CastChecked<UParticleSystem>(OuterObj);

	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("LockAxisFlags")))
		{
			PartSys->PostEditChange(PropertyThatChanged);
		}
	}
	Super::PostEditChange( PropertyThatChanged );
}

void UParticleModuleOrientationAxisLock::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleOrientationAxisLock::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleOrientationAxisLock::SetLockAxis(EParticleAxisLock eLockFlags)
{
	LockAxisFlags = eLockFlags;
}

/*-----------------------------------------------------------------------------
	UParticleModuleRequired implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleRequired);

void UParticleModuleRequired::PostEditChange(UProperty* PropertyThatChanged)
{
	if (SubImages_Horizontal < 1)
	{
		SubImages_Horizontal = 1;
	}
	if (SubImages_Vertical < 1)
	{
		SubImages_Vertical = 1;
	}
	for (INT Index = 0; Index < BurstList.Num(); Index++)
	{
		FParticleBurst* Burst = &(BurstList(Index));

		// Clamp them to positive numbers...
		Burst->Count = Max<INT>(0, Burst->Count);
		Burst->CountLow = Max<INT>(0, Burst->CountLow);
		Burst->CountLow = Min<INT>(Burst->Count, Burst->CountLow);
	}

	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("MaxDrawCount")))
		{
			if (MaxDrawCount >= 0)
			{
				bUseMaxDrawCount = TRUE;
			}
			else
			{
				bUseMaxDrawCount = FALSE;
			}
		}
	}

	Super::PostEditChange( PropertyThatChanged );
}

void UParticleModuleRequired::PostLoad()
{
	Super::PostLoad();

	if (SubImages_Horizontal < 1)
	{
		SubImages_Horizontal = 1;
	}
	if (SubImages_Vertical < 1)
	{
		SubImages_Vertical = 1;
	}
}

void UParticleModuleRequired::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
}

void UParticleModuleRequired::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
}

void UParticleModuleRequired::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleRequired::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

UBOOL UParticleModuleRequired::GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel)
{
	// Convert the module values
	UParticleModuleRequired*	RequiredSource	= Cast<UParticleModuleRequired>(SourceModule);
	if (!RequiredSource)
	{
		return FALSE;
	}

	UBOOL bResult	= TRUE;

	Material = RequiredSource->Material;
	ScreenAlignment = RequiredSource->ScreenAlignment;

	//bUseLocalSpace
	//bKillOnDeactivate
	//bKillOnCompleted
	//EmitterDuration
	//EmitterLoops
	//SpawnRate
//	Particle_ModifyVectorDistribution(RequiredSource->SpawnRate, Percentage / 100.0f);
	// @GEMINI_TODO: Make sure these functions are never called on console, or when the UDistributions are missing
	if (ConvertFloatDistribution(SpawnRate.Distribution, RequiredSource->SpawnRate.Distribution, Percentage) == FALSE)
	{
		bResult	= FALSE;
	}

	//ParticleBurstMethod
	//BurstList
	check(BurstList.Num() == RequiredSource->BurstList.Num());
	for (INT BurstIndex = 0; BurstIndex < RequiredSource->BurstList.Num(); BurstIndex++)
	{
		FParticleBurst* SourceBurst	= &(RequiredSource->BurstList(BurstIndex));
		FParticleBurst* Burst		= &(BurstList(BurstIndex));

		Burst->Time		= SourceBurst->Time;
		// Don't drop below 1...
		if (Burst->Count > 0)
		{
			Burst->Count	= appTrunc(SourceBurst->Count * (Percentage / 100.0f));
			if (Burst->Count == 0)
			{
				Burst->Count	= 1;
			}
		}
	}
	//InterpolationMethod
	//SubImages_Horizontal
	//SubImages_Vertical
	//bScaleUV
	//RandomImageTime
	//RandomImageChanges
	//bDirectUV
	//SubUVDataOffset
	//EmitterRenderMode
	//EmitterEditorColor

	return bResult;
}

/*-----------------------------------------------------------------------------
	UParticleModuleMeshRotation implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleMeshRotation);

void UParticleModuleMeshRotation::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
		FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
		if (MeshInst)
		{
			FVector Rotation = StartRotation.GetValue(Owner->EmitterTime, Owner->Component);

			if (bInheritParent)
			{
				FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
				FVector		ParentAffectedRotation	= Rotator.Euler();
				Rotation.X	+= ParentAffectedRotation.X / 360.0f;
				Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
				Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
			}

			FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
			PayloadData->Rotation.X	+= Rotation.X * 360.0f;
			PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
			PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
		}
}

void UParticleModuleMeshRotation::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleMeshRotation*	LowModule	= Cast<UParticleModuleMeshRotation>(LowerLODModule);

	SPAWN_INIT;
		FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
		if (MeshInst)
		{
			FVector RotationA	= StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
			FVector RotationB	= LowModule->StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
			FVector Rotation	= (RotationA * Multiplier) + (RotationB * (1.0f - Multiplier));

			if (bInheritParent)
			{
				FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
				FVector		ParentAffectedRotation	= Rotator.Euler();
				Rotation.X	+= ParentAffectedRotation.X / 360.0f;
				Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
				Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
			}

			FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
			PayloadData->Rotation.X	+= Rotation.X * 360.0f;
			PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
			PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
		}
}

/*-----------------------------------------------------------------------------
	UParticleModuleMeshRotationRate implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleMeshRotationRate);

void UParticleModuleMeshRotationRate::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
		FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
		if (MeshInst)
		{
			FVector StartRate = StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);// * ((FLOAT)PI/180.f);
			FVector StartValue;
			StartValue.X = StartRate.X * 360.0f;
			StartValue.Y = StartRate.Y * 360.0f;
			StartValue.Z = StartRate.Z * 360.0f;

			FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
			PayloadData->RotationRateBase	+= StartValue;
			PayloadData->RotationRate		+= StartValue;
		}
}

void UParticleModuleMeshRotationRate::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleMeshRotationRate*	LowModule	= Cast<UParticleModuleMeshRotationRate>(LowerLODModule);

	SPAWN_INIT;
		FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
		if (MeshInst)
		{
			FVector StartRateA	= StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);// * ((FLOAT)PI/180.f);
			FVector StartRateB	= LowModule->StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);// * ((FLOAT)PI/180.f);
			FVector	StartRate	= (StartRateA * Multiplier) + (StartRateB * (1.0f - Multiplier));

			FVector StartValue;
			StartValue.X = StartRate.X * 360.0f;
			StartValue.Y = StartRate.Y * 360.0f;
			StartValue.Z = StartRate.Z * 360.0f;

			FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
			PayloadData->RotationRateBase	+= StartValue;
			PayloadData->RotationRate		+= StartValue;
		}
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleMeshRotationRate::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorUniform* StartRotationRateDist = Cast<UDistributionVectorUniform>(StartRotationRate.Distribution);
	if (StartRotationRateDist)
	{
		StartRotationRateDist->Min = FVector(0.0f,0.0f,0.0f);
		StartRotationRateDist->Max = FVector(1.0f,1.0f,1.0f);
		StartRotationRateDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleMeshRotationRateMultiplyLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleMeshRotationRateMultiplyLife);

void UParticleModuleMeshRotationRateMultiplyLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if ((MeshInst == NULL) || (MeshInst->MeshRotationOffset == 0))
	{
		return;
	}

	SPAWN_INIT;
	{
		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		FVector RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		PayloadData->RotationRate *= RateScale;
	}
}

void UParticleModuleMeshRotationRateMultiplyLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if ((MeshInst == NULL) || (MeshInst->MeshRotationOffset == 0))
	{
		return;
	}

	BEGIN_UPDATE_LOOP;
	{
		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		FVector RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		PayloadData->RotationRate *= RateScale;
	}
	END_UPDATE_LOOP;
}

void UParticleModuleMeshRotationRateMultiplyLife::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if ((MeshInst == NULL) || (MeshInst->MeshRotationOffset == 0))
	{
		return;
	}

	UParticleModuleMeshRotationRateMultiplyLife* LowLOD = CastChecked<UParticleModuleMeshRotationRateMultiplyLife>(LowerLODModule);

	SPAWN_INIT;
	{
		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		FVector RateScaleA = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector RateScaleB = LowLOD->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		PayloadData->RotationRate *= (RateScaleA * Multiplier) + (RateScaleB * (1.0f - Multiplier));
	}
}

void UParticleModuleMeshRotationRateMultiplyLife::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if ((MeshInst == NULL) || (MeshInst->MeshRotationOffset == 0))
	{
		return;
	}

	UParticleModuleMeshRotationRateMultiplyLife* LowLOD = CastChecked<UParticleModuleMeshRotationRateMultiplyLife>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
	{
		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		FVector RateScaleA = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector RateScaleB = LowLOD->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		PayloadData->RotationRate *= (RateScaleA * Multiplier) + (RateScaleB * (1.0f - Multiplier));
	}
	END_UPDATE_LOOP;
}

void UParticleModuleMeshRotationRateMultiplyLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorUniform* LifeMultiplierDist = Cast<UDistributionVectorUniform>(LifeMultiplier.Distribution);
	if (LifeMultiplierDist)
	{
		LifeMultiplierDist->Min = FVector(0.0f,0.0f,0.0f);
		LifeMultiplierDist->Max = FVector(1.0f,1.0f,1.0f);
		LifeMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleRotation implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleRotation);

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	Particle.Rotation += (PI/180.f) * 360.0f * StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
}

void UParticleModuleRotation::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleRotation*	LowModule	= Cast<UParticleModuleRotation>(LowerLODModule);

	SPAWN_INIT;
	FLOAT	RotationA	 = StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	RotationB	 = LowModule->StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Rotation	+= (PI/180.f) * 360.0f * ((RotationA * Multiplier) + (RotationB * (1.0f - Multiplier)));
}

/*-----------------------------------------------------------------------------
	UParticleModuleRotationRate implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleRotationRate);

void UParticleModuleRotationRate::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FLOAT StartRotRate = (PI/180.f) * 360.0f * StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.RotationRate += StartRotRate;
	Particle.BaseRotationRate += StartRotRate;
}

void UParticleModuleRotationRate::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleRotationRate*	LowModule	= Cast<UParticleModuleRotationRate>(LowerLODModule);

	SPAWN_INIT;
	FLOAT StartRotRateA	= StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT StartRotRateB	= LowModule->StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT StartRotRate = (PI/180.f) * 360.0f * ((StartRotRateA * Multiplier)  + (StartRotRateB * (1.0f - Multiplier)));
	Particle.RotationRate		+= StartRotRate;
	Particle.BaseRotationRate	+= StartRotRate;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleRotationRate::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	StartRotationRate.Distribution = Cast<UDistributionFloatUniform>(StaticConstructObject(UDistributionFloatUniform::StaticClass(), this));
	UDistributionFloatUniform* StartRotationRateDist = Cast<UDistributionFloatUniform>(StartRotationRate.Distribution);
	if (StartRotationRateDist)
	{
		StartRotationRateDist->Min = 0.0f;
		StartRotationRateDist->Max = 1.0f;
		StartRotationRateDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleRotationOverLifetime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleRotationOverLifetime);

void UParticleModuleRotationOverLifetime::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FLOAT Rotation = RotationOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		// For now, we are just using the X-value
		if (Scale)
		{
			Particle.Rotation	*= Rotation * (PI/180.f) * 360.0f;
		}
		else
		{
			Particle.Rotation	+= Rotation * (PI/180.f) * 360.0f;
		}
	END_UPDATE_LOOP;
}

void UParticleModuleRotationOverLifetime::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleRotationOverLifetime*	LowModule	= Cast<UParticleModuleRotationOverLifetime>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FLOAT RotationA	= RotationOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT RotationB	= LowModule->RotationOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT Rotation	= (RotationA * Multiplier) + (RotationB * (1.0f - Multiplier));
		// For now, we are just using the X-value
		if (Scale)
		{
			Particle.Rotation	*= Rotation * (PI/180.f) * 360.0f;
		}
		else
		{
			Particle.Rotation	+= Rotation * (PI/180.f) * 360.0f;
		}
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleSize implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSize);

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;
}

void UParticleModuleSize::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*		LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSize*	LowModule	= Cast<UParticleModuleSize>(LowerLODModule);

	SPAWN_INIT;
	FVector SizeA		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector SizeB		 = LowModule->StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Size		 = (SizeA * Multiplier)+ (SizeB * (1.0f - Multiplier));
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeMultiplyVelocity implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeMultiplyVelocity);

void UParticleModuleSizeMultiplyVelocity::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector SizeScale = VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component) * Particle.Velocity.Size();
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyVelocity::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FVector SizeScale = VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component) * Particle.Velocity.Size();
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}
	END_UPDATE_LOOP;
}

void UParticleModuleSizeMultiplyVelocity::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSizeMultiplyVelocity*	LowModule	= Cast<UParticleModuleSizeMultiplyVelocity>(LowerLODModule);

	SPAWN_INIT;
	FVector SizeScaleA	= VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScaleB	= LowModule->VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScale	= ((SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier))) * Particle.Velocity.Size();
	if (MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if (MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if (MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyVelocity::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSizeMultiplyVelocity*	LowModule	= Cast<UParticleModuleSizeMultiplyVelocity>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector SizeScaleA	= VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScaleB	= LowModule->VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScale	= ((SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier))) * Particle.Velocity.Size();

		if (MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if (MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if (MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeMultiplyVelocity::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstant* VelocityMultiplierDist = Cast<UDistributionVectorConstant>(VelocityMultiplier.Distribution);
	if (VelocityMultiplierDist)
	{
		VelocityMultiplierDist->Constant = FVector(1.0f,1.0f,1.0f);
		VelocityMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeMultiplyLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeMultiplyLife);

void UParticleModuleSizeMultiplyLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	const FRawDistribution* FastDistribution = LifeMultiplier.GetFastRawDistribution();
	if( FastDistribution && MultiplyX && MultiplyY && MultiplyZ)
	{
		FVector SizeScale;
		// fast path
		BEGIN_UPDATE_LOOP;
			FastDistribution->GetValue3None(Particle.RelativeTime, &SizeScale.X);
			Particle.Size *= SizeScale;
		END_UPDATE_LOOP;
	}
	else
	{
		BEGIN_UPDATE_LOOP;
			FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
			if(MultiplyX)
			{
				Particle.Size.X *= SizeScale.X;
			}
			if(MultiplyY)
			{
				Particle.Size.Y *= SizeScale.Y;
			}
			if(MultiplyZ)
			{
				Particle.Size.Z *= SizeScale.Z;
			}
		END_UPDATE_LOOP;
	}
}

void UParticleModuleSizeMultiplyLife::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSizeMultiplyLife*	LowModule	= Cast<UParticleModuleSizeMultiplyLife>(LowerLODModule);

	SPAWN_INIT;
	FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
	if (MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if (MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if (MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyLife::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSizeMultiplyLife*	LowModule	= Cast<UParticleModuleSizeMultiplyLife>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));

		if (MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if (MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if (MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeMultiplyLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	LifeMultiplier.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* LifeMultiplierDist = Cast<UDistributionVectorConstantCurve>(LifeMultiplier.Distribution);
	if (LifeMultiplierDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = LifeMultiplierDist->CreateNewKey(Key * 1.0f);
			for (INT SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				LifeMultiplierDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
			}
		}
		LifeMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeScale implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeScale);

void UParticleModuleSizeScale::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FVector ScaleFactor = SizeScale.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Size = Particle.BaseSize * ScaleFactor;
	END_UPDATE_LOOP;
}

void UParticleModuleSizeScale::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSizeScale*	LowModule	= Cast<UParticleModuleSizeScale>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector ScaleFactorA	= SizeScale.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ScaleFactorB	= LowModule->SizeScale.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ScaleFactor		= (ScaleFactorA * Multiplier) + (ScaleFactorB * (1.0f - Multiplier));
		Particle.Size			= Particle.BaseSize * ScaleFactor;
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeScale::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstant* SizeScaleDist = Cast<UDistributionVectorConstant>(SizeScale.Distribution);
	if (SizeScaleDist)
	{
		SizeScaleDist->Constant = FVector(1.0f,1.0f,1.0f);
		SizeScaleDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSpawnPerUnit implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSpawnPerUnit);

/**
 *	Called on a particle that is freshly spawned by the emitter.
 *	
 *	@param	Owner		The FParticleEmitterInstance that spawned the particle.
 *	@param	Offset		The modules offset into the data payload of the particle.
 *	@param	SpawnTime	The time of the spawn.
 */
void UParticleModuleSpawnPerUnit::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FParticleSpawnPerUnitInstancePayload, SPUPayload);
		SPUPayload.CurrentDistanceTravelled = 0.0f;
	}
}

/**
 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *
 *	@return UINT		The number fo bytes the module needs per emitter instance.
 */
UINT UParticleModuleSpawnPerUnit::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleSpawnPerUnitInstancePayload);
}

/**
 *	In the editor, called on a particle that is freshly spawned by the emitter.
 *	Is responsible for performing the intepolation between LOD levels for realtime 
 *	previewing.
 *	
 *	@param	Owner			The FParticleEmitterInstance that spawned the particle.
 *	@param	Offset			The modules offset into the data payload of the particle.
 *	@param	SpawnTime		The time of the spawn.
 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
 *	@param	Multiplier		The interpolation alpha value to use.
 */
void UParticleModuleSpawnPerUnit::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleModuleSpawnPerUnit::Spawn(Owner, Offset, SpawnTime);
}

/**
 *	Retrieve the spawn amount this module is contributing.
 *	Note that if multiple Spawn-specific modules are present, if any one
 *	of them ignores the SpawnRate processing it will be ignored.
 *
 *	@param	Owner		The particle emitter instance that is spawning.
 *	@param	Offset		The offset into the particle payload for the module.
 *	@param	OldLeftover	The bit of timeslice left over from the previous frame.
 *	@param	DeltaTime	The time that has expired since the last frame.
 *	@param	Number		The number of particles to spawn. (OUTPUT)
 *	@param	Rate		The spawn rate of the module. (OUTPUT)
 *
 *	@return	UBOOL		FALSE if the SpawnRate should be ignored.
 *						TRUE if the SpawnRate should still be processed.
 */
UBOOL UParticleModuleSpawnPerUnit::GetSpawnAmount(FParticleEmitterInstance* Owner, 
	INT Offset, FLOAT OldLeftover, FLOAT DeltaTime, INT& Number, FLOAT& Rate)
{
	check(Owner);

	UBOOL bMoved = FALSE;
	FParticleSpawnPerUnitInstancePayload* SPUPayload = NULL;
	FLOAT NewTravelLeftover = 0.0f;

	FLOAT ParticlesPerUnit = SpawnPerUnit.GetValue(Owner->EmitterTime, Owner->Component) / UnitScalar;
	if (ParticlesPerUnit >= 0.0f)
	{
		FLOAT LeftoverTravel = 0.0f;
		BYTE* InstData = Owner->GetModuleInstanceData(this);
		if (InstData)
		{
			SPUPayload = (FParticleSpawnPerUnitInstancePayload*)InstData;
			LeftoverTravel = SPUPayload->CurrentDistanceTravelled;
		}
		
		// Calculate movement delta over last frame, include previous remaining delta
		FVector TravelDirection = Owner->Location - Owner->OldLocation;
		// Calculate distance traveled
		FLOAT TravelDistance = TravelDirection.Size();

		if (TravelDistance > 0.0f)
		{
			if (TravelDistance > (MovementTolerance * UnitScalar))
			{
				bMoved = TRUE;
			}

			// Normalize direction for use later
			TravelDirection.Normalize();

			// Calculate number of particles to emit
			FLOAT NewLeftover = (TravelDistance + LeftoverTravel) * ParticlesPerUnit;
			Number = appFloor(NewLeftover);
			Rate = Number / DeltaTime;
			NewTravelLeftover = (TravelDistance + LeftoverTravel) - (Number * UnitScalar);
			if (SPUPayload)
			{
				SPUPayload->CurrentDistanceTravelled = Max<FLOAT>(0.0f, NewTravelLeftover);
			}

		}
		else
		{
			Number = 0;
			Rate = 0.0f;
		}
	}
	else
	{
		Number = 0;
		Rate = 0.0f;
	}

	if (bIgnoreSpawnRateWhenMoving == TRUE)
	{
		if (bMoved == TRUE)
		{
			return FALSE;
		}
		return TRUE;
	}

	return bProcessSpawnRate;
}

/**
 *	Retrieve the spawn amount this module is contributing.
 *	Note that if multiple Spawn-specific modules are present, if any one
 *	of them ignores the SpawnRate processing it will be ignored.
 *	Editor-only version - this will interpolate between the module and the provided LowerLODModule
 *	using the value of multiplier.
 *
 *	@param	Owner			The particle emitter instance that is spawning.
 *	@param	Offset			The offset into the particle payload for the module.
 *	@param	LowerLODModule	The lower LOD module to interpolate with.
 *	@param	Multiplier		The interpolation 'alpha' value.
 *	@param	OldLeftover		The bit of timeslice left over from the previous frame.
 *	@param	DeltaTime		The time that has expired since the last frame.
 *	@param	Number			The number of particles to spawn. (OUTPUT)
 *	@param	Rate			The spawn rate of the module. (OUTPUT)
 *
 *	@return	UBOOL			FALSE if the SpawnRate should be ignored.
 *							TRUE if the SpawnRate should still be processed.
 */
UBOOL UParticleModuleSpawnPerUnit::GetSpawnAmount(FParticleEmitterInstance* Owner, 
	INT Offset, UParticleModule* LowerLODModule, FLOAT Multiplier, FLOAT OldLeftover, 
	FLOAT DeltaTime, INT& Number, FLOAT& Rate)
{
	check(Owner);

	UParticleModuleSpawnPerUnit* LowLOD = Cast<UParticleModuleSpawnPerUnit>(LowerLODModule);
	UBOOL bMoved = FALSE;
	FLOAT NewTravelLeftover = 0.0;

	FLOAT HighParticlesPerUnit = SpawnPerUnit.GetValue(Owner->EmitterTime, Owner->Component) / UnitScalar;
	FLOAT LowParticlesPerUnit = LowLOD->SpawnPerUnit.GetValue(Owner->EmitterTime, Owner->Component) / LowLOD->UnitScalar;
	FLOAT ParticlesPerUnit = (HighParticlesPerUnit * Multiplier) + (LowParticlesPerUnit * (1.0f - Multiplier));
	if (ParticlesPerUnit >= 0.0f)
	{
		// Calculate movement delta over last frame, include previous remaining delta
		FVector TravelDirection = Owner->Location - Owner->OldLocation;
		// Calculate distance traveled
		FLOAT TravelDistance = TravelDirection.Size();
		if (TravelDistance > 0.0f)
		{
			if (TravelDistance > (MovementTolerance * UnitScalar))
			{
				bMoved = TRUE;
			}

			// Normalize direction for use later
			TravelDirection.Normalize();

			// Calculate number of particles to emit
			FLOAT NewLeftover = OldLeftover + TravelDistance * ParticlesPerUnit;
			Number = appFloor(NewLeftover);
			Rate = Number / DeltaTime;
		}
		else
		{
			Number = 0;
			Rate = 0.0f;
		}
	}
	else
	{
		Number = 0;
		Rate = 0.0f;
	}

	if (bIgnoreSpawnRateWhenMoving == TRUE)
	{
		if (bMoved == TRUE)
		{
			return FALSE;
		}
		return TRUE;
	}

	return bProcessSpawnRate;
}

/*-----------------------------------------------------------------------------
	UParticleModuleSubUV implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSubUV);

void UParticleModuleSubUV::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	// Grab the interpolation method...
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	if (eMethod == PSUVIM_None)
	{
		return;
	}

#ifndef NX_DISABLE_FLUIDS
	if (LODLevel->TypeDataModule && !LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataNxFluid::StaticClass()) )
#else
	if (LODLevel->TypeDataModule)
#endif
	{
		// Only do SubUV on MeshEmitters - not trails or beams.
		if (LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
		{
			SpawnMesh(Owner, Offset, SpawnTime);
		}
	}
	else
	{
		SpawnSprite(Owner, Offset, SpawnTime);
	}
}

void UParticleModuleSubUV::SpawnSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	FParticleSpriteSubUVEmitterInstance* Instance = CastEmitterInstanceChecked<FParticleSpriteSubUVEmitterInstance>(Owner);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	// This is NOT directUV!
	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT SubImagesH		= LODLevel->RequiredModule->SubImages_Horizontal;
	INT	SubImagesV		= LODLevel->RequiredModule->SubImages_Vertical;
	INT TotalSubImages	= SubImagesH * SubImagesV;
	
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	FLOAT	Interp	= 0.0f;
	INT		ImageH;
	INT		ImageV;
	INT		Image2H;
	INT		Image2V	= 0;
	INT		ImageIndex;
	
	UBOOL	bUpdateImage	= TRUE;

	SPAWN_INIT;

		FSubUVSpritePayload* SubUVPayload = NULL;

		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVSpriteRandomPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= (FSubUVSpritePayload*)&TempPayload;
		}
		else
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVSpritePayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= &TempPayload;
		}

		bUpdateImage = DetermineSpriteImageIndex(Owner, &Particle, eMethod, SubUVPayload, ImageIndex, Interp, SpawnTime);

		ImageH = ImageIndex % LODLevel->RequiredModule->SubImages_Horizontal;
		ImageV = ImageIndex / LODLevel->RequiredModule->SubImages_Horizontal;
		
		if (bUpdateImage)
		{
			if (ImageH == (LODLevel->RequiredModule->SubImages_Horizontal - 1))
			{
				Image2H = 0;
				if (ImageV == (LODLevel->RequiredModule->SubImages_Vertical - 1))
				{
					Image2V = 0;
				}
				else
				{
					Image2V = ImageV + 1;
				}
			}
			else
			{
				Image2H = ImageH + 1;
				Image2V = ImageV;
			}
		}
		else
		{
			Image2H = ImageH;
			Image2V = ImageV;
		}

		// Update the payload
		SubUVPayload->Interpolation	= Interp;
		SubUVPayload->ImageH		= (FLOAT)ImageH;
		SubUVPayload->ImageV		= (FLOAT)ImageV;
		SubUVPayload->Image2H		= (FLOAT)Image2H;
		SubUVPayload->Image2V		= (FLOAT)Image2V;
}

void UParticleModuleSubUV::SpawnMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	// This is NOT directUV!
	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	FLOAT baseU = (1.0f / LODLevel->RequiredModule->SubImages_Horizontal);
	FLOAT baseV = (1.0f / LODLevel->RequiredModule->SubImages_Vertical);

	FLOAT fInterp;
	INT iImageIndex;
	INT iImageH;
	INT iImageV;

	SPAWN_INIT;
		FSubUVMeshPayload* SubUVPayload	= NULL;

		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVMeshRandomPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= (FSubUVMeshPayload*)&TempPayload;
		}
		else
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVMeshPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= &TempPayload;
		}

		UBOOL bUpdateImage = DetermineMeshImageIndex(
			Owner, &Particle, eMethod, SubUVPayload, iImageIndex, fInterp, SpawnTime);
		if (bUpdateImage)
		{
			iImageH = iImageIndex % LODLevel->RequiredModule->SubImages_Horizontal;
			iImageV = iImageIndex / LODLevel->RequiredModule->SubImages_Horizontal;

			// Update the payload
			SubUVPayload->UVOffset.X	= baseU * iImageH;
			SubUVPayload->UVOffset.Y	= baseV * iImageV;
		}
}

void UParticleModuleSubUV::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	// Grab the interpolation method...
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	if (eMethod == PSUVIM_None)
	{
		return;
	}
	else 
	if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
	{
		if (LODLevel->RequiredModule->RandomImageChanges == 0)
		{
			// Never change the random image...
			return;
		}
	}

#ifndef NX_DISABLE_FLUIDS
	if (LODLevel->TypeDataModule && !LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataNxFluid::StaticClass()) )
#else
	if (LODLevel->TypeDataModule)
#endif
	{
		// Only do SubUV on MeshEmitters - not trails or beams.
		if (LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
		{
			UpdateMesh(Owner, Offset, DeltaTime);
		}
	}
	else
	{
		UpdateSprite(Owner, Offset, DeltaTime);
	}
}

void UParticleModuleSubUV::UpdateSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	FParticleSpriteSubUVEmitterInstance* Instance = CastEmitterInstanceChecked<FParticleSpriteSubUVEmitterInstance>(Owner);
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);
	// This is NOT directUV!
	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT SubImagesH		= LODLevel->RequiredModule->SubImages_Horizontal;
	INT	SubImagesV		= LODLevel->RequiredModule->SubImages_Vertical;
	INT iTotalSubImages = SubImagesH * SubImagesV;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	float fInterp	= 0.0f;
	INT iImageH;
	INT iImageV;
	INT iImage2H;
	INT iImage2V	= 0;
	INT	iImageIndex;

	BEGIN_UPDATE_LOOP;
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		UBOOL	bUpdateImage	= TRUE;

		FSubUVSpritePayload* SubUVPayload = NULL;

		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVSpriteRandomPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= (FSubUVSpritePayload*)&TempPayload;
		}
		else
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVSpritePayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= &TempPayload;
		}

		bUpdateImage = DetermineSpriteImageIndex(Owner, &Particle, eMethod, SubUVPayload, iImageIndex, fInterp, DeltaTime);

		iImageH = iImageIndex % LODLevel->RequiredModule->SubImages_Horizontal;
		iImageV = iImageIndex / LODLevel->RequiredModule->SubImages_Horizontal;
		
		if (bUpdateImage)
		{
			if (iImageH == (LODLevel->RequiredModule->SubImages_Horizontal - 1))
			{
				iImage2H = 0;
				if (iImageV == (LODLevel->RequiredModule->SubImages_Vertical - 1))
				{
					iImage2V = 0;
				}
				else
				{
					iImage2V = iImageV + 1;
				}
			}
			else
			{
				iImage2H = iImageH + 1;
				iImage2V = iImageV;
			}
		}
		else
		{
			iImage2H = iImageH;
			iImage2V = iImageV;
		}

		// Update the payload
		SubUVPayload->Interpolation	= fInterp;
		SubUVPayload->ImageH		= (FLOAT)iImageH;
		SubUVPayload->ImageV		= (FLOAT)iImageV;
		SubUVPayload->Image2H		= (FLOAT)iImage2H;
		SubUVPayload->Image2V		= (FLOAT)iImage2V;

	END_UPDATE_LOOP;
}

void UParticleModuleSubUV::UpdateMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);
	// This is NOT directUV!
	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	FLOAT baseU = (1.0f / LODLevel->RequiredModule->SubImages_Horizontal);
	FLOAT baseV = (1.0f / LODLevel->RequiredModule->SubImages_Vertical);

	FLOAT fInterp;
	INT iImageIndex;
	INT iImageH;
	INT iImageV;

	BEGIN_UPDATE_LOOP;
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		FSubUVMeshPayload* SubUVPayload	= NULL;

		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVMeshRandomPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= (FSubUVMeshPayload*)&TempPayload;
		}
		else
		{
			INT	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			
			PARTICLE_ELEMENT(FSubUVMeshPayload, TempPayload);
			CurrentOffset	= TempOffset;
			SubUVPayload	= &TempPayload;
		}

		UBOOL bUpdateImage = DetermineMeshImageIndex(
			Owner, &Particle, eMethod, SubUVPayload, iImageIndex, fInterp, DeltaTime);
		if (bUpdateImage)
		{
			iImageH = iImageIndex % LODLevel->RequiredModule->SubImages_Horizontal;
			iImageV = iImageIndex / LODLevel->RequiredModule->SubImages_Horizontal;

			// Update the payload
			SubUVPayload->UVOffset.X	= baseU * iImageH;
			SubUVPayload->UVOffset.Y	= baseV * iImageV;
		}

	END_UPDATE_LOOP;
}

void UParticleModuleSubUV::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// For now, just do the standard Spawn...
	Spawn(Owner, Offset, SpawnTime);
}

void UParticleModuleSubUV::SpawnEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnEditorTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleSubUV::SpawnEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnEditorTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleSubUV::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// For now, just do the standard Update...
	Update(Owner, Offset, DeltaTime);
}

void UParticleModuleSubUV::UpdateEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleSubUV::UpdateEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

UBOOL UParticleModuleSubUV::DetermineSpriteImageIndex(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
	EParticleSubUVInterpMethod eMethod, FSubUVSpritePayload*& SubUVPayload, INT& ImageIndex, FLOAT& Interp, FLOAT DeltaTime)
{
	UBOOL	bUpdateImage	= TRUE;

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	INT		TotalSubImages	= LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;

	ImageIndex	= appTrunc(SubUVPayload->ImageH + SubUVPayload->ImageV * LODLevel->RequiredModule->SubImages_Horizontal);

	if ((eMethod == PSUVIM_Linear) || (eMethod == PSUVIM_Linear_Blend))
	{
		Interp = SubImageIndex.GetValue(Particle->RelativeTime, Owner->Component);
		// Assuming a 0..<# sub images> range here...
		ImageIndex = appTrunc(Interp);
		ImageIndex = Clamp(ImageIndex, 0, TotalSubImages - 1);

		if (Interp > (FLOAT)ImageIndex)
		{
			Interp = Interp - (FLOAT)ImageIndex;
		}
		else
		{
			Interp = (FLOAT)ImageIndex - Interp;
		}

		if (eMethod == PSUVIM_Linear)
			Interp = 0.0f;
	}
	else
	if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
	{
		FSubUVSpriteRandomPayload* TempPayload = (FSubUVSpriteRandomPayload*)(SubUVPayload);
		
		if ((LODLevel->RequiredModule->RandomImageTime == 0.0f) ||
			((Particle->RelativeTime - TempPayload->RandomImageTime) > LODLevel->RequiredModule->RandomImageTime) ||
			(TempPayload->RandomImageTime == 0.0f))
		{
			Interp = appSRand();
			ImageIndex = appTrunc(Interp * TotalSubImages);

			TempPayload->RandomImageTime	= Particle->RelativeTime;
		}
	
		bUpdateImage	= FALSE;
		
		if (eMethod == PSUVIM_Random)
			Interp = 0.0f;
	}
	else
	{
		Interp		= 0.0f;
		ImageIndex	= 0;
	}

	return bUpdateImage;
}

UBOOL UParticleModuleSubUV::DetermineMeshImageIndex(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
	EParticleSubUVInterpMethod eMethod, FSubUVMeshPayload*& SubUVPayload, INT& ImageIndex, FLOAT& Interp, FLOAT DeltaTime)
{
	UBOOL bUpdateImage = TRUE;

	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	INT TotalSubImages = 
		LODLevel->RequiredModule->SubImages_Horizontal * 
		LODLevel->RequiredModule->SubImages_Vertical;

	if ((eMethod == PSUVIM_Linear) || (eMethod == PSUVIM_Linear_Blend))
	{
		Interp = SubImageIndex.GetValue(Particle->RelativeTime, Owner->Component);
		// Assuming a 0..<# sub images> range here...
		ImageIndex = appTrunc(Interp);
		ImageIndex = Clamp(ImageIndex, 0, TotalSubImages - 1);

		if (Interp > (FLOAT)ImageIndex)
		{
			Interp = Interp - (FLOAT)ImageIndex;
		}
		else
		{
			Interp = (FLOAT)ImageIndex - Interp;
		}

		if (eMethod == PSUVIM_Linear)
		{
			Interp = 0.0f;
		}
	}
	else
	if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
	{
		FSubUVMeshRandomPayload* TempPayload = (FSubUVMeshRandomPayload*)(SubUVPayload);
		
		if ((LODLevel->RequiredModule->RandomImageTime == 0.0f) ||
			((Particle->RelativeTime - TempPayload->RandomImageTime) > LODLevel->RequiredModule->RandomImageTime) ||
			(TempPayload->RandomImageTime == 0.0f))
		{
			Interp = appSRand();
			ImageIndex = appTrunc(Interp * TotalSubImages);

			TempPayload->RandomImageTime	= Particle->RelativeTime;
		}
		else
		{
            bUpdateImage = FALSE;
		}
		
		if (eMethod == PSUVIM_Random)
		{
			Interp = 0.0f;
		}
	}
	else
	{
		Interp = 0.0f;
		ImageIndex = 0;
	}

	return bUpdateImage;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSubUV::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	SubImageIndex.Distribution = Cast<UDistributionFloatConstantCurve>(StaticConstructObject(UDistributionFloatConstantCurve::StaticClass(), this));
	UDistributionFloatConstantCurve* SubImageIndexDist = Cast<UDistributionFloatConstantCurve>(SubImageIndex.Distribution);
	if (SubImageIndexDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = SubImageIndexDist->CreateNewKey(Key * 1.0f);
			SubImageIndexDist->SetKeyOut(0, KeyIndex, 0.0f);
		}
		SubImageIndexDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSubUVSelect implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSubUVSelect);

void UParticleModuleSubUVSelect::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	// Grab the interpolation method...
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	if (eMethod == PSUVIM_None)
	{
		return;
	}

	if (LODLevel->TypeDataModule)
	{
		// Only do SubUV on MeshEmitters - not trails or beams.
		if (LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
		{
			UpdateMesh(Owner, Offset, DeltaTime);
		}
	}
	else
	{
		UpdateSprite(Owner, Offset, DeltaTime);
	}
}

void UParticleModuleSubUVSelect::UpdateSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	FParticleSpriteSubUVEmitterInstance* Instance = CastEmitterInstanceChecked<FParticleSpriteSubUVEmitterInstance>(Owner);
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

    float   fInterp = 0.0f;
	INT     iImageH;
	INT     iImageV;
	INT     iImage2H;
	INT     iImage2V = 0;

	BEGIN_UPDATE_LOOP;
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		if ((eMethod == PSUVIM_Linear) || (eMethod == PSUVIM_Linear_Blend))
		{
			FVector vInterp = SubImageSelect.GetValue(Particle.RelativeTime, Owner->Component);
            iImageH = appTrunc(vInterp.X);
            iImageV = appTrunc(vInterp.Y);
		}
		else
		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
            iImageH = appTrunc(appSRand() * iTotalSubImages);
            iImageV = appTrunc(appSRand() * iTotalSubImages);
		}
		else
		{
            iImageH = 0;
            iImageV = 0;
		}

		if (iImageH == (LODLevel->RequiredModule->SubImages_Horizontal - 1))
		{
			iImage2H = 0;
			if (iImageV == (LODLevel->RequiredModule->SubImages_Vertical - 1))
			{
				iImage2V = 0;
			}
			else
			{
				iImage2V = iImageV + 1;
			}
		}
		else
		{
			iImage2H = iImageH + 1;
			iImage2V = iImageV;
		}

		// Update the payload
		FLOAT* PayloadData = (FLOAT*)(((BYTE*)&Particle) + PayloadOffset);
		
		//     FLOAT Interp
		//     FLOAT imageH, imageV
		//     FLOAT image2H, image2V
		PayloadData[0] = fInterp;
		PayloadData[1] = (FLOAT)iImageH;
		PayloadData[2] = (FLOAT)iImageV;
		PayloadData[3] = (FLOAT)iImage2H;
		PayloadData[4] = (FLOAT)iImage2V;

	END_UPDATE_LOOP;
}

void UParticleModuleSubUVSelect::UpdateMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	LODLevel->RequiredModule->bDirectUV = false;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	FLOAT baseU = (1.0f / LODLevel->RequiredModule->SubImages_Horizontal);
	FLOAT baseV = (1.0f / LODLevel->RequiredModule->SubImages_Vertical);

	float   fInterp = 0.0f;
	INT     iImageH;
	INT     iImageV;
	INT     iImage2H;
	INT     iImage2V = 0;

	BEGIN_UPDATE_LOOP;
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		if ((eMethod == PSUVIM_Linear) || (eMethod == PSUVIM_Linear_Blend))
		{
			FVector vInterp = SubImageSelect.GetValue(Particle.RelativeTime, Owner->Component);
            iImageH = appTrunc(vInterp.X);
            iImageV = appTrunc(vInterp.Y);
		}
		else
		if ((eMethod == PSUVIM_Random) || (eMethod == PSUVIM_Random_Blend))
		{
            iImageH = appTrunc(appSRand() * iTotalSubImages);
            iImageV = appTrunc(appSRand() * iTotalSubImages);
		}
		else
		{
            iImageH = 0;
            iImageV = 0;
		}

		if (iImageH == (LODLevel->RequiredModule->SubImages_Horizontal - 1))
		{
			iImage2H = 0;
			if (iImageV == (LODLevel->RequiredModule->SubImages_Vertical - 1))
			{
				iImage2V = 0;
			}
			else
			{
				iImage2V = iImageV + 1;
			}
		}
		else
		{
			iImage2H = iImageH + 1;
			iImage2V = iImageV;
		}

		// Update the payload
		FSubUVMeshPayload* PayloadData = (FSubUVMeshPayload*)(((BYTE*)&Particle) + PayloadOffset);

		PayloadData->UVOffset.X	= baseU * iImageH;
		PayloadData->UVOffset.Y	= baseV * iImageV;

	END_UPDATE_LOOP;
}

void UParticleModuleSubUVSelect::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// For now, just do the standard Update...
	Update(Owner, Offset, DeltaTime);
}

void UParticleModuleSubUVSelect::UpdateEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSubUVSelect*	LowModule	= Cast<UParticleModuleSubUVSelect>(LowerLODModule);
}

void UParticleModuleSubUVSelect::UpdateEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSubUVSelect*	LowModule	= Cast<UParticleModuleSubUVSelect>(LowerLODModule);
}

/*-----------------------------------------------------------------------------
	UParticleModuleSubUVDirect implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSubUVDirect);

void UParticleModuleSubUVDirect::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	if (eMethod == PSUVIM_None)
	{
		return;
	}

	if (LODLevel->TypeDataModule)
	{
		// Only do SubUV on MeshEmitters - not trails or beams.
		if (LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
		{
			UpdateMesh(Owner, Offset, DeltaTime);
		}
	}
	else
	{
		UpdateSprite(Owner, Offset, DeltaTime);
	}
}

void UParticleModuleSubUVDirect::UpdateSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	FParticleSpriteSubUVEmitterInstance* Instance = CastEmitterInstanceChecked<FParticleSpriteSubUVEmitterInstance>(Owner);
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	LODLevel->RequiredModule->bDirectUV = true;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	BEGIN_UPDATE_LOOP;
	{
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		FVector vPosition = SubUVPosition.GetValue(Particle.RelativeTime, Owner->Component);
		FVector vSize = SubUVSize.GetValue(Particle.RelativeTime, Owner->Component);

		// Update the payload
		FLOAT* PayloadData = (FLOAT*)(((BYTE*)&Particle) + PayloadOffset);
		
		//     FLOAT Interp
		//     FLOAT imageH, imageV
		//     FLOAT image2H, image2V
		PayloadData[0] = 0.0f;
		PayloadData[1] = (FLOAT)vPosition.X;
		PayloadData[2] = (FLOAT)vPosition.Y;
		PayloadData[3] = (FLOAT)vSize.X;
		PayloadData[4] = (FLOAT)vSize.Y;
	}
	END_UPDATE_LOOP;
}

void UParticleModuleSubUVDirect::UpdateMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(HighestLODLevel);

	LODLevel->RequiredModule->bDirectUV = true;

	INT	PayloadOffset	= Owner->SubUVDataOffset;
	INT iTotalSubImages = LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
	EParticleSubUVInterpMethod eMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);

	FLOAT baseU = (1.0f / LODLevel->RequiredModule->SubImages_Horizontal);
	FLOAT baseV = (1.0f / LODLevel->RequiredModule->SubImages_Vertical);

	BEGIN_UPDATE_LOOP;
	{
		if (Particle.RelativeTime > 1.0f)
		{
			CONTINUE_UPDATE_LOOP;
		}

		FVector vPosition = SubUVPosition.GetValue(Particle.RelativeTime, Owner->Component);
		FVector vSize = SubUVSize.GetValue(Particle.RelativeTime, Owner->Component);

		// Update the payload
		FSubUVMeshPayload* PayloadData = (FSubUVMeshPayload*)(((BYTE*)&Particle) + PayloadOffset);

		PayloadData->UVOffset.X	= baseU * vPosition.X;
		PayloadData->UVOffset.Y	= baseV * vPosition.Y;
		//@todo. How to pass in the size?????
		// There is no simple way to do this without custom shader code!
	}
	END_UPDATE_LOOP;
}

void UParticleModuleSubUVDirect::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// For now, just call the standard Update...
	Update(Owner, Offset, DeltaTime);
}

void UParticleModuleSubUVDirect::UpdateEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSubUVDirect*	LowModule	= Cast<UParticleModuleSubUVDirect>(LowerLODModule);
}

void UParticleModuleSubUVDirect::UpdateEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleSubUVDirect*	LowModule	= Cast<UParticleModuleSubUVDirect>(LowerLODModule);
}

/*-----------------------------------------------------------------------------
	UParticleModuleRotationRateMultiplyLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleRotationRateMultiplyLife);

void UParticleModuleRotationRateMultiplyLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FLOAT RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.RotationRate *= RateScale;
}

void UParticleModuleRotationRateMultiplyLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FLOAT RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.RotationRate *= RateScale;
	END_UPDATE_LOOP;
}

void UParticleModuleRotationRateMultiplyLife::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*							LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleRotationRateMultiplyLife*	LowModule	= Cast<UParticleModuleRotationRateMultiplyLife>(LowerLODModule);

	SPAWN_INIT;
	FLOAT RateScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT RateScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT RateScale		= (RateScaleA * Multiplier) + (RateScaleB * (1.0f - Multiplier));
	Particle.RotationRate *= RateScale;
}

void UParticleModuleRotationRateMultiplyLife::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*							LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleRotationRateMultiplyLife*	LowModule	= Cast<UParticleModuleRotationRateMultiplyLife>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FLOAT RateScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT RateScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT RateScale		= (RateScaleA * Multiplier) + (RateScaleB * (1.0f - Multiplier));
		Particle.RotationRate *= RateScale;
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleRotationRateMultiplyLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	LifeMultiplier.Distribution = Cast<UDistributionFloatConstantCurve>(StaticConstructObject(UDistributionFloatConstantCurve::StaticClass(), this));
	UDistributionFloatConstantCurve* LifeMultiplierDist = Cast<UDistributionFloatConstantCurve>(LifeMultiplier.Distribution);
	if (LifeMultiplierDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = LifeMultiplierDist->CreateNewKey(Key * 1.0f);
			LifeMultiplierDist->SetKeyOut(0, KeyIndex, 1.0f);
		}
		LifeMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleAcceleration implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleAcceleration);

void UParticleModuleAcceleration::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	PARTICLE_ELEMENT(FVector, UsedAcceleration);
	UsedAcceleration = Acceleration.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Velocity		+= UsedAcceleration * SpawnTime;
	Particle.BaseVelocity	+= UsedAcceleration * SpawnTime;
}

void UParticleModuleAcceleration::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		PARTICLE_ELEMENT(FVector, UsedAcceleration);
		Particle.Velocity		+= UsedAcceleration * DeltaTime;
		Particle.BaseVelocity	+= UsedAcceleration * DeltaTime;
	END_UPDATE_LOOP;
}

UINT UParticleModuleAcceleration::RequiredBytes(FParticleEmitterInstance* Owner)
{
	// FVector UsedAcceleration
	return sizeof(FVector);
}

void UParticleModuleAcceleration::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleAcceleration*	LowModule	= Cast<UParticleModuleAcceleration>(LowerLODModule);

	SPAWN_INIT;
	PARTICLE_ELEMENT(FVector, UsedAcceleration);
	FVector	UsedAccelerationA	= Acceleration.GetValue(Owner->EmitterTime, Owner->Component);
	FVector	UsedAccelerationB	= LowModule->Acceleration.GetValue(Owner->EmitterTime, Owner->Component);
	UsedAcceleration		 = (UsedAccelerationA * Multiplier) + (UsedAccelerationB * (1.0f - Multiplier));
	Particle.Velocity		+= UsedAcceleration * SpawnTime;
	Particle.BaseVelocity	+= UsedAcceleration * SpawnTime;
}

void UParticleModuleAcceleration::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// In this case, the interpolation is only required in the Spawn function.
	// So, just call the standard Update function.
	Update(Owner, Offset, DeltaTime);
}

/*-----------------------------------------------------------------------------
	UParticleModuleAccelerationOverLifetime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleAccelerationOverLifetime);

void UParticleModuleAccelerationOverLifetime::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		// Acceleration should always be in world space...
		FVector Accel = AccelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Velocity		+= Accel * DeltaTime;
		Particle.BaseVelocity	+= Accel * DeltaTime;
	END_UPDATE_LOOP;
}

void UParticleModuleAccelerationOverLifetime::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*							LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleAccelerationOverLifetime*	LowModule	= Cast<UParticleModuleAccelerationOverLifetime>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		// Acceleration should always be in world space...
		FVector AccelA	= AccelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector AccelB	= LowModule->AccelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector Accel	= (AccelA * Multiplier) + (AccelB * (1.0f - Multiplier));

		Particle.Velocity		+= Accel * DeltaTime;
		Particle.BaseVelocity	+= Accel * DeltaTime;
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataBase implementation.
-----------------------------------------------------------------------------*/
FParticleEmitterInstance* UParticleModuleTypeDataBase::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	return NULL;
}

void UParticleModuleTypeDataBase::SetToSensibleDefaults()
{
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataSubUV implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataSubUV);

FParticleEmitterInstance* UParticleModuleTypeDataSubUV::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	FParticleEmitterInstance* Instance = new FParticleSpriteSubUVEmitterInstance();
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataSubUV::SetToSensibleDefaults()
{
}

void UParticleModuleTypeDataSubUV::PostEditChange(UProperty* PropertyThatChanged)
{
	if (PropertyThatChanged)
	{
		UObject* OuterObj = GetOuter();
		check(OuterObj);
		UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
		if (LODLevel)
		{
			// The outer is incorrect - warn the user and handle it
			warnf(NAME_Warning, TEXT("UParticleModuleTypeDataSubUV has an incorrect outer... run FixupEmitters on package %s"),
				*(OuterObj->GetOutermost()->GetPathName()));
			OuterObj = LODLevel->GetOuter();
			UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
			check(Emitter);
			OuterObj = Emitter->GetOuter();
		}
		UParticleSystem* PartSys = PartSys = CastChecked<UParticleSystem>(OuterObj);
		PartSys->PostEditChange(PropertyThatChanged);
	}
	Super::PostEditChange( PropertyThatChanged );
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataMesh implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataMesh);

FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	SetToSensibleDefaults();
	FParticleEmitterInstance* Instance = new FParticleMeshEmitterInstance();
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataMesh::SetToSensibleDefaults()
{
	if (Mesh == NULL)
		Mesh = (UStaticMesh*)UObject::StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("EditorMeshes.TexPropCube"),NULL,LOAD_None,NULL);
}

void UParticleModuleTypeDataMesh::PostEditChange(UProperty* PropertyThatChanged)
{
	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("Mesh")))
		{
			UObject* OuterObj = GetOuter();
			check(OuterObj);
			UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
			if (LODLevel)
			{
				// The outer is incorrect - warn the user and handle it
				warnf(NAME_Warning, TEXT("UParticleModuleTypeDataMesh has an incorrect outer... run FixupEmitters on package %s"),
					*(OuterObj->GetOutermost()->GetPathName()));
				OuterObj = LODLevel->GetOuter();
				UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
				check(Emitter);
				OuterObj = Emitter->GetOuter();
			}
			UParticleSystem* PartSys = CastChecked<UParticleSystem>(OuterObj);

			PartSys->PostEditChange(PropertyThatChanged);
		}
	}
	Super::PostEditChange( PropertyThatChanged );
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataTrail implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataTrail);

FParticleEmitterInstance* UParticleModuleTypeDataTrail::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	SetToSensibleDefaults();
	FParticleEmitterInstance* Instance = new FParticleTrailEmitterInstance();
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataTrail::SetToSensibleDefaults()
{
}

void UParticleModuleTypeDataTrail::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataBeam implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataBeam);

void UParticleModuleTypeDataBeam::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	if (EndPointMethod == PEBEPM_Distribution_Constant)
	{
		SPAWN_INIT;
		PARTICLE_ELEMENT(FVector, Tangent);
		Tangent = EndPointDirection.GetValue(Owner->EmitterTime, Owner->Component);
	}
}

void UParticleModuleTypeDataBeam::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	if (EndPointMethod == PEBEPM_Distribution_Constant)
	{
		UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
		UParticleModuleTypeDataBeam*	LowModule	= Cast<UParticleModuleTypeDataBeam>(LowerLODModule);

		SPAWN_INIT;
		PARTICLE_ELEMENT(FVector, Tangent);
		FVector	TangentA = EndPointDirection.GetValue(Owner->EmitterTime, Owner->Component);
		FVector	TangentB = EndPointDirection.GetValue(Owner->EmitterTime, Owner->Component);
		Tangent = (TangentA * Multiplier) + (TangentB * (1.0f - Multiplier));
	}
}

FVector UParticleModuleTypeDataBeam::DetermineEndPointPosition(FParticleEmitterInstance* Owner, FLOAT DeltaTime)
{
	FVector	vPosition	= Owner->Component->LocalToWorld.GetOrigin();
	FLOAT	fLoopTime	= Owner->EmitterTime;

	FParticleBeamEmitterInstance* pkBeamInst = CastEmitterInstance<FParticleBeamEmitterInstance>(Owner);
	check(pkBeamInst);
/***
	switch ((EBeamMethod)pkBeamInst->iBeamMethod)
	{
	case PEBM_Distance:
		vPosition = Owner->Component->LocalToWorld.GetOrigin() + 
			(Owner->Component->LocalToWorld.GetAxis(0) * 
			pkBeamInst->BeamTypeData->Distance.GetValue(fLoopTime, Owner->Component));
		break;
	case PEBM_EndPoints:
	case PEBM_EndPoints_Interpolated:
		vPosition = pkBeamInst->BeamTypeData->EndPoint.GetValue(fLoopTime, Owner->Component);
		break;
	case PEBM_UserSet_EndPoints:
	case PEBM_UserSet_EndPoints_Interpolated:
		if (pkBeamInst->UserSetEndPointArray.Num())
		{
			vPosition	= pkBeamInst->UserSetEndPointArray(0);
		}
		else
		{
			vPosition	= FVector(0.0f);
		}
		break;
	}
***/
	return vPosition;
}

FVector UParticleModuleTypeDataBeam::DetermineParticlePosition(FParticleEmitterInstance* Owner, FBaseParticle* pkParticle, FLOAT DeltaTime)
{
	FVector	vPosition	= Owner->Component->LocalToWorld.GetOrigin();
/***
	FLOAT	fLoopTime	= Owner->EmitterTime;

	UParticleBeamEmitterInstance* pkBeamInst = Cast<UParticleBeamEmitterInstance>(Owner);
	check(pkBeamInst);

	switch ((EBeamMethod)pkBeamInst->iBeamMethod)
	{
	case PEBM_Distance:
		vPosition = Owner->Component->LocalToWorld.GetOrigin() + 
			(Owner->Component->LocalToWorld.GetAxis(0) * 
			pkBeamInst->BeamTypeData->Distance.GetValue(fLoopTime));
		break;
	case PEBM_EndPoints:
		vPosition = pkBeamInst->BeamTypeData->EndPoint.GetValue(fLoopTime);
		break;
	case PEBM_EndPoints_Interpolated:
		vPosition = pkBeamInst->BeamTypeData->EndPoint.GetValue(fLoopTime);
		break;
	case PEBM_UserSet_EndPoints:
		vPosition = pkBeamInst->vUserSetEndPoint;
		break;
	case PEBM_UserSet_EndPoints_Interpolated:
		vPosition = pkBeamInst->vUserSetEndPoint;
		break;
	}
***/
	return vPosition;
}

UINT UParticleModuleTypeDataBeam::RequiredBytes(FParticleEmitterInstance* Owner)
{
	if (EndPointMethod == PEBEPM_Distribution_Constant)
	{
		//FVector vTangent
		return sizeof(FVector);
	}
	return 0;
}

FParticleEmitterInstance* UParticleModuleTypeDataBeam::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	SetToSensibleDefaults();
	FParticleEmitterInstance* Instance = new FParticleBeamEmitterInstance();
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataBeam::SetToSensibleDefaults()
{
}

void UParticleModuleTypeDataBeam::PreUpdate(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	check(!TEXT("BeamEmitterInstance deprecated"));
/***
	FParticleBeamEmitterInstance* pkBeamInst = CastEmitterInstanceChecked<FParticleBeamEmitterInstance>(Owner);

	// Set up the end point(s)
	FVector	vPosition	= DetermineEndPointPosition(Owner, DeltaTime);
	if (pkBeamInst->EndPointArray.Num() == 0)
	{
		pkBeamInst->EndPointArray.Add(1);
	}
	pkBeamInst->EndPointArray(0) = vPosition;
***/
}

void UParticleModuleTypeDataBeam::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberBase implementation.
-----------------------------------------------------------------------------*/
/** This function will determine the proper uber-module to utilize.					*/
UParticleModule* UParticleModuleUberBase::DetermineBestUberModule(UParticleEmitter* InputEmitter)
{
	// Check each available uber module to see if it is compatible...
	// Go largest module replacement count to smallest...
	UParticleModuleUberLTISIVCLILIRSSBLIRR* LTISIVCLILIRSSBLIRR = 
		ConstructObject<UParticleModuleUberLTISIVCLILIRSSBLIRR>(
			UParticleModuleUberLTISIVCLILIRSSBLIRR::StaticClass(), 
			InputEmitter->GetOuter());
	if (LTISIVCLILIRSSBLIRR)
	{
		if (LTISIVCLILIRSSBLIRR->IsCompatible(InputEmitter))
		{
			// We've got one...
			return LTISIVCLILIRSSBLIRR;
		}
	}

	UParticleModuleUberLTISIVCLIL* LTISIVCLIL = 
		ConstructObject<UParticleModuleUberLTISIVCLIL>(
			UParticleModuleUberLTISIVCLIL::StaticClass(), 
			InputEmitter->GetOuter());
	if (LTISIVCLIL)
	{
		if (LTISIVCLIL->IsCompatible(InputEmitter))
		{
			// We've got one...
			return LTISIVCLIL;
		}
	}

	UParticleModuleUberLTISIVCL* LTISIVCL = 
		ConstructObject<UParticleModuleUberLTISIVCL>(
			UParticleModuleUberLTISIVCL::StaticClass(), 
			InputEmitter->GetOuter());
	if (LTISIVCL)
	{
		if (LTISIVCL->IsCompatible(InputEmitter))
		{
			// We've got one...
			return LTISIVCL;
		}
	}

	return NULL;
}

/** Used by derived classes to indicate they could be used on the given emitter.	*/
UBOOL UParticleModuleUberBase::IsCompatible(UParticleEmitter* InputEmitter)
{
#if 0
	UBOOL bFoundAll	= TRUE;

	if (InputEmitter)
	{
		// Each RequiredModule must be present in the InputEmitter
		for (INT RequiedIndex = 0; RequiedIndex < RequiredModules.Num(); RequiedIndex++)
		{
			FName	RequiredName	= RequiredModules(RequiedIndex);
			UBOOL	bFound			= FALSE;

			for (INT ModuleIndex = 0; ModuleIndex < InputEmitter->Modules.Num(); ModuleIndex++)
			{
				UParticleModule* Module = InputEmitter->Modules(ModuleIndex);
				if (Module && appStrstr(*Module->GetName(), *RequiredName))
				{
					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
			{
				bFoundAll	= FALSE;
				break;
			}
		}
	}
	else
	{
		bFoundAll	= FALSE;
	}

	return bFoundAll;
#else
	return FALSE;
#endif
}

/** Copy the contents of the modules to the UberModule								*/
UBOOL UParticleModuleUberBase::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
	return FALSE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberLTISIVCL implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberLTISIVCL);

/** Copy the contents of the modules to the UberModule								*/
UBOOL UParticleModuleUberLTISIVCL::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
#if 0
	UParticleModuleLifetime*		LifetimeModule		= NULL;
	UParticleModuleSize*			SizeModule			= NULL;
	UParticleModuleVelocity*		VelocityModule		= NULL;
	UParticleModuleColorOverLife*	ColorOverLifeModule	= NULL;
	UObject*						DupObject;
	UBOOL							bMultipleModules	= FALSE;

	// Copy the module data from each module to the new one...
	for (INT SourceModuleIndex = 0; SourceModuleIndex < InputEmitter->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= InputEmitter->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime, LifetimeModule->Lifetime, InputEmitter, TEXT("None"));
				check(DupObject);
				Lifetime		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize, SizeModule->StartSize, InputEmitter, TEXT("None"));
				check(DupObject);
				StartSize		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Velocity
		if (SourceModule->IsA(UParticleModuleVelocity::StaticClass()))
		{
			if (VelocityModule == NULL)
			{
				VelocityModule		= Cast<UParticleModuleVelocity>(SourceModule);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocity, VelocityModule->StartVelocity, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocity		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocityRadial, VelocityModule->StartVelocityRadial, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocityRadial	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife, ColorOverLifeModule->ColorOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				ColorOverLife		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife, ColorOverLifeModule->AlphaOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				AlphaOverLife		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(VelocityModule			== NULL) || 
		(ColorOverLifeModule	== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	InputEmitter->Modules.RemoveItem(LifetimeModule);
	InputEmitter->Modules.RemoveItem(SizeModule);
	InputEmitter->Modules.RemoveItem(VelocityModule);
	InputEmitter->Modules.RemoveItem(ColorOverLifeModule);

	// Add the Uber
	InputEmitter->Modules.AddItem(this);

	//
	InputEmitter->UpdateModuleLists();

	return TRUE;
#else
	return FALSE;
#endif
}

/** Spawn - called when spawning particles											*/
#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCL::Spawn(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
		const UINT ActiveParticles, const UINT ParticleStride, INT ParticleIndex, 
		INT Offset, FLOAT SpawnTime)
{
	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle->OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle->OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle->OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle->OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle->RelativeTime = SpawnTime * Particle->OneOverMaxLifetime;

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Size		+= Size;
	Particle->BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle->Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle->Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Velocity		+= Vel;
	Particle->BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	Particle->Color = ColorFromVector(ColorVec, fAlpha);
}
#else	//#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCL::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color = ColorFromVector(ColorVec, fAlpha);
}
#endif	//#if defined(_PSYS_PASS_PARTICLE_)

/** Update - called when updating particles											*/
void UParticleModuleUberLTISIVCL::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		// Lifetime
		//@todo. Remove this commented out code, and in turn, the Update function 
		// of the Lifetime module?
		// Updating of relative time is a common occurance in any emitter, so that 
		// was moved up to the base emitter class. In the Tick function, when 
		// resetting the velocity and size, the following also occurs:
		//     Particle.RelativeTime += Particle.OneOverMaxLifetime * DeltaTime;
		// The killing of particles was moved to a virtual function, KillParticles, 
		// in the UParticleEmitterInstance class. This allows for custom emitters, 
		// such as the MeshEmitter, to properly 'kill' a particle - in that case, 
		// removing the mesh from the scene.

		// Size
		// No Update operation required

		// Velocity
		// No Update operation required

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color = ColorFromVector(ColorVec, fAlpha);
	}
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberLTISIVCLIL implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberLTISIVCLIL);

/** Copy the contents of the modules to the UberModule								*/
UBOOL UParticleModuleUberLTISIVCLIL::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
#if 0
	UParticleModuleLifetime*		LifetimeModule		= NULL;
	UParticleModuleSize*			SizeModule			= NULL;
	UParticleModuleVelocity*		VelocityModule		= NULL;
	UParticleModuleColorOverLife*	ColorOverLifeModule	= NULL;
	UParticleModuleLocation*		LocationModule		= NULL;
	UObject*						DupObject;
	UBOOL							bMultipleModules	= FALSE;

	// Copy the module data from each module to the new one...
	for (INT SourceModuleIndex = 0; SourceModuleIndex < InputEmitter->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= InputEmitter->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime, LifetimeModule->Lifetime, InputEmitter, TEXT("None"));
				check(DupObject);
				Lifetime		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize, SizeModule->StartSize, InputEmitter, TEXT("None"));
				check(DupObject);
				StartSize		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Velocity
		if (SourceModule->IsA(UParticleModuleVelocity::StaticClass()))
		{
			if (VelocityModule == NULL)
			{
				VelocityModule		= Cast<UParticleModuleVelocity>(SourceModule);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocity, VelocityModule->StartVelocity, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocity		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocityRadial, VelocityModule->StartVelocityRadial, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocityRadial	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife, ColorOverLifeModule->ColorOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				ColorOverLife		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife, ColorOverLifeModule->AlphaOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				AlphaOverLife		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Location
		if (SourceModule->IsA(UParticleModuleLocation::StaticClass()))
		{
			if (LocationModule == NULL)
			{
				LocationModule	= Cast<UParticleModuleLocation>(SourceModule);
				DupObject		= StaticDuplicateObject(LocationModule->StartLocation, LocationModule->StartLocation, InputEmitter, TEXT("None"));
				check(DupObject);
				StartLocation	= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(VelocityModule			== NULL) || 
		(ColorOverLifeModule	== NULL) || 
		(LocationModule			== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	InputEmitter->Modules.RemoveItem(LifetimeModule);
	InputEmitter->Modules.RemoveItem(SizeModule);
	InputEmitter->Modules.RemoveItem(VelocityModule);
	InputEmitter->Modules.RemoveItem(ColorOverLifeModule);
	InputEmitter->Modules.RemoveItem(LocationModule);

	// Add the Uber
	InputEmitter->Modules.AddItem(this);

	//
	InputEmitter->UpdateModuleLists();

	return TRUE;
#else
	return FALSE;
#endif
}

/** Spawn - called when spawning particles											*/
#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCLIL::Spawn(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
		const UINT ActiveParticles, const UINT ParticleStride, INT ParticleIndex, 
		INT Offset, FLOAT SpawnTime)
{
	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle->OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle->OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle->OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle->OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle->RelativeTime = SpawnTime * Particle->OneOverMaxLifetime;

	// Location
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle->Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
		Particle->Location += StartLoc;
	}

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Size		+= Size;
	Particle->BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle->Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle->Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Velocity		+= Vel;
	Particle->BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	Particle->Color = ColorFromVector(ColorVec, fAlpha);
}
#else	//#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCLIL::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Location
	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle.Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
		Particle.Location += StartLoc;
	}

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color = ColorFromVector(ColorVec, fAlpha);
}
#endif	//#if defined(_PSYS_PASS_PARTICLE_)

/** Update - called when updating particles											*/
void UParticleModuleUberLTISIVCLIL::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		// Lifetime
		//@todo. Remove this commented out code, and in turn, the Update function 
		// of the Lifetime module?
		// Updating of relative time is a common occurance in any emitter, so that 
		// was moved up to the base emitter class. In the Tick function, when 
		// resetting the velocity and size, the following also occurs:
		//     Particle.RelativeTime += Particle.OneOverMaxLifetime * DeltaTime;
		// The killing of particles was moved to a virtual function, KillParticles, 
		// in the UParticleEmitterInstance class. This allows for custom emitters, 
		// such as the MeshEmitter, to properly 'kill' a particle - in that case, 
		// removing the mesh from the scene.

		// Location
		// No Update operation required

		// Size
		// No Update operation required

		// Velocity
		// No Update operation required

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color = ColorFromVector(ColorVec, fAlpha);
	}
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberLTISIVCLILIRSSBLIRR implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberLTISIVCLILIRSSBLIRR);

/** Copy the contents of the modules to the UberModule								*/
UBOOL UParticleModuleUberLTISIVCLILIRSSBLIRR::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
#if 0
	UParticleModuleLifetime*			LifetimeModule			= NULL;
	UParticleModuleSize*				SizeModule				= NULL;
	UParticleModuleVelocity*			VelocityModule			= NULL;
	UParticleModuleColorOverLife*		ColorOverLifeModule		= NULL;
	UParticleModuleLocation*			LocationModule			= NULL;
	UParticleModuleRotation*			RotationModule			= NULL;
	UParticleModuleSizeMultiplyLife*	SizeMultiplyLifeModule	= NULL;
	UParticleModuleRotationRate*		RotationRateModule		= NULL;
	UObject*							DupObject;
	UBOOL								bMultipleModules		= FALSE;

	// Copy the module data from each module to the new one...
	for (INT SourceModuleIndex = 0; SourceModuleIndex < InputEmitter->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= InputEmitter->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime, LifetimeModule->Lifetime, InputEmitter, TEXT("None"));
				check(DupObject);
				Lifetime		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize, SizeModule->StartSize, InputEmitter, TEXT("None"));
				check(DupObject);
				StartSize		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Velocity
		if (SourceModule->IsA(UParticleModuleVelocity::StaticClass()))
		{
			if (VelocityModule == NULL)
			{
				VelocityModule		= Cast<UParticleModuleVelocity>(SourceModule);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocity, VelocityModule->StartVelocity, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocity		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(VelocityModule->StartVelocityRadial, VelocityModule->StartVelocityRadial, InputEmitter, TEXT("None"));
				check(DupObject);
				StartVelocityRadial	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife, ColorOverLifeModule->ColorOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				ColorOverLife		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife, ColorOverLifeModule->AlphaOverLife, InputEmitter, TEXT("None"));
				check(DupObject);
				AlphaOverLife		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Location
		if (SourceModule->IsA(UParticleModuleLocation::StaticClass()))
		{
			if (LocationModule == NULL)
			{
				LocationModule	= Cast<UParticleModuleLocation>(SourceModule);
				DupObject		= StaticDuplicateObject(LocationModule->StartLocation, LocationModule->StartLocation, InputEmitter, TEXT("None"));
				check(DupObject);
				StartLocation	= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Rotation
		if (SourceModule->IsA(UParticleModuleRotation::StaticClass()))
		{
			if (RotationModule == NULL)
			{
				RotationModule	= Cast<UParticleModuleRotation>(SourceModule);
				DupObject		= StaticDuplicateObject(RotationModule->StartRotation, RotationModule->StartRotation, InputEmitter, TEXT("None"));
				check(DupObject);
				StartRotation	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// SizeMultiplyLife
		if (SourceModule->IsA(UParticleModuleSizeMultiplyLife::StaticClass()))
		{
			if (SizeMultiplyLifeModule == NULL)
			{
				SizeMultiplyLifeModule	= Cast<UParticleModuleSizeMultiplyLife>(SourceModule);
				DupObject				= StaticDuplicateObject(SizeMultiplyLifeModule->LifeMultiplier, SizeMultiplyLifeModule->LifeMultiplier, InputEmitter, TEXT("None"));
				check(DupObject);
				SizeLifeMultiplier		= Cast<UDistributionVector>(DupObject);
				SizeMultiplyX			= SizeMultiplyLifeModule->MultiplyX;
				SizeMultiplyY			= SizeMultiplyLifeModule->MultiplyY;
				SizeMultiplyZ			= SizeMultiplyLifeModule->MultiplyZ;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// RotationRate
		if (SourceModule->IsA(UParticleModuleRotationRate::StaticClass()))
		{
			if (RotationRateModule == NULL)
			{
				RotationRateModule	= Cast<UParticleModuleRotationRate>(SourceModule);
				DupObject			= StaticDuplicateObject(RotationRateModule->StartRotationRate, RotationRateModule->StartRotationRate, InputEmitter, TEXT("None"));
				check(DupObject);
				StartRotationRate	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(VelocityModule			== NULL) || 
		(ColorOverLifeModule	== NULL) || 
		(LocationModule			== NULL) || 
		(RotationModule			== NULL) || 
		(SizeMultiplyLifeModule	== NULL) || 
		(RotationRateModule		== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	InputEmitter->Modules.RemoveItem(LifetimeModule);
	InputEmitter->Modules.RemoveItem(SizeModule);
	InputEmitter->Modules.RemoveItem(VelocityModule);
	InputEmitter->Modules.RemoveItem(ColorOverLifeModule);
	InputEmitter->Modules.RemoveItem(LocationModule);
	InputEmitter->Modules.RemoveItem(RotationModule);
	InputEmitter->Modules.RemoveItem(SizeMultiplyLifeModule);
	InputEmitter->Modules.RemoveItem(RotationRateModule);

	// Add the Uber
	InputEmitter->Modules.AddItem(this);

	//
	InputEmitter->UpdateModuleLists();

	return TRUE;
#else
	return FALSE;
#endif
}

/** Spawn - called when spawning particles											*/
#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCLILIRSSBLIRR::Spawn(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
		const UINT ActiveParticles, const UINT ParticleStride, INT ParticleIndex, 
		INT Offset, FLOAT SpawnTime)
{
	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle->OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle->OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle->OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle->OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle->RelativeTime = SpawnTime * Particle->OneOverMaxLifetime;

	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Location
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle->Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
		Particle->Location += StartLoc;
	}

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Size		+= Size;
	Particle->BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle->Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle->Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->Velocity		+= Vel;
	Particle->BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle->RelativeTime, Owner->Component);
	Particle->Color = ColorFromVector(ColorVec, fAlpha);

	// Rotation
	Particle->Rotation += (PI/180.f) * 360.0f * StartRotation.GetValue(Owner->EmitterTime, Owner->Component);

	// SizeMultipleByLife
	FVector SizeScale = LifeMultiplier.GetValue(Particle->RelativeTime, Owner->Component);
	if (MultiplyX)
	{
		Particle->Size.X *= SizeScale.X;
	}
	if (MultiplyY)
	{
		Particle->Size.Y *= SizeScale.Y;
	}
	if (MultiplyZ)
	{
		Particle->Size.Z *= SizeScale.Z;
	}

	// RotationRate
	FLOAT StartRotRate = (PI/180.f) * 360.0f * StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);
	Particle->RotationRate += StartRotRate;
	Particle->BaseRotationRate += StartRotRate;
}
#else	//#if defined(_PSYS_PASS_PARTICLE_)
void UParticleModuleUberLTISIVCLILIRSSBLIRR::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Location
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Particle.Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
		Particle.Location += StartLoc;
	}

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color = ColorFromVector(ColorVec, fAlpha);

	// Rotation
	Particle.Rotation += (PI/180.f) * 360.0f * StartRotation.GetValue(Owner->EmitterTime, Owner->Component);

	// SizeMultipleByLife
	FVector SizeScale = SizeLifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if (SizeMultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if (SizeMultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if (SizeMultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// RotationRate
	FLOAT StartRotRate = (PI/180.f) * 360.0f * StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.RotationRate += StartRotRate;
	Particle.BaseRotationRate += StartRotRate;
}
#endif	//#if defined(_PSYS_PASS_PARTICLE_)

/** Update - called when updating particles											*/
void UParticleModuleUberLTISIVCLILIRSSBLIRR::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		// Lifetime
		//@todo. Remove this commented out code, and in turn, the Update function 
		// of the Lifetime module?
		// Updating of relative time is a common occurance in any emitter, so that 
		// was moved up to the base emitter class. In the Tick function, when 
		// resetting the velocity and size, the following also occurs:
		//     Particle.RelativeTime += Particle.OneOverMaxLifetime * DeltaTime;
		// The killing of particles was moved to a virtual function, KillParticles, 
		// in the UParticleEmitterInstance class. This allows for custom emitters, 
		// such as the MeshEmitter, to properly 'kill' a particle - in that case, 
		// removing the mesh from the scene.

		// Location
		// No Update operation required

		// Size
		// No Update operation required

		// Velocity
		// No Update operation required

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color = ColorFromVector(ColorVec, fAlpha);

		// Rotation
		// No Update operation required

		// SizeMultipleLife
		FVector SizeScale = SizeLifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		if (SizeMultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if (SizeMultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if (SizeMultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// RotationRate
		// No Update operation required
	}
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberRainDrops implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberRainDrops);

void UParticleModuleUberRainDrops::PostLoad()
{
	Super::PostLoad();

	// We need to convert distributions from [0..256] to [0..1].
	if (GetLinker() && (GetLinker()->Ver() < VER_PARTICLESYSTEM_LINEARCOLOR_SUPPORT))
	{
		ColorOverLife.X /= 255.9f;
		Clamp<FLOAT>(ColorOverLife.X, 0.0f, 1.0f);
		ColorOverLife.Y /= 255.9f;
		Clamp<FLOAT>(ColorOverLife.Y, 0.0f, 1.0f);
		ColorOverLife.Z /= 255.9f;
		Clamp<FLOAT>(ColorOverLife.Z, 0.0f, 1.0f);
		AlphaOverLife /= 255.9f;
		Clamp<FLOAT>(AlphaOverLife, 0.0f, 1.0f);

		MarkPackageDirty();
	}
}

void UParticleModuleUberRainDrops::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;


	FLOAT	Rand;

	SPAWN_INIT;

	// Lifetime
	Rand = appSRand();
	FLOAT MaxLifetime = (LifetimeMax * Rand) + (LifetimeMin * (1.0f - Rand));
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector Size;
	Rand = appSRand();
	Size.X		= (StartSizeMax.X * Rand) + (StartSizeMin.X * (1.0f - Rand));
	Rand = appSRand();
	Size.Y		= (StartSizeMax.Y * Rand) + (StartSizeMin.Y * (1.0f - Rand));
	Rand = appSRand();
	Size.Z		= (StartSizeMax.Z * Rand) + (StartSizeMin.Z * (1.0f - Rand));
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// Velocity
	FVector FromOrigin;
	FVector Vel;

	Rand = appSRand();
	Vel.X = (StartVelocityMax.X * Rand) + (StartVelocityMin.X * (1.0f - Rand));
	Rand = appSRand();
	Vel.Y = (StartVelocityMax.Y * Rand) + (StartVelocityMin.Y * (1.0f - Rand));
	Rand = appSRand();
	Vel.Z = (StartVelocityMax.Z * Rand) + (StartVelocityMin.Z * (1.0f - Rand));

	if (bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * ((StartVelocityRadialMax * Rand) + (StartVelocityRadialMax * (1.0f - Rand)));
	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;

	// ColorOverLife
	Particle.Color		= ColorFromVector(ColorOverLife, AlphaOverLife);
	Particle.BaseColor	= Particle.Color;

	// Cylinder
	if (bIsUsingCylinder)
	{
		INT	RadialIndex0	= 0;	//X
		INT	RadialIndex1	= 1;	//Y
		INT	HeightIndex		= 2;	//Z

		switch (PC_HeightAxis)
		{
		case PMLPC_HEIGHTAXIS_X:
			RadialIndex0	= 1;	//Y
			RadialIndex1	= 2;	//Z
			HeightIndex		= 0;	//X
			break;
		case PMLPC_HEIGHTAXIS_Y:
			RadialIndex0	= 0;	//X
			RadialIndex1	= 2;	//Z
			HeightIndex		= 1;	//Y
			break;
		case PMLPC_HEIGHTAXIS_Z:
			break;
		}

		// Determine the unit direction
		FVector UnitDir, UnitDirTemp;
		DetermineUnitDirection(Owner, UnitDirTemp);
		UnitDir[RadialIndex0]	= UnitDirTemp[RadialIndex0];
		UnitDir[RadialIndex1]	= UnitDirTemp[RadialIndex1];
		UnitDir[HeightIndex]	= UnitDirTemp[HeightIndex];

		FVector NormalizedDir = UnitDir;
		NormalizedDir.Normalize();

		FVector2D UnitDir2D(UnitDir[RadialIndex0], UnitDir[RadialIndex1]);
		FVector2D NormalizedDir2D = UnitDir2D.SafeNormal();

		// Determine the position
		FVector Offset(0.0f);

		// Always want Z in the [-Height, Height] range
		Offset[HeightIndex] = UnitDir[HeightIndex] * PC_StartHeight;

		NormalizedDir[RadialIndex0] = NormalizedDir2D.X;
		NormalizedDir[RadialIndex1] = NormalizedDir2D.Y;

		if (bSurfaceOnly)
		{
			// Clamp the X,Y to the outer edge...

			if (Abs(Offset[HeightIndex]) == PC_StartHeight)
			{
				// On the caps, it can be anywhere within the 'circle'
				Offset[RadialIndex0] = UnitDir[RadialIndex0] * PC_StartRadius;
				Offset[RadialIndex1] = UnitDir[RadialIndex1] * PC_StartRadius;
			}
			else
			{
				// On the sides, it must be on the 'circle'
				Offset[RadialIndex0] = NormalizedDir[RadialIndex0] * PC_StartRadius;
				Offset[RadialIndex1] = NormalizedDir[RadialIndex1] * PC_StartRadius;
			}
		}
		else
		{
			Offset[RadialIndex0] = UnitDir[RadialIndex0] * PC_StartRadius;
			Offset[RadialIndex1] = UnitDir[RadialIndex1] * PC_StartRadius;
		}

		// Clamp to the radius...
		FVector	Max;
		
		Max[RadialIndex0]	= Abs(NormalizedDir[RadialIndex0]) * PC_StartRadius;
		Max[RadialIndex1]	= Abs(NormalizedDir[RadialIndex1]) * PC_StartRadius;
		Max[HeightIndex]	= PC_StartHeight;

		Offset[RadialIndex0]	= Clamp<FLOAT>(Offset[RadialIndex0], -Max[RadialIndex0], Max[RadialIndex0]);
		Offset[RadialIndex1]	= Clamp<FLOAT>(Offset[RadialIndex1], -Max[RadialIndex1], Max[RadialIndex1]);
		Offset[HeightIndex]		= Clamp<FLOAT>(Offset[HeightIndex],  -Max[HeightIndex],  Max[HeightIndex]);

		// Add in the start location
		Offset[RadialIndex0]	+= PC_StartLocation[RadialIndex0];
		Offset[RadialIndex1]	+= PC_StartLocation[RadialIndex1];
		Offset[HeightIndex]		+= PC_StartLocation[HeightIndex];

		if (bUseLocalSpace == FALSE)
		{
			Offset = Owner->Component->LocalToWorld.TransformNormal(Offset);
		}
		Particle.Location += Offset;

		if (bVelocity)
		{
			FVector Velocity;
			Velocity[RadialIndex0]	= Offset[RadialIndex0]	- PC_StartLocation[RadialIndex0];
			Velocity[RadialIndex1]	= Offset[RadialIndex1]	- PC_StartLocation[RadialIndex1];
			Velocity[HeightIndex]	= Offset[HeightIndex]	- PC_StartLocation[HeightIndex];

			if (bRadialVelocity)
			{
				Velocity[HeightIndex]	= 0.0f;
			}
			Velocity	*= PC_VelocityScale;

			Particle.Velocity		+= Velocity;
			Particle.BaseVelocity	+= Velocity;
		}
	}

	// Location
	FVector	StartLoc;

	Rand	= appSRand();
	StartLoc.X	= (StartLocationMax.X * Rand) + (StartLocationMin.X * (1.0f - Rand));
	Rand	= appSRand();
	StartLoc.Y	= (StartLocationMax.Y * Rand) + (StartLocationMin.Y * (1.0f - Rand));
	Rand	= appSRand();
	StartLoc.Z	= (StartLocationMax.Z * Rand) + (StartLocationMin.Z * (1.0f - Rand));

	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		StartLoc = Owner->Component->LocalToWorld.TransformNormal(StartLoc);
	}
	Particle.Location += StartLoc;
}

void UParticleModuleUberRainDrops::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
}

void UParticleModuleUberRainDrops::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	Spawn(Owner, Offset, SpawnTime);
}

void UParticleModuleUberRainDrops::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
}

void UParticleModuleUberRainDrops::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Lifetime

	// Size

	// Velocity

	// ColorOverLife

	// Cylinder

	// Location
}

void UParticleModuleUberRainDrops::DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& UnitDir)
{
	FVector Rand;

	// Grab 3 random numbers for the axes
	Rand.X = appSRand();
	Rand.Y = appSRand();
	Rand.Z = appSRand();

	// Set the unit dir
	if (bPositive_X && bNegative_X)
	{
		UnitDir.X = Rand.X * 2 - 1;
	}
	else
	if (bPositive_X)
	{
		UnitDir.X = Rand.X;
	}
	else
	if (bNegative_X)
	{
		UnitDir.X = -Rand.X;
	}
	else
	{
		UnitDir.X = 0.0f;
	}

	if (bPositive_Y && bNegative_Y)
	{
		UnitDir.Y = Rand.Y * 2 - 1;
	}
	else
	if (bPositive_Y)
	{
		UnitDir.Y = Rand.Y;
	}
	else
	if (bNegative_Y)
	{
		UnitDir.Y = -Rand.Y;
	}
	else
	{
		UnitDir.Y = 0.0f;
	}

	if (bPositive_Z && bNegative_Z)
	{
		UnitDir.Z = Rand.Z * 2 - 1;
	}
	else
	if (bPositive_Z)
	{
		UnitDir.Z = Rand.Z;
	}
	else
	if (bNegative_Z)
	{
		UnitDir.Z = -Rand.Z;
	}
	else
	{
		UnitDir.Z = 0.0f;
	}
}

UBOOL UParticleModuleUberRainDrops::IsCompatible(UParticleEmitter* InputEmitter)
{
	UBOOL bFoundAll	= FALSE;

	if (InputEmitter)
	{
		UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
		check(LODLevel);

		if (LODLevel->Modules.Num() == 5)
		{
			// They MUST be in this order
			// Lifetime
			// Size
			// Velocity
			// ColorOverLife
			// Location
			if (LODLevel->Modules(0)->IsA(UParticleModuleLifetime::StaticClass())					&&
				LODLevel->Modules(1)->IsA(UParticleModuleSize::StaticClass())						&&
				LODLevel->Modules(2)->IsA(UParticleModuleVelocity::StaticClass())					&&
				LODLevel->Modules(3)->IsA(UParticleModuleColorOverLife::StaticClass())				&&
				LODLevel->Modules(4)->IsA(UParticleModuleLocation::StaticClass()))
			{
				bFoundAll	= TRUE;
			}
		}
		else
		if (LODLevel->Modules.Num() == 6)
		{
			// They MUST be in this order
			// Lifetime
			// Size
			// Velocity
			// ColorOverLife
			// Cylinder
			// Location
			if (LODLevel->Modules(0)->IsA(UParticleModuleLifetime::StaticClass())					&&
				LODLevel->Modules(1)->IsA(UParticleModuleSize::StaticClass())						&&
				LODLevel->Modules(2)->IsA(UParticleModuleVelocity::StaticClass())					&&
				LODLevel->Modules(3)->IsA(UParticleModuleColorOverLife::StaticClass())				&&
				LODLevel->Modules(4)->IsA(UParticleModuleLocationPrimitiveCylinder::StaticClass())	&&
				LODLevel->Modules(5)->IsA(UParticleModuleLocation::StaticClass()))
			{
				bFoundAll	= TRUE;
			}
		}
	}

	return bFoundAll;
}

UBOOL UParticleModuleUberRainDrops::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
	if (InputEmitter->LODLevels.Num() > 2)
	{
		appMsgf(AMT_OK, TEXT("Can't convert an emitter with specific LOD levels!"));
		return FALSE;
	}

	// Lifetime
	// Size
	// Velocity
	// ColorOverLife
	// Cylinder
	// Location
	UParticleModuleLifetime*					LifetimeModule		= NULL;
	UParticleModuleSize*						SizeModule			= NULL;
	UParticleModuleVelocity*					VelocityModule		= NULL;
	UParticleModuleColorOverLife*				ColorOverLifeModule	= NULL;
	UParticleModuleLocationPrimitiveCylinder*	CylinderModule		= NULL;
	UParticleModuleLocation*					LocationModule		= NULL;
	UBOOL										bMultipleModules	= FALSE;

	bIsUsingCylinder = FALSE;

	// Copy the module data from each module to the new one...
	UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
	check(LODLevel);

	for (INT SourceModuleIndex = 0; SourceModuleIndex < LODLevel->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= LODLevel->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				LifetimeModule->Lifetime.Distribution->GetOutRange(LifetimeMin, LifetimeMax);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				SizeModule->StartSize.Distribution->GetRange(StartSizeMin, StartSizeMax);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Velocity
		if (SourceModule->IsA(UParticleModuleVelocity::StaticClass()))
		{
			if (VelocityModule == NULL)
			{
				VelocityModule		= Cast<UParticleModuleVelocity>(SourceModule);
				VelocityModule->StartVelocity.Distribution->GetRange(StartVelocityMin, StartVelocityMax);
				VelocityModule->StartVelocityRadial.Distribution->GetOutRange(StartVelocityRadialMin, StartVelocityRadialMax);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				ColorOverLifeModule->ColorOverLife.Distribution->GetRange(ColorOverLife, ColorOverLife);
				ColorOverLifeModule->AlphaOverLife.Distribution->GetOutRange(AlphaOverLife, AlphaOverLife);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// PrimitiveCylinder
		if (SourceModule->IsA(UParticleModuleLocationPrimitiveCylinder::StaticClass()))
		{
			if (CylinderModule == NULL)
			{
				CylinderModule	= Cast<UParticleModuleLocationPrimitiveCylinder>(SourceModule);

				bIsUsingCylinder	= TRUE;
				bPositive_X			= CylinderModule->Positive_X;
				bPositive_Y			= CylinderModule->Positive_Y;
				bPositive_Z			= CylinderModule->Positive_Z;
				bNegative_X			= CylinderModule->Negative_X;
				bNegative_Y			= CylinderModule->Negative_Y;
				bNegative_Z			= CylinderModule->Negative_Z;
				bSurfaceOnly		= CylinderModule->SurfaceOnly;
				bVelocity			= CylinderModule->Velocity;
				bRadialVelocity		= CylinderModule->RadialVelocity;
				CylinderModule->VelocityScale.Distribution->GetOutRange(PC_VelocityScale, PC_VelocityScale);
				CylinderModule->StartLocation.Distribution->GetRange(PC_StartLocation, PC_StartLocation);
				CylinderModule->StartRadius.Distribution->GetOutRange(PC_StartRadius, PC_StartRadius);
				CylinderModule->StartHeight.Distribution->GetOutRange(PC_StartHeight, PC_StartHeight);
				PC_HeightAxis		= CylinderModule->HeightAxis;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// InitialLocation
		if (SourceModule->IsA(UParticleModuleLocation::StaticClass()))
		{
			if (LocationModule == NULL)
			{
				LocationModule	= Cast<UParticleModuleLocation>(SourceModule);
				LocationModule->StartLocation.Distribution->GetRange(StartLocationMin, StartLocationMax);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if (LODLevel->Modules.Num() == 5)
	{
		if ((LifetimeModule			== NULL) || 
			(SizeModule				== NULL) || 
			(VelocityModule			== NULL) || 
			(ColorOverLifeModule	== NULL) || 
			(LocationModule			== NULL))
		{
			// Failed the conversion!
			return FALSE;
		}
	}
	else
	if (LODLevel->Modules.Num() == 6)
	{
		if ((LifetimeModule			== NULL) || 
			(SizeModule				== NULL) || 
			(VelocityModule			== NULL) || 
			(ColorOverLifeModule	== NULL) || 
			(CylinderModule			== NULL) || 
			(LocationModule			== NULL))
		{
			// Failed the conversion!
			return FALSE;
		}
	}

	// Remove the modules
	LODLevel->Modules.RemoveItem(LifetimeModule);
	LODLevel->Modules.RemoveItem(SizeModule);
	LODLevel->Modules.RemoveItem(VelocityModule);
	LODLevel->Modules.RemoveItem(ColorOverLifeModule);
	if (CylinderModule)
	{
		LODLevel->Modules.RemoveItem(CylinderModule);
	}
	LODLevel->Modules.RemoveItem(LocationModule);

	// Add the Uber
	LODLevel->Modules.AddItem(this);

	//
	LODLevel->UpdateModuleLists();

	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberRainImpacts implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberRainImpacts);

void UParticleModuleUberRainImpacts::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// InitialMeshRotation
	if (MeshInst)
	{
		FVector Rotation = StartRotation.GetValue(Owner->EmitterTime, Owner->Component);

		if (bInheritParent)
		{
			FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
			FVector		ParentAffectedRotation	= Rotator.Euler();
			Rotation.X	+= ParentAffectedRotation.X / 360.0f;
			Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
			Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
		}

		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		PayloadData->Rotation.X	+= Rotation.X * 360.0f;
		PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
		PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
	}

	// SizeByLife
	FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// Cylinder
	if (bIsUsingCylinder)
	{
		INT	RadialIndex0	= 0;	//X
		INT	RadialIndex1	= 1;	//Y
		INT	HeightIndex		= 2;	//Z

		switch (PC_HeightAxis)
		{
		case PMLPC_HEIGHTAXIS_X:
			RadialIndex0	= 1;	//Y
			RadialIndex1	= 2;	//Z
			HeightIndex		= 0;	//X
			break;
		case PMLPC_HEIGHTAXIS_Y:
			RadialIndex0	= 0;	//X
			RadialIndex1	= 2;	//Z
			HeightIndex		= 1;	//Y
			break;
		case PMLPC_HEIGHTAXIS_Z:
			break;
		}

		// Determine the start location for the sphere
		FVector StartLoc = PC_StartLocation.GetValue(Owner->EmitterTime, Owner->Component);

		// Determine the unit direction
		FVector UnitDir, UnitDirTemp;
		DetermineUnitDirection(Owner, UnitDirTemp);
		UnitDir[RadialIndex0]	= UnitDirTemp[RadialIndex0];
		UnitDir[RadialIndex1]	= UnitDirTemp[RadialIndex1];
		UnitDir[HeightIndex]	= UnitDirTemp[HeightIndex];

		FVector NormalizedDir = UnitDir;
		NormalizedDir.Normalize();

		FVector2D UnitDir2D(UnitDir[RadialIndex0], UnitDir[RadialIndex1]);
		FVector2D NormalizedDir2D = UnitDir2D.SafeNormal();

		// Determine the position
		FVector Offset(0.0f);
		FLOAT	StartRadius	= PC_StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	StartHeight	= PC_StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;

		// Always want Z in the [-Height, Height] range
		Offset[HeightIndex] = UnitDir[HeightIndex] * StartHeight;

		NormalizedDir[RadialIndex0] = NormalizedDir2D.X;
		NormalizedDir[RadialIndex1] = NormalizedDir2D.Y;

		if (bSurfaceOnly)
		{
			// Clamp the X,Y to the outer edge...

			if (Abs(Offset[HeightIndex]) == StartHeight)
			{
				// On the caps, it can be anywhere within the 'circle'
				Offset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
				Offset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
			}
			else
			{
				// On the sides, it must be on the 'circle'
				Offset[RadialIndex0] = NormalizedDir[RadialIndex0] * StartRadius;
				Offset[RadialIndex1] = NormalizedDir[RadialIndex1] * StartRadius;
			}
		}
		else
		{
			Offset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
			Offset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
		}

		// Clamp to the radius...
		FVector	Max;
		
		Max[RadialIndex0]	= Abs(NormalizedDir[RadialIndex0]) * StartRadius;
		Max[RadialIndex1]	= Abs(NormalizedDir[RadialIndex1]) * StartRadius;
		Max[HeightIndex]	= StartHeight;

		Offset[RadialIndex0]	= Clamp<FLOAT>(Offset[RadialIndex0], -Max[RadialIndex0], Max[RadialIndex0]);
		Offset[RadialIndex1]	= Clamp<FLOAT>(Offset[RadialIndex1], -Max[RadialIndex1], Max[RadialIndex1]);
		Offset[HeightIndex]		= Clamp<FLOAT>(Offset[HeightIndex],  -Max[HeightIndex],  Max[HeightIndex]);

		// Add in the start location
		Offset[RadialIndex0]	+= StartLoc[RadialIndex0];
		Offset[RadialIndex1]	+= StartLoc[RadialIndex1];
		Offset[HeightIndex]		+= StartLoc[HeightIndex];

		if (bUseLocalSpace == FALSE)
		{
			Offset = Owner->Component->LocalToWorld.TransformNormal(Offset);
		}
		Particle.Location += Offset;

		if (bVelocity)
		{
			FVector Velocity;
			Velocity[RadialIndex0]	= Offset[RadialIndex0]	- StartLoc[RadialIndex0];
			Velocity[RadialIndex1]	= Offset[RadialIndex1]	- StartLoc[RadialIndex1];
			Velocity[HeightIndex]	= Offset[HeightIndex]	- StartLoc[HeightIndex];

			if (bRadialVelocity)
			{
				Velocity[HeightIndex]	= 0.0f;
			}
			Velocity	*= PC_VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);

			Particle.Velocity		+= Velocity;
			Particle.BaseVelocity	+= Velocity;
		}
	}

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	Particle.BaseColor	= Particle.Color;

}

void UParticleModuleUberRainImpacts::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// InitialMeshRotation

		// SizeByLife
		FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// Cylinder

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color		= ColorFromVector(ColorVec, fAlpha);

	END_UPDATE_LOOP;
}

void UParticleModuleUberRainImpacts::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	FParticleMeshEmitterInstance*	MeshInst	= CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainImpacts*	LowModule	= Cast<UParticleModuleUberRainImpacts>(LowerLODModule);

	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetimeA	= Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetimeB	= LowModule->Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetime	= (MaxLifetimeA * Multiplier) + (MaxLifetimeB * (1.0f - Multiplier));
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector SizeA		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector SizeB		 = LowModule->StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Size		 = (SizeA * Multiplier)+ (SizeB * (1.0f - Multiplier));
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// InitialMeshRotation
	if (MeshInst)
	{
		FVector RotationA	= StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector RotationB	= LowModule->StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector Rotation	= (RotationA * Multiplier) + (RotationB * (1.0f - Multiplier));

		if (bInheritParent)
		{
			FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
			FVector		ParentAffectedRotation	= Rotator.Euler();
			Rotation.X	+= ParentAffectedRotation.X / 360.0f;
			Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
			Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
		}

		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		PayloadData->Rotation.X	+= Rotation.X * 360.0f;
		PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
		PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
	}

	// SizeByLife
	FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
	if (MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if (MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if (MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// Cylinder
	if (bIsUsingCylinder)
	{
		INT	RadialIndex0	= 0;	//X
		INT	RadialIndex1	= 1;	//Y
		INT	HeightIndex		= 2;	//Z

		check(PC_HeightAxis == LowModule->PC_HeightAxis);

		switch (PC_HeightAxis)
		{
		case PMLPC_HEIGHTAXIS_X:
			RadialIndex0	= 1;	//Y
			RadialIndex1	= 2;	//Z
			HeightIndex		= 0;	//X
			break;
		case PMLPC_HEIGHTAXIS_Y:
			RadialIndex0	= 0;	//X
			RadialIndex1	= 2;	//Z
			HeightIndex		= 1;	//Y
			break;
		case PMLPC_HEIGHTAXIS_Z:
			break;
		}

		// Determine the start location for the sphere
		FVector StartLocA = PC_StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector StartLocB = LowModule->PC_StartLocation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector StartLoc  = (StartLocA * Multiplier) + (StartLocB * (1.0f - Multiplier));

		// Determine the unit direction
		FVector UnitDir, UnitDirTemp;
		DetermineUnitDirection(Owner, UnitDirTemp);
		UnitDir[RadialIndex0]	= UnitDirTemp[RadialIndex0];
		UnitDir[RadialIndex1]	= UnitDirTemp[RadialIndex1];
		UnitDir[HeightIndex]	= UnitDirTemp[HeightIndex];

		FVector NormalizedDir = UnitDir;
		NormalizedDir.Normalize();

		FVector2D UnitDir2D(UnitDir[RadialIndex0], UnitDir[RadialIndex1]);
		FVector2D NormalizedDir2D = UnitDir2D.SafeNormal();

		// Determine the position
		FVector PosOffset(0.0f);

		FLOAT	StartRadiusA	= PC_StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	StartRadiusB	= LowModule->PC_StartRadius.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	StartRadius		= (StartRadiusA	* Multiplier) + (StartRadiusB * (1.0f - Multiplier));

		FLOAT	StartHeightA	= PC_StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;
		FLOAT	StartHeightB	= LowModule->PC_StartHeight.GetValue(Owner->EmitterTime, Owner->Component) / 2.0f;
		FLOAT	StartHeight		= (StartHeightA	* Multiplier) + (StartHeightB * (1.0f - Multiplier));

		// Always want Z in the [-Height, Height] range
		PosOffset[HeightIndex] = UnitDir[HeightIndex] * StartHeight;

		NormalizedDir[RadialIndex0] = NormalizedDir2D.X;
		NormalizedDir[RadialIndex1] = NormalizedDir2D.Y;

		if (bSurfaceOnly)
		{
			// Clamp the X,Y to the outer edge...

			if (Abs(PosOffset[HeightIndex]) == StartHeight)
			{
				// On the caps, it can be anywhere within the 'circle'
				PosOffset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
				PosOffset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
			}
			else
			{
				// On the sides, it must be on the 'circle'
				PosOffset[RadialIndex0] = NormalizedDir[RadialIndex0] * StartRadius;
				PosOffset[RadialIndex1] = NormalizedDir[RadialIndex1] * StartRadius;
			}
		}
		else
		{
			PosOffset[RadialIndex0] = UnitDir[RadialIndex0] * StartRadius;
			PosOffset[RadialIndex1] = UnitDir[RadialIndex1] * StartRadius;
		}

		// Clamp to the radius...
		FVector	Max;
		
		Max[RadialIndex0]	= Abs(NormalizedDir[RadialIndex0]) * StartRadius;
		Max[RadialIndex1]	= Abs(NormalizedDir[RadialIndex1]) * StartRadius;
		Max[HeightIndex]	= StartHeight;

		PosOffset[RadialIndex0]	= Clamp<FLOAT>(PosOffset[RadialIndex0], -Max[RadialIndex0], Max[RadialIndex0]);
		PosOffset[RadialIndex1]	= Clamp<FLOAT>(PosOffset[RadialIndex1], -Max[RadialIndex1], Max[RadialIndex1]);
		PosOffset[HeightIndex]	= Clamp<FLOAT>(PosOffset[HeightIndex],  -Max[HeightIndex],  Max[HeightIndex]);

		// Add in the start location
		PosOffset[RadialIndex0]	+= StartLoc[RadialIndex0];
		PosOffset[RadialIndex1]	+= StartLoc[RadialIndex1];
		PosOffset[HeightIndex]	+= StartLoc[HeightIndex];

		if (bUseLocalSpace == FALSE)
		{
			PosOffset = Owner->Component->LocalToWorld.TransformNormal(PosOffset);
		}
		Particle.Location += PosOffset;

		if (bVelocity)
		{
			FLOAT	VelocityA;
			FLOAT	VelocityB;
			FVector	Velocity;
			Velocity[RadialIndex0]	= PosOffset[RadialIndex0]	- StartLoc[RadialIndex0];
			Velocity[RadialIndex1]	= PosOffset[RadialIndex1]	- StartLoc[RadialIndex1];
			Velocity[HeightIndex]	= PosOffset[HeightIndex]	- StartLoc[HeightIndex];

			if (bRadialVelocity)
			{
				Velocity[HeightIndex]	= 0.0f;
			}

			VelocityA	 = PC_VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
			VelocityB	 = LowModule->PC_VelocityScale.GetValue(Owner->EmitterTime, Owner->Component);
			Velocity	*= ((VelocityA * Multiplier) + (VelocityB * (1.0f - Multiplier)));

			Particle.Velocity		+= Velocity;
			Particle.BaseVelocity	+= Velocity;
		}
	}

	// ColorOverLife
	FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

	FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

	Particle.Color		= ColorFromVector(ColorVec, Alpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleUberRainImpacts::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainImpacts*	LowModule	= Cast<UParticleModuleUberRainImpacts>(LowerLODModule);

	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// InitialMeshRotation

		// SizeByLife
		FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
		if (MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if (MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if (MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// Cylinder

		// ColorOverLife
		FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

		Particle.Color		= ColorFromVector(ColorVec, Alpha);

	END_UPDATE_LOOP;
}

void UParticleModuleUberRainImpacts::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Lifetime

	// Size

	// InitialMeshRotation

	// SizeByLife

	// Cylinder

	// ColorOverLife
}

void UParticleModuleUberRainImpacts::DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& UnitDir)
{
	FVector Rand;

	// Grab 3 random numbers for the axes
	Rand.X = appSRand();
	Rand.Y = appSRand();
	Rand.Z = appSRand();

	// Set the unit dir
	if (bPositive_X && bNegative_X)
	{
		UnitDir.X = Rand.X * 2 - 1;
	}
	else
	if (bPositive_X)
	{
		UnitDir.X = Rand.X;
	}
	else
	if (bNegative_X)
	{
		UnitDir.X = -Rand.X;
	}
	else
	{
		UnitDir.X = 0.0f;
	}

	if (bPositive_Y && bNegative_Y)
	{
		UnitDir.Y = Rand.Y * 2 - 1;
	}
	else
	if (bPositive_Y)
	{
		UnitDir.Y = Rand.Y;
	}
	else
	if (bNegative_Y)
	{
		UnitDir.Y = -Rand.Y;
	}
	else
	{
		UnitDir.Y = 0.0f;
	}

	if (bPositive_Z && bNegative_Z)
	{
		UnitDir.Z = Rand.Z * 2 - 1;
	}
	else
	if (bPositive_Z)
	{
		UnitDir.Z = Rand.Z;
	}
	else
	if (bNegative_Z)
	{
		UnitDir.Z = -Rand.Z;
	}
	else
	{
		UnitDir.Z = 0.0f;
	}
}

UBOOL UParticleModuleUberRainImpacts::IsCompatible(UParticleEmitter* InputEmitter)
{
	UBOOL bFoundAll	= FALSE;

	if (InputEmitter)
	{
		UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
		check(LODLevel);

		if (LODLevel->TypeDataModule)
		{
			if (LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
			{
				if (LODLevel->Modules.Num() == 6)
				{
					// They MUST be in this order
					// Lifetime
					// Size
					// InitialMeshRotation
					// SizeByLife
					// Cylinder
					// ColorOverLife
					if (LODLevel->Modules(0)->IsA(UParticleModuleLifetime::StaticClass())					&&
						LODLevel->Modules(1)->IsA(UParticleModuleSize::StaticClass())						&&
						LODLevel->Modules(2)->IsA(UParticleModuleMeshRotation::StaticClass())				&&
						LODLevel->Modules(3)->IsA(UParticleModuleSizeMultiplyLife::StaticClass())			&&
						LODLevel->Modules(4)->IsA(UParticleModuleLocationPrimitiveCylinder::StaticClass())	&&
						LODLevel->Modules(5)->IsA(UParticleModuleColorOverLife::StaticClass()))
					{
						bFoundAll	= TRUE;
					}
				}
			}
		}
	}

	return bFoundAll;
}

UBOOL UParticleModuleUberRainImpacts::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
	if (InputEmitter->LODLevels.Num() > 2)
	{
		appMsgf(AMT_OK, TEXT("Can't convert an emitter with specific LOD levels!"));
		return FALSE;
	}

	// Lifetime
	// Size
	// InitialMeshRotation
	// SizeByLife
	// Cylinder
	// ColorOverLife
	UParticleModuleLifetime*					LifetimeModule		= NULL;
	UParticleModuleSize*						SizeModule			= NULL;
	UParticleModuleMeshRotation*				MeshRotationModule	= NULL;
	UParticleModuleSizeMultiplyLife*			SizeByLifeModule	= NULL;
	UParticleModuleLocationPrimitiveCylinder*	CylinderModule		= NULL;
	UParticleModuleColorOverLife*				ColorOverLifeModule	= NULL;
	UObject*									DupObject;
	UBOOL										bMultipleModules	= FALSE;

	bIsUsingCylinder = TRUE;

	// Copy the module data from each module to the new one...
	UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
	check(LODLevel);

	UObject*	DestOuter	= this;

	for (INT SourceModuleIndex = 0; SourceModuleIndex < LODLevel->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= LODLevel->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime.Distribution, LifetimeModule->Lifetime.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				Lifetime.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize.Distribution, SizeModule->StartSize.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartSize.Distribution		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// InitialMeshRotation
		if (SourceModule->IsA(UParticleModuleMeshRotation::StaticClass()))
		{
			if (MeshRotationModule == NULL)
			{
				MeshRotationModule	= Cast<UParticleModuleMeshRotation>(SourceModule);
				DupObject		= StaticDuplicateObject(MeshRotationModule->StartRotation.Distribution, MeshRotationModule->StartRotation.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartRotation.Distribution	= Cast<UDistributionVector>(DupObject);
				bInheritParent	= MeshRotationModule->bInheritParent;
			}
			else
			{
			}
		}
		else
		// SizeByLife
		if (SourceModule->IsA(UParticleModuleSizeMultiplyLife::StaticClass()))
		{
			if (SizeByLifeModule == NULL)
			{
				SizeByLifeModule	= Cast<UParticleModuleSizeMultiplyLife>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeByLifeModule->LifeMultiplier.Distribution, SizeByLifeModule->LifeMultiplier.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				LifeMultiplier.Distribution	= Cast<UDistributionVector>(DupObject);
				MultiplyX		= SizeByLifeModule->MultiplyX;
				MultiplyY		= SizeByLifeModule->MultiplyY;
				MultiplyZ		= SizeByLifeModule->MultiplyZ;
			}
			else
			{
			}
		}
		else
		// PrimitiveCylinder
		if (SourceModule->IsA(UParticleModuleLocationPrimitiveCylinder::StaticClass()))
		{
			if (CylinderModule == NULL)
			{
				CylinderModule	= Cast<UParticleModuleLocationPrimitiveCylinder>(SourceModule);

				bIsUsingCylinder	= TRUE;
				bPositive_X			= CylinderModule->Positive_X;
				bPositive_Y			= CylinderModule->Positive_Y;
				bPositive_Z			= CylinderModule->Positive_Z;
				bNegative_X			= CylinderModule->Negative_X;
				bNegative_Y			= CylinderModule->Negative_Y;
				bNegative_Z			= CylinderModule->Negative_Z;
				bSurfaceOnly		= CylinderModule->SurfaceOnly;
				bVelocity			= CylinderModule->Velocity;
				DupObject			= StaticDuplicateObject(CylinderModule->VelocityScale.Distribution, CylinderModule->VelocityScale.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				PC_VelocityScale.Distribution	= Cast<UDistributionFloat>(DupObject);
				DupObject			= StaticDuplicateObject(CylinderModule->StartLocation.Distribution, CylinderModule->StartLocation.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				PC_StartLocation.Distribution	= Cast<UDistributionVector>(DupObject);
				bRadialVelocity		= CylinderModule->RadialVelocity;
				DupObject			= StaticDuplicateObject(CylinderModule->StartRadius.Distribution, CylinderModule->StartRadius.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				PC_StartRadius.Distribution		= Cast<UDistributionFloat>(DupObject);
				DupObject			= StaticDuplicateObject(CylinderModule->StartHeight.Distribution, CylinderModule->StartHeight.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				PC_StartHeight.Distribution		= Cast<UDistributionFloat>(DupObject);
				PC_HeightAxis		= CylinderModule->HeightAxis;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife.Distribution, ColorOverLifeModule->ColorOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				ColorOverLife.Distribution		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife.Distribution, ColorOverLifeModule->AlphaOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				AlphaOverLife.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(MeshRotationModule		== NULL) || 
		(SizeByLifeModule		== NULL) || 
		(CylinderModule			== NULL) || 
		(ColorOverLifeModule	== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	LODLevel->Modules.RemoveItem(LifetimeModule);
	LODLevel->Modules.RemoveItem(SizeModule);
	LODLevel->Modules.RemoveItem(MeshRotationModule);
	LODLevel->Modules.RemoveItem(SizeByLifeModule);
	LODLevel->Modules.RemoveItem(CylinderModule);
	LODLevel->Modules.RemoveItem(ColorOverLifeModule);

	// Add the Uber
	LODLevel->Modules.AddItem(this);

	//
	LODLevel->UpdateModuleLists();

	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberRainSplashA implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberRainSplashA);

void UParticleModuleUberRainSplashA::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// MeshRotation
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if (MeshInst)
	{
		FVector Rotation = StartRotation.GetValue(Owner->EmitterTime, Owner->Component);

		if (bInheritParent)
		{
			FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
			FVector		ParentAffectedRotation	= Rotator.Euler();
			Rotation.X	+= ParentAffectedRotation.X / 360.0f;
			Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
			Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
		}

		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		PayloadData->Rotation.X	+= Rotation.X * 360.0f;
		PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
		PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
	}

	// SizeByLife
	FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleUberRainSplashA::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// MeshRotation

		// SizeByLife
		FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color		= ColorFromVector(ColorVec, fAlpha);

	END_UPDATE_LOOP;
}

void UParticleModuleUberRainSplashA::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainSplashA*	LowModule	= Cast<UParticleModuleUberRainSplashA>(LowerLODModule);

	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetimeA	= Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetimeB	= LowModule->Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetime	= (MaxLifetimeA * Multiplier) + (MaxLifetimeB * (1.0f - Multiplier));
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector SizeA		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector SizeB		 = LowModule->StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Size		 = (SizeA * Multiplier)+ (SizeB * (1.0f - Multiplier));
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// MeshRotation
	FParticleMeshEmitterInstance* MeshInst = CastEmitterInstance<FParticleMeshEmitterInstance>(Owner);
	if (MeshInst)
	{
		FVector RotationA	= StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector RotationB	= LowModule->StartRotation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector Rotation	= (RotationA * Multiplier) + (RotationB * (1.0f - Multiplier));

		if (bInheritParent)
		{
			FRotator	Rotator	= Owner->Component->LocalToWorld.Rotator();
			FVector		ParentAffectedRotation	= Rotator.Euler();
			Rotation.X	+= ParentAffectedRotation.X / 360.0f;
			Rotation.Y	+= ParentAffectedRotation.Y / 360.0f;
			Rotation.Z	+= ParentAffectedRotation.Z / 360.0f;
		}

		FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshInst->MeshRotationOffset);
		PayloadData->Rotation.X	+= Rotation.X * 360.0f;
		PayloadData->Rotation.Y	+= Rotation.Y * 360.0f;
		PayloadData->Rotation.Z	+= Rotation.Z * 360.0f;
	}

	// SizeByLife
	FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// ColorOverLife
	FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

	FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

	Particle.Color		= ColorFromVector(ColorVec, Alpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleUberRainSplashA::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainSplashA*	LowModule	= Cast<UParticleModuleUberRainSplashA>(LowerLODModule);

	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// MeshRotation

		// SizeByLife
		FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// ColorOverLife
		FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

		Particle.Color		= ColorFromVector(ColorVec, Alpha);
	END_UPDATE_LOOP;
}

UBOOL UParticleModuleUberRainSplashA::IsCompatible(UParticleEmitter* InputEmitter)
{
	UBOOL bFoundAll	= FALSE;

	if (InputEmitter)
	{
		UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
		check(LODLevel);

		if (LODLevel->Modules.Num() == 5)
		{
			// They MUST be in this order
			// Lifetime
			// Size
			// MeshRotation Module Members
			// SizeByLife Module Members
			// ColorOverLife
			if (LODLevel->Modules(0)->IsA(UParticleModuleLifetime::StaticClass())					&&
				LODLevel->Modules(1)->IsA(UParticleModuleSize::StaticClass())						&&
				LODLevel->Modules(2)->IsA(UParticleModuleMeshRotation::StaticClass())				&&
				LODLevel->Modules(3)->IsA(UParticleModuleSizeMultiplyLife::StaticClass())			&&
				LODLevel->Modules(4)->IsA(UParticleModuleColorOverLife::StaticClass()))
			{
				bFoundAll	= TRUE;
			}
		}
	}

	return bFoundAll;
}

UBOOL UParticleModuleUberRainSplashA::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
	if (InputEmitter->LODLevels.Num() > 2)
	{
		appMsgf(AMT_OK, TEXT("Can't convert an emitter with specific LOD levels!"));
		return FALSE;
	}

	// Lifetime
	// Size
	// ColorOverLife
	UParticleModuleLifetime*					LifetimeModule		= NULL;
	UParticleModuleSize*						SizeModule			= NULL;
	UParticleModuleMeshRotation*				MeshRotationModule	= NULL;
	UParticleModuleSizeMultiplyLife*			SizeByLifeModule	= NULL;
	UParticleModuleColorOverLife*				ColorOverLifeModule	= NULL;
	UObject*									DupObject;
	UBOOL										bMultipleModules	= FALSE;

	// Copy the module data from each module to the new one...
	UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
	check(LODLevel);

	UObject*	DestOuter	= this;

	for (INT SourceModuleIndex = 0; SourceModuleIndex < LODLevel->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= LODLevel->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime.Distribution, LifetimeModule->Lifetime.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				Lifetime.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize.Distribution, SizeModule->StartSize.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartSize.Distribution		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// MeshRotation
		if (SourceModule->IsA(UParticleModuleMeshRotation::StaticClass()))
		{
			if (MeshRotationModule == NULL)
			{
				MeshRotationModule	= Cast<UParticleModuleMeshRotation>(SourceModule);
				DupObject			= StaticDuplicateObject(MeshRotationModule->StartRotation.Distribution, MeshRotationModule->StartRotation.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartRotation.Distribution		= Cast<UDistributionVector>(DupObject);
				bInheritParent		= MeshRotationModule->bInheritParent;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// SizeByLife
		if (SourceModule->IsA(UParticleModuleSizeMultiplyLife::StaticClass()))
		{
			if (SizeByLifeModule == NULL)
			{
				SizeByLifeModule	= Cast<UParticleModuleSizeMultiplyLife>(SourceModule);
				DupObject			= StaticDuplicateObject(SizeByLifeModule->LifeMultiplier.Distribution, SizeByLifeModule->LifeMultiplier.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				LifeMultiplier.Distribution		= Cast<UDistributionVector>(DupObject);
				MultiplyX			= SizeByLifeModule->MultiplyX;
				MultiplyY			= SizeByLifeModule->MultiplyY;
				MultiplyZ			= SizeByLifeModule->MultiplyZ;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife.Distribution, ColorOverLifeModule->ColorOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				ColorOverLife.Distribution		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife.Distribution, ColorOverLifeModule->AlphaOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				AlphaOverLife.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(MeshRotationModule		== NULL) || 
		(SizeByLifeModule		== NULL) || 
		(ColorOverLifeModule	== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	LODLevel->Modules.RemoveItem(LifetimeModule);
	LODLevel->Modules.RemoveItem(SizeModule);
	LODLevel->Modules.RemoveItem(MeshRotationModule);
	LODLevel->Modules.RemoveItem(SizeByLifeModule);
	LODLevel->Modules.RemoveItem(ColorOverLifeModule);

	// Add the Uber
	LODLevel->Modules.AddItem(this);

	//
	LODLevel->UpdateModuleLists();

	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleUberRainSplashB implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleUberRainSplashB);

void UParticleModuleUberRainSplashB::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// ColorOverLife
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	Particle.BaseColor	= Particle.Color;

	// SizeByLife
	FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// InitialRotRate
}

void UParticleModuleUberRainSplashB::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// ColorOverLife
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color		= ColorFromVector(ColorVec, fAlpha);

		// SizeByLife
		FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// InitialRotRate
	END_UPDATE_LOOP;
}

void UParticleModuleUberRainSplashB::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainSplashB*	LowModule	= Cast<UParticleModuleUberRainSplashB>(LowerLODModule);

	UBOOL	bUseLocalSpace	= LODLevel->RequiredModule->bUseLocalSpace;

	SPAWN_INIT;

	// Lifetime
	FLOAT MaxLifetimeA	= Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetimeB	= LowModule->Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetime	= (MaxLifetimeA * Multiplier) + (MaxLifetimeB * (1.0f - Multiplier));
	if (Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;

	// Size
	FVector SizeA		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector SizeB		 = LowModule->StartSize.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Size		 = (SizeA * Multiplier)+ (SizeB * (1.0f - Multiplier));
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;

	// ColorOverLife
	FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

	FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

	Particle.Color		= ColorFromVector(ColorVec, Alpha);
	Particle.BaseColor	= Particle.Color;

	// SizeByLife
	FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
	if (MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if (MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if (MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}

	// InitialRotRate
}

void UParticleModuleUberRainSplashB::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleUberRainSplashB*	LowModule	= Cast<UParticleModuleUberRainSplashB>(LowerLODModule);

	BEGIN_UPDATE_LOOP;

		// Lifetime

		// Size

		// ColorOverLife
		FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

		Particle.Color		= ColorFromVector(ColorVec, Alpha);

		// SizeByLife
		FVector SizeScaleA	= LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScaleB	= LowModule->LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
		FVector SizeScale	= (SizeScaleA * Multiplier) + (SizeScaleB * (1.0f - Multiplier));
		if (MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if (MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if (MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}

		// InitialRotRate
	END_UPDATE_LOOP;
}

UBOOL UParticleModuleUberRainSplashB::IsCompatible(UParticleEmitter* InputEmitter)
{
	UBOOL bFoundAll	= FALSE;

	if (InputEmitter)
	{
		UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
		check(LODLevel);

		if (LODLevel->Modules.Num() == 5)
		{
			// They MUST be in this order
			// Lifetime
			// Size
			// ColorOverLife
			// SizeByLife
			// InitialRotRate
			if (LODLevel->Modules(0)->IsA(UParticleModuleLifetime::StaticClass())					&&
				LODLevel->Modules(1)->IsA(UParticleModuleSize::StaticClass())						&&
				LODLevel->Modules(2)->IsA(UParticleModuleColorOverLife::StaticClass())				&&
				LODLevel->Modules(3)->IsA(UParticleModuleSizeMultiplyLife::StaticClass())			&&
				LODLevel->Modules(4)->IsA(UParticleModuleRotationRate::StaticClass())
				)
			{
				bFoundAll	= TRUE;
			}
		}
	}

	return bFoundAll;
}

UBOOL UParticleModuleUberRainSplashB::ConvertToUberModule(UParticleEmitter* InputEmitter)
{
	if (InputEmitter->LODLevels.Num() > 2)
	{
		appMsgf(AMT_OK, TEXT("Can't convert an emitter with specific LOD levels!"));
		return FALSE;
	}

	// Lifetime
	// Size
	// ColorOverLife
	// SizeByLife
	// InitialRotRate
	UParticleModuleLifetime*					LifetimeModule		= NULL;
	UParticleModuleSize*						SizeModule			= NULL;
	UParticleModuleColorOverLife*				ColorOverLifeModule	= NULL;
	UParticleModuleSizeMultiplyLife*			SizeByLifeModule	= NULL;
	UParticleModuleRotationRate*				InitRotRateModule	= NULL;
	UObject*									DupObject;
	UBOOL										bMultipleModules	= FALSE;

	// Copy the module data from each module to the new one...
	UParticleLODLevel* LODLevel	= InputEmitter->LODLevels(0);
	check(LODLevel);

	UObject*	DestOuter	= this;

	for (INT SourceModuleIndex = 0; SourceModuleIndex < LODLevel->Modules.Num(); SourceModuleIndex++)
	{
		UParticleModule* SourceModule	= LODLevel->Modules(SourceModuleIndex);

		// Lifetime
		if (SourceModule->IsA(UParticleModuleLifetime::StaticClass()))
		{
			// Copy the contents
			if (LifetimeModule == NULL)
			{
				LifetimeModule	= Cast<UParticleModuleLifetime>(SourceModule);
				DupObject		= StaticDuplicateObject(LifetimeModule->Lifetime.Distribution, LifetimeModule->Lifetime.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				Lifetime.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// Size
		if (SourceModule->IsA(UParticleModuleSize::StaticClass()))
		{
			if (SizeModule == NULL)
			{
				SizeModule		= Cast<UParticleModuleSize>(SourceModule);
				DupObject		= StaticDuplicateObject(SizeModule->StartSize.Distribution, SizeModule->StartSize.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartSize.Distribution		= Cast<UDistributionVector>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// ColorOverLife
		if (SourceModule->IsA(UParticleModuleColorOverLife::StaticClass()))
		{
			if (ColorOverLifeModule == NULL)
			{
				ColorOverLifeModule	= Cast<UParticleModuleColorOverLife>(SourceModule);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->ColorOverLife.Distribution, ColorOverLifeModule->ColorOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				ColorOverLife.Distribution		= Cast<UDistributionVector>(DupObject);
				DupObject			= StaticDuplicateObject(ColorOverLifeModule->AlphaOverLife.Distribution, ColorOverLifeModule->AlphaOverLife.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				AlphaOverLife.Distribution		= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// SizeByLife
		if (SourceModule->IsA(UParticleModuleSizeMultiplyLife::StaticClass()))
		{
			if (SizeByLifeModule == NULL)
			{
				SizeByLifeModule	= Cast<UParticleModuleSizeMultiplyLife>(SourceModule);
				DupObject			= StaticDuplicateObject(SizeByLifeModule->LifeMultiplier.Distribution, SizeByLifeModule->LifeMultiplier.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				LifeMultiplier.Distribution		= Cast<UDistributionVector>(DupObject);
				MultiplyX			= SizeByLifeModule->MultiplyX;
				MultiplyY			= SizeByLifeModule->MultiplyY;
				MultiplyZ			= SizeByLifeModule->MultiplyZ;
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		// InitialRotRate
		if (SourceModule->IsA(UParticleModuleRotationRate::StaticClass()))
		{
			if (InitRotRateModule == NULL)
			{
				InitRotRateModule	= Cast<UParticleModuleRotationRate>(SourceModule);
				DupObject			= StaticDuplicateObject(InitRotRateModule->StartRotationRate.Distribution, InitRotRateModule->StartRotationRate.Distribution, DestOuter, TEXT("None"));
				check(DupObject);
				StartRotationRate.Distribution	= Cast<UDistributionFloat>(DupObject);
			}
			else
			{
				// Error - multiple modules...
				bMultipleModules	= TRUE;
			}
		}
		else
		{
			// Just leave the module in its place...
		}
	}

	// Ensure that we found all the required modules
	if ((LifetimeModule			== NULL) || 
		(SizeModule				== NULL) || 
		(ColorOverLifeModule	== NULL) || 
		(SizeByLifeModule		== NULL) || 
		(InitRotRateModule		== NULL))
	{
		// Failed the conversion!
		return FALSE;
	}

	// Remove the modules
	LODLevel->Modules.RemoveItem(LifetimeModule);
	LODLevel->Modules.RemoveItem(SizeModule);
	LODLevel->Modules.RemoveItem(ColorOverLifeModule);
	LODLevel->Modules.RemoveItem(SizeByLifeModule);
	LODLevel->Modules.RemoveItem(InitRotRateModule);

	// Add the Uber
	LODLevel->Modules.AddItem(this);

	//
	LODLevel->UpdateModuleLists();

	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocity implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocity);

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	FVector FromOrigin;
	FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;

}

void UParticleModuleVelocity::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleVelocity*	LowModule	= Cast<UParticleModuleVelocity>(LowerLODModule);

	SPAWN_INIT;

	FVector FromOrigin;
	FVector VelA	= StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	FVector VelB	= LowModule->StartVelocity.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Vel		= (VelA * Multiplier) + (VelB * (1.0f - Multiplier));

	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		FromOrigin = Particle.Location.SafeNormal();
	}
	else
	{
		FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
		Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
	}

	FLOAT	RadialA = StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	RadialB = LowModule->StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	Radial	= ((RadialA * Multiplier) + (RadialB * (1.0f - Multiplier)));
	Vel += FromOrigin * Radial;

	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityInheritParent implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocityInheritParent);

void UParticleModuleVelocityInheritParent::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	FVector Vel = FVector(0.0f);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Vel = Owner->Component->LocalToWorld.InverseTransformNormal(Owner->Component->PartSysVelocity);
	}
	else
	{
		Vel = Owner->Component->PartSysVelocity;
	}

	FVector vScale = Scale.GetValue(Owner->EmitterTime, Owner->Component);

	Vel *= vScale;

	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;
}

void UParticleModuleVelocityInheritParent::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleVelocityInheritParent*	LowModule	= Cast<UParticleModuleVelocityInheritParent>(LowerLODModule);

	SPAWN_INIT;

	FVector Vel = FVector(0.0f);

	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Vel = Owner->Component->LocalToWorld.InverseTransformNormal(Owner->Component->PartSysVelocity);
	}
	else
	{
		Vel = Owner->Component->PartSysVelocity;
	}

	FVector ScaleA	= Scale.GetValue(Owner->EmitterTime, Owner->Component);
	FVector ScaleB	= LowModule->Scale.GetValue(Owner->EmitterTime, Owner->Component);
	FVector Scale	= (ScaleA * Multiplier) + (ScaleB * (1.0f - Multiplier));

	Vel *= Scale;

	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityOverLifetime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocityOverLifetime);

void UParticleModuleVelocityOverLifetime::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	if (Absolute)
	{
		SPAWN_INIT;
		FVector Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Velocity		= Vel;
		Particle.BaseVelocity	= Vel;
	}
}

void UParticleModuleVelocityOverLifetime::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	BEGIN_UPDATE_LOOP;
		FVector Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		// If Absolute, set it directly.
		// If not, use it as a multiplier.
		if (Absolute)
		{
			// We need to take local space into account in this case
			//@todo. Should this be done in both cases (Absolute and not)???
			if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
			{
				Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
			}
			Particle.Velocity	 = Vel;
		}
		else
		{
			Particle.Velocity	*= Vel;
		}
	END_UPDATE_LOOP;
}

void UParticleModuleVelocityOverLifetime::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleVelocityOverLifetime*	LowModule	= Cast<UParticleModuleVelocityOverLifetime>(LowerLODModule);

	if (Absolute)
	{
		SPAWN_INIT;
		FVector VelA	= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector VelB	= LowModule->VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector Vel		= (VelA * Multiplier) + (VelB * (1.0f - Multiplier));
		Particle.Velocity		= Vel;
		Particle.BaseVelocity	= Vel;
	}
}

void UParticleModuleVelocityOverLifetime::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*						LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleVelocityOverLifetime*	LowModule	= Cast<UParticleModuleVelocityOverLifetime>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector VelA	= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector VelB	= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector Vel		= (VelA * Multiplier) + (VelB * (1.0f - Multiplier));
		// If Absolute, set it directly.
		// If not, use it as a multiplier.
		if (Absolute)
		{
			// We need to take local space into account in this case
			//@todo. Should this be done in both cases (Absolute and not)???
			if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
			{
				Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
			}
			Particle.Velocity	 = Vel;
		}
		else
		{
			Particle.Velocity	*= Vel;
		}
	END_UPDATE_LOOP;
}

/*-----------------------------------------------------------------------------
	UParticleModuleColor implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleColor);

void UParticleModuleColor::PostLoad()
{
	Super::PostLoad();

	// We need to convert distributions from [0..256] to [0..1].
	if (GetLinker() && (GetLinker()->Ver() < VER_PARTICLESYSTEM_LINEARCOLOR_SUPPORT))
	{
		ConvertColorVectorDistribution(StartColor.Distribution);
		ConvertColorFloatDistribution(StartAlpha.Distribution);

		MarkPackageDirty();
	}
}

void UParticleModuleColor::PostEditChange(UProperty* PropertyThatChanged)
{
	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("bClampAlpha")))
		{
			UObject* OuterObj = GetOuter();
			check(OuterObj);
			UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
			if (LODLevel)
			{
				// The outer is incorrect - warn the user and handle it
				warnf(NAME_Warning, TEXT("UParticleModuleColor has an incorrect outer... run FixupEmitters on package %s"),
					*(OuterObj->GetOutermost()->GetPathName()));
				OuterObj = LODLevel->GetOuter();
				UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
				check(Emitter);
				OuterObj = Emitter->GetOuter();
			}
			UParticleSystem* PartSys = CastChecked<UParticleSystem>(OuterObj);
			PartSys->UpdateColorModuleClampAlpha(this);
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

void UParticleModuleColor::AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup)
{
	// Iterate over object and find any InterpCurveFloats or UDistributionFloats
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		// attempt to get a distribution from a random struct property
		UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(*It, (BYTE*)this);
		if (Distribution)
		{
			if(Distribution->IsA(UDistributionFloat::StaticClass()))
			{
				if (bClampAlpha == TRUE)
				{
					EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE, TRUE, 0.0f, 1.0f);
				}
				else
				{
					EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE);
				}
			}
			else
			{
				EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE);
			}
		}
	}
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector ColorVec	= StartColor.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	fAlpha		= StartAlpha.GetValue(Owner->EmitterTime, Owner->Component);
	Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleColor::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*		LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleColor*	LowModule	= Cast<UParticleModuleColor>(LowerLODModule);

	SPAWN_INIT;
	FVector ColorVecA	= StartColor.GetValue(Owner->EmitterTime, Owner->Component);
	FVector ColorVecB	= LowModule->StartColor.GetValue(Owner->EmitterTime, Owner->Component);
	FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

	FLOAT	AlphaA		= StartAlpha.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	AlphaB		= LowModule->StartAlpha.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

	Particle.Color		= ColorFromVector(ColorVec, Alpha);
	Particle.BaseColor	= Particle.Color;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleColor::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstant* StartColorDist = Cast<UDistributionVectorConstant>(StartColor.Distribution);
	if (StartColorDist)
	{
		StartColorDist->Constant = FVector(1.0f,1.0f,1.0f);
		StartColorDist->bIsDirty = TRUE;
	}

}

/*-----------------------------------------------------------------------------
	UParticleModuleColorByParameter implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleColorByParameter);

void UParticleModuleColorByParameter::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	UParticleSystemComponent* Component = Owner->Component;

	UBOOL bFound = FALSE;
	for (INT i = 0; i < Component->InstanceParameters.Num(); i++)
	{
		FParticleSysParam Param = Component->InstanceParameters(i);
		if (Param.Name == ColorParam)
		{
			Particle.Color.R = Clamp<FLOAT>((FLOAT)Param.Color.R / 255.9f, 0.f, 1.f);
			Particle.Color.G = Clamp<FLOAT>((FLOAT)Param.Color.G / 255.9f, 0.f, 1.f);
			Particle.Color.B = Clamp<FLOAT>((FLOAT)Param.Color.B / 255.9f, 0.f, 1.f);
			Particle.Color.A = Clamp<FLOAT>((FLOAT)Param.Color.A / 255.9f, 0.f, 1.f);

			bFound	= TRUE;
			break;
		}
	}

	if (!bFound)
	{
		Particle.Color	= DefaultColor;
	}
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleColorByParameter::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	// Just call the standard update.
	Spawn(Owner, Offset, SpawnTime);
}

void UParticleModuleColorByParameter::AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp)
{
	UBOOL	bFound	= FALSE;

	for (INT i = 0; i < PSysComp->InstanceParameters.Num(); i++)
	{
		FParticleSysParam* Param = &(PSysComp->InstanceParameters(i));
		
		if (Param->Name == ColorParam)
		{
			bFound	=	TRUE;
			break;
		}
	}

	if (!bFound)
	{
		INT NewParamIndex = PSysComp->InstanceParameters.AddZeroed();
		PSysComp->InstanceParameters(NewParamIndex).Name		= ColorParam;
		PSysComp->InstanceParameters(NewParamIndex).ParamType	= PSPT_Color;
		PSysComp->InstanceParameters(NewParamIndex).Actor		= NULL;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleColorOverLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleColorOverLife);

void UParticleModuleColorOverLife::PostLoad()
{
	Super::PostLoad();

	// We need to convert distributions from [0..256] to [0..1].
	if (GetLinker() && (GetLinker()->Ver() < VER_PARTICLESYSTEM_LINEARCOLOR_SUPPORT))
	{
		ConvertColorVectorDistribution(ColorOverLife.Distribution);
		ConvertColorFloatDistribution(AlphaOverLife.Distribution);

		MarkPackageDirty();
	}
}

void UParticleModuleColorOverLife::PostEditChange(UProperty* PropertyThatChanged)
{
	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("bClampAlpha")))
		{
			UObject* OuterObj = GetOuter();
			check(OuterObj);
			UParticleLODLevel* LODLevel = Cast<UParticleLODLevel>(OuterObj);
			if (LODLevel)
			{
				// The outer is incorrect - warn the user and handle it
				warnf(NAME_Warning, TEXT("UParticleModuleColorOverLife has an incorrect outer... run FixupEmitters on package %s"),
					*(OuterObj->GetOutermost()->GetPathName()));
				OuterObj = LODLevel->GetOuter();
				UParticleEmitter* Emitter = Cast<UParticleEmitter>(OuterObj);
				check(Emitter);
				OuterObj = Emitter->GetOuter();
			}
			UParticleSystem* PartSys = CastChecked<UParticleSystem>(OuterObj);

			PartSys->UpdateColorModuleClampAlpha(this);
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

void UParticleModuleColorOverLife::AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup)
{
	// Iterate over object and find any InterpCurveFloats or UDistributionFloats
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		// attempt to get a distribution from a random struct property
		UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(*It, (BYTE*)this);
		if (Distribution)
		{
			if(Distribution->IsA(UDistributionFloat::StaticClass()))
			{
				// We are assuming that this is the alpha...
				if (bClampAlpha == TRUE)
				{
					EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE, TRUE, 0.0f, 1.0f);
				}
				else
				{
					EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE);
				}
			}
			else
			{
				// We are assuming that this is the color...
				EdSetup->AddCurveToCurrentTab(Distribution, It->GetName(), ModuleEditorColor, TRUE, TRUE);
			}
		}
	}
}

void UParticleModuleColorOverLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FVector ColorVec	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	fAlpha		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Color		= ColorFromVector(ColorVec, fAlpha);
	END_UPDATE_LOOP;
}

void UParticleModuleColorOverLife::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleColorOverLife*	LowModule	= Cast<UParticleModuleColorOverLife>(LowerLODModule);

	SPAWN_INIT;
	FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

	FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

	Particle.Color		= ColorFromVector(ColorVec, Alpha);
	Particle.BaseColor	= Particle.Color;
}

void UParticleModuleColorOverLife::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleColorOverLife*	LowModule	= Cast<UParticleModuleColorOverLife>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector ColorVecA	= ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVecB	= LowModule->ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));

		Particle.Color		= ColorFromVector(ColorVec, Alpha);
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleColorOverLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	ColorOverLife.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* ColorOverLifeDist = Cast<UDistributionVectorConstantCurve>(ColorOverLife.Distribution);
	if (ColorOverLifeDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = ColorOverLifeDist->CreateNewKey(Key * 1.0f);
			for (INT SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				if (Key == 0)
				{
					ColorOverLifeDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
				}
				else
				{
					ColorOverLifeDist->SetKeyOut(SubIndex, KeyIndex, 0.0f);
				}
			}
		}
		ColorOverLifeDist->bIsDirty = TRUE;
	}

	AlphaOverLife.Distribution = Cast<UDistributionFloatConstantCurve>(StaticConstructObject(UDistributionFloatConstantCurve::StaticClass(), this));
	UDistributionFloatConstantCurve* AlphaOverLifeDist = Cast<UDistributionFloatConstantCurve>(AlphaOverLife.Distribution);
	if (AlphaOverLifeDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = AlphaOverLifeDist->CreateNewKey(Key * 1.0f);
			AlphaOverLifeDist->SetKeyOut(0, KeyIndex, 1.0f);
		}
		AlphaOverLifeDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleColorScaleOverLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleColorScaleOverLife);

void UParticleModuleColorScaleOverLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector ColorVec;
	FLOAT	fAlpha;

	if (bEmitterTime)
	{
		ColorVec	= ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
		fAlpha		= AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
	}
	else
	{
		ColorVec	= ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		fAlpha		= AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
	}

	Particle.Color.R *= ColorVec.X;
	Particle.Color.G *= ColorVec.Y;
	Particle.Color.B *= ColorVec.Z;
	Particle.Color.A *= fAlpha;
}

void UParticleModuleColorScaleOverLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{ 
	const FRawDistribution* FastColorScaleOverLife = ColorScaleOverLife.GetFastRawDistribution();
	const FRawDistribution* FastAlphaScaleOverLife = AlphaScaleOverLife.GetFastRawDistribution();
	FVector ColorVec;
	FLOAT	fAlpha;
	if( FastColorScaleOverLife && FastAlphaScaleOverLife )
	{
		// fast path
		BEGIN_UPDATE_LOOP;
			if (bEmitterTime)
			{
				FastColorScaleOverLife->GetValue3None(Owner->EmitterTime, &ColorVec.X);
				FastAlphaScaleOverLife->GetValue1None(Owner->EmitterTime, &fAlpha);
			}
			else
			{
				FastColorScaleOverLife->GetValue3None(Particle.RelativeTime, &ColorVec.X);
				FastAlphaScaleOverLife->GetValue1None(Particle.RelativeTime, &fAlpha);
			}
			Particle.Color.R *= ColorVec.X;
			Particle.Color.G *= ColorVec.Y;
			Particle.Color.B *= ColorVec.Z;
			Particle.Color.A *= fAlpha;
		END_UPDATE_LOOP;
	}
	else
	{
		BEGIN_UPDATE_LOOP;
			if (bEmitterTime)
			{
				ColorVec	= ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
				fAlpha		= AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
			}
			else
			{
				ColorVec	= ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
				fAlpha		= AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
			}
			Particle.Color.R *= ColorVec.X;
			Particle.Color.G *= ColorVec.Y;
			Particle.Color.B *= ColorVec.Z;
			Particle.Color.A *= fAlpha;
		END_UPDATE_LOOP;
	}
}

void UParticleModuleColorScaleOverLife::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleColorScaleOverLife*	LowModule	= Cast<UParticleModuleColorScaleOverLife>(LowerLODModule);

	SPAWN_INIT;
	FVector ColorVec;
	FLOAT	Alpha;

	if (bEmitterTime)
	{
		FVector	ColorVecA	= ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
		FVector	ColorVecB	= LowModule->ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
		ColorVec			= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
		Alpha				= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));
	}
	else
	{
		FVector	ColorVecA	= ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FVector	ColorVecB	= LowModule->ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		ColorVec			= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));

		FLOAT	AlphaA		= AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		FLOAT	AlphaB		= LowModule->AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
		Alpha				= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));
	}

	Particle.Color.R *= ColorVec.X;
	Particle.Color.G *= ColorVec.Y;
	Particle.Color.B *= ColorVec.Z;
	Particle.Color.A *= Alpha;
}

void UParticleModuleColorScaleOverLife::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleColorScaleOverLife*	LowModule	= Cast<UParticleModuleColorScaleOverLife>(LowerLODModule);

	BEGIN_UPDATE_LOOP;
		FVector ColorVec;
		FLOAT	Alpha;

		if (bEmitterTime)
		{
			FVector	ColorVecA	= ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
			FVector	ColorVecB	= ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
			FLOAT	AlphaA		= AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
			FLOAT	AlphaB		= AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);

			ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));
			Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));
		}
		else
		{
			FVector	ColorVecA	= ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
			FVector	ColorVecB	= ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
			FLOAT	AlphaA		= AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
			FLOAT	AlphaB		= AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);

			ColorVec	= (ColorVecA * Multiplier) + (ColorVecB * (1.0f - Multiplier));
			Alpha		= (AlphaA * Multiplier) + (AlphaB * (1.0f - Multiplier));
		}

		Particle.Color.R *= ColorVec.X;
		Particle.Color.G *= ColorVec.Y;
		Particle.Color.B *= ColorVec.Z;
		Particle.Color.A *= Alpha;
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleColorScaleOverLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	ColorScaleOverLife.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* ColorScaleOverLifeDist = Cast<UDistributionVectorConstantCurve>(ColorScaleOverLife.Distribution);
	if (ColorScaleOverLifeDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = ColorScaleOverLifeDist->CreateNewKey(Key * 1.0f);
			for (INT SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				ColorScaleOverLifeDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
			}
		}
		ColorScaleOverLifeDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleKillBox implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleKillBox);

	//## BEGIN PROPS ParticleModuleKillBox
//	struct FRawDistributionVector LowerLeftCorner;
//	struct FRawDistributionVector UpperRightCorner;
//	BITFIELD bAbsolute:1;
	//## END PROPS ParticleModuleKillBox

void UParticleModuleKillBox::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);

	FVector CheckLL = LowerLeftCorner.GetValue(Owner->EmitterTime, Owner->Component);
	FVector CheckUR = UpperRightCorner.GetValue(Owner->EmitterTime, Owner->Component);
	if (bAbsolute == FALSE)
	{
		CheckLL += Owner->Component->LocalToWorld.GetOrigin();
		CheckUR += Owner->Component->LocalToWorld.GetOrigin();
	}
	FBox CheckBox = FBox(CheckLL, CheckUR);

	BEGIN_UPDATE_LOOP;
	{
		FVector Position = Particle.Location;

		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			Position = Owner->Component->LocalToWorld.TransformNormal(Position);
		}

		// Determine if the particle is inside the box
		UBOOL bIsInside = CheckBox.IsInside(Position);

		// If we are killing on the inside, and it is inside
		//	OR
		// If we are killing on the outside, and it is outside
		// kill the particle
		if (bKillInside == bIsInside)
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleKillBox::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleKillBox*	LowModule = Cast<UParticleModuleKillBox>(LowerLODModule);

	FVector HighCheckLL = LowerLeftCorner.GetValue(Owner->EmitterTime, Owner->Component);
	FVector HighCheckUR = UpperRightCorner.GetValue(Owner->EmitterTime, Owner->Component);
	FVector LowCheckLL = LowModule->LowerLeftCorner.GetValue(Owner->EmitterTime, Owner->Component);
	FVector LowCheckUR = LowModule->UpperRightCorner.GetValue(Owner->EmitterTime, Owner->Component);
	if (bAbsolute == FALSE)
	{
		HighCheckLL += Owner->Component->LocalToWorld.GetOrigin();
		HighCheckUR += Owner->Component->LocalToWorld.GetOrigin();
		LowCheckLL += Owner->Component->LocalToWorld.GetOrigin();
		LowCheckUR += Owner->Component->LocalToWorld.GetOrigin();
	}
	FVector CheckLL = (HighCheckLL * Multiplier) + (LowCheckLL * (1.0f - Multiplier));
	FVector CheckUR = (HighCheckUR * Multiplier) + (LowCheckUR * (1.0f - Multiplier));

	FBox CheckBox = FBox(CheckLL, CheckUR);

	BEGIN_UPDATE_LOOP;
	{
		FVector Position = Particle.Location;

		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			Position = Owner->Component->LocalToWorld.TransformNormal(Position);
		}

		// Determine if the particle is inside the box
		UBOOL bIsInside = CheckBox.IsInside(Position);

		// If we are killing on the inside, and it is inside
		//	OR
		// If we are killing on the outside, and it is outside
		// kill the particle
		if (bKillInside == bIsInside)
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleKillBox::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	FVector CheckLL = LowerLeftCorner.GetValue(Owner->EmitterTime, Owner->Component);
	FVector CheckUR = UpperRightCorner.GetValue(Owner->EmitterTime, Owner->Component);
	if (bAbsolute == FALSE)
	{
		CheckLL += Owner->Component->LocalToWorld.GetOrigin();
		CheckUR += Owner->Component->LocalToWorld.GetOrigin();
	}
	FBox CheckBox = FBox(CheckLL, CheckUR);

	DrawWireBox(PDI, CheckBox, ModuleEditorColor, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleKillHeight implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleKillHeight);

	//## BEGIN PROPS ParticleModuleKillHeight
//	struct FRawDistributionFloat Height;
//	BITFIELD bAbsolute:1;
	//## END PROPS ParticleModuleKillHeight
void UParticleModuleKillHeight::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);

	FLOAT CheckHeight = Height.GetValue(Owner->EmitterTime, Owner->Component);
	if (bAbsolute == FALSE)
	{
		CheckHeight += Owner->Component->LocalToWorld.GetOrigin().Z;
	}

	BEGIN_UPDATE_LOOP;
	{
		FVector Position = Particle.Location;

		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			Position = Owner->Component->LocalToWorld.TransformNormal(Position);
		}

		if ((bFloor == TRUE) && (Position.Z < CheckHeight))
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
		else
		if ((bFloor == FALSE) && (Position.Z > CheckHeight))
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleKillHeight::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, 
	FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleKillHeight*	LowModule	= Cast<UParticleModuleKillHeight>(LowerLODModule);

	FLOAT HighHeight = Height.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT LowHeight = LowModule->Height.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT CheckHeight = (HighHeight * Multiplier) + (LowHeight * (1.0f - Multiplier));
	if (bAbsolute == FALSE)
	{
		CheckHeight += Owner->Component->LocalToWorld.GetOrigin().Z;
	}

	BEGIN_UPDATE_LOOP;
	{
		FVector Position = Particle.Location;

		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			Position = Owner->Component->LocalToWorld.TransformNormal(Position);
		}

		if ((bFloor == TRUE) && (Position.Z < CheckHeight))
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
		else
		if ((bFloor == FALSE) && (Position.Z > CheckHeight))
		{
			// Kill the particle...
			Owner->KillParticle(i);
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleKillHeight::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	FVector OwnerPosition = Owner->Component->LocalToWorld.GetOrigin();
	FLOAT CheckHeight = Height.GetValue(Owner->EmitterTime, Owner->Component);
	if (bAbsolute == FALSE)
	{
		CheckHeight += OwnerPosition.Z;
	}

	FVector Pt1 = FVector(OwnerPosition.X - 100.0f, OwnerPosition.Y - 100.0f, CheckHeight);
	FVector Pt2 = FVector(OwnerPosition.X + 100.0f, OwnerPosition.Y - 100.0f, CheckHeight);
	FVector Pt3 = FVector(OwnerPosition.X - 100.0f, OwnerPosition.Y + 100.0f, CheckHeight);
	FVector Pt4 = FVector(OwnerPosition.X + 100.0f, OwnerPosition.Y + 100.0f, CheckHeight);

	PDI->DrawLine(Pt1, Pt2, ModuleEditorColor, SDPG_World);
	PDI->DrawLine(Pt1, Pt3, ModuleEditorColor, SDPG_World);
	PDI->DrawLine(Pt2, Pt4, ModuleEditorColor, SDPG_World);
	PDI->DrawLine(Pt3, Pt4, ModuleEditorColor, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleLifetime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleLifetime);

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FLOAT MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	if(Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;
}

void UParticleModuleLifetime::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleLifetime*	LowModule	= Cast<UParticleModuleLifetime>(LowerLODModule);

	SPAWN_INIT;
	FLOAT MaxLifetimeA	= Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetimeB	= LowModule->Lifetime.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT MaxLifetime	= (MaxLifetimeA * Multiplier) + (MaxLifetimeB * (1.0f - Multiplier));
	if(Particle.OneOverMaxLifetime > 0.f)
	{
		// Another module already modified lifetime.
		Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
	}
	else
	{
		// First module to modify lifetime.
		Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
	}
	Particle.RelativeTime = SpawnTime * Particle.OneOverMaxLifetime;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleLifetime::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionFloatUniform* LifetimeDist = Cast<UDistributionFloatUniform>(Lifetime.Distribution);
	if (LifetimeDist)
	{
		LifetimeDist->Min = 1.0f;
		LifetimeDist->Max = 1.0f;
		LifetimeDist->bIsDirty = TRUE;
	}
}

FLOAT UParticleModuleLifetime::GetMaxLifetime()
{
	// Check the distribution for the max value
	FLOAT Min, Max;
	Lifetime.GetOutRange(Min, Max);
	return Max;
}

/*-----------------------------------------------------------------------------
	UParticleModuleAttractorLine implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleAttractorLine);

void UParticleModuleAttractorLine::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	FVector Line = EndPoint1 - EndPoint0;
	FVector LineNorm = Line;
	LineNorm.Normalize();

	BEGIN_UPDATE_LOOP;
		// Determine the position of the particle projected on the line
		FVector AdjustedLocation = Particle.Location - Owner->Component->LocalToWorld.GetOrigin();
		FVector EP02Particle = AdjustedLocation - EndPoint0;

		FVector ProjectedParticle = Line * (Line | EP02Particle) / (Line | Line);

		// Determine the 'ratio' of the line that has been traveled by the particle
		FLOAT VRatioX = 0.0f;
		FLOAT VRatioY = 0.0f;
		FLOAT VRatioZ = 0.0f;

		if (Line.X)
			VRatioX = (ProjectedParticle.X - EndPoint0.X) / Line.X;
		if (Line.Y)
			VRatioY = (ProjectedParticle.Y - EndPoint0.Y) / Line.Y;
		if (Line.Z)
			VRatioZ = (ProjectedParticle.Z - EndPoint0.Z) / Line.Z;

		bool bProcess = false;
		FLOAT fRatio = 0.0f;

		if (VRatioX || VRatioY || VRatioZ)
		{
			// If there are multiple ratios, they should be the same...
			if (VRatioX)
				fRatio = VRatioX;
			else
			if (VRatioY)
				fRatio = VRatioY;
			else
			if (VRatioZ)
				fRatio = VRatioZ;
		}

		if ((fRatio >= 0.0f) && (fRatio <= 1.0f))
			bProcess = true;

		if (bProcess)
		{
			// Look up the Range and Strength at that position on the line
			FLOAT AttractorRange = Range.GetValue(fRatio, Owner->Component);
	        
			FVector LineToPoint = AdjustedLocation - ProjectedParticle;
    		FLOAT Distance = LineToPoint.Size();

			if ((AttractorRange > 0) && (Distance <= AttractorRange))
			{
				// Adjust the strength based on the range ratio
				FLOAT AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange, Owner->Component);
				FVector Direction = LineToPoint^Line;
    			// Adjust the VELOCITY of the particle based on the attractor... 
        		Particle.Velocity += Direction * AttractorStrength * DeltaTime;
			}
		}
	END_UPDATE_LOOP;
}

void UParticleModuleAttractorLine::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleAttractorLine*	LowModule	= Cast<UParticleModuleAttractorLine>(LowerLODModule);

	FVector Line = EndPoint1 - EndPoint0;
	FVector LineNorm = Line;
	LineNorm.Normalize();

	BEGIN_UPDATE_LOOP;
		// Determine the position of the particle projected on the line
		FVector AdjustedLocation	= Particle.Location - Owner->Component->LocalToWorld.GetOrigin();
		FVector EP02Particle		= AdjustedLocation - EndPoint0;
		FVector ProjectedParticle	= Line * (Line | EP02Particle) / (Line | Line);

		// Determine the 'ratio' of the line that has been traveled by the particle
		FLOAT VRatioX = 0.0f;
		FLOAT VRatioY = 0.0f;
		FLOAT VRatioZ = 0.0f;

		if (Line.X)
		{
			VRatioX = (ProjectedParticle.X - EndPoint0.X) / Line.X;
		}
		if (Line.Y)
		{
			VRatioY = (ProjectedParticle.Y - EndPoint0.Y) / Line.Y;
		}
		if (Line.Z)
		{
			VRatioZ = (ProjectedParticle.Z - EndPoint0.Z) / Line.Z;
		}

		UBOOL	bProcess	= FALSE;
		FLOAT	Ratio		= 0.0f;

		if (VRatioX || VRatioY || VRatioZ)
		{
			// If there are multiple ratios, they should be the same...
			if (VRatioX)
			{
				Ratio = VRatioX;
			}
			else
			if (VRatioY)
			{
				Ratio = VRatioY;
			}
			else
			if (VRatioZ)
			{
				Ratio = VRatioZ;
			}
		}

		if ((Ratio >= 0.0f) && (Ratio <= 1.0f))
		{
			bProcess = TRUE;
		}

		if (bProcess)
		{
			// Look up the Range and Strength at that position on the line
			FLOAT AttractorRangeA	= Range.GetValue(Ratio, Owner->Component);
			FLOAT AttractorRangeB	= LowModule->Range.GetValue(Ratio, Owner->Component);
			FLOAT AttractorRange	= (AttractorRangeA * Multiplier) + (AttractorRangeB * (1.0f - Multiplier));
	        
			FVector LineToPoint	= AdjustedLocation - ProjectedParticle;
    		FLOAT	Distance	= LineToPoint.Size();

			if ((AttractorRange > 0) && (Distance <= AttractorRange))
			{
				// Adjust the strength based on the range ratio
				FLOAT AttractorStrengthA	= Strength.GetValue((AttractorRange - Distance) / AttractorRange, Owner->Component);
				FLOAT AttractorStrengthB	= LowModule->Strength.GetValue((AttractorRange - Distance) / AttractorRange, Owner->Component);
				FLOAT AttractorStrength		= (AttractorStrengthA * Multiplier) + (AttractorStrengthB * (1.0f - Multiplier));

				FVector Direction = LineToPoint^Line;
    			// Adjust the VELOCITY of the particle based on the attractor... 
        		Particle.Velocity += Direction * AttractorStrength * DeltaTime;
			}
		}
	END_UPDATE_LOOP;
}

void UParticleModuleAttractorLine::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	PDI->DrawLine(EndPoint0, EndPoint1, ModuleEditorColor, SDPG_World);

	UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	FLOAT CurrRatio = Owner->EmitterTime / LODLevel->RequiredModule->EmitterDuration;
	FLOAT LineRange = Range.GetValue(CurrRatio, Owner->Component);

	// Determine the position of the range at this time.
	FVector LinePos = EndPoint0 + CurrRatio * (EndPoint1 - EndPoint0);

	// Draw a wire star at the position of the range.
	DrawWireStar(PDI,LinePos, 10.0f, ModuleEditorColor, SDPG_World);
	// Draw bounding circle for the current range.
	// This should be oriented such that it appears correctly... ie, as 'open' to the camera as possible
	DrawCircle(PDI,LinePos, FVector(1,0,0), FVector(0,1,0), ModuleEditorColor, LineRange, 32, SDPG_World);
}

/*-----------------------------------------------------------------------------
	UParticleModuleAttractorParticle implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleAttractorParticle);
/**
 *	UParticleModuleAttractorParticle::Spawn
 */
void UParticleModuleAttractorParticle::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* AttractorEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				AttractorEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (AttractorEmitterInst == NULL)
	{
		// No source emitter, so we don't spawn??
		return;
	}

	SPAWN_INIT
	{
		PARTICLE_ELEMENT(FAttractorParticlePayload,	Data);
		
		FBaseParticle* Source = AttractorEmitterInst->GetParticle(LastSelIndex);
		if (Source == NULL)
		{
			switch (SelectionMethod)
			{
			case EAPSM_Random:
				LastSelIndex		= appTrunc(appSRand() * AttractorEmitterInst->ActiveParticles);
				Data.SourceIndex	= LastSelIndex;
				break;
			case EAPSM_Sequential:
				{
					for (INT ui = 0; ui < AttractorEmitterInst->ActiveParticles; ui++)
					{
						Source = AttractorEmitterInst->GetParticle(ui);
						if (Source)
						{
							LastSelIndex		= ui;
							Data.SourceIndex	= LastSelIndex;
							break;
						}
					}
				}
				break;
			}
			Data.SourcePointer	= (UINT)(PTRINT)Source;
			if (Source)
			{
				Data.SourceVelocity	= Source->Velocity;
			}
		}
		else
		{
			Data.SourceIndex = LastSelIndex++;
		}
	}
}

/**
 *	UParticleModuleAttractorParticle::Update
 */
void UParticleModuleAttractorParticle::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* AttractorEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				AttractorEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (AttractorEmitterInst == NULL)
	{
		// No source emitter, so we don't update??
		return;
	}

	UParticleLODLevel* LODLevel		= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	UParticleLODLevel* SrcLODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(AttractorEmitterInst);
	check(SrcLODLevel);

	UBOOL bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
	UBOOL bSrcUseLocalSpace = SrcLODLevel->RequiredModule->bUseLocalSpace;

	BEGIN_UPDATE_LOOP;
	{
		// Find the source particle
		PARTICLE_ELEMENT(FAttractorParticlePayload,	Data);

		if (Data.SourceIndex == 0xffffffff)
		{
#if 1
			if (bInheritSourceVel)
			{
				Particle.Velocity	+= Data.SourceVelocity;
			}
#endif
			CONTINUE_UPDATE_LOOP;
		}

		FBaseParticle* Source = AttractorEmitterInst->GetParticle(Data.SourceIndex);
		if (!Source)
		{
			CONTINUE_UPDATE_LOOP;
		}

		if ((Data.SourcePointer != 0) && 
			(Source != (FBaseParticle*)(PTRINT)(Data.SourcePointer)) && 
			(bRenewSource == FALSE))
		{
			Data.SourceIndex	= 0xffffffff;
			Data.SourcePointer	= 0;
#if 0
			if (bInheritSourceVel)
			{
				Particle.Velocity		+= Data.SourceVelocity;
				Particle.BaseVelocity	+= Data.SourceVelocity;
			}
#endif
			CONTINUE_UPDATE_LOOP;
		}

		FLOAT	AttractorRange		= Range.GetValue(Source->RelativeTime, Owner->Component);
		FVector SrcLocation			= Source->Location;
		FVector	ParticleLocation	= Particle.Location;
		if (bUseLocalSpace != bSrcUseLocalSpace)
		{
			if (bSrcUseLocalSpace)
			{
				SrcLocation = Owner->Component->LocalToWorld.TransformNormal(SrcLocation);
			}
			if (bUseLocalSpace)
			{
				ParticleLocation = Owner->Component->LocalToWorld.TransformNormal(Particle.Location);
			}
		}

		FVector Dir			= SrcLocation - ParticleLocation;
		FLOAT	Distance	= Dir.Size();
		if (Distance <= AttractorRange)
		{
			// Determine the strength
			FLOAT AttractorStrength = 0.0f;

			if (bStrengthByDistance)
			{
				// on actual distance
				AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange);
			}
			else
			{
				// on emitter time
				AttractorStrength = Strength.GetValue(Source->RelativeTime, Owner->Component);
			}

			// Adjust the VELOCITY of the particle based on the attractor... 
			Dir.Normalize();
    		Particle.Velocity	+= Dir * AttractorStrength * DeltaTime;
			Data.SourceVelocity	 = Source->Velocity;
			if (bAffectBaseVelocity)
			{
				Particle.BaseVelocity	+= Dir * AttractorStrength * DeltaTime;
			}
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleAttractorParticle::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	Spawn(Owner, Offset, SpawnTime);
}

void UParticleModuleAttractorParticle::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleLODLevel*					LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleAttractorParticle*	LowModule	= Cast<UParticleModuleAttractorParticle>(LowerLODModule);

	// We need to look up the emitter instance...
	// This may not need to be done every Spawn, but in the short term, it will to be safe.
	// (In the editor, the source emitter may be deleted, etc...)
	FParticleEmitterInstance* AttractorEmitterInst = NULL;
	if (EmitterName != NAME_None)
	{
		for (INT ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++)
		{
			FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances(ii);
			if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName))
			{
				AttractorEmitterInst = pkEmitInst;
				break;
			}
		}
	}

	if (AttractorEmitterInst == NULL)
	{
		// No source emitter, so we don't update??
		return;
	}

	UParticleLODLevel* SrcLODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(AttractorEmitterInst);

	UBOOL bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
	UBOOL bSrcUseLocalSpace = SrcLODLevel->RequiredModule->bUseLocalSpace;

	BEGIN_UPDATE_LOOP;
	{
		// Find the source particle
		PARTICLE_ELEMENT(FAttractorParticlePayload,	Data);

		if (Data.SourceIndex == 0xffffffff)
		{
#if 1
			if (bInheritSourceVel)
			{
				Particle.Velocity	+= Data.SourceVelocity;
			}
#endif
			CONTINUE_UPDATE_LOOP;
		}

		FBaseParticle* Source = AttractorEmitterInst->GetParticle(Data.SourceIndex);
		if (!Source)
		{
			CONTINUE_UPDATE_LOOP;
		}

		if ((Data.SourcePointer != 0) && 
			(Source != (FBaseParticle*)(PTRINT)(Data.SourcePointer)) && 
			(bRenewSource == FALSE))
		{
			Data.SourceIndex	= 0xffffffff;
			Data.SourcePointer	= 0;
#if 0
			if (bInheritSourceVel)
			{
				Particle.Velocity		+= Data.SourceVelocity;
				Particle.BaseVelocity	+= Data.SourceVelocity;
			}
#endif
			CONTINUE_UPDATE_LOOP;
		}

		FLOAT AttractorRangeA	= Range.GetValue(Source->RelativeTime, Owner->Component);
		FLOAT AttractorRangeB	= LowModule->Range.GetValue(Source->RelativeTime, Owner->Component);
		FLOAT AttractorRange	= (AttractorRangeA * Multiplier) + (AttractorRangeB * (1.0f - Multiplier));

		FVector SrcLocation			= Source->Location;
		FVector	ParticleLocation	= Particle.Location;
		if (bUseLocalSpace != bSrcUseLocalSpace)
		{
			if (bSrcUseLocalSpace)
			{
				SrcLocation = Owner->Component->LocalToWorld.TransformNormal(SrcLocation);
			}
			if (bUseLocalSpace)
			{
				ParticleLocation = Owner->Component->LocalToWorld.TransformNormal(Particle.Location);
			}
		}

		FVector Dir			= SrcLocation - ParticleLocation;
		FLOAT	Distance	= Dir.Size();
		if (Distance <= AttractorRange)
		{
			// Determine the strength
			FLOAT AttractorStrength = 0.0f;

			if (bStrengthByDistance)
			{
				// on actual distance
				FLOAT	AttractorStrengthA	= Strength.GetValue((AttractorRangeA - Distance) / AttractorRangeA);
				FLOAT	AttractorStrengthB	= LowModule->Strength.GetValue((AttractorRangeB - Distance) / AttractorRangeB);
				AttractorStrength = (AttractorStrengthA * Multiplier) + (AttractorStrengthB * (1.0f - Multiplier));
			}
			else
			{
				// on emitter time
				FLOAT	AttractorStrengthA	= Strength.GetValue(Source->RelativeTime, Owner->Component);
				FLOAT	AttractorStrengthB	= LowModule->Strength.GetValue(Source->RelativeTime, Owner->Component);
				AttractorStrength = (AttractorStrengthA * Multiplier) + (AttractorStrengthB * (1.0f - Multiplier));
			}

			// Adjust the VELOCITY of the particle based on the attractor... 
			Dir.Normalize();
    		Particle.Velocity	+= Dir * AttractorStrength * DeltaTime;
			if (bAffectBaseVelocity)
			{
				Particle.BaseVelocity	+= Dir * AttractorStrength * DeltaTime;
			}
			Data.SourceVelocity	 = Source->Velocity;
		}
	}
	END_UPDATE_LOOP;
}

/**
 *	UParticleModuleAttractorParticle::Render3DPreview
 */
UINT UParticleModuleAttractorParticle::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FAttractorParticlePayload);
}

/*-----------------------------------------------------------------------------
	UParticleModuleAttractorPoint implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleAttractorPoint);
/***
void UParticleModuleAttractorPoint::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	//@todo. Remove this is Spawn doesn't get used!
//	UParticleModule::Spawn(Owner, Offset, SpawnTime);
	// Grab the position of the attractor in Emitter time???
	FVector AttractorPosition = Position.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT AttractorRange = Range.GetValue(Owner->EmitterTime, Owner->Component);

	SPAWN_INIT
	{
		// If the particle is within range...
		FVector Dir = AttractorPosition - (Particle.Location - Owner->Location);
		FLOAT Distance = Dir.Size();
		if (Distance <= AttractorRange)
		{
			// Determine the strength
			FLOAT AttractorStrength = 0.0f;

			if (StrengthByDistance)
			{
				// on actual distance
				AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange);
			}
			else
			{
				// on emitter time
				AttractorStrength = Strength.GetValue(Owner->EmitterTime, Owner->Component);
			}

			// Adjust the VELOCITY of the particle based on the attractor... 
			Dir.Normalize();
    		Particle.Velocity		+= Dir * AttractorStrength * SpawnTime;
			if (bAffectBaseVelocity)
			{
				Particle.BaseVelocity	+= Dir * AttractorStrength * SpawnTime;
			}
		}
	}
}
***/
void UParticleModuleAttractorPoint::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	check(Owner);
	UParticleSystemComponent* Component = Owner->Component;

	// Grab the position of the attractor in Emitter time???
	FVector AttractorPosition = Position.GetValue(Owner->EmitterTime, Component);
	FLOAT AttractorRange = Range.GetValue(Owner->EmitterTime, Component);

	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	check(LODLevel->RequiredModule);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		// Transform the attractor into world space
		AttractorPosition = Component->LocalToWorld.TransformFVector(AttractorPosition);

		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}
	}
	FLOAT ScaleSize = Scale.Size();

	AttractorRange *= ScaleSize;

	BEGIN_UPDATE_LOOP;
		// If the particle is within range...
		FVector Dir = AttractorPosition - Particle.Location;
		FLOAT Distance = Dir.Size();
		if (Distance <= AttractorRange)
		{
			// Determine the strength
			FLOAT AttractorStrength = 0.0f;

			if (StrengthByDistance)
			{
				// on actual distance
				AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange);
			}
			else
			{
				// on emitter time
				AttractorStrength = Strength.GetValue(Owner->EmitterTime, Component);
			}
			if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
			{
				AttractorStrength *= ScaleSize;
			}

			// Adjust the VELOCITY of the particle based on the attractor... 
			Dir.Normalize();
    		Particle.Velocity	+= Dir * AttractorStrength * DeltaTime;
			if (bAffectBaseVelocity)
			{
				Particle.BaseVelocity	+= Dir * AttractorStrength * DeltaTime;
			}
		}
	END_UPDATE_LOOP;
}

void UParticleModuleAttractorPoint::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	check(Owner);
	UParticleSystemComponent* Component = Owner->Component;

	UParticleLODLevel*				LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleAttractorPoint*	LowModule	= Cast<UParticleModuleAttractorPoint>(LowerLODModule);

	// Grab the position of the attractor in Emitter time???
	FVector AttractorPositionA	= Position.GetValue(Owner->EmitterTime, Component);
	FVector AttractorPositionB	= LowModule->Position.GetValue(Owner->EmitterTime, Component);
	FLOAT	AttractorRangeA		= Range.GetValue(Owner->EmitterTime, Component);
	FLOAT	AttractorRangeB		= LowModule->Range.GetValue(Owner->EmitterTime, Component);

	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		// Transform the attractor into world space
		AttractorPositionA = Component->LocalToWorld.TransformFVector(AttractorPositionA);
		AttractorPositionB = Component->LocalToWorld.TransformFVector(AttractorPositionB);

		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}
	}
	FLOAT ScaleSize = Scale.Size();
	AttractorRangeA *= ScaleSize;
	AttractorRangeB *= ScaleSize;

	FVector AttractorPosition	= (AttractorPositionA * Multiplier) + (AttractorPositionB * (1.0f - Multiplier));
	FLOAT	AttractorRange		= (AttractorRangeA * Multiplier) + (AttractorRangeB * (1.0f - Multiplier));

	BEGIN_UPDATE_LOOP;
		// If the particle is within range...
		FVector Dir = AttractorPosition - Particle.Location;
		FLOAT Distance = Dir.Size();
		if (Distance <= AttractorRange)
		{
			// Determine the strength
			FLOAT AttractorStrength = 0.0f;
			FLOAT AttractorStrengthA;
			FLOAT AttractorStrengthB;

			if (StrengthByDistance)
			{
				// on actual distance
				AttractorStrengthA	= Strength.GetValue((AttractorRangeA - Distance) / AttractorRangeA);
				AttractorStrengthB	= LowModule->Strength.GetValue((AttractorRangeB - Distance) / AttractorRangeB);
			}
			else
			{
				// on emitter time
				AttractorStrengthA	= Strength.GetValue(Owner->EmitterTime, Owner->Component);
				AttractorStrengthB	= LowModule->Strength.GetValue(Owner->EmitterTime, Owner->Component);
			}
			if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
			{
				AttractorStrengthA *= ScaleSize;
				AttractorStrengthB *= ScaleSize;
			}
			AttractorStrength = (AttractorStrengthA * Multiplier) + (AttractorStrengthB * (1.0f - Multiplier));

			// Adjust the VELOCITY of the particle based on the attractor... 
			Dir.Normalize();
    		Particle.Velocity	+= Dir * AttractorStrength * DeltaTime;
			if (bAffectBaseVelocity)
			{
				Particle.BaseVelocity	+= Dir * AttractorStrength * DeltaTime;
			}
		}
	END_UPDATE_LOOP;
}

void UParticleModuleAttractorPoint::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	FVector PointPos = Position.GetValue(Owner->EmitterTime, Owner->Component);
//    FLOAT PointStr = Strength.GetValue(Owner->EmitterTime, Owner->Component);
	FLOAT PointRange = Range.GetValue(Owner->EmitterTime, Owner->Component);

	// Draw a wire star at the position.
	DrawWireStar(PDI,PointPos, 10.0f, ModuleEditorColor, SDPG_World);
	
	// Draw bounding circles for the range.
	DrawCircle(PDI,PointPos, FVector(1,0,0), FVector(0,1,0), ModuleEditorColor, PointRange, 32, SDPG_World);
	DrawCircle(PDI,PointPos, FVector(1,0,0), FVector(0,0,1), ModuleEditorColor, PointRange, 32, SDPG_World);
	DrawCircle(PDI,PointPos, FVector(0,1,0), FVector(0,0,1), ModuleEditorColor, PointRange, 32, SDPG_World);

	// Draw lines showing the path of travel...
	INT	NumKeys = Position.Distribution->GetNumKeys();
	INT	NumSubCurves = Position.Distribution->GetNumSubCurves();

	FVector InitialPosition;
	FVector SamplePosition[2];

	for (INT i = 0; i < NumKeys; i++)
	{
		FLOAT X = Position.Distribution->GetKeyOut(0, i);
		FLOAT Y = Position.Distribution->GetKeyOut(1, i);
		FLOAT Z = Position.Distribution->GetKeyOut(2, i);

		if (i == 0)
		{
			InitialPosition.X = X;
			InitialPosition.Y = Y;
			InitialPosition.Z = Z;
			SamplePosition[1].X = X;
			SamplePosition[1].Y = Y;
			SamplePosition[1].Z = Z;
		}
		else
		{
			SamplePosition[0].X = SamplePosition[1].X;
			SamplePosition[0].Y = SamplePosition[1].Y;
			SamplePosition[0].Z = SamplePosition[1].Z;
			SamplePosition[1].X = X;
			SamplePosition[1].Y = Y;
			SamplePosition[1].Z = Z;

			// Draw a line...
			PDI->DrawLine(SamplePosition[0], SamplePosition[1], ModuleEditorColor, SDPG_World);
		}
	}
}

/*-----------------------------------------------------------------------------
	Parameter-based distributions
-----------------------------------------------------------------------------*/

UBOOL UDistributionFloatParticleParameter::GetParamValue(UObject* Data, FName ParamName, FLOAT& OutFloat)
{
	UBOOL bFoundParam = false;

	UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Data);
	if(ParticleComp)
	{
		bFoundParam = ParticleComp->GetFloatParameter(ParameterName, OutFloat);
	}

	return bFoundParam;
}

UBOOL UDistributionVectorParticleParameter::GetParamValue(UObject* Data, FName ParamName, FVector& OutVector)
{
	UBOOL bFoundParam = false;

	UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Data);
	if(ParticleComp)
	{
		bFoundParam = ParticleComp->GetVectorParameter(ParameterName, OutVector);

		// If we failed to get a Vector parameter with the given name, see if we can get a Float parameter
		if(!bFoundParam)
		{
			FLOAT OutFloat;
			bFoundParam = ParticleComp->GetFloatParameter(ParameterName, OutFloat);
			if(bFoundParam)
			{
				OutVector = FVector(OutFloat);
			}
		}
	}

	return bFoundParam;
}
