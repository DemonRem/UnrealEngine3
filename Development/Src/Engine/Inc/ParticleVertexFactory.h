/*=============================================================================
	ParticleVertexFactory.h: Particle vertex factory definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 *	
 */
class FParticleVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleVertexFactory);

public:

	FParticleVertexFactory() :
		EmitterNormalsMode(0)
	{}

	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);

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

	inline void SetNormalsData(BYTE InEmitterNormalsMode, FVector InNormalsSphereCenter, FVector InNormalsCylinderDirection)
	{
		EmitterNormalsMode = InEmitterNormalsMode;
		NormalsSphereCenter = InNormalsSphereCenter;
		NormalsCylinderDirection = InNormalsCylinderDirection;
	}

	BYTE		GetScreenAlignment()				{	return ScreenAlignment;	}
	BYTE		GetLockAxisFlag()					{	return LockAxisFlag;	}
	FVector&	GetLockAxisUp()						{	return LockAxisUp;		}
	FVector&	GetLockAxisRight()					{	return LockAxisRight;	}

	/**For mobile, whether or not to use point sprites for this particle system*/
	virtual	UBOOL UsePointSprites (void) const { return FALSE; } 

	/** Helper function that can be called externally to get screen alignment */
	static ESpriteScreenAlignment StaticGetSpriteScreenAlignment(BYTE LockAxisFlag, BYTE ScreenAlignment);

	/** Return the sprite screen alignment */
	virtual ESpriteScreenAlignment GetSpriteScreenAlignment() const;

	/** Set the vertex factory type */
	void SetVertexFactoryType(BYTE InVertexFactoryType) { VertexFactoryType = InVertexFactoryType; }
	/** Return the vertex factory type */
	BYTE GetVertexFactoryType() const { return VertexFactoryType; }

private:
	BYTE		ScreenAlignment;
	BYTE		LockAxisFlag;
	BYTE		EmitterNormalsMode;
	BYTE		VertexFactoryType;
	FVector		LockAxisUp;
	FVector		LockAxisRight;
	FVector		NormalsSphereCenter;
	FVector		NormalsCylinderDirection;

	friend class FParticleVertexFactoryShaderParameters;
};

/**
 *	
 */
class FParticleDynamicParameterVertexFactory : public FParticleVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleDynamicParameterVertexFactory);

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
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);
};

class FParticlePointSpriteVertexFactory : public FParticleVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticlePointSpriteVertexFactory);

public:
	// FRenderResource interface.
	virtual void InitRHI();

	/**For mobile, whether or not to use point sprites for this particle system*/
	virtual	UBOOL UsePointSprites (void) const { return TRUE; } 
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
		NormalsTypeParameter.Bind(ParameterMap, TEXT("NormalsType"),TRUE);
		NormalsSphereCenterParameter.Bind(ParameterMap, TEXT("NormalsSphereCenter"),TRUE);
		NormalsCylinderUnitDirectionParameter.Bind(ParameterMap, TEXT("NormalsCylinderUnitDirection"),TRUE);
	}

	virtual void Serialize(FArchive& Ar)
	{
		Ar << CameraWorldPositionParameter;
		Ar << CameraRightParameter;
		Ar << CameraUpParameter;
		Ar << ScreenAlignmentParameter;
		Ar << LocalToWorldParameter;
		Ar << AxisRotationVectorSourceIndexParameter;
		Ar << AxisRotationVectorsArrayParameter;
		Ar << ParticleUpRightResultScalarsParameter;
		Ar << NormalsTypeParameter;
		Ar << NormalsSphereCenterParameter;
		Ar << NormalsCylinderUnitDirectionParameter;
		
		// set parameter names for platforms that need them
		CameraWorldPositionParameter.SetShaderParamName(TEXT("CameraWorldPosition"));
		CameraRightParameter.SetShaderParamName(TEXT("CameraRight"));
		CameraUpParameter.SetShaderParamName(TEXT("CameraUp"));
		ScreenAlignmentParameter.SetShaderParamName(TEXT("ScreenAlignment"));
		LocalToWorldParameter.SetShaderParamName(TEXT("LocalToWorld"));
		AxisRotationVectorSourceIndexParameter.SetShaderParamName(TEXT("AxisRotationVectorSourceIndex"));
		AxisRotationVectorsArrayParameter.SetShaderParamName(TEXT("AxisRotationVectors"));
		ParticleUpRightResultScalarsParameter.SetShaderParamName(TEXT("ParticleUpRightResultScalars"));
	}

	virtual void Set(FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView& View) const;

	virtual void SetMesh(FShader* VertexShader,const FMeshElement& Mesh,const FSceneView& View) const;

private:
	FShaderParameter CameraWorldPositionParameter;
	FShaderParameter CameraRightParameter;
	FShaderParameter CameraUpParameter;
	FShaderParameter ScreenAlignmentParameter;
	FShaderParameter LocalToWorldParameter;
	FShaderParameter AxisRotationVectorSourceIndexParameter;
	FShaderParameter AxisRotationVectorsArrayParameter;
	FShaderParameter ParticleUpRightResultScalarsParameter;
	FShaderParameter NormalsTypeParameter;
	FShaderParameter NormalsSphereCenterParameter;
	FShaderParameter NormalsCylinderUnitDirectionParameter;
};
