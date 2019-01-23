/*=============================================================================
	D3DShaders.cpp: D3D shader RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI

FVertexShaderRHIRef RHICreateVertexShader(const TArray<BYTE>& Code)
{
	TD3DRef<IDirect3DVertexShader9> VertexShader;
	VERIFYD3DRESULT(GDirect3DDevice->CreateVertexShader((DWORD*)&Code(0),VertexShader.GetInitReference()));
	return VertexShader;
}

FPixelShaderRHIRef RHICreatePixelShader(const TArray<BYTE>& Code)
{
	TD3DRef<IDirect3DPixelShader9> PixelShader;
	VERIFYD3DRESULT(GDirect3DDevice->CreatePixelShader((DWORD*)&Code(0),PixelShader.GetInitReference()));
	return PixelShader;
}

/**
* Creates a bound shader state instance which encapsulates a decl, vertex shader, and pixel shader
* @param VertexDeclaration - existing vertex decl
* @param StreamStrides - optional stream strides
* @param VertexShader - existing vertex shader
* @param PixelShader - existing pixel shader
*/
FBoundShaderStateRHIRef RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclaration, 
	DWORD *StreamStrides, 
	FVertexShaderRHIParamRef VertexShader, 
	FPixelShaderRHIParamRef PixelShader )
{
	FBoundShaderStateRHIRef BoundShaderState;
	BoundShaderState.VertexDeclaration = VertexDeclaration;
	BoundShaderState.VertexShader = VertexShader;
	BoundShaderState.PixelShader = PixelShader;
	return BoundShaderState;
}

/**
* Initializes GRHIShaderPlatform
*/
void RHIInitializeShaderPlatform()
{
	static UBOOL bIsInitialized = FALSE;
	if( !bIsInitialized )
	{
		InitializeD3DShaderPlatform();
		bIsInitialized = TRUE;
	}
}

#endif
