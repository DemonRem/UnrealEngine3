/*=============================================================================
	D3DCommands.cpp: D3D RHI commands implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI
#include "EngineParticleClasses.h"
#include "ChartCreation.h"

// Globals

/** Toggle for MSAA tiling support */
UBOOL GUseTilingCode = FALSE;
/**
*	The size to check against for Draw*UP call vertex counts.
*	If greater than this value, the draw call will not occur.
*/
INT GDrawUPVertexCheckCount = MAXINT;		
/**
*	The size to check against for Draw*UP call index counts.
*	If greater than this value, the draw call will not occur.
*/
INT GDrawUPIndexCheckCount = MAXINT;

/**
 *	Cached current view for RHISetViewPixelParameters.
 */
const FSceneView* GCurrentView = NULL;


// Vertex state.
void RHISetStreamSource(FCommandContextRHI* Context,UINT StreamIndex,FVertexBufferRHIParamRef VertexBuffer,UINT Stride,UBOOL bUseInstanceIndex,UINT NumVerticesPerInstance,UINT NumInstances)
{
	GDirect3DDevice->SetStreamSource(StreamIndex,VertexBuffer,0,Stride);

	// Set the vertex stream frequency.
	DWORD Frequency = 1;
	if(bUseInstanceIndex || NumInstances > 1)
	{
		Frequency = bUseInstanceIndex ? 
			(D3DSTREAMSOURCE_INSTANCEDATA | 1) :
			(D3DSTREAMSOURCE_INDEXEDDATA | NumInstances);
	}
	GDirect3DDevice->SetStreamSourceFreq(StreamIndex,Frequency);
}

FORCEINLINE DWORD FLOAT_TO_DWORD( FLOAT f ) { return *((DWORD*)&f); }

// Rasterizer state.
void RHISetRasterizerState(FCommandContextRHI* Context,FRasterizerStateRHIParamRef NewState)
{
	GDirect3DDevice->SetRenderState(D3DRS_FILLMODE,NewState->FillMode);
	GDirect3DDevice->SetRenderState(D3DRS_CULLMODE,NewState->CullMode);
	GDirect3DDevice->SetRenderState(D3DRS_DEPTHBIAS,FLOAT_TO_DWORD(NewState->DepthBias));
	GDirect3DDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,FLOAT_TO_DWORD(NewState->SlopeScaleDepthBias));
}
void RHISetViewport(FCommandContextRHI* Context,UINT MinX,UINT MinY,FLOAT MinZ,UINT MaxX,UINT MaxY,FLOAT MaxZ)
{
	D3DVIEWPORT9 Viewport = { MinX, MinY, MaxX - MinX, MaxY - MinY, MinZ, MaxZ };
	//avoid setting a 0 extent viewport, which the debug runtime doesn't like
	if (Viewport.Width > 0 && Viewport.Height > 0)
	{
		GDirect3DDevice->SetViewport(&Viewport);
	}
}
void RHISetScissorRect(FCommandContextRHI* Context,UBOOL bEnable,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY)
{
	// Defined in UnPlayer.cpp. Used here to disable scissors when doing highres screenshots.
	extern UBOOL GIsTiledScreenshot;
	bEnable = GIsTiledScreenshot ? FALSE : bEnable;

	if(bEnable)
	{
		RECT ScissorRect;
		ScissorRect.left = MinX;
		ScissorRect.right = MaxX;
		ScissorRect.top = MinY;
		ScissorRect.bottom = MaxY;
		GDirect3DDevice->SetScissorRect(&ScissorRect);
	}
	GDirect3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,bEnable);
}

/**
 * Set depth bounds test state.  
 * When enabled, incoming fragments are killed if the value already in the depth buffer is outside of [MinZ, MaxZ]
 */
void RHISetDepthBoundsTest(FCommandContextRHI* Context,UBOOL bEnable,FLOAT MinZ,FLOAT MaxZ)
{
	//not supported by d3d
}


/**
* Set bound shader state. This will set the vertex decl/shader, and pixel shader
* @param Context - context we are rendering to
* @param BoundShaderState - state resource
*/
void RHISetBoundShaderState(FCommandContextRHI* Context, FBoundShaderStateRHIParamRef BoundShaderState)
{
	check(IsValidRef(BoundShaderState.VertexDeclaration));
	GDirect3DDevice->SetVertexDeclaration(BoundShaderState.VertexDeclaration);
	GDirect3DDevice->SetVertexShader(BoundShaderState.VertexShader);
	if ( IsValidRef(BoundShaderState.PixelShader) )
	{
		GDirect3DDevice->SetPixelShader(BoundShaderState.PixelShader);
	}
	else
	{
		// use special null pixel shader when PixelSahder was set to NULL
		TShaderMapRef<FNULLPixelShader> NullPixelShader(GetGlobalShaderMap());
		GDirect3DDevice->SetPixelShader(NullPixelShader->GetPixelShader());
	}
}

void RHISetSamplerState(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT SamplerIndex,FSamplerStateRHIParamRef NewState,FTextureRHIParamRef NewTexture)
{
	GDirect3DDevice->SetTexture(SamplerIndex,(IDirect3DBaseTexture9*)NewTexture);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_SRGBTEXTURE,NewTexture.IsSRGB());
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_MAGFILTER,NewState->MagFilter);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_MINFILTER,NewState->MinFilter);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_MIPFILTER,NewState->MipFilter);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_ADDRESSU,NewState->AddressU);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_ADDRESSV,NewState->AddressV);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_ADDRESSW,NewState->AddressW);
	GDirect3DDevice->SetSamplerState(SamplerIndex,D3DSAMP_MIPMAPLODBIAS,NewState->MipMapLODBias);
}

void RHISetVertexShaderParameter(FCommandContextRHI* Context,FVertexShaderRHIParamRef PixelShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue)
{
	GDirect3DDevice->SetVertexShaderConstantF(BaseRegisterIndex,NewValue,NumVectors);
}

void RHISetPixelShaderParameter(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue)
{
	GDirect3DDevice->SetPixelShaderConstantF(BaseRegisterIndex,NewValue,NumVectors);
}

/**
 * Set engine shader parameters for the view.
 * @param Context				Context we are rendering to
 * @param View					The current view
 * @param ViewProjectionMatrix	Matrix that transforms from world space to projection space for the view
 * @param ViewOrigin			World space position of the view's origin
 */
void RHISetViewParameters( FCommandContextRHI* Context, const FSceneView* View, const FMatrix& ViewProjectionMatrix, const FVector4& ViewOrigin )
{
	GDirect3DDevice->SetVertexShaderConstantF( VSR_ViewProjMatrix, (const FLOAT*) &ViewProjectionMatrix, 4 );
	GDirect3DDevice->SetVertexShaderConstantF( VSR_ViewOrigin, (const FLOAT*) &ViewOrigin, 1 );
	if ( View != GCurrentView )
	{
		GDirect3DDevice->SetPixelShaderConstantF( PSR_MinZ_MaxZ_Ratio, (const FLOAT*) &View->InvDeviceZToWorldZTransform, 1 );
		GDirect3DDevice->SetPixelShaderConstantF( PSR_ScreenPositionScaleBias, (const FLOAT*) &View->ScreenPositionScaleBias, 1 );
		GCurrentView = View;
	}
}

/**
 * Not used on PC
 */
void RHISetRenderTargetBias( FCommandContextRHI* Context, FLOAT ColorBias )
{
}
void RHISetShaderRegisterAllocation(UINT NumVertexShaderRegisters, UINT NumPixelShaderRegisters)
{
}
void RHIReduceTextureCachePenalty( FPixelShaderRHIParamRef PixelShader )
{
}

// Output state.
void RHISetDepthState(FCommandContextRHI* Context,FDepthStateRHIParamRef NewState)
{
	GDirect3DDevice->SetRenderState(D3DRS_ZENABLE,NewState->bZEnable);
	GDirect3DDevice->SetRenderState(D3DRS_ZWRITEENABLE,NewState->bZWriteEnable);
	GDirect3DDevice->SetRenderState(D3DRS_ZFUNC,NewState->ZFunc);
}
void RHISetStencilState(FCommandContextRHI* Context,FStencilStateRHIParamRef NewState)
{
	GDirect3DDevice->SetRenderState(D3DRS_STENCILENABLE,NewState->bStencilEnable);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILFUNC,NewState->StencilFunc);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILFAIL,NewState->StencilFail);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILZFAIL,NewState->StencilZFail);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILPASS,NewState->StencilPass);
	GDirect3DDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,NewState->bTwoSidedStencilMode);
	GDirect3DDevice->SetRenderState(D3DRS_CCW_STENCILFUNC,NewState->CCWStencilFunc);
	GDirect3DDevice->SetRenderState(D3DRS_CCW_STENCILFAIL,NewState->CCWStencilFail);
	GDirect3DDevice->SetRenderState(D3DRS_CCW_STENCILZFAIL,NewState->CCWStencilZFail);
	GDirect3DDevice->SetRenderState(D3DRS_CCW_STENCILPASS,NewState->CCWStencilPass);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILMASK,NewState->StencilReadMask);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK,NewState->StencilWriteMask);
	GDirect3DDevice->SetRenderState(D3DRS_STENCILREF,NewState->StencilRef);
}
void RHISetBlendState(FCommandContextRHI* Context,FBlendStateRHIParamRef NewState)
{
	GDirect3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,NewState->bAlphaBlendEnable);
	GDirect3DDevice->SetRenderState(D3DRS_BLENDOP,NewState->ColorBlendOperation);
	GDirect3DDevice->SetRenderState(D3DRS_SRCBLEND,NewState->ColorSourceBlendFactor);
	GDirect3DDevice->SetRenderState(D3DRS_DESTBLEND,NewState->ColorDestBlendFactor);
	GDirect3DDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,NewState->bSeparateAlphaBlendEnable);
	GDirect3DDevice->SetRenderState(D3DRS_BLENDOPALPHA,NewState->AlphaBlendOperation);
	GDirect3DDevice->SetRenderState(D3DRS_SRCBLENDALPHA,NewState->AlphaSourceBlendFactor);
	GDirect3DDevice->SetRenderState(D3DRS_DESTBLENDALPHA,NewState->AlphaDestBlendFactor);
	GDirect3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE,NewState->bAlphaTestEnable);
	GDirect3DDevice->SetRenderState(D3DRS_ALPHAFUNC,NewState->AlphaFunc);
	GDirect3DDevice->SetRenderState(D3DRS_ALPHAREF,NewState->AlphaRef);
}

void RHISetRenderTarget(FCommandContextRHI* Context, FSurfaceRHIParamRef NewRenderTarget, FSurfaceRHIParamRef NewDepthStencilTarget)
{
	// Reset all texture references, to ensure a reference to this render target doesn't remain set.
	for(UINT TextureIndex = 0;TextureIndex < 16;TextureIndex++)
	{
		GDirect3DDevice->SetTexture(TextureIndex,NULL);
	}

	if(!IsValidRef(NewRenderTarget))
	{
		// 1. If we're setting a NULL color buffer, we must also set a NULL depth buffer.
		// 2. If we're setting a NULL color buffer, we're going to use the back buffer instead (D3D shortcoming).
		check( IsValidRef(GD3DBackBuffer) && !IsValidRef(NewDepthStencilTarget) );
		GDirect3DDevice->SetRenderTarget(0,GD3DBackBuffer);
	}
	else
	{
		GDirect3DDevice->SetRenderTarget(0,NewRenderTarget);
	}

	GDirect3DDevice->SetDepthStencilSurface(NewDepthStencilTarget);

	// Detect when the back buffer is being set, and set the correct viewport.
	if( (!IsValidRef(NewRenderTarget) || NewRenderTarget == GD3DBackBuffer) && 
		GD3DDrawingViewport )
	{
		D3DVIEWPORT9 D3DViewport = { 0, 0, GD3DDrawingViewport->GetSizeX(), GD3DDrawingViewport->GetSizeY(), 0.0f, 1.0f };
		GDirect3DDevice->SetViewport(&D3DViewport);
	}
}
void RHISetColorWriteEnable(FCommandContextRHI* Context,UBOOL bEnable)
{
	DWORD EnabledStateValue = D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED;
	GDirect3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE,bEnable ? EnabledStateValue : 0);
}
void RHISetColorWriteMask(FCommandContextRHI* Context, UINT ColorWriteMask)
{
	DWORD EnabledStateValue;
	EnabledStateValue  = (ColorWriteMask & CW_RED) ? D3DCOLORWRITEENABLE_RED : 0;
	EnabledStateValue |= (ColorWriteMask & CW_GREEN) ? D3DCOLORWRITEENABLE_GREEN : 0;
	EnabledStateValue |= (ColorWriteMask & CW_BLUE) ? D3DCOLORWRITEENABLE_BLUE : 0;
	EnabledStateValue |= (ColorWriteMask & CW_ALPHA) ? D3DCOLORWRITEENABLE_ALPHA : 0;
	GDirect3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, EnabledStateValue );
}

// Not supported
void RHIBeginHiStencilRecord(FCommandContextRHI* Context) { }
void RHIBeginHiStencilPlayback(FCommandContextRHI* Context) { }
void RHIEndHiStencil(FCommandContextRHI* Context) { }

// Occlusion queries.
void RHIBeginOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery)
{
	OcclusionQuery->Issue(D3DISSUE_BEGIN);
}
void RHIEndOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery)
{
	OcclusionQuery->Issue(D3DISSUE_END);
}

// Primitive drawing.

static D3DPRIMITIVETYPE GetD3DPrimitiveType(UINT PrimitiveType)
{
	switch(PrimitiveType)
	{
	case PT_TriangleList: return D3DPT_TRIANGLELIST;
	case PT_TriangleFan: return D3DPT_TRIANGLEFAN;
	case PT_TriangleStrip: return D3DPT_TRIANGLESTRIP;
	case PT_LineList: return D3DPT_LINELIST;
	default: appErrorf(TEXT("Unknown primitive type: %u"),PrimitiveType);
	};
	return D3DPT_TRIANGLELIST;
}

void RHIDrawPrimitive(FCommandContextRHI* Context,UINT PrimitiveType,UINT BaseVertexIndex,UINT NumPrimitives)
{
	INC_DWORD_STAT(STAT_DrawPrimitiveCalls);
	INC_DWORD_STAT_BY(STAT_RHITriangles,(DWORD)(PrimitiveType != PT_LineList ? NumPrimitives : 0));
	INC_DWORD_STAT_BY(STAT_RHILines,(DWORD)(PrimitiveType == PT_LineList ? NumPrimitives : 0));
	GDirect3DDevice->DrawPrimitive(
		GetD3DPrimitiveType(PrimitiveType),
		BaseVertexIndex,
		NumPrimitives
		);
}

void RHIDrawIndexedPrimitive(FCommandContextRHI* Context,FIndexBufferRHIParamRef IndexBuffer,UINT PrimitiveType,INT BaseVertexIndex,UINT MinIndex,UINT NumVertices,UINT StartIndex,UINT NumPrimitives)
{
	INC_DWORD_STAT(STAT_DrawPrimitiveCalls);
	INC_DWORD_STAT_BY(STAT_RHITriangles,(DWORD)(PrimitiveType != PT_LineList ? NumPrimitives : 0));
	INC_DWORD_STAT_BY(STAT_RHILines,(DWORD)(PrimitiveType == PT_LineList ? NumPrimitives : 0));
	GDirect3DDevice->SetIndices(IndexBuffer);
	GDirect3DDevice->DrawIndexedPrimitive(
		GetD3DPrimitiveType(PrimitiveType),
		BaseVertexIndex,
		MinIndex,
		NumVertices,
		StartIndex,
		NumPrimitives
		);
}

static UBOOL GBegunDrawPrimiveUP;

static TArray<BYTE> GDrawPrimitiveUPVertexData;
static UINT GNumVertices = 0;
static UINT GVertexDataStride = 0;

static TArray<BYTE> GDrawPrimitiveUPIndexData;
static UINT GPrimitiveType = 0;
static UINT GNumPrimitives = 0;
static UINT GMinVertexIndex = 0;
static UINT GIndexDataStride = 0;

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex 
 * @param OutVertexData Reference to the allocated vertex memory
 */
void RHIBeginDrawPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData)
{
	check(!GBegunDrawPrimiveUP);

	if((UINT)GDrawPrimitiveUPVertexData.Num() < NumVertices * VertexDataStride)
	{
		GDrawPrimitiveUPVertexData.Empty(NumVertices * VertexDataStride);
		GDrawPrimitiveUPVertexData.Add(NumVertices * VertexDataStride - GDrawPrimitiveUPVertexData.Num());
	}
	OutVertexData = &GDrawPrimitiveUPVertexData(0);

	GPrimitiveType = PrimitiveType;
	GNumPrimitives = NumPrimitives;
	GNumVertices = NumVertices;
	GVertexDataStride = VertexDataStride;
	GBegunDrawPrimiveUP = TRUE;
}

/**
 * Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
 * @param Context Rendering device context to use
 */
void RHIEndDrawPrimitiveUP(FCommandContextRHI* Context)
{
	check(GBegunDrawPrimiveUP);
	GBegunDrawPrimiveUP = FALSE;

	// for now (while RHIDrawPrimitiveUP still exists), just call it because it does the same work we need here
	RHIDrawPrimitiveUP(Context, GPrimitiveType, GNumPrimitives, &GDrawPrimitiveUPVertexData(0), GVertexDataStride);
}
/**
 * Draw a primitive using the vertices passed in
 * VertexData is NOT created by BeginDrawPrimitveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param VertexData A reference to memory preallocate in RHIBeginDrawPrimitiveUP
 * @param VertexDataStride The size of one vertex
 */
void RHIDrawPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, const void* VertexData,UINT VertexDataStride)
{
	INC_DWORD_STAT(STAT_DrawPrimitiveCalls);
	INC_DWORD_STAT_BY(STAT_RHITriangles,(DWORD)(PrimitiveType != PT_LineList ? NumPrimitives : 0));
	INC_DWORD_STAT_BY(STAT_RHILines,(DWORD)(PrimitiveType == PT_LineList ? NumPrimitives : 0));

	// Reset vertex stream 0's frequency.
	GDirect3DDevice->SetStreamSourceFreq(0,1);

	GDirect3DDevice->DrawPrimitiveUP(
		GetD3DPrimitiveType(PrimitiveType),
		NumPrimitives,
		VertexData,
		VertexDataStride
		);

}

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawIndexedPrimitiveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex
 * @param OutVertexData Reference to the allocated vertex memory
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumIndices Number of indices to be written
 * @param IndexDataStride Size of each index (either 2 or 4 bytes)
 * @param OutIndexData Reference to the allocated index memory
 */
void RHIBeginDrawIndexedPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData, UINT MinVertexIndex, UINT NumIndices, UINT IndexDataStride, void*& OutIndexData)
{
	check(!GBegunDrawPrimiveUP);

	if((UINT)GDrawPrimitiveUPVertexData.Num() < NumVertices * VertexDataStride)
	{
		GDrawPrimitiveUPVertexData.Empty(NumVertices * VertexDataStride);
		GDrawPrimitiveUPVertexData.Add(NumVertices * VertexDataStride - GDrawPrimitiveUPVertexData.Num());
	}
	OutVertexData = &GDrawPrimitiveUPVertexData(0);

	if((UINT)GDrawPrimitiveUPIndexData.Num() < NumIndices * IndexDataStride)
	{
		GDrawPrimitiveUPIndexData.Empty(NumIndices * IndexDataStride);
		GDrawPrimitiveUPIndexData.Add(NumIndices * IndexDataStride - GDrawPrimitiveUPIndexData.Num());
	}
	OutIndexData = &GDrawPrimitiveUPIndexData(0);

	check((sizeof(WORD) == IndexDataStride) || (sizeof(DWORD) == IndexDataStride));

	GPrimitiveType = PrimitiveType;
	GNumPrimitives = NumPrimitives;
	GMinVertexIndex = MinVertexIndex;
	GIndexDataStride = IndexDataStride;

	GNumVertices = NumVertices;
	GVertexDataStride = VertexDataStride;
	
	GBegunDrawPrimiveUP = TRUE;
}

/**
 * Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
 * @param Context Rendering device context to use
 */
void RHIEndDrawIndexedPrimitiveUP(FCommandContextRHI* Context)
{
	check(GBegunDrawPrimiveUP);
	GBegunDrawPrimiveUP = FALSE;

	// for now (while RHIDrawPrimitiveUP still exists), just call it because it does the same work we need here
	RHIDrawIndexedPrimitiveUP(Context, GPrimitiveType, GMinVertexIndex, GNumVertices, GNumPrimitives, &GDrawPrimitiveUPIndexData(0), GIndexDataStride, &GDrawPrimitiveUPVertexData(0), GVertexDataStride);
}

/**
 * Draw a primitive using the vertices passed in as described the passed in indices. 
 * IndexData and VertexData are NOT created by BeginDrawIndexedPrimitveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumVertices The number of vertices in the vertex buffer
 * @param NumPrimitives THe number of primitives described by the index buffer
 * @param IndexData The memory preallocated in RHIBeginDrawIndexedPrimitiveUP
 * @param IndexDataStride The size of one index
 * @param VertexData The memory preallocate in RHIBeginDrawIndexedPrimitiveUP
 * @param VertexDataStride The size of one vertex
 */
void RHIDrawIndexedPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT NumPrimitives, const void* IndexData, UINT IndexDataStride, const void* VertexData, UINT VertexDataStride)
{
	INC_DWORD_STAT(STAT_DrawPrimitiveCalls);
	INC_DWORD_STAT_BY(STAT_RHITriangles,(DWORD)(PrimitiveType != PT_LineList ? NumPrimitives : 0));
	INC_DWORD_STAT_BY(STAT_RHILines,(DWORD)(PrimitiveType == PT_LineList ? NumPrimitives : 0));

	// Reset vertex stream 0's frequency.
	GDirect3DDevice->SetStreamSourceFreq(0,1);

	GDirect3DDevice->DrawIndexedPrimitiveUP(
		GetD3DPrimitiveType(PrimitiveType),
		MinVertexIndex,
		NumVertices,
		NumPrimitives,
		IndexData,
		IndexDataStride == sizeof(WORD) ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
		VertexData,
		VertexDataStride
		);
}

//
/**
 * Draw a sprite particle emitter.
 *
 * @param Context Rendering device context to use
 * @param Mesh The mesh element containing the data for rendering the sprite particles
 */
void RHIDrawSpriteParticles(FCommandContextRHI* Context, const FMeshElement& Mesh)
{
	check(Mesh.DynamicVertexData);
	FDynamicSpriteEmitterData* SpriteData = (FDynamicSpriteEmitterData*)(Mesh.DynamicVertexData);

	// Sort the particles if required
	INT ParticleCount = SpriteData->ActiveParticleCount;

	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	INT StartIndex = 0;
	INT EndIndex = ParticleCount;
	if ((SpriteData->MaxDrawCount >= 0) && (ParticleCount > SpriteData->MaxDrawCount))
	{
		ParticleCount = SpriteData->MaxDrawCount;
	}

	TArray<FParticleOrder>* ParticleOrder = (TArray<FParticleOrder>*)(Mesh.DynamicIndexData);

	// Render the particles are indexed tri-lists
	void* OutVertexData = NULL;
	void* OutIndexData = NULL;

	// Get the memory from the device for copying the particle vertex/index data to
	RHIBeginDrawIndexedPrimitiveUP(Context, PT_TriangleList, 
		ParticleCount * 2, ParticleCount * 4, sizeof(FParticleSpriteVertex), OutVertexData, 
		0, ParticleCount * 6, sizeof(WORD), OutIndexData);

	if (OutVertexData && OutIndexData)
	{
		// Pack the data
		FParticleSpriteVertex* Vertices = (FParticleSpriteVertex*)OutVertexData;
		SpriteData->GetVertexAndIndexData(Vertices, OutIndexData, ParticleOrder);
		// End the draw, which will submit the data for rendering
		RHIEndDrawIndexedPrimitiveUP(Context);
	}
}

/**
 * Draw a sprite subuv particle emitter.
 *
 * @param Context Rendering device context to use
 * @param Mesh The mesh element containing the data for rendering the sprite subuv particles
 */
void RHIDrawSubUVParticles(FCommandContextRHI* Context, const FMeshElement& Mesh)
{
	check(Mesh.DynamicVertexData);
	FDynamicSubUVEmitterData* SubUVData = (FDynamicSubUVEmitterData*)(Mesh.DynamicVertexData);

	// Sort the particles if required
	INT ParticleCount = SubUVData->ActiveParticleCount;

	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	INT StartIndex = 0;
	INT EndIndex = ParticleCount;
	if ((SubUVData->MaxDrawCount >= 0) && (ParticleCount > SubUVData->MaxDrawCount))
	{
		ParticleCount = SubUVData->MaxDrawCount;
	}

	TArray<FParticleOrder>* ParticleOrder = (TArray<FParticleOrder>*)(Mesh.DynamicIndexData);

	// Render the particles are indexed tri-lists
	void* OutVertexData = NULL;
	void* OutIndexData = NULL;

	// Get the memory from the device for copying the particle vertex/index data to
	RHIBeginDrawIndexedPrimitiveUP(Context, PT_TriangleList, 
		ParticleCount * 2, ParticleCount * 4, sizeof(FParticleSpriteSubUVVertex), OutVertexData, 
		0, ParticleCount * 6, sizeof(WORD), OutIndexData);

	if (OutVertexData && OutIndexData)
	{
		// Pack the data
		FParticleSpriteSubUVVertex* Vertices = (FParticleSpriteSubUVVertex*)OutVertexData;
		SubUVData->GetVertexAndIndexData(Vertices, OutIndexData, ParticleOrder);
		// End the draw, which will submit the data for rendering
		RHIEndDrawIndexedPrimitiveUP(Context);
	}
}

// Raster operations.
void RHIClear(FCommandContextRHI* Context,UBOOL bClearColor,const FLinearColor& Color,UBOOL bClearDepth,FLOAT Depth,UBOOL bClearStencil,DWORD Stencil)
{
	// Determine the clear flags.
	DWORD Flags = 0;
	if(bClearColor)
	{
		Flags |= D3DCLEAR_TARGET;
	}
	if(bClearDepth)
	{
		Flags |= D3DCLEAR_ZBUFFER;
	}
	if(bClearStencil)
	{
		Flags |= D3DCLEAR_STENCIL;
	}

	// Clear the render target/depth-stencil surfaces based on the flags.
	FColor QuantizedColor(Color.Quantize());
	GDirect3DDevice->Clear(0,NULL,Flags,D3DCOLOR_RGBA(QuantizedColor.R,QuantizedColor.G,QuantizedColor.B,QuantizedColor.A),Depth,Stencil);
}

// Functions to yield and regain rendering control from D3D

void RHISuspendRendering()
{
	// Not supported
}

void RHIResumeRendering()
{
	// Not supported
}

// Kick the rendering commands that are currently queued up in the GPU command buffer.
void RHIKickCommandBuffer( FCommandContextRHI* Context )
{
	// Not really supported
}

/*
 * Returns the total GPU time taken to render the last frame. Same metric as appCycles().
 */
DWORD RHIGetGPUFrameCycles()
{
	return GGPUFrameTime;
}

/*
 * Returns an approximation of the available memory that textures can use, which is video + AGP where applicable, rounded to the nearest MB, in MB.
 */
DWORD RHIGetAvailableTextureMemory()
{
	//apparently GetAvailableTextureMem() returns available bytes (the docs don't say) rounded to the nearest MB.
	return GDirect3DDevice->GetAvailableTextureMem() / 1048576;
}

// not used on PC

void RHIMSAAInitPrepass()
{
}
void RHIMSAAFixViewport()
{
}
void RHIMSAABeginRendering()
{
}
void RHIMSAAEndRendering(const FTexture2DRHIRef& DepthTexture, const FTexture2DRHIRef& ColorTexture)
{
}
void RHIMSAARestoreDepth(const FTexture2DRHIRef& DepthTexture)
{
}

#endif
