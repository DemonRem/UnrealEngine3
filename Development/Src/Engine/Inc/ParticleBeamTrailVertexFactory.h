/*=============================================================================
	ParticleBeamTrailVertexFactory.h: Shared Particle Beam and Trail vertex 
										factory definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 *	
 */
class FParticleBeamTrailVertexFactory : public FParticleVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleBeamTrailVertexFactory);

public:
	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	// FRenderResource interface.
	virtual void InitRHI();

	static UBOOL CreateCachedBeamTrailParticleDeclaration();

protected:
	/** The RHI vertex declaration used to render the factory normally. */
	static FVertexDeclarationRHIRef BeamTrailParticleDeclaration;

private:
};

/**
 *	
 */
class FParticleBeamTrailVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		// Dump all the parameters...
		CameraWorldPositionParameter.Bind(ParameterMap,TEXT("CameraWorldPosition"),TRUE);
		CameraRightParameter.Bind(ParameterMap,TEXT("CameraRight"),TRUE);
		CameraUpParameter.Bind(ParameterMap,TEXT("CameraUp"),TRUE);
		ScreenAlignmentParameter.Bind(ParameterMap,TEXT("ScreenAlignment"),TRUE);
		LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorld"));
	}

	virtual void Serialize(FArchive& Ar)
	{
		Ar << CameraWorldPositionParameter;
		Ar << CameraRightParameter;
		Ar << CameraUpParameter;
		Ar << ScreenAlignmentParameter;
		Ar << LocalToWorldParameter;
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
};
