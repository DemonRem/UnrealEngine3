/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineMaterialClasses.h"
#include "GenericBrowser.h"
#include "MaterialEditorBase.h"
#include "NewMaterialEditor.h"
#include "MaterialEditorContextMenus.h"
#include "MaterialEditorPreviewScene.h"
#include "MaterialEditorToolBar.h"
#include "UnLinkedObjDrawUtils.h"
#include "BusyCursor.h"
#include "ScopedTransaction.h"
#include "PropertyWindow.h"
#include "PropertyUtils.h"

IMPLEMENT_CLASS( UMaterialEditorOptions );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static helpers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

/** Location of the realtime preview icon, relative to the upper-hand corner of the material expression node. */
static const FIntPoint PreviewIconLoc( 2, 6 );

/** Width of the realtime preview icon. */
static const INT PreviewIconWidth = 10;

static const FColor ConnectionNormalColor(0,0,0);
static const FColor ConnectionSelectedColor(255,128,0);

} // namespace

/**
 * An input to a compound material expression.
 */
class FCompoundInput
{
public:
	UMaterialExpression*	Expression;
	INT						InputIndex;
};

/**
 * An output from a compound material expression.
 */
class FCompoundOutput
{
public:
	UMaterialExpression*	Expression;
	FExpressionOutput		ExpressionOutput;
	FCompoundOutput():
		ExpressionOutput(FALSE)
	{}
};

/**
 * Information 
 */
class FCompoundInfo
{
public:
	/** Inputs to the compound expression. */
	TArray<FCompoundInput> CompoundInputs;
	/** Outputs to the compound expression. */
	TArray<FCompoundOutput> CompoundOutputs;
};

/**
 * Class for rendering previews of material expressions in the material editor's linked object viewport.
 */
class FExpressionPreview : public FMaterial, public FMaterialRenderProxy
{
public:
	FExpressionPreview() :
		// Compile all the preview shaders together
		// They compile much faster together than one at a time since the shader compiler will use multiple cores
		bDeferCompilation(TRUE)
	{
		// Register this FMaterial derivative with AddEditorLoadedMaterialResource since it does not have a corresponding UMaterialInterface
		FMaterial::AddEditorLoadedMaterialResource(this);
	}

	FExpressionPreview(UMaterialExpression* InExpression) :	
		bDeferCompilation(TRUE),
		Expression(InExpression)
	{
		FMaterial::AddEditorLoadedMaterialResource(this);
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return TRUE if the shader should be compiled
	 */
	virtual UBOOL ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
	{
		if(VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLocalVertexFactory"), FNAME_Find)))
		{
			// we only need the non-light-mapped, base pass, local vertex factory shaders for drawing an opaque Material Tile
			// @todo: Added a FindShaderType by fname or something"

			if(appStristr(ShaderType->GetName(), TEXT("BasePassVertexShaderFNoLightMapPolicyFNoDensityPolicy")) ||
				appStristr(ShaderType->GetName(), TEXT("BasePassHullShaderFNoLightMapPolicyFNoDensityPolicy")) ||
				appStristr(ShaderType->GetName(), TEXT("BasePassDomainShaderFNoLightMapPolicyFNoDensityPolicy")))
			{
				return TRUE;
			}
			else if(appStristr(ShaderType->GetName(), TEXT("BasePassPixelShaderFNoLightMapPolicy")))
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	/** Whether shaders should be compiled right away in CompileShaderMap or deferred until later. */
	virtual UBOOL DeferFinishCompiling() const { return bDeferCompilation; }

	////////////////
	// FMaterialRenderProxy interface.

	virtual const FMaterial* GetMaterial() const
	{
		if(GetShaderMap())
		{
			return this;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(FALSE)->GetMaterial();
		}
	}

	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		return Expression->GetOuterUMaterial()->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		return Expression->GetOuterUMaterial()->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		return Expression->GetOuterUMaterial()->GetRenderProxy(0)->GetTextureValue(ParameterName,OutValue,Context);
	}
	
	// Material properties.
	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	virtual INT CompileProperty(EMaterialShaderPlatform MatPlatform,EMaterialProperty Property,FMaterialCompiler* Compiler) const
	{
		Compiler->SetMaterialProperty(Property);
		if( Property == MP_EmissiveColor )
		{
			// Get back into gamma corrected space, as DrawTile does not do this adjustment.
			return Compiler->Power(Expression->CompilePreview(Compiler), Compiler->Constant(1.f/2.2f));
		}
		else if ( Property == MP_WorldPositionOffset)
		{
			//set to 0 to prevent off by 1 pixel errors
			return Compiler->Constant(0.0f);
		}
		else
		{
			return Compiler->Constant(1.0f);
		}
	}

	virtual FString GetMaterialUsageDescription() const { return FString::Printf(TEXT("FExpressionPreview %s"), Expression ? *Expression->GetName() : TEXT("NULL")); }
	virtual UBOOL IsTwoSided() const { return FALSE; }
	virtual UBOOL RenderTwoSidedSeparatePass() const { return FALSE; }
	virtual UBOOL RenderLitTranslucencyPrepass() const { return FALSE; }
	virtual UBOOL RenderLitTranslucencyDepthPostpass() const { return FALSE; }
	virtual UBOOL NeedsDepthTestDisabled() const { return FALSE; }
	virtual UBOOL IsLightFunction() const { return FALSE; }
	virtual UBOOL IsUsedWithFogVolumes() const { return FALSE; }
	virtual UBOOL IsSpecialEngineMaterial() const { return FALSE; }
	virtual UBOOL IsTerrainMaterial() const { return FALSE; }
	virtual UBOOL IsDecalMaterial() const { return FALSE; }
	virtual UBOOL IsWireframe() const { return FALSE; }
	virtual UBOOL IsDistorted() const { return FALSE; }
	virtual UBOOL HasSubsurfaceScattering() const { return FALSE; }
	virtual UBOOL HasSeparateTranslucency() const { return FALSE; }
	virtual UBOOL IsMasked() const { return FALSE; }
	virtual UBOOL CastLitTranslucencyShadowAsMasked() const { return FALSE; }
	virtual enum EBlendMode GetBlendMode() const { return BLEND_Opaque; }
	virtual enum EMaterialLightingModel GetLightingModel() const { return MLM_Unlit; }
	virtual FLOAT GetOpacityMaskClipValue() const { return 0.5f; }
	virtual FString GetFriendlyName() const { return FString::Printf(TEXT("FExpressionPreview %s"), Expression ? *Expression->GetName() : TEXT("NULL")); }
	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual UBOOL IsPersistent() const { return FALSE; }

	const UMaterialExpression* GetExpression() const
	{
		return Expression;
	}

	friend FArchive& operator<< ( FArchive& Ar, FExpressionPreview& V )
	{
		return Ar << V.Expression;
	}

	UBOOL bDeferCompilation;

private:
	UMaterialExpression* Expression;
};

namespace {

/**
 * Class for rendering the material on the preview mesh in the Material Editor
 */
class FPreviewMaterial : public FMaterialResource, public FMaterialRenderProxy
{
public:
	FPreviewMaterial(UMaterial* InMaterial)
	:	FMaterialResource(InMaterial)
	{
		CacheShaders();
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return TRUE if the shader should be compiled
	 */
	virtual UBOOL ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
	{
		// only generate the needed shaders (which should be very restrictive)
		// @todo: Add a FindShaderType by fname or something

		// Always allow HitProxy shaders.
		if (appStristr(ShaderType->GetName(), TEXT("HitProxy")))
		{
			return TRUE;
		}

		// we only need local vertex factory for the preview static mesh
		if (VertexFactoryType != FindVertexFactoryType(FName(TEXT("FLocalVertexFactory"), FNAME_Find)))
		{
			//cache for gpu skinned vertex factory if the material allows it
			//this way we can have a preview skeletal mesh
			if (!IsUsedWithSkeletalMesh() || VertexFactoryType != FindVertexFactoryType(FName(TEXT("FGPUSkinVertexFactory"), FNAME_Find)))
			{
				return FALSE;
			}
		}

		// look for any of the needed type
		UBOOL bShaderTypeMatches = FALSE;
		if (appStristr(ShaderType->GetName(), TEXT("BasePassPixelShaderFNoLightMapPolicy")))
		{
			bShaderTypeMatches = TRUE;
		}
		//used for instruction count on lit materials
		else if (appStristr(ShaderType->GetName(), TEXT("TBasePassPixelShaderFDirectionalLightMapTexturePolicyNoSkyLight")))
		{
			bShaderTypeMatches = TRUE;
		}
		//used for instruction count on lit particle materials
		else if (appStristr(ShaderType->GetName(), TEXT("TBasePassPixelShaderFDirectionalLightLightMapPolicySkyLight")))
		{
			bShaderTypeMatches = TRUE;
		}
		//used for instruction count on lit materials
		else if (appStristr(ShaderType->GetName(), TEXT("TLightPixelShaderFPointLightPolicyFNoStaticShadowingPolicy")))
		{
			bShaderTypeMatches = TRUE;
		}
		// Pick tessellation shader based on material settings
		else if(appStristr(ShaderType->GetName(), TEXT("BasePassVertexShaderFNoLightMapPolicyFNoDensityPolicy")) ||
			appStristr(ShaderType->GetName(), TEXT("BasePassHullShaderFNoLightMapPolicyFNoDensityPolicy")) ||
			appStristr(ShaderType->GetName(), TEXT("BasePassDomainShaderFNoLightMapPolicyFNoDensityPolicy")))
		{
			bShaderTypeMatches = TRUE;
		}
		else if (appStristr(ShaderType->GetName(), TEXT("TDistortion")))
		{
			bShaderTypeMatches = TRUE;
		}
		else if (appStristr(ShaderType->GetName(), TEXT("TSubsurface")))
		{
			bShaderTypeMatches = TRUE;
		}
		else if (appStristr(ShaderType->GetName(), TEXT("TLight")))
		{
			if (appStristr(ShaderType->GetName(), TEXT("FDirectionalLightPolicyFShadowTexturePolicy")) ||
				appStristr(ShaderType->GetName(), TEXT("FDirectionalLightPolicyFNoStaticShadowingPolicy")))
			{
				bShaderTypeMatches = TRUE;
			}
		}
		else if (IsUsedWithFogVolumes() 
			&& (appStristr(ShaderType->GetName(), TEXT("FFogVolumeApply"))
			|| appStristr(ShaderType->GetName(), TEXT("TFogIntegral")) && appStristr(ShaderType->GetName(), TEXT("FConstantDensityPolicy"))))
		{
			bShaderTypeMatches = TRUE;
		}
		else if (appStristr(ShaderType->GetName(), TEXT("DepthOnly")))
		{
			bShaderTypeMatches = TRUE;
		}
		else if (appStristr(ShaderType->GetName(), TEXT("ShadowDepth")))
		{
			bShaderTypeMatches = TRUE;
		}
		else if ((RenderLitTranslucencyDepthPostpass() || GetBlendMode() == BLEND_SoftMasked)
			&& appStristr(ShaderType->GetName(), TEXT("FTranslucencyPostRenderDepth")))
		{
			bShaderTypeMatches = TRUE;
		}

		return bShaderTypeMatches;
	}

	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual UBOOL IsPersistent() const { return FALSE; }

	// FMaterialRenderProxy interface
	virtual const FMaterial* GetMaterial() const
	{
		if(GetShaderMap())
		{
			return this;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(FALSE)->GetMaterial();
		}
	}

	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		return Material->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		return Material->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		return Material->GetRenderProxy(0)->GetTextureValue(ParameterName,OutValue,Context);
	}
};

} // namespace



namespace {

/**
 * Hitproxy for the realtime preview icon on material expression nodes.
 */
struct HRealtimePreviewProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( HRealtimePreviewProxy, HHitProxy );

	UMaterialExpression*	MaterialExpression;

	HRealtimePreviewProxy(UMaterialExpression* InMaterialExpression)
		:	HHitProxy( HPP_UI )
		,	MaterialExpression( InMaterialExpression )
	{}
	virtual void Serialize(FArchive& Ar)
	{
		Ar << MaterialExpression;
	}
};

/**
 * Returns a reference to the material to be used as the material expression copy-paste buffer.
 */
static UMaterial* GetCopyPasteBuffer()
{
	if ( !GUnrealEd->MaterialCopyPasteBuffer )
	{
		GUnrealEd->MaterialCopyPasteBuffer = ConstructObject<UMaterial>( UMaterial::StaticClass() );
	}
	return GUnrealEd->MaterialCopyPasteBuffer;
}

/**
 * Returns the UStruct corrsponding to UMaterialExpression::ExpressionInput.
 */
static const UStruct* GetExpressionInputStruct()
{
	static const UStruct* ExpressionInputStruct = FindField<UStruct>( UMaterialExpression::StaticClass(), TEXT("ExpressionInput") );
	check( ExpressionInputStruct );
	return ExpressionInputStruct;
}

/**
 * Allows access to a material's input by index.
 */
static FExpressionInput* GetMaterialInput(UMaterial* Material, INT Index)
{
	FExpressionInput* ExpressionInput = NULL;
	switch( Index )
	{
	case 0: ExpressionInput = &Material->DiffuseColor ; break;
	case 1: ExpressionInput = &Material->DiffusePower ; break;
	case 2: ExpressionInput = &Material->EmissiveColor ; break;
	case 3: ExpressionInput = &Material->SpecularColor ; break;
	case 4: ExpressionInput = &Material->SpecularPower ; break;
	case 5: ExpressionInput = &Material->Opacity ; break;
	case 6: ExpressionInput = &Material->OpacityMask ; break;
	case 7: ExpressionInput = &Material->Distortion ; break;
	case 8: ExpressionInput = &Material->TwoSidedLightingMask ; break;
	case 9: ExpressionInput = &Material->TwoSidedLightingColor ; break;
	case 10: ExpressionInput = &Material->Normal ; break;
	case 11: ExpressionInput = &Material->CustomLighting ; break;
	case 12: ExpressionInput = &Material->CustomSkylightDiffuse ; break;
	case 13: ExpressionInput = &Material->AnisotropicDirection ; break;
	case 14: ExpressionInput = &Material->WorldPositionOffset ; break;
	case 15: ExpressionInput = &Material->WorldDisplacement ; break;
	case 16: ExpressionInput = &Material->TessellationFactors ; break;
	case 17: ExpressionInput = &Material->SubsurfaceInscatteringColor; break;
	case 18: ExpressionInput = &Material->SubsurfaceAbsorptionColor; break;
	case 19: ExpressionInput = &Material->SubsurfaceScatteringRadius; break;
	default: appErrorf( TEXT("%i: Invalid material input index"), Index );
	}
	return ExpressionInput;
}

/**
 * Finds the material expression that maps to the given index when unused
 * connections are hidden. 
 *
 * When unused connections are hidden, the given input index does not correspond to the
 * visible input node presented to the user. This function translates it to the correct input.
 */
static FExpressionInput* GetMatchingVisibleMaterialInput(UMaterial* Material, INT Index)
{
	FExpressionInput* MaterialInput = NULL;
	INT VisibleNodesToFind = Index;
	INT InputIndex = 0;

	// When VisibleNodesToFind is zero, then we found the corresponding input.
	while (VisibleNodesToFind >= 0)
	{
		MaterialInput = GetMaterialInput( Material, InputIndex );

		if (MaterialInput->IsConnected())
		{
			VisibleNodesToFind--;
		}

		InputIndex++;
	}

	// If VisibleNodesToFind is less than zero, then the loop to find the matching material input 
	// terminated prematurely. The material input could be pointing to the incorrect input.
	check(VisibleNodesToFind < 0);

	return MaterialInput;
}

/**
 * Returns the expression input from the given material.
 *
 * This function is a wrapper for finding the correct material input in the event that unused connectors are hidden. 
 */
static FExpressionInput* GetMaterialInputConditional(UMaterial* Material, INT Index, UBOOL bAreUnusedConnectorsHidden)
{
	FExpressionInput* MaterialInput = NULL;

	if(bAreUnusedConnectorsHidden)
	{
		MaterialInput = GetMatchingVisibleMaterialInput(Material, Index);
	} 
	else
	{
		MaterialInput = GetMaterialInput(Material, Index);
	}

	return MaterialInput;
}

/**
 * Connects the specified input expression to the specified output expression.
 */
static void ConnectExpressions(FExpressionInput& Input, const FExpressionOutput& Output, UMaterialExpression* Expression)
{
	Input.Expression = Expression;
	Input.Mask = Output.Mask;
	Input.MaskR = Output.MaskR;
	Input.MaskG = Output.MaskG;
	Input.MaskB = Output.MaskB;
	Input.MaskA = Output.MaskA;
}

/**
 * Connects the MaterialInputIndex'th material input to the MaterialExpressionOutputIndex'th material expression output.
 */
static void ConnectMaterialToMaterialExpression(UMaterial* Material,
												INT MaterialInputIndex,
												UMaterialExpression* MaterialExpression,
												INT MaterialExpressionOutputIndex,
												UBOOL bUnusedConnectionsHidden)
{
	// Assemble a list of outputs this material expression has.
	TArray<FExpressionOutput> Outputs;
	MaterialExpression->GetOutputs(Outputs);

	const FExpressionOutput& ExpressionOutput = Outputs( MaterialExpressionOutputIndex );
	FExpressionInput* MaterialInput = GetMaterialInputConditional( Material, MaterialInputIndex, bUnusedConnectionsHidden );

	ConnectExpressions( *MaterialInput, ExpressionOutput, MaterialExpression );
}

/** 
 * Helper struct that contains a reference to an expression and a subset of its inputs.
 */
struct FMaterialExpressionReference
{
public:
	FMaterialExpressionReference(UMaterialExpression* InExpression) :
		Expression(InExpression)
	{}

	FMaterialExpressionReference(FExpressionInput* InInput) :
		Expression(NULL)
	{
		Inputs.AddItem(InInput);
	}

	UMaterialExpression* Expression;
	TArray<FExpressionInput*> Inputs;
};

/**
 * Assembles a list of UMaterialExpressions and their FExpressionInput objects that refer to the specified FExpressionOutput.
 */
static void GetListOfReferencingInputs(const UMaterialExpression* InMaterialExpression, UMaterial* Material, TArray<FMaterialExpressionReference>& OutReferencingInputs, const FExpressionOutput* MaterialExpressionOutput)
{
	OutReferencingInputs.Empty();

	// Gather references from other expressions
	for ( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );

		const TArray<FExpressionInput*>& ExpressionInputs  =MaterialExpression->GetInputs();
		FMaterialExpressionReference NewReference(MaterialExpression);
		for ( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
		{
			FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
			if ( Input->Expression == InMaterialExpression &&
				(!MaterialExpressionOutput ||
				Input->Mask == MaterialExpressionOutput->Mask &&
				Input->MaskR == MaterialExpressionOutput->MaskR &&
				Input->MaskG == MaterialExpressionOutput->MaskG &&
				Input->MaskB == MaterialExpressionOutput->MaskB &&
				Input->MaskA == MaterialExpressionOutput->MaskA) )
			{
				NewReference.Inputs.AddItem(Input);
			}
		}

		if (NewReference.Inputs.Num() > 0)
		{
			OutReferencingInputs.AddItem(NewReference);
		}
	}

	// Gather references from material inputs
#define __GATHER_REFERENCE_TO_EXPRESSION( MatExpr, Mat, Prop ) \
	if ( Mat->Prop.Expression == InMaterialExpression && \
		(!MaterialExpressionOutput || \
		Mat->Prop.Mask == MaterialExpressionOutput->Mask && \
		Mat->Prop.MaskR == MaterialExpressionOutput->MaskR && \
		Mat->Prop.MaskG == MaterialExpressionOutput->MaskG && \
		Mat->Prop.MaskB == MaterialExpressionOutput->MaskB && \
		Mat->Prop.MaskA == MaterialExpressionOutput->MaskA )) \
	{ OutReferencingInputs.AddItem(FMaterialExpressionReference(&(Mat->Prop))); }

	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, DiffuseColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, DiffusePower );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, EmissiveColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, SpecularColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, SpecularPower );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, Opacity );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, OpacityMask );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, Distortion );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, TwoSidedLightingMask );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, TwoSidedLightingColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, Normal );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, CustomLighting );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, CustomSkylightDiffuse );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, AnisotropicDirection );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, WorldPositionOffset );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, WorldDisplacement );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, TessellationFactors );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, SubsurfaceInscatteringColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, SubsurfaceAbsorptionColor );
	__GATHER_REFERENCE_TO_EXPRESSION( InMaterialExpression, Material, SubsurfaceScatteringRadius );
#undef __GATHER_REFERENCE_TO_EXPRESSION
}

/**
 * Assembles a list of FExpressionInput objects that refer to the specified FExpressionOutput.
 */
static void GetListOfReferencingInputs(const UMaterialExpression* InMaterialExpression, UMaterial* Material, TArray<FExpressionInput*>& OutReferencingInputs, const FExpressionOutput* MaterialExpressionOutput = NULL)
{
	TArray<FMaterialExpressionReference> References;
	GetListOfReferencingInputs(InMaterialExpression, Material, References, MaterialExpressionOutput);
	OutReferencingInputs.Empty();
	for (INT ReferenceIndex = 0; ReferenceIndex < References.Num(); ReferenceIndex++)
	{
		for (INT InputIndex = 0; InputIndex < References(ReferenceIndex).Inputs.Num(); InputIndex++)
		{
			OutReferencingInputs.AddItem(References(ReferenceIndex).Inputs(InputIndex));
		}
	}
}

/**
 * Swaps all links to OldExpression with NewExpression.  NewExpression may be NULL.
 */
static void SwapLinksToExpression(UMaterialExpression* OldExpression, UMaterialExpression* NewExpression, UMaterial* Material)
{
	check(OldExpression);
	check(Material);

	Material->Modify();

	{
		// Move any of OldExpression's inputs over to NewExpression
		const TArray<FExpressionInput*>& OldExpressionInputs = OldExpression->GetInputs();

		TArray<FExpressionInput*> NewExpressionInputs;
		if (NewExpression)
		{
			NewExpression->Modify();
			NewExpressionInputs = NewExpression->GetInputs();
		}

		// Copy the inputs over, matching them up based on the order they are declared in each class
		for (INT InputIndex = 0; InputIndex < OldExpressionInputs.Num() && InputIndex < NewExpressionInputs.Num(); InputIndex++)
		{
			*NewExpressionInputs(InputIndex) = *OldExpressionInputs(InputIndex);
		}
	}	
	
	// Move any links from other expressions to OldExpression over to NewExpression
	TArray<FExpressionOutput> Outputs;
	OldExpression->GetOutputs(Outputs);
	TArray<FExpressionOutput> NewOutputs;
	if (NewExpression)
	{
		NewExpression->GetOutputs(NewOutputs);
	}
	else
	{
		NewOutputs.AddItem(FExpressionOutput(FALSE));
	}

	for (INT OutputIndex = 0; OutputIndex < Outputs.Num(); OutputIndex++)
	{
		const FExpressionOutput& CurrentOutput = Outputs(OutputIndex);
		
		FExpressionOutput NewOutput(FALSE);
		UBOOL bFoundMatchingOutput = FALSE;
		// Try to find an equivalent output in NewExpression
		for (INT NewOutputIndex = 0; NewOutputIndex < NewOutputs.Num(); NewOutputIndex++)
		{
			const FExpressionOutput& CurrentNewOutput = NewOutputs(NewOutputIndex);
			if(	CurrentOutput.Mask == CurrentNewOutput.Mask
				&& CurrentOutput.MaskR == CurrentNewOutput.MaskR
				&& CurrentOutput.MaskG == CurrentNewOutput.MaskG
				&& CurrentOutput.MaskB == CurrentNewOutput.MaskB
				&& CurrentOutput.MaskA == CurrentNewOutput.MaskA )
			{
				NewOutput = CurrentNewOutput;
				bFoundMatchingOutput = TRUE;
			}
		}
		// Couldn't find an equivalent output in NewExpression, just pick the first
		// The user will have to fix up any issues from the mismatch
		if (!bFoundMatchingOutput)
		{
			NewOutput = NewOutputs(0);
		}

		TArray<FMaterialExpressionReference> ReferencingInputs;
		GetListOfReferencingInputs(OldExpression, Material, ReferencingInputs, &CurrentOutput);
		for (INT ReferenceIndex = 0; ReferenceIndex < ReferencingInputs.Num(); ReferenceIndex++)
		{
			FMaterialExpressionReference& CurrentReference = ReferencingInputs(ReferenceIndex);
			if (CurrentReference.Expression)
			{
				CurrentReference.Expression->Modify();
			}
			// Move the link to OldExpression over to NewExpression
			for (INT InputIndex = 0; InputIndex < CurrentReference.Inputs.Num(); InputIndex++)
			{
				ConnectExpressions(*CurrentReference.Inputs(InputIndex), NewOutput, NewExpression);
			}
		}
	}
}

/**
 * Populates the specified material's Expressions array (eg if cooked out or old content).
 * Also ensures materials and expressions are RF_Transactional for undo/redo support.
 */
static void InitExpressions(UMaterial* Material)
{
	FString ParmName;

	if( Material->Expressions.Num() == 0 )
	{
		for( TObjectIterator<UMaterialExpression> It; It; ++It )
		{
			UMaterialExpression* MaterialExpression = *It;
			if( MaterialExpression->GetOuter() == Material && !MaterialExpression->IsPendingKill() )
			{
				// Comment expressions are stored in a separate list.
				if ( MaterialExpression->IsA( UMaterialExpressionComment::StaticClass() ) )
				{
					Material->EditorComments.AddItem( static_cast<UMaterialExpressionComment*>(MaterialExpression) );
				}
				else
				{
					Material->Expressions.AddItem( MaterialExpression );
				}

				Material->AddExpressionParameter(MaterialExpression);
			}
		}
	}
	else
	{
		Material->BuildEditorParameterList();
	}

	// Propagate RF_Transactional to all referenced material expressions.
	Material->SetFlags( RF_Transactional );
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->Expressions.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( MaterialExpressionIndex );
		MaterialExpression->SetFlags( RF_Transactional );
	}
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->EditorComments.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionComment* Comment = Material->EditorComments( MaterialExpressionIndex );
		Comment->SetFlags( RF_Transactional );
	}
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->EditorCompounds.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionCompound* Compound = Material->EditorCompounds( MaterialExpressionIndex );
		Compound->SetFlags( RF_Transactional );
	}

}

} // namespace

/** Implementation of Preview Material functions*/
FMaterialResource* UPreviewMaterial::AllocateResource()
{
	return new FPreviewMaterial(this);
}
IMPLEMENT_CLASS(UPreviewMaterial);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMaterialEditor
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/** 
 * TRUE if the list of UMaterialExpression-derived classes shared between material
 * editors has already been created.
 */
UBOOL			WxMaterialEditor::bMaterialExpressionClassesInitialized = FALSE;

/** Static array of UMaterialExpression-derived classes, shared between all material editor instances. */
TArray<UClass*> WxMaterialEditor::MaterialExpressionClasses;

/** Static array of categorized material expression classes. */
TArray<FCategorizedMaterialExpressionNode> WxMaterialEditor::CategorizedMaterialExpressionClasses;
TArray<UClass*> WxMaterialEditor::FavoriteExpressionClasses;
TArray<UClass*> WxMaterialEditor::UnassignedExpressionClasses;

/**
 * Comparison function used to sort material expression classes by name.
 */
IMPLEMENT_COMPARE_POINTER( UClass, MaterialEditor, { return appStricmp( *A->GetName(), *B->GetName() ); } )

/**
 *	Grab the expression array for the given category.
 *	
 *	@param	InCategoryName	The category to retrieve
 *	@param	bCreate			If TRUE, create the entry if not found.
 *
 *	@return	The category node.
 */
FCategorizedMaterialExpressionNode* WxMaterialEditor::GetCategoryNode(FString& InCategoryName, UBOOL bCreate)
{
	for (INT CheckIndex = 0; CheckIndex < CategorizedMaterialExpressionClasses.Num(); CheckIndex++)
	{
		FCategorizedMaterialExpressionNode* CheckNode = &(CategorizedMaterialExpressionClasses(CheckIndex));
		if (CheckNode)
		{
			if (CheckNode->CategoryName == InCategoryName)
			{
				return CheckNode;
			}
		}
	}

	if (bCreate == TRUE)
	{
		FCategorizedMaterialExpressionNode* NewNode = new(CategorizedMaterialExpressionClasses)FCategorizedMaterialExpressionNode;
		check(NewNode);

		NewNode->CategoryName = InCategoryName;
		return NewNode;
	}

	return NULL;
}

/** Sorting helper... */
IMPLEMENT_COMPARE_CONSTREF(FCategorizedMaterialExpressionNode,NewMaterialEditor,{ return A.CategoryName > B.CategoryName ? 1 : -1; });

/**
 * Initializes the list of UMaterialExpression-derived classes shared between all material editor instances.
 */
void WxMaterialEditor::InitMaterialExpressionClasses()
{
	if( !bMaterialExpressionClassesInitialized )
	{
		UMaterialEditorOptions* TempEditorOptions = ConstructObject<UMaterialEditorOptions>( UMaterialEditorOptions::StaticClass() );
		UClass* BaseType = UMaterialExpression::StaticClass();
		if( BaseType )
		{
			TArray<UStructProperty*>	ExpressionInputs;
			const UStruct*				ExpressionInputStruct = GetExpressionInputStruct();

			for( TObjectIterator<UClass> It ; It ; ++It )
			{
				UClass* Class = *It;
				if( !(Class->ClassFlags & CLASS_Abstract) && !(Class->ClassFlags & CLASS_Deprecated) )
				{
					if( Class->IsChildOf(UMaterialExpression::StaticClass()) )
					{
						ExpressionInputs.Empty();

						// Exclude comments and compounds from the expression list, as well as the base parameter expression, as it should not be used directly
						if ( Class != UMaterialExpressionComment::StaticClass() && Class != UMaterialExpressionCompound::StaticClass() && Class != UMaterialExpressionParameter::StaticClass() )
						{
							MaterialExpressionClasses.AddItem( Class );

							// Initialize the expression class input map.							
							for( TFieldIterator<UStructProperty> InputIt(Class) ; InputIt ; ++InputIt )
							{
								UStructProperty* StructProp = *InputIt;
								if( StructProp->Struct == ExpressionInputStruct )
								{
									ExpressionInputs.AddItem( StructProp );
								}
							}

							// See if it is in the favorites array...
							for (INT FavoriteIndex = 0; FavoriteIndex < TempEditorOptions->FavoriteExpressions.Num(); FavoriteIndex++)
							{
								if (Class->GetName() == TempEditorOptions->FavoriteExpressions(FavoriteIndex))
								{
									FavoriteExpressionClasses.AddUniqueItem(Class);
								}
							}

							// Category fill...
							UMaterialExpression* TempObject = Cast<UMaterialExpression>(Class->GetDefaultObject());
							if (TempObject)
							{
								if (TempObject->MenuCategories.Num() == 0)
								{
									UnassignedExpressionClasses.AddItem(Class);
								}
								else
								{
									for (INT CategoryIndex = 0; CategoryIndex < TempObject->MenuCategories.Num(); CategoryIndex++)
									{
										FString TempName = TempObject->MenuCategories(CategoryIndex).ToString();
										FCategorizedMaterialExpressionNode* CategoryNode = GetCategoryNode(
											TempName, TRUE);
										check(CategoryNode);

										CategoryNode->MaterialExpressionClasses.AddUniqueItem(Class);
									}
								}
							}
						}
					}
				}
			}
		}

		Sort<USE_COMPARE_POINTER(UClass,MaterialEditor)>( static_cast<UClass**>(MaterialExpressionClasses.GetTypedData()), MaterialExpressionClasses.Num() );
		Sort<USE_COMPARE_CONSTREF(FCategorizedMaterialExpressionNode,NewMaterialEditor)>(&(CategorizedMaterialExpressionClasses(0)),CategorizedMaterialExpressionClasses.Num());

		bMaterialExpressionClassesInitialized = TRUE;
	}
}

/**
 * Remove the expression from the favorites menu list.
 */
void WxMaterialEditor::RemoveMaterialExpressionFromFavorites(UClass* InClass)
{
	FavoriteExpressionClasses.RemoveItem(InClass);
}

/**
 * Add the expression to the favorites menu list.
 */
void WxMaterialEditor::AddMaterialExpressionToFavorites(UClass* InClass)
{
	FavoriteExpressionClasses.AddUniqueItem(InClass);
}

BEGIN_EVENT_TABLE( WxMaterialEditor, WxMaterialEditorBase )
	EVT_CLOSE( WxMaterialEditor::OnClose )
	EVT_MENU_RANGE( IDM_NEW_SHADER_EXPRESSION_START, IDM_NEW_SHADER_EXPRESSION_END, WxMaterialEditor::OnNewMaterialExpression )
	EVT_MENU( ID_MATERIALEDITOR_NEW_COMMENT, WxMaterialEditor::OnNewComment )
	EVT_MENU( ID_MATERIALEDITOR_NEW_COMPOUND_EXPRESSION, WxMaterialEditor::OnNewCompoundExpression )
	EVT_MENU( IDM_USE_CURRENT_TEXTURE, WxMaterialEditor::OnUseCurrentTexture )

	EVT_TOOL( IDM_SHOW_BACKGROUND, WxMaterialEditor::OnShowBackground )
	EVT_TOOL( ID_MATERIALEDITOR_TOGGLEGRID, WxMaterialEditor::OnToggleGrid )
	EVT_TOOL( ID_MATERIALEDITOR_SHOWHIDE_CONNECTORS, WxMaterialEditor::OnShowHideConnectors )

	EVT_TOOL( ID_MATERIALEDITOR_REALTIME_EXPRESSIONS, WxMaterialEditor::OnRealTimeExpressions )
	EVT_TOOL( ID_MATERIALEDITOR_ALWAYS_REFRESH_ALL_PREVIEWS, WxMaterialEditor::OnAlwaysRefreshAllPreviews )

	EVT_UPDATE_UI( ID_MATERIALEDITOR_REALTIME_EXPRESSIONS, WxMaterialEditor::UI_RealTimeExpressions )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_SHOWHIDE_CONNECTORS, WxMaterialEditor::UI_HideUnusedConnectors )

	EVT_BUTTON( ID_MATERIALEDITOR_APPLY, WxMaterialEditor::OnApply )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_APPLY, WxMaterialEditor::UI_Apply )

	EVT_TOOL( ID_MATERIALEDITOR_FLATTEN, WxMaterialEditor::OnFlatten )

	EVT_TOOL( ID_MATERIALEDITOR_TOGGLESTATS, WxMaterialEditor::OnToggleStats )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_TOGGLESTATS, WxMaterialEditor::UI_ToggleStats )

	EVT_TOOL( ID_MATERIALEDITOR_VIEWSOURCE, WxMaterialEditor::OnViewSource )
	EVT_UPDATE_UI( ID_MATERIALEDITOR_VIEWSOURCE, WxMaterialEditor::UI_ViewSource )

	EVT_TOOL( ID_GO_HOME, WxMaterialEditor::OnCameraHome )
	EVT_TOOL( ID_MATERIALEDITOR_CLEAN_UNUSED_EXPRESSIONS, WxMaterialEditor::OnCleanUnusedExpressions )

	EVT_TREE_BEGIN_DRAG( ID_MATERIALEDITOR_MATERIALEXPRESSIONTREE, WxMaterialEditor::OnMaterialExpressionTreeDrag )
	EVT_LIST_BEGIN_DRAG( ID_MATERIALEDITOR_MATERIALEXPRESSIONLIST, WxMaterialEditor::OnMaterialExpressionListDrag )

	EVT_MENU( ID_MATERIALEDITOR_DUPLICATE_NODES, WxMaterialEditor::OnDuplicateObjects )
	EVT_MENU( ID_MATERIALEDITOR_DELETE_NODE, WxMaterialEditor::OnDeleteObjects )
	EVT_MENU( ID_MATERIALEDITOR_CONVERT_TO_PARAMETER, WxMaterialEditor::OnConvertObjects )
	EVT_MENU( ID_MATERIALEDITOR_SELECT_DOWNSTREAM_NODES, WxMaterialEditor::OnSelectDownsteamNodes )
	EVT_MENU( ID_MATERIALEDITOR_SELECT_UPSTREAM_NODES, WxMaterialEditor::OnSelectUpsteamNodes )
	EVT_MENU( ID_MATERIALEDITOR_BREAK_LINK, WxMaterialEditor::OnBreakLink )
	EVT_MENU( ID_MATERIALEDITOR_BREAK_ALL_LINKS, WxMaterialEditor::OnBreakAllLinks )
	EVT_MENU( ID_MATERIALEDITOR_REMOVE_FROM_FAVORITES, WxMaterialEditor::OnRemoveFromFavorites )
	EVT_MENU( ID_MATERIALEDITOR_ADD_TO_FAVORITES, WxMaterialEditor::OnAddToFavorites )
	EVT_MENU( ID_MATERIALEDITOR_DEBUG_NODE, WxMaterialEditor::OnDebugNode )

	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_DiffuseColor, WxMaterialEditor::OnConnectToMaterial_DiffuseColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_DiffusePower, WxMaterialEditor::OnConnectToMaterial_DiffusePower )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_EmissiveColor, WxMaterialEditor::OnConnectToMaterial_EmissiveColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_SpecularColor, WxMaterialEditor::OnConnectToMaterial_SpecularColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_SpecularPower, WxMaterialEditor::OnConnectToMaterial_SpecularPower )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_Opacity, WxMaterialEditor::OnConnectToMaterial_Opacity )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_OpacityMask, WxMaterialEditor::OnConnectToMaterial_OpacityMask )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_Distortion, WxMaterialEditor::OnConnectToMaterial_Distortion)
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_TransmissionMask, WxMaterialEditor::OnConnectToMaterial_TransmissionMask )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_TransmissionColor, WxMaterialEditor::OnConnectToMaterial_TransmissionColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_Normal, WxMaterialEditor::OnConnectToMaterial_Normal )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_CustomLighting, WxMaterialEditor::OnConnectToMaterial_CustomLighting )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_CustomLightingDiffuse, WxMaterialEditor::OnConnectToMaterial_CustomLightingDiffuse )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_AnisotropicDirection, WxMaterialEditor::OnConnectToMaterial_AnisotropicDirection )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_WorldPositionOffset, WxMaterialEditor::OnConnectToMaterial_WorldPositionOffset )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_WorldDisplacement, WxMaterialEditor::OnConnectToMaterial_WorldDisplacement )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_TessellationFactors, WxMaterialEditor::OnConnectToMaterial_TessellationFactors )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_SubsurfaceInscatteringColor, WxMaterialEditor::OnConnectToMaterial_SubsurfaceInscatteringColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_SubsurfaceAbsorptionColor, WxMaterialEditor::OnConnectToMaterial_SubsurfaceAbsorptionColor )
	EVT_MENU( ID_MATERIALEDITOR_CONNECT_TO_SubsurfaceScatteringRadius, WxMaterialEditor::OnConnectToMaterial_SubsurfaceScatteringRadius )
	
	EVT_TEXT(ID_MATERIALEDITOR_SEARCH, WxMaterialEditor::OnSearchChanged)
	EVT_BUTTON(ID_SEARCHTEXTCTRL_FINDNEXT_BUTTON, WxMaterialEditor::OnSearchNext)
	EVT_BUTTON(ID_SEARCHTEXTCTRL_FINDPREV_BUTTON, WxMaterialEditor::OnSearchPrev)
	
END_EVENT_TABLE()

/**
* A class to hold the data about an item in the tree
*/
class FExpressionTreeData : public wxTreeItemData
{
	INT Index;
public:
	FExpressionTreeData( INT InIndex )
	:	Index(InIndex)
	{}
	INT GetIndex()
	{
		return Index;
	}
};



/**
 * A panel containing a list of material expression node types.
 */
struct FExpressionListEntry
{
	FExpressionListEntry( const TCHAR* InExpressionName, INT InClassIndex )
	:	ExpressionName(InExpressionName), ClassIndex(InClassIndex)
	{}

	FString ExpressionName;
	INT ClassIndex;
};

IMPLEMENT_COMPARE_CONSTREF( FExpressionListEntry, NewMaterialEditor, { return appStricmp(*A.ExpressionName,*B.ExpressionName); } );

class WxMaterialExpressionList : public wxPanel
{
public:
	WxMaterialExpressionList(wxWindow* InParent)
		: wxPanel( InParent )
	{
		// Search/Filter box
		FilterSearchCtrl = new WxSearchControl;
		FilterSearchCtrl->Create(this, ID_MATERIALEDITOR_MATERIALEXPRESSIONLIST_SEARCHBOX);

		// Tree
		MaterialExpressionTree = new wxTreeCtrl( this, ID_MATERIALEDITOR_MATERIALEXPRESSIONTREE, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT|wxTR_SINGLE );
		MaterialExpressionFilterList = new wxListCtrl( this, ID_MATERIALEDITOR_MATERIALEXPRESSIONLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_NO_HEADER|wxLC_SINGLE_SEL );

		// Hide the list unless we have a filter.
		MaterialExpressionFilterList->Show(FALSE);

		Sizer = new wxBoxSizer(wxVERTICAL);
		Sizer->Add( FilterSearchCtrl, 0, wxGROW|wxALL, 4 );
		Sizer->Add( MaterialExpressionTree, 1, wxGROW|wxALL, 4 );
		Sizer->Add( MaterialExpressionFilterList, 1, wxGROW|wxALL, 4 );
		SetSizer( Sizer );
		SetAutoLayout(true);
	}

	void PopulateExpressionsTree()
	{
		MaterialExpressionTree->Freeze();
		MaterialExpressionTree->DeleteAllItems();

		TMap<FName,wxTreeItemId> ExpressionCategoryMap;

		check( WxMaterialEditor::bMaterialExpressionClassesInitialized );

		// not shown so doesn't need localization
		wxTreeItemId RootItem = MaterialExpressionTree->AddRoot(TEXT("Expressions"));

		for( INT Index = 0 ; Index < WxMaterialEditor::MaterialExpressionClasses.Num() ; ++Index )
		{
			UClass* MaterialExpressionClass = WxMaterialEditor::MaterialExpressionClasses(Index);

			// Trim the material expression name and add it to the list used for filtering.
			FString ExpressionName = FString(*MaterialExpressionClass->GetName()).Mid(appStrlen(TEXT("MaterialExpression"))); 
			new(ExpressionList) FExpressionListEntry(*ExpressionName, Index);

			UMaterialExpression* TempObject = Cast<UMaterialExpression>(MaterialExpressionClass->GetDefaultObject());
			if (TempObject)
			{
				// add at top level as there are no categories
				if (TempObject->MenuCategories.Num() == 0)
				{
					MaterialExpressionTree->AppendItem( RootItem, *ExpressionName, -1, -1, new FExpressionTreeData(Index) );
				}
				else
				{
					for( INT CategoryIdx = 0 ; CategoryIdx < TempObject->MenuCategories.Num() ; CategoryIdx++ )
					{
						// find or create category tree items as necessary
						FName CategoryName = TempObject->MenuCategories(CategoryIdx);
						wxTreeItemId* ExistingItem = ExpressionCategoryMap.Find(CategoryName);
						if ( ExistingItem == NULL )
						{
							ExistingItem = &ExpressionCategoryMap.Set( CategoryName, MaterialExpressionTree->AppendItem( RootItem, *CategoryName.ToString() ) );
							MaterialExpressionTree->SetItemBold( *ExistingItem );
						}

						// add the new item
						wxTreeItemId NewItem = MaterialExpressionTree->AppendItem( *ExistingItem, *ExpressionName, -1, -1, new FExpressionTreeData(Index) );
					}
				}
			}
		}

		// Sort
		for( TMap<FName,wxTreeItemId>::TIterator It(ExpressionCategoryMap); It; ++It )
		{
			MaterialExpressionTree->SortChildren(It.Value());
		}
		MaterialExpressionTree->SortChildren(RootItem);

		MaterialExpressionTree->ExpandAll();
		MaterialExpressionTree->Thaw();

		Sort<USE_COMPARE_CONSTREF(FExpressionListEntry,NewMaterialEditor)>( &ExpressionList(0), ExpressionList.Num() );
	}

	void OnFilterChanged( wxCommandEvent& In )
	{
		FString FilterString = In.GetString().c_str();
		if( FilterString.Len() > 0 )
		{
			MaterialExpressionTree->Show(FALSE);
			MaterialExpressionFilterList->Show(TRUE);

			MaterialExpressionFilterList->Freeze();
			MaterialExpressionFilterList->ClearAll();
			MaterialExpressionFilterList->InsertColumn( 0, TEXT(""), wxLIST_FORMAT_CENTRE, 250 );

			INT ItemIndex = 0;
			for( INT Index=0; Index < ExpressionList.Num(); Index++ )
			{
				if( ExpressionList(Index).ExpressionName.InStr(*FilterString, FALSE, TRUE) != -1 )
				{
					// Add the item, and the 
					MaterialExpressionFilterList->InsertItem( ItemIndex, *ExpressionList(Index).ExpressionName );
					MaterialExpressionFilterList->SetItemData( ItemIndex, ExpressionList(Index).ClassIndex );
					ItemIndex++;
				}
			}		

			MaterialExpressionFilterList->Thaw();
		}
		else
		{
			MaterialExpressionTree->Show(TRUE);
			MaterialExpressionFilterList->Show(FALSE);
		}
		Sizer->Layout();
	}

	FString GetSelectedTreeString(wxTreeEvent& DragEvent)
	{
		FString Result;
		wxTreeItemId SelectedItem = DragEvent.GetItem();
		wxTreeItemData* SelectedItemData = NULL;
		if ( SelectedItem.IsOk() && (SelectedItemData=MaterialExpressionTree->GetItemData(SelectedItem))!=NULL )
		{
			Result = FString::Printf( TEXT("%i"), static_cast<FExpressionTreeData*>(SelectedItemData)->GetIndex() );
		}
		return Result;
	}

	FString GetSelectedListString()
	{
		FString Result;
		const long ItemIndex = MaterialExpressionFilterList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( ItemIndex != -1 )
		{
			Result = FString::Printf( TEXT("%i"), MaterialExpressionFilterList->GetItemData(ItemIndex) );
		}
		return Result;
	}

	DECLARE_EVENT_TABLE()

private:
	wxTreeCtrl* MaterialExpressionTree;
	wxListCtrl* MaterialExpressionFilterList;
	WxSearchControl* FilterSearchCtrl;
	TArray<FExpressionListEntry> ExpressionList;
	wxBoxSizer* Sizer;
};

BEGIN_EVENT_TABLE( WxMaterialExpressionList, wxPanel )
	EVT_TEXT(ID_MATERIALEDITOR_MATERIALEXPRESSIONLIST_SEARCHBOX,WxMaterialExpressionList::OnFilterChanged)
END_EVENT_TABLE()


/**
* WxMaterialEditorSourceWindow - Source code display window
*/

struct FSourceHighlightRange
{
	FSourceHighlightRange(INT InBegin, INT InEnd)
	:	Begin(InBegin)
	,	End(InEnd)
	{}
	INT Begin;
	INT End;
};

class WxMaterialEditorSourceWindow : public wxPanel
{
	wxRichTextCtrl* RichTextCtrl;
	UMaterial* Material;
	UBOOL bNeedsUpdate;
	UBOOL bNeedsRedraw;

	FString Source;
	TMap<UMaterialExpression*,INT> ExpressionCodeMap[MP_MAX];
	TMap<INT, TArray<FSourceHighlightRange> > HighlightRangeMap;
	TArray<FSourceHighlightRange>* CurrentHighlightRangeArray;

public:

	UMaterialExpression* SelectedExpression;

	WxMaterialEditorSourceWindow(wxWindow* InParent, UMaterial* InMaterial)
	:	wxPanel( InParent )
	,	SelectedExpression(NULL)
	,	Material(InMaterial)
	,	bNeedsUpdate(FALSE)
	,	bNeedsRedraw(FALSE)
	,	CurrentHighlightRangeArray(NULL)
	{
		// Rich Text control
		RichTextCtrl = new wxRichTextCtrl( this, ID_MATERIALEDITOR_VIEWSOURCE_RICHTEXT, wxEmptyString, wxDefaultPosition,wxDefaultSize, wxRE_MULTILINE|wxRE_READONLY );

		wxBoxSizer* Sizer = new wxBoxSizer(wxVERTICAL);
		Sizer->Add( RichTextCtrl, 1, wxGROW|wxALL, 4 );
		SetSizer( Sizer );
		SetAutoLayout(true);
	}

#define MARKTAG TEXT("/*MARK_")
#define MARKTAGLEN 7

	void RefreshWindow(UBOOL bForce=FALSE)
	{
		// Don't do anything if the source window isn't visible.
		if( !bForce && !IsShownOnScreen() )
		{
			bNeedsUpdate = TRUE;
			return;
		}

		RegenerateSource(bForce);
	}

	void Redraw(UBOOL bForce=FALSE)
	{
		if( !bForce && !IsShownOnScreen() )
		{
			bNeedsRedraw = TRUE;
			return;
		}

		bNeedsRedraw = FALSE;

		RichTextCtrl->Freeze();
		RichTextCtrl->BeginSuppressUndo();

		// reset all to normal
		wxTextAttr NormalStyle;
		NormalStyle.SetTextColour(wxColour(0,0,0));

		// remove previous highlighting (this is much faster than setting the entire text)
		if( CurrentHighlightRangeArray )
		{
			for( INT i=0;i<CurrentHighlightRangeArray->Num();i++ )
			{
				FSourceHighlightRange& Range = (*CurrentHighlightRangeArray)(i);
				RichTextCtrl->SetStyle(Range.Begin, Range.End+1, NormalStyle);
			}
		}

		// Highlight the last-selected expression
		INT ShowPositionStart = MAXINT;
		INT ShowPositionEnd = -1;
		if( SelectedExpression )
		{
			wxTextAttr HighlightStyle;
			HighlightStyle.SetTextColour(wxColour(255,0,0));

			for (INT PropertyIndex = 0; PropertyIndex < MP_MAX; PropertyIndex++)
			{
				INT* SelectedCodeIndex = ExpressionCodeMap[PropertyIndex].Find(SelectedExpression);
				if( SelectedCodeIndex )
				{
					CurrentHighlightRangeArray = HighlightRangeMap.Find(*SelectedCodeIndex);
					if( CurrentHighlightRangeArray )
					{
						for( INT i=0;i<CurrentHighlightRangeArray->Num();i++ )
						{
							FSourceHighlightRange& Range = (*CurrentHighlightRangeArray)(i);
							RichTextCtrl->SetStyle(Range.Begin, Range.End+1, HighlightStyle);

							ShowPositionStart = Min(Range.Begin, ShowPositionStart);
							ShowPositionEnd = Max(Range.End+1, ShowPositionEnd);
						}
					}
					break;
				}
			}
		}

		RichTextCtrl->EndSuppressUndo();
		RichTextCtrl->Thaw();

		// Scroll the control if necessary to show the highlight regions
		if( ShowPositionStart != MAXINT )
		{
			if( !RichTextCtrl->IsPositionVisible(ShowPositionEnd) )
			{
				RichTextCtrl->ShowPosition(ShowPositionEnd);
			}
			if( !RichTextCtrl->IsPositionVisible(ShowPositionStart) )
			{
				RichTextCtrl->ShowPosition(ShowPositionStart);
			}
		}
	}

private:
	void OnShow(wxShowEvent& Event)
	{
		if( Event.GetShow() )
		{
			if( bNeedsUpdate )
			{
				RegenerateSource();
			}
			if( bNeedsRedraw )
			{
				Redraw();
			}
		}
	}

	// EVT_SHOW isn't reliable for docking windows, so we will use Paint instead.
	void OnPaint(wxPaintEvent& Event)
	{
		wxPaintDC dc(this);

		if( bNeedsUpdate )
		{
			RegenerateSource();
		}
		if( bNeedsRedraw )
		{
			Redraw();
		}
	}

	void RegenerateSource(UBOOL bForce=FALSE)
	{
		bNeedsUpdate = FALSE;

		Source = TEXT("");
		for (INT PropertyIndex = 0; PropertyIndex < MP_MAX; PropertyIndex++)
		{
			ExpressionCodeMap[PropertyIndex].Empty();
		}
		HighlightRangeMap.Empty();
		CurrentHighlightRangeArray = NULL;

		FString MarkupSource;
		if( Material->GetMaterialResource()->GetMaterialExpressionSource( MarkupSource, ExpressionCodeMap) )
		{
			// Remove line-feeds and leave just CRs so the character counts match the selection ranges.
			MarkupSource.ReplaceInline(TEXT("\n"), TEXT(""));

			// Extract highlight ranges from markup tags

			// Make a copy so we can insert null terminators.
			TCHAR* MarkupSourceCopy = new TCHAR[MarkupSource.Len()+1];
			appStrcpy(MarkupSourceCopy, MarkupSource.Len()+1, *MarkupSource);

			TCHAR* Ptr = MarkupSourceCopy;
			while( Ptr && *Ptr != '\0' )
			{
				TCHAR* NextTag = appStrstr( Ptr, MARKTAG );
				if( !NextTag )
				{
					// No more tags, so we're done!
					Source += Ptr;
					break;
				}

				// Copy the text up to the tag.
				*NextTag = '\0';
				Source += Ptr;

				// Advance past the markup tag to see what type it is (beginning or end)
				NextTag += MARKTAGLEN;
				INT TagNumber = appAtoi(NextTag+1);
				switch(*NextTag)
				{
				case 'B':
					{
						// begin tag
						TArray<FSourceHighlightRange>* RangeArray = HighlightRangeMap.Find(TagNumber);
						if( !RangeArray )
						{
							RangeArray = &HighlightRangeMap.Set(TagNumber,TArray<FSourceHighlightRange>() );
						}
						new(*RangeArray) FSourceHighlightRange(Source.Len(),-1);
					}
					break;
				case 'E':
					{
						// end tag
						TArray<FSourceHighlightRange>* RangeArray = HighlightRangeMap.Find(TagNumber);
						check(RangeArray);
						check(RangeArray->Num() > 0);
						(*RangeArray)(RangeArray->Num()-1).End = Source.Len();
					}
					break;
				default:
					appErrorf(TEXT("Unexpected character in material source markup tag"));
				}
				Ptr = appStrstr(NextTag, TEXT("*/")) + 2;
			}

			delete[] MarkupSourceCopy;
		}
		RichTextCtrl->Freeze();
		RichTextCtrl->BeginSuppressUndo();
		RichTextCtrl->Clear();
		RichTextCtrl->WriteText(*Source);
		RichTextCtrl->EndSuppressUndo();
		RichTextCtrl->Thaw();

		Redraw(bForce);
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WxMaterialEditorSourceWindow, wxPanel )
	EVT_SHOW(WxMaterialEditorSourceWindow::OnShow)
	EVT_PAINT(WxMaterialEditorSourceWindow::OnPaint)
END_EVENT_TABLE()


WxMaterialEditor::WxMaterialEditor(wxWindow* InParent, wxWindowID InID, UMaterial* InMaterial)
	:	WxMaterialEditorBase( InParent, InID, InMaterial )
	,  FDockingParent(this)
	,	ConnObj( NULL )
	,	ConnType( LOC_INPUT )
	,	ConnIndex( 0 )
	,	OriginalMaterial( InMaterial )
	,	ToolBar( NULL )
	,	MaterialExpressionList( NULL )
	,	SourceWindow(NULL)
	,	EditorOptions( NULL )
	,	bMaterialDirty( FALSE )
	,	bAlwaysRefreshAllPreviews( FALSE )
	,	bUseUnsortedMenus( FALSE )
	,	bHideUnusedConnectors( FALSE )
	,	bShowStats ( TRUE )
	, 	ScopedTransaction ( NULL )
	,	DebugExpression( NULL )
	,	DebugMaterial( NULL )
	,	SelectedSearchResult(0)
	,	DblClickObject(NULL)
	,	DblClickConnType(INDEX_NONE)
	,	DblClickConnIndex(INDEX_NONE)
{
	// Create a copy of the material for preview usage (duplicating to a different class than original!)
	// Propagate all object flags except for RF_Standalone, otherwise the preview material won't GC once
	// the material editor releases the reference.
	Material = (UMaterial*)UObject::StaticDuplicateObject(OriginalMaterial, OriginalMaterial, UObject::GetTransientPackage(), NULL, ~RF_Standalone, UPreviewMaterial::StaticClass()); 
	// copy material usage
	for( INT Usage=0; Usage < MATUSAGE_MAX; Usage++ )
	{
		const EMaterialUsage UsageEnum = (EMaterialUsage)Usage;
		if( OriginalMaterial->GetUsageByFlag(UsageEnum) )
		{
			UBOOL bNeedsRecompile=FALSE;
			Material->SetMaterialUsage(bNeedsRecompile,UsageEnum);
		}
	}
	// Manually copy bUsedAsSpecialEngineMaterial as it is duplicate transient to prevent accidental creation of new special engine materials
	Material->bUsedAsSpecialEngineMaterial = OriginalMaterial->bUsedAsSpecialEngineMaterial;

	// copy the flattened texture manually because it's duplicatetransient so it's NULLed when duplicating normally
	// (but we don't want it NULLed in this case)
	Material->MobileBaseTexture = OriginalMaterial->MobileBaseTexture;

	SetPreviewMaterial( Material );

	// Mark material as being the preview material to avoid unnecessary work in UMaterial::PostEditChange. This property is duplicatetransient so we don't have
	// to worry about resetting it when propagating the preview to the original material after editing.
	// Note:  The material editor must reattach the preview meshes through RefreshPreviewViewport() after calling Material::PEC()!
	Material->bIsPreviewMaterial = TRUE;

	// Set the material editor window title to include the material being edited.
	SetTitle( *FString::Printf( LocalizeSecure(LocalizeUnrealEd("MaterialEditorCaption_F"), *OriginalMaterial->GetPathName()) ) );

	// Initialize the shared list of material expression classes.
	InitMaterialExpressionClasses();

	// Make sure the material is the list of material expressions it references.
	InitExpressions( Material );

	// Set default size
	SetSize(1152,864);

	// Load the desired window position from .ini file.
	FWindowUtil::LoadPosSize(TEXT("MaterialEditor"), this, 256, 256, 1152, 864);

	// Create property window.
	PropertyWindow = new WxPropertyWindowHost;
	PropertyWindow->Create( this, this );
	
	// Create a material editor drop target.
	/** Drop target for the material editor's linked object viewport. */
	class WxMaterialEditorDropTarget : public wxTextDropTarget
	{
	public:
		WxMaterialEditorDropTarget(WxMaterialEditor* InMaterialEditor)
			: MaterialEditor( InMaterialEditor )
			, HasData( FALSE )
		{}
		virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text)
		{
			const INT LocX = (x - MaterialEditor->LinkedObjVC->Origin2D.X)/MaterialEditor->LinkedObjVC->Zoom2D;
			const INT LocY = (y - MaterialEditor->LinkedObjVC->Origin2D.Y)/MaterialEditor->LinkedObjVC->Zoom2D;
			long MaterialExpressionIndex;
			UBOOL IsExpression = text.ToLong(&MaterialExpressionIndex) && (MaterialExpressionIndex > -1) && (MaterialExpressionIndex < MaterialEditor->MaterialExpressionClasses.Num());
			UBOOL IsTexture2D = appStrstr(text, *UTexture2D::StaticClass()->GetName()) != NULL;
			if ( IsExpression )
			{
				UClass* NewExpressionClass = MaterialEditor->MaterialExpressionClasses(MaterialExpressionIndex);
				MaterialEditor->CreateNewMaterialExpression( NewExpressionClass, FALSE, TRUE, FIntPoint(LocX, LocY) );
			}
			else if ( IsTexture2D )
			{
				MaterialEditor->CreateNewMaterialExpression( UMaterialExpressionTextureSample::StaticClass(), FALSE, TRUE, FIntPoint(LocX, LocY) );
			}
			HasData = FALSE;
			return true;
		}
		virtual bool IsAcceptedData(IDataObject *pIDataSource) const
		{
			if ( wxTextDropTarget::IsAcceptedData(pIDataSource) )
			{
				// hook to set our datasource prior to calling GetData() from OnEnter;
				// some evil stuff here - don't try this at home!
				const_cast<WxMaterialEditorDropTarget*>(this)->SetDataSource(pIDataSource);
				return true;
			}

			return false;
		}
		virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
		{
			// populate the data so we can check its type in OnDragOver
			if (GetData())
			{
				HasData = TRUE;
			}

			return def;
		}
		virtual void OnLeave()
		{
			HasData = FALSE;
		}
		virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
		{
			// check if the data has been populated by OnEnter
			if (HasData)
			{
				wxTextDataObject* Data = (wxTextDataObject*)GetDataObject();
				if (Data)
				{
					const wxString& Text = Data->GetText();
					long MaterialExpressionIndex;
					UBOOL IsExpression = Text.ToLong(&MaterialExpressionIndex) && (MaterialExpressionIndex > -1) && (MaterialExpressionIndex < MaterialEditor->MaterialExpressionClasses.Num());
					UBOOL IsTexture2D = appStrstr(Text, *UTexture2D::StaticClass()->GetName()) != NULL;
					
					// only textures and new expressions can be dragged and dropped into the material editor
					if (IsExpression || IsTexture2D)
					{
						return def;
					}
				}
			}

			// for object types that aren't allowed to dragged and dropped into the material editor we return wxDragNone, which,
			// changes the mouse icon to reflect that the currently selected object cannot be dropped into the editor
			return wxDragNone;
		}
		WxMaterialEditor* MaterialEditor;
	
	private:
		UBOOL HasData;
	};
	WxMaterialEditorDropTarget* MaterialEditorDropTarget = new WxMaterialEditorDropTarget(this);

	// Create linked-object tree window.
	WxLinkedObjVCHolder* TreeWin = new WxLinkedObjVCHolder( this, -1, this );
	TreeWin->SetDropTarget( MaterialEditorDropTarget );
	LinkedObjVC = TreeWin->LinkedObjVC;
	LinkedObjVC->SetRedrawInTick( FALSE );

	// Create material expression list.
	MaterialExpressionList = new WxMaterialExpressionList( this );
	MaterialExpressionList->PopulateExpressionsTree();

	// Create source window
	SourceWindow = new WxMaterialEditorSourceWindow( this, Material );

	// Add docking windows.
	{
		SetDockHostSize(FDockingParent::DH_Bottom, 150);
		SetDockHostSize(FDockingParent::DH_Right, 150);

		AddDockingWindow( TreeWin, FDockingParent::DH_None, NULL );

		AddDockingWindow(PropertyWindow, FDockingParent::DH_Bottom, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("PropertiesCaption_F"), *OriginalMaterial->GetPathName())), *LocalizeUnrealEd("Properties"));
		AddDockingWindow(SourceWindow, FDockingParent::DH_Bottom, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("SourceCaption_F"), *OriginalMaterial->GetPathName())), *LocalizeUnrealEd("Source"));

		// Source window is hidden by default.
		ShowDockingWindow(SourceWindow, FALSE);

		AddDockingWindow((wxWindow*)PreviewWin, FDockingParent::DH_Left, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("PreviewCaption_F"), *OriginalMaterial->GetPathName())), *LocalizeUnrealEd("Preview"));
		AddDockingWindow(MaterialExpressionList, FDockingParent::DH_Right, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("MaterialExpressionListCaption_F"), *OriginalMaterial->GetPathName())), *LocalizeUnrealEd("MaterialExpressions"), wxSize(285, 240));

		// Try to load a existing layout for the docking windows.
		LoadDockingLayout();
	}
	
	// If the Source window is visible, we need to update it now, otherwise defer til it's shown.
	FDockingParent::FDockWindowState SourceWindowState;
	if( GetDockingWindowState(SourceWindow, SourceWindowState) && SourceWindowState.bIsVisible )
	{
		SourceWindow->RefreshWindow(TRUE);
	}
	else
	{
		SourceWindow->RefreshWindow(FALSE);
	}

	wxMenuBar* MenuBar = new wxMenuBar();
	AppendWindowMenu(MenuBar);
	SetMenuBar(MenuBar);

	ToolBar = new WxMaterialEditorToolBar( this, -1 );
	SetToolBar( ToolBar );
	LinkedObjVC->Origin2D = FIntPoint(-Material->EditorX,-Material->EditorY);

	BackgroundTexture = LoadObject<UTexture2D>(NULL, TEXT("EditorMaterials.MaterialsBackground"), NULL, LOAD_None, NULL);

	// Load editor settings from disk.
	LoadEditorSettings();

	// Set the preview mesh for the material.  This call must occur after the toolbar is initialized.
	if ( !SetPreviewMesh( *Material->PreviewMesh ) )
	{
		// The material preview mesh couldn't be found or isn't loaded.  Default to the one of the primitive types.
		SetPrimitivePreview();
	}

	// Initialize the contents of the property window.
	UpdatePropertyWindow();

	// Initialize the material input list.
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Diffuse"), &Material->DiffuseColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("DiffusePower"), &Material->DiffusePower ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Emissive"), &Material->EmissiveColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Specular"), &Material->SpecularColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("SpecularPower"), &Material->SpecularPower ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Opacity"), &Material->Opacity ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("OpacityMask"), &Material->OpacityMask ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Distortion"), &Material->Distortion ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("TransmissionMask"), &Material->TwoSidedLightingMask ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("TransmissionColor"), &Material->TwoSidedLightingColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("Normal"), &Material->Normal ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("CustomLighting"), &Material->CustomLighting ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("CustomLightingDiffuse"), &Material->CustomSkylightDiffuse ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("AnisotropicDirection"), &Material->AnisotropicDirection ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("WorldPositionOffset"), &Material->WorldPositionOffset ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("WorldDisplacement"), &Material->WorldDisplacement ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("TessellationFactors"), &Material->TessellationFactors ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("SubsurfaceInscatteringColor"), &Material->SubsurfaceInscatteringColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("SubsurfaceAbsorptionColor"), &Material->SubsurfaceAbsorptionColor ) );
	MaterialInputs.AddItem( FMaterialInputInfo( TEXT("SubsurfaceScatteringRadius"), &Material->SubsurfaceScatteringRadius ) );
	

	// Initialize expression previews.
	ForceRefreshExpressionPreviews();

	// Initialize compound expression info.
	FlushCompoundExpressionInfo( TRUE );

	GCallbackEvent->Register(CALLBACK_PreEditorClose, this);
	//listen for color picker events
	GCallbackEvent->Register(CALLBACK_ColorPickerChanged,this);
}

WxMaterialEditor::~WxMaterialEditor()
{
	// make sure rendering thread is done with the expressions, since they are about to be deleted
	FlushRenderingCommands();

	// Delete compound info.
	FlushCompoundExpressionInfo( FALSE );

	// Save editor settings to disk.
	SaveEditorSettings();

	check( !ScopedTransaction );

	// Null out the debug material so they can be GC'ed
	DebugMaterial = NULL;
	DebugExpression = NULL;

#if WITH_MANAGED_CODE
	UnBindColorPickers(this);
#endif
}

/**
 * Load editor settings from disk (docking state, window pos/size, option state, etc).
 */
void WxMaterialEditor::LoadEditorSettings()
{
	EditorOptions				= ConstructObject<UMaterialEditorOptions>( UMaterialEditorOptions::StaticClass() );
	bShowGrid					= EditorOptions->bShowGrid;
	bShowBackground				= EditorOptions->bShowBackground;
	bHideUnusedConnectors		= EditorOptions->bHideUnusedConnectors;
	bAlwaysRefreshAllPreviews	= EditorOptions->bAlwaysRefreshAllPreviews;
	bUseUnsortedMenus			= EditorOptions->bUseUnsortedMenus;

	if ( ToolBar )
	{
		ToolBar->SetShowGrid( bShowGrid );
		ToolBar->SetShowBackground( bShowBackground );
		ToolBar->SetHideConnectors( bHideUnusedConnectors );
		ToolBar->SetAlwaysRefreshAllPreviews( bAlwaysRefreshAllPreviews );
		ToolBar->SetRealtimeMaterialPreview( EditorOptions->bRealtimeMaterialViewport );
		ToolBar->SetRealtimeExpressionPreview( EditorOptions->bRealtimeExpressionViewport );
	}
	if ( PreviewVC )
	{
		PreviewVC->SetShowGrid( bShowGrid );
		PreviewVC->SetRealtime( EditorOptions->bRealtimeMaterialViewport || GEditor->AccessUserSettings().bStartInRealtimeMode );
	}
	if ( LinkedObjVC )
	{
		LinkedObjVC->SetRealtime( EditorOptions->bRealtimeExpressionViewport );
	}

	// Primitive type
	INT PrimType;
	if(GConfig->GetInt(TEXT("MaterialEditor"), TEXT("PrimType"), PrimType, GEditorUserSettingsIni))
	{
		PreviewPrimType = (EThumbnailPrimType)PrimType;
	}
	else
	{
		PreviewPrimType = TPT_Sphere;
	}

}

/**
 * Saves editor settings to disk (docking state, window pos/size, option state, etc).
 */
void WxMaterialEditor::SaveEditorSettings()
{
	// Save docking layout.
	SaveDockingLayout();

	// Save window position/size.
	FWindowUtil::SavePosSize(TEXT("MaterialEditor"), this);

	if ( EditorOptions )
	{
		check( PreviewVC );
		check( LinkedObjVC );
		EditorOptions->bShowGrid					= bShowGrid;
		EditorOptions->bShowBackground				= bShowBackground;
		EditorOptions->bHideUnusedConnectors		= bHideUnusedConnectors;
		EditorOptions->bAlwaysRefreshAllPreviews	= bAlwaysRefreshAllPreviews;
		EditorOptions->bRealtimeMaterialViewport	= PreviewVC->IsRealtime();
		EditorOptions->bRealtimeExpressionViewport	= LinkedObjVC->IsRealtime();
		EditorOptions->SaveConfig();
	}

	GConfig->SetInt(TEXT("MaterialEditor"), TEXT("PrimType"), PreviewPrimType, GEditorUserSettingsIni);

	//Material->PreviewCamPos = PreviewVC->ViewLocation;
	//Material->EditorPitch = PreviewVC->ViewRotation.Pitch;
	//Material->EditorYaw = PreviewVC->ViewRotation.Yaw;
}

/**
 * Called by SetPreviewMesh, allows derived types to veto the setting of a preview mesh.
 *
 * @return	TRUE if the specified mesh can be set as the preview mesh, FALSE otherwise.
 */
UBOOL WxMaterialEditor::ApproveSetPreviewMesh(UStaticMesh* InStaticMesh, USkeletalMesh* InSkeletalMesh)
{
	UBOOL bApproved = TRUE;
	// Only permit the use of a skeletal mesh if the material has bUsedWithSkeltalMesh.
	if ( InSkeletalMesh && !Material->bUsedWithSkeletalMesh )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MaterialEditor_CantPreviewOnSkelMesh") );
		bApproved = FALSE;
	}
	return bApproved;
}

/**
 * Empties compound info.  If bRefresh is TRUE, all information is regenerated.
 */
void WxMaterialEditor::FlushCompoundExpressionInfo(UBOOL bRefresh)
{
	for( CompoundInfoMapType::TIterator It(CompoundInfoMap) ; It ; ++It )
	{
		FCompoundInfo* Info = It.Value();
		delete Info;
	}
	CompoundInfoMap.Empty();

	if ( bRefresh )
	{
		for( INT CompoundIndex = 0 ; CompoundIndex < Material->EditorCompounds.Num() ; ++CompoundIndex )
		{
			UMaterialExpressionCompound* Compound = Material->EditorCompounds(CompoundIndex);
			UpdateCompoundExpressionInfo( Compound );
		}
	}
}

/** Refreshes the viewport containing the material expression graph. */
void WxMaterialEditor::RefreshExpressionViewport()
{
	LinkedObjVC->Viewport->Invalidate();
}

/**
 * Refreshes the preview for the specified material expression.  Does nothing if the specified expression
 * has a bRealtimePreview of FALSE.
 *
 * @param	MaterialExpression		The material expression to update.
 */
void WxMaterialEditor::RefreshExpressionPreview(UMaterialExpression* MaterialExpression, UBOOL bRecompile)
{
	if ( MaterialExpression->bRealtimePreview || MaterialExpression->bNeedToUpdatePreview )
	{
		for( INT PreviewIndex = 0 ; PreviewIndex < ExpressionPreviews.Num() ; ++PreviewIndex )
		{
			FExpressionPreview& ExpressionPreview = ExpressionPreviews(PreviewIndex);
			if( ExpressionPreview.GetExpression() == MaterialExpression )
			{
				// we need to make sure the rendering thread isn't drawing this tile
				FlushRenderingCommands();
				ExpressionPreviews.Remove( PreviewIndex );
				MaterialExpression->bNeedToUpdatePreview = FALSE;
				if (bRecompile)
				{
					UBOOL bNewlyCreated;
					GetExpressionPreview(MaterialExpression, bNewlyCreated, FALSE);
				}
				break;
			}
		}
	}
}

/**
 * Refreshes material expression previews.  Refreshes all previews if bAlwaysRefreshAllPreviews is TRUE.
 * Otherwise, refreshes only those previews that have a bRealtimePreview of TRUE.
 */
void WxMaterialEditor::RefreshExpressionPreviews()
{
	const FScopedBusyCursor BusyCursor;

	if ( bAlwaysRefreshAllPreviews )
	{
		// we need to make sure the rendering thread isn't drawing these tiles
		FlushRenderingCommands();

		// Refresh all expression previews.
		ExpressionPreviews.Empty();
	}
	else
	{
		// Only refresh expressions that are marked for realtime update.
		for ( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
			RefreshExpressionPreview( MaterialExpression, FALSE );
		}
	}

	TArray<FExpressionPreview*> ExpressionPreviewsBeingCompiled;
	ExpressionPreviewsBeingCompiled.Empty(50);
	// Go through all expression previews and create new ones as needed, and maintain a list of previews that are being compiled
	for( INT ExpressionIndex = 0; ExpressionIndex < Material->Expressions.Num(); ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
		if ( !MaterialExpression->IsA(UMaterialExpressionComment::StaticClass()) )
		{
			UBOOL bNewlyCreated;
			FExpressionPreview* Preview = GetExpressionPreview( MaterialExpression, bNewlyCreated, TRUE );
			if (bNewlyCreated && Preview)
			{
				ExpressionPreviewsBeingCompiled.AddItem(Preview);
			}
		}
	}

	GShaderCompilingThreadManager->FinishDeferredCompilation();
}

/**
 * Refreshes all material expression previews, regardless of whether or not realtime previews are enabled.
 */
void WxMaterialEditor::ForceRefreshExpressionPreviews()
{
	// Initialize expression previews.
	const UBOOL bOldAlwaysRefreshAllPreviews = bAlwaysRefreshAllPreviews;
	bAlwaysRefreshAllPreviews = TRUE;
	RefreshExpressionPreviews();
	bAlwaysRefreshAllPreviews = bOldAlwaysRefreshAllPreviews;
}

/**
 * @param		InMaterialExpression	The material expression to query.
 * @param		ConnType				Type of the connection (LOC_INPUT or LOC_OUTPUT).
 * @param		ConnIndex				Index of the connection to query
 * @return								A viewport location for the connection.
 */
FIntPoint WxMaterialEditor::GetExpressionConnectionLocation(UMaterialExpression* InMaterialExpression, INT ConnType, INT ConnIndex)
{
	const FMaterialNodeDrawInfo& ExpressionDrawInfo = GetDrawInfo( InMaterialExpression );

	FIntPoint Result(0,0);
	if ( ConnType == LOC_OUTPUT ) // connectors on the right side of the material
	{
		Result.X = InMaterialExpression->MaterialExpressionEditorX + ExpressionDrawInfo.DrawWidth + LO_CONNECTOR_LENGTH;
		Result.Y = ExpressionDrawInfo.RightYs(ConnIndex);
	}
	else if ( ConnType == LOC_INPUT ) // connectors on the left side of the material
	{
		Result.X = InMaterialExpression->MaterialExpressionEditorX - LO_CONNECTOR_LENGTH,
		Result.Y = ExpressionDrawInfo.LeftYs(ConnIndex);
	}
	return Result;
}

/**
 * @param		InMaterial	The material to query.
 * @param		ConnType	Type of the connection (LOC_OUTPUT).
 * @param		ConnIndex	Index of the connection to query
 * @return					A viewport location for the connection.
 */
FIntPoint WxMaterialEditor::GetMaterialConnectionLocation(UMaterial* InMaterial, INT ConnType, INT ConnIndex)
{
	FIntPoint Result(0,0);
	if ( ConnType == LOC_OUTPUT ) // connectors on the right side of the material
	{
		Result.X = InMaterial->EditorX + MaterialDrawInfo.DrawWidth + LO_CONNECTOR_LENGTH;
		Result.Y = MaterialDrawInfo.RightYs( ConnIndex );
	}
	return Result;
}

/**
 * Returns the expression preview for the specified material expression.
 */
FExpressionPreview* WxMaterialEditor::GetExpressionPreview(UMaterialExpression* MaterialExpression, UBOOL& bNewlyCreated, UBOOL bDeferCompilation)
{
	bNewlyCreated = FALSE;
	if (MaterialExpression->bHidePreviewWindow == FALSE)
	{
		FExpressionPreview*	Preview = NULL;
		for( INT PreviewIndex = 0 ; PreviewIndex < ExpressionPreviews.Num() ; ++PreviewIndex )
		{
			FExpressionPreview& ExpressionPreview = ExpressionPreviews(PreviewIndex);
			if( ExpressionPreview.GetExpression() == MaterialExpression )
			{
				Preview = &ExpressionPreviews(PreviewIndex);
				break;
			}
		}

		if( !Preview )
		{
			bNewlyCreated = TRUE;
			Preview = new(ExpressionPreviews) FExpressionPreview(MaterialExpression);
			if (!bDeferCompilation)
			{
				Preview->bDeferCompilation = FALSE;
			}
			Preview->CacheShaders();
			if (!bDeferCompilation)
			{
				Preview->bDeferCompilation = TRUE;
			}
		}
		return Preview;
	}

	return NULL;
}

/**
* Returns the drawinfo object for the specified expression, creating it if one does not currently exist.
*/
WxMaterialEditor::FMaterialNodeDrawInfo& WxMaterialEditor::GetDrawInfo(UMaterialExpression* MaterialExpression)
{
	FMaterialNodeDrawInfo* ExpressionDrawInfo = MaterialNodeDrawInfo.Find( MaterialExpression );
	return ExpressionDrawInfo ? *ExpressionDrawInfo : MaterialNodeDrawInfo.Set( MaterialExpression, FMaterialNodeDrawInfo(MaterialExpression->MaterialExpressionEditorY) );
}

/**
 * Disconnects and removes the given expressions, comments and compounds.
 */
void WxMaterialEditor::DeleteObjects( const TArray<UMaterialExpression*>& Expressions, const TArray<UMaterialExpressionComment*>& Comments, const TArray<UMaterialExpressionCompound*>& Compounds)
{
	const UBOOL bHaveExpressionsToDelete	= Expressions.Num() > 0;
	const UBOOL bHaveCommentsToDelete		= Comments.Num() > 0;
	const UBOOL bHaveCompoundsToDelete		= Compounds.Num() > 0;
	if ( bHaveExpressionsToDelete || bHaveCommentsToDelete || bHaveCompoundsToDelete )
	{
		// If we are debugging and the expression being debugged was deleted
		UBOOL bDebugExpressionDeleted			= FALSE;

		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorDelete")) );
			Material->Modify();

			// Whack selected expressions.
			for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Expressions.Num() ; ++MaterialExpressionIndex )
			{
				UMaterialExpression* MaterialExpression = Expressions( MaterialExpressionIndex );

#if WITH_MANAGED_CODE
				UnBindColorPickers(MaterialExpression);
#endif

				if( DebugExpression == MaterialExpression )
				{
					// The expression being debugged is also being deleted
					bDebugExpressionDeleted = TRUE;
				}

				MaterialExpression->Modify();
				SwapLinksToExpression(MaterialExpression, NULL, Material);
				Material->Expressions.RemoveItem( MaterialExpression );
				Material->RemoveExpressionParameter(MaterialExpression);
				// Make sure the deleted expression is caught by gc
				MaterialExpression->MarkPendingKill();
			}	

			// Whack selected comments.
			for( INT CommentIndex = 0 ; CommentIndex < Comments.Num() ; ++CommentIndex )
			{
				UMaterialExpressionComment* Comment = Comments( CommentIndex );
				Comment->Modify();
				Material->EditorComments.RemoveItem( Comment );
			}

			// Whack selected compounds.
			for( INT CompoundIndex = 0 ; CompoundIndex < Compounds.Num() ; ++CompoundIndex )
			{
				UMaterialExpressionCompound* Compound = Compounds( CompoundIndex );
				Compound->Modify();
				Material->EditorCompounds.RemoveItem( Compound );
				for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Compound->MaterialExpressions.Num() ; ++MaterialExpressionIndex )
				{
					UMaterialExpression* MaterialExpression = Compound->MaterialExpressions( MaterialExpressionIndex );
					check( MaterialExpression->Compound == Compound );
					MaterialExpression->Modify();
					MaterialExpression->Compound = NULL;
				}
				FCompoundInfo** CompoundInfo = CompoundInfoMap.Find( Compound );
				check( CompoundInfo );
				delete *CompoundInfo;
				CompoundInfoMap.Remove( Compound );
			}
		} // ScopedTransaction

		// Deselect all expressions, comments and compounds.
		EmptySelection();

		if ( bHaveExpressionsToDelete )
		{
			if( bDebugExpressionDeleted )
			{
				// The debug expression was deleted.  Null out our reference to it and reset to the normal preview mateiral
				DebugExpression = NULL;
				SetPreviewMaterial( Material );
			}
			RefreshSourceWindowMaterial();
			UpdateSearch(FALSE);
		}
		UpdatePreviewMaterial();
		Material->MarkPackageDirty();
		bMaterialDirty = TRUE;

		UpdatePropertyWindow();

		if ( bHaveExpressionsToDelete )
		{
			RefreshExpressionPreviews();
		}
		RefreshExpressionViewport();
	}
}

/**
 * Disconnects and removes the selected material expressions.
 */
void WxMaterialEditor::DeleteSelectedObjects()
{
	DeleteObjects(SelectedExpressions, SelectedComments, SelectedCompounds);
}

/**
 * Pastes into this material from the editor's material copy buffer.
 *
 * @param	PasteLocation	If non-NULL locate the upper-left corner of the new nodes' bound at this location.
 */
void WxMaterialEditor::PasteExpressionsIntoMaterial(const FIntPoint* PasteLocation)
{
	if ( GetCopyPasteBuffer()->Expressions.Num() > 0 || GetCopyPasteBuffer()->EditorComments.Num() > 0 )
	{
		// Empty the selection set, as we'll be selecting the newly created expressions.
		EmptySelection();

		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorPaste")) );
			Material->Modify();

			// Copy the expressions in the material copy buffer into this material.
			TArray<UMaterialExpression*> NewExpressions;
			TArray<UMaterialExpression*> NewComments;

			UMaterialExpression::CopyMaterialExpressions( GetCopyPasteBuffer()->Expressions, GetCopyPasteBuffer()->EditorComments, Material, NewExpressions, NewComments );

			// Append the comments list to the expressions list so we can position all of the items at once.
			NewExpressions.Append(NewComments);
				
			// Locate and select the newly created nodes.
			const FIntRect NewExpressionBounds( GetBoundingBoxOfExpressions( NewExpressions ) );
			for ( INT ExpressionIndex = 0 ; ExpressionIndex < NewExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* NewExpression = NewExpressions( ExpressionIndex );
				if ( PasteLocation )
				{
					// We're doing a paste here.
					NewExpression->MaterialExpressionEditorX += -NewExpressionBounds.Min.X + PasteLocation->X;
					NewExpression->MaterialExpressionEditorY += -NewExpressionBounds.Min.Y + PasteLocation->Y;
				}
				else
				{
					// We're doing a duplicate or straight-up paste; offset the nodes by a fixed amount.
					const INT DuplicateOffset = 30;
					NewExpression->MaterialExpressionEditorX += DuplicateOffset;
					NewExpression->MaterialExpressionEditorY += DuplicateOffset;
				}
				AddToSelection( NewExpression );
				Material->AddExpressionParameter(NewExpression);
			}
		}

		// Update the current preview material
		UpdatePreviewMaterial();
		RefreshSourceWindowMaterial();
		UpdateSearch(FALSE);
		Material->MarkPackageDirty();

		UpdatePropertyWindow();

		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Duplicates the selected material expressions.  Preserves internal references.
 */
void WxMaterialEditor::DuplicateSelectedObjects()
{
	// Clear the material copy buffer and copy the selected expressions into it.
	TArray<UMaterialExpression*> NewExpressions;
	TArray<UMaterialExpression*> NewComments;

	GetCopyPasteBuffer()->Expressions.Empty();
	GetCopyPasteBuffer()->EditorComments.Empty();
	UMaterialExpression::CopyMaterialExpressions( SelectedExpressions, SelectedComments, GetCopyPasteBuffer(), NewExpressions, NewComments );

	// Paste the material copy buffer into this material.
	PasteExpressionsIntoMaterial( NULL );
}

/**
 * Deletes any disconnected material expressions.
 */
void WxMaterialEditor::CleanUnusedExpressions()
{
	EmptySelection();

	// The set of material expressions to visit.
	TArray<UMaterialExpression*> Stack;

	// Populate the stack with inputs to the material.
	for ( INT MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs(MaterialInputIndex);
		UMaterialExpression* Expression = MaterialInput.Input->Expression;
		if ( Expression )
		{
			Stack.Push( Expression );
		}
	}

	// Depth-first traverse the material expression graph.
	TArray<UMaterialExpression*>	NewExpressions;
	TMap<UMaterialExpression*, INT> ReachableExpressions;
	while ( Stack.Num() > 0 )
	{
		UMaterialExpression* MaterialExpression = Stack.Pop();
		INT *AlreadyVisited = ReachableExpressions.Find( MaterialExpression );
		if ( !AlreadyVisited )
		{
			// Mark the expression as reachable.
			ReachableExpressions.Set( MaterialExpression, 0 );
			NewExpressions.AddItem( MaterialExpression );

			// Iterate over the expression's inputs and add them to the pending stack.
			const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
			for( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ ExpressionInputIndex )
			{
				FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
				UMaterialExpression* InputExpression = Input->Expression;
				if ( InputExpression )
				{
					Stack.Push( InputExpression );
				}
			}
		}
	}

	// Kill off expressions referenced by the material that aren't reachable.
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorCleanUnusedExpressions")) );
		Material->Modify();
		Material->Expressions = NewExpressions;
	}

	RefreshExpressionViewport();
	bMaterialDirty = TRUE;
}

/** Draws the root node -- that is, the node corresponding to the final material. */
void WxMaterialEditor::DrawMaterialRoot(UBOOL bIsSelected, FCanvas* Canvas)
{
	// Construct the FLinkedObjDrawInfo for use by the linked-obj drawing utils.
	FLinkedObjDrawInfo ObjInfo;
	ObjInfo.ObjObject = Material;

	// Check if we want to pan, and our mouse is over a material input.
	UBOOL bPanMouseOverInput = DblClickObject==Material && DblClickConnType==LOC_OUTPUT;

	const FColor MaterialInputColor( 0, 0, 0 );
	for ( INT MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs(MaterialInputIndex);
		const UBOOL bShouldAddInputConnector = !bHideUnusedConnectors || MaterialInput.Input->Expression;
		if ( bShouldAddInputConnector )
		{
			if( bPanMouseOverInput && MaterialInput.Input->Expression && DblClickConnIndex==ObjInfo.Outputs.Num() )
			{
				PanLocationOnscreen( MaterialInput.Input->Expression->MaterialExpressionEditorX+50, MaterialInput.Input->Expression->MaterialExpressionEditorY+50, 100 );
			}
			ObjInfo.Outputs.AddItem( FLinkedObjConnInfo(MaterialInput.Name, MaterialInputColor) );
		}
	}

	// Generate border color
	const FColor BorderColor( bIsSelected ? FColor( 255, 255, 0 ) : FColor( 0, 0, 0 ) );

	// Highlight the currently moused over connector
	HighlightActiveConnectors( ObjInfo );

	// Use util to draw box with connectors, etc.
	const FIntPoint MaterialPos( Material->EditorX, Material->EditorY );
	FLinkedObjDrawUtils::DrawLinkedObj( Canvas, ObjInfo, *Material->GetName(), NULL, BorderColor, FColor(112,112,112), MaterialPos );

	// Copy off connector values for use
	MaterialDrawInfo.DrawWidth	= ObjInfo.DrawWidth;
	MaterialDrawInfo.RightYs	= ObjInfo.OutputY;
	check( bHideUnusedConnectors || MaterialDrawInfo.RightYs.Num() == MaterialInputs.Num() );
}

/**
 * Render connections between the material's inputs and the material expression outputs connected to them.
 */
void WxMaterialEditor::DrawMaterialRootConnections(FCanvas* Canvas)
{
	// Compensates for the difference between the number of rendered inputs
	// (based on bHideUnusedConnectors) and the true number of inputs.
	INT ActiveInputCounter = -1;

	TArray<FExpressionInput*> ReferencingInputs;
	for ( INT MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs(MaterialInputIndex);
		UMaterialExpression* MaterialExpression = MaterialInput.Input->Expression;
		if ( MaterialExpression )
		{
			++ActiveInputCounter;

			TArray<FExpressionOutput>	Outputs;
			MaterialExpression->GetOutputs(Outputs);

			INT ActiveOutputCounter = -1;
			INT OutputIndex = 0;
			for( ; OutputIndex < Outputs.Num() ; ++OutputIndex )
			{
				const FExpressionOutput& Output = Outputs(OutputIndex);

				// If unused connectors are hidden, the material expression output index needs to be transformed
				// to visible index rather than absolute.
				if ( bHideUnusedConnectors )
				{
					// Get a list of inputs that refer to the selected output.
					GetListOfReferencingInputs( MaterialExpression, Material, ReferencingInputs, &Output );
					if ( ReferencingInputs.Num() > 0 )
					{
						++ActiveOutputCounter;
					}
				}

				if(	Output.Mask == MaterialInput.Input->Mask
					&& Output.MaskR == MaterialInput.Input->MaskR
					&& Output.MaskG == MaterialInput.Input->MaskG
					&& Output.MaskB == MaterialInput.Input->MaskB
					&& Output.MaskA == MaterialInput.Input->MaskA )
				{
					break;
				}
			}
			check( OutputIndex < Outputs.Num() );

			const INT MaterialInputLookupIndex			= bHideUnusedConnectors ? ActiveInputCounter : MaterialInputIndex;
			const FIntPoint Start( GetMaterialConnectionLocation(Material,LOC_OUTPUT,MaterialInputLookupIndex) );

			const INT ExpressionOutputLookupIndex		= bHideUnusedConnectors ? ActiveOutputCounter : OutputIndex;
			const FIntPoint End( GetExpressionConnectionLocation(MaterialExpression,LOC_INPUT,ExpressionOutputLookupIndex) );

			// If either of the connection ends are highlighted then highlight the line.
			FColor Color;

			if(IsConnectorHighlighted(Material, LOC_OUTPUT, MaterialInputLookupIndex) || IsConnectorHighlighted(MaterialExpression, LOC_INPUT, ExpressionOutputLookupIndex))
			{
				Color = ConnectionSelectedColor;
			}
			else
			{
				Color = ConnectionNormalColor;
			}

			// DrawCurves
			{
				const FLOAT Tension = Abs<INT>( Start.X - End.X );
				FLinkedObjDrawUtils::DrawSpline( Canvas, End, -Tension*FVector2D(1,0), Start, -Tension*FVector2D(1,0), Color, TRUE );
			}
		}
	}
}

void WxMaterialEditor::DrawMaterialExpressionLinkedObj(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const TCHAR* Name, const TCHAR* Comment, const FColor& BorderColor, const FColor& TitleBkgColor, UMaterialExpression* MaterialExpression, UBOOL bRenderPreview)
{
	static const INT ExpressionPreviewSize = 96;
	static const INT ExpressionPreviewBorder = 1;
	//static const INT ExpressionPreviewSize = ExpressionPreviewBaseSize + ExpressionPreviewBorder;
	
	// If an expression is currently being debugged
	const UBOOL bDebugging = (MaterialExpression == DebugExpression);

	const FIntPoint Pos( MaterialExpression->MaterialExpressionEditorX, MaterialExpression->MaterialExpressionEditorY );
#if 0
	FLinkedObjDrawUtils::DrawLinkedObj( Canvas, ObjInfo, Name, Comment, BorderColor, TitleBkgColor, Pos );
#else
	const UBOOL bHitTesting = Canvas->IsHitTesting();

	const FIntPoint TitleSize	= FLinkedObjDrawUtils::GetTitleBarSize(Canvas, Name);
	const FIntPoint LogicSize	= FLinkedObjDrawUtils::GetLogicConnectorsSize(ObjInfo);

	// Includes one-pixel border on left and right and a one-pixel border between the preview icon and the title text.
	ObjInfo.DrawWidth	= 2 + 1 + PreviewIconWidth + Max(Max(TitleSize.X, LogicSize.X), ExpressionPreviewSize+2*ExpressionPreviewBorder);
	const INT BodyHeight = 2 + Max(LogicSize.Y, ExpressionPreviewSize+2*ExpressionPreviewBorder);

	// Includes one-pixel spacer between title and body.
	ObjInfo.DrawHeight	= TitleSize.Y + 1 + BodyHeight;

	if(bHitTesting) Canvas->SetHitProxy( new HLinkedObjProxy(ObjInfo.ObjObject) );

	if( bDebugging )
	{
		// Draw a red box on top of the normal title bar that indicates we are currently debugging this node
		FLinkedObjDrawUtils::DrawTitleBar( Canvas, FIntPoint(Pos.X, Pos.Y - TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, TitleSize.Y), BorderColor, FColor(200,0,0), TEXT("Previewing") );
	}

	FLinkedObjDrawUtils::DrawTitleBar(Canvas, Pos, FIntPoint(ObjInfo.DrawWidth, TitleSize.Y), BorderColor, TitleBkgColor, Name, Comment);

	FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X,		Pos.Y + TitleSize.Y + 1,	ObjInfo.DrawWidth,		BodyHeight,		0.0f,0.0f,0.0f,0.0f, BorderColor );
	FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X + 1,	Pos.Y + TitleSize.Y + 2,	ObjInfo.DrawWidth - 2,	BodyHeight-2,	0.0f,0.0f,0.0f,0.0f, FColor(140,140,140) );

	if ( bRenderPreview )
	{
		UBOOL bNewlyCreated;
		FExpressionPreview* ExpressionPreview = GetExpressionPreview( MaterialExpression, bNewlyCreated, FALSE);
		FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X + 1 + ExpressionPreviewBorder,	Pos.Y + TitleSize.Y + 2 + ExpressionPreviewBorder,	ExpressionPreviewSize,	ExpressionPreviewSize,	0.0f,0.0f,1.0f,1.0f, ExpressionPreview );
	}

	if(bHitTesting) Canvas->SetHitProxy( NULL );

	// Highlight the currently moused over connector
	HighlightActiveConnectors( ObjInfo );

	//const FLinearColor ConnectorTileBackgroundColor( 0.f, 0.f, 0.f, 0.5f );
	FLinkedObjDrawUtils::DrawLogicConnectors(Canvas, ObjInfo, Pos + FIntPoint(0, TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, LogicSize.Y), NULL);//&ConnectorTileBackgroundColor);
#endif
}

/**
 * Draws messages on the specified viewport and canvas.
 */
void WxMaterialEditor::DrawMessages( FViewport* Viewport, FCanvas* Canvas )
{
	if( DebugExpression != NULL )
	{
		// If there is a debug expression then we are debugging 
		Canvas->PushAbsoluteTransform( FMatrix::Identity );

		// The message to display in the viewport.
		FString Name = FString::Printf( TEXT("Previewing: %s"), *DebugExpression->GetName() );

		// Size of the tile we are about to draw.  Should extend the length of the view in X.
		const FIntPoint TileSize( Viewport->GetSizeX(), 25);
		
		const FColor DebugColor( 200,0,0 );
		const FColor FontColor( 255,255,128 );

		UFont* FontToUse = GEditor->EditorFont;
		
		DrawTile( Canvas, 0.0f, 0.0f, TileSize.X, TileSize.Y, 0.0f, 0.0f, 0.0f, 0.0f, DebugColor );

		INT XL, YL;
		StringSize( FontToUse, XL, YL, *Name );
		if( XL > TileSize.X )
		{
			// There isn't enough room to show the debug expression name
			Name = TEXT("Previewing");
			StringSize( FontToUse, XL, YL, *Name );
		}
		
		// Center the string in the middle of the tile.
		const FIntPoint StringPos( (TileSize.X-XL)/2, ((TileSize.Y-YL)/2)+1 );
		// Draw the preview message
		DrawShadowedString( Canvas, StringPos.X, StringPos.Y, *Name, FontToUse, FontColor );

		Canvas->PopTransform();
	}
}

/**
 * Called when the user double-clicks an object in the viewport
 *
 * @param	Obj		the object that was double-clicked on
 */
void WxMaterialEditor::DoubleClickedObject(UObject* Obj)
{
	check(Obj);

	UMaterialExpressionConstant3Vector* Constant3Expression = Cast<UMaterialExpressionConstant3Vector>(Obj);
	UMaterialExpressionConstant4Vector* Constant4Expression = Cast<UMaterialExpressionConstant4Vector>(Obj);

	FColorChannelStruct ChannelEditStruct;
	if (Constant3Expression)
	{
		ChannelEditStruct.Red = &Constant3Expression->R;
		ChannelEditStruct.Green = &Constant3Expression->G;
		ChannelEditStruct.Blue = &Constant3Expression->B;
	}
	else if (Constant4Expression)
	{
		ChannelEditStruct.Red = &Constant4Expression->R;
		ChannelEditStruct.Green = &Constant4Expression->G;
		ChannelEditStruct.Blue = &Constant4Expression->B;
		ChannelEditStruct.Alpha = &Constant4Expression->A;
	}

	//pass in one of the child property nodes, to force the previews to update
	FPropertyNode* NotifyNode = PropertyWindow->FindPropertyNode(TEXT("R"));
	if (ChannelEditStruct.Red || ChannelEditStruct.Green || ChannelEditStruct.Blue || ChannelEditStruct.Alpha)
	{
		FPickColorStruct PickColorStruct;
		PickColorStruct.RefreshWindows.AddItem(this);
		PickColorStruct.PropertyWindow = PropertyWindow->GetPropertyWindowForCallbacks();
		PickColorStruct.PropertyNode = NotifyNode;
		PickColorStruct.PartialFLOATColorArray.AddItem(ChannelEditStruct);
		PickColorStruct.ParentObjects.AddItem(Obj);
		PickColorStruct.bSendEventsOnlyOnMouseUp = TRUE;

		PickColor(PickColorStruct);

	}
}

/**
* Called when double-clicking a connector.
* Used to pan the connection's link into view
*
* @param	Connector	The connector that was double-clicked
*/
void WxMaterialEditor::DoubleClickedConnector(FLinkedObjectConnector& Connector)
{
	DblClickObject = Connector.ConnObj;
	DblClickConnType = Connector.ConnType;
	DblClickConnIndex = Connector.ConnIndex;
}


/** Draws the specified material expression node. */
void WxMaterialEditor::DrawMaterialExpression(UMaterialExpression* MaterialExpression, UBOOL bExpressionSelected, FCanvas* Canvas)
{
	// Don't render the expression if it is contained by a compound that is visible.
	const UBOOL bCompoundVisible = MaterialExpression->Compound && !MaterialExpression->Compound->bExpanded;
	if ( bCompoundVisible )
	{
		return;
	}

	// Construct the FLinkedObjDrawInfo for use by the linked-obj drawing utils.
	FLinkedObjDrawInfo ObjInfo;
	ObjInfo.ObjObject = MaterialExpression;

	// Check if we want to pan, and our mouse is over an input for this expression.
	UBOOL bPanMouseOverInput = DblClickObject==MaterialExpression && DblClickConnType==LOC_OUTPUT;

	// Material expression inputs, drawn on the right side of the node.
	const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
	for( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ ExpressionInputIndex )
	{
		FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
		UMaterialExpression* InputExpression = Input->Expression;
		const UBOOL bShouldAddInputConnector = !bHideUnusedConnectors || InputExpression;
		if ( bShouldAddInputConnector )
		{
			if( bPanMouseOverInput && Input->Expression && DblClickConnIndex==ObjInfo.Outputs.Num() )
			{
				PanLocationOnscreen( Input->Expression->MaterialExpressionEditorX+50, Input->Expression->MaterialExpressionEditorY+50, 100 );
			}

			FString InputName = MaterialExpression->GetInputName(ExpressionInputIndex);
			// Shorten long expression input names.
			if ( !appStricmp( *InputName, TEXT("Coordinates") ) )
			{
				InputName = TEXT("UVs");
			}
			else if ( !appStricmp( *InputName, TEXT("Input") ) )
			{
				InputName = TEXT("");
			}
			else if ( !appStricmp( *InputName, TEXT("Exponent") ) )
			{
				InputName = TEXT("Exp");
			}
			else if ( !appStricmp( *InputName, TEXT("AGreaterThanB") ) )
			{
				InputName = TEXT("A>B");
			}
			else if ( !appStricmp( *InputName, TEXT("AEqualsB") ) )
			{
				InputName = TEXT("A=B");
			}
			else if ( !appStricmp( *InputName, TEXT("ALessThanB") ) )
			{
				InputName = TEXT("A<B");
			}
			ObjInfo.Outputs.AddItem( FLinkedObjConnInfo(*InputName, FColor(0,0,0)) );
		}
	}

	// Material expression outputs, drawn on the left side of the node
	TArray<FExpressionOutput> Outputs;
	MaterialExpression->GetOutputs(Outputs);

	// Check if we want to pan, and our mouse is over an output for this expression.
	UBOOL bPanMouseOverOutput = DblClickObject==MaterialExpression && DblClickConnType==LOC_INPUT;

	TArray<FExpressionInput*> ReferencingInputs;
	for( INT OutputIndex = 0 ; OutputIndex < Outputs.Num() ; ++OutputIndex )
	{
		const FExpressionOutput& ExpressionOutput = Outputs(OutputIndex);
		UBOOL bShouldAddOutputConnector = TRUE;
		if ( bHideUnusedConnectors )
		{
			// Get a list of inputs that refer to the selected output.
			GetListOfReferencingInputs( MaterialExpression, Material, ReferencingInputs, &ExpressionOutput );
			bShouldAddOutputConnector = ReferencingInputs.Num() > 0;
		}

		if ( bShouldAddOutputConnector )
		{
			const TCHAR* OutputName = TEXT("");
			FColor OutputColor( 0, 0, 0 );

			if( ExpressionOutput.Mask )
			{
				if		( ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// R
					OutputColor = FColor( 255, 0, 0 );
					//OutputName = TEXT("R");
				}
				else if	(!ExpressionOutput.MaskR &&  ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// G
					OutputColor = FColor( 0, 255, 0 );
					//OutputName = TEXT("G");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG &&  ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// B
					OutputColor = FColor( 0, 0, 255 );
					//OutputName = TEXT("B");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB &&  ExpressionOutput.MaskA)
				{
					// A
					OutputColor = FColor( 255, 255, 255 );
					//OutputName = TEXT("A");
				}
				else
				{
					// RGBA
					//OutputName = TEXT("RGBA");
				}
			}

			if (MaterialExpression->bShowOutputNameOnPin == TRUE)
			{
				OutputName = *(ExpressionOutput.Name);
			}

			// If this is the output we're hovering over, pan its first connection into view.
			if( bPanMouseOverOutput && DblClickConnIndex==ObjInfo.Inputs.Num() )
			{
				// Find what this output links to.
				TArray<FMaterialExpressionReference> References;
				GetListOfReferencingInputs(MaterialExpression, Material, References, &ExpressionOutput);
				if( References.Num() > 0 )
				{
					if( References(0).Expression == NULL )
					{
						// connects to the root node
						PanLocationOnscreen( Material->EditorX+50, Material->EditorY+50, 100 );
					}
					else
					{
						PanLocationOnscreen( References(0).Expression->MaterialExpressionEditorX+50, References(0).Expression->MaterialExpressionEditorY+50, 100 );

					}
				}
			}

			// We use the "Inputs" array so that the connectors are drawn on the left side of the node.
			ObjInfo.Inputs.AddItem( FLinkedObjConnInfo(OutputName, OutputColor) );
		}
	}

	// Determine the texture dependency length for the material and the expression.
	const FMaterialResource* MaterialResource = Material->GetMaterialResource();
	INT MaxTextureDependencyLength = MaterialResource->GetMaxTextureDependencyLength();
	const INT* TextureDependencyLength = MaterialResource->GetTextureDependencyLengthMap().Find(MaterialExpression);

	UBOOL bCurrentSearchResult = SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() && SearchResults(SelectedSearchResult) == MaterialExpression;

	// Generate border color
	FColor BorderColor;
	if(bCurrentSearchResult)
	{
		BorderColor = FColor( 128, 255, 128 );
	}
	else if(bExpressionSelected)
	{
		BorderColor = FColor( 255, 255, 0 );
	}
	else if( DebugExpression == MaterialExpression )
	{
		// If we are currently debugging a node, its border should be red.
		BorderColor = FColor( 200, 0, 0 );
	}
	else if(TextureDependencyLength && *TextureDependencyLength == MaxTextureDependencyLength && MaxTextureDependencyLength > 1)
	{
		BorderColor = FColor( 255, 0, 255 );
	}
	else if(UMaterial::IsParameter(MaterialExpression))
	{
		if(Material->HasDuplicateParameters(MaterialExpression))
		{
			BorderColor = FColor( 0, 255, 255 );
		}
		else
		{
			BorderColor = FColor( 0, 128, 128 );
		}
	}
	else if (UMaterial::IsDynamicParameter(MaterialExpression))
	{
		if (Material->HasDuplicateDynamicParameters(MaterialExpression))
		{
			BorderColor = FColor( 0, 255, 255 );
		}
		else
		{
			BorderColor = FColor( 0, 128, 128 );
		}
	}
	else
	{
		BorderColor = FColor( 0, 0, 0 );
	}

	// Use util to draw box with connectors, etc.
	DrawMaterialExpressionLinkedObj( Canvas, ObjInfo, *MaterialExpression->GetCaption(), NULL, BorderColor, bCurrentSearchResult ? BorderColor : FColor(112,112,112), MaterialExpression, !(MaterialExpression->bHidePreviewWindow) );

	// Read back the height of the first connector on the left side of the node,
	// for use later when drawing connections to this node.
	FMaterialNodeDrawInfo& ExpressionDrawInfo	= GetDrawInfo( MaterialExpression );
	ExpressionDrawInfo.LeftYs					= ObjInfo.InputY;
	ExpressionDrawInfo.RightYs					= ObjInfo.OutputY;
	ExpressionDrawInfo.DrawWidth				= ObjInfo.DrawWidth;

	check( bHideUnusedConnectors || ExpressionDrawInfo.RightYs.Num() == ExpressionInputs.Num() );

	// Draw realtime preview indicator above the node.
	if (MaterialExpression->bHidePreviewWindow == FALSE)
	{
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X, MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y, PreviewIconWidth, PreviewIconWidth ) )
		{
			const UBOOL bHitTesting = Canvas->IsHitTesting();
			if( bHitTesting )  Canvas->SetHitProxy( new HRealtimePreviewProxy( MaterialExpression ) );

			// Draw black background icon.
			FLinkedObjDrawUtils::DrawTile( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X,		MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y, PreviewIconWidth,	PreviewIconWidth, 0.f, 0.f, 1.f, 1.f, FColor(0,0,0) );

			// Draw yellow fill if realtime preview is enabled for this node.
			if( MaterialExpression->bRealtimePreview )
			{
				FLinkedObjDrawUtils::DrawTile( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X+1,	MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y+1, PreviewIconWidth-2,	PreviewIconWidth-2,	0.f, 0.f, 1.f, 1.f, FColor(255,215,0) );
			}

			// Draw a small red icon above the node if realtime preview is enabled for all nodes.
			if ( bAlwaysRefreshAllPreviews )
			{
				FLinkedObjDrawUtils::DrawTile( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X+2,	MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y+2, PreviewIconWidth-4,	PreviewIconWidth-4,	0.f, 0.f, 1.f, 1.f, FColor(255,0,0) );
			}
			if( bHitTesting )  Canvas->SetHitProxy( NULL );
		}
	}
}

/**
 * Render connectors between this material expression's inputs and the material expression outputs connected to them.
 */
void WxMaterialEditor::DrawMaterialExpressionConnections(UMaterialExpression* MaterialExpression, FCanvas* Canvas)
{
	// Compensates for the difference between the number of rendered inputs
	// (based on bHideUnusedConnectors) and the true number of inputs.
	INT ActiveInputCounter = -1;

	TArray<FExpressionInput*> ReferencingInputs;

	const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
	for( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ ExpressionInputIndex )
	{
		FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
		UMaterialExpression* InputExpression = Input->Expression;
		if ( InputExpression )
		{
			++ActiveInputCounter;

			// Find the matching output.
			TArray<FExpressionOutput> Outputs;
			InputExpression->GetOutputs(Outputs);
			INT ActiveOutputCounter = -1;
			INT OutputIndex = 0;
			for( ; OutputIndex < Outputs.Num() ; ++OutputIndex )
			{
				const FExpressionOutput& Output = Outputs(OutputIndex);

				// If unused connectors are hidden, the material expression output index needs to be transformed
				// to visible index rather than absolute.
				if ( bHideUnusedConnectors )
				{
					// Get a list of inputs that refer to the selected output.
					GetListOfReferencingInputs( InputExpression, Material, ReferencingInputs, &Output );
					if ( ReferencingInputs.Num() > 0 )
					{
						++ActiveOutputCounter;
					}
				}

				if(	Output.Mask == Input->Mask
					&& Output.MaskR == Input->MaskR
					&& Output.MaskG == Input->MaskG
					&& Output.MaskB == Input->MaskB
					&& Output.MaskA == Input->MaskA )
				{
					break;
				}
			}
			check( OutputIndex < Outputs.Num() );

			const INT ExpressionInputLookupIndex		= bHideUnusedConnectors ? ActiveInputCounter : ExpressionInputIndex;
			const FIntPoint Start( GetExpressionConnectionLocation(MaterialExpression,LOC_OUTPUT,ExpressionInputLookupIndex) );
			const INT ExpressionOutputLookupIndex		= bHideUnusedConnectors ? ActiveOutputCounter : OutputIndex;
			const FIntPoint End( GetExpressionConnectionLocation(InputExpression,LOC_INPUT,ExpressionOutputLookupIndex) );


			// If either of the connection ends are highlighted then highlight the line.
			FColor Color;

			if(IsConnectorHighlighted(MaterialExpression, LOC_OUTPUT, ExpressionInputLookupIndex) || IsConnectorHighlighted(InputExpression, LOC_INPUT, ExpressionOutputLookupIndex))
			{
				Color = ConnectionSelectedColor;
			}
			else
			{
				Color = ConnectionNormalColor;
			}
			
			// DrawCurves
			{
				const FLOAT Tension = Abs<INT>( Start.X - End.X );
				FLinkedObjDrawUtils::DrawSpline( Canvas, End, -Tension*FVector2D(1,0), Start, -Tension*FVector2D(1,0), Color, TRUE );
			}
		}
	}
}

/** Draws comments for the specified material expression node. */
void WxMaterialEditor::DrawMaterialExpressionComments(UMaterialExpression* MaterialExpression, FCanvas* Canvas)
{
	// Don't render the expression if it is contained by a compound that is visible.
	const UBOOL bCompoundVisible = MaterialExpression->Compound && !MaterialExpression->Compound->bExpanded;
	if ( bCompoundVisible )
	{
		return;
	}

	// Draw the material expression comment string unzoomed.
	if( MaterialExpression->Desc.Len() > 0 )
	{
		const FLOAT OldZoom2D = FLinkedObjDrawUtils::GetUniformScaleFromMatrix(Canvas->GetFullTransform());

		INT XL, YL;
		StringSize( GEditor->EditorFont, XL, YL, *MaterialExpression->Desc );

		// We always draw comment-box text at normal size (don't scale it as we zoom in and out.)
		const INT x = appTrunc( MaterialExpression->MaterialExpressionEditorX*OldZoom2D );
		const INT y = appTrunc( MaterialExpression->MaterialExpressionEditorY*OldZoom2D - YL );

		// Viewport cull at a zoom of 1.0, because that's what we'll be drawing with.
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, MaterialExpression->MaterialExpressionEditorX, MaterialExpression->MaterialExpressionEditorY - YL, XL, YL ) )
		{
			Canvas->PushRelativeTransform(FScaleMatrix(1.0f / OldZoom2D));
			{
				FLinkedObjDrawUtils::DrawString( Canvas, x, y, *MaterialExpression->Desc, GEditor->EditorFont, FColor(64,64,192) );
			}
			Canvas->PopTransform();
		}
	}
}

/** Draws UMaterialExpressionComment nodes. */
void WxMaterialEditor::DrawCommentExpressions(FCanvas* Canvas)
{
	const UBOOL bHitTesting = Canvas->IsHitTesting();
	for ( INT CommentIndex = 0 ; CommentIndex < Material->EditorComments.Num() ; ++CommentIndex )
	{
		UMaterialExpressionComment* Comment = Material->EditorComments(CommentIndex);

		const UBOOL bSelected = SelectedComments.ContainsItem( Comment );
		const UBOOL bCurrentSearchResult = SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() && SearchResults(SelectedSearchResult) == Comment;

		const FColor FrameColor = bCurrentSearchResult ? FColor( 128, 255, 128 ) : bSelected ? FColor(255,255,0) : FColor(0, 0, 0);

		for(INT i=0; i<1; i++)
		{
			DrawLine2D(Canvas, FVector2D(Comment->MaterialExpressionEditorX,						Comment->MaterialExpressionEditorY + i),					FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX,		Comment->MaterialExpressionEditorY + i),					FrameColor );
			DrawLine2D(Canvas, FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX - i,	Comment->MaterialExpressionEditorY),						FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX - i,	Comment->MaterialExpressionEditorY + Comment->SizeY),		FrameColor );
			DrawLine2D(Canvas, FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX,		Comment->MaterialExpressionEditorY + Comment->SizeY - i),	FVector2D(Comment->MaterialExpressionEditorX,						Comment->MaterialExpressionEditorY + Comment->SizeY - i),	FrameColor );
			DrawLine2D(Canvas, FVector2D(Comment->MaterialExpressionEditorX + i,					Comment->MaterialExpressionEditorY + Comment->SizeY),		FVector2D(Comment->MaterialExpressionEditorX + i,					Comment->MaterialExpressionEditorY - 1),					FrameColor );
		}

		// Draw little sizing triangle in bottom left.
		const INT HandleSize = 16;
		const FIntPoint A(Comment->MaterialExpressionEditorX + Comment->SizeX,				Comment->MaterialExpressionEditorY + Comment->SizeY);
		const FIntPoint B(Comment->MaterialExpressionEditorX + Comment->SizeX,				Comment->MaterialExpressionEditorY + Comment->SizeY - HandleSize);
		const FIntPoint C(Comment->MaterialExpressionEditorX + Comment->SizeX - HandleSize,	Comment->MaterialExpressionEditorY + Comment->SizeY);
		const BYTE TriangleAlpha = (bSelected) ? 255 : 32; // Make it more transparent if comment is not selected.

		if(bHitTesting)  Canvas->SetHitProxy( new HLinkedObjProxySpecial(Comment, 1) );
		DrawTriangle2D( Canvas, A, FVector2D(0,0), B, FVector2D(0,0), C, FVector2D(0,0), FColor(0,0,0,TriangleAlpha) );
		if(bHitTesting)  Canvas->SetHitProxy( NULL );

		// Check there are some valid chars in string. If not - we can't select it! So we force it back to default.
		UBOOL bHasProperChars = FALSE;
		for( INT i = 0 ; i < Comment->Text.Len() ; ++i )
		{
			if ( Comment->Text[i] != ' ' )
			{
				bHasProperChars = TRUE;
				break;
			}
		}
		if ( !bHasProperChars )
		{
			Comment->Text = TEXT("Comment");
		}

		const FLOAT OldZoom2D = FLinkedObjDrawUtils::GetUniformScaleFromMatrix(Canvas->GetFullTransform());

		INT XL, YL;
		StringSize( GEditor->EditorFont, XL, YL, *Comment->Text );

		// We always draw comment-box text at normal size (don't scale it as we zoom in and out.)
		const INT x = appTrunc(Comment->MaterialExpressionEditorX*OldZoom2D + 2);
		const INT y = appTrunc(Comment->MaterialExpressionEditorY*OldZoom2D - YL - 2);

		// Viewport cull at a zoom of 1.0, because that's what we'll be drawing with.
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, Comment->MaterialExpressionEditorX+2, Comment->MaterialExpressionEditorY-YL-2, XL, YL ) )
		{
			Canvas->PushRelativeTransform(FScaleMatrix(1.0f / OldZoom2D));
			{
				// We only set the hit proxy for the comment text.
				if ( bHitTesting ) Canvas->SetHitProxy( new HLinkedObjProxy(Comment) );
				DrawShadowedString(Canvas, x, y, *Comment->Text, GEditor->EditorFont, FColor(64,64,192) );
				if ( bHitTesting ) Canvas->SetHitProxy( NULL );
			}
			Canvas->PopTransform();
		}
	}
}

/**
 * Draws the specified material expression compound node.
 */
void WxMaterialEditor::DrawCompoundExpression(UMaterialExpressionCompound* Compound, UBOOL bSelected, FCanvas* Canvas)
{
	// Construct the FLinkedObjDrawInfo for use by the linked-obj drawing utils.
	FLinkedObjDrawInfo ObjInfo;
	ObjInfo.ObjObject = Compound;

	FCompoundInfo** CompoundInfo = CompoundInfoMap.Find( Compound );
	check( CompoundInfo );
	FCompoundInfo* Info = *CompoundInfo;

	// Material expression inputs, drawn on the right side of the node.
	for ( INT CompoundInputIndex = 0 ; CompoundInputIndex < Info->CompoundInputs.Num() ; ++CompoundInputIndex )
	{
		const FCompoundInput& CompoundInput				= Info->CompoundInputs(CompoundInputIndex);
		UMaterialExpression* MaterialExpression	= CompoundInput.Expression;
		const FExpressionInput* Input					= MaterialExpression->GetInput(CompoundInput.InputIndex);
		const UMaterialExpression* InputExpression		= Input->Expression;
		const UBOOL bShouldAddInputConnector			= !bHideUnusedConnectors || InputExpression;
		if ( bShouldAddInputConnector )
		{
			FString InputName = MaterialExpression->GetInputName(CompoundInput.InputIndex);
			// Shorten long expression input names.
			if ( !appStricmp( *InputName, TEXT("Coordinates") ) )
			{
				InputName = TEXT("UVs");
			}
			else if ( !appStricmp( *InputName, TEXT("Input") ) )
			{
				InputName = TEXT("");
			}
			else if ( !appStricmp( *InputName, TEXT("Exponent") ) )
			{
				InputName = TEXT("Exp");
			}
			ObjInfo.Outputs.AddItem( FLinkedObjConnInfo(*InputName, FColor(0,0,0)) );
		}
	}

	// Material expression outputs, drawn on the left side of the node
	for ( INT CompoundOutputIndex = 0 ; CompoundOutputIndex < Info->CompoundOutputs.Num() ; ++CompoundOutputIndex )
	{
		const FCompoundOutput& Output					= Info->CompoundOutputs(CompoundOutputIndex);
		const UMaterialExpression* MaterialExpression	= Output.Expression;
		const FExpressionOutput& ExpressionOutput		= Output.ExpressionOutput;

		UBOOL bShouldAddOutputConnector					= TRUE;
		if ( bHideUnusedConnectors )
		{
			// Get a list of inputs that refer to the selected output.
			TArray<FExpressionInput*> ReferencingInputs;
			GetListOfReferencingInputs( MaterialExpression, Material, ReferencingInputs, &ExpressionOutput );
			bShouldAddOutputConnector = ReferencingInputs.Num() > 0;
		}

		if ( bShouldAddOutputConnector )
		{
			const TCHAR* OutputName = TEXT("");
			FColor OutputColor( 0, 0, 0 );

			if( ExpressionOutput.Mask )
			{
				if		( ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// R
					OutputColor = FColor( 255, 0, 0 );
					//OutputName = TEXT("R");
				}
				else if	(!ExpressionOutput.MaskR &&  ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// G
					OutputColor = FColor( 0, 255, 0 );
					//OutputName = TEXT("G");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG &&  ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// B
					OutputColor = FColor( 0, 0, 255 );
					//OutputName = TEXT("B");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB &&  ExpressionOutput.MaskA)
				{
					// A
					OutputColor = FColor( 255, 255, 255 );
					//OutputName = TEXT("A");
				}
				else
				{
					// RGBA
					//OutputName = TEXT("RGBA");
				}
			}
			// We use the "Inputs" array so that the connectors are drawn on the left side of the node.
			ObjInfo.Inputs.AddItem( FLinkedObjConnInfo(OutputName, OutputColor) );
		}
	}

	// Generate border color
	const FColor BorderColor( bSelected ? FColor( 255, 255, 0 ) : FColor( 0, 0, 0 ) );

	// Highlight the currently moused over connector
	HighlightActiveConnectors( ObjInfo );

	// Use util to draw box with connectors, etc.
	DrawMaterialExpressionLinkedObj( Canvas, ObjInfo, *Compound->GetCaption(), NULL, BorderColor, FColor(112,112,112), Compound, FALSE );

	// Read back the height of the first connector on the left side of the node,
	// for use later when drawing connections to this node.
	FMaterialNodeDrawInfo& ExpressionDrawInfo	= GetDrawInfo( Compound );
	ExpressionDrawInfo.LeftYs					= ObjInfo.InputY;
	ExpressionDrawInfo.RightYs					= ObjInfo.OutputY;
	ExpressionDrawInfo.DrawWidth				= ObjInfo.DrawWidth;
	check( bHideUnusedConnectors || ExpressionDrawInfo.RightYs.Num() == Info->CompoundInputs.Num() );
}

/**
 * Highlights any active logic connectors in the specified linked object.
 */
void WxMaterialEditor::HighlightActiveConnectors(FLinkedObjDrawInfo &ObjInfo)
{
	const FColor HighlightedColor(255, 255, 0);

	if(LinkedObjVC->MouseOverConnType==LOC_INPUT)
	{
		for(INT InputIdx=0; InputIdx<ObjInfo.Inputs.Num(); InputIdx++)
		{
			if(IsConnectorHighlighted(ObjInfo.ObjObject, LOC_INPUT, InputIdx))
			{
				ObjInfo.Inputs(InputIdx).Color = HighlightedColor;
			}
		}
	}
	else if(LinkedObjVC->MouseOverConnType==LOC_OUTPUT)
	{
		for(INT OutputIdx=0; OutputIdx<ObjInfo.Outputs.Num(); OutputIdx++)
		{
			if(IsConnectorHighlighted(ObjInfo.ObjObject, LOC_OUTPUT, OutputIdx))
			{
				ObjInfo.Outputs(OutputIdx).Color = HighlightedColor;
			}
		}
	}
}

/** @return Returns whether the specified connector is currently highlighted or not. */
UBOOL WxMaterialEditor::IsConnectorHighlighted(UObject* Object, BYTE ConnType, INT ConnIndex)
{
	UBOOL bResult = FALSE;

	if(Object==LinkedObjVC->MouseOverObject && ConnIndex==LinkedObjVC->MouseOverConnIndex && ConnType==LinkedObjVC->MouseOverConnType)
	{
		bResult = TRUE;
	}

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FLinkedObjViewportClient interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WxMaterialEditor::DrawObjects(FViewport* Viewport, FCanvas* Canvas)
{
	if (BackgroundTexture != NULL)
	{
		Clear( Canvas, FColor(161,161,161) );

		Canvas->PushAbsoluteTransform(FMatrix::Identity);

		const INT ViewWidth = LinkedObjVC->Viewport->GetSizeX();
		const INT ViewHeight = LinkedObjVC->Viewport->GetSizeY();

		// draw the texture to the side, stretched vertically
		FLinkedObjDrawUtils::DrawTile( Canvas, ViewWidth - BackgroundTexture->SizeX, 0,
			BackgroundTexture->SizeX, ViewHeight,
			0.f, 0.f,
			1.f, 1.f,
			FLinearColor::White,
			BackgroundTexture->Resource );

		// stretch the left part of the texture to fill the remaining gap
		if (ViewWidth > BackgroundTexture->SizeX)
		{
			FLinkedObjDrawUtils::DrawTile( Canvas, 0, 0,
				ViewWidth - BackgroundTexture->SizeX, ViewHeight,
				0.f, 0.f,
				0.1f, 0.1f,
				FLinearColor::White,
				BackgroundTexture->Resource );
		}

		Canvas->PopTransform();
	}

	// Draw comments.
	DrawCommentExpressions( Canvas );

	// Draw the root node -- that is, the node corresponding to the final material.
	DrawMaterialRoot( FALSE, Canvas );

	// Draw the material expression nodes.
	for( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
		// @hack DB: For some reason, materials that were resaved in gemini have comment expressions in the material's
		// @hack DB: expressions list.  The check below is required to prevent them from rendering as normal expression nodes.
		if ( !MaterialExpression->IsA(UMaterialExpressionComment::StaticClass()) )
		{
			const UBOOL bExpressionSelected = SelectedExpressions.ContainsItem( MaterialExpression );
			DrawMaterialExpression( MaterialExpression, bExpressionSelected, Canvas );
		}
	}
#if 0
	// Draw the compound expressions.
	for( INT ExpressionIndex = 0 ; ExpressionIndex < Material->EditorCompounds.Num() ; ++ExpressionIndex )
	{
		UMaterialExpressionCompound* CompoundExpression = Material->EditorCompounds( ExpressionIndex );
		const UBOOL bExpressionSelected = SelectedCompounds.ContainsItem( CompoundExpression );
		DrawCompoundExpression( CompoundExpression, bExpressionSelected, RI );
	}
#endif
	// Render connections between the material's inputs and the material expression outputs connected to them.
	DrawMaterialRootConnections( Canvas );

	// Render connectors between material expressions' inputs and the material expression outputs connected to them.
	for( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
		const UBOOL bExpressionSelected = SelectedExpressions.ContainsItem( MaterialExpression );
		DrawMaterialExpressionConnections( MaterialExpression, Canvas );
	}

	// Draw the material expression comments.
	for( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
		DrawMaterialExpressionComments( MaterialExpression, Canvas );
	}

	const FMaterialResource* MaterialResource = Material->GetMaterialResource(MSP_BASE);
	
	if( bShowStats)
	{
		INT DrawPositionY = 5;
		
		Canvas->PushAbsoluteTransform(FMatrix::Identity);

		// Only draw material stats if enabled.
		DrawMaterialInfoStrings(Canvas, Material, MaterialResource, DrawPositionY);
	
		Canvas->PopTransform();
	}

	DblClickObject = NULL;
	DblClickConnType = INDEX_NONE;
	DblClickConnIndex = INDEX_NONE;
}

/**
 * Called when the user right-clicks on an empty region of the expression viewport.
 */
void WxMaterialEditor::OpenNewObjectMenu()
{
	// The user has clicked on the background, so clear the last connector object reference so that
	// any newly created material expression node will not be automatically connected to the
	// connector last clicked upon.
	ConnObj = NULL;

	WxMaterialEditorContextMenu_NewNode Menu( this );
	FTrackPopupMenu tpm( this, &Menu );
	tpm.Show();
}


/**
 * Called when the user right-clicks on an object in the viewport.
 */
void WxMaterialEditor::OpenObjectOptionsMenu()
{
	WxMaterialEditorContextMenu_NodeOptions Menu( this );
	FTrackPopupMenu tpm( this, &Menu );
	tpm.Show();
}

/**
 * Called when the user right-clicks on an object connector in the viewport.
 */
void WxMaterialEditor::OpenConnectorOptionsMenu()
{
	WxMaterialEditorContextMenu_ConnectorOptions Menu( this );
	FTrackPopupMenu tpm( this, &Menu );
	tpm.Show();
}



/**
 * Updates the editor's property window to contain material expression properties if any are selected.
 * Otherwise, properties for the material are displayed.
 */
void WxMaterialEditor::UpdatePropertyWindow()
{
	if( SelectedExpressions.Num() == 0 && SelectedCompounds.Num() == 0 && SelectedComments.Num() == 0 )
	{
		PropertyWindow->SetObject( Material, EPropertyWindowFlags::ShouldShowCategories | EPropertyWindowFlags::Sorted);
	}
	else
	{
		TArray<UMaterialExpression*> SelectedObjects;

		// Expressions
		for ( INT Idx = 0 ; Idx < SelectedExpressions.Num() ; ++Idx )
		{
			SelectedObjects.AddItem( SelectedExpressions(Idx) );
		}
		
		// Compounds
		for ( INT Idx = 0 ; Idx < SelectedCompounds.Num() ; ++Idx )
		{
			SelectedObjects.AddItem( (UMaterialExpression*)SelectedCompounds(Idx) );
		}

		// Comments
		for ( INT Idx = 0 ; Idx < SelectedComments.Num() ; ++Idx )
		{
			SelectedObjects.AddItem( (UMaterialExpression*)SelectedComments(Idx) );
		}

		PropertyWindow->SetObjectArray( SelectedObjects, EPropertyWindowFlags::ShouldShowCategories | EPropertyWindowFlags::Sorted);
	}
}

/**
 * Deselects all selected material expressions and comments.
 */
void WxMaterialEditor::EmptySelection()
{
	SelectedExpressions.Empty();
	SelectedComments.Empty();
	SelectedCompounds.Empty();

	SourceWindow->SelectedExpression = NULL;
	SourceWindow->Redraw();
}

/**
 * Add the specified object to the list of selected objects
 */
void WxMaterialEditor::AddToSelection(UObject* Obj)
{
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		SelectedComments.AddUniqueItem( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpressionCompound::StaticClass()) )
	{
		SelectedCompounds.AddUniqueItem( static_cast<UMaterialExpressionCompound*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		SelectedExpressions.AddUniqueItem( static_cast<UMaterialExpression*>(Obj) );

		SourceWindow->SelectedExpression = static_cast<UMaterialExpression*>(Obj);
		SourceWindow->Redraw();
	}
}

/**
 * Remove the specified object from the list of selected objects.
 */
void WxMaterialEditor::RemoveFromSelection(UObject* Obj)
{
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		SelectedComments.RemoveItem( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpressionCompound::StaticClass()) )
	{
		SelectedCompounds.RemoveItem( static_cast<UMaterialExpressionCompound*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		SelectedExpressions.RemoveItem( static_cast<UMaterialExpression*>(Obj) );
	}
}

/**
 * Checks whether the specified object is currently selected.
 *
 * @return	TRUE if the specified object is currently selected
 */
UBOOL WxMaterialEditor::IsInSelection(UObject* Obj) const
{
	UBOOL bIsInSelection = FALSE;
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		bIsInSelection = SelectedComments.ContainsItem( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpressionCompound::StaticClass()) )
	{
		bIsInSelection = SelectedCompounds.ContainsItem( static_cast<UMaterialExpressionCompound*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		bIsInSelection = SelectedExpressions.ContainsItem( static_cast<UMaterialExpression*>(Obj) );
	}
	return bIsInSelection;
}

/**
 * Returns the number of selected objects
 */
INT WxMaterialEditor::GetNumSelected() const
{
	return SelectedExpressions.Num() + SelectedComments.Num() + SelectedCompounds.Num();
}

/**
 * Called when the user clicks on an unselected link connector.
 *
 * @param	Connector	the link connector that was clicked on
 */
void WxMaterialEditor::SetSelectedConnector(FLinkedObjectConnector& Connector)
{
	ConnObj = Connector.ConnObj;
	ConnType = Connector.ConnType;
	ConnIndex = Connector.ConnIndex;
	if ( ConnObj )
	{
		debugf(TEXT("SetSelectedConnector: %s(%s) %i %i"), *ConnObj->GetName(), *ConnObj->GetClass()->GetName(), ConnType, ConnIndex);
	}
	else
	{
		debugf(TEXT("SetSelectedConnector: %i %i"),ConnType, ConnIndex);
	}
}

/**
 * Gets the position of the selected connector.
 *
 * @return	an FIntPoint that represents the position of the selected connector, or (0,0)
 *			if no connectors are currently selected
 */
FIntPoint WxMaterialEditor::GetSelectedConnLocation(FCanvas* Canvas)
{
	if( ConnObj )
	{
		UMaterialExpression* ExpressionNode = Cast<UMaterialExpression>( ConnObj );
		if( ExpressionNode )
		{
			return GetExpressionConnectionLocation( ExpressionNode, ConnType, ConnIndex );
		}

		UMaterial* MaterialNode = Cast<UMaterial>( ConnObj );
		if( MaterialNode )
		{
			return GetMaterialConnectionLocation( MaterialNode, ConnType, ConnIndex );
		}
	}

	return FIntPoint(0,0);
}

/**
 * Gets the EConnectorHitProxyType for the currently selected connector
 *
 * @return	the type for the currently selected connector, or 0 if no connector is selected
 */
INT WxMaterialEditor::GetSelectedConnectorType()
{
	return ConnType;
}

/**
 * Makes a connection between connectors.
 */
void WxMaterialEditor::MakeConnectionToConnector(FLinkedObjectConnector& Connector)
{
	// Nothing to do if object pointers are NULL.
	if ( !Connector.ConnObj || !ConnObj )
	{
		return;
	}

	// Avoid connections to yourself.
	if( Connector.ConnObj == ConnObj )
	{
		return;
	}

	UMaterialExpressionCompound* EndConnCompound	= Cast<UMaterialExpressionCompound>( Connector.ConnObj );
	UMaterialExpressionCompound* ConnCompound		= Cast<UMaterialExpressionCompound>( ConnObj );

	// @todo DB: implement connections via compounds.
	if ( EndConnCompound || ConnCompound )
	{
		return;
	}

	UBOOL bConnectionWasMade = FALSE;

	UMaterial* EndConnMaterial						= Cast<UMaterial>( Connector.ConnObj );
	UMaterialExpression* EndConnMaterialExpression	= Cast<UMaterialExpression>( Connector.ConnObj );
	check( EndConnMaterial || EndConnMaterialExpression );

	UMaterial* ConnMaterial							= Cast<UMaterial>( ConnObj );
	UMaterialExpression* ConnMaterialExpression		= Cast<UMaterialExpression>( ConnObj );
	check( ConnMaterial || ConnMaterialExpression );

	// Material to . . .
	if ( ConnMaterial )
	{
		check( ConnType == LOC_OUTPUT ); // Materials are only connected along their right side.

		// Material to expression.
		if ( EndConnMaterialExpression )
		{
			if ( Connector.ConnType == LOC_INPUT ) // Material expressions can only connect to their materials on their left side.
			{
				//debugf(TEXT("mat: %i expr: %i"), ConnIndex, Connector.ConnIndex);
				const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorMakeConnection")) );
				Material->Modify();
				ConnectMaterialToMaterialExpression( ConnMaterial, ConnIndex, EndConnMaterialExpression, Connector.ConnIndex, bHideUnusedConnectors );
				bConnectionWasMade = TRUE;
			}
		}
	}
	// Expression to . . .
	else if ( ConnMaterialExpression )
	{
		// Expression to expression.
		if ( EndConnMaterialExpression )
		{
			if ( ConnType != Connector.ConnType )
			{
				//debugf(TEXT("expr1: %i expr2: %i"), ConnIndex, Connector.ConnIndex);
				UMaterialExpression* InputExpression;
				UMaterialExpression* OutputExpression;
				INT InputIndex;
				INT OutputIndex;

				if( ConnType == LOC_OUTPUT && Connector.ConnType == LOC_INPUT )
				{
					InputExpression = ConnMaterialExpression;
					InputIndex = ConnIndex;
					OutputExpression = EndConnMaterialExpression;
					OutputIndex = Connector.ConnIndex;
				}
				else
				{
					InputExpression = EndConnMaterialExpression;
					InputIndex = Connector.ConnIndex;
					OutputExpression = ConnMaterialExpression;
					OutputIndex = ConnIndex;
				}

				// Input expression.				
				FExpressionInput* ExpressionInput = InputExpression->GetInput(InputIndex);

				// Output expression.
				TArray<FExpressionOutput> Outputs;
				OutputExpression->GetOutputs(Outputs);
				const FExpressionOutput& ExpressionOutput = Outputs( OutputIndex );

				const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorMakeConnection")) );
				InputExpression->Modify();
				ConnectExpressions( *ExpressionInput, ExpressionOutput, OutputExpression );

				bConnectionWasMade = TRUE;
			}
			else
			{
				//debugf( TEXT("rejecting like to like") );
			}
		}
		// Expression to Material.
		else if ( EndConnMaterial )
		{
			check( Connector.ConnType == LOC_OUTPUT ); // Materials are only connected along their right side.
			if ( ConnType == LOC_INPUT ) // Material expressions can only connect to their materials on their left side.
			{
				//debugf(TEXT("expr: %i mat: %i"), ConnIndex, Connector.ConnIndex);
				const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorMakeConnection")) );
				Material->Modify();
				ConnectMaterialToMaterialExpression( EndConnMaterial, Connector.ConnIndex, ConnMaterialExpression, ConnIndex, bHideUnusedConnectors );
				bConnectionWasMade = TRUE;
			}
		}
	}
	
	if ( bConnectionWasMade )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();

		Material->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Breaks the link currently indicated by ConnObj/ConnType/ConnIndex.
 */
void WxMaterialEditor::BreakConnLink(UBOOL bOnlyIfMouseOver)
{
	if ( !ConnObj )
	{
		return;
	}

	UBOOL bConnectionWasBroken = FALSE;

	UMaterialExpression* MaterialExpression	= Cast<UMaterialExpression>( ConnObj );
	UMaterial* MaterialNode					= Cast<UMaterial>( ConnObj );

	if ( ConnType == LOC_OUTPUT ) // right side
	{
		if (MaterialNode && (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialNode, ConnType, ConnIndex)))
		{
			// Clear the material input.
			FExpressionInput* MaterialInput = GetMaterialInputConditional( MaterialNode, ConnIndex, bHideUnusedConnectors );
			if ( MaterialInput->Expression != NULL )
			{
				const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorBreakConnection")) );
				Material->Modify();
				MaterialInput->Expression = NULL;

				bConnectionWasBroken = TRUE;
			}
		}
		else if (MaterialExpression && (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialExpression, ConnType, ConnIndex)))
		{
			FExpressionInput* Input = MaterialExpression->GetInput(ConnIndex);

			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorBreakConnection")) );
			MaterialExpression->Modify();
			Input->Expression = NULL;

			bConnectionWasBroken = TRUE;
		}
	}
	else if ( ConnType == LOC_INPUT ) // left side
	{
		// Only expressions can have connectors on the left side.
		check( MaterialExpression );
		if (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialExpression, ConnType, ConnIndex))
		{
			TArray<FExpressionOutput> Outputs;
			MaterialExpression->GetOutputs(Outputs);
			const FExpressionOutput& ExpressionOutput = Outputs( ConnIndex );

			// Get a list of inputs that refer to the selected output.
			TArray<FMaterialExpressionReference> ReferencingInputs;
			GetListOfReferencingInputs( MaterialExpression, Material, ReferencingInputs, &ExpressionOutput );

			bConnectionWasBroken = ReferencingInputs.Num() > 0;
			if ( bConnectionWasBroken )
			{
				// Clear all referencing inputs.
				const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorBreakConnection")) );
				for ( INT RefIndex = 0 ; RefIndex < ReferencingInputs.Num() ; ++RefIndex )
				{
					if (ReferencingInputs(RefIndex).Expression)
					{
						ReferencingInputs(RefIndex).Expression->Modify();
					}
					else
					{
						Material->Modify();
					}
					for ( INT InputIndex = 0 ; InputIndex < ReferencingInputs(RefIndex).Inputs.Num() ; ++InputIndex )
					{
						FExpressionInput* Input = ReferencingInputs(RefIndex).Inputs(InputIndex);
						check( Input->Expression == MaterialExpression );
						Input->Expression = NULL;
					}
				}
			}
		}
	}

	if ( bConnectionWasBroken )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();

		Material->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Connects an expression output to the specified material input slot.
 *
 * @param	MaterialInputIndex		Index to the material input (Diffuse=0, Emissive=1, etc).
 */
void WxMaterialEditor::OnConnectToMaterial(INT MaterialInputIndex)
{
	// Checks if over expression connection.
	UMaterialExpression* MaterialExpression = NULL;
	if ( ConnObj )
	{
		MaterialExpression = Cast<UMaterialExpression>( ConnObj );
	}

	UBOOL bConnectionWasMade = FALSE;
	if ( MaterialExpression && ConnType == LOC_INPUT )
	{
		UMaterial* MaterialNode = Cast<UMaterial>( ConnObj );

		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorMakeConnection")) );
		Material->Modify();
		ConnectMaterialToMaterialExpression(Material, MaterialInputIndex, MaterialExpression, ConnIndex, bHideUnusedConnectors);
		bConnectionWasMade = TRUE;
	}

	if ( bConnectionWasMade && MaterialExpression )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();
		Material->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Breaks all connections to/from selected material expressions.
 */
void WxMaterialEditor::BreakAllConnectionsToSelectedExpressions()
{
	if ( SelectedExpressions.Num() > 0 )
	{
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorBreakAllConnections")) );
			for( INT ExpressionIndex = 0 ; ExpressionIndex < SelectedExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* MaterialExpression = SelectedExpressions( ExpressionIndex );
				MaterialExpression->Modify();

				// Break links to expression.
				SwapLinksToExpression(MaterialExpression, NULL, Material);
			}
		}
	
		// Update the current preview material. 
		UpdatePreviewMaterial();
		Material->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/** Removes the selected expression from the favorites list. */
void WxMaterialEditor::RemoveSelectedExpressionFromFavorites()
{
	if ( SelectedExpressions.Num() == 1 )
	{
		UMaterialExpression* MaterialExpression = SelectedExpressions(0);
		if (MaterialExpression)
		{
			RemoveMaterialExpressionFromFavorites(MaterialExpression->GetClass());
			EditorOptions->FavoriteExpressions.RemoveItem(MaterialExpression->GetClass()->GetName());
			EditorOptions->SaveConfig();
		}
	}
}

/** Adds the selected expression to the favorites list. */
void WxMaterialEditor::AddSelectedExpressionToFavorites()
{
	if ( SelectedExpressions.Num() == 1 )
	{
		UMaterialExpression* MaterialExpression = SelectedExpressions(0);
		if (MaterialExpression)
		{
			AddMaterialExpressionToFavorites(MaterialExpression->GetClass());
			EditorOptions->FavoriteExpressions.AddUniqueItem(MaterialExpression->GetClass()->GetName());
			EditorOptions->SaveConfig();
		}
	}
}

/**
 * Called when the user releases the mouse over a link connector and is holding the ALT key.
 * Commonly used as a shortcut to breaking connections.
 *
 * @param	Connector	The connector that was ALT+clicked upon.
 */
void WxMaterialEditor::AltClickConnector(FLinkedObjectConnector& Connector)
{
	BreakConnLink(FALSE);
}

/**
 * Called when the user performs a draw operation while objects are selected.
 *
 * @param	DeltaX	the X value to move the objects
 * @param	DeltaY	the Y value to move the objects
 */
void WxMaterialEditor::MoveSelectedObjects(INT DeltaX, INT DeltaY)
{
	const UBOOL bFirstMove = LinkedObjVC->DistanceDragged < 4;
	if ( bFirstMove )
	{
		ExpressionsLinkedToComments.Empty();
	}

	TArray<UMaterialExpression*> ExpressionsToMove;

	// Add selected expressions.
	for( INT ExpressionIndex = 0 ; ExpressionIndex < SelectedExpressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* Expression = SelectedExpressions(ExpressionIndex);
		ExpressionsToMove.AddItem( Expression );
	}

	if ( SelectedComments.Num() > 0 )
	{
		TArray<FIntRect> CommentRects;

		// Add selected comments.  At the same time, create rects for the comments.
		for( INT CommentIndex = 0 ; CommentIndex < SelectedComments.Num() ; ++CommentIndex )
		{
			UMaterialExpressionComment* Comment = SelectedComments(CommentIndex);
			ExpressionsToMove.AddItem( Comment );
			CommentRects.AddItem( FIntRect( FIntPoint(Comment->MaterialExpressionEditorX, Comment->MaterialExpressionEditorY),
											FIntPoint(Comment->MaterialExpressionEditorX+Comment->SizeX, Comment->MaterialExpressionEditorY+Comment->SizeY) ) );
		}

		// If this is the first move, generate a list of expressions that are contained by comments.
		if ( bFirstMove )
		{
			for( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* Expression = Material->Expressions(ExpressionIndex);
				const FIntPoint ExpressionPos( Expression->MaterialExpressionEditorX, Expression->MaterialExpressionEditorY );
				for( INT CommentIndex = 0 ; CommentIndex < CommentRects.Num() ; ++CommentIndex )
				{
					const FIntRect& CommentRect = CommentRects(CommentIndex);
					if ( CommentRect.Contains(ExpressionPos) )
					{
						ExpressionsLinkedToComments.AddUniqueItem( Expression );
						break;
					}
				}
			}

			// Also check comments to see if they are contained by other comments which are selected
			for ( INT CommentIndex = 0; CommentIndex < Material->EditorComments.Num(); ++CommentIndex )
			{
				UMaterialExpressionComment* CurComment = Material->EditorComments( CommentIndex );
				const FIntPoint ExpressionPos( CurComment->MaterialExpressionEditorX, CurComment->MaterialExpressionEditorY );
				for ( INT CommentRectIndex = 0; CommentRectIndex < CommentRects.Num(); ++CommentRectIndex )
				{
					const FIntRect& CommentRect = CommentRects(CommentRectIndex);
					if ( CommentRect.Contains( ExpressionPos ) )
					{
						ExpressionsLinkedToComments.AddUniqueItem( CurComment );
						break;
					}
				}

			}
		}

		// Merge the expression lists.
		for( INT ExpressionIndex = 0 ; ExpressionIndex < ExpressionsLinkedToComments.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* Expression = ExpressionsLinkedToComments(ExpressionIndex);
			ExpressionsToMove.AddUniqueItem( Expression );
		}
	}

	// Perform the move.
	if ( SelectedCompounds.Num() > 0 || ExpressionsToMove.Num() > 0 )
	{
		for( INT CompoundIndex = 0 ; CompoundIndex < SelectedCompounds.Num() ; ++CompoundIndex )
		{
			UMaterialExpressionCompound* Compound = SelectedCompounds(CompoundIndex);
			Compound->MaterialExpressionEditorX += DeltaX;
			Compound->MaterialExpressionEditorY += DeltaY;
			for( INT ExpressionIndex = 0 ; ExpressionIndex < Compound->MaterialExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* Expression = Compound->MaterialExpressions(ExpressionIndex);
				Expression->MaterialExpressionEditorX += DeltaX;
				Expression->MaterialExpressionEditorY += DeltaY;
			}
		}

		for( INT ExpressionIndex = 0 ; ExpressionIndex < ExpressionsToMove.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* Expression = ExpressionsToMove(ExpressionIndex);
			Expression->MaterialExpressionEditorX += DeltaX;
			Expression->MaterialExpressionEditorY += DeltaY;
		}
		Material->MarkPackageDirty();
		bMaterialDirty = TRUE;
	}
}

void WxMaterialEditor::EdHandleKeyInput(FViewport* Viewport, FName Key, EInputEvent Event)
{
	const UBOOL bCtrlDown = Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl);
	if( Event == IE_Pressed )
	{
		if ( bCtrlDown )
		{
			if( Key == KEY_C )
			{
				// Clear the material copy buffer and copy the selected expressions into it.
				TArray<UMaterialExpression*> NewExpressions;
				TArray<UMaterialExpression*> NewComments;
				GetCopyPasteBuffer()->Expressions.Empty();
				GetCopyPasteBuffer()->EditorComments.Empty();

				UMaterialExpression::CopyMaterialExpressions( SelectedExpressions, SelectedComments, GetCopyPasteBuffer(), NewExpressions, NewComments );
			}
			else if ( Key == KEY_V )
			{
				// Paste the material copy buffer into this material.
				PasteExpressionsIntoMaterial( NULL );
			}
			else if( Key == KEY_W )
			{
				DuplicateSelectedObjects();
			}
			else if( Key == KEY_Y )
			{
				Redo();
			}
			else if( Key == KEY_Z )
			{
				Undo();
			}
		}
		else
		{
			if( Key == KEY_Delete )
			{
				DeleteSelectedObjects();
			}
			else if ( Key == KEY_SpaceBar )
			{
				// Spacebar force-refreshes previews.
				ForceRefreshExpressionPreviews();
				RefreshExpressionViewport();
				RefreshPreviewViewport();
			}
			else if ( Key == KEY_LeftMouseButton )
			{
				// Check if the "Toggle realtime preview" icon was clicked on.
				Viewport->Invalidate();
				const INT HitX = Viewport->GetMouseX();
				const INT HitY = Viewport->GetMouseY();
				HHitProxy*	HitResult = Viewport->GetHitProxy( HitX, HitY );
				if ( HitResult && HitResult->IsA( HRealtimePreviewProxy::StaticGetType() ) )
				{
					// Toggle the material expression's realtime preview state.
					UMaterialExpression* MaterialExpression = static_cast<HRealtimePreviewProxy*>( HitResult )->MaterialExpression;
					check( MaterialExpression );
					{
						const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorTogglePreview")) );
						MaterialExpression->Modify();
						MaterialExpression->bRealtimePreview = !MaterialExpression->bRealtimePreview;
					}
					MaterialExpression->PreEditChange( NULL );
					MaterialExpression->PostEditChange();
					MaterialExpression->MarkPackageDirty();
					bMaterialDirty = TRUE;

					// Update the preview.
					RefreshExpressionPreview( MaterialExpression, TRUE );
					RefreshPreviewViewport();
				}
			}
		}
		if ( Key == KEY_Enter )
		{
			if (bMaterialDirty)
			{
				UpdateOriginalMaterial();
			}
		}
	}
	else if ( Event == IE_Released )
	{
		// LMBRelease + hotkey places certain material expression types.
		if( Key == KEY_LeftMouseButton )
		{
			const UBOOL bShiftDown = Viewport->KeyState(KEY_LeftShift) || Viewport->KeyState(KEY_RightShift);
			if( bShiftDown && Viewport->KeyState(KEY_C))
			{
				CreateNewCommentExpression();
			}
			else if ( bCtrlDown )
			{
				BreakConnLink(TRUE);
			}
			else
			{
				struct FMaterialExpressionHotkey
				{
					FName		Key;
					UClass*		MaterialExpressionClass;
				};
				static FMaterialExpressionHotkey MaterialExpressionHotkeys[] =
				{
					{ KEY_A, UMaterialExpressionAdd::StaticClass() },
					{ KEY_B, UMaterialExpressionBumpOffset::StaticClass() },
					{ KEY_C, UMaterialExpressionComponentMask::StaticClass() },
					{ KEY_D, UMaterialExpressionDivide::StaticClass() },
					{ KEY_E, UMaterialExpressionPower::StaticClass() },
					{ KEY_I, UMaterialExpressionIf::StaticClass() },
					{ KEY_L, UMaterialExpressionLinearInterpolate::StaticClass() },
					{ KEY_M, UMaterialExpressionMultiply::StaticClass() },
					{ KEY_N, UMaterialExpressionNormalize::StaticClass() },
					{ KEY_O, UMaterialExpressionOneMinus::StaticClass() },
					{ KEY_P, UMaterialExpressionPanner::StaticClass() },
					{ KEY_R, UMaterialExpressionReflectionVector::StaticClass() },
					{ KEY_S, UMaterialExpressionScalarParameter::StaticClass() },
					{ KEY_T, UMaterialExpressionTextureSample::StaticClass() },
					{ KEY_U, UMaterialExpressionTextureCoordinate::StaticClass() },
					{ KEY_V, UMaterialExpressionVectorParameter::StaticClass() },
					{ KEY_One, UMaterialExpressionConstant::StaticClass() },
					{ KEY_Two, UMaterialExpressionConstant2Vector::StaticClass() },
					{ KEY_Three, UMaterialExpressionConstant3Vector::StaticClass() },
					{ KEY_Four, UMaterialExpressionConstant4Vector::StaticClass() },
					{ NAME_None, NULL },
				};

				// Iterate over all expression hotkeys, reporting the first expression that has a key down.
				UClass* NewMaterialExpressionClass = NULL;
				for ( INT Index = 0 ; ; ++Index )
				{
					const FMaterialExpressionHotkey& ExpressionHotKey = MaterialExpressionHotkeys[Index];
					if ( ExpressionHotKey.MaterialExpressionClass )
					{
						if ( Viewport->KeyState(ExpressionHotKey.Key) )
						{
							NewMaterialExpressionClass = ExpressionHotKey.MaterialExpressionClass;
							break;
						}
					}
					else
					{
						// A NULL MaterialExpressionClass indicates end of list.
						break;
					}
				}

				// Create a new material expression if the associated hotkey was found to be down.
				if ( NewMaterialExpressionClass )
				{
					const INT LocX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
					const INT LocY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
					CreateNewMaterialExpression( NewMaterialExpressionClass, FALSE, TRUE, FIntPoint(LocX, LocY) );
				}
			}
		}
	}
}

/**
 * Called once the user begins a drag operation.  Transactions expression, comment and compound position.
 */
void WxMaterialEditor::BeginTransactionOnSelected()
{
	check( !ScopedTransaction );
	ScopedTransaction = new FScopedTransaction( *LocalizeUnrealEd(TEXT("LinkedObjectModify")) );

	Material->Modify();
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->Expressions.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( MaterialExpressionIndex );
		MaterialExpression->Modify();
	}
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->EditorComments.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionComment* Comment = Material->EditorComments( MaterialExpressionIndex );
		Comment->Modify();
	}
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->EditorCompounds.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionCompound* Compound = Material->EditorCompounds( MaterialExpressionIndex );
		Compound->Modify();
	}
}

/**
 * Called when the user releases the mouse button while a drag operation is active.
 */
void WxMaterialEditor::EndTransactionOnSelected()
{
	check( ScopedTransaction );
	delete ScopedTransaction;
	ScopedTransaction = NULL;
}


/**
 *	Handling for dragging on 'special' hit proxies. Should only have 1 object selected when this is being called. 
 *	In this case used for handling resizing handles on Comment objects. 
 *
 *	@param	DeltaX			Horizontal drag distance this frame (in pixels)
 *	@param	DeltaY			Vertical drag distance this frame (in pixels)
 *	@param	SpecialIndex	Used to indicate different classes of action. @see HLinkedObjProxySpecial.
 */
void WxMaterialEditor::SpecialDrag( INT DeltaX, INT DeltaY, INT NewX, INT NewY, INT SpecialIndex )
{
	// Can only 'special drag' one object at a time.
	if( SelectedComments.Num() == 1 )
	{
		if ( SpecialIndex == 1 )
		{
			// Apply dragging to the comment size.
			UMaterialExpressionComment* Comment = SelectedComments(0);
			Comment->SizeX += DeltaX;
			Comment->SizeX = ::Max<INT>(Comment->SizeX, 64);

			Comment->SizeY += DeltaY;
			Comment->SizeY = ::Max<INT>(Comment->SizeY, 64);
			Comment->MarkPackageDirty();
			bMaterialDirty = TRUE;
		}
	}
}

/**
 * Updates the original material with the changes made in the editor
 */
void WxMaterialEditor::UpdateOriginalMaterial()
{
	// Cannot save material to a cooked package
	if( Material->GetOutermost()->PackageFlags & PKG_Cooked )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_OperationDisallowedOnCookedContent") );
		return;
	}

	//remove any memory copies of shader files, so they will be reloaded from disk
	//this way the material editor can be used for quick shader iteration
	FlushShaderFileCache();

	//recompile and refresh the preview material so it will be updated if there was a shader change
	UpdatePreviewMaterial();

	const FScopedBusyCursor BusyCursor;

	const FString LocalizedMaterialEditorApply( LocalizeUnrealEd("ToolTip_MaterialEditorApply") );
	GWarn->BeginSlowTask( *LocalizedMaterialEditorApply, TRUE );
	GWarn->StatusUpdatef( 1, 1, *LocalizedMaterialEditorApply );

	// make sure that any staticmeshes, etc using this material will stop using the FMaterialResource of the original 
	// material, and will use the new FMaterialResource created when we make a new UMaterial inplace
	FGlobalComponentReattachContext RecreateComponents;

	// overwrite the original material in place by constructing a new one with the same name
	OriginalMaterial = (UMaterial*)UObject::StaticDuplicateObject(Material, Material, OriginalMaterial->GetOuter(), *OriginalMaterial->GetName(), ~0, OriginalMaterial->GetClass());
	// Restore RF_Standalone on the original material, as it had been removed from the preview material so that it could be GC'd.
	OriginalMaterial->SetFlags( RF_Standalone );

	// copy the flattened texture manually because it's duplicatetransient so it's NULLed when duplicating normally
	// (but we don't want it NULLed in this case)
	OriginalMaterial->MobileBaseTexture = Material->MobileBaseTexture;

	// Manually copy bUsedAsSpecialEngineMaterial as it is duplicate transient to prevent accidental creation of new special engine materials
	OriginalMaterial->bUsedAsSpecialEngineMaterial = Material->bUsedAsSpecialEngineMaterial;

	// let the material update itself if necessary
	OriginalMaterial->PreEditChange(NULL);
	OriginalMaterial->PostEditChange();
	OriginalMaterial->MarkPackageDirty();

	//copy the compile errors from the original material to the preview material
	//this is necessary since some errors are not encountered while compiling the preview material, but only when compiling the full material
	//without this copy the errors will only be reported after the material editor is closed and reopened
	const FMaterialResource* OriginalMaterialResource = OriginalMaterial->GetMaterialResource(MSP_BASE);
	FMaterialResource* PreviewMaterialResource = Material->GetMaterialResource(MSP_BASE);
	if (OriginalMaterialResource && PreviewMaterialResource)
	{
		PreviewMaterialResource->SetCompileErrors(OriginalMaterialResource->GetCompileErrors());
	}

	// clear the dirty flag
	bMaterialDirty = FALSE;

	// update the world's viewports
	GCallbackEvent->Send(CALLBACK_RefreshEditor);
	GCallbackEvent->Send(CALLBACK_RedrawAllViewports);

	for (FObjectIterator It; It; ++It)
	{
		UMaterialInstance* MatInst = Cast<UMaterialInstance>(*It);
		ATerrain* Terrain = Cast<ATerrain>(*It);
		AEmitter* Emitter = Cast<AEmitter>(*It);
		if (MatInst)
		{
			UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MatInst);
			if (MIC)
			{
				// Loop through all loaded material instance constants and update their parameter names since they may have changed.
				MIC->UpdateParameterNames();
			}

			// Go through all loaded material instances and recompile their static permutation resources if needed
			// This is necessary since the parent UMaterial stores information about how it should be rendered, (eg bUsesDistortion)
			// but the child can have its own shader map which may not contain all the shaders that the parent's settings indicate that it should.
			// this code is duplicated in Material.cpp UMaterial::SetMaterialUsage
			UMaterial* BaseMaterial = MatInst->GetMaterial();
			//only recompile if the instance is a child of the material that got updated
			if (OriginalMaterial == BaseMaterial)
			{
				MatInst->InitStaticPermutation();
			}
		}
		else if (Terrain)
		{
			Terrain->UpdateCachedMaterial(OriginalMaterial);
		}
		else if (Emitter && Emitter->ParticleSystemComponent)
		{
			Emitter->ParticleSystemComponent->bIsViewRelevanceDirty = TRUE;
		}
	}

	GWarn->EndSlowTask();

	// flatten this material if needed
	GCallbackEvent->Send(CALLBACK_MobileFlattenedTextureUpdate, OriginalMaterial);
	
	// copy the dominant texture into the preview material so user can see it
	Material->MobileBaseTexture = OriginalMaterial->MobileBaseTexture;
}

/**
 * Uses the global Undo transactor to reverse changes, update viewports etc.
 */
void WxMaterialEditor::Undo()
{
	EmptySelection();

	INT NumExpressions = Material->Expressions.Num();
	GEditor->UndoTransaction();

	if(NumExpressions != Material->Expressions.Num())
	{
		Material->BuildEditorParameterList();
	}

	UpdateSearch(FALSE);

	// Update the current preview material.
	UpdatePreviewMaterial();

	UpdatePropertyWindow();

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	bMaterialDirty = TRUE;
}

/**
 * Uses the global Undo transactor to redo changes, update viewports etc.
 */
void WxMaterialEditor::Redo()
{
	EmptySelection();

	INT NumExpressions = Material->Expressions.Num();
	GEditor->RedoTransaction();

	if(NumExpressions != Material->Expressions.Num())
	{
		Material->BuildEditorParameterList();
	}

	// Update the current preview material.
	UpdatePreviewMaterial();

	UpdatePropertyWindow();

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	bMaterialDirty = TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FSerializableObject interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WxMaterialEditor::Serialize(FArchive& Ar)
{
	WxMaterialEditorBase::Serialize(Ar);

	Ar << Material;
	Ar << SelectedExpressions;
	Ar << SelectedComments;
	Ar << SelectedCompounds;
	Ar << ExpressionPreviews;
	Ar << ExpressionsLinkedToComments;
	Ar << EditorOptions;
	Ar << OriginalMaterial;
	Ar << DebugMaterial;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FNotifyHook interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WxMaterialEditor::NotifyPreChange(void* Src, UProperty* PropertyAboutToChange)
{
	check( !ScopedTransaction );
	ScopedTransaction = new FScopedTransaction( *LocalizeUnrealEd(TEXT("MaterialEditorEditProperties")) );
	FlushRenderingCommands();
}

void WxMaterialEditor::NotifyPostChange(void* Src, UProperty* PropertyThatChanged)
{
	check( ScopedTransaction );

	if ( PropertyThatChanged )
	{
		const FName NameOfPropertyThatChanged( *PropertyThatChanged->GetName() );
		if ( NameOfPropertyThatChanged == FName(TEXT("PreviewMesh")) ||
			 NameOfPropertyThatChanged == FName(TEXT("bUsedWithSkeletalMesh")) )
		{
			// SetPreviewMesh will return FALSE if the material has bUsedWithSkeletalMesh and
			// a skeleton was requested, in which case revert to a sphere static mesh.
			if ( !SetPreviewMesh( *Material->PreviewMesh ) )
			{
				SetPreviewMesh( GUnrealEd->GetThumbnailManager()->TexPropSphere, NULL );
			}
		}

		WxPropertyControl* PropertyWindowItem = static_cast<WxPropertyControl*>( Src );
		WxPropertyWindow* PropertyWindow = PropertyWindowItem->GetPropertyWindow();
		for ( WxPropertyWindow::TObjectIterator Itor( PropertyWindow->ObjectIterator() ) ; Itor ; ++Itor )
		{
			UMaterialExpression* Expression = Cast< UMaterialExpression >( *Itor );
			if(Expression)
			{
				if(NameOfPropertyThatChanged == FName(TEXT("ParameterName")))
				{
					Material->UpdateExpressionParameterName(Expression);
				}
				else if (NameOfPropertyThatChanged == FName(TEXT("ParamNames")))
				{
					Material->UpdateExpressionDynamicParameterNames(Expression);
				}
				else
				{
					Material->PropagateExpressionParameterChanges(Expression);
				}
			}
		}
	}

	// Update the current preview material.
	UpdatePreviewMaterial();

	delete ScopedTransaction;
	ScopedTransaction = NULL;

	UpdateSearch(FALSE);
	Material->MarkPackageDirty();
	RefreshExpressionPreviews();
	RefreshSourceWindowMaterial();
	bMaterialDirty = TRUE;
}

void WxMaterialEditor::NotifyExec(void* Src, const TCHAR* Cmd)
{
	GUnrealEd->NotifyExec( Src, Cmd );
}

/** 
 * Computes a bounding box for the selected material expressions.  Output is not sensible if no expressions are selected.
 */
FIntRect WxMaterialEditor::GetBoundingBoxOfSelectedExpressions()
{
	return GetBoundingBoxOfExpressions( SelectedExpressions );
}

/** 
 * Computes a bounding box for the specified set of material expressions.  Output is not sensible if the set is empty.
 */
FIntRect WxMaterialEditor::GetBoundingBoxOfExpressions(const TArray<UMaterialExpression*>& Expressions)
{
	FIntRect Result(0, 0, 0, 0);
	UBOOL bResultValid = FALSE;

	for( INT ExpressionIndex = 0 ; ExpressionIndex < Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* Expression = Expressions(ExpressionIndex);
		// @todo DB: Remove these hardcoded extents.
		const FIntRect ObjBox( Expression->MaterialExpressionEditorX-30, Expression->MaterialExpressionEditorY-20, Expression->MaterialExpressionEditorX+150, Expression->MaterialExpressionEditorY+150 );

		if( bResultValid )
		{
			// Expand result box to encompass
			Result.Min.X = ::Min(Result.Min.X, ObjBox.Min.X);
			Result.Min.Y = ::Min(Result.Min.Y, ObjBox.Min.Y);

			Result.Max.X = ::Max(Result.Max.X, ObjBox.Max.X);
			Result.Max.Y = ::Max(Result.Max.Y, ObjBox.Max.Y);
		}
		else
		{
			// Use this objects bounding box to initialise result.
			Result = ObjBox;
			bResultValid = TRUE;
		}
	}

	return Result;
}

/**
 * Creates a new material expression comment frame.
 */
void WxMaterialEditor::CreateNewCommentExpression()
{
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("NewComment"), TEXT("CommentText"), TEXT("Comment") );
	if ( Result == wxID_OK )
	{
		UMaterialExpressionComment* NewComment = NULL;
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorNewComment")) );
			Material->Modify();

			NewComment = ConstructObject<UMaterialExpressionComment>( UMaterialExpressionComment::StaticClass(), Material, NAME_None, RF_Transactional );

			// Add to the list of comments associated with this material.
			Material->EditorComments.AddItem( NewComment );

			if ( SelectedExpressions.Num() > 0 )
			{
				const FIntRect SelectedBounds = GetBoundingBoxOfSelectedExpressions();
				NewComment->MaterialExpressionEditorX = SelectedBounds.Min.X;
				NewComment->MaterialExpressionEditorY = SelectedBounds.Min.Y;
				NewComment->SizeX = SelectedBounds.Max.X - SelectedBounds.Min.X;
				NewComment->SizeY = SelectedBounds.Max.Y - SelectedBounds.Min.Y;
			}
			else
			{

				NewComment->MaterialExpressionEditorX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
				NewComment->MaterialExpressionEditorY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
				NewComment->SizeX = 128;
				NewComment->SizeY = 128;
			}

			NewComment->Text = dlg.GetEnteredString();
		}

		// Select the new comment.
		EmptySelection();
		AddToSelection( NewComment );

		Material->MarkPackageDirty();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Updates the CompoundInfoMap entry for the specified compound.
 */
void WxMaterialEditor::UpdateCompoundExpressionInfo(UMaterialExpressionCompound* Compound)
{
	FCompoundInfo* Info;
	FCompoundInfo** OldInfo = CompoundInfoMap.Find( Compound );
	if ( OldInfo )
	{
		// Clear out old info.
		Info = *OldInfo;
		Info->CompoundInputs.Empty();
		Info->CompoundOutputs.Empty();
	}
	else
	{
		// No old info found; allocate new info.
		Info = new FCompoundInfo;
		CompoundInfoMap.Set( Compound, Info );
	}

	// Make a list of compound inputs (expression inputs that are connected to expressions outside of the compound).
	for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Compound->MaterialExpressions.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Compound->MaterialExpressions( MaterialExpressionIndex );

		const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
		for( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ ExpressionInputIndex )
		{
			FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
			UMaterialExpression* InputExpression = Input->Expression;

			// Store the input if it refers to an expression outside of the compound.
			if ( InputExpression && !Compound->MaterialExpressions.ContainsItem( InputExpression ) )
			{
				FCompoundInput* CompoundInput = new(Info->CompoundInputs) FCompoundInput;
				CompoundInput->Expression = MaterialExpression;
				CompoundInput->InputIndex = ExpressionInputIndex;
			}
		}
	}

	// Make a list of compound outputs (expression outputs that are referred to by expressions outside of the compound).
	for ( INT ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions( ExpressionIndex );
		if ( Compound->MaterialExpressions.ContainsItem( MaterialExpression ) )
		{
			continue;
		}

		// See if this expression takes input from anything in the compound.
		const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
		for( INT ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ ExpressionInputIndex )
		{
			FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
			UMaterialExpression* InputExpression = Input->Expression;
			if ( InputExpression && Compound->MaterialExpressions.ContainsItem( InputExpression ) )
			{
				// Determine which output is connected to the external object.
				TArray<FExpressionOutput> Outputs;
				InputExpression->GetOutputs( Outputs );
				for( INT OutputIndex = 0 ; OutputIndex < Outputs.Num() ; ++OutputIndex )
				{
					FExpressionOutput& Output = Outputs(OutputIndex);
					if(	Output.Mask == Input->Mask
						&& Output.MaskR == Input->MaskR
						&& Output.MaskG == Input->MaskG
						&& Output.MaskB == Input->MaskB
						&& Output.MaskA == Input->MaskA )
					{
						FCompoundOutput* CompoundOutput = new(Info->CompoundOutputs) FCompoundOutput;
						CompoundOutput->Expression = MaterialExpression;
						CompoundOutput->ExpressionOutput = Output;
					}
				}
			}
		}
	}
}

/**
 * Creates a new compound material expression from the selected material expressions.
 */
void WxMaterialEditor::CreateNewCompoundExpression()
{
	if ( SelectedExpressions.Num() > 0 )
	{
		// Make sure no selected material expressions 1) already belong to compounds; or 2) are compounds themselves.
		FString ErrorString;
		for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < SelectedExpressions.Num() ; ++MaterialExpressionIndex )
		{
			UMaterialExpression* MaterialExpression			= SelectedExpressions( MaterialExpressionIndex );
			if ( MaterialExpression->Compound )
			{
				const FString NewError( FString::Printf( LocalizeSecure(LocalizeUnrealEd("Error_ExpressionAlreadyBelongsToCompound_F"), *MaterialExpression->GetCaption(), *MaterialExpression->Compound->GetCaption()) ) );
				ErrorString += FString::Printf( TEXT("%s\n"), *NewError );
			}
			else
			{
				UMaterialExpressionCompound* CompoundExpression	= Cast<UMaterialExpressionCompound>( MaterialExpression );
				if ( CompoundExpression )
				{
					const FString NewError( FString::Printf( LocalizeSecure(LocalizeUnrealEd("Error_ExpressionCompoundsCantContainOtherCompounds_F"), *MaterialExpression->GetCaption()) ) );
					ErrorString += NewError + TEXT("\n");
				}
			}
		}

		if ( ErrorString.Len() )
		{
			FString CompleteErrorString( FString::Printf( TEXT("%s\n\n"), *LocalizeUnrealEd("Error_CantCreateMaterialExpressionCompound") ) );
			CompleteErrorString += ErrorString;
			appMsgf( AMT_OK, *CompleteErrorString );
			return;
		}

		// Create a new compound expression.
		UMaterialExpressionCompound* NewCompound = NULL;
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorNewCompound")) );
			Material->Modify();

			NewCompound = ConstructObject<UMaterialExpressionCompound>( UMaterialExpressionCompound::StaticClass(), Material, NAME_None, RF_Transactional );
			Material->EditorCompounds.AddItem( NewCompound );

			const FIntRect SelectedBounds = GetBoundingBoxOfSelectedExpressions();
			const FIntPoint Extents( SelectedBounds.Max.X - SelectedBounds.Min.X, SelectedBounds.Max.Y - SelectedBounds.Min.Y );
			NewCompound->MaterialExpressionEditorX = SelectedBounds.Min.X + Extents.X/2;
			NewCompound->MaterialExpressionEditorY = SelectedBounds.Min.Y + Extents.Y/2;
			NewCompound->Caption = FString::Printf( TEXT("Compound Expression (%i)"), SelectedExpressions.Num() );

			// Link expressions to the compound and vice versa.
			for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < SelectedExpressions.Num() ; ++MaterialExpressionIndex )
			{
				UMaterialExpression* MaterialExpression = SelectedExpressions( MaterialExpressionIndex );
				check( MaterialExpression->Compound == NULL );
				MaterialExpression->Compound = NewCompound;
				NewCompound->MaterialExpressions.AddItem( MaterialExpression );
			}
		}

		UpdateCompoundExpressionInfo( NewCompound );

		// Select the new compound node.
		EmptySelection();
		AddToSelection( NewCompound );

		Material->MarkPackageDirty();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

/**
 * Creates a new material expression of the specified class.  Will automatically connect output 0
 * of the new expression to an input tab, if one was clicked on.
 *
 * @param	NewExpressionClass		The type of material expression to add.  Must be a child of UMaterialExpression.
 * @param	bAutoConnect			If TRUE, connect the new material expression to the most recently selected connector, if possible.
 * @param	bAutoSelect				If TRUE, deselect all expressions and select the newly created one.
 * @param	NodePos					Position of the new node.
 */
UMaterialExpression* WxMaterialEditor::CreateNewMaterialExpression(UClass* NewExpressionClass, UBOOL bAutoConnect, UBOOL bAutoSelect, const FIntPoint& NodePos)
{
	check( NewExpressionClass->IsChildOf(UMaterialExpression::StaticClass()) );

	// Create the new expression.
	UMaterialExpression* NewExpression = NULL;
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorNewExpression")) );
		Material->Modify();

		NewExpression = ConstructObject<UMaterialExpression>( NewExpressionClass, Material, NAME_None, RF_Transactional );
		Material->Expressions.AddItem( NewExpression );

		// If the new expression is created connected to an input tab, offset it by this amount.
		INT NewConnectionOffset = 0;

		if (bAutoConnect)
		{
			// If an input tab was clicked on, bind the new expression to that tab.
			if ( ConnType == LOC_OUTPUT && ConnObj )
			{
				UMaterial* ConnMaterial							= Cast<UMaterial>( ConnObj );
				UMaterialExpression* ConnMaterialExpression		= Cast<UMaterialExpression>( ConnObj );
				check( ConnMaterial  || ConnMaterialExpression );

				if ( ConnMaterial )
				{
					ConnectMaterialToMaterialExpression( ConnMaterial, ConnIndex, NewExpression, 0, bHideUnusedConnectors );
				}
				else if ( ConnMaterialExpression )
				{
					UMaterialExpression* InputExpression = ConnMaterialExpression;
					UMaterialExpression* OutputExpression = NewExpression;

					INT InputIndex = ConnIndex;
					INT OutputIndex = 0;

					// Input expression.
					FExpressionInput* ExpressionInput = InputExpression->GetInput(InputIndex);

					// Output expression.
					TArray<FExpressionOutput> Outputs;
					OutputExpression->GetOutputs(Outputs);
					const FExpressionOutput& ExpressionOutput = Outputs( OutputIndex );

					InputExpression->Modify();
					ConnectExpressions( *ExpressionInput, ExpressionOutput, OutputExpression );
				}

				// Offset the new expression it by this amount.
				NewConnectionOffset = 30;
			}
		}

		// Set the expression location.
		NewExpression->MaterialExpressionEditorX = NodePos.X + NewConnectionOffset;
		NewExpression->MaterialExpressionEditorY = NodePos.Y + NewConnectionOffset;

		// If the user is adding a texture sample, automatically assign the currently selected texture to it.
		UMaterialExpressionTextureSample* METextureSample = Cast<UMaterialExpressionTextureSample>( NewExpression );
		if( METextureSample )
		{
			GCallbackEvent->Send( CALLBACK_LoadSelectedAssetsIfNeeded );
			METextureSample->Texture = GEditor->GetSelectedObjects()->GetTop<UTexture>();
		}

		// If the user is creating any parameter objects, then generate a GUID for it.
		UMaterialExpressionParameter* ParameterExpression = Cast<UMaterialExpressionParameter>( NewExpression );

		if( ParameterExpression )
		{
			ParameterExpression->ConditionallyGenerateGUID(TRUE);
		}

		UMaterialExpressionTextureSampleParameter* TextureParameterExpression = Cast<UMaterialExpressionTextureSampleParameter>( NewExpression );
		if( TextureParameterExpression )
		{
			// Change the parameter's name on creation to mirror the object's name; this avoids issues of having colliding parameter
			// names and having the name left as "None"
			TextureParameterExpression->ParameterName = TextureParameterExpression->GetFName();
			TextureParameterExpression->ConditionallyGenerateGUID(TRUE);
		}

		UMaterialExpressionComponentMask* ComponentMaskExpression = Cast<UMaterialExpressionComponentMask>( NewExpression );
		// Setup defaults for the most likely use case
		// For Gears2 content a mask of .rg is 47% of all mask combinations
		// Can't change default properties as that will affect existing content
		if( ComponentMaskExpression )
		{
			ComponentMaskExpression->R = TRUE;
			ComponentMaskExpression->G = TRUE;
		}

		UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskExpression = Cast<UMaterialExpressionStaticComponentMaskParameter>( NewExpression );
		// Setup defaults for the most likely use case
		// For Gears2 content a static mask of .r is 67% of all mask combinations
		// Can't change default properties as that will affect existing content
		if( StaticComponentMaskExpression )
		{
			StaticComponentMaskExpression->DefaultR = TRUE;
		}

		Material->AddExpressionParameter(NewExpression);
	}

	// Select the new node.
	if ( bAutoSelect )
	{
		EmptySelection();
		AddToSelection( NewExpression );
	}

	RefreshSourceWindowMaterial();
	UpdateSearch(FALSE);

	// Update the current preview material.
	UpdatePreviewMaterial();
	Material->MarkPackageDirty();

	UpdatePropertyWindow();


	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	bMaterialDirty = TRUE;
	return NewExpression;
}

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxMaterialEditor::GetDockingParentName() const
{
	return TEXT("MaterialEditor");
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxMaterialEditor::GetDockingParentVersion() const
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Wx event handlers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WxMaterialEditor::OnClose(wxCloseEvent& In)
{
	// @todo DB: Store off the viewport camera position/orientation to the material.
	//AnimTree->PreviewCamPos = PreviewVC->ViewLocation;
	//AnimTree->PreviewCamRot = PreviewVC->ViewRotation;

	if (bMaterialDirty)
	{
		// find out the user wants to do with this dirty material
		INT YesNoCancelReply = appMsgf(AMT_YesNoCancel, *LocalizeUnrealEd("Prompt_MaterialEditorClose"));
		// act on it
		switch (YesNoCancelReply)
		{
			case 0: // Yes
				// update material
				UpdateOriginalMaterial();
				break;

			case 1: // No
				// do nothing
				break;

			case 2: // Cancel
				// veto closing the window
				if (In.CanVeto())
				{
					In.Veto();
				}
				// don't call destroy!
				return;
		}
	}

	// Reset the transaction buffer so that references to the preview material are cleared.
	GEditor->ResetTransaction( TEXT("CloseMaterialEditor") );
	Destroy();
}

void WxMaterialEditor::OnNewMaterialExpression(wxCommandEvent& In)
{
	const INT NewNodeClassIndex = In.GetId() - IDM_NEW_SHADER_EXPRESSION_START;

	UClass* NewExpressionClass = NULL;
	if (bUseUnsortedMenus == TRUE)
	{
	check( NewNodeClassIndex >= 0 && NewNodeClassIndex < MaterialExpressionClasses.Num() );
		NewExpressionClass = MaterialExpressionClasses(NewNodeClassIndex);
	}
	else
	{
		INT NodeClassIndex = NewNodeClassIndex;
		INT FavoriteCount = FavoriteExpressionClasses.Num();
		if (NodeClassIndex < FavoriteCount)
		{
			// It's from the favorite expressions menu...
			NewExpressionClass = FavoriteExpressionClasses(NodeClassIndex);
		}
		else
		{
			INT Count = FavoriteCount;
			for (INT CheckIndex = 0; (CheckIndex < CategorizedMaterialExpressionClasses.Num()) && (NewExpressionClass == NULL); CheckIndex++)
			{
				FCategorizedMaterialExpressionNode& CategoryNode = CategorizedMaterialExpressionClasses(CheckIndex);
				if (NodeClassIndex < (Count + CategoryNode.MaterialExpressionClasses.Num()))
				{
					NodeClassIndex -= Count;
					NewExpressionClass = CategoryNode.MaterialExpressionClasses(NodeClassIndex);
				}
				Count += CategoryNode.MaterialExpressionClasses.Num();
			}

			if (NewExpressionClass == NULL)
			{
				// Assume it is in the unassigned list...
				NodeClassIndex -= Count;
				if (NodeClassIndex < UnassignedExpressionClasses.Num())
				{
					NewExpressionClass = UnassignedExpressionClasses(NodeClassIndex);
				}
			}
		}
	}

	if (NewExpressionClass)
	{
		const INT LocX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
		const INT LocY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
		CreateNewMaterialExpression( NewExpressionClass, TRUE, TRUE, FIntPoint(LocX, LocY) );
	}
}

void WxMaterialEditor::OnNewComment(wxCommandEvent& In)
{
	CreateNewCommentExpression();
}

void WxMaterialEditor::OnNewCompoundExpression(wxCommandEvent& In)
{
	CreateNewCompoundExpression();
}

void WxMaterialEditor::OnUseCurrentTexture(wxCommandEvent& In)
{
	// Set the currently selected texture in the generic browser
	// as the texture to use in all selected texture sample expressions.
	GCallbackEvent->Send( CALLBACK_LoadSelectedAssetsIfNeeded );
	UTexture* SelectedTexture = GEditor->GetSelectedObjects()->GetTop<UTexture>();
	if ( SelectedTexture )
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("UseCurrentTexture")) );
		for( INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < SelectedExpressions.Num() ; ++MaterialExpressionIndex )
		{
			UMaterialExpression* MaterialExpression = SelectedExpressions( MaterialExpressionIndex );
			if ( MaterialExpression->IsA(UMaterialExpressionTextureSample::StaticClass()) )
			{
				UMaterialExpressionTextureSample* TextureSample = static_cast<UMaterialExpressionTextureSample*>(MaterialExpression);
				TextureSample->Modify();
				TextureSample->Texture = SelectedTexture;
			}
		}

		// Update the current preview material. 
		UpdatePreviewMaterial();
		Material->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		UpdatePropertyWindow();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		bMaterialDirty = TRUE;
	}
}

void WxMaterialEditor::OnDuplicateObjects(wxCommandEvent& In)
{
	DuplicateSelectedObjects();
}

void WxMaterialEditor::OnPasteHere(wxCommandEvent& In)
{
	const FIntPoint ClickLocation( (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D, (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D );
	PasteExpressionsIntoMaterial( &ClickLocation );
}

void WxMaterialEditor::OnConvertObjects(wxCommandEvent& In)
{
	if (SelectedExpressions.Num() > 0)
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("MaterialEditorConvert")) );
		Material->Modify();
		TArray<UMaterialExpression*> ExpressionsToDelete;
		TArray<UMaterialExpression*> ExpressionsToSelect;
		for (INT ExpressionIndex = 0; ExpressionIndex < SelectedExpressions.Num(); ++ExpressionIndex)
		{
			// Look for the supported classes to convert from
			UMaterialExpression* CurrentSelectedExpression = SelectedExpressions(ExpressionIndex);
			UMaterialExpressionConstant* Constant1Expression = Cast<UMaterialExpressionConstant>(CurrentSelectedExpression);
			UMaterialExpressionConstant2Vector* Constant2Expression = Cast<UMaterialExpressionConstant2Vector>(CurrentSelectedExpression);
			UMaterialExpressionConstant3Vector* Constant3Expression = Cast<UMaterialExpressionConstant3Vector>(CurrentSelectedExpression);
			UMaterialExpressionConstant4Vector* Constant4Expression = Cast<UMaterialExpressionConstant4Vector>(CurrentSelectedExpression);
			UMaterialExpressionTextureSample* TextureSampleExpression = Cast<UMaterialExpressionTextureSample>(CurrentSelectedExpression);
			UMaterialExpressionComponentMask* ComponentMaskExpression = Cast<UMaterialExpressionComponentMask>(CurrentSelectedExpression);
			UMaterialExpressionParticleSubUV* ParticleSubUVExpression = Cast<UMaterialExpressionParticleSubUV>(CurrentSelectedExpression);
			UMaterialExpressionMeshSubUV* MeshSubUVExpression = Cast<UMaterialExpressionMeshSubUV>(CurrentSelectedExpression);
			UMaterialExpressionMeshSubUVBlend* MeshSubUVBlendExpression = Cast<UMaterialExpressionMeshSubUVBlend>(CurrentSelectedExpression);

			// Setup the class to convert to
			UClass* ClassToCreate = NULL;
			if (Constant1Expression)
			{
				ClassToCreate = UMaterialExpressionScalarParameter::StaticClass();
			}
			else if (Constant2Expression || Constant3Expression || Constant4Expression)
			{
				ClassToCreate = UMaterialExpressionVectorParameter::StaticClass();
			}
			else if (MeshSubUVBlendExpression) // Has to come before the MeshSubUV comparison...
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterMeshSubUVBlend::StaticClass();
			}
			else if (MeshSubUVExpression) // Has to come before the TextureSample comparison...
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterMeshSubUV::StaticClass();
			}
			else if (ParticleSubUVExpression) // Has to come before the TextureSample comparison...
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterSubUV::StaticClass();
			}
			else if (TextureSampleExpression && TextureSampleExpression->Texture && TextureSampleExpression->Texture->IsA(UTextureCube::StaticClass()))
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterCube::StaticClass();
			}	
			else if (TextureSampleExpression)
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameter2D::StaticClass();
			}	
			else if (ComponentMaskExpression)
			{
				ClassToCreate = UMaterialExpressionStaticComponentMaskParameter::StaticClass();
			}

			if (ClassToCreate)
			{
				UMaterialExpression* NewExpression = CreateNewMaterialExpression(ClassToCreate, FALSE, FALSE, FIntPoint(CurrentSelectedExpression->MaterialExpressionEditorX, CurrentSelectedExpression->MaterialExpressionEditorY));
				SwapLinksToExpression(CurrentSelectedExpression, NewExpression, Material);
				UBOOL bNeedsRefresh = FALSE;

				// Copy over expression-specific values
				if (Constant1Expression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionScalarParameter>(NewExpression)->DefaultValue = Constant1Expression->R;
				}
				else if (Constant2Expression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = FLinearColor(Constant2Expression->R, Constant2Expression->G, 0);
				}
				else if (Constant3Expression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = FLinearColor(Constant3Expression->R, Constant3Expression->G, Constant3Expression->B);
				}
				else if (Constant4Expression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = FLinearColor(Constant4Expression->R, Constant4Expression->G, Constant4Expression->B, Constant4Expression->A);
				}
				else if (TextureSampleExpression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionTextureSample>(NewExpression)->Texture = TextureSampleExpression->Texture;
				}
				else if (ComponentMaskExpression)
				{
					bNeedsRefresh = TRUE;
					UMaterialExpressionStaticComponentMaskParameter* ComponentMask = CastChecked<UMaterialExpressionStaticComponentMaskParameter>(NewExpression);
					ComponentMask->DefaultR = ComponentMaskExpression->R;
					ComponentMask->DefaultG = ComponentMaskExpression->G;
					ComponentMask->DefaultB = ComponentMaskExpression->B;
					ComponentMask->DefaultA = ComponentMaskExpression->A;
				}
				else if (MeshSubUVBlendExpression) // Has to come before the MeshSubUV comparison...
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionTextureSampleParameterMeshSubUVBlend>(NewExpression)->Texture = MeshSubUVBlendExpression->Texture;
				}
				else if (MeshSubUVExpression) // Has to come before the TextureSample comparison...
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionTextureSampleParameterMeshSubUV>(NewExpression)->Texture = MeshSubUVExpression->Texture;
				}
				else if (ParticleSubUVExpression)
				{
					bNeedsRefresh = TRUE;
					CastChecked<UMaterialExpressionTextureSampleParameterSubUV>(NewExpression)->Texture = ParticleSubUVExpression->Texture;
				}

				if (bNeedsRefresh)
				{
					// Refresh the expression preview if we changed its properties after it was created
					NewExpression->bNeedToUpdatePreview = TRUE;
					RefreshExpressionPreview( NewExpression, TRUE );
				}

				ExpressionsToDelete.AddUniqueItem(CurrentSelectedExpression);
				ExpressionsToSelect.AddItem(NewExpression);
			}
		}
		// Delete the replaced expressions
		TArray< UMaterialExpressionComment *> Comments;
		TArray< UMaterialExpressionCompound *> Compounds;

		DeleteObjects(ExpressionsToDelete, Comments, Compounds );

		// Select each of the newly converted expressions
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		// If only one expression was converted, select it's "Parameter Name" property in the property window
		if (ExpressionsToSelect.Num() == 1)
		{
			// Update the property window so that it displays the properties for the new expression
			UpdatePropertyWindow();

			FPropertyNode* ParamNameProp = PropertyWindow->FindPropertyNode(TEXT("ParameterName"));

			if (ParamNameProp)
			{
				WxPropertyControl* PropertyNodeWindow = ParamNameProp->GetNodeWindow();
				
				check( PropertyNodeWindow );
				check( PropertyNodeWindow->InputProxy );

				// Set the focus to the "Parameter Name" property
				PropertyWindow->SetActiveFocus(PropertyNodeWindow, TRUE);
				PropertyNodeWindow->InputProxy->OnSetFocus(PropertyNodeWindow);
			}
		}
	}
}

void WxMaterialEditor::OnSelectDownsteamNodes(wxCommandEvent& In)
{
	TArray<UMaterialExpression*> ExpressionsToEvaluate;	// Graph nodes that need to be traced downstream
	TArray<UMaterialExpression*> ExpressionsEvalated;	// Keep track of evaluated graph nodes so we don't re-evaluate them
	TArray<UMaterialExpression*> ExpressionsToSelect;	// Downstream graph nodes that will end up being selected

	// Add currently selected graph nodes to the "need to be traced downstream" list
	for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(SelectedExpressions); ExpressionIter; ++ExpressionIter )
	{
		ExpressionsToEvaluate.AddItem(*ExpressionIter);
	}

	// Generate a list of downstream nodes
	while (ExpressionsToEvaluate.Num() > 0)
	{
		UMaterialExpression* CurrentExpression = ExpressionsToEvaluate.Last();
		if (CurrentExpression)
		{
			TArray<FExpressionOutput> Outputs;
			CurrentExpression->GetOutputs(Outputs);
			
			for (INT OutputIndex = 0; OutputIndex < Outputs.Num(); OutputIndex++)
			{
				const FExpressionOutput& CurrentOutput = Outputs(OutputIndex);
				TArray<FMaterialExpressionReference> ReferencingInputs;
				GetListOfReferencingInputs(CurrentExpression, Material, ReferencingInputs, &CurrentOutput);

				for (INT ReferenceIndex = 0; ReferenceIndex < ReferencingInputs.Num(); ReferenceIndex++)
				{
					FMaterialExpressionReference& CurrentReference = ReferencingInputs(ReferenceIndex);
					if (CurrentReference.Expression)
					{
						INT index = -1;
						ExpressionsEvalated.FindItem(CurrentReference.Expression, index);

						if (index < 0)
						{
							// This node is a downstream node (so, we'll need to select it) and it's children need to be traced as well
							ExpressionsToSelect.AddItem(CurrentReference.Expression);
							ExpressionsToEvaluate.AddItem(CurrentReference.Expression);
						}
					}
				}
			}
		}

		// This graph node has now been examined
		ExpressionsEvalated.AddItem(CurrentExpression);
		ExpressionsToEvaluate.RemoveItem(CurrentExpression);
	}

	// Select all downstream nodes
	if (ExpressionsToSelect.Num() > 0)
	{
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		UpdatePropertyWindow();
	}
}

void WxMaterialEditor::OnSelectUpsteamNodes(wxCommandEvent& In)
{
	TArray<UMaterialExpression*> ExpressionsToEvaluate;	// Graph nodes that need to be traced upstream
	TArray<UMaterialExpression*> ExpressionsEvalated;	// Keep track of evaluated graph nodes so we don't re-evaluate them
	TArray<UMaterialExpression*> ExpressionsToSelect;	// Upstream graph nodes that will end up being selected

	// Add currently selected graph nodes to the "need to be traced upstream" list
	for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(SelectedExpressions); ExpressionIter; ++ExpressionIter )
	{
		ExpressionsToEvaluate.AddItem(*ExpressionIter);
	}

	// Generate a list of upstream nodes
	while (ExpressionsToEvaluate.Num() > 0)
	{
		UMaterialExpression* CurrentExpression = ExpressionsToEvaluate.Last();
		if (CurrentExpression)
		{
			const TArray<FExpressionInput*>& Intputs = CurrentExpression->GetInputs();

			for (INT InputIndex = 0; InputIndex < Intputs.Num(); InputIndex++)
			{
				const FExpressionInput* CurrentInput = Intputs(InputIndex);
				if (CurrentInput->Expression)
				{
					INT index = -1;
					ExpressionsEvalated.FindItem(CurrentInput->Expression, index);

					if (index < 0)
					{
						// This node is a upstream node (so, we'll need to select it) and it's children need to be traced as well
						ExpressionsToSelect.AddItem(CurrentInput->Expression);
						ExpressionsToEvaluate.AddItem(CurrentInput->Expression);
					}
				}
			}
		}

		// This graph node has now been examined
		ExpressionsEvalated.AddItem(CurrentExpression);
		ExpressionsToEvaluate.RemoveItem(CurrentExpression);
	}

	// Select all upstream nodes
	if (ExpressionsToSelect.Num() > 0)
	{
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		UpdatePropertyWindow();
	}
}

void WxMaterialEditor::OnDeleteObjects(wxCommandEvent& In)
{
	DeleteSelectedObjects();
}

void WxMaterialEditor::OnBreakLink(wxCommandEvent& In)
{
	BreakConnLink(FALSE);
}

void WxMaterialEditor::OnBreakAllLinks(wxCommandEvent& In)
{
	BreakAllConnectionsToSelectedExpressions();
}

void WxMaterialEditor::OnRemoveFromFavorites(wxCommandEvent& In)
{
	RemoveSelectedExpressionFromFavorites();
}

void WxMaterialEditor::OnAddToFavorites(wxCommandEvent& In)
{
	AddSelectedExpressionToFavorites();
}

/**
 * Called when clicking the "Preview Node on Mesh" option in a material expression context menu.
 */
void WxMaterialEditor::OnDebugNode(wxCommandEvent &In)
{
	if( DebugExpression == SelectedExpressions(0) )
	{
		// If we are already debugging the selected expression toggle debugging off
		DebugExpression = NULL;
		SetPreviewMaterial( Material );
		// Recompile the preview material to get changes that might have been made during debugging
		UpdatePreviewMaterial();
	}
	else if ( SelectedExpressions.Num() == 1 )
	{
		if( DebugMaterial == NULL )
		{
			// Create the debug material if it hasnt already been created
			DebugMaterial = (UMaterial*)UObject::StaticConstructObject( UMaterial::StaticClass(), UObject::GetTransientPackage(), NAME_None, RF_Public);
			DebugMaterial->bIsPreviewMaterial = TRUE;
		}
		
		// The preview window should now show the debug material
		SetPreviewMaterial( DebugMaterial );
		
		// Connect the selected expression to the emissive node of the debug material.  The emissive material is not affected by light which is why its a good choice.
		ConnectMaterialToMaterialExpression( DebugMaterial, 2, SelectedExpressions(0), 0, FALSE );
		// Set the debug expression
		DebugExpression = SelectedExpressions(0);
		
		// Recompile the preview material
		UpdatePreviewMaterial();
	}
}

void WxMaterialEditor::OnConnectToMaterial_DiffuseColor(wxCommandEvent& In) { OnConnectToMaterial(0); }
void WxMaterialEditor::OnConnectToMaterial_DiffusePower(wxCommandEvent& In) { OnConnectToMaterial(1); }
void WxMaterialEditor::OnConnectToMaterial_EmissiveColor(wxCommandEvent& In) { OnConnectToMaterial(2); }
void WxMaterialEditor::OnConnectToMaterial_SpecularColor(wxCommandEvent& In) { OnConnectToMaterial(3); }
void WxMaterialEditor::OnConnectToMaterial_SpecularPower(wxCommandEvent& In) { OnConnectToMaterial(4); }
void WxMaterialEditor::OnConnectToMaterial_Opacity(wxCommandEvent& In) { OnConnectToMaterial(5); }
void WxMaterialEditor::OnConnectToMaterial_OpacityMask(wxCommandEvent& In) { OnConnectToMaterial(6); }
void WxMaterialEditor::OnConnectToMaterial_Distortion(wxCommandEvent& In) { OnConnectToMaterial(7);	}
void WxMaterialEditor::OnConnectToMaterial_TransmissionMask(wxCommandEvent& In) { OnConnectToMaterial(8);	}
void WxMaterialEditor::OnConnectToMaterial_TransmissionColor(wxCommandEvent& In) { OnConnectToMaterial(9);	}
void WxMaterialEditor::OnConnectToMaterial_Normal(wxCommandEvent& In) { OnConnectToMaterial(10);	}
void WxMaterialEditor::OnConnectToMaterial_CustomLighting(wxCommandEvent& In) { OnConnectToMaterial(11);	}
void WxMaterialEditor::OnConnectToMaterial_CustomLightingDiffuse(wxCommandEvent& In) { OnConnectToMaterial(12);	}
void WxMaterialEditor::OnConnectToMaterial_AnisotropicDirection(wxCommandEvent& In) { OnConnectToMaterial(13);	}
void WxMaterialEditor::OnConnectToMaterial_WorldPositionOffset(wxCommandEvent& In) { OnConnectToMaterial(14);	}
void WxMaterialEditor::OnConnectToMaterial_WorldDisplacement(wxCommandEvent& In) { OnConnectToMaterial(15);	}
void WxMaterialEditor::OnConnectToMaterial_TessellationFactors(wxCommandEvent& In) { OnConnectToMaterial(16);	}
void WxMaterialEditor::OnConnectToMaterial_SubsurfaceInscatteringColor(wxCommandEvent& In) { OnConnectToMaterial(17); }
void WxMaterialEditor::OnConnectToMaterial_SubsurfaceAbsorptionColor(wxCommandEvent& In) { OnConnectToMaterial(18); }
void WxMaterialEditor::OnConnectToMaterial_SubsurfaceScatteringRadius(wxCommandEvent& In) { OnConnectToMaterial(19); }

void WxMaterialEditor::OnShowHideConnectors(wxCommandEvent& In)
{
	bHideUnusedConnectors = !bHideUnusedConnectors;
	RefreshExpressionViewport();
}


void WxMaterialEditor::OnRealTimeExpressions(wxCommandEvent& In)
{
	LinkedObjVC->ToggleRealtime();
}

void WxMaterialEditor::OnAlwaysRefreshAllPreviews(wxCommandEvent& In)
{
	bAlwaysRefreshAllPreviews = In.IsChecked() ? TRUE : FALSE;
	if ( bAlwaysRefreshAllPreviews )
	{
		RefreshExpressionPreviews();
	}
	RefreshExpressionViewport();
}

void WxMaterialEditor::OnApply(wxCommandEvent& In)
{
	UpdateOriginalMaterial();
}

//forward declaration for texture flattening
void ConditionalFlattenMaterial(UMaterialInterface* MaterialInterface, UBOOL bReflattenAutoFlattened, const UBOOL bInForceFlatten);

void WxMaterialEditor::OnFlatten(wxCommandEvent& In)
{
	//make sure the changes have been committed
	UpdateOriginalMaterial();

	//bake out the flattened texture using the newly committed original material (for naming)
	const UBOOL bReflattenAutoFlattened = TRUE;
	const UBOOL bForceFlatten = TRUE;
	ConditionalFlattenMaterial(OriginalMaterial, bReflattenAutoFlattened, bForceFlatten);
}


void WxMaterialEditor::OnToggleStats(wxCommandEvent& In)
{
	// Toggle the showing of material stats each time the user presses the show stats button
	bShowStats = !bShowStats;
	RefreshExpressionViewport();
}

void WxMaterialEditor::UI_ToggleStats(wxUpdateUIEvent& In)
{
	In.Check( bShowStats == TRUE );
}

void WxMaterialEditor::OnViewSource(wxCommandEvent& In)
{
	FDockingParent::FDockWindowState SourceWindowState;
	if( GetDockingWindowState(SourceWindow, SourceWindowState) )
	{
		ShowDockingWindow( SourceWindow, !SourceWindowState.bIsVisible );
	}
}

void WxMaterialEditor::UI_ViewSource(wxUpdateUIEvent& In)
{
	FDockingParent::FDockWindowState SourceWindowState;
	if( GetDockingWindowState(SourceWindow, SourceWindowState) )
	{
		In.Check( SourceWindowState.bIsVisible ? TRUE : FALSE );
	}
}

void WxMaterialEditor::OnSearchChanged(wxCommandEvent& In)
{
	SearchQuery = In.GetString().c_str();
	UpdateSearch(TRUE);
}

void WxMaterialEditor::ShowSearchResult()
{
	if( SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() )
	{
		UMaterialExpression* Expression = SearchResults(SelectedSearchResult);

		// Select the selected search item
		EmptySelection();
		AddToSelection(Expression);
		UpdatePropertyWindow();

		PanLocationOnscreen( Expression->MaterialExpressionEditorX+50, Expression->MaterialExpressionEditorY+50, 100 );
	}
}

/**
* PanLocationOnscreen: pan the viewport if necessary to make the particular location visible
*
* @param	X, Y		Coordinates of the location we want onscreen
* @param	Border		Minimum distance we want the location from the edge of the screen.
*/
void WxMaterialEditor::PanLocationOnscreen( INT LocationX, INT LocationY, INT Border )
{
	// Find the bound of the currently visible area of the expression viewport
	INT X1 = -LinkedObjVC->Origin2D.X + appRound((FLOAT)Border*LinkedObjVC->Zoom2D);
	INT Y1 = -LinkedObjVC->Origin2D.Y + appRound((FLOAT)Border*LinkedObjVC->Zoom2D);
	INT X2 = -LinkedObjVC->Origin2D.X + LinkedObjVC->Viewport->GetSizeX() - appRound((FLOAT)Border*LinkedObjVC->Zoom2D);
	INT Y2 = -LinkedObjVC->Origin2D.Y + LinkedObjVC->Viewport->GetSizeY() - appRound((FLOAT)Border*LinkedObjVC->Zoom2D);

	INT X = appRound(((FLOAT)LocationX) * LinkedObjVC->Zoom2D);
	INT Y = appRound(((FLOAT)LocationY) * LinkedObjVC->Zoom2D);

	// See if we need to pan the viewport to show the selected search result.
	LinkedObjVC->DesiredOrigin2D = LinkedObjVC->Origin2D;
	UBOOL bChanged = FALSE;
	if( X < X1 )
	{
		LinkedObjVC->DesiredOrigin2D.X += (X1 - X);
		bChanged = TRUE;
	}
	if( Y < Y1 )
	{
		LinkedObjVC->DesiredOrigin2D.Y += (Y1 - Y);
		bChanged = TRUE;
	}
	if( X > X2 )
	{
		LinkedObjVC->DesiredOrigin2D.X -= (X - X2);
		bChanged = TRUE;
	}
	if( Y > Y2 )
	{
		LinkedObjVC->DesiredOrigin2D.Y -= (Y - Y2);
		bChanged = TRUE;
	}
	if( bChanged )
	{
		// Pan to the result in 0.1 seconds
		LinkedObjVC->DesiredPanTime = 0.1f;
	}
	RefreshExpressionViewport();
}

/**
* Comparison function used to sort search results
*/
IMPLEMENT_COMPARE_POINTER(UMaterialExpression,MaterialEditorSearch,
{ 
	// Divide into grid cells and step horizontally and then vertically.
	INT AGridX = A->MaterialExpressionEditorX / 100;
	INT AGridY = A->MaterialExpressionEditorY / 100;
	INT BGridX = B->MaterialExpressionEditorX / 100;
	INT BGridY = B->MaterialExpressionEditorY / 100;

	if( AGridY < BGridY )
	{
		return -1;
	}
	else
	if( AGridY > BGridY )
	{
		return 1;
	}
	else
	{
		return AGridX - BGridX;
	}
} )

/**
* Updates the SearchResults array based on the search query
*
* @param	bQueryChanged		Indicates whether the update is because of a query change or a potential material content change.
*/
void WxMaterialEditor::UpdateSearch( UBOOL bQueryChanged )
{
	SearchResults.Empty();

	if( SearchQuery.Len() == 0 )
	{
		if( bQueryChanged )
		{
			// We just cleared the search
			SelectedSearchResult = 0;
			ToolBar->SearchControl.EnableNextPrev(FALSE,FALSE);
			RefreshExpressionViewport();
		}
	}
	else
	{
		// Search expressions
		for( INT Index=0;Index<Material->Expressions.Num();Index++ )
		{
			if(Material->Expressions(Index)->MatchesSearchQuery(*SearchQuery) )
			{
				SearchResults.AddItem(Material->Expressions(Index));
			}
		}

		// Search comments
		for( INT Index=0;Index<Material->EditorComments.Num();Index++ )
		{
			if(Material->EditorComments(Index)->MatchesSearchQuery(*SearchQuery) )
			{
				SearchResults.AddItem(Material->EditorComments(Index));
			}
		}

		Sort<USE_COMPARE_POINTER(UMaterialExpression,MaterialEditorSearch)>( &SearchResults(0), SearchResults.Num() );

		if( bQueryChanged )
		{
			// This is a new query rather than a material change, so navigate to first search result.
			SelectedSearchResult = 0;
			ShowSearchResult();
		}
		else
		{
			if( SelectedSearchResult < 0 || SelectedSearchResult >= SearchResults.Num() )
			{
				SelectedSearchResult = 0;
			}
		}

		ToolBar->SearchControl.EnableNextPrev(SearchResults.Num()>0, SearchResults.Num()>0);
	}
}


void WxMaterialEditor::OnSearchNext(wxCommandEvent& In)
{
	SelectedSearchResult++;
	if( SelectedSearchResult >= SearchResults.Num() )
	{
		SelectedSearchResult = 0;
	}
	ShowSearchResult();
}

void WxMaterialEditor::OnSearchPrev(wxCommandEvent& In)
{
	SelectedSearchResult--;
	if( SelectedSearchResult < 0 )
	{
		SelectedSearchResult = Max<INT>(SearchResults.Num()-1,0);
	}
	ShowSearchResult();
}


/**
 * Routes the event to the appropriate handlers
 *
 * @param InType the event that was fired
 */
void WxMaterialEditor::Send(ECallbackEventType InType)
{
	switch( InType )
	{
		case CALLBACK_ColorPickerChanged:
			ForceRefreshExpressionPreviews();
			PropertyWindow->Rebuild();
			break;

		case CALLBACK_PreEditorClose:
			if ( bMaterialDirty )
			{
				// find out the user wants to do with this dirty material
				switch( appMsgf(AMT_YesNo, *LocalizeUnrealEd("Prompt_MaterialEditorClose")) )
				{
				case ART_Yes:
					UpdateOriginalMaterial();
					break;
				}
			}
			break;
	}
}

void WxMaterialEditor::UI_RealTimeExpressions(wxUpdateUIEvent& In)
{
	In.Check( LinkedObjVC->IsRealtime() == TRUE );
}

void WxMaterialEditor::UI_AlwaysRefreshAllPreviews(wxUpdateUIEvent& In)
{
	In.Check( bAlwaysRefreshAllPreviews == TRUE );
}

void WxMaterialEditor::UI_HideUnusedConnectors(wxUpdateUIEvent& In)
{
	In.Check( bHideUnusedConnectors == TRUE );
}


void WxMaterialEditor::UI_Apply(wxUpdateUIEvent& In)
{
	In.Enable(bMaterialDirty == TRUE);
}

void WxMaterialEditor::OnCameraHome(wxCommandEvent& In)
{
	LinkedObjVC->Origin2D = FIntPoint(-Material->EditorX,-Material->EditorY);
	RefreshExpressionViewport();
}

void WxMaterialEditor::OnCleanUnusedExpressions(wxCommandEvent& In)
{
	CleanUnusedExpressions();
}

void WxMaterialEditor::OnSetPreviewMeshFromSelection(wxCommandEvent& In)
{
	UBOOL bFoundPreviewMesh = FALSE;
	GCallbackEvent->Send(CALLBACK_LoadSelectedAssetsIfNeeded);

	// Look for a selected static mesh.
	UStaticMesh* SelectedStaticMesh = GEditor->GetSelectedObjects()->GetTop<UStaticMesh>();
	if ( SelectedStaticMesh )
	{
		SetPreviewMesh( SelectedStaticMesh, NULL );
		Material->PreviewMesh = SelectedStaticMesh->GetPathName();
		bFoundPreviewMesh = TRUE;
	}
	else
	{
		// No static meshes were selected; look for a selected skeletal mesh.
		USkeletalMesh* SelectedSkeletalMesh = GEditor->GetSelectedObjects()->GetTop<USkeletalMesh>();
		if ( SelectedSkeletalMesh && SetPreviewMesh( NULL, SelectedSkeletalMesh ) )
		{
			Material->PreviewMesh = SelectedSkeletalMesh->GetPathName();
			bFoundPreviewMesh = TRUE;
		}
	}

	if ( bFoundPreviewMesh )
	{
		Material->MarkPackageDirty();
		RefreshPreviewViewport();
		bMaterialDirty = TRUE;
	}
}

void WxMaterialEditor::OnMaterialExpressionTreeDrag(wxTreeEvent& DragEvent)
{
	const FString SelectedString( MaterialExpressionList->GetSelectedTreeString(DragEvent) );
	if ( SelectedString.Len() > 0 )
	{
		wxTextDataObject DataObject( *SelectedString );
		wxDropSource DragSource( this );
		DragSource.SetData( DataObject );
		DragSource.DoDragDrop( TRUE );
	}
}

void WxMaterialEditor::OnMaterialExpressionListDrag(wxListEvent& In)
{
	const FString SelectedString( MaterialExpressionList->GetSelectedListString() );
	if ( SelectedString.Len() > 0 )
	{
		wxTextDataObject DataObject( *SelectedString );
		wxDropSource DragSource( this );
		DragSource.SetData( DataObject );
		DragSource.DoDragDrop( TRUE );
	}
}

/**
 * Retrieves all visible parameters within the material.
 *
 * @param	Material			The material to retrieve the parameters from.
 * @param	MaterialInstance	The material instance that contains all parameter overrides.
 * @param	VisisbleExpressions	The array that will contain the id's of the visible parameter expressions.
 */
void WxMaterialEditor::GetVisibleMaterialParameters(const UMaterial *Material, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions)
{
	VisibleExpressions.Empty();

	InitMaterialExpressionClasses();

	TArray<UMaterialExpression*> ProcessedExpressions;
	GetVisibleMaterialParametersFromExpression(Material->DiffuseColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->DiffusePower.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->SpecularColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->SpecularPower.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->Normal.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->EmissiveColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->Opacity.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->OpacityMask.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->Distortion.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->CustomLighting.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->CustomSkylightDiffuse.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->AnisotropicDirection.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->TwoSidedLightingMask.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->TwoSidedLightingColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->WorldPositionOffset.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->WorldDisplacement.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->TessellationFactors.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->SubsurfaceInscatteringColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->SubsurfaceAbsorptionColor.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	GetVisibleMaterialParametersFromExpression(Material->SubsurfaceScatteringRadius.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);

}

/**
 *	Checks if the given expression is in the favorites list...
 *
 *	@param	InExpression	The expression to check for.
 *
 *	@return	UBOOL			TRUE if it's in the list, FALSE if not.
 */
UBOOL WxMaterialEditor::IsMaterialExpressionInFavorites(UMaterialExpression* InExpression)
{
	for (INT CheckIndex = 0; CheckIndex < FavoriteExpressionClasses.Num(); CheckIndex++)
	{
		if (FavoriteExpressionClasses(CheckIndex) == InExpression->GetClass())
		{
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * Recursively walks the expression tree and parses the visible expression branches.
 *
 * @param	MaterialExpression	The expression to parse.
 * @param	MaterialInstance	The material instance that contains all parameter overrides.
 * @param	VisisbleExpressions	The array that will contain the id's of the visible parameter expressions.
 */
void WxMaterialEditor::GetVisibleMaterialParametersFromExpression(UMaterialExpression *MaterialExpression, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions, TArray<UMaterialExpression*> &ProcessedExpressions)
{
	if(!MaterialExpression)
	{
		return;
	}

	check(MaterialInstance);

	//don't allow re-entrant expressions to continue
	if (ProcessedExpressions.ContainsItem(MaterialExpression))
	{
		return;
	}
	ProcessedExpressions.Push(MaterialExpression);

	// if it's a material parameter it must be visible so add it to the map
	UMaterialExpressionParameter *Param = Cast<UMaterialExpressionParameter>(MaterialExpression);
	UMaterialExpressionTextureSampleParameter *TexParam = Cast<UMaterialExpressionTextureSampleParameter>(MaterialExpression);
	UMaterialExpressionFontSampleParameter *FontParam = Cast<UMaterialExpressionFontSampleParameter>(MaterialExpression);
	if(Param)
	{
		VisibleExpressions.AddUniqueItem(Param->ExpressionGUID);
	}
	else if(TexParam)
	{
		VisibleExpressions.AddUniqueItem(TexParam->ExpressionGUID);
	}
	else if(FontParam)
	{
		VisibleExpressions.AddUniqueItem(FontParam->ExpressionGUID);
	}

	// check if it's a switch expression and branch according to its value
	UMaterialExpressionStaticSwitchParameter *Switch = Cast<UMaterialExpressionStaticSwitchParameter>(MaterialExpression);
	if(Switch)
	{
		UBOOL Value = FALSE;
		FGuid ExpressionID;
		MaterialInstance->GetStaticSwitchParameterValue(Switch->ParameterName, Value, ExpressionID);

		if(Value)
		{
			GetVisibleMaterialParametersFromExpression(Switch->A.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
		}
		else
		{
			GetVisibleMaterialParametersFromExpression(Switch->B.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
		}
	}
	else
	{
		const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
		for(INT ExpressionInputIndex = 0; ExpressionInputIndex < ExpressionInputs.Num(); ExpressionInputIndex++)
		{
			//retrieve the expression input and then start parsing its children
			FExpressionInput* Input = ExpressionInputs(ExpressionInputIndex);
			GetVisibleMaterialParametersFromExpression(Input->Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
		}
	}

	UMaterialExpression* TopExpression = ProcessedExpressions.Pop();
	//ensure that the top of the stack matches what we expect (the same as MaterialExpression)
	check(MaterialExpression == TopExpression);
}

/**
* Run the HLSL material translator and refreshes the View Source window.
*/
void WxMaterialEditor::RefreshSourceWindowMaterial()
{
	SourceWindow->RefreshWindow();
}

/**
 * Recompiles the material used the preview window.
 */
void WxMaterialEditor::UpdatePreviewMaterial( )
{
	if( DebugExpression )
	{
		// If we are debugging and expression, update the debug material
		DebugMaterial->PreEditChange( NULL );
		DebugMaterial->PostEditChange();
	}
	else 
	{
		// Update the regular preview material when not debugging an expression.
		Material->PreEditChange( NULL );
		Material->PostEditChange();
	}

	// Reattach all components that use the preview material, since UMaterial::PEC does not reattach components using a bIsPreviewMaterial=true material
	RefreshPreviewViewport();
}
