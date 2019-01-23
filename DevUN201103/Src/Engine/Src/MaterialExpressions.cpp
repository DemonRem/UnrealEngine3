/*=============================================================================
	MaterialExpressions.cpp - Material expressions implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(UMaterialExpression);
IMPLEMENT_CLASS(UMaterialExpressionTextureSample);
IMPLEMENT_CLASS(UMaterialExpressionMeshEmitterDynamicParameter);
IMPLEMENT_CLASS(UMaterialExpressionMeshEmitterVertexColor);
IMPLEMENT_CLASS(UMaterialExpressionMultiply);
IMPLEMENT_CLASS(UMaterialExpressionDivide);
IMPLEMENT_CLASS(UMaterialExpressionSubtract);
IMPLEMENT_CLASS(UMaterialExpressionLinearInterpolate);
IMPLEMENT_CLASS(UMaterialExpressionAdd);
IMPLEMENT_CLASS(UMaterialExpressionTextureCoordinate);
IMPLEMENT_CLASS(UMaterialExpressionStaticComponentMaskParameter );
IMPLEMENT_CLASS(UMaterialExpressionComponentMask);
IMPLEMENT_CLASS(UMaterialExpressionDotProduct);
IMPLEMENT_CLASS(UMaterialExpressionCrossProduct);
IMPLEMENT_CLASS(UMaterialExpressionConstantClamp);
IMPLEMENT_CLASS(UMaterialExpressionClamp);
IMPLEMENT_CLASS(UMaterialExpressionConstant);
IMPLEMENT_CLASS(UMaterialExpressionConstant2Vector);
IMPLEMENT_CLASS(UMaterialExpressionConstant3Vector);
IMPLEMENT_CLASS(UMaterialExpressionConstant4Vector);
IMPLEMENT_CLASS(UMaterialExpressionTime);
IMPLEMENT_CLASS(UMaterialExpressionCameraVector);
IMPLEMENT_CLASS(UMaterialExpressionCameraWorldPosition);
IMPLEMENT_CLASS(UMaterialExpressionReflectionVector);
IMPLEMENT_CLASS(UMaterialExpressionPanner);
IMPLEMENT_CLASS(UMaterialExpressionRotator);
IMPLEMENT_CLASS(UMaterialExpressionSine);
IMPLEMENT_CLASS(UMaterialExpressionCosine);
IMPLEMENT_CLASS(UMaterialExpressionBumpOffset);
IMPLEMENT_CLASS(UMaterialExpressionAppendVector);
IMPLEMENT_CLASS(UMaterialExpressionFloor);
IMPLEMENT_CLASS(UMaterialExpressionCeil);
IMPLEMENT_CLASS(UMaterialExpressionFmod);
IMPLEMENT_CLASS(UMaterialExpressionFrac);
IMPLEMENT_CLASS(UMaterialExpressionAbs);
IMPLEMENT_CLASS(UMaterialExpressionDepthBiasBlend);
IMPLEMENT_CLASS(UMaterialExpressionDepthBiasedAlpha);
IMPLEMENT_CLASS(UMaterialExpressionDepthBiasedBlend);
IMPLEMENT_CLASS(UMaterialExpressionDesaturation);
IMPLEMENT_CLASS(UMaterialExpressionParameter);
IMPLEMENT_CLASS(UMaterialExpressionVectorParameter);
IMPLEMENT_CLASS(UMaterialExpressionScalarParameter);
IMPLEMENT_CLASS(UMaterialExpressionStaticSwitchParameter);
IMPLEMENT_CLASS(UMaterialExpressionNormalize);
IMPLEMENT_CLASS(UMaterialExpressionVertexColor);
IMPLEMENT_CLASS(UMaterialExpressionParticleSubUV);
IMPLEMENT_CLASS(UMaterialExpressionParticleMacroUV);
IMPLEMENT_CLASS(UMaterialExpressionMeshSubUV);
IMPLEMENT_CLASS(UMaterialExpressionMeshSubUVBlend);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameter);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameter2D);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterCube);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterMovie);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterMeshSubUV);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterMeshSubUVBlend);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterNormal);
IMPLEMENT_CLASS(UMaterialExpressionTextureSampleParameterSubUV);
IMPLEMENT_CLASS(UMaterialExpressionFlipBookSample);
IMPLEMENT_CLASS(UMaterialExpressionLensFlareIntensity);
IMPLEMENT_CLASS(UMaterialExpressionLensFlareOcclusion);
IMPLEMENT_CLASS(UMaterialExpressionLensFlareRadialDistance);
IMPLEMENT_CLASS(UMaterialExpressionLensFlareRayDistance);
IMPLEMENT_CLASS(UMaterialExpressionLensFlareSourceDistance);
IMPLEMENT_CLASS(UMaterialExpressionLightVector);
IMPLEMENT_CLASS(UMaterialExpressionScreenPosition);
IMPLEMENT_CLASS(UMaterialExpressionPixelDepth);
IMPLEMENT_CLASS(UMaterialExpressionDestColor);
IMPLEMENT_CLASS(UMaterialExpressionDestDepth);
IMPLEMENT_CLASS(UMaterialExpressionPower);
IMPLEMENT_CLASS(UMaterialExpressionSquareRoot);
IMPLEMENT_CLASS(UMaterialExpressionIf);
IMPLEMENT_CLASS(UMaterialExpressionOcclusionPercentage);
IMPLEMENT_CLASS(UMaterialExpressionOneMinus);
IMPLEMENT_CLASS(UMaterialExpressionSceneTexture);
IMPLEMENT_CLASS(UMaterialExpressionSceneDepth);
IMPLEMENT_CLASS(UMaterialExpressionTransform);
IMPLEMENT_CLASS(UMaterialExpressionTransformPosition);
IMPLEMENT_CLASS(UMaterialExpressionComment);
IMPLEMENT_CLASS(UMaterialExpressionCompound);
IMPLEMENT_CLASS(UMaterialExpressionFresnel);
IMPLEMENT_CLASS(UMaterialExpressionFontSample);
IMPLEMENT_CLASS(UMaterialExpressionFontSampleParameter);
IMPLEMENT_CLASS(UMaterialExpressionWorldPosition);
IMPLEMENT_CLASS(UMaterialExpressionObjectWorldPosition);
IMPLEMENT_CLASS(UMaterialExpressionObjectRadius);
IMPLEMENT_CLASS(UMaterialExpressionFluidNormal);
IMPLEMENT_CLASS(UMaterialExpressionDeriveNormalZ);
IMPLEMENT_CLASS(UMaterialExpressionConstantBiasScale);
IMPLEMENT_CLASS(UMaterialExpressionDynamicParameter);
IMPLEMENT_CLASS(UMaterialExpressionCustom);
IMPLEMENT_CLASS(UMaterialExpressionCustomTexture);
IMPLEMENT_CLASS(UMaterialExpressionLightmassReplace);
IMPLEMENT_CLASS(UMaterialExpressionLightmapUVs);
IMPLEMENT_CLASS(UMaterialExpressionObjectOrientation);
IMPLEMENT_CLASS(UMaterialExpressionWindDirectionAndSpeed);
IMPLEMENT_CLASS(UMaterialExpressionFoliageImpulseDirection);
IMPLEMENT_CLASS(UMaterialExpressionFoliageNormalizedRotationAxisAndAngle);
IMPLEMENT_CLASS(UMaterialExpressionRotateAboutAxis);
IMPLEMENT_CLASS(UMaterialExpressionSphereMask);
IMPLEMENT_CLASS(UMaterialExpressionDistance);
IMPLEMENT_CLASS(UMaterialExpressionTwoSidedSign);
IMPLEMENT_CLASS(UMaterialExpressionWorldNormal);
IMPLEMENT_CLASS(UMaterialExpressionPerInstanceRandom);
IMPLEMENT_CLASS(UMaterialExpressionAntialiasedTextureMask);
IMPLEMENT_CLASS(UMaterialExpressionTerrainLayerWeight);
IMPLEMENT_CLASS(UMaterialExpressionTerrainLayerCoords);
IMPLEMENT_CLASS(UMaterialExpressionDepthOfFieldFunction);

#define SWAP_REFERENCE_TO( ExpressionInput, ToBeRemovedExpression, ToReplaceWithExpression )	\
if( ExpressionInput.Expression == ToBeRemovedExpression )										\
{																								\
	ExpressionInput.Expression = ToReplaceWithExpression;										\
}


//  Helper to unpack a texture

static INT UnpackTexture(FMaterialCompiler* Compiler, INT ArgA, const UTexture* Texture, BYTE CompressionSettings)
{
	switch( CompressionSettings )
	{
	case TC_Normalmap:
	case TC_NormalmapAlpha:
	case TC_NormalmapUncompressed:
	case TC_NormalmapBC5:
		{
			ArgA = Compiler->BiasNormalizeNormalMap(ArgA, CompressionSettings);

			// handle any unusual normal unpacking
			FLOAT NormalMapUnpackMin[4] = { -1, -1, -1, +0 };
			FLOAT NormalMapUnpackMax[4] = { +1, +1, +1, +1 };
			if( appMemcmp(Texture->UnpackMin,NormalMapUnpackMin,sizeof(NormalMapUnpackMin)) != 0 || 
				appMemcmp(Texture->UnpackMax,NormalMapUnpackMax,sizeof(NormalMapUnpackMax)) != 0 )
			{
				// adjust the normal unpacking to 0-1 range
				ArgA = Compiler->Mul(Compiler->Add(ArgA, Compiler->Constant4(1.f,1.f,1.f,0.f)), Compiler->Constant4(0.5f,0.5f,0.5f,1.f));
			}
			else
			{
				return ArgA;
			}
		}
		break;
	default:
		break;
	}

	INT ArgB = Compiler->Constant4(
		Texture->UnpackMax[0] - Texture->UnpackMin[0],
		Texture->UnpackMax[1] - Texture->UnpackMin[1],
		Texture->UnpackMax[2] - Texture->UnpackMin[2],
		Texture->UnpackMax[3] - Texture->UnpackMin[3]
	);
	INT ArgC =  Compiler->Constant4(
		Texture->UnpackMin[0],
		Texture->UnpackMin[1],
		Texture->UnpackMin[2],
		Texture->UnpackMin[3]
	);

	return Compiler->Add(Compiler->Mul(ArgA, ArgB), ArgC);

}

/**
* Copies the SrcExpressions into the specified material.  Preserves internal references.
* New material expressions are created within the specified material.
*/
void UMaterialExpression::CopyMaterialExpressions(const TArray<UMaterialExpression*>& SrcExpressions, const TArray<UMaterialExpressionComment*>& SrcExpressionComments, 
									UMaterial* Material, TArray<UMaterialExpression*>& OutNewExpressions, TArray<UMaterialExpression*>& OutNewComments)
{
	OutNewExpressions.Empty();
	OutNewComments.Empty();

	TMap<UMaterialExpression*,UMaterialExpression*> SrcToDestMap;

	// Duplicate source expressions into the editor's material copy buffer.
	for( INT SrcExpressionIndex = 0 ; SrcExpressionIndex < SrcExpressions.Num() ; ++SrcExpressionIndex )
	{
		UMaterialExpression*	SrcExpression		= SrcExpressions(SrcExpressionIndex);
		UMaterialExpression*	NewExpression		= Cast<UMaterialExpression>(UObject::StaticDuplicateObject( SrcExpression, SrcExpression, Material, NULL, RF_Transactional ));

		SrcToDestMap.Set( SrcExpression, NewExpression );

		// Add to list of material expressions associated with the copy buffer.
		Material->Expressions.AddItem( NewExpression );

		// Generate GUID if we are copying a parameter expression
		UMaterialExpressionParameter* ParameterExpression = Cast<UMaterialExpressionParameter>( NewExpression );
		if( ParameterExpression )
		{
			ParameterExpression->ConditionallyGenerateGUID(TRUE);
		}

		UMaterialExpressionTextureSampleParameter* TextureParameterExpression = Cast<UMaterialExpressionTextureSampleParameter>( NewExpression );
		if( TextureParameterExpression )
		{
			TextureParameterExpression->ConditionallyGenerateGUID(TRUE);
		}

		// Record in output list.
		OutNewExpressions.AddItem( NewExpression );
	}

	// Fix up internal references.  Iterate over the inputs of the new expressions, and for each input that refers
	// to an expression that was duplicated, point the reference to that new expression.  Otherwise, clear the input.
	for( INT NewExpressionIndex = 0 ; NewExpressionIndex < OutNewExpressions.Num() ; ++NewExpressionIndex )
	{
		UMaterialExpression* NewExpression = OutNewExpressions(NewExpressionIndex);
		const TArray<FExpressionInput*>& ExpressionInputs = NewExpression->GetInputs();
		for ( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
		{
			FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
			UMaterialExpression* InputExpression = Input->Expression;
			if ( InputExpression )
			{
				UMaterialExpression** NewExpression = SrcToDestMap.Find( InputExpression );
				if ( NewExpression )
				{
					check( *NewExpression );
					Input->Expression = *NewExpression;
				}
				else
				{
					Input->Expression = NULL;
				}
			}
		}
	}

	// Copy Selected Comments
	for( INT CommentIndex=0; CommentIndex<SrcExpressionComments.Num(); CommentIndex++)
	{
		UMaterialExpressionComment* ExpressionComment = SrcExpressionComments(CommentIndex);
		UMaterialExpressionComment* NewComment = Cast<UMaterialExpressionComment>(UObject::StaticDuplicateObject(ExpressionComment, ExpressionComment, Material, NULL));

		// Add reference to the material
		Material->EditorComments.AddItem(NewComment);

		// Add to the output array.
		OutNewComments.AddItem(NewComment);
	}
}


//
//	UMaterialExpression::PostEditChange
//

void UMaterialExpression::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	check(GetOuterUMaterial());
#if !CONSOLE
	if (!GIsImportingT3D && !GIsTransacting)
#endif
	{
		// Don't recompile the outer material if we are in the middle of a transaction
		// as there may be many expressions in the transaction buffer and we would just be recompiling over and over again.
		GetOuterUMaterial()->PreEditChange(NULL);
		GetOuterUMaterial()->PostEditChange();
	}

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if( PropertyThatChanged != NULL )
	{
		// Update the preview for this node if we adjusted a property
		bNeedToUpdatePreview = TRUE;
	}
}

//
//	UMaterialExpression::GetOutputs
//

void UMaterialExpression::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(0);
}

//
//	UMaterialExpression::GetInputs
//
const TArray<FExpressionInput*> UMaterialExpression::GetInputs()
{
	TArray<FExpressionInput*> Result;
	for( TFieldIterator<UStructProperty> InputIt(GetClass()) ; InputIt ; ++InputIt )
	{
		UStructProperty* StructProp = *InputIt;
		if( StructProp->Struct->GetFName() == NAME_ExpressionInput )
		{
			FExpressionInput* Input = (FExpressionInput*)((BYTE*)this + StructProp->Offset);
			Result.AddItem(Input);
		}
	}
	return Result;
}

//
//	UMaterialExpression::GetInput
//

FExpressionInput* UMaterialExpression::GetInput(INT InputIndex)
{
	INT Index = 0;
	for( TFieldIterator<UStructProperty> InputIt(GetClass()) ; InputIt ; ++InputIt )
	{
		UStructProperty* StructProp = *InputIt;
		if( StructProp->Struct->GetFName() == NAME_ExpressionInput )
		{
			FExpressionInput* Input = (FExpressionInput*)((BYTE*)this + StructProp->Offset);
			if( Index == InputIndex )
			{
				return Input;
			}
			Index++;
		}
	}

	return NULL;
}

//
//	UMaterialExpression::GetInputName
//

FString UMaterialExpression::GetInputName(INT InputIndex) const
{
	INT Index = 0;
	for( TFieldIterator<UStructProperty> InputIt(GetClass()) ; InputIt ; ++InputIt )
	{
		UStructProperty* StructProp = *InputIt;
		if( StructProp->Struct->GetFName() == NAME_ExpressionInput )
		{
			if( Index == InputIndex )
			{
				return StructProp->GetFName().ToString();
			}
			Index++;
		}
	}
	return TEXT("");
}

//
//	UMaterialExpression::GetWidth
//

INT UMaterialExpression::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

//
//	UMaterialExpression::GetHeight
//

INT UMaterialExpression::GetHeight() const
{
	TArray<FExpressionOutput>	Outputs;
	GetOutputs(Outputs);
	return Max(ME_CAPTION_HEIGHT + (Outputs.Num() * ME_STD_TAB_HEIGHT),ME_CAPTION_HEIGHT+ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2));
}

//
//	UMaterialExpression::UsesLeftGutter
//

UBOOL UMaterialExpression::UsesLeftGutter() const
{
	return 0;
}

//
//	UMaterialExpression::UsesRightGutter
//

UBOOL UMaterialExpression::UsesRightGutter() const
{
	return 0;
}

//
//	UMaterialExpression::GetCaption
//

FString UMaterialExpression::GetCaption() const
{
	return TEXT("Expression");
}

INT UMaterialExpression::CompilerError(FMaterialCompiler* Compiler, const TCHAR* pcMessage)
{
	return Compiler->Errorf(TEXT("%s> %s"), Desc.Len() > 0 ? *Desc : *GetCaption(), pcMessage);
}

void UMaterialExpression::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if ( Ar.Ver() < VER_DEPRECATED_EDITOR_POSITION )
	{
		MaterialExpressionEditorX = EditorX_DEPRECATED;
		MaterialExpressionEditorY = EditorY_DEPRECATED;
	}

	if (GetOuter() && GetOuter()->IsA(UMaterial::StaticClass()) && Cast<UMaterial>(GetOuter())->IsFallbackMaterial())
	{
		// Allow legacy fallback materials and their expressions to be garbage collected
		ClearFlags(RF_Standalone);
		// Objects in startup packages will be part of the root set
		RemoveFromRoot();
	}
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpression::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	return Desc.InStr(SearchQuery, FALSE, TRUE) != -1;
}

//
//	UMaterialExpressionTextureSample::Compile
//
INT UMaterialExpressionTextureSample::Compile(FMaterialCompiler* Compiler)
{
#if EXCEPTIONS_DISABLED
	// if we can't throw the error below, attempt to thwart the error by using the default texture
	// @todo: handle missing cubemaps and 3d textures?
	if (!Texture)
	{
		debugf(TEXT("Using default texture instead of real texture!"));
		Texture = GWorld->GetWorldInfo()->DefaultTexture;
	}
#endif

	if (Texture)
	{
		INT TextureCodeIndex = Compiler->Texture(Texture);

		INT ArgA = Compiler->TextureSample(
						TextureCodeIndex,
						Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE)
						);

		return UnpackTexture(Compiler, ArgA, Texture, Texture->CompressionSettings);
	}
	else
	{
		if (Desc.Len() > 0)
		{
			return Compiler->Errorf(TEXT("%s> Missing input texture"), *Desc);
		}
		else
		{
			return Compiler->Errorf(TEXT("TextureSample> Missing input texture"));
		}
	}
}

//
//	UMaterialExpressionTextureSample::GetOutputs
//

void UMaterialExpressionTextureSample::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

INT UMaterialExpressionTextureSample::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 * Replaces references to the passed in expression with references to a different expression or NULL.
 * @param	OldExpression		Expression to find reference to.
 * @param	NewExpression		Expression to replace reference with.
 */
void UMaterialExpressionTextureSample::SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression)
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinates, OldExpression, NewExpression );
}

//
//	UMaterialExpressionTextureSample::GetCaption
//

FString UMaterialExpressionTextureSample::GetCaption() const
{
	return TEXT("Texture Sample");
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionTextureSample::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( Texture!=NULL && Texture->GetName().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}

void UMaterialExpressionAdd::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//  UMaterialExpressionTextureSampleParameter
//
INT UMaterialExpressionTextureSampleParameter::Compile(FMaterialCompiler* Compiler)
{
	if (Texture == NULL)
	{
		return CompilerError(Compiler, GetRequirements());
	}

    if (Texture)
    {
        if (!TextureIsValid(Texture))
        {
            return CompilerError(Compiler, GetRequirements());
        }
    }

	if (!ParameterName.IsValid() || (ParameterName.GetIndex() == NAME_None))
	{
		return UMaterialExpressionTextureSample::Compile(Compiler);
	}

	INT TextureCodeIndex = Compiler->TextureParameter(ParameterName, Texture);


	INT ArgA = Compiler->TextureSample(
					TextureCodeIndex,
					Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE)
					);
#if GEMINI_TODO
	// Should we handle texture parameter UnpackMin/UnpackMax, or just use the default texture's unpack settings?
#else
	return UnpackTexture(Compiler, ArgA, Texture, Texture->CompressionSettings);
#endif
}

FString UMaterialExpressionTextureSampleParameter::GetCaption() const
{
	return FString::Printf( TEXT("Param '%s'"), *ParameterName.ToString() ); 
}

UBOOL UMaterialExpressionTextureSampleParameter::TextureIsValid( UTexture* /*InTexture*/ )
{
    return FALSE;
}

const TCHAR* UMaterialExpressionTextureSampleParameter::GetRequirements()
{
    return TEXT("Invalid texture type");
}

/**
 *	Sets the default texture if none is set
 */
void UMaterialExpressionTextureSampleParameter::SetDefaultTexture()
{
	// Does nothing in the base case...
}

/** 
 * Generates a GUID for this expression if one doesn't already exist. 
 *
 * @param bForceGeneration	Whether we should generate a GUID even if it is already valid.
 */
void UMaterialExpressionTextureSampleParameter::ConditionallyGenerateGUID(UBOOL bForceGeneration)
{
	// If we are in the editor, and we don't have a valid GUID yet, generate one.
	if(GIsEditor && !GIsGame)
	{
		if(bForceGeneration || ExpressionGUID.IsValid()==FALSE)
		{
			ExpressionGUID = appCreateGuid();
			MarkPackageDirty();
		}
	}
}

/** Tries to generate a GUID. */
void UMaterialExpressionTextureSampleParameter::PostLoad()
{
	Super::PostLoad();

	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionTextureSampleParameter::PostDuplicate()
{
	Super::PostDuplicate();

	// We do not force a guid regen here because this function is used when the Material Editor makes a copy of a material to edit.
	// If we forced a GUID regen, it would cause all of the guids for a material to change everytime a material was edited.
	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionTextureSampleParameter::PostEditImport()
{
	Super::PostEditImport();

	ConditionallyGenerateGUID(TRUE);
}

//
//  UMaterialExpressionTextureSampleParameter2D
//
FString UMaterialExpressionTextureSampleParameter2D::GetCaption() const
{
	return FString::Printf( TEXT("Param2D '%s'"), *ParameterName.ToString() ); 
}

UBOOL UMaterialExpressionTextureSampleParameter2D::TextureIsValid( UTexture* InTexture )
{
	UBOOL Result=FALSE;
	if (InTexture)		
    {
        if( InTexture->GetClass() == UTexture2D::StaticClass() ) 
		{
			Result = TRUE;
		}
		if( InTexture->IsA(UTextureRenderTarget2D::StaticClass()) )	
		{
			Result = TRUE;
		}
	}
    return Result;
}

const TCHAR* UMaterialExpressionTextureSampleParameter2D::GetRequirements()
{
    return TEXT("Requires Texture2D");
}

/**
 *	Sets the default texture if none is set
 */
void UMaterialExpressionTextureSampleParameter2D::SetDefaultTexture()
{
	Texture = LoadObject<UTexture2D>(NULL, TEXT("EngineResources.DefaultTexture"), NULL, LOAD_None, NULL);
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionTextureSampleParameter::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( ParameterName.ToString().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}

//
//  UMaterialExpressionTextureSampleParameterCube
//
FString UMaterialExpressionTextureSampleParameterCube::GetCaption() const
{
	return FString::Printf( TEXT("ParamCube'%s'"), *ParameterName.ToString() ); 
}

UBOOL UMaterialExpressionTextureSampleParameterCube::TextureIsValid( UTexture* InTexture )
{
	UBOOL Result=false;
    if (InTexture)
    {
		if( InTexture->GetClass() == UTextureCube::StaticClass() ) {
			Result = true;
		}
		if( InTexture->IsA(UTextureRenderTargetCube::StaticClass()) ) {
			Result = true;
		}
    }
    return Result;
}

const TCHAR* UMaterialExpressionTextureSampleParameterCube::GetRequirements()
{
    return TEXT("Requires TextureCube");
}

/**
 *	Sets the default texture if none is set
 */
void UMaterialExpressionTextureSampleParameterCube::SetDefaultTexture()
{
	Texture = LoadObject<UTextureCube>(NULL, TEXT("EngineResources.DefaultTextureCube"), NULL, LOAD_None, NULL);
}

/**
* Textual description for this material expression
*
* @return	Caption text
*/	
FString UMaterialExpressionTextureSampleParameterMovie::GetCaption() const
{
	return FString::Printf( TEXT("ParamMovie '%s'"), *ParameterName.ToString() ); 
}

/**
* Return true if the texture is a movie texture
*
* @return	true/false
*/	
UBOOL UMaterialExpressionTextureSampleParameterMovie::TextureIsValid( UTexture* InTexture )
{
	UBOOL Result=false;
    if (InTexture)
    {
		Result = (InTexture->GetClass() == UTextureMovie::StaticClass());
    }
    return Result;
}

/**
* Called when TextureIsValid==false
*
* @return	Descriptive error text
*/	
const TCHAR* UMaterialExpressionTextureSampleParameterMovie::GetRequirements()
{
    return TEXT("Requires TextureMovie");
}

/** 
 *	UMaterialExpressionTextureSampleParameterMeshSubUV 
 */
INT UMaterialExpressionTextureSampleParameterMeshSubUV::Compile(FMaterialCompiler* Compiler)
{
	if (Texture == NULL)
	{
		return CompilerError(Compiler, GetRequirements());
	}

	if (!TextureIsValid(Texture))
	{
		return CompilerError(Compiler, GetRequirements());
	}

	//                         | Mul |
	//	VectorParam(Scale)---->|A    |----\           | Add |   |Sample|
	//	TextureCoord(0)------->|B    |     \--------->|A    |-->|Coord |
	//	VectorParam(SubUVMeshOffset)-->CompMask(XY)-->|B    |   |      |

	// Out	 = sub-UV sample of the input texture
	//
	INT TextureCodeIndex = Compiler->TextureParameter(ParameterName, Texture);

	INT Tex = Compiler->TextureSample(
				TextureCodeIndex,
				Compiler->Add(
					Compiler->Mul(
					Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
						Compiler->ComponentMask(
							Compiler->VectorParameter(FName(TEXT("TextureScaleParameter")),FLinearColor::White),
							1, 
							1, 
							0, 
							0
							)
						),
					Compiler->ComponentMask(
						Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
						1, 
						1, 
						0, 
						0
						)
					)
				);
	return UnpackTexture(Compiler, Tex, Texture, Texture->CompressionSettings);
}

FString UMaterialExpressionTextureSampleParameterMeshSubUV::GetCaption() const
{
	return TEXT("Parameter MeshSubUV");
}

UBOOL UMaterialExpressionTextureSampleParameterMeshSubUV::TextureIsValid( UTexture* InTexture )
{
	return UMaterialExpressionTextureSampleParameter2D::TextureIsValid(InTexture);
}

const TCHAR* UMaterialExpressionTextureSampleParameterMeshSubUV::GetRequirements()
{
	return UMaterialExpressionTextureSampleParameter2D::GetRequirements();
}

/** 
 *	UMaterialExpressionTextureSampleParameterMeshSubUVBlend 
 */
INT UMaterialExpressionTextureSampleParameterMeshSubUVBlend::Compile(FMaterialCompiler* Compiler)
{
	if (Texture == NULL)
	{
		return CompilerError(Compiler, GetRequirements());
	}

	if (!TextureIsValid(Texture))
	{
		return CompilerError(Compiler, GetRequirements());
	}

	// Out	 = sub-UV sample of the input texture
	//
	INT TextureCodeIndex		= Compiler->TextureParameter(ParameterName, Texture);
	INT UnpackScaleIndex		= Compiler->Constant4(
									Texture->UnpackMax[0] - Texture->UnpackMin[0],
									Texture->UnpackMax[1] - Texture->UnpackMin[1],
									Texture->UnpackMax[2] - Texture->UnpackMin[2],
									Texture->UnpackMax[3] - Texture->UnpackMin[3]
									);
	INT UnpackOffsetIndex		= Compiler->Constant4(
									Texture->UnpackMin[0],
									Texture->UnpackMin[1],
									Texture->UnpackMin[2],
									Texture->UnpackMin[3]
									);

	INT ScaleParameterIndex		= Compiler->ComponentMask(
									Compiler->VectorParameter(FName(TEXT("TextureScaleParameter")),FLinearColor::White),
									1, 
									1, 
									0, 
									0
									);

	INT OffsetParameterIndex_0	= Compiler->ComponentMask(
									Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
									1, 
									1, 
									0, 
									0
									);

	INT TextureSampleIndex_0	= Compiler->TextureSample(
									TextureCodeIndex,
									Compiler->Add(
										Compiler->Mul(
											Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
											ScaleParameterIndex
											),
										OffsetParameterIndex_0
										)
									);
	
	INT OffsetParameterIndex_1	= Compiler->ComponentMask(
									Compiler->VectorParameter(FName(TEXT("TextureOffset1Parameter")),FLinearColor::Black),
									1, 
									1, 
									0, 
									0
									);

	INT TextureSampleIndex_1	= Compiler->TextureSample(
									TextureCodeIndex,
									Compiler->Add(
										Compiler->Mul(
											Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
											ScaleParameterIndex
											),
										OffsetParameterIndex_1
										)
									);

	INT ResultIndex_0, ResultIndex_1;
	switch( Texture->CompressionSettings )
	{
	case TC_Normalmap:
	case TC_NormalmapAlpha:
	case TC_NormalmapUncompressed:
	case TC_NormalmapBC5:
		ResultIndex_0 = Compiler->BiasNormalizeNormalMap(TextureSampleIndex_0, Texture->CompressionSettings);
		ResultIndex_1 = Compiler->BiasNormalizeNormalMap(TextureSampleIndex_1, Texture->CompressionSettings);
		break;
	default:
		ResultIndex_0 = Compiler->Add(Compiler->Mul(TextureSampleIndex_0, UnpackScaleIndex),UnpackOffsetIndex);
		ResultIndex_1 = Compiler->Add(Compiler->Mul(TextureSampleIndex_1, UnpackScaleIndex),UnpackOffsetIndex);
		break;
	}

	INT LerpAlphaIndex			= Compiler->ComponentMask(
									Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
									0, 
									0, 
									1, 
									0
									);

	return Compiler->Lerp(ResultIndex_0, ResultIndex_1, LerpAlphaIndex);
}

FString UMaterialExpressionTextureSampleParameterMeshSubUVBlend::GetCaption() const
{
	return TEXT("Parameter MeshSubUVBlend");
}

/** 
 *	UMaterialExpressionTextureSampleParameterSubUV
 */
INT UMaterialExpressionTextureSampleParameterSubUV::Compile(FMaterialCompiler* Compiler)
{
	if (Texture == NULL)
	{
		return CompilerError(Compiler, GetRequirements());
	}

    if (Texture)
    {
        if (!TextureIsValid(Texture))
        {
            return CompilerError(Compiler, GetRequirements());
        }
    }

	// Out	 = linear interpolate... using 2 sub-images of the texture
	// A	 = RGB sample texture with TexCoord0
	// B	 = RGB sample texture with TexCoord1
	// Alpha = x component of TexCoord2
	//
	INT TextureCodeIndex = Compiler->TextureParameter(ParameterName, Texture);

	INT TexSampleA = Compiler->TextureSample(TextureCodeIndex, Compiler->TextureCoordinate(0, FALSE, FALSE) );
	INT TexSampleB = Compiler->TextureSample(TextureCodeIndex, Compiler->TextureCoordinate(1, FALSE, FALSE) );

	INT LerpA = UnpackTexture(Compiler, TexSampleA, Texture, Texture->CompressionSettings);
	INT LerpB = UnpackTexture(Compiler, TexSampleB, Texture, Texture->CompressionSettings);

	return 
		Compiler->Lerp(
			LerpA, 
			LerpB, 
			// Lerp 'alpha' comes from the 3rd texture set...
			Compiler->ComponentMask(
				Compiler->TextureCoordinate(2, FALSE, FALSE), 
				1, 0, 0, 0
			)
		);
}

FString UMaterialExpressionTextureSampleParameterSubUV::GetCaption() const
{
	return TEXT("Parameter SubUV");
}

UBOOL UMaterialExpressionTextureSampleParameterSubUV::TextureIsValid( UTexture* InTexture )
{
	return UMaterialExpressionTextureSampleParameter2D::TextureIsValid(InTexture);
}

const TCHAR* UMaterialExpressionTextureSampleParameterSubUV::GetRequirements()
{
	return UMaterialExpressionTextureSampleParameter2D::GetRequirements();
}

//
//	UMaterialExpressionFlipBookSample
//
INT UMaterialExpressionFlipBookSample::Compile(FMaterialCompiler* Compiler)
{
	if (Texture)
	{
		if (!Texture->IsA(UTextureFlipBook::StaticClass()))
		{
			return Compiler->Errorf(TEXT("FlipBookSample> Texture is not a FlipBook"));
		}

		// Ensure the texture is loaded...
		Texture->ConditionalPostLoad();

		//                                 | Mul |
		//	FlipBookScale-->CompMask(XY)-->|A    |----\      | Add |   |Sample|
		//	TextureCoord(0)--------------->|B    |     \---->|A    |-->|Coord |
		//	FlipBookOffset->CompMask(XY)-------------------->|B    |   |      |
		// Out	 = sub-UV sample of the input texture
		//
		UTextureFlipBook* FlipBook = CastChecked<UTextureFlipBook>(Texture);

		// Compile the texture itself
		INT TextureCodeIndex	= Compiler->Texture(Texture);
		// The scale is constant per texture, and we don't allow for parameterized flipbooks...
		FVector Scale;
		FlipBook->GetFlipBookScale(Scale);
		INT FBScale = Compiler->Constant2(Scale.X, Scale.Y);
		// The input coordinates are like any other texture-expression
		INT TextureCoord = Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE);
		// Scale the UV
		INT ScaledUV = Compiler->Mul(FBScale, TextureCoord);
		// The offset is dynamic... we need an uniform expression parameter
		INT FBOffset = Compiler->FlipBookOffset(FlipBook);
		INT FBOffset_Masked = Compiler->ComponentMask(FBOffset,1,1,0,0);
		// Apply the offset
		INT FinalUV = Compiler->Add(ScaledUV, FBOffset_Masked);

		INT ArgA = Compiler->TextureSample(
			TextureCodeIndex, 
			FinalUV
			);

		return UnpackTexture(Compiler, ArgA, Texture, Texture->CompressionSettings);
	}
	else
	{
		if (Desc.Len() > 0)
		{
			return Compiler->Errorf(TEXT("%s> Missing input texture"), *Desc);
		}
		else
		{
			return Compiler->Errorf(TEXT("TextureSample> Missing input texture"));
		}
	}
}

FString UMaterialExpressionFlipBookSample::GetCaption() const
{
    return TEXT("FlipBook");
}

//
//	UMaterialExpressionAdd::Compile
//

INT UMaterialExpressionAdd::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Add input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Add input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Add(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionAdd::GetCaption
//

FString UMaterialExpressionAdd::GetCaption() const
{
	return TEXT("Add");
}

//
// UMaterialExpressionMeshEmitterDynamicParameter
//
INT UMaterialExpressionMeshEmitterDynamicParameter::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->VectorParameter(FName(TEXT("MeshEmitterDynamicParameter")),FLinearColor::White);
}

FString UMaterialExpressionMeshEmitterDynamicParameter::GetCaption() const
{
	return TEXT("MeshEmit DynParam");
}

//
//	UMaterialExpressionMeshEmitterVertexColor::Compile
//
INT UMaterialExpressionMeshEmitterVertexColor::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->VectorParameter(FName(TEXT("MeshEmitterVertexColor")),FLinearColor::White);
}

//
//	UMaterialExpressionMeshEmitterVertexColor::GetCaption
//
FString UMaterialExpressionMeshEmitterVertexColor::GetCaption() const
{
	return TEXT("MeshEmit VertColor");
}

//
//	UMaterialExpressionMeshEmitterVertexColor::GetOutputs
//
void UMaterialExpressionMeshEmitterVertexColor::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

//
//	UMaterialExpressionMultiply::Compile
//

INT UMaterialExpressionMultiply::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Multiply input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Multiply input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Mul(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionMultiply::GetCaption
//

FString UMaterialExpressionMultiply::GetCaption() const
{
	return TEXT("Multiply");
}

void UMaterialExpressionMultiply::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionDestColor
///////////////////////////////////////////////////////////////////////////////
/**
 *	Compile the expression
 *
 *	@param	Compiler	The compiler to utilize
 *
 *	@return				The index of the resulting code chunk.
 *						INDEX_NONE if error.
 */
INT UMaterialExpressionDestColor::Compile(FMaterialCompiler* Compiler)
{
	// resulting index to compiled code chunk
	// add the code chunk for the scene color sample        
	INT Result = Compiler->DestColor();
	return Result;
}

/**
 *	Retrieve the outputs this expression supplies.
 *
 *	@param	Outputs		The array to insert the available outputs in.
 */
void UMaterialExpressionDestColor::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	// RGB
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	// R
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	// G
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	// B
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	// A
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

/**
 *	Get the caption to display on this material expression
 *
 *	@return			An FString containing the display caption
 */
FString UMaterialExpressionDestColor::GetCaption() const
{
	return TEXT("DestColor");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionDestDepth
///////////////////////////////////////////////////////////////////////////////
/**
 *	Compile the expression
 *
 *	@param	Compiler	The compiler to utilize
 *
 *	@return				The index of the resulting code chunk.
 *						INDEX_NONE if error.
 */
INT UMaterialExpressionDestDepth::Compile(FMaterialCompiler* Compiler)
{
	// resulting index to compiled code chunk
	// add the code chunk for the scene depth sample        
	INT Result = Compiler->DestDepth(bNormalize);
	return Result;
}

/**
 *	Retrieve the outputs this expression supplies.
 *
 *	@param	Outputs		The array to insert the available outputs in.
 */
void UMaterialExpressionDestDepth::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	// Depth
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}

/**
 *	Get the caption to display on this material expression
 *
 *	@return			An FString containing the display caption
 */
FString UMaterialExpressionDestDepth::GetCaption() const
{
	return TEXT("DestDepth");
}

//
//	UMaterialExpressionDivide::Compile
//

INT UMaterialExpressionDivide::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Divide input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Divide input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Div(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionDivide::GetCaption
//

FString UMaterialExpressionDivide::GetCaption() const
{
	return TEXT("Divide");
}

void UMaterialExpressionDivide::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionSubtract::Compile
//

INT UMaterialExpressionSubtract::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Subtract input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Subtract input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Sub(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionSubtract::GetCaption
//

FString UMaterialExpressionSubtract::GetCaption() const
{
	return TEXT("Subtract");
}

void UMaterialExpressionSubtract::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionLinearInterpolate::Compile
//

INT UMaterialExpressionLinearInterpolate::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing LinearInterpolate input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing LinearInterpolate input B"));
	}
	else if(!Alpha.Expression)
	{
		return Compiler->Errorf(TEXT("Missing LinearInterpolate input Alpha"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		INT Arg3 = Alpha.Compile(Compiler);
		return Compiler->Lerp(
			Arg1,
			Arg2,
			Arg3
			);
	}
}

//
//	UMaterialExpressionLinearInterpolate::GetCaption
//

FString UMaterialExpressionLinearInterpolate::GetCaption() const
{
	return TEXT("Lerp");
}

void UMaterialExpressionLinearInterpolate::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Alpha, OldExpression, NewExpression );
}

//
//	UMaterialExpressionConstant::Compile
//

INT UMaterialExpressionConstant::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->Constant(R);
}

//
//	UMaterialExpressionConstant::GetCaption
//

FString UMaterialExpressionConstant::GetCaption() const
{
	return FString::Printf( TEXT("%.4g"), R );
}

//
//	UMaterialExpressionConstant2Vector::Compile
//

INT UMaterialExpressionConstant2Vector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->Constant2(R,G);
}

//
//	UMaterialExpressionConstant2Vector::GetCaption
//

FString UMaterialExpressionConstant2Vector::GetCaption() const
{
	return FString::Printf( TEXT("%.3g,%.3g"), R, G );
}

//
//	UMaterialExpressionConstant3Vector::Compile
//

INT UMaterialExpressionConstant3Vector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->Constant3(R,G,B);
}

//
//	UMaterialExpressionConstant3Vector::GetCaption
//

FString UMaterialExpressionConstant3Vector::GetCaption() const
{
	return FString::Printf( TEXT("%.3g,%.3g,%.3g"), R, G, B );
}

//
//	UMaterialExpressionConstant4Vector::Compile
//

INT UMaterialExpressionConstant4Vector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->Constant4(R,G,B,A);
}

//
//	UMaterialExpressionConstant4Vector::GetCaption
//

FString UMaterialExpressionConstant4Vector::GetCaption() const
{
	return FString::Printf( TEXT("%.2g,%.2g,%.2g,%.2g"), R, G, B, A );
}


//
//	UMaterialExpressionConstantClamp
//

INT UMaterialExpressionConstantClamp::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->Clamp( Input.Compile(Compiler), Compiler->Constant(this->Min), Compiler->Constant(this->Max) );
}

FString UMaterialExpressionConstantClamp::GetCaption() const
{
	return TEXT("Constant Clamp");
}

void UMaterialExpressionConstantClamp::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}


//
//	UMaterialExpressionClamp::Compile
//

INT UMaterialExpressionClamp::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Clamp input"));
	}
	else
	{
		if(!Min.Expression && !Max.Expression)
			return Input.Compile(Compiler);
		else if(!Min.Expression)
		{
			INT Arg1 = Input.Compile(Compiler);
			INT Arg2 = Max.Compile(Compiler);
			return Compiler->Min(
				Arg1,
				Arg2
				);
		}
		else if(!Max.Expression)
		{
			INT Arg1 = Input.Compile(Compiler);
			INT Arg2 = Min.Compile(Compiler);
			return Compiler->Max(
				Arg1,
				Arg2
				);
		}
		else
		{
			INT Arg1 = Input.Compile(Compiler);
			INT Arg2 = Min.Compile(Compiler);
			INT Arg3 = Max.Compile(Compiler);
			return Compiler->Clamp(
				Arg1,
				Arg2,
				Arg3
				);
		}
	}
}

//
//	UMaterialExpressionClamp::GetCaption
//

FString UMaterialExpressionClamp::GetCaption() const
{
	return TEXT("Clamp");
}

void UMaterialExpressionClamp::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Min, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Max, OldExpression, NewExpression );
}

//
//	UMaterialExpressionTextureCoordinate::Compile
//

INT UMaterialExpressionTextureCoordinate::Compile(FMaterialCompiler* Compiler)
{
	// Depending on whether we have U and V scale values that differ, we can perform a multiply by either
	// a scalar or a float2.  These tiling values are baked right into the shader node, so they're always
	// known at compile time.
	if( Abs( UTiling - VTiling ) > SMALL_NUMBER )
	{
		return Compiler->Mul(Compiler->TextureCoordinate(CoordinateIndex, UnMirrorU, UnMirrorV),Compiler->Constant2(UTiling, VTiling));
	}
	else
	{
		return Compiler->Mul(Compiler->TextureCoordinate(CoordinateIndex, UnMirrorU, UnMirrorV),Compiler->Constant(UTiling));
	}
}

//
//	UMaterialExpressionTextureCoordinate::GetCaption
//

FString UMaterialExpressionTextureCoordinate::GetCaption() const
{
	return TEXT("TexCoord");
}

//
//	UMaterialExpressionDotProduct::Compile
//

INT UMaterialExpressionDotProduct::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing DotProduct input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing DotProduct input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Dot(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionDotProduct::GetCaption
//

FString UMaterialExpressionDotProduct::GetCaption() const
{
	return TEXT("Dot");
}

void UMaterialExpressionDotProduct::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionCrossProduct::Compile
//

INT UMaterialExpressionCrossProduct::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing CrossProduct input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing CrossProduct input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->Cross(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionCrossProduct::GetCaption
//

FString UMaterialExpressionCrossProduct::GetCaption() const
{
	return TEXT("Cross");
}

void UMaterialExpressionCrossProduct::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionComponentMask::Compile
//

INT UMaterialExpressionComponentMask::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing ComponentMask input"));
	}

	return Compiler->ComponentMask(
		Input.Compile(Compiler),
		R,
		G,
		B,
		A
		);
}

//
//	UMaterialExpressionComponentMask::GetCaption
//

FString UMaterialExpressionComponentMask::GetCaption() const
{
	FString Str(TEXT("Mask ("));
	if ( R ) Str += TEXT(" R");
	if ( G ) Str += TEXT(" G");
	if ( B ) Str += TEXT(" B");
	if ( A ) Str += TEXT(" A");
	Str += TEXT(" )");
	return Str;
}

void UMaterialExpressionComponentMask::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}


//
//	UMaterialExpressionStaticComponentMaskParameter::Compile
//

INT UMaterialExpressionStaticComponentMaskParameter::Compile(FMaterialCompiler* Compiler)
{
	//if the override is set, use its values
	if (InstanceOverride)
	{
		if(!Input.Expression)
		{
			return Compiler->Errorf(TEXT("Missing ComponentMask input"));
		}
		else
		{
			return Compiler->ComponentMask(
				Input.Compile(Compiler),
				InstanceOverride->R,
				InstanceOverride->G,
				InstanceOverride->B,
				InstanceOverride->A
				);
		}
	}
	else
	{
		//use the default expression settings
		if(!Input.Expression)
		{
			return Compiler->Errorf(TEXT("Missing ComponentMask input"));
		}
		else
		{
			return Compiler->ComponentMask(
				Input.Compile(Compiler),
				DefaultR,
				DefaultG,
				DefaultB,
				DefaultA
				);
		}
	}
}

//
//	UMaterialExpressionStaticComponentMaskParameter::GetCaption
//

FString UMaterialExpressionStaticComponentMaskParameter::GetCaption() const
{
	return FString::Printf(TEXT("Mask Param '%s'"), *ParameterName.ToString());
}

void UMaterialExpressionStaticComponentMaskParameter::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(0);
}

void UMaterialExpressionStaticComponentMaskParameter::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

#if WITH_EDITOR
/**
 *	Called by the CleanupMaterials function, this will clear the inputs of the expression.
 *	This only needs to be implemented by expressions that have bUsedByStaticParameterSet set to TRUE.
 */
void UMaterialExpressionStaticComponentMaskParameter::ClearInputExpressions()
{
	Input.Expression = NULL;
}
#endif

//
//	UMaterialExpressionTime::Compile
//

INT UMaterialExpressionTime::Compile(FMaterialCompiler* Compiler)
{
	return bIgnorePause ? Compiler->RealTime() : Compiler->GameTime();
}

//
//	UMaterialExpressionTime::GetCaption
//

FString UMaterialExpressionTime::GetCaption() const
{
	return TEXT("Time");
}

//
//	UMaterialExpressionCameraVector::Compile
//

INT UMaterialExpressionCameraVector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->CameraVector();
}

//
//	UMaterialExpressionCameraVector::GetCaption
//

FString UMaterialExpressionCameraVector::GetCaption() const
{
	return TEXT("Camera Vector");
}

//
//	UMaterialExpressionCameraWorldPosition::Compile
//

INT UMaterialExpressionCameraWorldPosition::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->CameraWorldPosition();
}

//
//	UMaterialExpressionCameraWorldPosition::GetCaption
//

FString UMaterialExpressionCameraWorldPosition::GetCaption() const
{
	return TEXT("Camera World Position");
}

//
//	UMaterialExpressionReflectionVector::Compile
//

INT UMaterialExpressionReflectionVector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ReflectionVector();
}

//
//	UMaterialExpressionReflectionVector::GetCaption
//

FString UMaterialExpressionReflectionVector::GetCaption() const
{
	return TEXT("Reflection Vector");
}

//
//	UMaterialExpressionPanner::Compile
//

INT UMaterialExpressionPanner::Compile(FMaterialCompiler* Compiler)
{
	INT Arg1 = Compiler->PeriodicHint(Compiler->Mul(Time.Expression ? Time.Compile(Compiler) : Compiler->GameTime(),Compiler->Constant(SpeedX)));
	INT Arg2 = Compiler->PeriodicHint(Compiler->Mul(Time.Expression ? Time.Compile(Compiler) : Compiler->GameTime(),Compiler->Constant(SpeedY)));
	INT Arg3 = Coordinate.Expression ? Coordinate.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE);
	return Compiler->Add(
			Compiler->AppendVector(
				Arg1,
				Arg2
				),
			Arg3
			);
}

void UMaterialExpressionPanner::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinate, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Time, OldExpression, NewExpression );
}

//
//	UMaterialExpressionPanner::GetCaption
//

FString UMaterialExpressionPanner::GetCaption() const
{
	return TEXT("Panner");
}

//
//	UMaterialExpressionRotator::Compile
//

INT UMaterialExpressionRotator::Compile(FMaterialCompiler* Compiler)
{
	INT	Cosine = Compiler->Cosine(Compiler->Mul(Time.Expression ? Time.Compile(Compiler) : Compiler->GameTime(),Compiler->Constant(Speed))),
		Sine = Compiler->Sine(Compiler->Mul(Time.Expression ? Time.Compile(Compiler) : Compiler->GameTime(),Compiler->Constant(Speed))),
		RowX = Compiler->AppendVector(Cosine,Compiler->Mul(Compiler->Constant(-1.0f),Sine)),
		RowY = Compiler->AppendVector(Sine,Cosine),
		Origin = Compiler->Constant2(CenterX,CenterY),
		BaseCoordinate = Coordinate.Expression ? Coordinate.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE);

	const INT Arg1 = Compiler->Dot(RowX,Compiler->Sub(Compiler->ComponentMask(BaseCoordinate,1,1,0,0),Origin));
	const INT Arg2 = Compiler->Dot(RowY,Compiler->Sub(Compiler->ComponentMask(BaseCoordinate,1,1,0,0),Origin));

	if(Compiler->GetType(BaseCoordinate) == MCT_Float3)
		return Compiler->AppendVector(
				Compiler->Add(
					Compiler->AppendVector(
						Arg1,
						Arg2
						),
					Origin
					),
				Compiler->ComponentMask(BaseCoordinate,0,0,1,0)
				);
	else
	{
		const INT ArgOne = Compiler->Dot(RowX,Compiler->Sub(BaseCoordinate,Origin));
		const INT ArgTwo = Compiler->Dot(RowY,Compiler->Sub(BaseCoordinate,Origin));

		return Compiler->Add(
				Compiler->AppendVector(
					ArgOne,
					ArgTwo
					),
				Origin
				);
	}
}

void UMaterialExpressionRotator::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinate, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Time, OldExpression, NewExpression );
}

//
//	UMaterialExpressionRotator::GetCaption
//

FString UMaterialExpressionRotator::GetCaption() const
{
	return TEXT("Rotator");
}

//
//	UMaterialExpressionSine::Compile
//

INT UMaterialExpressionSine::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Sine input"));
	}

	return Compiler->Sine(Period > 0.0f ? Compiler->Mul(Input.Compile(Compiler),Compiler->Constant(2.0f * (FLOAT)PI / Period)) : Input.Compile(Compiler));
}

void UMaterialExpressionSine::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

//
//	UMaterialExpressionSine::GetCaption
//

FString UMaterialExpressionSine::GetCaption() const
{
	return TEXT("Sine");
}

//
//	UMaterialExpressionCosine::Compile
//

INT UMaterialExpressionCosine::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Cosine input"));
	}

	return Compiler->Cosine(Compiler->Mul(Input.Compile(Compiler),Period > 0.0f ? Compiler->Constant(2.0f * (FLOAT)PI / Period) : 0));
}

void UMaterialExpressionCosine::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

//
//	UMaterialExpressionCosine::GetCaption
//

FString UMaterialExpressionCosine::GetCaption() const
{
	return TEXT("Cosine");
}

//
//	UMaterialExpressionBumpOffset::Compile
//

INT UMaterialExpressionBumpOffset::Compile(FMaterialCompiler* Compiler)
{
	if(!Height.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Height input"));
	}

	return Compiler->Add(
			Compiler->Mul(
				Compiler->ComponentMask(Compiler->CameraVector(),1,1,0,0),
				Compiler->Add(
					Compiler->Mul(
						Compiler->Constant(HeightRatio),
						Compiler->ForceCast(Height.Compile(Compiler),MCT_Float1)
						),
					Compiler->Constant(-ReferencePlane * HeightRatio)
					)
				),
			Coordinate.Expression ? Coordinate.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE)
			);
}

//
//	UMaterialExpressionBumpOffset::GetCaption
//

FString UMaterialExpressionBumpOffset::GetCaption() const
{
	return TEXT("BumpOffset");
}

void UMaterialExpressionBumpOffset::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Height, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinate, OldExpression, NewExpression );
}

//
//	UMaterialExpressionAppendVector::Compile
//

INT UMaterialExpressionAppendVector::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing AppendVector input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing AppendVector input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return Compiler->AppendVector(
			Arg1,
			Arg2
			);
	}
}

//
//	UMaterialExpressionAppendVector::GetCaption
//

FString UMaterialExpressionAppendVector::GetCaption() const
{
	return TEXT("Append");
}

void UMaterialExpressionAppendVector::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionFloor::Compile
//

INT UMaterialExpressionFloor::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Floor input"));
	}

	return Compiler->Floor(Input.Compile(Compiler));
}

//
//	UMaterialExpressionFloor::GetCaption
//

FString UMaterialExpressionFloor::GetCaption() const
{
	return TEXT("Floor");
}

void UMaterialExpressionFloor::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

//
//	UMaterialExpressionCeil::Compile
//

INT UMaterialExpressionCeil::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Ceil input"));
	}
	return Compiler->Ceil(Input.Compile(Compiler));
}

void UMaterialExpressionCeil::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

//
//	UMaterialExpressionCeil::GetCaption
//

FString UMaterialExpressionCeil::GetCaption() const
{
	return TEXT("Ceil");
}

//
//	UMaterialExpressionFmod
//
INT UMaterialExpressionFmod::Compile(FMaterialCompiler* Compiler)
{
	if (!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Fmod input A"));
	}
	if (!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Fmod input B"));
	}
	return Compiler->Fmod(A.Compile(Compiler), B.Compile(Compiler));
}

FString UMaterialExpressionFmod::GetCaption() const
{
	return TEXT("Fmod");
}

/**
 * Replaces references to the passed in expression with references to a different expression or NULL.
 * @param	OldExpression		Expression to find reference to.
 * @param	NewExpression		Expression to replace reference with.
 */
void UMaterialExpressionFmod::SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression)
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

//
//	UMaterialExpressionFrac::Compile
//

INT UMaterialExpressionFrac::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Frac input"));
	}

	return Compiler->Frac(Input.Compile(Compiler));
}

void UMaterialExpressionFrac::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

//
//	UMaterialExpressionFrac::GetCaption
//

FString UMaterialExpressionFrac::GetCaption() const
{
	return TEXT("Frac");
}

///////////////////////////////////////////////////////////
// UMaterialExpressionDepthBiasBlend
///////////////////////////////////////////////////////////
#define _DEPTHBIAS_USE_ONE_MINUS_BIAS_
/**
 *	Compile the material expression
 *
 *	@param	Compiler	Pointer to the material compiler to use
 *
 *	@return	INT			The compiled code index
 */	
INT UMaterialExpressionDepthBiasBlend::Compile(FMaterialCompiler* Compiler)
{
	//   +----------------+
	//   | DepthBiasBlend |
	//   +----------------+
	// --| RGB       Bias |--       BiasCalc = (1 - Bias) or -Bias... still determining.
	// --| R    BiasScale |--
	// --| G              |
	// --| B              |
	// --| Alpha          |
	//   +----------------+                     +-----+     +----------+
	//                      +------------+      | Sub |  /--| DstDepth |
	//        +-----+    /--| PixelDepth |      |    A|-/   +----------+-----------+
	//        | If  |   /   +------------+  /---|    B|-----| BiasCalc * BiasScale |
	// RGB ---|    A|--/  /----------------/    +-----+     +----------------------+
	//        |    B|----/  +-----+     +------------+                    +----------+
	//        |    >|--\    | If  |  /--| DstDepth   |     +--------+   /-| DstColor |
	//        |    =|-------|    A|-/   +------------+     | Lerp   |  /  +----------+
	//        |    <|-\     |    B|-----| PixelDepth |     |       A|-/ /-| SrcColor |   +----------+      +------------+
	//        +-----+ |     |    >|-\   +------------+     |       B|--/  +----------+   | Subtract |   /--| DstDepth   |
	//                |     |    =|------------------------|   Alpha|-\                  |         A|--/   +------------+
	//                |     |    <|--\                     +--------+  \              /--|         B|------| PixelDepth |
	//                |     +-----+   \                                 \   +-----+  /   +----------+      +------------+
	//                |  +----------+  \   +----------+                  \--| /  A|-/   +----------------------+
	//                \--| SrcColor |   \--| DstColor |                     |    B|-----| BiasCalc * BiasScale |
	//                   +----------+      +----------+                     +-----+     +----------------------+
	//
    //@todo. Add support for this
    //BITFIELD bNormalize:1;

#if EXCEPTIONS_DISABLED
	// if we can't throw the error below, attempt to thwart the error by using the default texture
	// @todo: handle missing cubemaps and 3d textures?
	if (!Texture)
	{
		debugf(TEXT("Using default texture instead of real texture!"));
		Texture = GWorld->GetWorldInfo()->DefaultTexture;
	}
#endif

	if (Texture)
	{
		INT SrcTextureCodeIndex = Compiler->Texture(Texture);

		INT ArgA	=	Compiler->TextureSample(
							SrcTextureCodeIndex,
							Coordinates.Expression ? 
								Coordinates.Compile(Compiler) : 
								Compiler->TextureCoordinate(0, FALSE, FALSE) 
							);

		INT Arg_SrcSample = UnpackTexture(Compiler, ArgA, Texture, Texture->CompressionSettings);

		INT Arg_DstSample	=	Compiler->DestColor();
		INT	Arg_SrcDepth	=	Compiler->PixelDepth(bNormalize);
		INT	Arg_DstDepth	=	Compiler->DestDepth(bNormalize);
		INT	Arg_Constant_0	=	Compiler->Constant(0.0f);
		INT	Arg_Constant_1	=	Compiler->Constant(1.0f);

#if defined(_DEPTHBIAS_USE_ONE_MINUS_BIAS_)
		INT	Arg_Bias		=	(Bias.Expression) ? 
									Compiler->Sub(Arg_Constant_1, Bias.Compile(Compiler)) : 
									Arg_Constant_1;
#else	//#if defined(_DEPTHBIAS_USE_ONE_MINUS_BIAS_)
		INT	Arg_Bias		=	(Bias.Expression) ? 
									Compiler->Sub(Arg_Constant_0, Bias.Compile(Compiler)) : 
									Arg_Constant_0;
#endif	//#if defined(_DEPTHBIAS_USE_ONE_MINUS_BIAS_)
		
		INT	Arg_BiasScaleConstant		=	Compiler->Constant(BiasScale);
		INT	Arg_ScaledBias				=	Compiler->Mul(Arg_Bias, Arg_BiasScaleConstant);
		INT	Arg_Sub_DstDepth_Bias		=	Compiler->Sub(Arg_DstDepth, Arg_ScaledBias);
		INT	Arg_Sub_DstDepth_SrcDepth	=	Compiler->Sub(Arg_DstDepth, Arg_SrcDepth);
		INT	Arg_Div_DZ_Sub_SZ_Bias		=	Compiler->Div(Arg_Sub_DstDepth_SrcDepth, Arg_ScaledBias);
		INT Arg_ClampedLerpBias			=	Compiler->Clamp(Arg_Div_DZ_Sub_SZ_Bias, Arg_Constant_0, Arg_Constant_1);

		INT	Arg_Lerp_Dst_Src_Color		=	
								Compiler->Lerp(
									Arg_DstSample, 
									Arg_SrcSample, 
									Arg_ClampedLerpBias
									);

		INT	Arg_If_DZ_SZ	=	Compiler->If(
									Arg_DstDepth, 
									Arg_SrcDepth, 
									Arg_Lerp_Dst_Src_Color, 
									Arg_Lerp_Dst_Src_Color, 
									Arg_DstSample
									);

		INT	Arg_If_SZ_DZ_Add_Bias	= Compiler->If(
										Arg_SrcDepth,
										Arg_Sub_DstDepth_Bias,
										Arg_If_DZ_SZ,
										Arg_If_DZ_SZ,
										Arg_SrcSample
										);

		return Arg_If_SZ_DZ_Add_Bias;
	}
	else
	{
		if (Desc.Len() > 0)
		{
			return Compiler->Errorf(TEXT("%s> Missing input texture"), *Desc);
		}
		else
		{
			return Compiler->Errorf(TEXT("TextureSample> Missing input texture"));
		}
	}
}

/**
 *	Get the outputs associated with the expression
 *
 *	@param	Outputs		The array that contains the output expression
 */	
void UMaterialExpressionDepthBiasBlend::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
}

/**
 */	
INT UMaterialExpressionDepthBiasBlend::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 */	
FString UMaterialExpressionDepthBiasBlend::GetCaption() const
{
	return TEXT("DepthBiasBlend");
}

void UMaterialExpressionDepthBiasBlend::SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression)
{
	Super::SwapReferenceTo(OldExpression,NewExpression);
	SWAP_REFERENCE_TO(Bias, OldExpression, NewExpression );
	SWAP_REFERENCE_TO(Coordinates, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////
// UMaterialExpressionDepthBiasedAlpha
///////////////////////////////////////////////////////////
    //## BEGIN PROPS MaterialExpressionDepthBiasedAlpha
//    BITFIELD bNormalize:1;
//    FLOAT BiasScale;
//    FExpressionInput Alpha;
//    FExpressionInput Bias;
    //## END PROPS MaterialExpressionDepthBiasedAlpha

/**
 *	Compile the material expression
 *
 *	@param	Compiler	Pointer to the material compiler to use
 *
 *	@return	INT			The compiled code index
 */	
INT UMaterialExpressionDepthBiasedAlpha::Compile(FMaterialCompiler* Compiler)
{
	INT ResultIdx = INDEX_NONE;

	// source alpha input
	INT Arg_SourceA = Alpha.Expression ? Alpha.Compile(Compiler) : Compiler->Constant(1.0f);
	// bias input
	INT Arg_Bias = Bias.Expression ? Bias.Compile(Compiler) : Compiler->Constant(0.5f);
	// bias scale
	INT Arg_BiasScale = Compiler->Constant(BiasScale);

	// make sure source alpha is 1 component
	EMaterialValueType Type_SourceA = Compiler->GetType(Arg_SourceA);
	if (!(Type_SourceA & MCT_Float1))
	{
		Arg_SourceA = Compiler->ComponentMask(Arg_SourceA, 1, 0, 0, 0);
	}

	// add depth bias alpha code chunk
	ResultIdx = Compiler->DepthBiasedAlpha(Arg_SourceA, Arg_Bias, Arg_BiasScale);

	return ResultIdx;
}

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionDepthBiasedAlpha::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	return UMaterialExpression::GetOutputs(Outputs);
}

/**
 */	
INT UMaterialExpressionDepthBiasedAlpha::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 */	
FString UMaterialExpressionDepthBiasedAlpha::GetCaption() const
{
	return TEXT("DepthBiasedAlpha");
}

void UMaterialExpressionDepthBiasedAlpha::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo(OldExpression,NewExpression);
	SWAP_REFERENCE_TO(Bias, OldExpression, NewExpression );
	SWAP_REFERENCE_TO(Alpha, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////
// UMaterialExpressionDepthBiasedBlend
///////////////////////////////////////////////////////////

//## BEGIN PROPS MaterialExpressionDepthBiasedBlend
//BITFIELD bNormalize:1;
//FLOAT BiasScale;
//FExpressionInput SourceRGB;
//FExpressionInput SourceA;
//FExpressionInput Bias;
//## END PROPS MaterialExpressionDepthBiasedBlend

/**
 *	Compile the material expression
 *
 *	@param	Compiler	Pointer to the material compiler to use
 *
 *	@return	INT			The compiled code index
 */	
INT UMaterialExpressionDepthBiasedBlend::Compile(FMaterialCompiler* Compiler)
{
	INT ResultIdx = INDEX_NONE;

	// source RGB color input 
	INT	Arg_SourceRGB	= RGB.Expression	? RGB.Compile( Compiler ) : Compiler->Constant3( 0.f, 0.f, 0.f );
	// source Alpha input 
	INT	Arg_SourceA		= Alpha.Expression	? Alpha.Compile( Compiler )	: Compiler->Constant( 1.0f );
	// bias input
	INT Arg_Bias = Bias.Compile( Compiler );
	// bias scale property
	INT Arg_BiasScale = Compiler->Constant( BiasScale );

	// make sure source Alpha is 1 component
	EMaterialValueType Type_SourceA = Compiler->GetType( Arg_SourceA );
	if( !(Type_SourceA & MCT_Float1) )
	{
		return Compiler->Errorf( TEXT("Alpha input must be float1") );
	}
	// make sure source RGB is 3 components
	EMaterialValueType Type_SourceRGB = Compiler->GetType( Arg_SourceRGB );
	if( Type_SourceRGB == MCT_Float4 )
	{
		Arg_SourceRGB = Compiler->ComponentMask( Arg_SourceRGB, 1, 1, 1, 0 );
	}

	// add depth bias blend code chunk
	INT	Arg_RGB_Component	= Compiler->DepthBiasedBlend( Arg_SourceRGB, Arg_Bias, Arg_BiasScale );
	// append the alpha to RGB for a 4 component result
	ResultIdx =  Compiler->AppendVector(Arg_RGB_Component, Arg_SourceA);

	return ResultIdx;
}


/**
 *	Get the outputs associated with the expression
 *
 *	@param	Outputs		The array that contains the output expression
 */	
void UMaterialExpressionDepthBiasedBlend::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

/**
 */	
INT UMaterialExpressionDepthBiasedBlend::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 */	
FString UMaterialExpressionDepthBiasedBlend::GetCaption() const
{
	return TEXT("DepthBiasedBlend");
}

void UMaterialExpressionDepthBiasedBlend::SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression)
{
	Super::SwapReferenceTo(OldExpression,NewExpression);
	SWAP_REFERENCE_TO(Bias, OldExpression, NewExpression );
	SWAP_REFERENCE_TO(Alpha, OldExpression, NewExpression );
	SWAP_REFERENCE_TO(RGB, OldExpression, NewExpression );
}

//
//	UMaterialExpressionDesaturation::Compile
//

INT UMaterialExpressionDesaturation::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
		return Compiler->Errorf(TEXT("Missing Desaturation input"));

	INT	Color = Input.Compile(Compiler),
		Grey = Compiler->Dot(Color,Compiler->Constant3(LuminanceFactors.R,LuminanceFactors.G,LuminanceFactors.B));

	if(Percent.Expression)
		return Compiler->Lerp(Color,Grey,Percent.Compile(Compiler));
	else
		return Grey;
}

void UMaterialExpressionDesaturation::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Percent, OldExpression, NewExpression );
}

//
//	UMaterialExpressionParameter
//


/** 
 * Generates a GUID for this expression if one doesn't already exist. 
 *
 * @param bForceGeneration	Whether we should generate a GUID even if it is already valid.
 */
void UMaterialExpressionParameter::ConditionallyGenerateGUID(UBOOL bForceGeneration)
{
	// If we are in the editor, and we don't have a valid GUID yet, generate one.
	if(GIsEditor && !GIsGame)
	{
		if(bForceGeneration || ExpressionGUID.IsValid()==FALSE)
		{
			ExpressionGUID = appCreateGuid();
			MarkPackageDirty();
		}
	}
}

/** Tries to generate a GUID. */
void UMaterialExpressionParameter::PostLoad()
{
	Super::PostLoad();

	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionParameter::PostDuplicate()
{
	Super::PostDuplicate();

	// We do not force a guid regen here because this function is used when the Material Editor makes a copy of a material to edit.
	// If we forced a GUID regen, it would cause all of the guids for a material to change everytime a material was edited.
	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionParameter::PostEditImport()
{
	Super::PostEditImport();

	ConditionallyGenerateGUID(TRUE);
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionParameter::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( ParameterName.ToString().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}

//
//	UMaterialExpressionVectorParameter::Compile
//

INT UMaterialExpressionVectorParameter::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->VectorParameter(ParameterName,DefaultValue);
}

//
//	UMaterialExpressionVectorParameter::GetOutputs
//

void UMaterialExpressionVectorParameter::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

FString UMaterialExpressionVectorParameter::GetCaption() const
{
	return FString::Printf(
		 TEXT("Param '%s' (%.3g,%.3g,%.3g,%.3g)"),
		 *ParameterName.ToString(),
		 DefaultValue.R,
		 DefaultValue.G,
		 DefaultValue.B,
		 DefaultValue.A );
}
 
//
//	UMaterialExpressionScalarParameter::Compile
//

INT UMaterialExpressionScalarParameter::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ScalarParameter(ParameterName,DefaultValue);
}

FString UMaterialExpressionScalarParameter::GetCaption() const
{
	 return FString::Printf(
		 TEXT("Param '%s' (%.4g)"),
		 *ParameterName.ToString(),
		 DefaultValue ); 
}


//
//	UMaterialExpressionStaticSwitchParameter::Compile
//

INT UMaterialExpressionStaticSwitchParameter::Compile(FMaterialCompiler* Compiler)
{
	//if the override is set, use the values from it
	if (InstanceOverride)
	{
		if (InstanceOverride->Value)
		{
			return A.Compile(Compiler);
		}
		else
		{
			return B.Compile(Compiler);
		}
	}
	else
	{
		//use the default values for the expression
		if (DefaultValue)
		{
			return A.Compile(Compiler);
		}
		else
		{
			return B.Compile(Compiler);
		}
	}
}

FString UMaterialExpressionStaticSwitchParameter::GetCaption() const
{
	if (ExtendedCaptionDisplay)
	{
		if (DefaultValue)
		{
			return FString::Printf(TEXT("Static Switch Param '%s' (TRUE)"), *ParameterName.ToString()); 
		}
		else
		{
			return FString::Printf(TEXT("Static Switch Param '%s' (FALSE)"), *ParameterName.ToString()); 
		}
	}
	else
	{
		return FString::Printf(TEXT("Switch Param '%s'"), *ParameterName.ToString()); 
	}
}

void UMaterialExpressionStaticSwitchParameter::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

#if WITH_EDITOR
/**
 *	Called by the CleanupMaterials function, this will clear the inputs of the expression.
 *	This only needs to be implemented by expressions that have bUsedByStaticParameterSet set to TRUE.
 */
void UMaterialExpressionStaticSwitchParameter::ClearInputExpressions()
{
	A.Expression = NULL;
	B.Expression = NULL;
}
#endif

//
//	UMaterialExpressionNormalize
//

INT UMaterialExpressionNormalize::Compile(FMaterialCompiler* Compiler)
{
	if(!VectorInput.Expression)
		return Compiler->Errorf(TEXT("Missing Normalize input"));

	INT	V = VectorInput.Compile(Compiler);

	return Compiler->Div(V,Compiler->SquareRoot(Compiler->Dot(V,V)));
}

void UMaterialExpressionNormalize::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( VectorInput, OldExpression, NewExpression );
}

//
INT UMaterialExpressionVertexColor::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->VertexColor();
}

void UMaterialExpressionVertexColor::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

FString UMaterialExpressionVertexColor::GetCaption() const
{
	return TEXT("Vertex Color");
}

//
//	MaterialExpressionEmittterParameter
//
void UMaterialExpressionDynamicParameter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

/**
 * Replaces references to the passed in expression with references to a different expression or NULL.
 * @param	OldExpression		Expression to find reference to.
 * @param	NewExpression		Expression to replace reference with.
 */
void UMaterialExpressionDynamicParameter::SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression)
{
	Super::SwapReferenceTo(OldExpression, NewExpression);
}

/**
 * Creates the new shader code chunk needed for the Abs expression
 *
 * @param	Compiler - Material compiler that knows how to handle this expression
 * @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
 */	
INT UMaterialExpressionDynamicParameter::Compile( FMaterialCompiler* Compiler )
{
	return Compiler->DynamicParameter();
}

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionDynamicParameter::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);

	Outputs(0).Name = *(ParamNames(0));
	Outputs(1).Name = *(ParamNames(1));
	Outputs(2).Name = *(ParamNames(2));
	Outputs(3).Name = *(ParamNames(3));
}

/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionDynamicParameter::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 * Textual description for this material expression
 *
 * @return	Caption text
 */	
FString UMaterialExpressionDynamicParameter::GetCaption() const
{
	return TEXT("Dynamic Parameter");
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionDynamicParameter::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	for( INT Index=0;Index<ParamNames.Num();Index++ )
	{
		if( ParamNames(Index).InStr(SearchQuery, FALSE, TRUE) != -1 )
		{
			return TRUE;
		}
	}

	return Super::MatchesSearchQuery(SearchQuery);
}

//
//	MaterialExpressionParticleSubUV
//
INT UMaterialExpressionParticleSubUV::Compile(FMaterialCompiler* Compiler)
{
	if (Texture)
	{
		// Out	 = linear interpolate... using 2 sub-images of the texture
		// A	 = RGB sample texture with TexCoord0
		// B	 = RGB sample texture with TexCoord1
		// Alpha = x component of TexCoord2
		//
		INT TextureCodeIndexA = Compiler->Texture(Texture);
		INT TextureCodeIndexB = Compiler->Texture(Texture);

		INT TexSampleA = Compiler->TextureSample(TextureCodeIndexA, Compiler->TextureCoordinate(0, FALSE, FALSE) );
		INT TexSampleB = Compiler->TextureSample(TextureCodeIndexB, Compiler->TextureCoordinate(1, FALSE, FALSE) );

		INT LerpA = UnpackTexture(Compiler, TexSampleA, Texture, Texture->CompressionSettings);
		INT LerpB = UnpackTexture(Compiler, TexSampleB, Texture, Texture->CompressionSettings);

		return 
			Compiler->Lerp(
				LerpA, 
				LerpB, 
				// Lerp 'alpha' comes from the 3rd texture set...
				Compiler->ComponentMask(
					Compiler->TextureCoordinate(2, FALSE, FALSE), 
					1, 0, 0, 0
				)
			);

	}
	else
	{
		return Compiler->Errorf(TEXT("Missing ParticleSubUV input texture"));
	}
}

void UMaterialExpressionParticleSubUV::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

INT UMaterialExpressionParticleSubUV::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

FString UMaterialExpressionParticleSubUV::GetCaption() const
{
	return TEXT("Particle SubUV");
}

//
//	MaterialExpressionParticleMacroUV
//
INT UMaterialExpressionParticleMacroUV::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ParticleMacroUV(bUseViewSpace);
}

FString UMaterialExpressionParticleMacroUV::GetCaption() const
{
	return TEXT("Particle MacroUV");
}

//
//	UMaterialExpressionMeshSubUV
//
INT UMaterialExpressionMeshSubUV::Compile(FMaterialCompiler* Compiler)
{
	if (Texture)
	{
		//                         | Mul |
		//	VectorParam(Scale)---->|A    |----\           | Add |   |Sample|
		//	TextureCoord(0)------->|B    |     \--------->|A    |-->|Coord |
		//	VectorParam(SubUVMeshOffset)-->CompMask(XY)-->|B    |   |      |

		// Out	 = sub-UV sample of the input texture
		//
		INT TextureCodeIndex	= Compiler->Texture(Texture);

		INT TexSample = Compiler->TextureSample(
							TextureCodeIndex,
							Compiler->Add(
								Compiler->Mul(
									Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
									Compiler->ComponentMask(
										Compiler->VectorParameter(FName(TEXT("TextureScaleParameter")),FLinearColor::White),
										1, 
										1, 
										0, 
										0
										)
									),
								Compiler->ComponentMask(
									Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
									1, 
									1, 
									0, 
									0
									)
								)
							);


		return UnpackTexture(Compiler, TexSample, Texture, Texture->CompressionSettings);
	}
	else
	{
		return Compiler->Errorf(TEXT("%s missing texture"), *GetCaption());
	}
}

void UMaterialExpressionMeshSubUV::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

INT UMaterialExpressionMeshSubUV::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

FString UMaterialExpressionMeshSubUV::GetCaption() const
{
	return TEXT("Mesh SubUV");
}

//
//	UMaterialExpressionMeshSubUVBlend
//
INT UMaterialExpressionMeshSubUVBlend::Compile(FMaterialCompiler* Compiler)
{
	if (Texture)
	{
		// Out	 = sub-UV sample of the input texture
		//
		INT TextureCodeIndex		= Compiler->Texture(Texture);
		INT UnpackScaleIndex		= Compiler->Constant4(
										Texture->UnpackMax[0] - Texture->UnpackMin[0],
										Texture->UnpackMax[1] - Texture->UnpackMin[1],
										Texture->UnpackMax[2] - Texture->UnpackMin[2],
										Texture->UnpackMax[3] - Texture->UnpackMin[3]
										);
		INT UnpackOffsetIndex		= Compiler->Constant4(
										Texture->UnpackMin[0],
										Texture->UnpackMin[1],
										Texture->UnpackMin[2],
										Texture->UnpackMin[3]
										);

		INT ScaleParameterIndex		= Compiler->ComponentMask(
										Compiler->VectorParameter(FName(TEXT("TextureScaleParameter")),FLinearColor::White),
										1, 
										1, 
										0, 
										0
										);

		INT OffsetParameterIndex_0	= Compiler->ComponentMask(
										Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
										1, 
										1, 
										0, 
										0
										);

		INT TextureSampleIndex_0	= Compiler->TextureSample(
										TextureCodeIndex,
										Compiler->Add(
											Compiler->Mul(
												Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
												ScaleParameterIndex
												),
											OffsetParameterIndex_0
											)
										);

		INT OffsetParameterIndex_1	= Compiler->ComponentMask(
										Compiler->VectorParameter(FName(TEXT("TextureOffset1Parameter")),FLinearColor::Black),
										1, 
										1, 
										0, 
										0
										);

		INT TextureSampleIndex_1	= Compiler->TextureSample(
										TextureCodeIndex,
										Compiler->Add(
											Compiler->Mul(
												Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE),
												ScaleParameterIndex
												),
											OffsetParameterIndex_1
											)
										);

		INT ResultIndex_0, ResultIndex_1;
		switch( Texture->CompressionSettings )
		{
		case TC_Normalmap:
		case TC_NormalmapAlpha:
		case TC_NormalmapUncompressed:
		case TC_NormalmapBC5:
			ResultIndex_0 = Compiler->BiasNormalizeNormalMap(TextureSampleIndex_0, Texture->CompressionSettings);
			ResultIndex_1 = Compiler->BiasNormalizeNormalMap(TextureSampleIndex_1, Texture->CompressionSettings);
			break;
		default:
			ResultIndex_0 = Compiler->Add(Compiler->Mul(TextureSampleIndex_0, UnpackScaleIndex),UnpackOffsetIndex);
			ResultIndex_1 = Compiler->Add(Compiler->Mul(TextureSampleIndex_1, UnpackScaleIndex),UnpackOffsetIndex);
			break;
		}

		INT LerpAlphaIndex			= Compiler->ComponentMask(
										Compiler->VectorParameter(FName(TEXT("TextureOffsetParameter")),FLinearColor::Black),
										0, 
										0, 
										1, 
										0
										);

		return Compiler->Lerp(
				ResultIndex_0, 
				ResultIndex_1, 
				LerpAlphaIndex);
	}
	else
	{
		return Compiler->Errorf(TEXT("%s missing texture"), *GetCaption());
	}
}

FString UMaterialExpressionMeshSubUVBlend::GetCaption() const
{
	return TEXT("Mesh SubUV Blend");
}

/**
 *	LensFlareIntensity
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionLensFlareIntensity::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->LensFlareIntesity();
}
	
/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLensFlareIntensity::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}
	
/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionLensFlareIntensity::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}
	
/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionLensFlareIntensity::GetCaption() const
{
	return TEXT("LensFlare Intensity");
}

/**
 *	LensFlareOcclusion
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionLensFlareOcclusion::Compile(FMaterialCompiler* Compiler)
{
	// Use the new, common OcclusionPercentage function
	return Compiler->OcclusionPercentage();
}
	
/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLensFlareOcclusion::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}
	
/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionLensFlareOcclusion::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}
	
/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionLensFlareOcclusion::GetCaption() const
{
	return TEXT("LensFlare Occlusion");
}

/**
 *	LensFlareRadialDistance
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionLensFlareRadialDistance::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->LensFlareRadialDistance();
}

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLensFlareRadialDistance::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}

/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionLensFlareRadialDistance::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionLensFlareRadialDistance::GetCaption() const
{
	return TEXT("LensFlare RadialDist");
}

/**
 *	LensFlareRayDistance
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionLensFlareRayDistance::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->LensFlareRayDistance();
}
	
/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLensFlareRayDistance::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}
	
/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionLensFlareRayDistance::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}
	
/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionLensFlareRayDistance::GetCaption() const
{
	return TEXT("LensFlare RayDist");
}	

/**
 *	LensFlareSourceDistance
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionLensFlareSourceDistance::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->LensFlareSourceDistance();
}	

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLensFlareSourceDistance::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}

/**
 *	Get the width required by this expression (in the material editor).
 *
 *	@return	INT			The width in pixels.
 */
INT UMaterialExpressionLensFlareSourceDistance::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionLensFlareSourceDistance::GetCaption() const
{
	return TEXT("LensFlare SourceDist");
}

INT UMaterialExpressionLightVector::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->LightVector();
}

FString UMaterialExpressionLightVector::GetCaption() const
{
	return TEXT("Light Vector");
}

INT UMaterialExpressionScreenPosition::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ScreenPosition( ScreenAlign );
}

FString UMaterialExpressionScreenPosition::GetCaption() const
{
	return TEXT("ScreenPos");
}

INT UMaterialExpressionSquareRoot::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing square root input"));
	}
	return Compiler->SquareRoot(Input.Compile(Compiler));
}

FString UMaterialExpressionSquareRoot::GetCaption() const
{
	return TEXT("Sqrt");
}

void UMaterialExpressionSquareRoot::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionPixelDepth
///////////////////////////////////////////////////////////////////////////////
/**
 *	Compile the expression
 *
 *	@param	Compiler	The compiler to utilize
 *
 *	@return				The index of the resulting code chunk.
 *						INDEX_NONE if error.
 */
INT UMaterialExpressionPixelDepth::Compile(FMaterialCompiler* Compiler)
{
	// resulting index to compiled code chunk
	// add the code chunk for the scene depth sample        
	INT Result = Compiler->PixelDepth(bNormalize);
	return Result;
}

/**
 *	Retrieve the outputs this expression supplies.
 *
 *	@param	Outputs		The array to insert the available outputs in.
 */
void UMaterialExpressionPixelDepth::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	// Depth
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}

/**
 *	Get the caption to display on this material expression
 *
 *	@return			An FString containing the display caption
 */
FString UMaterialExpressionPixelDepth::GetCaption() const
{
	return TEXT("PixelDepth");
}

//
INT UMaterialExpressionPower::Compile(FMaterialCompiler* Compiler)
{
	if(!Base.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Power Base input"));
	}
	if(!Exponent.Expression)
	{
		return Compiler->Errorf(TEXT("Missing Power Exponent input"));
	}

	INT Arg1 = Base.Compile(Compiler);
	INT Arg2 = Exponent.Compile(Compiler);
	return Compiler->Power(
		Arg1,
		Arg2
		);
}

FString UMaterialExpressionPower::GetCaption() const
{
	return TEXT("Power");
}

void UMaterialExpressionPower::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Base, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Exponent, OldExpression, NewExpression );
}

INT UMaterialExpressionIf::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing If A input"));
	}
	if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing If B input"));
	}
	if(!AGreaterThanB.Expression)
	{
		return Compiler->Errorf(TEXT("Missing If AGreaterThanB input"));
	}
	if(!AEqualsB.Expression)
	{
		return Compiler->Errorf(TEXT("Missing If AEqualsB input"));
	}
	if(!ALessThanB.Expression)
	{
		return Compiler->Errorf(TEXT("Missing If ALessThanB input"));
	}

	INT CompiledA = A.Compile(Compiler);
	INT CompiledB = B.Compile(Compiler);

	if(Compiler->GetType(CompiledA) != MCT_Float)
	{
		return Compiler->Errorf(TEXT("If input A must be of type float."));
	}

	if(Compiler->GetType(CompiledB) != MCT_Float)
	{
		return Compiler->Errorf(TEXT("If input B must be of type float."));
	}

	INT Arg3 = AGreaterThanB.Compile(Compiler);
	INT Arg4 = AEqualsB.Compile(Compiler);
	INT Arg5 = ALessThanB.Compile(Compiler);

	return Compiler->If(CompiledA,CompiledB,Arg3, Arg4, Arg5);
}

FString UMaterialExpressionIf::GetCaption() const
{
	return TEXT("If");
}

void UMaterialExpressionIf::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( AGreaterThanB, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( AEqualsB, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( ALessThanB, OldExpression, NewExpression );
}

/**
 *	Occlusion Percentage expression
 */
/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionOcclusionPercentage::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->OcclusionPercentage();
}

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionOcclusionPercentage::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}
	
/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionOcclusionPercentage::GetCaption() const
{
	return TEXT("OcclusionPercentage");
}	

INT UMaterialExpressionOneMinus::Compile(FMaterialCompiler* Compiler)
{
	if(!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing 1-x input"));
	}
	return Compiler->Sub(Compiler->Constant(1.0f),Input.Compile(Compiler));
}

FString UMaterialExpressionOneMinus::GetCaption() const
{
	return TEXT("1-x");
}

void UMaterialExpressionOneMinus::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

/**
 * Creates the new shader code chunk needed for the Abs expression
 *
 * @param	Compiler - Material compiler that knows how to handle this expression
 * @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
 */	
INT UMaterialExpressionAbs::Compile( FMaterialCompiler* Compiler )
{
	INT Result=INDEX_NONE;

	if( !Input.Expression )
	{
		// an input expression must exist
		Result = Compiler->Errorf( TEXT("Missing Abs input") );
	}
	else
	{
		// evaluate the input expression first and use that as
		// the parameter for the Abs expression
		Result = Compiler->Abs( Input.Compile(Compiler) );
	}

	return Result;
}

/**
 * Textual description for this material expression
 *
 * @return	Caption text
 */	
FString UMaterialExpressionAbs::GetCaption() const
{
	return TEXT("Abs");
}

void UMaterialExpressionAbs::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionSceneTexture
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for sampling the scene's lighting texture
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionSceneTexture::Compile( FMaterialCompiler* Compiler )
{
	// resulting index to compiled code chunk
	INT Result=INDEX_NONE;
	// resulting index to compiled code for the tex coordinates if available
	INT CoordIdx = INDEX_NONE;
	// if there are valid texture coordinate inputs then compile them
	if( Coordinates.Expression )
	{
		CoordIdx = Coordinates.Compile(Compiler);
	}
    // add the code chunk for the scene texture sample        
	Result = Compiler->SceneTextureSample( SceneTextureType, CoordIdx, ScreenAlign );
	return Result;
}

/**
* Fill in the array of valid outputs for this material expression
*
* @param	Outputs - array of this expression's valid outputs
*/	
void UMaterialExpressionSceneTexture::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	// RGB
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	// R
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	// G
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	// B
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	// A
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

void UMaterialExpressionSceneTexture::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinates, OldExpression, NewExpression );
}

/**
* Text description of this expression
*/
FString UMaterialExpressionSceneTexture::GetCaption() const
{
	return TEXT("Scene Texture Sample");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionSceneDepth
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for sampling the scene's depth
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionSceneDepth::Compile( FMaterialCompiler* Compiler )
{
	// resulting index to compiled code chunk
	INT Result=INDEX_NONE;
	// resulting index to compiled code for the tex coordinates if available
	INT CoordIdx = INDEX_NONE;
	// if there are valid texture coordinate inputs then compile them
	if( Coordinates.Expression )
	{
		CoordIdx = Coordinates.Compile(Compiler);
	}
	// add the code chunk for the scene depth sample        
	Result = Compiler->SceneTextureDepth( bNormalize, CoordIdx );
	return Result;
}

/**
* Fill in the array of valid outputs for this material expression
*
* @param	Outputs - array of this expression's valid outputs
*/	
void UMaterialExpressionSceneDepth::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,0,0,0);
}

void UMaterialExpressionSceneDepth::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinates, OldExpression, NewExpression );
}

/**
* Text description of this expression
*/
FString UMaterialExpressionSceneDepth::GetCaption() const
{
	return TEXT("Scene Depth");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionTransform
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for transforming an input vector to a new coordinate space
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionTransform::Compile(FMaterialCompiler* Compiler)
{
	INT Result=INDEX_NONE;

	if( !Input.Expression )
	{
		Result = Compiler->Errorf(TEXT("Missing Transform input vector"));
	}
	else
	{
		INT VecInputIdx = Input.Compile(Compiler);
		Result = Compiler->TransformVector( TransformSourceType, TransformType, VecInputIdx );
	}

	return Result;
}

/**
* Text description of this expression
*/
FString UMaterialExpressionTransform::GetCaption() const
{
	return TEXT("Vector Transform");
}

void UMaterialExpressionTransform::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionTransformPosition
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for transforming an input vector to a new coordinate space
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionTransformPosition::Compile(FMaterialCompiler* Compiler)
{
	INT Result=INDEX_NONE;

	if( !Input.Expression )
	{
		Result = Compiler->Errorf(TEXT("Missing Transform Position input vector"));
	}
	else
	{
		INT VecInputIdx = Input.Compile(Compiler);
		Result = Compiler->TransformPosition( TransformType, VecInputIdx );
	}

	return Result;
}

/**
* Text description of this expression
*/
FString UMaterialExpressionTransformPosition::GetCaption() const
{
	return TEXT("Position Transform");
}

void UMaterialExpressionTransformPosition::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionComment
///////////////////////////////////////////////////////////////////////////////

/**
 * Text description of this expression.
 */
FString UMaterialExpressionComment::GetCaption() const
{
	return TEXT("Comment");
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionComment::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( Text.InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}


///////////////////////////////////////////////////////////////////////////////
// MaterialExpressionCompound
///////////////////////////////////////////////////////////////////////////////

/**
 * Text description of this expression.
 */
FString UMaterialExpressionCompound::GetCaption() const
{
	return Caption.Len() > 0 ? *Caption : TEXT("Compound Expression");
}

/**
 * Recursively gathers all UMaterialExpression objects referenced by this expression.
 * Including self.
 *
 * @param	Expressions		Reference to array of material expressions to add to
 */	
void UMaterialExpressionCompound::GetExpressions( TArray<const UMaterialExpression*>& Expressions ) const
{
	//Super::GetExpressions( Expressions );
}

void UMaterialExpressionCompound::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	for(INT ExpressionIndex = 0;ExpressionIndex < MaterialExpressions.Num();ExpressionIndex++)
	{
		if(MaterialExpressions(ExpressionIndex) == OldExpression)
		{
			MaterialExpressions(ExpressionIndex) = NewExpression;
		}
	}
	MaterialExpressions.RemoveItem(NULL);
}

void UMaterialExpressionFresnel::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Normal, OldExpression, NewExpression );
}

/**
 * Spits out the proper shader code for the approximate Fresnel term
 *
 * @param Compiler the compiler compiling this expression
 */
INT UMaterialExpressionFresnel::Compile(FMaterialCompiler* Compiler)
{
	// pow(1 - max(0,Normal dot Camera),Exponent)
	//
	INT NormalArg = Normal.Expression ? Normal.Compile(Compiler) : Compiler->Constant3(0.f,0.f,1.f);
	INT DotArg = Compiler->Dot(NormalArg,Compiler->CameraVector());
	INT MaxArg = Compiler->Max(Compiler->Constant(0.f),DotArg);
	INT MinusArg = Compiler->Sub(Compiler->Constant(1.f),MaxArg);
	INT PowArg = Compiler->Power(MinusArg,Compiler->Constant(Exponent));
	return PowArg;
}

/*-----------------------------------------------------------------------------
UMaterialExpressionFontSample
-----------------------------------------------------------------------------*/

/** 
* Generate the compiled material string for this expression
* @param Compiler - shader material compiler
* @return index to the new generated expression
*/
INT UMaterialExpressionFontSample::Compile(FMaterialCompiler* Compiler)
{
	INT Result = -1;
#if EXCEPTIONS_DISABLED
	// if we can't throw the error below, attempt to thwart the error by using the default font
	if( !Font )
	{
		debugf(TEXT("Using default font instead of real font!"));
		Font = GEngine->GetMediumFont();
		FontTexturePage = 0;
	}
	else if( !Font->Textures.IsValidIndex(FontTexturePage) )
	{
		debugf(TEXT("Invalid font page %d. Max allowed is %d"),FontTexturePage,Font->Textures.Num());
		FontTexturePage = 0;
	}
#endif
	if( Font )
	{
		if( Font->Textures.IsValidIndex(FontTexturePage) )
		{
			UTexture* Texture = Font->Textures(FontTexturePage);
			if( !Texture )
			{
				debugf(TEXT("Invalid font texture. Using default texture"));
				Texture = Texture = GWorld->GetWorldInfo()->DefaultTexture;
			}
			check(Texture);

			INT TextureCodeIndex = Compiler->Texture(Texture);
			INT ArgA = Compiler->TextureSample(
				TextureCodeIndex,
				Compiler->TextureCoordinate(0, FALSE, FALSE) 
				);
			INT ArgB = Compiler->Constant4(
				Texture->UnpackMax[0] - Texture->UnpackMin[0],
				Texture->UnpackMax[1] - Texture->UnpackMin[1],
				Texture->UnpackMax[2] - Texture->UnpackMin[2],
				Texture->UnpackMax[3] - Texture->UnpackMin[3]
			);
			INT ArgC =  Compiler->Constant4(
				Texture->UnpackMin[0],
				Texture->UnpackMin[1],
				Texture->UnpackMin[2],
				Texture->UnpackMin[3]
			);
			Result = Compiler->Add(Compiler->Mul(ArgA, ArgB), ArgC);
		}
		else
		{
			Result = CompilerError(Compiler, *FString::Printf(TEXT("Invalid font page %d. Max allowed is %d"),FontTexturePage,Font->Textures.Num()));
		}		
	}
	else
	{
		Result = CompilerError(Compiler, TEXT("Missing input Font"));
	}
	return Result;
}

/**
* List of outputs from this expression
* @param Outputs - out list of expression
*/
void UMaterialExpressionFontSample::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(1,1,1,1,0);
	new(Outputs) FExpressionOutput(1,1,0,0,0);
	new(Outputs) FExpressionOutput(1,0,1,0,0);
	new(Outputs) FExpressionOutput(1,0,0,1,0);
	new(Outputs) FExpressionOutput(1,0,0,0,1);
}

/**
* Width of the thumbnail for this expression int he material editor
* @return size in pixels
*/
INT UMaterialExpressionFontSample::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

/**
* Caption description for this expression
* @return string caption
*/
FString UMaterialExpressionFontSample::GetCaption() const
{
	return TEXT("Font Sample");
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionFontSample::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( Font!=NULL && Font->GetName().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}

/*-----------------------------------------------------------------------------
UMaterialExpressionFontSampleParameter
-----------------------------------------------------------------------------*/

/** 
* Generate the compiled material string for this expression
* @param Compiler - shader material compiler
* @return index to the new generated expression
*/
INT UMaterialExpressionFontSampleParameter::Compile(FMaterialCompiler* Compiler)
{
	INT Result = -1;
	if( !ParameterName.IsValid() || 
		ParameterName.GetIndex() == NAME_None || 
		!Font ||
		!Font->Textures.IsValidIndex(FontTexturePage) )
	{
		Result = UMaterialExpressionFontSample::Compile(Compiler);
	}
	else 
	{
		UTexture* Texture = Font->Textures(FontTexturePage);
		if( !Texture )
		{
			debugf(TEXT("Invalid font texture. Using default texture"));
			Texture = Texture = GWorld->GetWorldInfo()->DefaultTexture;
		}
		check(Texture);
		INT TextureCodeIndex = Compiler->TextureParameter(ParameterName,Texture);
		INT ArgA = Compiler->TextureSample(
			TextureCodeIndex,
			Compiler->TextureCoordinate(0, FALSE, FALSE) 
			);
		INT ArgB = Compiler->Constant4(
			Texture->UnpackMax[0] - Texture->UnpackMin[0],
			Texture->UnpackMax[1] - Texture->UnpackMin[1],
			Texture->UnpackMax[2] - Texture->UnpackMin[2],
			Texture->UnpackMax[3] - Texture->UnpackMin[3]
		);
		INT ArgC =  Compiler->Constant4(
			Texture->UnpackMin[0],
			Texture->UnpackMin[1],
			Texture->UnpackMin[2],
			Texture->UnpackMin[3]
		);
		Result = Compiler->Add(Compiler->Mul(ArgA, ArgB), ArgC);
	}
	return Result;
}

/**
* Caption description for this expression
* @return string caption
*/
FString UMaterialExpressionFontSampleParameter::GetCaption() const
{
	return FString::Printf( TEXT("FontParam '%s'"), *ParameterName.ToString() ); 
}

/**
*	Sets the default Font if none is set
*/
void UMaterialExpressionFontSampleParameter::SetDefaultFont()
{
	GEngine->GetMediumFont();
}

/** 
* Generates a GUID for this expression if one doesn't already exist. 
*
* @param bForceGeneration	Whether we should generate a GUID even if it is already valid.
*/
void UMaterialExpressionFontSampleParameter::ConditionallyGenerateGUID(UBOOL bForceGeneration)
{
	// If we are in the editor, and we don't have a valid GUID yet, generate one.
	if(GIsEditor && !GIsGame)
	{
		if(bForceGeneration || ExpressionGUID.IsValid()==FALSE)
		{
			ExpressionGUID = appCreateGuid();
			MarkPackageDirty();
		}
	}
}

/** Tries to generate a GUID. */
void UMaterialExpressionFontSampleParameter::PostLoad()
{
	Super::PostLoad();

	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionFontSampleParameter::PostDuplicate()
{
	Super::PostDuplicate();

	// We do not force a guid regen here because this function is used when the Material Editor makes a copy of a material to edit.
	// If we forced a GUID regen, it would cause all of the guids for a material to change everytime a material was edited.
	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionFontSampleParameter::PostEditImport()
{
	Super::PostEditImport();

	ConditionallyGenerateGUID(TRUE);
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionFontSampleParameter::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( ParameterName.ToString().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionWorldPosition
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for accessing world position
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionWorldPosition::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->WorldPosition();
}

/**
* Text description of this expression
*/
FString UMaterialExpressionWorldPosition::GetCaption() const
{
	return TEXT("World Position");
}

void UMaterialExpressionWorldPosition::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionObjectWorldPosition
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for accessing object world position
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionObjectWorldPosition::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ObjectWorldPosition();
}

/**
* Text description of this expression
*/
FString UMaterialExpressionObjectWorldPosition::GetCaption() const
{
	return TEXT("Object World Position");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionObjectRadius
///////////////////////////////////////////////////////////////////////////////

INT UMaterialExpressionObjectRadius::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ObjectRadius();
}

/**
* Text description of this expression
*/
FString UMaterialExpressionObjectRadius::GetCaption() const
{
	return TEXT("Object Radius");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionFluidNormal
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for accessing fluid normals.
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionFluidNormal::Compile(FMaterialCompiler* Compiler)
{
	INT TextureCoordArg = Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE);
	INT DerivedFluidNormal = Compiler->FluidDetailNormal(TextureCoordArg);
	return DerivedFluidNormal;
}

/**
* Text description of this expression
*/
FString UMaterialExpressionFluidNormal::GetCaption() const
{
	return TEXT("Fluid Normal");
}

void UMaterialExpressionFluidNormal::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinates, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionDeriveNormalZ
///////////////////////////////////////////////////////////////////////////////

/**
* Create the shader code chunk for deriving Z for a normal whose X and Y are known, 
* based on the assumption that the normal is unit length, and Z will be positive.
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionDeriveNormalZ::Compile(FMaterialCompiler* Compiler)
{
	if(!InXY.Expression)
	{
		return Compiler->Errorf(TEXT("Missing input normal xy vector whose z should be derived."));
	}

	//z = sqrt(1 - ( x * x + y * y));
	INT InputVector = Compiler->ForceCast(InXY.Compile(Compiler), MCT_Float2);
	INT DotResult = Compiler->Dot(InputVector, InputVector);
	INT InnerResult = Compiler->Sub(Compiler->Constant(1), DotResult);
	INT DerivedZ = Compiler->SquareRoot(InnerResult);
	INT AppendedResult = Compiler->ForceCast(Compiler->AppendVector(InputVector, DerivedZ), MCT_Float3);

	return AppendedResult;
}

/**
* Text description of this expression
*/
FString UMaterialExpressionDeriveNormalZ::GetCaption() const
{
	return TEXT("DeriveNormalZ");
}

void UMaterialExpressionDeriveNormalZ::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionConstantBiasScale
///////////////////////////////////////////////////////////////////////////////

/**
* Biases, then scales an input expression
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/	
INT UMaterialExpressionConstantBiasScale::Compile(FMaterialCompiler* Compiler)
{
	if (!Input.Expression)
	{
		return Compiler->Errorf(TEXT("Missing ConstantBiasScale input"));
	}

	return Compiler->Mul(Compiler->Add(Compiler->Constant(Bias), Input.Compile(Compiler)), Compiler->Constant(Scale));
}

/**
* Text description of this expression
*/
FString UMaterialExpressionConstantBiasScale::GetCaption() const
{
	return TEXT("ConstantBiasScale");
}

void UMaterialExpressionConstantBiasScale::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Input, OldExpression, NewExpression );
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionCustom
///////////////////////////////////////////////////////////////////////////////

/**
* Custom expression running the HLSL code provided.
*
* @param	Compiler - Material compiler that knows how to handle this expression
* @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
*/
INT UMaterialExpressionCustom::Compile(FMaterialCompiler* Compiler)
{
	TArray<INT> CompiledInputs;

	for( INT i=0;i<Inputs.Num();i++ )
	{
		// skip over unnamed inputs
		if( Inputs(i).InputName.Len()==0 )
		{
			CompiledInputs.AddItem(INDEX_NONE);
		}
		else
		{
			if(!Inputs(i).Input.Expression)
			{
				return Compiler->Errorf(TEXT("Custom material %s missing input %d (%s)"), *Description, i+1, *Inputs(i).InputName);
			}
			INT InputCode = Inputs(i).Input.Compile(Compiler);
			if( InputCode < 0 )
			{
				return InputCode;
			}
			CompiledInputs.AddItem( InputCode );
		}
	}

	return Compiler->CustomExpression(this, CompiledInputs);
}

/**
* Text description of this expression, specified by the user.
*/
FString UMaterialExpressionCustom::GetCaption() const
{
	return Description;
}

void UMaterialExpressionCustom::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	for( INT i = 0; i < Inputs.Num(); i++ )
	{
		SWAP_REFERENCE_TO( Inputs(i).Input, OldExpression, NewExpression );
	}
}

//
//	UMaterialExpressionCustom::GetInputs
//
const TArray<FExpressionInput*> UMaterialExpressionCustom::GetInputs()
{
	TArray<FExpressionInput*> Result;
	for( INT i = 0; i < Inputs.Num(); i++ )
	{
		Result.AddItem(&Inputs(i).Input);
	}
	return Result;
}

//
//	UMaterialExpressionCustom::GetInput
//

FExpressionInput* UMaterialExpressionCustom::GetInput(INT InputIndex)
{
	if( InputIndex < Inputs.Num() )
	{
		return &Inputs(InputIndex).Input;
	}
	return NULL;
}

//
//	UMaterialExpressionCustom::GetInputName
//

FString UMaterialExpressionCustom::GetInputName(INT InputIndex) const
{
	if( InputIndex < Inputs.Num() )
	{
		return Inputs(InputIndex).InputName;
	}
	return TEXT("");
}

//
//	UMaterialExpressionCustom::PostEditChange
//

void UMaterialExpressionCustom::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// strip any spaces from input name
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if( PropertyThatChanged && PropertyThatChanged->GetFName() == FName(TEXT("InputName")) )
	{
		for( INT i=0;i<Inputs.Num();i++ )
		{
			Inputs(i).InputName.ReplaceInline(TEXT(" "),TEXT(""));
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionCustomTexture
///////////////////////////////////////////////////////////////////////////////

//
//	UMaterialExpressionCustomTexture::Compile
//
INT UMaterialExpressionCustomTexture::Compile(FMaterialCompiler* Compiler)
{
#if EXCEPTIONS_DISABLED
	// if we can't throw the error below, attempt to thwart the error by using the default texture
	// @todo: handle missing cubemaps and 3d textures?
	if (!Texture)
	{
		debugf(TEXT("Using default texture instead of real texture!"));
		Texture = GWorld->GetWorldInfo()->DefaultTexture;
	}
#endif

	if (Texture)
	{
		return  Compiler->Texture(Texture);
	}
	else
	{
		if (Desc.Len() > 0)
		{
			return Compiler->Errorf(TEXT("%s> Missing input texture"), *Desc);
		}
		else
		{
			return Compiler->Errorf(TEXT("CustomTexture> Missing input texture"));
		}
	}
}

//
//	UMaterialExpressionCustomTexture::CompilePreview
//
INT UMaterialExpressionCustomTexture::CompilePreview(FMaterialCompiler* Compiler)
{
#if EXCEPTIONS_DISABLED
	// if we can't throw the error below, attempt to thwart the error by using the default texture
	// @todo: handle missing cubemaps and 3d textures?
	if (!Texture)
	{
		debugf(TEXT("Using default texture instead of real texture!"));
		Texture = GWorld->GetWorldInfo()->DefaultTexture;
	}
#endif

	if (Texture)
	{
		// Preview just returns the sampled texture rather than the texture sampler.
		INT TextureCodeIndex = Compiler->Texture(Texture);
		INT Sample = Compiler->TextureSample(TextureCodeIndex, Compiler->TextureCoordinate(0, FALSE, FALSE));
		return UnpackTexture(Compiler, Sample, Texture, Texture->CompressionSettings);
	}
	else
	{
		if (Desc.Len() > 0)
		{
			return Compiler->Errorf(TEXT("%s> Missing input texture"), *Desc);
		}
		else
		{
			return Compiler->Errorf(TEXT("CustomTexture> Missing input texture"));
		}
	}
}

//
//	UMaterialExpressionCustomTexture::GetOutputs
//

void UMaterialExpressionCustomTexture::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(0);
}

INT UMaterialExpressionCustomTexture::GetWidth() const
{
	return ME_STD_THUMBNAIL_SZ+(ME_STD_BORDER*2);
}

//
//	UMaterialExpressionCustomTexture::GetCaption
//
FString UMaterialExpressionCustomTexture::GetCaption() const
{
	return TEXT("Custom Texture");
}

/**
* MatchesSearchQuery: Check this expression to see if it matches the search query
* @param SearchQuery - User's search query (never blank)
* @return TRUE if the expression matches the search query
*/
UBOOL UMaterialExpressionCustomTexture::MatchesSearchQuery( const TCHAR* SearchQuery )
{
	if( Texture!=NULL && Texture->GetName().InStr(SearchQuery, FALSE, TRUE) != -1 )
	{
		return TRUE;
	}

	return Super::MatchesSearchQuery(SearchQuery);
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionTextureSampleParameterNormal
///////////////////////////////////////////////////////////////////////////////

//
//  UMaterialExpressionTextureSampleParameterNormal
//
INT UMaterialExpressionTextureSampleParameterNormal::Compile(FMaterialCompiler* Compiler)
{
	if (Texture == NULL)
	{
		return CompilerError(Compiler, GetRequirements());
	}

	if (Texture)
	{
		if (!TextureIsValid(Texture))
		{
			return CompilerError(Compiler, GetRequirements());
		}
	}

	if (!ParameterName.IsValid() || (ParameterName.GetIndex() == NAME_None))
	{
		return UMaterialExpressionTextureSample::Compile(Compiler);
	}

	INT TextureCodeIndex = Compiler->TextureParameter(ParameterName, Texture);


	INT Tex = Compiler->TextureSample(
		TextureCodeIndex,
		Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE)
		);

	// Use the compression settings either from the Instance, or from the default texture.
	BYTE CompressionSettings = InstanceOverride ? InstanceOverride->CompressionSettings : Texture->CompressionSettings;


	return UnpackTexture(Compiler, Tex, Texture, CompressionSettings);
}

//
//  UMaterialExpressionTextureSampleParameterNormal
//
FString UMaterialExpressionTextureSampleParameterNormal::GetCaption() const
{
	return FString::Printf( TEXT("NormalParam '%s'"), *ParameterName.ToString() ); 
}

UBOOL UMaterialExpressionTextureSampleParameterNormal::TextureIsValid( UTexture* InTexture )
{
	UBOOL Result=FALSE;
	if (InTexture)		
	{
		if( InTexture->GetClass() == UTexture2D::StaticClass() ) 
		{
			Result = TRUE;
		}
	}
	return Result;
}

const TCHAR* UMaterialExpressionTextureSampleParameterNormal::GetRequirements()
{
	return TEXT("Requires Normal-format Texture2D");
}

/**
*	Sets the default texture if none is set
*/
void UMaterialExpressionTextureSampleParameterNormal::SetDefaultTexture()
{
	Texture = LoadObject<UTexture2D>(NULL, TEXT("EngineMaterials.DefaultNormal"), NULL, LOAD_None, NULL);
}

//
//	UMaterialExpressionLightmapUVs
//
/**
 * Creates the new shader code chunk needed for the Abs expression
 *
 * @param	Compiler - Material compiler that knows how to handle this expression
 * @return	Index to the new FMaterialCompiler::CodeChunk entry for this expression
 */	
INT UMaterialExpressionLightmapUVs::Compile( FMaterialCompiler* Compiler )
{
	return Compiler->LightmapUVs();
}

/**
 *	Get the outputs supported by this expression.
 *
 *	@param	Outputs		The TArray of outputs to fill in.
 */
void UMaterialExpressionLightmapUVs::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	// Lightmap texture coordinates...
	new(Outputs) FExpressionOutput(1,1,1,0,0);
}

/**
 * Textual description for this material expression
 *
 * @return	Caption text
 */	
FString UMaterialExpressionLightmapUVs::GetCaption() const
{
	return TEXT("LightmapUVs");
}


//
//	UMaterialExpressionLightmassReplace
//

INT UMaterialExpressionLightmassReplace::Compile(FMaterialCompiler* Compiler)
{
	if (!Realtime.Expression)
	{
		return Compiler->Errorf(TEXT("Missing LightmassReplace input Realtime"));
	}
	else if (!Lightmass.Expression)
	{
		return Compiler->Errorf(TEXT("Missing LightmassReplace input Lightmass"));
	}
	else
	{
		INT Arg1 = Realtime.Compile(Compiler);
		INT Arg2 = Lightmass.Compile(Compiler);
		return Compiler->LightmassReplace(Arg1, Arg2);
	}
}

FString UMaterialExpressionLightmassReplace::GetCaption() const
{
	return TEXT("LightmassReplace");
}

void UMaterialExpressionLightmassReplace::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Realtime, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Lightmass, OldExpression, NewExpression );
}

INT UMaterialExpressionObjectOrientation::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->ObjectOrientation();
}

FString UMaterialExpressionObjectOrientation::GetCaption() const
{
	return TEXT("ObjectOrientation");
}

INT UMaterialExpressionWindDirectionAndSpeed::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->WindDirectionAndSpeed();
}

FString UMaterialExpressionWindDirectionAndSpeed::GetCaption() const
{
	return TEXT("WindDirectionAndSpeed");
}


INT UMaterialExpressionFoliageImpulseDirection::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->FoliageImpulseDirection();
}

FString UMaterialExpressionFoliageImpulseDirection::GetCaption() const
{
	return TEXT("FoliageImpulseDirection");
}

INT UMaterialExpressionFoliageNormalizedRotationAxisAndAngle::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->FoliageNormalizedRotationAxisAndAngle();
}

FString UMaterialExpressionFoliageNormalizedRotationAxisAndAngle::GetCaption() const
{
	return TEXT("FoliageNormalizedRotationAxisAndAngle");
}

INT UMaterialExpressionRotateAboutAxis::Compile(FMaterialCompiler* Compiler)
{
	if (!NormalizedRotationAxisAndAngle.Expression)
	{
		return Compiler->Errorf(TEXT("Missing RotateAboutAxis input NormalizedRotationAxisAndAngle"));
	}
	else if (!PositionOnAxis.Expression)
	{
		return Compiler->Errorf(TEXT("Missing RotateAboutAxis input PositionOnAxis"));
	}
	else if (!Position.Expression)
	{
		return Compiler->Errorf(TEXT("Missing RotateAboutAxis input Position"));
	}
	else
	{
		return Compiler->RotateAboutAxis(NormalizedRotationAxisAndAngle.Compile(Compiler), PositionOnAxis.Compile(Compiler), Position.Compile(Compiler));
	}
}

FString UMaterialExpressionRotateAboutAxis::GetCaption() const
{
	return TEXT("RotateAboutAxis");
}

void UMaterialExpressionRotateAboutAxis::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( NormalizedRotationAxisAndAngle, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( PositionOnAxis, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Position, OldExpression, NewExpression );
}

///////////////////////////////////////////////////////////////////////////////
// Static functions so it can be used other material expressions.
///////////////////////////////////////////////////////////////////////////////

/** Does not use length() to allow optimizations. */
static INT CompileHelperLength( FMaterialCompiler* Compiler, INT A, INT B )
{
	INT Delta = Compiler->Sub(A, B);
	INT Dist2 = Compiler->Dot(Delta, Delta);
	return Compiler->SquareRoot(Dist2);
}

/** Used Clamp(), which will be optimized away later to a saturate(). */
static INT CompileHelperSaturate( FMaterialCompiler* Compiler, INT A )
{
	return Compiler->Clamp(A, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionSphereMask
///////////////////////////////////////////////////////////////////////////////

void UMaterialExpressionSphereMask::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

INT UMaterialExpressionSphereMask::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		INT Distance = CompileHelperLength(Compiler, Arg1, Arg2);
		INT ArgInvRadius = Compiler->Constant(1.0f / Max(0.00001f, AttenuationRadius));
		INT NormalizeDistance = Compiler->Mul(Distance, ArgInvRadius);

		// Hardness is in percent 0=soft .. 100=hard
		// Max to avoid div by 0
		float InvHardness = 1.0f / Max(HardnessPercent*0.01f, 0.00001f);

		INT NegNormalizedDistance = Compiler->Sub(Compiler->Constant(1.0f), NormalizeDistance);
		INT MaskUnclamped = Compiler->Mul(NegNormalizedDistance, Compiler->Constant(InvHardness));

		return CompileHelperSaturate(Compiler, MaskUnclamped);
	}
}

FString UMaterialExpressionSphereMask::GetCaption() const
{
	return TEXT("SphereMask");
}


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionDistance
///////////////////////////////////////////////////////////////////////////////

void UMaterialExpressionDistance::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( A, OldExpression, NewExpression );
	SWAP_REFERENCE_TO( B, OldExpression, NewExpression );
}

INT UMaterialExpressionDistance::Compile(FMaterialCompiler* Compiler)
{
	if(!A.Expression)
	{
		return Compiler->Errorf(TEXT("Missing input A"));
	}
	else if(!B.Expression)
	{
		return Compiler->Errorf(TEXT("Missing input B"));
	}
	else
	{
		INT Arg1 = A.Compile(Compiler);
		INT Arg2 = B.Compile(Compiler);
		return CompileHelperLength(Compiler, Arg1, Arg2);
	}
}

FString UMaterialExpressionDistance::GetCaption() const
{
	return TEXT("Distance");
}

INT UMaterialExpressionTwoSidedSign::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->TwoSidedSign();
}

FString UMaterialExpressionTwoSidedSign::GetCaption() const
{
	return TEXT("TwoSidedSign");
}

INT UMaterialExpressionWorldNormal::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->WorldNormal();
}

FString UMaterialExpressionWorldNormal::GetCaption() const
{
	return TEXT("WorldNormal");
}



///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionPerInstanceRandom
///////////////////////////////////////////////////////////////////////////////

//
//	UMaterialExpressionPerInstanceRandom::Compile
//

INT UMaterialExpressionPerInstanceRandom::Compile(FMaterialCompiler* Compiler)
{
	return Compiler->PerInstanceRandom();
}

//
//	UMaterialExpressionPerInstanceRandom::GetCaption
//

FString UMaterialExpressionPerInstanceRandom::GetCaption() const
{
	return TEXT("PerInstanceRandom");
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionAntialiasedTextureMask
///////////////////////////////////////////////////////////////////////////////

void UMaterialExpressionAntialiasedTextureMask::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Coordinates, OldExpression, NewExpression );
}

INT UMaterialExpressionAntialiasedTextureMask::Compile(FMaterialCompiler* Compiler)
{
	if(!Texture)
	{
		return Compiler->Errorf(TEXT("UMaterialExpressionAntialiasedTextureMask> Missing input texture"));
	}

	INT ArgCoord = Coordinates.Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(0, FALSE, FALSE);

	if (!TextureIsValid(Texture))
	{
		return CompilerError(Compiler, GetRequirements());
	}

	INT TextureCodeIndex;

	if (!ParameterName.IsValid() || (ParameterName.GetIndex() == NAME_None))
	{
		TextureCodeIndex = Compiler->Texture(Texture);
	}
	else
	{
		TextureCodeIndex = Compiler->TextureParameter(ParameterName, Texture);
	}

	return Compiler->AntialiasedTextureMask(TextureCodeIndex,ArgCoord,Threshold,Channel);
}

void UMaterialExpressionAntialiasedTextureMask::GetOutputs(TArray<FExpressionOutput>& Outputs) const
{
	new(Outputs) FExpressionOutput(0);
}


FString UMaterialExpressionAntialiasedTextureMask::GetCaption() const
{
	return FString::Printf( TEXT("AAMasked Param2D '%s'"), *ParameterName.ToString() ); 
}

UBOOL UMaterialExpressionAntialiasedTextureMask::TextureIsValid( UTexture* InTexture )
{
	UBOOL Result=FALSE;
	if (InTexture)		
	{
		if( InTexture->GetClass() == UTexture2D::StaticClass() ) 
		{
			Result = TRUE;
		}
		if( InTexture->IsA(UTextureRenderTarget2D::StaticClass()) )	
		{
			Result = TRUE;
		}
	}
	return Result;
}

const TCHAR* UMaterialExpressionAntialiasedTextureMask::GetRequirements()
{
	return TEXT("Requires Texture2D");
}

void UMaterialExpressionAntialiasedTextureMask::SetDefaultTexture()
{
	Texture = LoadObject<UTexture2D>(NULL, TEXT("EngineResources.DefaultTexture"), NULL, LOAD_None, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionTerrainLayerWeight
///////////////////////////////////////////////////////////////////////////////

INT UMaterialExpressionTerrainLayerWeight::Compile(FMaterialCompiler* Compiler)
{
	INT BaseCode = Base.Expression ? Base.Compile(Compiler) : Compiler->Constant3(0,0,0);

	//if the override is set, use the values from it
	if (InstanceOverride)
	{
		if (InstanceOverride->WeightmapIndex == INDEX_NONE)
		{
			return BaseCode;
		}
		else
		{
			INT LayerCode = Layer.Compile(Compiler);

			FString WeightmapName = FString::Printf(TEXT("Weightmap%d"),InstanceOverride->WeightmapIndex);
			INT TextureCodeIndex = Compiler->TextureParameter(FName(*WeightmapName),  GEngine->WeightMapPlaceholderTexture);
			INT WeightmapCode = Compiler->TextureSample(TextureCodeIndex, Compiler->TextureCoordinate(1, FALSE, FALSE));
			FString LayerMaskName = FString::Printf(TEXT("LayerMask_%s"),*InstanceOverride->ParameterName.ToString());
			INT WeightCode = Compiler->Dot(WeightmapCode,Compiler->VectorParameter(FName(*LayerMaskName), FLinearColor(1.f,0.f,0.f,0.f)));
			return Compiler->Add(BaseCode, Compiler->Mul(LayerCode, WeightCode));
		}
	}
	else
	{
		return Compiler->Add(BaseCode, Compiler->Mul(Layer.Compile(Compiler), Compiler->Constant(PreviewWeight)));
	}
}

FString UMaterialExpressionTerrainLayerWeight::GetCaption() const
{
	return FString::Printf(TEXT("Layer '%s'"), *ParameterName.ToString());
}

/** 
* Generates a GUID for this expression if one doesn't already exist. 
*
* @param bForceGeneration	Whether we should generate a GUID even if it is already valid.
*/
void UMaterialExpressionTerrainLayerWeight::ConditionallyGenerateGUID(UBOOL bForceGeneration)
{
	// If we are in the editor, and we don't have a valid GUID yet, generate one.
	if(GIsEditor && !GIsGame)
	{
		if(bForceGeneration || ExpressionGUID.IsValid()==FALSE)
		{
			ExpressionGUID = appCreateGuid();
			MarkPackageDirty();
		}
	}
}

/** Tries to generate a GUID. */
void UMaterialExpressionTerrainLayerWeight::PostLoad()
{
	Super::PostLoad();

	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionTerrainLayerWeight::PostDuplicate()
{
	Super::PostDuplicate();

	// We do not force a guid regen here because this function is used when the Material Editor makes a copy of a material to edit.
	// If we forced a GUID regen, it would cause all of the guids for a material to change everytime a material was edited.
	ConditionallyGenerateGUID(FALSE);
}

/** Tries to generate a GUID. */
void UMaterialExpressionTerrainLayerWeight::PostEditImport()
{
	Super::PostEditImport();

	ConditionallyGenerateGUID(TRUE);
}

#if WITH_EDITOR
/**
 *	Called by the CleanupMaterials function, this will clear the inputs of the expression.
 *	This only needs to be implemented by expressions that have bUsedByStaticParameterSet set to TRUE.
 */
void UMaterialExpressionTerrainLayerWeight::ClearInputExpressions()
{
	Base.Expression = NULL;
	Layer.Expression = NULL;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionTerrainLayerCoords
///////////////////////////////////////////////////////////////////////////////

INT UMaterialExpressionTerrainLayerCoords::Compile(FMaterialCompiler* Compiler)
{
	INT	BaseUV;
	switch(MappingType)
	{
	case TCMT_Auto:
	case TCMT_XY: BaseUV = Compiler->TextureCoordinate(3, FALSE, FALSE); break;
	case TCMT_XZ: BaseUV = Compiler->TextureCoordinate(4, FALSE, FALSE); break;
	case TCMT_YZ: BaseUV = Compiler->TextureCoordinate(5, FALSE, FALSE); break;
	default: appErrorf(TEXT("Invalid mapping type %u"),MappingType); return INDEX_NONE;
	};

	FLOAT	Scale = MappingScale == 0.0f ? 1.0f : MappingScale;

	return Compiler->Add(
		Compiler->AppendVector(
		Compiler->Dot(BaseUV,Compiler->Constant2(+appCos(MappingRotation * PI / 180.0) / Scale,+appSin(MappingRotation * PI / 180.0) / Scale)),
		Compiler->Dot(BaseUV,Compiler->Constant2(-appSin(MappingRotation * PI / 180.0) / Scale,+appCos(MappingRotation * PI / 180.0) / Scale))
		),
		Compiler->Constant2(MappingPanU,MappingPanV)
		);
}

FString UMaterialExpressionTerrainLayerCoords::GetCaption() const
{
	//return FString::Printf(TEXT("Layer '%s' Cooords"), *LayerName.ToString());
	return FString(TEXT("TerrainCoords"));
}

///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionDepthOfFieldFunction
///////////////////////////////////////////////////////////////////////////////

/**
 *	Compile this expression with the given compiler.
 *	
 *	@return	INT			The code index for this expression.
 */
INT UMaterialExpressionDepthOfFieldFunction::Compile(FMaterialCompiler* Compiler)
{
	INT DepthInput;

	if(Depth.Expression)
	{
		// using the input allows more custom behavior
		DepthInput = Depth.Compile(Compiler);
	}
	else
	{
		// no input means we use the PixelDepth
		DepthInput = Compiler->PixelDepth(FALSE);
	}

	if(DepthInput == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return Compiler->DepthOfFieldFunction(DepthInput, FunctionValue);
}

/**
 *	Returns the text to display on the material expression (in the material editor).
 *
 *	@return	FString		The text to display.
 */
FString UMaterialExpressionDepthOfFieldFunction::GetCaption() const
{
	return FString(TEXT("DepthOfFieldFunction"));
}

/**
 * Replaces references to the passed in expression with references to a different expression or NULL.
 * @param	OldExpression		Expression to find reference to.
 * @param	NewExpression		Expression to replace reference with.
 */
void UMaterialExpressionDepthOfFieldFunction::SwapReferenceTo( UMaterialExpression* OldExpression, UMaterialExpression* NewExpression )
{
	Super::SwapReferenceTo( OldExpression, NewExpression );
	SWAP_REFERENCE_TO( Depth, OldExpression, NewExpression );
}