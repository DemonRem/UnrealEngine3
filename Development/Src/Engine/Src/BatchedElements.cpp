/*=============================================================================
	BatchedElements.cpp: Batched element rendering.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "BatchedElements.h"

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FSimpleElementPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FSimpleElementPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap,TEXT("Texture"));
		GammaParameter.Bind(Initializer.ParameterMap,TEXT("Gamma"));
	}
	FSimpleElementPixelShader() {}

	void SetParameters(FCommandContextRHI* Context,const FTexture* Texture,FLOAT Gamma,EBlendMode BlendMode)
	{
		SetTextureParameter(Context,GetPixelShader(),TextureParameter,Texture);
		SetPixelShaderValue(Context,GetPixelShader(),GammaParameter,Gamma);
		RHISetRenderTargetBias(Context, (BlendMode == BLEND_Modulate) ? 1.0f : appPow(2.0f,GCurrentColorExpBias));
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << TextureParameter;
		Ar << GammaParameter;
	}

private:
	FShaderParameter TextureParameter;
	FShaderParameter GammaParameter;
};

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementHitProxyPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FSimpleElementHitProxyPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return Platform == SP_PCD3D_SM3 || Platform == SP_PCD3D_SM2; }

	FSimpleElementHitProxyPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap,TEXT("Texture"));
	}
	FSimpleElementHitProxyPixelShader() {}

	void SetParameters(FCommandContextRHI* Context,const FTexture* Texture)
	{
		SetTextureParameter(Context,GetPixelShader(),TextureParameter,Texture);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << TextureParameter;
	}

private:
	FShaderParameter TextureParameter;
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

	void SetParameters(FCommandContextRHI* Context,const FMatrix& Transform)
	{
		SetVertexShaderValue(Context,GetVertexShader(),TransformParameter,Transform);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << TransformParameter;
	}

private:
	FShaderParameter TransformParameter;
};

// Shader implementations.
IMPLEMENT_SHADER_TYPE(,FSimpleElementPixelShader,TEXT("SimpleElementPixelShader"),TEXT("Main"),SF_Pixel,VER_SIMPLE_ELEMENT_SHADER_RECOMPILE,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementHitProxyPixelShader,TEXT("SimpleElementHitProxyPixelShader"),TEXT("Main"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(,FSimpleElementVertexShader,TEXT("SimpleElementVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/** The simple element vertex declaration. */
TGlobalResource<FSimpleElementVertexDeclaration> GSimpleElementVertexDeclaration;

void FBatchedElements::AddLine(const FVector& Start,const FVector& End,const FLinearColor& Color,FHitProxyId HitProxyId)
{
	// Ensure the line isn't masked out.  Some legacy code relies on Color.A being ignored.
	FLinearColor OpaqueColor(Color);
	OpaqueColor.A = 1;

	new(LineVertices) FSimpleElementVertex(Start,FVector2D(0,0),OpaqueColor,HitProxyId);
	new(LineVertices) FSimpleElementVertex(End,FVector2D(0,0),OpaqueColor,HitProxyId);
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

void FBatchedElements::AddQuadVertex(const FVector4& InPosition,const FVector2D& InTextureCoordinate,const FLinearColor& InColor,FHitProxyId HitProxyId, const FTexture* Texture,EBlendMode BlendMode)
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

void FBatchedElements::AddTriangle(INT V0,INT V1,INT V2,const FTexture* Texture,EBlendMode BlendMode)
{
	INT MaxIndicesAllowed = GDrawUPIndexCheckCount / sizeof(INT);
	INT MaxVerticesAllowed = GDrawUPVertexCheckCount / sizeof(FSimpleElementVertex);

	// Find an existing mesh element for the given texture and blend mode
	FBatchedMeshElement* MeshElement = NULL;
	for(INT MeshIndex = 0;MeshIndex < MeshElements.Num();MeshIndex++)
	{
		const FBatchedMeshElement& CurMeshElement = MeshElements(MeshIndex);
		if( CurMeshElement.Texture == Texture && 
			CurMeshElement.BlendMode == BlendMode &&
			// make sure we are not overflowing on indices
			(CurMeshElement.Indices.Num()+3) < MaxIndicesAllowed )
		{
			// make sure we are not overflowing on vertices
			INT DeltaV0 = (V0 - (INT)CurMeshElement.MinVertex);
			INT DeltaV1 = (V1 - (INT)CurMeshElement.MinVertex);
			INT DeltaV2 = (V2 - (INT)CurMeshElement.MinVertex);
			if( DeltaV0 >= 0 && DeltaV0 < MaxVerticesAllowed &&
				DeltaV1 >= 0 && DeltaV1 < MaxVerticesAllowed &&
				DeltaV2 >= 0 && DeltaV2 < MaxVerticesAllowed )
			{
				MeshElement = &MeshElements(MeshIndex);
				break;
			}			
		}
	}
	if(!MeshElement)
	{
		// make sure that vertex indices are close enough to fit within MaxVerticesAllowed
		if( Abs(V0 - V1) >= MaxVerticesAllowed ||
			Abs(V0 - V2) >= MaxVerticesAllowed )
		{
			warnf(TEXT("Omitting FBatchedElements::AddTriangle due to sparce vertices V0=%i,V1=%i,V2=%i"),V0,V1,V2);
		}
		else
		{
			// Create a new mesh element for the texture if this is the first triangle encountered using it.
			MeshElement = new(MeshElements) FBatchedMeshElement;
			MeshElement->Texture = Texture;
			MeshElement->BlendMode = BlendMode;
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
}

void FBatchedElements::AddSprite(
	const FVector& Position,
	FLOAT SizeX,
	FLOAT SizeY,
	const FTexture* Texture,
	const FLinearColor& Color,
	FHitProxyId HitProxyId
	)
{
	FBatchedSprite* Sprite = new(Sprites) FBatchedSprite;
	Sprite->Position = Position;
	Sprite->SizeX = SizeX;
	Sprite->SizeY = SizeY;
	Sprite->Texture = Texture;
	Sprite->Color = Color.Quantize();
	Sprite->HitProxyId = HitProxyId;
}

/** Translates a EBlendMode into a RHI state change for rendering a mesh with the blend mode normally. */
static void SetBlendState(FCommandContextRHI* Context, EBlendMode BlendMode)
{
	switch(BlendMode)
	{
	case BLEND_Opaque:
		RHISetBlendState(Context,TStaticBlendState<>::GetRHI());
		break;
	case BLEND_Masked:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,128>::GetRHI());
		break;
	case BLEND_Translucent:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One>::GetRHI());
		break;
	case BLEND_Additive:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());
		break;
	case BLEND_Modulate:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_DestColor,BF_Zero,BO_Add,BF_Zero,BF_One>::GetRHI());
		break;
	}
}

/** Translates a EBlendMode into a RHI state change for rendering a mesh with the blend mode for hit testing. */
static void SetHitTestingBlendState(FCommandContextRHI* Context, EBlendMode BlendMode)
{
	switch(BlendMode)
	{
	case BLEND_Opaque:
		RHISetBlendState(Context,TStaticBlendState<>::GetRHI());
		break;
	case BLEND_Masked:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,128>::GetRHI());
		break;
	case BLEND_Translucent:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	case BLEND_Additive:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	case BLEND_Modulate:
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,1>::GetRHI());
		break;
	}
}

FBoundShaderStateRHIRef FBatchedElements::RegularBoundShaderState;
FBoundShaderStateRHIRef FBatchedElements::HitTestingBoundShaderState;

UBOOL FBatchedElements::Draw(FCommandContextRHI* Context,const FMatrix& Transform,UINT ViewportSizeX,UINT ViewportSizeY,UBOOL bHitTesting,FLOAT Gamma) const
{
	if( HasPrimsToDraw() )
	{
		SCOPED_DRAW_EVENT(EventBatched)(DEC_SCENE_ITEMS,TEXT("BatchedElements"));

		FMatrix InvTransform = Transform.Inverse();
		FVector CameraX = InvTransform.TransformNormal(FVector(1,0,0)).SafeNormal();
		FVector CameraY = InvTransform.TransformNormal(FVector(0,1,0)).SafeNormal();

		TShaderMapRef<FSimpleElementVertexShader> VertexShader(GetGlobalShaderMap());
		
		// cache bound shader state
		if (bHitTesting)
		{
			if (!IsValidRef(HitTestingBoundShaderState))
			{
				DWORD Strides[MaxVertexElementCount];
				appMemzero(Strides, sizeof(Strides));
				Strides[0] = sizeof(FSimpleElementVertex);

				TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
				HitTestingBoundShaderState = RHICreateBoundShaderState(GSimpleElementVertexDeclaration.VertexDeclarationRHI, Strides, VertexShader->GetVertexShader(), HitTestingPixelShader->GetPixelShader());
			}
		}
		else
		{
			if (!IsValidRef(RegularBoundShaderState))
			{
				DWORD Strides[MaxVertexElementCount];
				appMemzero(Strides, sizeof(Strides));
				Strides[0] = sizeof(FSimpleElementVertex);

				TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
				RegularBoundShaderState = RHICreateBoundShaderState(GSimpleElementVertexDeclaration.VertexDeclarationRHI, Strides, VertexShader->GetVertexShader(), RegularPixelShader->GetPixelShader());
			}
		}		

		RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetBlendState(Context,TStaticBlendState<>::GetRHI());

		// Set the simple element vertex shader parameters
		VertexShader->SetParameters(Context,Transform);

		if( LineVertices.Num() > 0 || Points.Num() > 0 )
		{
			// Set the appropriate pixel shader parameters & shader state for the non-textured elements.
			if(bHitTesting)
			{
				TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
				HitTestingPixelShader->SetParameters(Context,GWhiteTexture);
				RHISetBoundShaderState(Context,HitTestingBoundShaderState);
			}
			else
			{
				TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
				RegularPixelShader->SetParameters(Context,GWhiteTexture,Gamma,BLEND_Opaque);
				RHISetBoundShaderState(Context,RegularBoundShaderState);
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
					RHIDrawPrimitiveUP(Context,PT_LineList,NumLinePrims,&LineVertices(MinVertex),sizeof(FSimpleElementVertex));
					MinVertex += NumLinePrims * 2;
				}
			}

			// Draw the point elements.
			if( Points.Num() > 0 )
			{
				SCOPED_DRAW_EVENT(EventPoints)(DEC_SCENE_ITEMS,TEXT("Points"));
				
				for(INT PointIndex = 0;PointIndex < Points.Num();PointIndex++)
				{
					// @GEMINI_TODO: Support quad primitives here

					// preallocate some memory to directly fill out
					void* VerticesPtr;
					RHIBeginDrawPrimitiveUP(Context, PT_TriangleFan, 2, 4, sizeof(FSimpleElementVertex), VerticesPtr);
					FSimpleElementVertex* PointVertices = (FSimpleElementVertex*)VerticesPtr;

					const FBatchedPoint& Point = Points(PointIndex);
					FVector4 TransformedPosition = Transform.TransformFVector4(Point.Position);

					// Generate vertices for the point such that the post-transform point size is constant.
					FVector WorldPointX = InvTransform.TransformNormal(FVector(1,0,0)) * Point.Size / ViewportSizeX * TransformedPosition.W,
						WorldPointY = InvTransform.TransformNormal(FVector(0,1,0)) * -Point.Size / ViewportSizeY * TransformedPosition.W;
					PointVertices[0] = FSimpleElementVertex(FVector4(Point.Position - WorldPointX - WorldPointY,1),FVector2D(0,0),Point.Color,Point.HitProxyId);
					PointVertices[1] = FSimpleElementVertex(FVector4(Point.Position - WorldPointX + WorldPointY,1),FVector2D(0,1),Point.Color,Point.HitProxyId);
					PointVertices[2] = FSimpleElementVertex(FVector4(Point.Position + WorldPointX + WorldPointY,1),FVector2D(1,1),Point.Color,Point.HitProxyId);
					PointVertices[3] = FSimpleElementVertex(FVector4(Point.Position + WorldPointX - WorldPointY,1),FVector2D(1,0),Point.Color,Point.HitProxyId);

					// Draw the sprite.
					RHIEndDrawPrimitiveUP(Context);
				}
			}
		}

		// Draw the sprites.
		if( Sprites.Num() > 0 )
		{
			SCOPED_DRAW_EVENT(EventSprites)(DEC_SCENE_ITEMS,TEXT("Sprites"));

			for(INT SpriteIndex = 0;SpriteIndex < Sprites.Num();SpriteIndex++)
			{
				const FBatchedSprite& Sprite = Sprites(SpriteIndex);

				// Set the appropriate pixel shader for the mesh.
				if(bHitTesting)
				{
					TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
					HitTestingPixelShader->SetParameters(Context,Sprite.Texture);
					RHISetBoundShaderState(Context, HitTestingBoundShaderState);				
				}
				else
				{
					TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
					RegularPixelShader->SetParameters(Context,Sprite.Texture,Gamma,BLEND_Masked);
					RHISetBoundShaderState(Context, RegularBoundShaderState);
				}

				// Use alpha testing, but not blending.
				RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero,CF_GreaterEqual,128>::GetRHI());

				// @GEMINI_TODO: Support quad primitives here

				// preallocate some memory to directly fill out
				void* VerticesPtr;
				RHIBeginDrawPrimitiveUP(Context, PT_TriangleFan, 2, 4, sizeof(FSimpleElementVertex), VerticesPtr);
				FSimpleElementVertex* SpriteVertices = (FSimpleElementVertex*)VerticesPtr;
				// Compute the sprite vertices.
				FVector WorldSpriteX = CameraX * Sprite.SizeX,
					WorldSpriteY = CameraY * -Sprite.SizeY;
				FColor SpriteColor = Sprite.Color;
				FLinearColor LinearSpriteColor = FLinearColor(SpriteColor);
				SpriteVertices[0] = FSimpleElementVertex(FVector4(Sprite.Position - WorldSpriteX - WorldSpriteY,1),FVector2D(0,0),LinearSpriteColor,Sprite.HitProxyId);
				SpriteVertices[1] = FSimpleElementVertex(FVector4(Sprite.Position - WorldSpriteX + WorldSpriteY,1),FVector2D(0,1),LinearSpriteColor,Sprite.HitProxyId);
				SpriteVertices[2] = FSimpleElementVertex(FVector4(Sprite.Position + WorldSpriteX + WorldSpriteY,1),FVector2D(1,1),LinearSpriteColor,Sprite.HitProxyId);
				SpriteVertices[3] = FSimpleElementVertex(FVector4(Sprite.Position + WorldSpriteX - WorldSpriteY,1),FVector2D(1,0),LinearSpriteColor,Sprite.HitProxyId);
				// Draw the sprite.
				RHIEndDrawPrimitiveUP(Context);
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
				if(bHitTesting)
				{
					TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
					HitTestingPixelShader->SetParameters(Context,MeshElement.Texture);
					SetHitTestingBlendState(Context, MeshElement.BlendMode);
					RHISetBoundShaderState(Context, HitTestingBoundShaderState);
				}
				else
				{
					TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
					RegularPixelShader->SetParameters(Context,MeshElement.Texture,Gamma,MeshElement.BlendMode);
					SetBlendState(Context, MeshElement.BlendMode);
					RHISetBoundShaderState(Context, RegularBoundShaderState);
				}

				// Draw the mesh.
				RHIDrawIndexedPrimitiveUP(
					Context,
					PT_TriangleList,
					0,
					MeshElement.MaxVertex - MeshElement.MinVertex + 1,
					MeshElement.Indices.Num() / 3,
					&MeshElement.Indices(0),
					sizeof(INT),
					&MeshVertices(MeshElement.MinVertex),
					sizeof(FSimpleElementVertex)
					);
			}

			// Draw the quad mesh elements.
			for(INT MeshIndex = 0;MeshIndex < QuadMeshElements.Num();MeshIndex++)
			{
				const FBatchedQuadMeshElement& MeshElement = QuadMeshElements(MeshIndex);

				// Set the appropriate pixel shader for the mesh.
				if(bHitTesting)
				{
					TShaderMapRef<FSimpleElementHitProxyPixelShader> HitTestingPixelShader(GetGlobalShaderMap());
					HitTestingPixelShader->SetParameters(Context,MeshElement.Texture);
					SetHitTestingBlendState(Context, MeshElement.BlendMode);
					RHISetBoundShaderState(Context, HitTestingBoundShaderState);
				}
				else
				{
					TShaderMapRef<FSimpleElementPixelShader> RegularPixelShader(GetGlobalShaderMap());
					RegularPixelShader->SetParameters(Context,MeshElement.Texture,Gamma,MeshElement.BlendMode);
					SetBlendState(Context, MeshElement.BlendMode);
					RHISetBoundShaderState(Context, RegularBoundShaderState);
				}

				// Draw the mesh.
				RHIDrawPrimitiveUP(
					Context,
					PT_QuadList,
					MeshElement.Vertices.Num() / 4,
					&MeshElement.Vertices(0),
					sizeof(FSimpleElementVertex)
					);
			}
		}

		// Restore the render target color bias.
		RHISetRenderTargetBias(Context, appPow(2.0f,GCurrentColorExpBias));

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
