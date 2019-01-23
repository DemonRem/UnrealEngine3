/*=============================================================================
SpeedTreeVertexFactory.cpp: SpeedTree vertex factory implementation.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "SpeedTree.h"

#if WITH_SPEEDTREE

UBOOL FSpeedTreeVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithSpeedTree() || Material->IsSpecialEngineMaterial();
}

void FSpeedTreeVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	Elements.AddItem(AccessStreamComponent(Data.PositionComponent, VEU_Position));

	if( Data.WindInfo.VertexBuffer )
		Elements.AddItem(AccessStreamComponent(Data.WindInfo, VEU_BlendIndices));

	EVertexElementUsage TangentBasisUsages[3] = { VEU_Tangent, VEU_Binormal, VEU_Normal };
	for( INT AxisIndex=0; AxisIndex<3; AxisIndex++ )
	{
		if( Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL )
		{
			Elements.AddItem(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex], TangentBasisUsages[AxisIndex]));
		}
	}

	if( Data.TextureCoordinates.Num() )
	{
		for( UINT CoordinateIndex=0; CoordinateIndex<Data.TextureCoordinates.Num(); CoordinateIndex++ )
		{
			Elements.AddItem(AccessStreamComponent(Data.TextureCoordinates(CoordinateIndex),VEU_TextureCoordinate,CoordinateIndex));
		}

		for( UINT CoordinateIndex=Data.TextureCoordinates.Num(); CoordinateIndex<MAX_TEXCOORDS; CoordinateIndex++ )
		{
			Elements.AddItem(AccessStreamComponent(Data.TextureCoordinates(Data.TextureCoordinates.Num()-1),VEU_TextureCoordinate,CoordinateIndex));
		}
	}

	if( Data.ShadowMapCoordinateComponent.VertexBuffer )
	{
		Elements.AddItem(AccessStreamComponent(Data.ShadowMapCoordinateComponent, VEU_Color));
	}
	else if( Data.TextureCoordinates.Num() )
	{
		Elements.AddItem(AccessStreamComponent(Data.TextureCoordinates(0), VEU_Color));
	}

	InitDeclaration(Elements, FVertexFactory::DataType(), TRUE, TRUE);
}

class FSpeedTreeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		LocalToWorldParameter.Bind( ParameterMap, TEXT("LocalToWorld") );
		WorldToLocalParameter.Bind( ParameterMap, TEXT("WorldToLocal"), TRUE );
		AlphaAdjustmentParameter.Bind( ParameterMap, TEXT("LODAlphaAdjustment"), TRUE );
		WindMatrixOffset.Bind( ParameterMap, TEXT("WindMatrixOffset"), TRUE );
		WindMatrices.Bind( ParameterMap, TEXT("WindMatrices"), TRUE );
		RotationOnlyMatrix.Bind( ParameterMap, TEXT("RotationOnlyMatrix"), TRUE );
		LeafRockAngles.Bind( ParameterMap, TEXT("LeafRockAngles"), TRUE );
		LeafRustleAngles.Bind( ParameterMap, TEXT("LeafRustleAngles"), TRUE );
		CameraAlignMatrix.Bind( ParameterMap, TEXT("CameraAlignMatrix"), TRUE );
		LeafAngleScalars.Bind( ParameterMap, TEXT("LeafAngleScalars"), TRUE );
	}

	virtual void Serialize(FArchive& Ar)
	{
		Ar << LocalToWorldParameter;
		Ar << WorldToLocalParameter;
		Ar << AlphaAdjustmentParameter;
		Ar << WindMatrixOffset;
		Ar << WindMatrices;
		Ar << RotationOnlyMatrix;
		Ar << LeafRockAngles;
		Ar << LeafRustleAngles;
		Ar << CameraAlignMatrix;
		Ar << LeafAngleScalars;
	}

	virtual void Set(FCommandContextRHI* Context, FShader* VertexShader, const FVertexFactory* VertexFactory, const FSceneView* View) const
	{
		if( AlphaAdjustmentParameter.IsBound() )
		{
			FLOAT AlphaAdjustment = ((FSpeedTreeVertexFactory*)VertexFactory)->GetAlphaAdjustment();
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), AlphaAdjustmentParameter, AlphaAdjustment );
		}

		if( RotationOnlyMatrix.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), RotationOnlyMatrix, ((FSpeedTreeVertexFactory*)VertexFactory)->GetRotationOnlyMatrix());
		}

		if( WindMatrixOffset.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), WindMatrixOffset, ((FSpeedTreeBranchVertexFactory*)VertexFactory)->GetWindMatrixOffset() );
		}

		if( WindMatrices.IsBound() )
		{
			SetVertexShaderValues( Context, VertexShader->GetVertexShader(), WindMatrices, (FMatrix*)((FSpeedTreeBranchVertexFactory*)VertexFactory)->GetWindMatrices(), 3);
		}

		if( LeafRockAngles.IsBound() )
		{
			FLOAT* RockAngles = ((FSpeedTreeLeafCardVertexFactory*)VertexFactory)->GetLeafRockAngles( );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), LeafRockAngles, *(FVector*)RockAngles );
		}

		if( LeafRustleAngles.IsBound() )
		{
			FLOAT* RustleAngles = ((FSpeedTreeLeafCardVertexFactory*)VertexFactory)->GetLeafRustleAngles( );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), LeafRustleAngles, *(FVector*)RustleAngles );
		}

		if( CameraAlignMatrix.IsBound() )
		{
			SetVertexShaderValues<FVector4>( Context, VertexShader->GetVertexShader(), CameraAlignMatrix, (FVector4*)&((FSpeedTreeLeafCardVertexFactory*)VertexFactory)->GetCameraAlignMatrix(),3);
		}

		if( LeafAngleScalars.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), LeafAngleScalars, ((FSpeedTreeLeafCardVertexFactory*)VertexFactory)->GetLeafAngleScalars());
		}
	}

	virtual void SetLocalTransforms(FCommandContextRHI* Context, FShader* VertexShader, const FMatrix& LocalToWorld, const FMatrix& WorldToLocal) const
	{
		SetVertexShaderValue( Context, VertexShader->GetVertexShader(), LocalToWorldParameter, LocalToWorld);
		SetVertexShaderValues( Context, VertexShader->GetVertexShader(), WorldToLocalParameter, (FVector4*)&WorldToLocal, 3);
	}

private:
	FShaderParameter LocalToWorldParameter;
	FShaderParameter WorldToLocalParameter;
	FShaderParameter AlphaAdjustmentParameter;
	FShaderParameter WindMatrixOffset;
	FShaderParameter WindMatrices;
	FShaderParameter RotationOnlyMatrix;
	FShaderParameter LeafRockAngles;
	FShaderParameter LeafRustleAngles;
	FShaderParameter CameraAlignMatrix;
	FShaderParameter LeafAngleScalars;
};

IMPLEMENT_VERTEX_FACTORY_TYPE(FSpeedTreeVertexFactory, FSpeedTreeVertexFactoryShaderParameters, "SpeedTreeBillboardVertexFactory", TRUE, TRUE, VER_SPEEDTREE_VERTEXSHADER_RENDERING,0);
IMPLEMENT_VERTEX_FACTORY_TYPE(FSpeedTreeBranchVertexFactory, FSpeedTreeVertexFactoryShaderParameters, "SpeedTreeBranchVertexFactory", TRUE, TRUE, VER_SPEEDTREE_VERTEXSHADER_RENDERING,0);
IMPLEMENT_VERTEX_FACTORY_TYPE(FSpeedTreeLeafCardVertexFactory, FSpeedTreeVertexFactoryShaderParameters, "SpeedTreeLeafCardVertexFactory", TRUE, TRUE, VER_SPEEDTREE_VERTEXSHADER_RENDERING,0);
IMPLEMENT_VERTEX_FACTORY_TYPE(FSpeedTreeLeafMeshVertexFactory, FSpeedTreeVertexFactoryShaderParameters, "SpeedTreeLeafMeshVertexFactory", TRUE, TRUE, VER_SPEEDTREE_VERTEXSHADER_RENDERING,0);


#endif // WITH_SPEEDTREE

