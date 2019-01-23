/**
 *	 
 *	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "MaterialInstance.h"

IMPLEMENT_CLASS(UMaterialInstanceTimeVarying);



static void UpdateMICResources(UMaterialInstanceTimeVarying* Instance)
{
	for(INT Selected = 0;Selected < 2;Selected++)
	{
		if(!Instance->Resources[Selected])
		{
			continue;
		}

		// Find the instance's parent.
		UMaterialInterface* Parent = NULL;
		if(Instance->Parent)
		{
			Parent = Instance->Parent;
		}

		// Don't use the instance's parent if it has a circular dependency on the instance.
		if(Parent && Parent->IsDependent(Instance))
		{
			Parent = NULL;
		}

		// If the instance doesn't have a valid parent, use the default material as the parent.
		if(!Parent)
		{
			if(GEngine && GEngine->DefaultMaterial)
			{
				Parent = GEngine->DefaultMaterial;
			}
			else
			{
				// A material instance was loaded with an invalid GEngine.
				// This is probably because loading the default properties for the GEngine class lead to a material instance being loaded
				// before GEngine has been created.  In this case, we'll just pull the default material config value straight from the INI.
				Parent = LoadObject<UMaterialInterface>(NULL,TEXT("engine-ini:Engine.Engine.DefaultMaterialName"),NULL,LOAD_None,NULL);
			}
		}

		check(Parent);

		// Set the FMaterialRenderProxy's game thread parent.
		Instance->Resources[Selected]->SetGameThreadParent(Parent);

		// Enqueue a rendering command with the updated material instance data.
		TSetResourceDataContext<FMaterialInstanceResource> NewData(Instance->Resources[Selected]);
		NewData->Parent = Parent;
		NewData->ParentInstance = Instance;
		for(INT ValueIndex = 0;ValueIndex < Instance->VectorParameterValues.Num();ValueIndex++)
		{
			NewData->VectorParameterMap.Set(
				Instance->VectorParameterValues(ValueIndex).ParameterName,
				Instance->VectorParameterValues(ValueIndex).ParameterValue
				);
		}
		for(INT ValueIndex = 0;ValueIndex < Instance->ScalarParameterValues.Num();ValueIndex++)
		{
			if( ( Instance->ScalarParameterValues(ValueIndex).bAutoActivate == TRUE )
				|| ( Instance->ScalarParameterValues(ValueIndex).StartTime >= 0.f )
				)
			{
				//debugf( TEXT( "Enqueing: %s" ), *Instance->ScalarParameterValues(ValueIndex).ParameterName.ToString() );

				FTimeVaryingDataType NewDatum;
				NewDatum.StartTime = Instance->ScalarParameterValues(ValueIndex).StartTime;
				NewDatum.ScalarValue = Instance->ScalarParameterValues(ValueIndex).ParameterValue;
				NewDatum.TheCurve = Instance->ScalarParameterValues(ValueIndex).ParameterValueCurve;

				NewData->ScalarOverTimeParameterMap.Set(
					Instance->ScalarParameterValues(ValueIndex).ParameterName,
					NewDatum
					);
			}
		}
		for (INT ValueIndex = 0; ValueIndex < Instance->TextureParameterValues.Num(); ValueIndex++)
		{
			if(Instance->TextureParameterValues(ValueIndex).ParameterValue)
			{
				NewData->TextureParameterMap.Set(
					Instance->TextureParameterValues(ValueIndex).ParameterName,
					Instance->TextureParameterValues(ValueIndex).ParameterValue
					);
			}
		}
		for( INT ValueIndex = 0; ValueIndex < Instance->FontParameterValues.Num(); ValueIndex++ )
		{
			// add font texture to the texture parameter list
			FFontParameterValueOverTime& FontParamValue = Instance->FontParameterValues(ValueIndex);
			if( FontParamValue.FontValue && 
				FontParamValue.FontValue->Textures.IsValidIndex(FontParamValue.FontPage) )
			{
				// get the texture for the font page
				UTexture* Texture = FontParamValue.FontValue->Textures(FontParamValue.FontPage);
				if( Texture )
				{
					NewData->TextureParameterMap.Set(
						FontParamValue.ParameterName,
						Texture
						);
				}
			}
		}
	}
}


UMaterialInstanceTimeVarying::UMaterialInstanceTimeVarying()
{

	// GIsUCCMake is not set when the class is initialized
	if(!GIsUCCMake && !HasAnyFlags(RF_ClassDefaultObject))
	{
		Resources[0] = new FMaterialInstanceTimeVaryingResource(this,FALSE);
		if(GIsEditor)
		{
			Resources[1] = new FMaterialInstanceTimeVaryingResource(this,TRUE);
		}
		UpdateMICResources(this);
	}

}


void UMaterialInstanceTimeVarying::PostLoad()
{
	// Ensure that the instance's parent is PostLoaded before the instance.
	if(Parent)
	{
		Parent->ConditionalPostLoad();
	}

	// Add references to the expression object if we do not have one already, and fix up any names that were changed.
	UpdateParameterNames();

	// We have to make sure the resources are created for all used textures.
	for( INT ValueIndex=0; ValueIndex<TextureParameterValues.Num(); ValueIndex++ )
	{
		// Make sure the texture is postloaded so the resource isn't null.
		UTexture* Texture = TextureParameterValues(ValueIndex).ParameterValue;
		if( Texture )
		{
			Texture->ConditionalPostLoad();
		}
	}
	// do the same for font textures
	for( INT ValueIndex=0; ValueIndex < FontParameterValues.Num(); ValueIndex++ )
	{
		// Make sure the font is postloaded so the resource isn't null.
		UFont* Font = FontParameterValues(ValueIndex).FontValue;
		if( Font )
		{
			Font->ConditionalPostLoad();
		}
	}

	Super::PostLoad();

	UpdateMICResources(this);
}


void UMaterialInstanceTimeVarying::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);
	UpdateMICResources(this);

	UpdateStaticPermutation();
}



/**
* Refreshes parameter names using the stored reference to the expression object for the parameter.
*/
void UMaterialInstanceTimeVarying::UpdateParameterNames()
{
	if(IsTemplate(RF_ClassDefaultObject)==FALSE)
	{
		// Get a pointer to the parent material.
		UMaterial* ParentMaterial = NULL;
		UMaterialInstance* ParentInst = this;
		while(ParentInst && ParentInst->Parent)
		{
			if(ParentInst->Parent->IsA(UMaterial::StaticClass()))
			{
				ParentMaterial = Cast<UMaterial>(ParentInst->Parent);
				break;
			}
			else
			{
				ParentInst = Cast<UMaterialInstance>(ParentInst->Parent);
			}
		}

		if(ParentMaterial)
		{
			UBOOL bMarkDirty = FALSE;

			// Scalar parameters
			bMarkDirty = UpdateParameterSet<FScalarParameterValueOverTime, UMaterialExpressionScalarParameter>(ScalarParameterValues, ParentMaterial) || bMarkDirty;

			// Vector parameters
			bMarkDirty = UpdateParameterSet<FVectorParameterValueOverTime, UMaterialExpressionVectorParameter>(VectorParameterValues, ParentMaterial) || bMarkDirty;

			// Texture parameters
			bMarkDirty = UpdateParameterSet<FTextureParameterValueOverTime, UMaterialExpressionTextureSampleParameter>(TextureParameterValues, ParentMaterial) || bMarkDirty;

			// Font parameters
			bMarkDirty = UpdateParameterSet<FFontParameterValueOverTime, UMaterialExpressionFontSampleParameter>(FontParameterValues, ParentMaterial) || bMarkDirty;

			for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
			{
				// Static switch parameters
				bMarkDirty = UpdateParameterSet<FStaticSwitchParameter, UMaterialExpressionStaticSwitchParameter>(StaticParameters[PlatformIndex]->StaticSwitchParameters, ParentMaterial) || bMarkDirty;

				// Static component mask parameters
				bMarkDirty = UpdateParameterSet<FStaticComponentMaskParameter, UMaterialExpressionStaticComponentMaskParameter>(StaticParameters[PlatformIndex]->StaticComponentMaskParameters, ParentMaterial) || bMarkDirty;

			}

			// Atleast 1 parameter changed, mark the package dirty.
			if(bMarkDirty)
			{
				MarkPackageDirty();
				UpdateMICResources(this);
			}
		}
	}
}





void UMaterialInstanceTimeVarying::SetParent(UMaterialInterface* NewParent)
{
	Super::SetParent( NewParent );

	UpdateMICResources(this);
}



// Set*ParameterValue - Updates the entry in ParameterValues for the named parameter, or adds a new entry.

void UMaterialInstanceTimeVarying::SetVectorParameterValue(FName ParameterName, FLinearColor Value)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for (INT ValueIndex = 0;ValueIndex < VectorParameterValues.Num();ValueIndex++)
	{
		if (VectorParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			VectorParameterValues(ValueIndex).ParameterValue = Value;
			bSetParameterValue = TRUE;
			break;
		}
	}

	if(!bSetParameterValue)
	{
		// If there's no element for the named parameter in array yet, add one.
		FVectorParameterValueOverTime*	NewParameterValue = new(VectorParameterValues) FVectorParameterValueOverTime;
		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->ParameterValue = Value;
		NewParameterValue->ExpressionGUID.Invalidate();
	}

	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}


/** 
 * For MITVs you can utilize both single Scalar values and InterpCurve values.
 *
 * If there is any data in the InterpCurve, then the MITV will utilize that. Else it will utilize the Scalar value
 * of the same name.
 **/
void UMaterialInstanceTimeVarying::SetScalarParameterValue(FName ParameterName, float Value)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for (INT ValueIndex = 0;ValueIndex < ScalarParameterValues.Num();ValueIndex++)
	{
		if (ScalarParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			ScalarParameterValues(ValueIndex).ParameterValue = Value;
			bSetParameterValue = TRUE;
			break;
		}
	}

	if(!bSetParameterValue)
	{
		// If there's no element for the named parameter in array yet, add one.
		FScalarParameterValueOverTime* NewParameterValue = new(ScalarParameterValues) FScalarParameterValueOverTime;
		appMemzero( &NewParameterValue->ParameterValueCurve.Points, sizeof(NewParameterValue->ParameterValueCurve.Points) );

		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->ParameterValue = Value;
		NewParameterValue->StartTime = -1.f;

		NewParameterValue->ExpressionGUID.Invalidate();
	}

	// Update the material instance data in the rendering thread.
 	UpdateMICResources(this);
}


void UMaterialInstanceTimeVarying::SetScalarCurveParameterValue(FName ParameterName, FInterpCurveFloat Value)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for (INT ValueIndex = 0;ValueIndex < ScalarParameterValues.Num();ValueIndex++)
	{
		if (ScalarParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			ScalarParameterValues(ValueIndex).ParameterValueCurve = Value;
			bSetParameterValue = TRUE;
			break;
		}
	}

	if(!bSetParameterValue)
	{
		// If there's no element for the named parameter in array yet, add one.
		FScalarParameterValueOverTime* NewParameterValue = new(ScalarParameterValues) FScalarParameterValueOverTime;
		appMemzero( &NewParameterValue->ParameterValueCurve.Points, sizeof(NewParameterValue->ParameterValueCurve.Points) );

		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->ParameterValueCurve = Value;
		NewParameterValue->ParameterValue = 0.0f;
		NewParameterValue->StartTime = -1.f;

		NewParameterValue->ExpressionGUID.Invalidate();
	}

	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}


void UMaterialInstanceTimeVarying::SetScalarStartTime(FName ParameterName, FLOAT Value)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for (INT ValueIndex = 0;ValueIndex < ScalarParameterValues.Num();ValueIndex++)
	{
		if (ScalarParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			ScalarParameterValues(ValueIndex).StartTime = GWorld->GetTimeSeconds() + Value;
			bSetParameterValue = TRUE;
			break;
		}
	}


	if(!bSetParameterValue)
	{
		// If there's no element for the named parameter in array yet, add one.
		FScalarParameterValueOverTime* NewParameterValue = new(ScalarParameterValues) FScalarParameterValueOverTime;
		appMemzero( &NewParameterValue->ParameterValueCurve.Points, sizeof(NewParameterValue->ParameterValueCurve.Points) );

		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->StartTime = GWorld->GetTimeSeconds() + Value;

		FLOAT Value;
		if( Parent != NULL )
		{
			Parent->GetScalarParameterValue( ParameterName, Value );
			NewParameterValue->ParameterValue = Value;
		}

		// we need to go ask our parent to find us the Curves we are going to add
		UMaterialInstanceTimeVarying* MITV = Cast<UMaterialInstanceTimeVarying>(Parent);
		if( MITV != NULL )
		{
			FInterpCurveInitFloat Values;
			MITV->GetScalarCurveParameterValue( ParameterName, Values );
			//debugf( TEXT( "NUM PTS %d"), Values.Points.Num() );
			NewParameterValue->ParameterValueCurve = Values;
		}

		NewParameterValue->ExpressionGUID.Invalidate();
	}


	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}


void UMaterialInstanceTimeVarying::SetTextureParameterValue(FName ParameterName, UTexture* Value)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for(INT ValueIndex = 0;ValueIndex < TextureParameterValues.Num();ValueIndex++)
	{
		if(TextureParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			TextureParameterValues(ValueIndex).ParameterValue = Value;
			bSetParameterValue = TRUE;
			break;
		}
	}

	if(!bSetParameterValue)
	{
		// If there's no element for the named parameter in array yet, add one.
		FTextureParameterValueOverTime*	NewParameterValue = new(TextureParameterValues) FTextureParameterValueOverTime;
		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->ParameterValue = Value;
		NewParameterValue->ExpressionGUID.Invalidate();
	}

	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}




/**
* Sets the value of the given font parameter.  
*
* @param	ParameterName	The name of the font parameter
* @param	OutFontValue	New font value to set for this MIC
* @param	OutFontPage		New font page value to set for this MIC
*/
void UMaterialInstanceTimeVarying::SetFontParameterValue(FName ParameterName,class UFont* FontValue,INT FontPage)
{
	UBOOL bSetParameterValue = FALSE;

	// Check for an existing value for the named parameter in the array.
	for( INT ValueIndex = 0; ValueIndex < FontParameterValues.Num(); ValueIndex++ )
	{
		if( FontParameterValues(ValueIndex).ParameterName == ParameterName )
		{
			FontParameterValues(ValueIndex).FontValue = FontValue;
			FontParameterValues(ValueIndex).FontPage = FontPage;
			bSetParameterValue = TRUE;
			break;
		}
	}
	if( !bSetParameterValue )
	{
		// If there's no element for the named parameter in array yet, add one.
		FFontParameterValue* NewParameterValue = new(FontParameterValues) FFontParameterValue;
		NewParameterValue->ParameterName = ParameterName;
		NewParameterValue->FontValue = FontValue;
		NewParameterValue->FontPage = FontPage;
		NewParameterValue->ExpressionGUID.Invalidate();
	}

	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}


/** Removes all parameter values */
void UMaterialInstanceTimeVarying::ClearParameterValues()
{
	VectorParameterValues.Empty();
	ScalarParameterValues.Empty();
	TextureParameterValues.Empty();
	FontParameterValues.Empty();

	// Update the material instance data in the rendering thread.
	UpdateMICResources(this);
}





UBOOL UMaterialInstanceTimeVarying::GetVectorParameterValue(FName ParameterName, FLinearColor& OutValue)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	FLinearColor* Value = NULL;
	for (INT ValueIndex = 0;ValueIndex < VectorParameterValues.Num();ValueIndex++)
	{
		if (VectorParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			Value = &VectorParameterValues(ValueIndex).ParameterValue;
			break;
		}
	}
	if(Value)
	{
		OutValue = *Value;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetVectorParameterValue(ParameterName,OutValue);
	}
	else
	{
		return FALSE;
	}
}

/** 
 * For MITVs you can utilize both single Scalar values and InterpCurve values.
 *
 * If there is any data in the InterpCurve, then the MITV will utilize that. Else it will utilize the Scalar value
 * of the same name.
 **/
UBOOL UMaterialInstanceTimeVarying::GetScalarParameterValue(FName ParameterName, FLOAT& OutValue)
{
	UBOOL bFoundAValue = FALSE;

	if(ReentrantFlag)
	{
		return FALSE;
	}

	FLOAT Value = 0; // = NULL;
	for (INT ValueIndex = 0;ValueIndex < ScalarParameterValues.Num();ValueIndex++)
	{
		if (ScalarParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			if( ScalarParameterValues(ValueIndex).ParameterValueCurve.Points.Num() > 0 )
			{
				if( ScalarParameterValues(ValueIndex).StartTime >= 0.0f )
				{
					const FLOAT EvalTime = (GWorld->GetTimeSeconds() - ScalarParameterValues(ValueIndex).StartTime);
					Value = ScalarParameterValues(ValueIndex).ParameterValueCurve.Eval( EvalTime, 0.0f );
					bFoundAValue = TRUE;
					break;
				}
			}
			else
			{
				Value = ScalarParameterValues(ValueIndex).ParameterValue;
				bFoundAValue = TRUE;
				break;
			}
		}

	}

	if( bFoundAValue == TRUE )
	{
		OutValue = Value;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetScalarParameterValue(ParameterName,OutValue);
	}
	else
	{
		return FALSE;
	}
}


UBOOL UMaterialInstanceTimeVarying::GetScalarCurveParameterValue(FName ParameterName,FInterpCurveFloat& OutValue)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	FLOAT NumPoints = 0; // = NULL;
	INT ValueIndex = 0;
	for( ValueIndex = 0;ValueIndex < ScalarParameterValues.Num();ValueIndex++)
	{
		if (ScalarParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			NumPoints = ScalarParameterValues(ValueIndex).ParameterValueCurve.Points.Num();
			break;
		}
	}
	if(NumPoints)
	{
		OutValue = ScalarParameterValues(ValueIndex).ParameterValueCurve;
		return TRUE;
	}
	else if(Cast<UMaterialInstanceTimeVarying>(Parent))
	{
		FMICReentranceGuard	Guard(this);
		return Cast<UMaterialInstanceTimeVarying>(Parent)->GetScalarCurveParameterValue(ParameterName,OutValue);
	}
	else
	{
		return FALSE;
	}
}


UBOOL UMaterialInstanceTimeVarying::GetTextureParameterValue(FName ParameterName, UTexture*& OutValue)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	UTexture** Value = NULL;
	for (INT ValueIndex = 0;ValueIndex < TextureParameterValues.Num();ValueIndex++)
	{
		if (TextureParameterValues(ValueIndex).ParameterName == ParameterName)
		{
			Value = &TextureParameterValues(ValueIndex).ParameterValue;
			break;
		}
	}
	if(Value && *Value)
	{
		OutValue = *Value;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetTextureParameterValue(ParameterName,OutValue);
	}
	else
	{
		return FALSE;
	}
}

/**
* Gets the value of the given font parameter.  If it is not found in this instance then
* the request is forwarded up the MIC chain.
*
* @param	ParameterName	The name of the font parameter
* @param	OutFontValue	Will contain the font value of the font value if successful
* @param	OutFontPage		Will contain the font value of the font page if successful
* @return					True if successful
*/
UBOOL UMaterialInstanceTimeVarying::GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue, INT& OutFontPage)
{
	UBOOL Result = FALSE;
	if( !ReentrantFlag )
	{
		UFont** FontValue = NULL;
		INT FontPage = 0;

		// find the parameter by name
		for( INT ValueIndex = 0; ValueIndex < FontParameterValues.Num(); ValueIndex++ )
		{
			if( FontParameterValues(ValueIndex).ParameterName == ParameterName )
			{
				FontValue = &FontParameterValues(ValueIndex).FontValue;
				FontPage = FontParameterValues(ValueIndex).FontPage;
				break;
			}
		}
		// return the valid values
		if( FontValue && *FontValue )
		{
			OutFontValue = *FontValue;
			OutFontPage = FontPage;
			Result = TRUE;
		}
		//@todo sz
		// 		// try the parent if values were invalid
		// 		else if( Parent )
		// 		{
		// 			FMICReentranceGuard	Guard(this);
		// 			Result = Parent->GetFontParameterValue(ParameterName,OutFontValue,OutFontPage);
		// 		}
	}
	return Result;	
}
