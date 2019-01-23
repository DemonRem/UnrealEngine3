/*=============================================================================
	DecalVertexFactory.cpp: Decal vertex factory implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "DecalVertexFactory.h"

#if !PS3
extern UBOOL STOP_COMPILER_WARNINGS_DecalVertexFactory = FALSE;
#endif

#if 0
/**
 * Creates declarations for each of the vertex stream components and
 * initializes the device resource.
 */
void FDecalVertexFactory::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// Positions.
	Elements.AddItem( AccessStreamComponent(Data.PositionComponent,VEU_Position) );

	// Tangents.
	Elements.AddItem( AccessStreamComponent(Data.TangentBasisComponents[0],VEU_Tangent) );
	Elements.AddItem( AccessStreamComponent(Data.TangentBasisComponents[1],VEU_Binormal) );
	Elements.AddItem( AccessStreamComponent(Data.TangentBasisComponents[2],VEU_Normal) );

	// Shadow map texture coordinates.
	Elements.AddItem( AccessStreamComponent(Data.ShadowMapCoordinateComponent,VEU_Color) );

	InitDeclaration( Elements );
}

/**
 * Shader parameters for use with FDecalVertexFactory.
 */
class FDecalVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	 * Bind shader constants by name
	 * @param	ParameterMap - mapping of named shader constants to indices
	 */
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		LocalToWorldParameter.Bind( ParameterMap, TEXT("LocalToWorld") );
		WorldToLocalParameter.Bind( ParameterMap, TEXT("WorldToLocal"), TRUE );
		WorldToDecalParameter.Bind( ParameterMap, TEXT("WorldToDecal") );
	}

	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar)
	{
		Ar << LocalToWorldParameter;
		Ar << WorldToLocalParameter;
		Ar << WorldToDecalParameter;
	}

	/**
	 * Set any shader data specific to this vertex factory.
	 */
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
	{
		const FMatrix& ShaderData = ((FDecalVertexFactory*)VertexFactory)->GetWorldToDecal();
		SetVertexShaderValues<FMatrix>(
			Context,
			VertexShader->GetVertexShader(),
			WorldToDecalParameter,
			&ShaderData,
			1
			);
	}

	/**
	 * Set the local to world transform shader
	 */
	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
	{
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LocalToWorldParameter,LocalToWorld);
		SetVertexShaderValues(Context,VertexShader->GetVertexShader(),WorldToLocalParameter,(FVector4*)&WorldToLocal,3);
	}
private:
	FShaderParameter LocalToWorldParameter;
	FShaderParameter WorldToLocalParameter;
	FShaderParameter WorldToDecalParameter;
};

/** Bind decal vertex factory to its shader file and its shader parameters. */
IMPLEMENT_VERTEX_FACTORY_TYPE(FDecalVertexFactory, FDecalVertexFactoryShaderParameters, "DecalVertexFactory", TRUE, TRUE, VER_JULY_XDK_UPGRADE, 0);
#endif
