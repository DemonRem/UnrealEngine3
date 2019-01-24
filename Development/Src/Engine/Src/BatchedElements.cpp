/*=============================================================================
	BatchedElements.cpp: Batched element rendering.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "BatchedElements.h"

/**
 * Simple pixel shader that just reads from a texture
 * This is used for resolves and needs to be as efficient as possible
 */
class FSimpleElementPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FSimpleElementPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap,TEXT("_Texture"));
		TextureComponentReplicateParameter.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicate"),TRUE);
		TextureComponentReplicateAlphaParameter.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicateAlpha"));
	}
	FSimpleElementPixelShader() {}

	void SetParameters(const FTexture* Texture)
	{
		SetTextureParameter(GetPixelShader(),TextureParameter,Texture);
		SetPixelShaderValue(GetPixelShader(),TextureComponentReplicateParameter,Texture->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,0));
		SetPixelShaderValue(GetPixelShader(),TextureComponentReplicateAlphaParameter,Texture->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,1));
		RHISetRenderTargetBias(appPow(2.0f,GCurrentColorExpBias));
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << TextureParameter;
		Ar << TextureComponentReplicateParameter;
		Ar << TextureComponentReplicateAlphaParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter TextureParameter;
	FShaderParameter TextureComponentReplicateParameter;
	FShaderParameter TextureComponentReplicateAlphaParameter;
};

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementGammaPixelShader : public FSimpleElementPixelShader
{
	DECLARE_SHADER_TYPE(FSimpleElementGammaPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FSimpleElementGammaPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FSimpleElementPixelShader(Initializer)
	{
		GammaParameter.Bind(Initializer.ParameterMap,TEXT("Gamma"));
	}
	FSimpleElementGammaPixelShader() {}

	void SetParameters(const FTexture* Texture,FLOAT Gamma,ESimpleElementBlendMode BlendMode)
	{
		FSimpleElementPixelShader::SetParameters(Texture);
		SetPixelShaderValue(GetPixelShader(),GammaParameter,Gamma);
		RHISetRenderTargetBias( (BlendMode == SE_BLEND_Modulate) ? 1.0f : appPow(2.0f,GCurrentColorExpBias));
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FSimpleElementPixelShader::Serialize(Ar);
		Ar << GammaParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter GammaParameter;
};

/**
 * A pixel shader for rendering a masked texture on a simple element.
 */
class FSimpleElementMaskedGammaPixelShader : public FSimpleElementGammaPixelShader
{
	DECLARE_SHADER_TYPE(FSimpleElementMaskedGammaPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FSimpleElementMaskedGammaPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FSimpleElementGammaPixelShader(Initializer)
	{
		ClipRefParameter.Bind(Initializer.ParameterMap,TEXT("ClipRef"));
	}
	FSimpleElementMaskedGammaPixelShader() {}

	void SetParameters(const FTexture* Texture,FLOAT Gamma,FLOAT ClipRef,ESimpleElementBlendMode BlendMode)
	{
		FSimpleElementGammaPixelShader::SetParameters(Texture,Gamma,BlendMode);
		SetPixelShaderValue(GetPixelShader(),ClipRefParameter,ClipRef);
		RHISetRenderTargetBias((BlendMode == SE_BLEND_Modulate) ? 1.0f : appPow(2.0f,GCurrentColorExpBias));
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FSimpleElementGammaPixelShader::Serialize(Ar);
		Ar << ClipRefParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter ClipRefParameter;
};

/**
* A pixel shader for rendering a masked texture using signed distance filed for alpha on a simple element.
*/
class FSimpleElementDistanceFieldGammaPixelShader : public FSimpleElementMaskedGammaPixelShader
{
	DECLARE_SHADER_TYPE(FSimpleElementDistanceFieldGammaPixelShader,Global);
public:

	/**
	* Determine if this shader should be compiled
	*
	* @param Platform - current shader platform being compiled
	* @return TRUE if this shader should be cached for the given platform
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	/**
	* Constructor
	*
	* @param Initializer - shader initialization container
	*/
	FSimpleElementDistanceFieldGammaPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FSimpleElementMaskedGammaPixelShader(Initializer)
	{
		SmoothWidthParameter.Bind(Initializer.ParameterMap,TEXT("SmoothWidth"),TRUE);
		EnableShadowParameter.Bind(Initializer.ParameterMap,TEXT("EnableShadow"),TRUE);
		ShadowDirectionParameter.Bind(Initializer.ParameterMap,TEXT("ShadowDirection"),TRUE);
		ShadowColorParameter.Bind(Initializer.ParameterMap,TEXT("ShadowColor"),TRUE);
		ShadowSmoothWidthParameter.Bind(Initializer.ParameterMap,TEXT("ShadowSmoothWidth"),TRUE);
		EnableGlowParameter.Bind(Initializer.ParameterMap,TEXT("EnableGlow"),TRUE);
		GlowColorParameter.Bind(Initializer.ParameterMap,TEXT("GlowColor"),TRUE);
		GlowOuterRadiusParameter.Bind(Initializer.ParameterMap,TEXT("GlowOuterRadius"),TRUE);
		GlowInnerRadiusParameter.Bind(Initializer.ParameterMap,TEXT("GlowInnerRadius"),TRUE);
	}
	/**
	* Default constructor
	*/
	FSimpleElementDistanceFieldGammaPixelShader() {}

	/**
	* Sets all the constant parameters for this shader
	*
	* @param Texture - 2d tile texture
	* @param Gamma - if gamma != 1.0 then a pow(color,Gamma) is applied
	* @param ClipRef - reference value to compare with alpha for killing pixels
	* @param SmoothWidth - The width to smooth the edge the texture
	* @param EnableShadow - Toggles drop shadow rendering
	* @param ShadowDirection - 2D vector specifying the direction of shadow
	* @param ShadowColor - Color of the shadowed pixels
	* @param ShadowSmoothWidth - The width to smooth the edge the shadow of the texture
	* @param BlendMode - current batched element blend mode being rendered
	*/
	void SetParameters(
		const FTexture* Texture,
		FLOAT Gamma,
		FLOAT ClipRef,
		FLOAT SmoothWidth,
		UBOOL EnableShadow,
		const FVector2D& ShadowDirection,
		const FLinearColor& ShadowColor,
		FLOAT ShadowSmoothWidth,
		const FDepthFieldGlowInfo& GlowInfo,
		ESimpleElementBlendMode BlendMode
		)
	{
		FSimpleElementMaskedGammaPixelShader::SetParameters(Texture,Gamma,ClipRef,BlendMode);
		SetPixelShaderValue(GetPixelShader(),SmoothWidthParameter,SmoothWidth);		
		SetPixelShaderBool(GetPixelShader(),EnableShadowParameter,EnableShadow);
		if (EnableShadow)
		{
			SetPixelShaderValue(GetPixelShader(),ShadowDirectionParameter,ShadowDirection);
			SetPixelShaderValue(GetPixelShader(),ShadowColorParameter,ShadowColor);
			SetPixelShaderValue(GetPixelShader(),ShadowSmoothWidthParameter,ShadowSmoothWidth);
		}
		SetPixelShaderBool(GetPixelShader(),EnableGlowParameter,GlowInfo.bEnableGlow);
		if (GlowInfo.bEnableGlow)
		{
			SetPixelShaderValue(GetPixelShader(),GlowColorParameter,GlowInfo.GlowColor);
			SetPixelShaderValue(GetPixelShader(),GlowOuterRadiusParameter,GlowInfo.GlowOuterRadius);
			SetPixelShaderValue(GetPixelShader(),GlowInnerRadiusParameter,GlowInfo.GlowInnerRadius);
		}
	}

	/**
	* Serialize constant paramaters for this shader
	* 
	* @param Ar - archive to serialize to
	* @return TRUE if any of the parameters were outdated
	*/
	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FSimpleElementMaskedGammaPixelShader::Serialize(Ar);
		Ar << SmoothWidthParameter;
		Ar << EnableShadowParameter;
		Ar << ShadowDirectionParameter;
		Ar << ShadowColorParameter;	
		Ar << ShadowSmoothWidthParameter;
		Ar << EnableGlowParameter;
		Ar << GlowColorParameter;
		Ar << GlowOuterRadiusParameter;
		Ar << GlowInnerRadiusParameter;
		return bShaderHasOutdatedParameters;
	}
	
private:
	/** The width to smooth the edge the texture */
	FShaderParameter SmoothWidthParameter;
	/** Toggles drop shadow rendering */
	FShaderParameter EnableShadowParameter;
	/** 2D vector specifying the direction of shadow */
	FShaderParameter ShadowDirectionParameter;
	/** Color of the shadowed pixels */
	FShaderParameter ShadowColorParameter;	
	/** The width to smooth the edge the shadow of the texture */
	FShaderParameter ShadowSmoothWidthParameter;
	/** whether to turn on the outline glow */
	FShaderParameter EnableGlowParameter;
	/** base color to use for the glow */
	FShaderParameter GlowColorParameter;
	/** outline glow outer radius */
	FShaderParameter GlowOuterRadiusParameter;
	/** outline glow inner radius */
	FShaderParameter GlowInnerRadiusParameter;
};

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementHitProxyPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FSimpleElementHitProxyPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsPCPlatform(Platform); 
	}


	FSimpleElementHitProxyPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap,TEXT("_Texture"));
	}
	FSimpleElementHitProxyPixelShader() {}

	void SetParameters(const FTexture* Texture)
	{
		SetTextureParameter(GetPixelShader(),TextureParameter,Texture);
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << TextureParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter TextureParameter;
};

/**
 * A vertex shader for rendering a texture on a simple element.
 */
class FSimpleElementVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(FSimpleElementVertexShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FSimpleElementVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		TransformParameter.Bind(Initializer.ParameterMap,TEXT("Transform"));
	}
	FSimpleElementVertexShader() {}

	void SetParameters(const FMatrix& Transform)
	{
		SetVertexShaderValue(GetVertexShader(),TransformParameter,Transform);
	}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << TransformParameter;

		// set parameter names for platforms that need them
		TransformParameter.SetShaderParamName(TEXT("Transform"));

		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter TransformParameter;
};

// Shader implementations.
IMPLEMENT_SHADER_TYPE(,FSimpleElementPixelShader,TEXT("SimpleElementPixelShader"),TEXT("Main"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_VER0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementGammaPixelShader,TEXT("SimpleElementPixelShader"),TEXT("GammaMain"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_VER0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementMaskedGammaPixelShader,TEXT("SimpleElementPixelShader"),TEXT("GammaMaskedMain"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_VER0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementDistanceFieldGammaPixelShader,TEXT("SimpleElementPixelShader"),TEXT("GammaDistanceFieldMain"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_VER0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementHitProxyPixelShader,TEXT("SimpleElementHitProxyPixelShader"),TEXT("Main"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_VER0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementVertexShader,TEXT("SimpleElementVertexShader"),TEXT("Main"),SF_Vertex,VER_SIMPLE_ELEMENT_SHADER_VER0,0);

/** The simple element vertex declaration. */
TGlobalResource<FSimpleElementVertexDeclaration> GSimpleElementVertexDeclaration;

void FBatchedElements::AddLine(const FVector& Start,const FVector& End,const FLinearColor& Color,FHitProxyId HitProxyId,const FLOAT Thickness)
{
	// Ensure the line isn't masked out.  Some legacy code relies on Color.A being ignored.
	FLinearColor OpaqueColor(Color);
	OpaqueColor.A = 1;

	if (Thickness == 0.0f)
	{
		new(LineVertices) FSimpleElementVertex(Start,FVector2D(0,0),OpaqueColor,HitProxyId);
		new(LineVertices) FSimpleElementVertex(End,FVector2D(0,0),OpaqueColor,HitProxyId);
	}
	else
	{
		FBatchedThickLines* ThickLine = new(ThickLines) FBatchedThickLines;
		ThickLine->Start = Start;
		ThickLine->End = End;
		ThickLine->Thickness = Thickness;
		ThickLine->Color = OpaqueColor;
		ThickLine->HitProxyId = HitProxyId;
	}
}

void FBatchedElements::AddPoint(const FVector& Position,FLOAT Size,const FLinearColor& Color,FHitProxyId HitProxyId)
{
	// Ensure the point isn't masked out.  Some legacy code relies on Color.A being ignored.
	FLinearColor OpaqueColor(Color);
	OpaqueColor.A = 1;

	FBatchedPoint* Point = new(Points) FBatchedPoint;
	Point->Position = Position;
	Point->Size = Size;
	Point->Color = OpaqueColor;
	Point->HitProxyId = HitProxyId;
}

INT FBatchedElements::AddVertex(const FVector4& InPosition,const FVector2D& InTextureCoordinate,const FLinearColor& InColor,FHitProxyId HitProxyId)
{
	INT VertexIndex = MeshVertices.Num();
	new(MeshVertices) FSimpleElementVertex(InPosition,InTextureCoordinate,InColor,HitProxyId);
	return VertexIndex;
}

void FBatchedElements::AddQuadVertex(const FVector4& InPosition,const FVector2D& InTextureCoordinate,const FLinearColor& InColor,FHitProxyId HitProxyId, const FTexture* Texture,ESimpleElementBlendMode BlendMode)
{
	check(GSupportsQuads);

	// @GEMINI_TODO: Speed this up (BeginQuad call that returns handle to a MeshElement?)

	// Find an existing mesh element for the given texture.
	FBatchedQuadMeshElement* MeshElement = NULL;
	for (INT MeshIndex = 0;MeshIndex < QuadMeshElements.Num();MeshIndex++)
	{
		if(QuadMeshElements(MeshIndex).Texture == Texture && QuadMeshElements(MeshIndex).BlendMode == BlendMode)
		{
			MeshElement = &QuadMeshElements(MeshIndex);
			break;
		}
	}

	if (!MeshElement)
	{
		// Create a new mesh element for the texture if this is the first triangle encountered using it.
		MeshElement = new(QuadMeshElements) FBatchedQuadMeshElement;
		MeshElement->Texture = Texture;
		MeshElement->BlendMode = BlendMode;
	}

	new(MeshElement->Vertices) FSimpleElementVertex(InPosition,InTextureCoordinate,InColor,HitProxyId);
}

/** Adds a triangle to the batch. */
void FBatchedElements::AddTriangle(INT V0,INT V1,INT V2,const FTexture* Texture,EBlendMode BlendMode)
{
	ESimpleElementBlendMode SimpleElementBlendMode = SE_BLEND_Opaque;
	switch (BlendMode)
	{
	case BLEND_Opaque:
		SimpleElementBlendMode = SE_BLEND_Opaque; 
		break;
	case BLEND_Masked:
	case BLEND_SoftMasked:
		SimpleElementBlendMode = SE_BLEND_Masked; 
		break;
	case BLEND_DitheredTranslucent:
	case BLEND_Translucent:
		SimpleElementBlendMode = SE_BLEND_Translucent; 
		break;
	case BLEND_Additive:
		SimpleElementBlendMode = SE_BLEND_Additive; 
		break;
	case BLEND_Modulate:
		SimpleElementBlendMode = SE_BLEND_Modulate; 
		break;
    case BLEND_AlphaComposite:
        SimpleElementBlendMode = SE_BLEND_AlphaComposite; 
        break;
	};	
	AddTriangle(V0,V1,V2,Texture,SimpleElementBlendMode);
}

void FBatchedElements::AddTriangle(INT V0, INT V1, INT V2, const FTexture* Texture, ESimpleElementBlendMode BlendMode, const FDepthFieldGlowInfo& GlowInfo)
{
	AddTriangle( V0, V1, V2, NULL, Texture, BlendMode, GlowInfo );
}

	
void FBatchedElements::AddTriangle(INT V0,INT V1,INT V2,FBatchedElementParameters* BatchedElementParameters,ESimpleElementBlendMode BlendMode)
{
	AddTriangle( V0, V1, V2, BatchedElementParameters, NULL, BlendMode );
}


void FBatchedElements::AddTriangle(INT V0,INT V1,INT V2,FBatchedElementParameters* BatchedElementParameters,const FTexture* Texture,ESimpleElementBlendMode BlendMode, const FDepthFieldGlowInfo& GlowInfo)
{
#if XBOX
	// Find an existing mesh element for the given texture and blend mode
	FBatchedMeshElement* MeshElement = NULL;
	for(INT MeshIndex = 0;MeshIndex < MeshElements.Num();MeshIndex++)
	{
		const FBatchedMeshElement& CurMeshElement = MeshElements(MeshIndex);
		if( CurMeshElement.Texture == Texture && 
			CurMeshElement.BatchedElementParameters.GetReference() == BatchedElementParameters &&
			CurMeshElement.BlendMode == BlendMode &&
			// make sure we are not overflowing on indices
			(CurMeshElement.Indices.Num()+3) < MaxMeshIndicesAllowed &&
			CurMeshElement.GlowInfo == GlowInfo )
		{
			// make sure we are not overflowing on vertices
			INT DeltaV0 = (V0 - (INT)CurMeshElement.MinVertex);
			INT DeltaV1 = (V1 - (INT)CurMeshElement.MinVertex);
			INT DeltaV2 = (V2 - (INT)CurMeshElement.MinVertex);
			if( DeltaV0 >= 0 && DeltaV0 < MaxMeshVerticesAllowed &&
				DeltaV1 >= 0 && DeltaV1 < MaxMeshVerticesAllowed &&
				DeltaV2 >= 0 && DeltaV2 < MaxMeshVerticesAllowed )
			{
				MeshElement = &MeshElements(MeshIndex);
				break;
			}			
		}
	}
	if(!MeshElement)
	{
		// make sure that vertex indices are close enough to fit within MaxVerticesAllowed
		if( Abs(V0 - V1) >= MaxMeshVerticesAllowed ||
			Abs(V0 - V2) >= MaxMeshVerticesAllowed )
		{
			warnf(TEXT("Omitting FBatchedElements::AddTriangle due to sparce vertices V0=%i,V1=%i,V2=%i"),V0,V1,V2);
		}
		else
		{
			// Create a new mesh element for the texture if this is the first triangle encountered using it.
			MeshElement = new(MeshElements) FBatchedMeshElement;
			MeshElement->Texture = Texture;
			MeshElement->BatchedElementParameters = BatchedElementParameters;
			MeshElement->BlendMode = BlendMode;
			MeshElement->GlowInfo = GlowInfo;
			MeshElement->MaxVertex = V0;
			// keep track of the min vertex index used
			MeshElement->MinVertex = Min(Min(V0,V1),V2);
		}
	}

	if( MeshElement )
	{
		// Add the triangle's indices to the mesh element's index array.
		MeshElement->Indices.AddItem(V0 - MeshElement->MinVertex);
		MeshElement->Indices.AddItem(V1 - MeshElement->MinVertex);
		MeshElement->Indices.AddItem(V2 - MeshElement->MinVertex);

		// keep track of max vertex used in this mesh batch
		MeshElement->MaxVertex = Max(Max(Max(V0,(INT)MeshElement->MaxVertex),V1),V2);
	}
#else
	// Find an existing mesh element for the given texture and blend mode
	FBatchedMeshElement* MeshElement = NULL;
	for(INT MeshIndex = 0;MeshIndex < MeshElements.Num();MeshIndex++)
	{
		if( MeshElements(MeshIndex).Texture == Texture && 
			MeshElements(MeshIndex).BatchedElementParameters.GetReference() == BatchedElementParameters &&
			MeshElements(MeshIndex).BlendMode == BlendMode &&
			MeshElements(MeshIndex).GlowInfo == GlowInfo )
		{
			MeshElement = &MeshElements(MeshIndex);
			break;
		}
	}
	if( !MeshElement )
	{
		// Create a new mesh element for the texture if this is the first triangle encountered using it.
		MeshElement = new(MeshElements) FBatchedMeshElement;
		MeshElement->Texture = Texture;
		MeshElement->BatchedElementParameters = BatchedElementParameters;
		MeshElement->BlendMode = BlendMode;
		MeshElement->GlowInfo = GlowInfo;
		MeshElement->MaxVertex = V0;
		// keep track of the min vertex index used
		MeshElement->MinVertex = Min(Min(V0,V1),V2);
	}

	// Add the triangle's indices to the mesh element's index array.
	MeshElement->Indices.AddItem(V0 - MeshElement->MinVertex);
	MeshElement->Indices.AddItem(V1 - MeshElement->MinVertex);
	MeshElement->Indices.AddItem(V2 - MeshElement->MinVertex);	

	// keep track of max vertex used in this mesh batch
	MeshElement->MaxVertex = Max(Max(Max(V0,(INT)MeshElement->MaxVertex),V1),V2);
#endif
}

/** 
* Reserves space in mesh vertex array
* 
* @param NumMeshVerts - number of verts to reserve space for
* @param Texture - used to find the mesh element entry
* @param BlendMode - used to find the mesh element entry
*/
void FBatchedElements::AddReserveVertices(INT NumMeshVerts)
{
	MeshVertices.Reserve( MeshVertices.Num() + NumMeshVerts );
}

/** 
* Reserves space in triangle arrays 
* 
* @param NumMeshTriangles - number of triangles to reserve space for
* @param Texture - used to find the mesh element entry
* @param BlendMode - used to find the mesh element entry
*/
void FBatchedElements::AddReserveTriangles(INT NumMeshTriangles,const FTexture* Texture,ESimpleElementBlendMode BlendMode)
{
	for(INT MeshIndex = 0;MeshIndex < MeshElements.Num();MeshIndex++)
	{
		FBatchedMeshElement& CurMeshElement = MeshElements(MeshIndex);
		if( CurMeshElement.Texture == Texture && 
			CurMeshElement.BatchedElementParameters.GetReference() == NULL &&
			CurMeshElement.BlendMode == BlendMode &&
			(CurMeshElement.Indices.Num()+3) < MaxMeshIndicesAllowed )
		{
			CurMeshElement.Indices.Reserve( CurMeshElement.Indices.Num() + NumMeshTriangles );
			break;
		}
	}	
}

void FBatchedElements::AddSprite(
	const FVector& Position,
	FLOAT SizeX,
	FLOAT SizeY,
	const FTexture* Texture,
	const FLinearColor& Color,
	FHitProxyId HitProxyId,
	FLOAT U,
	FLOAT UL,
	FLOAT V,
	FLOAT VL,
	BYTE BlendMode
	)
{
	FBatchedSprite* Sprite = new(Sprites) FBatchedSprite;
	Sprite->Position = Position;
	Sprite->SizeX = SizeX;
	Sprite->SizeY = SizeY;
	Sprite->Texture = Texture;
	Sprite->Color = Color.Quantize();
	Sprite->HitProxyId = HitProxyId;
	Sprite->U = U;
	Sprite->UL = UL == 0.f ? Texture->GetSizeX() : UL;
	Sprite->V = V;
	Sprite->VL = VL == 0.f ? Texture->GetSizeY() : VL;
	Sprite->BlendMode = BlendMode;
}

/** Translates a ESimpleElementBlendMode into a RHI state change for rendering a mesh with the blend mode normally. */
static void SetBlendState(ESimpleElementBlendMode BlendMode)
{
	switch(BlendMode)
	{
	case SE_BLEND_Opaque:
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		break;
	case SE_BLEND_Masked:
	case SE_BLEND_MaskedDistanceField:
	case SE_BLEND_MaskedDistanceFieldShadowed:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,128>::GetRHI());
		break;
	case SE_BLEND_Translucent:
	case SE_BLEND_TranslucentDistanceField:
	case SE_BLEND_TranslucentDistanceFieldShadowed:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One,CF_Greater,0>::GetRHI());
		break;
	case SE_BLEND_Additive:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());
		break;
	case SE_BLEND_Modulate:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_DestColor,BF_Zero,BO_Add,BF_Zero,BF_One>::GetRHI());
		break;
    case SE_BLEND_AlphaComposite:
        RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_InverseSourceAlpha,BO_Add,BF_One,BF_InverseSourceAlpha>::GetRHI());
        break;
	}
}

/** Translates a ESimpleElementBlendMode into a RHI state change for rendering a mesh with the blend mode for hit testing. */
static void SetHitTestingBlendState(ESimpleElementBlendMode BlendMode)
{
	switch(BlendMode)
	{
	case SE_BLEND_Opaque:
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		break;
	case SE_BLEND_Masked:
	case SE_BLEND_MaskedDistanceField:
	case SE_BLEND_MaskedDistanceFieldShadowed:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,128>::GetRHI());
		break;
    case SE_BLEND_AlphaComposite:
	case SE_BLEND_Translucent:
	case SE_BLEND_TranslucentDistanceField:
	case SE_BLEND_TranslucentDistanceFieldShadowed:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	case SE_BLEND_Additive:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	case SE_BLEND_Modulate:
		RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	}
}

FGlobalBoundShaderState FBatchedElements::SimpleBoundShaderState;
FGlobalBoundShaderState FBatchedElements::RegularBoundShaderState;
FGlobalBoundShaderState FBatchedElements::MaskedBoundShaderState;
FGlobalBoundShaderState FBatchedElements::DistanceFieldBoundShaderState;
FGlobalBoundShaderState FBatchedElements::HitTestingBoundShaderState;

/** Global alpha ref test value for rendering masked batched elements */
FLOAT GBatchedElementAlphaRefVal = 128.f;
/** Global smoothing width for rendering batched elements with distance field blend modes */
FLOAT GBatchedElementSmoothWidth = 6;

/*
 * Sets the appropriate vertex and pixel shader.
 */
void FBatchedElements::PrepareShaders(
	ESimpleElementBlendMode BlendMode,
	const FMatrix& Transform,
	FBatchedElementParameters* BatchedElementParameters,
	const FTexture* Texture,
	UBOOL bHitTesting,
	FLOAT Gamma,
	const FDepthFieldGlowInfo* GlowInfo
	) const
{
#if WITH_MOBILE_RHI
	if( GUsingMobileRHI )
	{
		//Reset all state so we don't pollute the cache
		if (BlendMode == SE_BLEND_Additive)
		{
			RHISetMobileSimpleParams(BLEND_Additive);
		}
		else if (BlendMode == SE_BLEND_Masked)
		{
			RHISetMobileSimpleParams(BLEND_Masked);
		}
		else
		{
			RHISetMobileSimpleParams(BLEND_Opaque);
		}
	}
#endif

	if( BatchedElementParameters != NULL )
	{
		// Use the vertex/pixel shader that we were given
		BatchedElementParameters->BindShaders_RenderThread( Transform, Gamma );
	}
	else
	{
		TShaderMapRef<FSimpleElementVertexShader> VertexShader(GetGlobalShaderMap());

	    // Set the simple element vertex shader parameters
	    VertexShader->SetParameters(Transform);
		    
	    if (bHitTesting)
	    {
		    TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
		    HitTestingPixelShader->SetParameters(Texture);
		    SetHitTestingBlendState( BlendMode);
		    SetGlobalBoundShaderState(HitTestingBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, 
			    *VertexShader, *HitTestingPixelShader, sizeof(FSimpleElementVertex));
	    }
	    else
	    {
	    
		    if (BlendMode == SE_BLEND_Masked)
		    {
			    // use clip() in the shader instead of alpha testing as cards that don't support floating point blending
			    // also don't support alpha testing to floating point render targets
			    RHISetBlendState(TStaticBlendState<>::GetRHI());
			    TShaderMapRef<FSimpleElementMaskedGammaPixelShader> MaskedPixelShader(GetGlobalShaderMap());
				    MaskedPixelShader->SetParameters(Texture,Gamma,GBatchedElementAlphaRefVal / 255.0f,BlendMode);
				    SetGlobalBoundShaderState( MaskedBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, 
					    *VertexShader, *MaskedPixelShader, sizeof(FSimpleElementVertex));
		    }
		    // render distance field elements
		    else if (
			    BlendMode == SE_BLEND_MaskedDistanceField || 
			    BlendMode == SE_BLEND_MaskedDistanceFieldShadowed ||
			    BlendMode == SE_BLEND_TranslucentDistanceField	||
			    BlendMode == SE_BLEND_TranslucentDistanceFieldShadowed)
		    {
			    FLOAT AlphaRefVal = GBatchedElementAlphaRefVal;
			    if (BlendMode == SE_BLEND_TranslucentDistanceField ||
				    BlendMode == SE_BLEND_TranslucentDistanceFieldShadowed)
			    {
				    // enable alpha blending and disable clip ref value for translucent rendering
				    RHISetBlendState(TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One,CF_Greater,0>::GetRHI());
				    AlphaRefVal = 0.0f;
			    }
			    else
			    {
				    // clip is done in shader so just render opaque
				    RHISetBlendState(TStaticBlendState<>::GetRHI());
			    }
			    
			    TShaderMapRef<FSimpleElementDistanceFieldGammaPixelShader> DistanceFieldPixelShader(GetGlobalShaderMap());			
			    
			    // @todo - expose these as options for batch rendering
			    static FVector2D ShadowDirection(-1.0f/Texture->GetSizeX(),-1.0f/Texture->GetSizeY());
			    static FLinearColor ShadowColor(FLinearColor::Black);
			    const FLOAT ShadowSmoothWidth = (GBatchedElementSmoothWidth * 2) / Texture->GetSizeX();
			    
			    const UBOOL EnableShadow = (
				    BlendMode == SE_BLEND_MaskedDistanceFieldShadowed || 
				    BlendMode == SE_BLEND_TranslucentDistanceFieldShadowed
				    );
			    
			    DistanceFieldPixelShader->SetParameters(
				    Texture,
				    Gamma,
				    AlphaRefVal / 255.0f,
				    GBatchedElementSmoothWidth / Texture->GetSizeX(),
				    EnableShadow,
				    ShadowDirection,
				    ShadowColor,
				    ShadowSmoothWidth,
				    (GlowInfo != NULL) ? *GlowInfo : FDepthFieldGlowInfo(EC_EventParm),
				    BlendMode
				    );
			    
			    SetGlobalBoundShaderState( DistanceFieldBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, 
				    *VertexShader, *DistanceFieldPixelShader, sizeof(FSimpleElementVertex));			
		    }
		    else
		    {
			    SetBlendState( BlendMode);
    
			    if (Abs(Gamma - 1.0f) < KINDA_SMALL_NUMBER)
			    {
				    TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
				    RegularPixelShader->SetParameters(Texture);
				    SetGlobalBoundShaderState( SimpleBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, 
					    *VertexShader, *RegularPixelShader, sizeof(FSimpleElementVertex));
			    }
			    else
			    {
				    TShaderMapRef<FSimpleElementGammaPixelShader> RegularPixelShader(GetGlobalShaderMap());
				    RegularPixelShader->SetParameters(Texture,Gamma,BlendMode);
				    SetGlobalBoundShaderState( RegularBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, 
					    *VertexShader, *RegularPixelShader, sizeof(FSimpleElementVertex));
			    }
		    }
	    }
	}
}

UBOOL FBatchedElements::Draw(const FMatrix& Transform,UINT ViewportSizeX,UINT ViewportSizeY,UBOOL bHitTesting,FLOAT Gamma) const
{
	if( HasPrimsToDraw() )
	{
		SCOPED_DRAW_EVENT(EventBatched)(DEC_SCENE_ITEMS,TEXT("BatchedElements"));

		FMatrix InvTransform = Transform.InverseSafe();
		FVector CameraX = InvTransform.TransformNormal(FVector(1,0,0)).SafeNormal();
		FVector CameraY = InvTransform.TransformNormal(FVector(0,1,0)).SafeNormal();
		FVector CameraZ = InvTransform.TransformNormal(FVector(0,0,1)).SafeNormal();

		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetBlendState(TStaticBlendState<>::GetRHI());

		if( LineVertices.Num() > 0 || Points.Num() > 0 || ThickLines.Num() > 0 )
		{
			// Lines/points don't support batched element parameters (yet!)
			FBatchedElementParameters* BatchedElementParameters = NULL;

			// Set the appropriate pixel shader parameters & shader state for the non-textured elements.
			PrepareShaders( SE_BLEND_Opaque, Transform, BatchedElementParameters, GWhiteTexture, bHitTesting, Gamma);
			if( !GSupportsDepthTextures )
			{
				// Disable alpha channel writes as it overwrites SceneDepth on PC
				RHISetColorWriteMask(CW_RGB);
			}

			// Draw the line elements.
			if( LineVertices.Num() > 0 )
			{
				SCOPED_DRAW_EVENT(EventLines)(DEC_SCENE_ITEMS,TEXT("Lines"));

				INT MaxVerticesAllowed = ((GDrawUPVertexCheckCount / sizeof(FSimpleElementVertex)) / 2) * 2;
				/*
				hack to avoid a crash when trying to render large numbers of line segments.
				*/
				MaxVerticesAllowed = Min(MaxVerticesAllowed, 64 * 1024);

				INT MinVertex=0;
				INT TotalVerts = (LineVertices.Num() / 2) * 2;
				while( MinVertex < TotalVerts )
				{
					INT NumLinePrims = Min( MaxVerticesAllowed, TotalVerts - MinVertex ) / 2;
					RHIDrawPrimitiveUP(PT_LineList,NumLinePrims,&LineVertices(MinVertex),sizeof(FSimpleElementVertex));
					MinVertex += NumLinePrims * 2;
				}
			}

			// Draw the point elements.
			if( Points.Num() > 0 )
			{
				SCOPED_DRAW_EVENT(EventPoints)(DEC_SCENE_ITEMS,TEXT("Points"));

				// preallocate some memory to directly fill out
				void* VerticesPtr;

				const INT NumPoints = Points.Num();
				const INT NumTris = NumPoints * 2;
				const INT NumVertices = NumTris * 3;
				RHIBeginDrawPrimitiveUP(PT_TriangleList, NumTris, NumVertices, sizeof(FSimpleElementVertex), VerticesPtr);
				FSimpleElementVertex* PointVertices = (FSimpleElementVertex*)VerticesPtr;

				INT VertIdx = 0;
				for(INT PointIndex = 0;PointIndex < NumPoints;PointIndex++)
				{
					// @GEMINI_TODO: Support quad primitives here
					const FBatchedPoint& Point = Points(PointIndex);
					FVector4 TransformedPosition = Transform.TransformFVector4(Point.Position);

					// Generate vertices for the point such that the post-transform point size is constant.
					const UINT ViewportMajorAxis = ViewportSizeX;//Max(ViewportSizeX, ViewportSizeY);
					const FVector WorldPointX = CameraX * Point.Size / ViewportMajorAxis * TransformedPosition.W;
					const FVector WorldPointY = CameraY * -Point.Size / ViewportMajorAxis * TransformedPosition.W;
					
					PointVertices[VertIdx + 0] = FSimpleElementVertex(FVector4(Point.Position + WorldPointX - WorldPointY,1),FVector2D(1,0),Point.Color,Point.HitProxyId);
					PointVertices[VertIdx + 1] = FSimpleElementVertex(FVector4(Point.Position + WorldPointX + WorldPointY,1),FVector2D(1,1),Point.Color,Point.HitProxyId);
					PointVertices[VertIdx + 2] = FSimpleElementVertex(FVector4(Point.Position - WorldPointX - WorldPointY,1),FVector2D(0,0),Point.Color,Point.HitProxyId);
					PointVertices[VertIdx + 3] = FSimpleElementVertex(FVector4(Point.Position + WorldPointX + WorldPointY,1),FVector2D(1,1),Point.Color,Point.HitProxyId);
					PointVertices[VertIdx + 4] = FSimpleElementVertex(FVector4(Point.Position - WorldPointX - WorldPointY,1),FVector2D(0,0),Point.Color,Point.HitProxyId);
					PointVertices[VertIdx + 5] = FSimpleElementVertex(FVector4(Point.Position - WorldPointX + WorldPointY,1),FVector2D(0,1),Point.Color,Point.HitProxyId);

					VertIdx += 6;
				}

				// Draw the sprite.
				RHIEndDrawPrimitiveUP();
			}

			if ( ThickLines.Num() > 0 )
			{
				SCOPED_DRAW_EVENT(EventThickLines)(DEC_SCENE_ITEMS,TEXT("ThickLines"));

				for(INT ThickIndex = 0;ThickIndex < ThickLines.Num();ThickIndex++)
				{
					// preallocate some memory to directly fill out
					void* VerticesPtr;
					RHIBeginDrawPrimitiveUP(PT_TriangleStrip, 2, 4, sizeof(FSimpleElementVertex), VerticesPtr);
					FSimpleElementVertex* ThickVertices = (FSimpleElementVertex*)VerticesPtr;

					const FBatchedThickLines& Line = ThickLines(ThickIndex);

					FVector RectLength = (Line.End - Line.Start).SafeNormal();
					FVector RectUp = (RectLength ^ CameraZ).SafeNormal();
					RectUp *= (Line.Thickness * 0.5f);
																													    
					ThickVertices[2] = FSimpleElementVertex(FVector4(Line.Start - RectUp, 1), FVector2D(0,0), Line.Color, Line.HitProxyId);
					ThickVertices[3] = FSimpleElementVertex(FVector4(Line.Start + RectUp, 1), FVector2D(0,1), Line.Color, Line.HitProxyId);
					ThickVertices[1] = FSimpleElementVertex(FVector4(Line.End + RectUp, 1), FVector2D(1,1), Line.Color, Line.HitProxyId);
					ThickVertices[0] = FSimpleElementVertex(FVector4(Line.End - RectUp, 1), FVector2D(1,0), Line.Color, Line.HitProxyId);

					// Draw the sprite.
					RHIEndDrawPrimitiveUP();
				}
			}

			if( !GSupportsDepthTextures )
			{
				// Renable alpha channel writes
				RHISetColorWriteMask(CW_RGBA);
			}
		}

		// Draw the sprites.
		if( Sprites.Num() > 0 )
		{
			SCOPED_DRAW_EVENT(EventSprites)(DEC_SCENE_ITEMS,TEXT("Sprites"));

			// Sprites don't support batched element parameters (yet!)
			FBatchedElementParameters* BatchedElementParameters = NULL;

			INT SpriteCount = Sprites.Num();
			//Sort sprites by texture
			appQsort((void*)&Sprites(0), SpriteCount, sizeof(FBatchedSprite), (QSORT_COMPARE)FBatchedElements::TextureCompare);
			
			//First time init
			const FTexture* CurrentTexture = Sprites(0).Texture;
			ESimpleElementBlendMode CurrentBlendMode = (ESimpleElementBlendMode)Sprites(0).BlendMode;

			TArray<FSimpleElementVertex> SpriteList;
			for(INT SpriteIndex = 0;SpriteIndex < SpriteCount;SpriteIndex++)
			{
				const FBatchedSprite& Sprite = Sprites(SpriteIndex);
				if (CurrentTexture != Sprite.Texture || CurrentBlendMode != Sprite.BlendMode)
				{
					//New batch, draw previous and clear
					const INT VertexCount = SpriteList.Num();
					const INT PrimCount = VertexCount / 3;
					PrepareShaders( CurrentBlendMode, Transform, BatchedElementParameters, CurrentTexture, bHitTesting, Gamma);
					RHIDrawPrimitiveUP(PT_TriangleList, PrimCount, &SpriteList(0), sizeof(FSimpleElementVertex));

					SpriteList.Empty(6);
					CurrentTexture = Sprite.Texture;
					CurrentBlendMode = (ESimpleElementBlendMode)Sprite.BlendMode;
				}

				INT SpriteListIndex = SpriteList.Add(6);
				FSimpleElementVertex* Vertex = SpriteList.GetTypedData();

				// Compute the sprite vertices.
				const FVector WorldSpriteX = CameraX * Sprite.SizeX;
				const FVector WorldSpriteY = CameraY * -Sprite.SizeY;
				const FLinearColor LinearSpriteColor(Sprite.Color);

				const FLOAT UStart = Sprite.U/Sprite.Texture->GetSizeX();
				const FLOAT UEnd = (Sprite.U + Sprite.UL)/Sprite.Texture->GetSizeX();
				const FLOAT VStart = Sprite.V/Sprite.Texture->GetSizeY();
				const FLOAT VEnd = (Sprite.V + Sprite.VL)/Sprite.Texture->GetSizeY();

				Vertex[SpriteListIndex + 0] = FSimpleElementVertex(FVector4(Sprite.Position + WorldSpriteX - WorldSpriteY,1),FVector2D(UEnd,  VStart),LinearSpriteColor,Sprite.HitProxyId);
				Vertex[SpriteListIndex + 1] = FSimpleElementVertex(FVector4(Sprite.Position + WorldSpriteX + WorldSpriteY,1),FVector2D(UEnd,  VEnd  ),LinearSpriteColor,Sprite.HitProxyId);
				Vertex[SpriteListIndex + 2] = FSimpleElementVertex(FVector4(Sprite.Position - WorldSpriteX - WorldSpriteY,1),FVector2D(UStart,VStart),LinearSpriteColor,Sprite.HitProxyId);

				Vertex[SpriteListIndex + 3] = FSimpleElementVertex(FVector4(Sprite.Position + WorldSpriteX + WorldSpriteY,1),FVector2D(UEnd,  VEnd  ),LinearSpriteColor,Sprite.HitProxyId);
				Vertex[SpriteListIndex + 4] = FSimpleElementVertex(FVector4(Sprite.Position - WorldSpriteX - WorldSpriteY,1),FVector2D(UStart,VStart),LinearSpriteColor,Sprite.HitProxyId);
				Vertex[SpriteListIndex + 5] = FSimpleElementVertex(FVector4(Sprite.Position - WorldSpriteX + WorldSpriteY,1),FVector2D(UStart,VEnd  ),LinearSpriteColor,Sprite.HitProxyId);
			}

			if (SpriteList.Num() > 0)
			{
				//Draw last batch
				const INT VertexCount = SpriteList.Num();
				const INT PrimCount = VertexCount / 3;
				PrepareShaders( CurrentBlendMode, Transform, BatchedElementParameters, CurrentTexture, bHitTesting, Gamma);
				RHIDrawPrimitiveUP(PT_TriangleList, PrimCount, &SpriteList(0), sizeof(FSimpleElementVertex));
			}
		}

		if( MeshElements.Num() > 0 || QuadMeshElements.Num() > 0 )
		{
			SCOPED_DRAW_EVENT(EventMeshes)(DEC_SCENE_ITEMS,TEXT("Meshes"));		

			// Draw the mesh elements.
			for(INT MeshIndex = 0;MeshIndex < MeshElements.Num();MeshIndex++)
			{
				const FBatchedMeshElement& MeshElement = MeshElements(MeshIndex);

				// Set the appropriate pixel shader for the mesh.
				PrepareShaders(MeshElement.BlendMode, Transform, MeshElement.BatchedElementParameters, MeshElement.Texture, bHitTesting, Gamma, &MeshElement.GlowInfo);

				// Draw the mesh.
				RHIDrawIndexedPrimitiveUP(
					PT_TriangleList,
					0,
					MeshElement.MaxVertex - MeshElement.MinVertex + 1,
					MeshElement.Indices.Num() / 3,
					&MeshElement.Indices(0),
					sizeof(WORD),
					&MeshVertices(MeshElement.MinVertex),
					sizeof(FSimpleElementVertex)
					);
			}

			// Draw the quad mesh elements.
			for(INT MeshIndex = 0;MeshIndex < QuadMeshElements.Num();MeshIndex++)
			{
				const FBatchedQuadMeshElement& MeshElement = QuadMeshElements(MeshIndex);

				// Quads don't support batched element parameters (yet!)
				FBatchedElementParameters* BatchedElementParameters = NULL;

				// Set the appropriate pixel shader for the mesh.
				PrepareShaders(MeshElement.BlendMode,Transform,BatchedElementParameters,MeshElement.Texture,bHitTesting,Gamma);

				// Draw the mesh.
				RHIDrawPrimitiveUP(
					PT_QuadList,
					MeshElement.Vertices.Num() / 4,
					&MeshElement.Vertices(0),
					sizeof(FSimpleElementVertex)
					);
			}
		}

		// Restore the render target color bias.
		RHISetRenderTargetBias( appPow(2.0f,GCurrentColorExpBias));

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
