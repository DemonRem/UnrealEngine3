/*=============================================================================
	LocalVertexFactoryShaderParms.cpp: Local vertex factory shader parameters.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __LOCALVERTEXFACTORYSHADERPARMS_H__
#define __LOCALVERTEXFACTORYSHADERPARMS_H__

class FLocalVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	 * Bind shader constants by name
	 * @param	ParameterMap - mapping of named shader constants to indices
	 */
	virtual void Bind(const FShaderParameterMap& ParameterMap);

	/**
	 * Serialize shader params to an archive
	 * @param	Ar - archive to serialize to
	 */
	virtual void Serialize(FArchive& Ar);

	/**
	 * Set any shader data specific to this vertex factory
	 */
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const;
	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const;

private:
	FShaderParameter LocalToWorldParameter;
	FShaderParameter WorldToLocalParameter;
};

#endif // __LOCALVERTEXFACTORYSHADERPARMS_H__
