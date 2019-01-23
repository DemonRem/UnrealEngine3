/*=============================================================================
SpeedTreeVertexFactory.h: SpeedTree vertex factory definition.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if WITH_SPEEDTREE

class FSpeedTreeVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSpeedTreeVertexFactory);
public:

	struct DataType
	{
		FVertexStreamComponent								PositionComponent;
		FVertexStreamComponent								TangentBasisComponents[3];
		FVertexStreamComponent								WindInfo;
		TStaticArray<FVertexStreamComponent,MAX_TEXCOORDS>	TextureCoordinates;
		FVertexStreamComponent								ShadowMapCoordinateComponent;
	};

	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);
	virtual void InitRHI(void);

	void SetData(const DataType& InData)			
	{ 
		Data = InData; 
		UpdateRHI(); 
	}

	void SetAlphaAdjustment(FLOAT InAlphaAdjustment)	
	{ 
		AlphaAdjustment = InAlphaAdjustment; 
	}
	FLOAT GetAlphaAdjustment() const
	{ 
		return AlphaAdjustment; 
	}

	void SetRotationOnlyMatrix(const FMatrix& InRotationOnlyMatrix)	
	{ 
		RotationOnlyMatrix = InRotationOnlyMatrix; 
	}
	const FMatrix& GetRotationOnlyMatrix() const
	{ 
		return RotationOnlyMatrix; 
	}


private:
	DataType	Data;
	FLOAT		AlphaAdjustment;
	FMatrix		RotationOnlyMatrix;
};


class FSpeedTreeBranchVertexFactory : public FSpeedTreeVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSpeedTreeBranchVertexFactory);
public:

	void SetWindMatrices(const FLOAT* InMatrices)	
	{ 
		// assumes 3 4x4 float matrices
		appMemcpy(WindMatrices, InMatrices, 3 * sizeof(FMatrix)); 
	}
	FLOAT* GetWindMatrices() const
	{ 
		return (FLOAT*)WindMatrices[0].M; 
	}

	void SetWindMatrixOffset(FLOAT InWindMatrixOffset)	
	{ 
		WindMatrixOffset = InWindMatrixOffset; 
	}
	FLOAT GetWindMatrixOffset() const
	{ 
		return WindMatrixOffset; 
	}

private:
	FMatrix		WindMatrices[3];
	FLOAT		WindMatrixOffset;
};


class FSpeedTreeLeafCardVertexFactory : public FSpeedTreeBranchVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSpeedTreeLeafCardVertexFactory);
public:

	// pointers are also used to set
	FLOAT* GetLeafRockAngles()
	{ 
		return (FLOAT*)&LeafRockAngles.X; 
	}
	FLOAT* GetLeafRustleAngles()
	{ 
		return (FLOAT*)&LeafRustleAngles.X; 
	}

	void SetCameraAlignMatrix(const FMatrix& InCameraAlignMatrix)
	{
		CameraAlignMatrix = InCameraAlignMatrix;
	}
	const FMatrix& GetCameraAlignMatrix() const
	{
		return CameraAlignMatrix;
	}

	void SetLeafAngleScalars(const FVector2D& InLeafAngleScalars)
	{
		LeafAngleScalars = InLeafAngleScalars;
	}
	const FVector2D& GetLeafAngleScalars() const
	{
		return LeafAngleScalars;
	}

private:
	// angles hardcoded to 3
	FVector		LeafRockAngles;
	FVector		LeafRustleAngles;

	FMatrix		CameraAlignMatrix;
	FVector2D	LeafAngleScalars;
};


class FSpeedTreeLeafMeshVertexFactory : public FSpeedTreeLeafCardVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSpeedTreeLeafMeshVertexFactory);
public:

private:
};

#endif // WITH_SPEEDTREE

