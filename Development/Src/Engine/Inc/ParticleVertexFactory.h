/*=============================================================================
	ParticleVertexFactory.h: Particle vertex factory definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 *	
 */
class FParticleVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleVertexFactory);

public:
	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment);

	//
	void	SetScreenAlignment(BYTE InScreenAlignment)
	{
		ScreenAlignment = InScreenAlignment;
	}

	void	SetLockAxesFlag(BYTE InLockAxisFlag)
	{
		LockAxisFlag = InLockAxisFlag;
	}

	void	SetLockAxes(FVector& InLockAxisUp, FVector& InLockAxisRight)
	{
		LockAxisUp		= InLockAxisUp;
		LockAxisRight	= InLockAxisRight;
	}

	BYTE		GetScreenAlignment()				{	return ScreenAlignment;	}
	BYTE		GetLockAxisFlag()					{	return LockAxisFlag;	}
	FVector&	GetLockAxisUp()						{	return LockAxisUp;		}
	FVector&	GetLockAxisRight()					{	return LockAxisRight;	}

	static UBOOL CreateCachedSpriteParticleDeclaration();

protected:
	/** The RHI vertex declaration used to render the factory normally. */
	static FVertexDeclarationRHIRef SpriteParticleDeclaration;

private:
	BYTE		ScreenAlignment;
	BYTE		LockAxisFlag;
	FVector		LockAxisUp;
	FVector		LockAxisRight;
};

/**
 *	
 */
class FParticleVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		CameraWorldPositionParameter.Bind(ParameterMap,TEXT("CameraWorldPosition"),TRUE);
		CameraRightParameter.Bind(ParameterMap,TEXT("CameraRight"),TRUE);
		CameraUpParameter.Bind(ParameterMap,TEXT("CameraUp"),TRUE);
		ScreenAlignmentParameter.Bind(ParameterMap,TEXT("ScreenAlignment"),TRUE);
		LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorld"));
		AxisRotationVectorSourceIndexParameter.Bind(ParameterMap, TEXT("AxisRotationVectorSourceIndex"));
		AxisRotationVectorsArrayParameter.Bind(ParameterMap, TEXT("AxisRotationVectors"));
		ParticleUpRightResultScalarsParameter.Bind(ParameterMap, TEXT("ParticleUpRightResultScalars"));
	}

	virtual void Serialize(FArchive& Ar)
	{
		Ar << CameraWorldPositionParameter;
		Ar << CameraRightParameter;
		Ar << CameraUpParameter;
		Ar << ScreenAlignmentParameter;
		Ar << LocalToWorldParameter;
		if (Ar.Ver() >= VER_PARTICLE_SPRITE_SUBUV_MERGE)
		{
			Ar << AxisRotationVectorSourceIndexParameter;
			Ar << AxisRotationVectorsArrayParameter;
			Ar << ParticleUpRightResultScalarsParameter;
		}
	}

	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const;

	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
	{
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LocalToWorldParameter,LocalToWorld);
	}

private:
	FShaderParameter CameraWorldPositionParameter;
	FShaderParameter CameraRightParameter;
	FShaderParameter CameraUpParameter;
	FShaderParameter ScreenAlignmentParameter;
	FShaderParameter LocalToWorldParameter;
	FShaderParameter AxisRotationVectorSourceIndexParameter;
	FShaderParameter AxisRotationVectorsArrayParameter;
	FShaderParameter ParticleUpRightResultScalarsParameter;
};
