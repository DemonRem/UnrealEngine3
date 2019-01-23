/*=============================================================================
	TranslucentRendering.h: Translucent rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
* Vertex shader used for combining LDR translucency with scene color when floating point blending isn't supported
*/
class FLDRTranslucencyCombineVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FLDRTranslucencyCombineVertexShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	FLDRTranslucencyCombineVertexShader() {}

public:

	FLDRTranslucencyCombineVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
	}
};

/**
* Translucent draw policy factory.
* Creates the policies needed for rendering a mesh based on its material
*/
class FTranslucencyDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = FALSE };
	struct ContextType {};

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return TRUE if the mesh rendered
	*/
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo* View,
		ContextType DrawingContext,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return TRUE if the mesh rendered
	*/
	static UBOOL DrawStaticMesh(
		FCommandContextRHI* Context,
		const FViewInfo* View,
		ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);

	static UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		return !IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial()->GetBlendMode());
	}
};
