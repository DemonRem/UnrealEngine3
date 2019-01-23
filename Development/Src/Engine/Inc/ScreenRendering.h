/*=============================================================================
	ScreenRendering.h: D3D render target implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

struct FScreenVertex
{
	FVector2D Position;
	FVector2D UV;
};

/** The filter vertex declaration resource type. */
class FScreenVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	// Destructor.
	virtual ~FScreenVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.AddItem(FVertexElement(0,STRUCT_OFFSET(FScreenVertex,Position),VET_Float2,VEU_Position,0));
		Elements.AddItem(FVertexElement(0,STRUCT_OFFSET(FScreenVertex,UV),VET_Float2,VEU_TextureCoordinate,0));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.Release();
	}
};

extern TGlobalResource<FScreenVertexDeclaration> GScreenVertexDeclaration;

/**
 * A pixel shader for rendering a textured screen element.
 */
class FScreenPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FScreenPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FScreenPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		TextureParameter.Bind(Initializer.ParameterMap,TEXT("Texture"));
	}
	FScreenPixelShader() {}

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
 * A vertex shader for rendering a textured screen element.
 */
class FScreenVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(FScreenVertexShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FScreenVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
	}
	FScreenVertexShader() {}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
	}
};

/**
 * Draws a texture rectangle on the screen using normalized (-1 to 1) device coordinates.
 */
extern void DrawScreenQuad(  FCommandContextRHI* Context, FLOAT X0, FLOAT Y0, FLOAT U0, FLOAT V0, FLOAT X1, FLOAT Y1, FLOAT U1, FLOAT V1, const FTexture* Texture );
