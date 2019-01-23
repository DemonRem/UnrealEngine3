/*=============================================================================
	UnParticleSystemRender.cpp: Particle system rendering functions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"

#include "UnParticleHelper.h"

//@todo.SAS. Remove this once the Trail packing bug is corrected.
#include "ScenePrivate.h"

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_COMPARE_CONSTREF(FParticleOrder,UnParticleComponents,{ return A.Z < B.Z ? 1 : -1; });

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
FParticleDynamicData::FParticleDynamicData(UParticleSystemComponent* PartSysComp)
{
	if( PartSysComp->bForcedInActive == FALSE )
	{
		DynamicEmitterDataArray.Empty(PartSysComp->EmitterInstances.Num());

		for (INT EmitterIndex = 0; EmitterIndex < PartSysComp->EmitterInstances.Num(); EmitterIndex++)
		{
			FDynamicEmitterDataBase* NewDynamicEmitterData = NULL;
			FParticleEmitterInstance* EmitterInst = PartSysComp->EmitterInstances(EmitterIndex);
			if (EmitterInst)
			{
				NewDynamicEmitterData = EmitterInst->GetDynamicData(PartSysComp->IsOwnerSelected());
			}

			if (NewDynamicEmitterData != NULL)
			{
				INT NewIndex = DynamicEmitterDataArray.Add(1);
				check(NewIndex >= 0);
				DynamicEmitterDataArray(NewIndex) = NewDynamicEmitterData;

                // this is used to see PSC that are being updated and sent to the render thread that maybe should not be (e.g. they are across the map!)
				//debugf( TEXT("UpdateDynamicData this component: %s  template: %s"), *PartSysComp->GetOwner()->GetName(), *PartSysComp->Template->GetName() );
			}
		}
	}
	else
	{
		DynamicEmitterDataArray.Empty();
	}
}

void FDynamicSpriteEmitterDataBase::RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses)
{
	check(SceneProxy);

	const FMatrix& LocalToWorld = bUseLocalSpace ? SceneProxy->GetLocalToWorld() : FMatrix::Identity;

	FMatrix CameraToWorld = View->ViewMatrix.Inverse();
	FVector CamX = CameraToWorld.TransformNormal(FVector(1,0,0));
	FVector CamY = CameraToWorld.TransformNormal(FVector(0,1,0));

	FLinearColor EmitterEditorColor = FLinearColor(1.0f,1.0f,0);

	for (INT i = 0; i < ParticleCount; i++)
	{
		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

		FVector DrawLocation = LocalToWorld.TransformFVector(Particle.Location);
		if (bCrosses)
		{
			FVector Size = Particle.Size * Scale;
			PDI->DrawLine(DrawLocation - (0.5f * Size.X * CamX), DrawLocation + (0.5f * Size.X * CamX), EmitterEditorColor, DPGIndex);
			PDI->DrawLine(DrawLocation - (0.5f * Size.Y * CamY), DrawLocation + (0.5f * Size.Y * CamY), EmitterEditorColor, DPGIndex);
		}
		else
		{
			PDI->DrawPoint(DrawLocation, EmitterEditorColor, 2, DPGIndex);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//	ParticleMeshEmitterInstance
///////////////////////////////////////////////////////////////////////////////
// The resource used to render a UMaterialInstanceConstant.
class FMeshEmitterMaterialInstanceResource : public FMaterialRenderProxy
{
public:
	FMeshEmitterMaterialInstanceResource() : 
	    FMaterialRenderProxy()
	  , Parent(NULL)
	{
	}

	FMeshEmitterMaterialInstanceResource(FMaterialRenderProxy* InParent) : 
	    FMaterialRenderProxy()
	  , Parent(InParent)
	{
	}

	virtual UBOOL GetVectorValue(const FName& ParameterName,FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		if (ParameterName == NAME_MeshEmitterVertexColor)
		{
			*OutValue = Param_MeshEmitterVertexColor;
			return TRUE;
		}
		else
		if (ParameterName == NAME_TextureOffsetParameter)
		{ 
			*OutValue = Param_TextureOffsetParameter;
			return TRUE;
		}
		else
		if (ParameterName == NAME_TextureScaleParameter)
		{
			*OutValue = Param_TextureScaleParameter;
			return TRUE;
		}

		if (Parent == NULL)
		{
			return FALSE;
		}

		return Parent->GetVectorValue(ParameterName, OutValue, Context);
	}

	UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		return Parent->GetScalarValue(ParameterName, OutValue, Context);
	}

	UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue) const
	{
		return Parent->GetTextureValue(ParameterName, OutValue);
	}

	virtual const FMaterial* GetMaterial() const
	{
		return Parent->GetMaterial();
	}

	FMaterialRenderProxy* Parent;
	FLinearColor Param_MeshEmitterVertexColor;
	FLinearColor Param_TextureOffsetParameter;
	FLinearColor Param_TextureScaleParameter;
};

///////////////////////////////////////////////////////////////////////////////
//	FDynamicSpriteEmitterData
///////////////////////////////////////////////////////////////////////////////
UBOOL FDynamicSpriteEmitterData::GetVertexAndIndexData(FParticleSpriteVertex* VertexData, void* FillIndexData, TArray<FParticleOrder>* ParticleOrder)
{
	INT ParticleCount = ActiveParticleCount;
	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	INT StartIndex = 0;
	INT EndIndex = ParticleCount;
	if ((MaxDrawCount >= 0) && (ParticleCount > MaxDrawCount))
	{
		ParticleCount = MaxDrawCount;
	}

	// Pack the data
	INT	ParticleIndex;
	INT	ParticlePackingIndex = 0;
	INT	IndexPackingIndex = 0;

	FParticleSpriteVertex* Vertices = VertexData;
	WORD* Indices = (WORD*)FillIndexData;

	UBOOL bSorted = ParticleOrder ? TRUE : FALSE;

	for (INT i = 0; i < ParticleCount; i++)
	{
		if (bSorted)
		{
			ParticleIndex	= (*ParticleOrder)(i).ParticleIndex;
		}
		else
		{
			ParticleIndex	= i;
		}

		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);

		FVector Size = Particle.Size * Scale;
		if (ScreenAlignment == PSA_Square)
		{
			Size.Y = Size.X;
		}

		FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;
		FVector OrbitOffset(0.0f, 0.0f, 0.0f);
		FVector PrevOrbitOffset(0.0f, 0.0f, 0.0f);

		if (OrbitModuleOffset != 0)
		{
			INT CurrentOffset = OrbitModuleOffset;
			const BYTE* ParticleBase = (const BYTE*)&Particle;
			PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
			OrbitOffset = OrbitPayload.Offset;

			if (bUseLocalSpace == FALSE)
			{
				OrbitOffset = SceneProxy->GetLocalToWorld().TransformNormal(OrbitOffset);
			}
			PrevOrbitOffset = OrbitPayload.PreviousOffset;

			LocalOrbitPayload = &OrbitPayload;
		}

		// 0
		Vertices[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		Vertices[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		Vertices[ParticlePackingIndex].Size			= Size;
		Vertices[ParticlePackingIndex].Tex_U		= 0.f;
		Vertices[ParticlePackingIndex].Tex_V		= 0.f;
		Vertices[ParticlePackingIndex].Rotation		= Particle.Rotation;
		Vertices[ParticlePackingIndex].Color		= Particle.Color;
		ParticlePackingIndex++;
		// 1
		Vertices[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		Vertices[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		Vertices[ParticlePackingIndex].Size			= Size;
		Vertices[ParticlePackingIndex].Tex_U		= 0.f;
		Vertices[ParticlePackingIndex].Tex_V		= 1.f;
		Vertices[ParticlePackingIndex].Rotation		= Particle.Rotation;
		Vertices[ParticlePackingIndex].Color		= Particle.Color;
		ParticlePackingIndex++;
		// 2
		Vertices[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		Vertices[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		Vertices[ParticlePackingIndex].Size			= Size;
		Vertices[ParticlePackingIndex].Tex_U		= 1.f;
		Vertices[ParticlePackingIndex].Tex_V		= 1.f;
		Vertices[ParticlePackingIndex].Rotation		= Particle.Rotation;
		Vertices[ParticlePackingIndex].Color		= Particle.Color;
		ParticlePackingIndex++;
		// 3
		Vertices[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		Vertices[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		Vertices[ParticlePackingIndex].Size			= Size;
		Vertices[ParticlePackingIndex].Tex_U		= 1.f;
		Vertices[ParticlePackingIndex].Tex_V		= 0.f;
		Vertices[ParticlePackingIndex].Rotation		= Particle.Rotation;
		Vertices[ParticlePackingIndex].Color		= Particle.Color;
		ParticlePackingIndex++;

		if (Indices)
		{
			*Indices++ = (i * 4) + 0;
			*Indices++ = (i * 4) + 2;
			*Indices++ = (i * 4) + 3;
			*Indices++ = (i * 4) + 0;
			*Indices++ = (i * 4) + 1;
			*Indices++ = (i * 4) + 2;
		}

		if (LocalOrbitPayload)
		{
			LocalOrbitPayload->PreviousOffset = OrbitOffset;
		}
	}

	return TRUE;
}

void FDynamicSpriteEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	if (EmitterRenderMode == ERM_Normal)
	{
		FMatrix LocalToWorld = Proxy->GetLocalToWorld();

		VertexFactory->SetScreenAlignment(ScreenAlignment);
		VertexFactory->SetLockAxesFlag(LockAxisFlag);
		if (LockAxisFlag != EPAL_NONE)
		{
			FVector Up, Right;
			Proxy->GetAxisLockValues((FDynamicSpriteEmitterDataBase*)this, Up, Right);
			VertexFactory->SetLockAxes(Up, Right);
		}

		INT ParticleCount = ActiveParticleCount;

		UBOOL bSorted = FALSE;
		if( bRequiresSorting )
		{
			// If material is using unlit translucency and the blend mode is translucent or 
			// if it is using unlit distortion then we need to sort (back to front)
			const FMaterial* Material = MaterialResource->GetMaterial();
			if (Material && 
				Material->GetLightingModel() == MLM_Unlit && 
				(Material->GetBlendMode() == BLEND_Translucent || Material->IsDistorted())
				)
			{
				SCOPE_CYCLE_COUNTER(STAT_SortingTime);

				ParticleOrder.Empty(ParticleCount);

				// Take UseLocalSpace into account!
				UBOOL bLocalSpace = bUseLocalSpace;

				for (INT ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
				{
					DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
					FLOAT InZ;
					if (bLocalSpace)
					{
						InZ = View->ViewProjectionMatrix.TransformFVector(LocalToWorld.TransformFVector(Particle.Location)).Z;
					}
					else
					{
						InZ = View->ViewProjectionMatrix.TransformFVector(Particle.Location).Z;
					}
					new(ParticleOrder)FParticleOrder(ParticleIndex, InZ);
				}
				Sort<USE_COMPARE_CONSTREF(FParticleOrder,UnParticleComponents)>(&(ParticleOrder(0)),ParticleOrder.Num());
				bSorted	= TRUE;
			}
		}

		FMeshElement Mesh;

		Mesh.UseDynamicData			= TRUE;
		Mesh.IndexBuffer			= NULL;
		Mesh.VertexFactory			= VertexFactory;
		Mesh.DynamicVertexData		= this;
		Mesh.DynamicVertexStride	= sizeof(FParticleSpriteVertex);
		Mesh.DynamicIndexData		= NULL;
		Mesh.DynamicIndexStride		= 0;
		if (bSorted == TRUE)
		{
			Mesh.DynamicIndexData	= (void*)(&(ParticleOrder));
		}
		Mesh.LCI = NULL;
		if (bUseLocalSpace == TRUE)
		{
			Mesh.LocalToWorld = LocalToWorld;
			Mesh.WorldToLocal = Proxy->GetWorldToLocal();
		}
		else
		{
			Mesh.LocalToWorld = FMatrix::Identity;
			Mesh.WorldToLocal = FMatrix::Identity;
		}
		Mesh.FirstIndex				= 0;
		Mesh.MinVertexIndex			= 0;
		Mesh.MaxVertexIndex			= VertexCount - 1;
		Mesh.ParticleType			= PET_Sprite;
		Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
		Mesh.CastShadow				= Proxy->GetCastShadow();
		Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;

		Mesh.MaterialRenderProxy		= MaterialResource;
		Mesh.NumPrimitives			= ParticleCount;
		Mesh.Type					= PT_TriangleList;

		DrawRichMesh(
			PDI, 
			Mesh, 
			FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
			FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
			FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
			Proxy->GetPrimitiveSceneInfo(),
			Proxy->GetSelected()
			);
	}
	else
	if (EmitterRenderMode == ERM_Point)
	{
		RenderDebug(PDI, View, DPGIndex, FALSE);
	}
	else
	if (EmitterRenderMode == ERM_Cross)
	{
		RenderDebug(PDI, View, DPGIndex, TRUE);
	}
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicSubUVEmitterData
///////////////////////////////////////////////////////////////////////////////
UBOOL FDynamicSubUVEmitterData::GetVertexAndIndexData(FParticleSpriteSubUVVertex* VertexData, void* FillIndexData, TArray<FParticleOrder>* ParticleOrder)
{
	INT ParticleCount = ActiveParticleCount;
	// 'clamp' the number of particles actually drawn
	//@todo.SAS. If sorted, we really want to render the front 'N' particles...
	// right now it renders the back ones. (Same for SubUV draws)
	INT StartIndex = 0;
	INT EndIndex = ParticleCount;
	if ((MaxDrawCount >= 0) && (ParticleCount > MaxDrawCount))
	{
		ParticleCount = MaxDrawCount;
	}

	// Pack the data
	INT	ParticleIndex;
	INT	ParticlePackingIndex = 0;
	INT	IndexPackingIndex = 0;

	INT			SIHorz			= SubImages_Horizontal;
	INT			SIVert			= SubImages_Vertical;
	INT			iTotalSubImages = SIHorz * SIVert;
	FLOAT		baseU			= (1.0f / (FLOAT)SIHorz);
	FLOAT		baseV			= (1.0f / (FLOAT)SIVert);
	FLOAT		U;
	FLOAT		V;

	FParticleSpriteSubUVVertex* Vertices = VertexData;
	WORD* Indices = (WORD*)FillIndexData;

	UBOOL bSorted = ParticleOrder ? TRUE : FALSE;

	for (INT i = 0; i < ParticleCount; i++)
	{
		if (bSorted)
		{
			ParticleIndex	= (*ParticleOrder)(i).ParticleIndex;
		}
		else
		{
			ParticleIndex	= i;
		}

		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);

		FVector Size = Particle.Size * Scale;
		if (ScreenAlignment == PSA_Square)
		{
			Size.Y = Size.X;
		}

		FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;
		FVector OrbitOffset(0.0f, 0.0f, 0.0f);
		FVector PrevOrbitOffset(0.0f, 0.0f, 0.0f);

		if (OrbitModuleOffset != 0)
		{
			INT CurrentOffset = OrbitModuleOffset;
			const BYTE* ParticleBase = (const BYTE*)&Particle;
			PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
			OrbitOffset = OrbitPayload.Offset;

			if (bUseLocalSpace == FALSE)
			{
				OrbitOffset = SceneProxy->GetLocalToWorld().TransformNormal(OrbitOffset);
			}
			PrevOrbitOffset = OrbitPayload.PreviousOffset;

			LocalOrbitPayload = &OrbitPayload;
		}

		FSubUVSpritePayload* PayloadData = (FSubUVSpritePayload*)(((BYTE*)&Particle) + SubUVDataOffset);

		// 0
		VertexData[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		VertexData[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		VertexData[ParticlePackingIndex].Size			= Size;
		VertexData[ParticlePackingIndex].Rotation		= Particle.Rotation;
		VertexData[ParticlePackingIndex].Color			= Particle.Color;
		VertexData[ParticlePackingIndex].Interp			= PayloadData->Interpolation;
		if (bDirectUV)
		{
			U	= baseU * (PayloadData->ImageH + 0);
			V	= baseV * (PayloadData->ImageV + 0);
    		VertexData[ParticlePackingIndex].Tex_U	= U;
	    	VertexData[ParticlePackingIndex].Tex_V	= V;
			VertexData[ParticlePackingIndex].Tex_U2	= U;
			VertexData[ParticlePackingIndex].Tex_V2	= V;
		}
		else
		{
    		VertexData[ParticlePackingIndex].Tex_U	= baseU * (appTrunc(PayloadData->ImageH) + 0);
	    	VertexData[ParticlePackingIndex].Tex_V	= baseV * (appTrunc(PayloadData->ImageV) + 0);
			VertexData[ParticlePackingIndex].Tex_U2	= baseU * (appTrunc(PayloadData->Image2H) + 0);
			VertexData[ParticlePackingIndex].Tex_V2	= baseV * (appTrunc(PayloadData->Image2V) + 0);
		}
		VertexData[ParticlePackingIndex].SizeU		= 0.f;
		VertexData[ParticlePackingIndex].SizeV		= 0.f;
		ParticlePackingIndex++;
		// 1
		VertexData[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		VertexData[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		VertexData[ParticlePackingIndex].Size			= Size;
		VertexData[ParticlePackingIndex].Rotation		= Particle.Rotation;
		VertexData[ParticlePackingIndex].Color			= Particle.Color;
		VertexData[ParticlePackingIndex].Interp			= PayloadData->Interpolation;
		if (bDirectUV)
		{
			U	= baseU * (PayloadData->ImageH + 0);
			V	= baseV * (PayloadData->ImageV + PayloadData->Image2V);
			VertexData[ParticlePackingIndex].Tex_U	= U;
			VertexData[ParticlePackingIndex].Tex_V	= V;
			VertexData[ParticlePackingIndex].Tex_U2	= U;
			VertexData[ParticlePackingIndex].Tex_V2	= V;
		}
		else
		{
			VertexData[ParticlePackingIndex].Tex_U	= baseU * (appTrunc(PayloadData->ImageH) + 0);
			VertexData[ParticlePackingIndex].Tex_V	= baseV * (appTrunc(PayloadData->ImageV) + 1);
			VertexData[ParticlePackingIndex].Tex_U2	= baseU * (appTrunc(PayloadData->Image2H) + 0);
			VertexData[ParticlePackingIndex].Tex_V2	= baseV * (appTrunc(PayloadData->Image2V) + 1);
		}
		VertexData[ParticlePackingIndex].SizeU		= 0.f;
		VertexData[ParticlePackingIndex].SizeV		= 1.f;
		ParticlePackingIndex++;
		// 2
		VertexData[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		VertexData[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		VertexData[ParticlePackingIndex].Size			= Size;
		VertexData[ParticlePackingIndex].Rotation		= Particle.Rotation;
		VertexData[ParticlePackingIndex].Color			= Particle.Color;
		VertexData[ParticlePackingIndex].Interp			= PayloadData->Interpolation;
		if (bDirectUV)
		{
			U	= baseU * (PayloadData->ImageH + PayloadData->Image2H);
			V	= baseV * (PayloadData->ImageV + PayloadData->Image2V);
    		VertexData[ParticlePackingIndex].Tex_U	= U;
	    	VertexData[ParticlePackingIndex].Tex_V	= V;
			VertexData[ParticlePackingIndex].Tex_U2	= U;
			VertexData[ParticlePackingIndex].Tex_V2	= V;
		}
		else
		{
    		VertexData[ParticlePackingIndex].Tex_U	= baseU * (appTrunc(PayloadData->ImageH) + 1);
	    	VertexData[ParticlePackingIndex].Tex_V	= baseV * (appTrunc(PayloadData->ImageV) + 1);
			VertexData[ParticlePackingIndex].Tex_U2	= baseU * (appTrunc(PayloadData->Image2H) + 1);
			VertexData[ParticlePackingIndex].Tex_V2	= baseV * (appTrunc(PayloadData->Image2V) + 1);
		}
		VertexData[ParticlePackingIndex].SizeU		= 1.f;
		VertexData[ParticlePackingIndex].SizeV		= 1.f;
		ParticlePackingIndex++;
		// 3
		VertexData[ParticlePackingIndex].Position		= Particle.Location + OrbitOffset;
		VertexData[ParticlePackingIndex].OldPosition	= Particle.OldLocation + PrevOrbitOffset;
		VertexData[ParticlePackingIndex].Size			= Size;
		VertexData[ParticlePackingIndex].Rotation		= Particle.Rotation;
		VertexData[ParticlePackingIndex].Color			= Particle.Color;
		VertexData[ParticlePackingIndex].Interp			= PayloadData->Interpolation;
		if (bDirectUV)
		{
			U	= baseU * (PayloadData->ImageH + PayloadData->Image2H);
			V	= baseV * (PayloadData->ImageV + 0);
    		VertexData[ParticlePackingIndex].Tex_U	= U;
	    	VertexData[ParticlePackingIndex].Tex_V	= V;
			VertexData[ParticlePackingIndex].Tex_U2	= U;
			VertexData[ParticlePackingIndex].Tex_V2	= V;
		}
		else
		{
    		VertexData[ParticlePackingIndex].Tex_U	= baseU * (appTrunc(PayloadData->ImageH) + 1);
	    	VertexData[ParticlePackingIndex].Tex_V	= baseV * (appTrunc(PayloadData->ImageV) + 0);
			VertexData[ParticlePackingIndex].Tex_U2	= baseU * (appTrunc(PayloadData->Image2H) + 1);
			VertexData[ParticlePackingIndex].Tex_V2	= baseV * (appTrunc(PayloadData->Image2V) + 0);
		}
		VertexData[ParticlePackingIndex].SizeU		= 1.f;
		VertexData[ParticlePackingIndex].SizeV		= 0.f;
		ParticlePackingIndex++;

		if (Indices)
		{
			*Indices++ = (i * 4) + 0;
			*Indices++ = (i * 4) + 2;
			*Indices++ = (i * 4) + 3;
			*Indices++ = (i * 4) + 0;
			*Indices++ = (i * 4) + 1;
			*Indices++ = (i * 4) + 2;
		}

		if (LocalOrbitPayload)
		{
			LocalOrbitPayload->PreviousOffset = OrbitOffset;
		}
	}

	return TRUE;
}

void FDynamicSubUVEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	if (EmitterRenderMode == ERM_Normal)
	{
		FMatrix LocalToWorld = Proxy->GetLocalToWorld();

		VertexFactory->SetScreenAlignment(ScreenAlignment);
		VertexFactory->SetLockAxesFlag(LockAxisFlag);
		if (LockAxisFlag != EPAL_NONE)
		{
			FVector Up, Right;
			Proxy->GetAxisLockValues((FDynamicSpriteEmitterDataBase*)this, Up, Right);
			VertexFactory->SetLockAxes(Up, Right);
		}

		INT ParticleCount = ActiveParticleCount;

		UBOOL bSorted = FALSE;
		if( bRequiresSorting )
		{
			// If material is using unlit translucency and the blend mode is translucent or 
			// if it is using unlit distortion then we need to sort (back to front)
			const FMaterial* Material = MaterialResource->GetMaterial();
			if (Material && 
				Material->GetLightingModel() == MLM_Unlit && 
				(Material->GetBlendMode() == BLEND_Translucent || Material->IsDistorted())
				)
			{
				SCOPE_CYCLE_COUNTER(STAT_SortingTime);

				ParticleOrder.Empty(ParticleCount);

				// Take UseLocalSpace into account!
				UBOOL bLocalSpace = bUseLocalSpace;

				for (INT ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
				{
					DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
					FLOAT InZ;
					if (bLocalSpace)
					{
						InZ = View->ViewProjectionMatrix.TransformFVector(LocalToWorld.TransformFVector(Particle.Location)).Z;
					}
					else
					{
						InZ = View->ViewProjectionMatrix.TransformFVector(Particle.Location).Z;
					}
					new(ParticleOrder)FParticleOrder(ParticleIndex, InZ);
				}
				Sort<USE_COMPARE_CONSTREF(FParticleOrder,UnParticleComponents)>(&(ParticleOrder(0)),ParticleOrder.Num());
				bSorted	= TRUE;
			}
		}

		FMeshElement Mesh;

		Mesh.UseDynamicData			= TRUE;
		Mesh.IndexBuffer			= NULL;
		Mesh.VertexFactory			= VertexFactory;
		Mesh.DynamicVertexData		= this;
		Mesh.DynamicVertexStride	= sizeof(FParticleSpriteSubUVVertex);
		Mesh.DynamicIndexData		= NULL;
		Mesh.DynamicIndexStride		= 0;
		if (bSorted == TRUE)
		{
			Mesh.DynamicIndexData	= (void*)(&(ParticleOrder));
		}
		Mesh.LCI = NULL;
		if (bUseLocalSpace == TRUE)
		{
			Mesh.LocalToWorld = Proxy->GetLocalToWorld();
			Mesh.WorldToLocal = Proxy->GetWorldToLocal();
		}
		else
		{
			Mesh.LocalToWorld = FMatrix::Identity;
			Mesh.WorldToLocal = FMatrix::Identity;
		}
		Mesh.FirstIndex				= 0;
		Mesh.MinVertexIndex			= 0;
		Mesh.MaxVertexIndex			= VertexCount - 1;
		Mesh.ParticleType			= PET_SubUV;
		Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
		Mesh.CastShadow				= Proxy->GetCastShadow();
		Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;

		Mesh.MaterialRenderProxy		= MaterialResource;
		Mesh.NumPrimitives			= ParticleCount;
		Mesh.Type					= PT_TriangleList;

		DrawRichMesh(
			PDI, 
			Mesh, 
			FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
			FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
			FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
			Proxy->GetPrimitiveSceneInfo(),
			Proxy->GetSelected()
			);
	}
	else
	if (EmitterRenderMode == ERM_Point)
	{
		RenderDebug(PDI, View, DPGIndex, FALSE);
	}
	else
	if (EmitterRenderMode == ERM_Cross)
	{
		RenderDebug(PDI, View, DPGIndex, TRUE);
	}
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicMeshEmitterData
///////////////////////////////////////////////////////////////////////////////
FDynamicMeshEmitterData::FDynamicMeshEmitterData(const FParticleMeshEmitterInstance* InEmitterInstance,
	INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource,
	INT InSubUVInterpMethod, INT InSubUVDataOffset, UStaticMesh* InStaticMesh, 
	const UStaticMeshComponent* InStaticMeshComponent, INT InMaxDrawCount) :
	  FDynamicSpriteEmitterDataBase(InParticleCount, InParticleStride, bRequiresSorting, InMaterialResource, InMaxDrawCount)
	, SubUVInterpMethod(InSubUVInterpMethod)
	, SubUVDataOffset(InSubUVDataOffset)
	, StaticMesh(InStaticMesh)
{
	eEmitterType = DET_Mesh;
	if (InParticleCount > 0)
	{
		check(InParticleCount < 16 * 1024);	// TTP #33375
		check(InParticleStride < 2 * 1024);	// TTP #3375
		ParticleData = (BYTE*)appRealloc(ParticleData, InParticleCount * InParticleStride);
		check(ParticleData);
		ParticleIndices = (WORD*)appRealloc(ParticleIndices, InParticleCount * sizeof(WORD));
		check(ParticleIndices);
	}

	// Build the proxy's LOD data.
	LODs.Empty(StaticMesh->LODModels.Num());
	for (INT LODIndex = 0;LODIndex < StaticMesh->LODModels.Num();LODIndex++)
	{
		new(LODs) FLODInfo(InStaticMeshComponent, InEmitterInstance, LODIndex, bSelected);
	}
}

FDynamicMeshEmitterData::~FDynamicMeshEmitterData()
{
}

/** Information used by the proxy about a single LOD of the mesh. */
FDynamicMeshEmitterData::FLODInfo::FLODInfo(const UStaticMeshComponent* Component,
	const FParticleMeshEmitterInstance* MeshEmitInst, INT LODIndex, UBOOL bSelected)
{
	// Gather the materials applied to the LOD.
	Elements.Empty(Component->StaticMesh->LODModels(LODIndex).Elements.Num());
	for (INT MaterialIndex = 0; MaterialIndex < Component->StaticMesh->LODModels(LODIndex).Elements.Num(); MaterialIndex++)
	{
		FElementInfo ElementInfo;

		ElementInfo.MaterialInterface = NULL;

		// Determine the material applied to this element of the LOD.
		UMaterialInterface* MatInst = NULL;

		if (MeshEmitInst->Materials.Num() > MaterialIndex)
		{
			MatInst = MeshEmitInst->Materials(MaterialIndex);
		}

		if (MatInst == NULL)
		{
			MatInst = Component->GetMaterial(MaterialIndex,LODIndex);
		}

		if (MatInst)
		{
			ElementInfo.MaterialInterface = MatInst;
			MeshEmitInst->Component->SMMaterialInterfaces.AddUniqueItem(MatInst);
		}

		if (ElementInfo.MaterialInterface == NULL)
		{
			ElementInfo.MaterialInterface = GEngine->DefaultMaterial;
			MeshEmitInst->Component->SMMaterialInterfaces.AddUniqueItem(MatInst);
		}
		// Store the element info.
		Elements.AddItem(ElementInfo);
	}
}

void FDynamicMeshEmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	if (EmitterRenderMode == ERM_Normal)
	{
		EParticleSubUVInterpMethod eSubUVMethod = (EParticleSubUVInterpMethod)(SubUVInterpMethod);

		UBOOL bWireframe = ((View->Family->ShowFlags & SHOW_Wireframe) && !(View->Family->ShowFlags & SHOW_Materials));

		FMatrix kMat(FMatrix::Identity);
		// Reset velocity and size.

		INT ParticleCount = ActiveParticleCount;
		if ((MaxDrawCount >= 0) && (ParticleCount > MaxDrawCount))
		{
			ParticleCount = MaxDrawCount;
		}

		for (INT i = ParticleCount - 1; i >= 0; i--)
		{
			const INT	CurrentIndex	= ParticleIndices[i];
			const BYTE* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);

			if (Particle.RelativeTime < 1.0f)
			{
				FTranslationMatrix kTransMat(Particle.Location);
				FScaleMatrix kScaleMat(Particle.Size * Scale);
				FRotator kRotator;
				FMatrix Local2World = Proxy->GetLocalToWorld();

				if (ScreenAlignment == PSA_TypeSpecific)
				{
					FVector Location = Particle.Location;
					if (bUseLocalSpace)
					{
						Location = Local2World.TransformFVector(Location);
						kTransMat.SetOrigin(Location);
						Local2World.SetIdentity();
					}
					FVector	DirToCamera	= View->ViewOrigin - Location;
					DirToCamera.Normalize();
					if (DirToCamera.SizeSquared() <	0.5f)
					{
						// Assert possible if DirToCamera is not normalized
						DirToCamera	= FVector(1,0,0);
					}

					FVector	LocalSpaceFacingAxis = FVector(1,0,0); // facing axis is taken to be the local x axis.	
					FVector	LocalSpaceUpAxis = FVector(0,0,1); // up axis is taken to be the local z axis

					if (MeshAlignment == PSMA_MeshFaceCameraWithLockedAxis)
					{
						// TODO: Allow an arbitrary	vector to serve	as the locked axis

						// For the locked axis behavior, only rotate to	face the camera	about the
						// locked direction, and maintain the up vector	pointing towards the locked	direction
						// Find	the	rotation that points the localupaxis towards the targetupaxis
						FQuat PointToUp	= FQuatFindBetween(LocalSpaceUpAxis, LockedAxis);

						// Add in rotation about the TargetUpAxis to point the facing vector towards the camera
						FVector	DirToCameraInRotationPlane = DirToCamera - ((DirToCamera | LockedAxis)*LockedAxis);
						DirToCameraInRotationPlane.Normalize();
						FQuat PointToCamera	= FQuatFindBetween(PointToUp.RotateVector(LocalSpaceFacingAxis), DirToCameraInRotationPlane);

						// Set kRotator	to the composed	rotation
						FQuat MeshRotation = PointToCamera*PointToUp;
						kRotator = FRotator(MeshRotation);
					}
					else
					if (MeshAlignment == PSMA_MeshFaceCameraWithSpin)
					{
						// Implement a tangent-rotation	version	of point-to-camera.	 The facing	direction points to	the	camera,
						// with	no roll, and has addtional sprite-particle rotation	about the tangential axis
						// (c.f. the roll rotation is about	the	radial axis)

						// Find	the	rotation that points the facing	axis towards the camera
						FRotator PointToRotation = FRotator(FQuatFindBetween(LocalSpaceFacingAxis, DirToCamera));

						// When	constructing the rotation, we need to eliminate	roll around	the	dirtocamera	axis,
						// otherwise the particle appears to rotate	around the dircamera axis when it or the camera	moves
						PointToRotation.Roll = 0;

						// Add in the tangential rotation we do	want.
						FVector	vPositivePitch = FVector(0,0,1); //	this is	set	by the rotator's yaw/pitch/roll	reference frame
						FVector	vTangentAxis = vPositivePitch^DirToCamera;
						vTangentAxis.Normalize();
						if (vTangentAxis.SizeSquared() < 0.5f)
						{
							vTangentAxis = FVector(1,0,0); // assert is	possible if	FQuat axis/angle constructor is	passed zero-vector
						}

						FQuat AddedTangentialRotation =	FQuat(vTangentAxis,	Particle.Rotation);

						// Set kRotator	to the composed	rotation
						FQuat MeshRotation = AddedTangentialRotation*PointToRotation.Quaternion();
						kRotator = FRotator(MeshRotation);
					}
					else
					//if (MeshAlignment == PSMA_MeshFaceCameraWithRoll)
					{
						// Implement a roll-rotation version of	point-to-camera.  The facing direction points to the camera,
						// with	no roll, and then rotates about	the	direction_to_camera	by the spriteparticle rotation.

						// Find	the	rotation that points the facing	axis towards the camera
						FRotator PointToRotation = FRotator(FQuatFindBetween(LocalSpaceFacingAxis, DirToCamera));

						// When	constructing the rotation, we need to eliminate	roll around	the	dirtocamera	axis,
						// otherwise the particle appears to rotate	around the dircamera axis when it or the camera	moves
						PointToRotation.Roll = 0;

						// Add in the roll we do want.
						FQuat AddedRollRotation	= FQuat(DirToCamera, Particle.Rotation);

						// Set kRotator	to the composed	rotation
						FQuat MeshRotation = AddedRollRotation*PointToRotation.Quaternion();
						kRotator = FRotator(MeshRotation);
					}
				}
				else
				if (bMeshRotationActive)
				{
					FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
					kRotator = FRotator::MakeFromEuler(PayloadData->Rotation);
				}
				else
				{
					FLOAT fRot = Particle.Rotation * 180.0f / PI;
					FVector kRotVec = FVector(fRot, fRot, fRot);
					kRotator = FRotator::MakeFromEuler(kRotVec);
				}

				FRotationMatrix kRotMat(kRotator);
				kMat = kScaleMat * kRotMat * kTransMat;

				FVector OrbitOffset(0.0f, 0.0f, 0.0f);
				if (OrbitModuleOffset != 0)
				{
					INT CurrentOffset = OrbitModuleOffset;
					PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
					OrbitOffset = OrbitPayload.Offset;
					if (bUseLocalSpace == FALSE)
					{
						OrbitOffset = SceneProxy->GetLocalToWorld().TransformNormal(OrbitOffset);
					}

					FTranslationMatrix OrbitMatrix(OrbitOffset);
					kMat *= OrbitMatrix;
				}

				if (bUseLocalSpace)
				{
					kMat *= Local2World;
				}

				UBOOL bBadParent = FALSE;

				TArray<FMeshEmitterMaterialInstanceResource*> MEMatInstRes;
				//@todo. Handle LODs...
				//for (INT LODIndex = 0; LODIndex < MeshData->LODs.Num(); LODIndex++)
				for (INT LODIndex = 0; LODIndex < 1; LODIndex++)
				{
					FDynamicMeshEmitterData::FLODInfo* LODInfo = &(LODs(LODIndex));

					MEMatInstRes.Empty(LODInfo->Elements.Num());
					for (INT ElementIndex = 0; ElementIndex < LODInfo->Elements.Num(); ElementIndex++)
					{
						INT MIIndex = MEMatInstRes.Add();
						check(MIIndex >= 0);
						FMeshEmitterMaterialInstanceResource* MIRes = ::new FMeshEmitterMaterialInstanceResource();
						check(MIRes);
						MEMatInstRes(MIIndex) = MIRes;
						MIRes->Param_MeshEmitterVertexColor = Particle.Color;
						if (SubUVInterpMethod != PSUVIM_None)
						{
							FSubUVMeshPayload* SubUVPayload = NULL;

							if ((SubUVInterpMethod == PSUVIM_Random) || (SubUVInterpMethod == PSUVIM_Random_Blend))
							{
								FSubUVMeshRandomPayload* TempPayload = (FSubUVMeshRandomPayload*)(((BYTE*)&Particle) + SubUVDataOffset);
								SubUVPayload	= (FSubUVMeshPayload*)(TempPayload);
							}
							else
							{
								SubUVPayload	= (FSubUVMeshPayload*)(((BYTE*)&Particle) + SubUVDataOffset);
							}

							MIRes->Param_TextureOffsetParameter = 
								FLinearColor(SubUVPayload->UVOffset.X, SubUVPayload->UVOffset.Y, 0.0f, 0.0f);

							if (bScaleUV)
							{
								MIRes->Param_TextureScaleParameter = 
									FLinearColor((1.0f / (FLOAT)SubImages_Horizontal),
									(1.0f / (FLOAT)SubImages_Vertical),
									0.0f, 0.0f);
							}
							else
							{
								MIRes->Param_TextureScaleParameter = 
									FLinearColor(1.0f, 1.0f, 0.0f, 0.0f);
							}
						}

						const FDynamicMeshEmitterData::FLODInfo::FElementInfo& Info = LODInfo->Elements(ElementIndex);
						MIRes->Parent = Info.MaterialInterface->GetRenderProxy(bSelected);

						if (MIRes->Parent == NULL)
						{
							bBadParent = TRUE;
						}
					}
				}

				if (bBadParent == TRUE)
				{
					continue;
				}

				// Fill in the MeshElement and draw...
				const FStaticMeshRenderData& LODModel = StaticMesh->LODModels(0);
				FDynamicMeshEmitterData::FLODInfo* LODInfo = &(LODs(0));

				// Draw the static mesh elements.
				for(INT ElementIndex = 0;ElementIndex < LODModel.Elements.Num();ElementIndex++)
				{
					FMeshEmitterMaterialInstanceResource* MIRes = MEMatInstRes(ElementIndex);
					const FStaticMeshElement& Element = LODModel.Elements(ElementIndex);
					if (Element.NumTriangles == 0)
					{
						//@todo. This should never happen... but it does.
						continue;
					}
					FMeshElement Mesh;
					Mesh.VertexFactory = &LODModel.VertexFactory;
					Mesh.DynamicVertexData = NULL;
					Mesh.LCI = NULL;

					Mesh.LocalToWorld = kMat;
					Mesh.WorldToLocal = kMat.Inverse();
					//@todo motionblur

					Mesh.FirstIndex = Element.FirstIndex;
					Mesh.MinVertexIndex = Element.MinVertexIndex;
					Mesh.MaxVertexIndex = Element.MaxVertexIndex;
					Mesh.UseDynamicData = FALSE;
					Mesh.ReverseCulling = Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
					Mesh.CastShadow = Proxy->GetCastShadow();
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DPGIndex;
					if( bWireframe && LODModel.WireframeIndexBuffer.IsInitialized() )
					{
						Mesh.IndexBuffer = &LODModel.WireframeIndexBuffer;
						Mesh.MaterialRenderProxy = Proxy->GetDeselectedWireframeMatInst();
						Mesh.Type = PT_LineList;
						Mesh.NumPrimitives = LODModel.WireframeIndexBuffer.Indices.Num() / 2;
					}
					else
					{
						if (MaterialResource)
						{
							MIRes->Parent = (FMaterialRenderProxy*)(MaterialResource);
						}
						else
						if (MIRes->Parent == NULL)
						{
							if (Element.Material)
							{
								MIRes->Parent = Element.Material->GetRenderProxy(bSelected);
							}
							else
							{
								MIRes->Parent = GEngine->DefaultMaterial->GetRenderProxy(bSelected);
							}
						}

						Mesh.bWireframe = bWireframe;
						Mesh.IndexBuffer = &LODModel.IndexBuffer;
						Mesh.MaterialRenderProxy = MIRes;
						Mesh.Type = PT_TriangleList;
						Mesh.NumPrimitives = Element.NumTriangles;
					}
					PDI->DrawMesh(Mesh);
				}

				for (INT MEMIRIndex = 0; MEMIRIndex < MEMatInstRes.Num(); MEMIRIndex++)
				{
					FMeshEmitterMaterialInstanceResource* Res = MEMatInstRes(MEMIRIndex);
					if (Res)
					{
						delete Res;
						MEMatInstRes(MEMIRIndex) = NULL;
					}
				}
				MEMatInstRes.Empty();
			}
			else
			{
				// Remove it from the scene???
			}
		}
	}
	else
	if (EmitterRenderMode == ERM_Point)
	{
		RenderDebug(PDI, View, DPGIndex, FALSE);
	}
	else
	if (EmitterRenderMode == ERM_Cross)
	{
		RenderDebug(PDI, View, DPGIndex, TRUE);
	}
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicBeam2EmitterData
///////////////////////////////////////////////////////////////////////////////
void FDynamicBeam2EmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	VertexFactory->SetScreenAlignment(ScreenAlignment);
	VertexFactory->SetLockAxesFlag(LockAxisFlag);
	if (LockAxisFlag != EPAL_NONE)
	{
		FVector Up, Right;
		Proxy->GetAxisLockValues((FDynamicSpriteEmitterDataBase*)this, Up, Right);
		VertexFactory->SetLockAxes(Up, Right);
	}

	// Allocate and generate the data...
	// Determine the required particle count
	if (VertexData == NULL)
	{
		VertexData = (FParticleSpriteVertex*)appRealloc(VertexData, VertexCount * sizeof(FParticleSpriteVertex));
		check(VertexData);
	}

	INT	TrianglesToRender;

	INT	TrianglesToRender_Index = FillIndexData(Proxy, PDI, View, DPGIndex);
	//if (bSmoothNoise_Enabled || bLowFreqNoise_Enabled || bHighFreqNoise_Enabled)
	if (bLowFreqNoise_Enabled)
	{
		TrianglesToRender = FillData_Noise(Proxy, PDI, View, DPGIndex);
	}
	else
	{
		TrianglesToRender = FillVertexData_NoNoise(Proxy, PDI, View, DPGIndex);
	}
	TrianglesToRender = TrianglesToRender_Index;

	if (TrianglesToRender > 0)
	{
		FMeshElement Mesh;

		Mesh.IndexBuffer			= NULL;
		Mesh.VertexFactory			= VertexFactory;
		Mesh.DynamicVertexData		= VertexData;
		Mesh.DynamicVertexStride	= sizeof(FParticleSpriteVertex);
		Mesh.DynamicIndexData		= IndexData;
		Mesh.DynamicIndexStride		= IndexStride;
		Mesh.LCI					= NULL;
		if (bUseLocalSpace == TRUE)
		{
			Mesh.LocalToWorld = Proxy->GetLocalToWorld();
			Mesh.WorldToLocal = Proxy->GetWorldToLocal();
		}
		else
		{
			Mesh.LocalToWorld = FMatrix::Identity;
			Mesh.WorldToLocal = FMatrix::Identity;
		}
		Mesh.FirstIndex				= 0;
		if ((TrianglesToRender % 2) != 0)
		{
			TrianglesToRender--;
		}
		Mesh.NumPrimitives			= TrianglesToRender;
		Mesh.MinVertexIndex			= 0;
		Mesh.MaxVertexIndex			= VertexCount - 1;
		Mesh.UseDynamicData			= TRUE;
		Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
		Mesh.CastShadow				= Proxy->GetCastShadow();
		Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;

		//if ((View->Family->ShowFlags & SHOW_Wireframe) && !(View->Family->ShowFlags & SHOW_Materials))
		if (0)
		{
			Mesh.MaterialRenderProxy		= Proxy->GetDeselectedWireframeMatInst();
			Mesh.Type					= PT_LineList;
		}
		else
		{
			Mesh.MaterialRenderProxy		= MaterialResource;
			Mesh.Type					= PT_TriangleStrip;
		}

		//		PDI->DrawMesh(Mesh);
		DrawRichMesh(
			PDI,
			Mesh,
			FLinearColor(1.0f, 0.0f, 0.0f),
			FLinearColor(1.0f, 1.0f, 0.0f),
			FLinearColor(1.0f, 1.0f, 1.0f),
			Proxy->GetPrimitiveSceneInfo(),
			Proxy->GetSelected()
			);

		if (bRenderDirectLine == TRUE)
		{
			RenderDirectLine(Proxy, PDI, View, DPGIndex);
		}

		if ((bRenderLines == TRUE) ||
			(bRenderTessellation == TRUE))
		{
			RenderLines(Proxy, PDI, View, DPGIndex);
		}
	}
}

void FDynamicBeam2EmitterData::RenderDirectLine(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	for (INT Beam = 0; Beam < ActiveParticleCount; Beam++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * Beam);

		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		FLOAT*					NoiseRate			= NULL;
		FLOAT*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		FLOAT*					TaperValues			= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}

		DrawWireStar(PDI, BeamPayloadData->SourcePoint, 20.0f, FColor(0,255,0),DPGIndex);
		DrawWireStar(PDI, BeamPayloadData->TargetPoint, 20.0f, FColor(255,0,0),DPGIndex);
		PDI->DrawLine(BeamPayloadData->SourcePoint, BeamPayloadData->TargetPoint, FColor(255,255,0),DPGIndex);
	}
}

void FDynamicBeam2EmitterData::RenderLines(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	if (bLowFreqNoise_Enabled)
	{
		INT	TrianglesToRender = 0;

		FMatrix WorldToLocal = Proxy->GetWorldToLocal();
		FMatrix LocalToWorld = Proxy->GetLocalToWorld();
		FMatrix CameraToWorld = View->ViewMatrix.Inverse();
		FVector	ViewOrigin = CameraToWorld.GetOrigin();

		Sheets = (Sheets > 0) ? Sheets : 1;

		// Frequency is the number of noise points to generate, evenly distributed along the line.
		Frequency = (Frequency > 0) ? Frequency : 1;

		// NoiseTessellation is the amount of tessellation that should occur between noise points.
		INT	TessFactor	= NoiseTessellation ? NoiseTessellation : 1;
		FLOAT	InvTessFactor	= 1.0f / TessFactor;
		FColor	Color(255,0,0);
		INT		i;

		// The last position processed
		FVector	LastPosition, LastDrawPosition, LastTangent;
		// The current position
		FVector	CurrPosition, CurrDrawPosition;
		// The target
		FVector	TargetPosition, TargetDrawPosition;
		// The next target
		FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
		// The interperted draw position
		FVector InterpDrawPos;
		FVector	InterimDrawPosition;

		FVector	Size;

		FVector Location;
		FVector EndPoint;
		FVector Offset;
		FVector LastOffset;
		FLOAT	fStrength;
		FLOAT	fTargetStrength;

		INT	 VertexCount	= 0;

		// Tessellate the beam along the noise points
		for (i = 0; i < ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

			// Retrieve the beam data from the particle.
			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;
			FLOAT*					NoiseDistanceScale	= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (InterpolatedPointsOffset != -1)
			{
				InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
			}
			if (NoiseRateOffset != -1)
			{
				NoiseRate = (FLOAT*)((BYTE*)Particle + NoiseRateOffset);
			}
			if (NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (FLOAT*)((BYTE*)Particle + NoiseDeltaTimeOffset);
			}
			if (TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((BYTE*)Particle + TargetNoisePointsOffset);
			}
			if (NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((BYTE*)Particle + NextNoisePointsOffset);
			}
			if (TaperValuesOffset != -1)
			{
				TaperValues = (FLOAT*)((BYTE*)Particle + TaperValuesOffset);
			}
			if (NoiseDistanceScaleOffset != -1)
			{
				NoiseDistanceScale = (FLOAT*)((BYTE*)Particle + NoiseDistanceScaleOffset);
			}

			FLOAT NoiseDistScale = 1.0f;
			if (NoiseDistanceScale)
			{
				NoiseDistScale = *NoiseDistanceScale;
			}

			FVector* NoisePoints	= TargetNoisePoints;
			FVector* NextNoise		= NextNoisePoints;

			FLOAT NoiseRangeScaleFactor = NoiseRangeScale;
			//@todo. How to handle no noise points?
			// If there are no noise points, why are we in here?
			if (NoisePoints == NULL)
			{
				continue;
			}

			// Pin the size to the X component
			Size = FVector(Particle->Size.X * Scale.X);

			check(TessFactor > 0);

			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetAxis(0);
				fStrength	= NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;
			fTargetStrength	= NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			UBOOL	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			// Reset the texture coordinate
			LastPosition		= BeamPayloadData->SourcePoint;
			LastDrawPosition	= LastPosition;

			// Determine the current position by stepping the direct line and offsetting with the noise point. 
			CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

			if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
			{
				NoiseDir		= NextNoise[0] - NoisePoints[0];
				NoiseDir.Normalize();
				CheckNoisePoint	= NoisePoints[0] + NoiseDir * NoiseSpeed * *NoiseRate;
				if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[0].X) < NoiseLockRadius) &&
					(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[0].Y) < NoiseLockRadius) &&
					(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[0].Z) < NoiseLockRadius))
				{
					NoisePoints[0]	= NextNoise[0];
				}
				else
				{
					NoisePoints[0]	= CheckNoisePoint;
				}
			}

			CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[0] * NoiseDistScale);

			// Determine the offset for the leading edge
			Location	= LastDrawPosition;
			EndPoint	= CurrDrawPosition;

			// 'Lead' edge
			DrawWireStar(PDI, Location, 15.0f, FColor(0,255,0), DPGIndex);

			for (INT StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
			{
				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * NoiseSpeed * *NoiseRate;
					if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex].X) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < NoiseLockRadius))
					{
						NoisePoints[StepIndex]	= NextNoise[StepIndex];
					}
					else
					{
						NoisePoints[StepIndex]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex] * NoiseDistScale);

				// Prep the next draw position to determine tangents
				UBOOL bTarget = FALSE;
				NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
				if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
				{
					// If we are locked, and the next step is the target point, set the draw position as such.
					// (ie, we are on the last noise point...)
					NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
					if (bTargetNoise)
					{
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
							{
								NoisePoints[Frequency]	= NextNoise[Frequency];
							}
							else
							{
								NoisePoints[Frequency]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
					}
					TargetTangent = BeamPayloadData->TargetTangent;
					fTargetStrength	= BeamPayloadData->TargetStrength;
				}
				else
				{
					// Just another noise point... offset the target to get the draw position.
					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < NoiseLockRadius))
						{
							NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
						}
						else
						{
							NoisePoints[StepIndex + 1]	= CheckNoisePoint;
						}
					}

					NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex + 1] * NoiseDistScale);

					TargetTangent = ((1.0f - NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				InterimDrawPosition = LastDrawPosition;
				// Tessellate between the current position and the last position
				for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;

					FColor StarColor(255,0,255);
					if (TessIndex == 0)
					{
						StarColor = FColor(0,0,255);
					}
					else
					if (TessIndex == (TessFactor - 1))
					{
						StarColor = FColor(255,255,0);
					}

					// Generate the vertex
					DrawWireStar(PDI, EndPoint, 15.0f, StarColor, DPGIndex);
					PDI->DrawLine(Location, EndPoint, FLinearColor(1.0f,1.0f,0.0f), DPGIndex);
					InterimDrawPosition	= InterpDrawPos;
				}
				LastPosition		= CurrPosition;
				LastDrawPosition	= CurrDrawPosition;
				LastTangent			= TargetTangent;
			}

			if (bLocked)
			{
				// Draw the line from the last point to the target
				CurrDrawPosition	= BeamPayloadData->TargetPoint;
				if (bTargetNoise)
				{
					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
						{
							NoisePoints[Frequency]	= NextNoise[Frequency];
						}
						else
						{
							NoisePoints[Frequency]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
				}

				if (bUseTarget)
				{
					TargetTangent = BeamPayloadData->TargetTangent;
				}
				else
				{
					NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					TargetTangent = ((1.0f - NoiseTension) / 2.0f) * 
						(NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				// Tessellate this segment
				InterimDrawPosition = LastDrawPosition;
				for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;

					FColor StarColor(255,0,255);
					if (TessIndex == 0)
					{
						StarColor = FColor(255,255,255);
					}
					else
					if (TessIndex == (TessFactor - 1))
					{
						StarColor = FColor(255,255,0);
					}

					// Generate the vertex
					DrawWireStar(PDI, EndPoint, 15.0f, StarColor, DPGIndex);
					PDI->DrawLine(Location, EndPoint, FLinearColor(1.0f,1.0f,0.0f), DPGIndex);
					VertexCount++;
					InterimDrawPosition	= InterpDrawPos;
				}
			}
		}
	}

	if (InterpolationPoints > 1)
	{
		FMatrix CameraToWorld = View->ViewMatrix.Inverse();
		FVector	ViewOrigin = CameraToWorld.GetOrigin();
		INT TessFactor = InterpolationPoints ? InterpolationPoints : 1;

		if (TessFactor <= 1)
		{
			for (INT i = 0; i < ActiveParticleCount; i++)
			{
				DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);
				FBeam2TypeDataPayload* BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
				if (BeamPayloadData->TriangleCount == 0)
				{
					continue;
				}

				FVector EndPoint	= Particle->Location;
				FVector Location	= BeamPayloadData->SourcePoint;

				DrawWireStar(PDI, Location, 15.0f, FColor(255,0,0), DPGIndex);
				DrawWireStar(PDI, EndPoint, 15.0f, FColor(255,0,0), DPGIndex);
				PDI->DrawLine(Location, EndPoint, FColor(255,255,0), DPGIndex);
			}
		}
		else
		{
			for (INT i = 0; i < ActiveParticleCount; i++)
			{
				DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

				FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
				FVector*				InterpolatedPoints	= NULL;

				BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
				if (BeamPayloadData->TriangleCount == 0)
				{
					continue;
				}
				if (InterpolatedPointsOffset != -1)
				{
					InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
				}

				FVector Location;
				FVector EndPoint;

				check(InterpolatedPoints);	// TTP #33139

				Location	= BeamPayloadData->SourcePoint;
				EndPoint	= InterpolatedPoints[0];

				DrawWireStar(PDI, Location, 15.0f, FColor(255,0,0), DPGIndex);
				for (INT StepIndex = 0; StepIndex < BeamPayloadData->InterpolationSteps; StepIndex++)
				{
					EndPoint = InterpolatedPoints[StepIndex];
					DrawWireStar(PDI, EndPoint, 15.0f, FColor(255,0,0), DPGIndex);
					PDI->DrawLine(Location, EndPoint, FColor(255,255,0), DPGIndex);
					Location = EndPoint;
				}
			}
		}
	}
}

void FDynamicBeam2EmitterData::RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses)
{
}

INT FDynamicBeam2EmitterData::FillIndexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	INT	TrianglesToRender = 0;

	SCOPE_CYCLE_COUNTER(STAT_BeamIBPackingTime);

	// Beam2 polygons are packed and joined as follows:
	//
	// 1--3--5--7--9-...
	// |\ |\ |\ |\ |\...
	// | \| \| \| \| ...
	// 0--2--4--6--8-...
	//
	// (ie, the 'leading' edge of polygon (n) is the trailing edge of polygon (n+1)
	//
	// NOTE: This is primed for moving to tri-strips...
	//
	INT TessFactor	= InterpolationPoints ? InterpolationPoints : 1;
	if (Sheets <= 0)
	{
		Sheets = 1;
	}

	//	UBOOL bWireframe = ((View->Family->ShowFlags & SHOW_Wireframe) && !(View->Family->ShowFlags & SHOW_Materials));
	UBOOL bWireframe = FALSE;

	if (IndexData == NULL)
	{
		IndexCount = 0;
		for (INT ii = 0; ii < TrianglesPerSheet.Num(); ii++)
		{
			INT Triangles = TrianglesPerSheet(ii);
			if (bWireframe)
			{
				IndexCount += (8 * Triangles + 2) * Sheets;
			}
			else
			{
				if (IndexCount == 0)
				{
					IndexCount = 2;
				}
				IndexCount += Triangles * Sheets;
				IndexCount += 4 * (Sheets - 1);	// Degenerate indices between sheets
				if ((ii + 1) < TrianglesPerSheet.Num())
				{
					IndexCount += 4;	// Degenerate indices between beams
				}
			}
		}
		IndexData = appRealloc(IndexData, IndexCount * IndexStride);
		check(IndexData);
	}

	if (IndexStride == sizeof(WORD))
	{
		WORD*	Index				= (WORD*)IndexData;
		WORD	VertexIndex			= 0;
		WORD	StartVertexIndex	= 0;

		for (INT Beam = 0; Beam < ActiveParticleCount; Beam++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * Beam);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}

			if (bWireframe)
			{
				for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
				{
					VertexIndex = 0;

					// The 'starting' line
					TrianglesToRender += 1;
					*(Index++) = StartVertexIndex + 0;
					*(Index++) = StartVertexIndex + 1;

					// 4 lines per quad
					INT TriCount = TrianglesPerSheet(Beam);
					INT QuadCount = TriCount / 2;
					TrianglesToRender += TriCount * 2;

					for (INT i = 0; i < QuadCount; i++)
					{
						*(Index++) = StartVertexIndex + VertexIndex + 0;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 3;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 3;

						VertexIndex += 2;
					}

					StartVertexIndex += TriCount + 2;
				}
			}
			else
			{
				// 
				if (Beam == 0)
				{
					*(Index++) = VertexIndex++;	// SheetIndex + 0
					*(Index++) = VertexIndex++;	// SheetIndex + 1
				}

				for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
				{
					// 2 triangles per tessellation factor
					TrianglesToRender += BeamPayloadData->TriangleCount;

					// Sequentially step through each triangle - 1 vertex per triangle
					for (INT i = 0; i < BeamPayloadData->TriangleCount; i++)
					{
						*(Index++) = VertexIndex++;
					}

					// Degenerate tris
					if ((SheetIndex + 1) < Sheets)
					{
						*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
						*(Index++) = VertexIndex;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// Second vertex of the next sheet

						TrianglesToRender += 4;
					}
				}
				if ((Beam + 1) < ActiveParticleCount)
				{
					*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
					*(Index++) = VertexIndex;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// Second vertex of the next sheet

					TrianglesToRender += 4;
				}
			}
		}
	}
	else
	{
		check(!TEXT("Rendering beam with > 5000 vertices!"));
		DWORD*	Index		= (DWORD*)IndexData;
		DWORD	VertexIndex	= 0;
		for (INT Beam = 0; Beam < ActiveParticleCount; Beam++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * Beam);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}

			// 
			if (Beam == 0)
			{
				*(Index++) = VertexIndex++;	// SheetIndex + 0
				*(Index++) = VertexIndex++;	// SheetIndex + 1
			}

			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				// 2 triangles per tessellation factor
				TrianglesToRender += BeamPayloadData->TriangleCount;

				// Sequentially step through each triangle - 1 vertex per triangle
				for (INT i = 0; i < BeamPayloadData->TriangleCount; i++)
				{
					*(Index++) = VertexIndex++;
				}

				// Degenerate tris
				if ((SheetIndex + 1) < Sheets)
				{
					*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
					*(Index++) = VertexIndex;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// First vertex of the next sheet
					*(Index++) = VertexIndex++;		// Second vertex of the next sheet
					TrianglesToRender += 4;
				}
			}
			if ((Beam + 1) < ActiveParticleCount)
			{
				*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
				*(Index++) = VertexIndex;		// First vertex of the next sheet
				*(Index++) = VertexIndex++;		// First vertex of the next sheet
				*(Index++) = VertexIndex++;		// Second vertex of the next sheet
				TrianglesToRender += 4;
			}
		}
	}

	return TrianglesToRender;
}

INT FDynamicBeam2EmitterData::FillVertexData_NoNoise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	INT	TrianglesToRender = 0;

	FParticleSpriteVertex* Vertex = (FParticleSpriteVertex*)VertexData;
	FMatrix CameraToWorld = View->ViewMatrix.Inverse();
	FVector	ViewOrigin = CameraToWorld.GetOrigin();
	INT TessFactor = InterpolationPoints ? InterpolationPoints : 1;

	if (Sheets <= 0)
	{
		Sheets = 1;
	}

	if (TessFactor <= 1)
	{
		FVector	Offset, LastOffset;
		FVector	Size;

		for (INT i = 0; i < ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (InterpolatedPointsOffset != -1)
			{
				InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
			}
			if (NoiseRateOffset != -1)
			{
				NoiseRate = (FLOAT*)((BYTE*)Particle + NoiseRateOffset);
			}
			if (NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (FLOAT*)((BYTE*)Particle + NoiseDeltaTimeOffset);
			}
			if (TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((BYTE*)Particle + TargetNoisePointsOffset);
			}
			if (NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((BYTE*)Particle + NextNoisePointsOffset);
			}
			if (TaperValuesOffset != -1)
			{
				TaperValues = (FLOAT*)((BYTE*)Particle + TaperValuesOffset);
			}

			// Pin the size to the X component
			Size	= FVector(Particle->Size.X * Scale.X);

			FVector EndPoint	= Particle->Location;
			FVector Location	= BeamPayloadData->SourcePoint;
			FVector Right		= Location - EndPoint;
			Right.Normalize();
			FVector Up			= Right ^  (Location - ViewOrigin);
			Up.Normalize();
			FVector WorkingUp	= Up;

			FLOAT	fUEnd;
			FLOAT	Tiles		= 1.0f;
			if (TextureTileDistance > KINDA_SMALL_NUMBER)
			{
				FVector	Direction	= BeamPayloadData->TargetPoint - BeamPayloadData->SourcePoint;
				FLOAT	Distance	= Direction.Size();
				Tiles				= Distance / TextureTileDistance;
			}
			fUEnd		= Tiles;

			if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
			{
				fUEnd	= Tiles * BeamPayloadData->TravelRatio;
			}

			// For the direct case, this isn't a big deal, as it will not require much work per sheet.
			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				if (SheetIndex)
				{
					FLOAT	Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
					FQuat	QuatRotator	= FQuat(Right, Angle);
					WorkingUp			= QuatRotator.RotateVector(Up);
				}

				FLOAT	Taper	= 1.0f;

				if (TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				Offset.X		= WorkingUp.X * Size.X * Taper;
				Offset.Y		= WorkingUp.Y * Size.Y * Taper;
				Offset.Z		= WorkingUp.Z * Size.Z * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + Offset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= 0.0f;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;

				Vertex->Position	= Location - Offset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= 0.0f;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;

				if (TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[1];
				}

				Offset.X		= WorkingUp.X * Size.X * Taper;
				Offset.Y		= WorkingUp.Y * Size.Y * Taper;
				Offset.Z		= WorkingUp.Z * Size.Z * Taper;

				//
				Vertex->Position	= EndPoint + Offset;
				Vertex->OldPosition	= Particle->OldLocation;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fUEnd;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;

				Vertex->Position	= EndPoint - Offset;
				Vertex->OldPosition	= Particle->OldLocation;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fUEnd;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
			}
		}
	}
	else
	{
		FVector	Offset;
		FVector	Size;

		FLOAT	fTextureIncrement	= 1.0f / InterpolationPoints;;

		for (INT i = 0; i < ActiveParticleCount; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

			FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;

			BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
			if (BeamPayloadData->TriangleCount == 0)
			{
				continue;
			}
			if (InterpolatedPointsOffset != -1)
			{
				InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
			}
			if (NoiseRateOffset != -1)
			{
				NoiseRate = (FLOAT*)((BYTE*)Particle + NoiseRateOffset);
			}
			if (NoiseDeltaTimeOffset != -1)
			{
				NoiseDelta = (FLOAT*)((BYTE*)Particle + NoiseDeltaTimeOffset);
			}
			if (TargetNoisePointsOffset != -1)
			{
				TargetNoisePoints = (FVector*)((BYTE*)Particle + TargetNoisePointsOffset);
			}
			if (NextNoisePointsOffset != -1)
			{
				NextNoisePoints = (FVector*)((BYTE*)Particle + NextNoisePointsOffset);
			}
			if (TaperValuesOffset != -1)
			{
				TaperValues = (FLOAT*)((BYTE*)Particle + TaperValuesOffset);
			}

			if (TextureTileDistance > KINDA_SMALL_NUMBER)
			{
				FVector	Direction	= BeamPayloadData->TargetPoint - BeamPayloadData->SourcePoint;
				FLOAT	Distance	= Direction.Size();
				FLOAT	Tiles		= Distance / TextureTileDistance;
				fTextureIncrement	= Tiles / InterpolationPoints;
			}

			// Pin the size to the X component
			Size	= FVector(Particle->Size.X * Scale.X);

			FLOAT	Angle;
			FQuat	QuatRotator(0, 0, 0, 0);

			FVector Location;
			FVector EndPoint;
			FVector Right;
			FVector Up;
			FVector WorkingUp;
			FLOAT	fU;

			check(InterpolatedPoints);	// TTP #33139
			// For the direct case, this isn't a big deal, as it will not require much work per sheet.
			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				fU			= 0.0f;
				Location	= BeamPayloadData->SourcePoint;
				EndPoint	= InterpolatedPoints[0];
				Right		= Location - EndPoint;
				Right.Normalize();
				Up			= Right ^  (Location - ViewOrigin);
				Up.Normalize();

				if (SheetIndex)
				{
					Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
					QuatRotator	= FQuat(Right, Angle);

					WorkingUp	= QuatRotator.RotateVector(Up);
				}
				else
				{
					WorkingUp	= Up;
				}

				FLOAT	Taper	= 1.0f;

				if (TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				Offset.X	= WorkingUp.X * Size.X * Taper;
				Offset.Y	= WorkingUp.Y * Size.Y * Taper;
				Offset.Z	= WorkingUp.Z * Size.Z * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + Offset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;

				Vertex->Position	= Location - Offset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;

				for (INT StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					EndPoint	= InterpolatedPoints[StepIndex];
					Up			= Right ^  (Location - ViewOrigin);
					Up.Normalize();
					if (SheetIndex)
					{
						WorkingUp		= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[StepIndex + 1];
					}

					Offset.X		= WorkingUp.X * Size.X * Taper;
					Offset.Y		= WorkingUp.Y * Size.Y * Taper;
					Offset.Z		= WorkingUp.Z * Size.Z * Taper;

					//
					Vertex->Position	= EndPoint + Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;

					Vertex->Position	= EndPoint - Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;

					Location			 = EndPoint;
					fU					+= fTextureIncrement;
				}

				if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
				{
/***
					check(BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints) == 0);

					if (BeamPayloadData->Steps < TessFactor)
					{
					if (TaperMethod != PEBTM_None)
					{
					FLOAT	TargetTaper	= TaperValues[BeamPayloadData->Steps];
					Taper	= (Taper + TargetTaper) / 2.0f;
					}

					// This is jsut a partial line along the beam
					EndPoint	= Location + (InterpolatedPoints[BeamPayloadData->Steps] - Location) * BeamPayloadData->TravelRatio;
					Up			= Right ^  (EndPoint - ViewOrigin);
					Up.Normalize();
					if (SheetIndex)
					{
					WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
					WorkingUp	= Up;
					}

					Offset.X	= WorkingUp.X * Size.X * Taper;
					Offset.Y	= WorkingUp.Y * Size.Y * Taper;
					Offset.Z	= WorkingUp.Z * Size.Z * Taper;

					//
					Vertex->Position	= EndPoint + Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement * BeamPayloadData->TravelRatio;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;

					Vertex->Position	= EndPoint - Offset;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU + fTextureIncrement * BeamPayloadData->TravelRatio;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					}
					else
					{
					// Most likely, have gone past the target
					//@todo. How to handle this case...
					}
***/
				}
			}
		}
	}

	return TrianglesToRender;
}

INT FDynamicBeam2EmitterData::FillData_Noise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	INT	TrianglesToRender = 0;

	if (InterpolationPoints > 0)
	{
		return FillData_InterpolatedNoise(Proxy, PDI, View, DPGIndex);
	}

	FParticleSpriteVertex*			Vertex			= (FParticleSpriteVertex*)VertexData;
	FMatrix							CameraToWorld	= View->ViewMatrix.Inverse();
	
	if (Sheets <= 0)
	{
		Sheets = 1;
	}

	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	// Frequency is the number of noise points to generate, evenly distributed along the line.
	if (Frequency <= 0)
	{
		Frequency = 1;
	}

	// NoiseTessellation is the amount of tessellation that should occur between noise points.
	INT	TessFactor	= NoiseTessellation ? NoiseTessellation : 1;
	
	FLOAT	InvTessFactor	= 1.0f / TessFactor;
	FColor	Color(255,0,0);
	INT		i;

	// The last position processed
	FVector	LastPosition, LastDrawPosition, LastTangent;
	// The current position
	FVector	CurrPosition, CurrDrawPosition;
	// The target
	FVector	TargetPosition, TargetDrawPosition;
	// The next target
	FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
	// The interperted draw position
	FVector InterpDrawPos;
	FVector	InterimDrawPosition;

	FVector	Size;

	FLOAT	Angle;
	FQuat	QuatRotator;

	FVector Location;
	FVector EndPoint;
	FVector Right;
	FVector Up;
	FVector WorkingUp;
	FVector LastUp;
	FVector WorkingLastUp;
	FVector Offset;
	FVector LastOffset;
	FLOAT	fStrength;
	FLOAT	fTargetStrength;

	FLOAT	fU;
	FLOAT	TextureIncrement	= 1.0f / (((Frequency > 0) ? Frequency : 1) * TessFactor);	// TTP #33140/33159

	INT	 VertexCount	= 0;

	FMatrix WorldToLocal = Proxy->GetWorldToLocal();
	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	// Tessellate the beam along the noise points
	for (i = 0; i < ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

		// Retrieve the beam data from the particle.
		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		FLOAT*					NoiseRate			= NULL;
		FLOAT*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		FLOAT*					TaperValues			= NULL;
		FLOAT*					NoiseDistanceScale	= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}
		if (InterpolatedPointsOffset != -1)
		{
			InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
		}
		if (NoiseRateOffset != -1)
		{
			NoiseRate = (FLOAT*)((BYTE*)Particle + NoiseRateOffset);
		}
		if (NoiseDeltaTimeOffset != -1)
		{
			NoiseDelta = (FLOAT*)((BYTE*)Particle + NoiseDeltaTimeOffset);
		}
		if (TargetNoisePointsOffset != -1)
		{
			TargetNoisePoints = (FVector*)((BYTE*)Particle + TargetNoisePointsOffset);
		}
		if (NextNoisePointsOffset != -1)
		{
			NextNoisePoints = (FVector*)((BYTE*)Particle + NextNoisePointsOffset);
		}
		if (TaperValuesOffset != -1)
		{
			TaperValues = (FLOAT*)((BYTE*)Particle + TaperValuesOffset);
		}
		if (NoiseDistanceScaleOffset != -1)
		{
			NoiseDistanceScale = (FLOAT*)((BYTE*)Particle + NoiseDistanceScaleOffset);
		}

		FLOAT NoiseDistScale = 1.0f;
		if (NoiseDistanceScale)
		{
			NoiseDistScale = *NoiseDistanceScale;
		}

		FVector* NoisePoints	= TargetNoisePoints;
		FVector* NextNoise		= NextNoisePoints;

		FLOAT NoiseRangeScaleFactor = NoiseRangeScale;
		//@todo. How to handle no noise points?
		// If there are no noise points, why are we in here?
		if (NoisePoints == NULL)
		{
			continue;
		}

		// Pin the size to the X component
		Size	= FVector(Particle->Size.X * Scale.X);

		if (TessFactor <= 1)
		{
			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetAxis(0);
				fStrength	= NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;

			fTargetStrength	= NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			UBOOL	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				// Reset the texture coordinate
				fU					= 0.0f;
				LastPosition		= BeamPayloadData->SourcePoint;
				LastDrawPosition	= LastPosition;

				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[0] - NoisePoints[0];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[0] + NoiseDir * NoiseSpeed * *NoiseRate;
					if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[0].X) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[0].Y) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[0].Z) < NoiseLockRadius))
					{
						NoisePoints[0]	= NextNoise[0];
					}
					else
					{
						NoisePoints[0]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[0] * NoiseDistScale);

				// Determine the offset for the leading edge
				Location	= LastDrawPosition;
				EndPoint	= CurrDrawPosition;
				Right		= Location - EndPoint;
				Right.Normalize();
				LastUp		= Right ^  (Location - CameraToWorld.GetOrigin());
				LastUp.Normalize();

				if (SheetIndex)
				{
					Angle			= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
					QuatRotator		= FQuat(Right, Angle);
					WorkingLastUp	= QuatRotator.RotateVector(LastUp);
				}
				else
				{
					WorkingLastUp	= LastUp;
				}

				FLOAT	Taper	= 1.0f;

				if (TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
				LastOffset.Y	= WorkingLastUp.Y * Size.Y * Taper;
				LastOffset.Z	= WorkingLastUp.Z * Size.Z * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				VertexCount++;

				Vertex->Position	= Location - LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				VertexCount++;

				fU	+= TextureIncrement;

				for (INT StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					// Determine the current position by stepping the direct line and offsetting with the noise point. 
					CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < NoiseLockRadius))
						{
							NoisePoints[StepIndex]	= NextNoise[StepIndex];
						}
						else
						{
							NoisePoints[StepIndex]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex] * NoiseDistScale);

					// Prep the next draw position to determine tangents
					UBOOL bTarget = FALSE;
					NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
					{
						// If we are locked, and the next step is the target point, set the draw position as such.
						// (ie, we are on the last noise point...)
						NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
						if (bTargetNoise)
						{
							if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
							{
								NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
								NoiseDir.Normalize();
								CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
								if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
								{
									NoisePoints[Frequency]	= NextNoise[Frequency];
								}
								else
								{
									NoisePoints[Frequency]	= CheckNoisePoint;
								}
							}

							NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
						}
						TargetTangent = BeamPayloadData->TargetTangent;
						fTargetStrength	= BeamPayloadData->TargetStrength;
					}
					else
					{
						// Just another noise point... offset the target to get the draw position.
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < NoiseLockRadius))
							{
								NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
							}
							else
							{
								NoisePoints[StepIndex + 1]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex + 1] * NoiseDistScale);

						TargetTangent = ((1.0f - NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					InterimDrawPosition = LastDrawPosition;
					// Tessellate between the current position and the last position
					for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						Up			= Right ^  (Location - CameraToWorld.GetOrigin());
						Up.Normalize();

						if (SheetIndex)
						{
							Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
						}

						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.Y * Taper;
						Offset.Z	= WorkingUp.Z * Size.Z * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
					LastPosition		= CurrPosition;
					LastDrawPosition	= CurrDrawPosition;
					LastTangent			= TargetTangent;
				}

				if (bLocked)
				{
					// Draw the line from the last point to the target
					CurrDrawPosition	= BeamPayloadData->TargetPoint;
					if (bTargetNoise)
					{
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
							{
								NoisePoints[Frequency]	= NextNoise[Frequency];
							}
							else
							{
								NoisePoints[Frequency]	= CheckNoisePoint;
							}
						}

						CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
					}

					if (bUseTarget)
					{
						TargetTangent = BeamPayloadData->TargetTangent;
					}
					else
					{
						NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
						TargetTangent = ((1.0f - NoiseTension) / 2.0f) * 
							(NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					// Tessellate this segment
					InterimDrawPosition = LastDrawPosition;
					for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						Up			= Right ^  (Location - CameraToWorld.GetOrigin());
						Up.Normalize();

						if (SheetIndex)
						{
							Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
						}

						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.Y * Taper;
						Offset.Z	= WorkingUp.Z * Size.Z * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
				}
			}
		}
		else
		{
			// Setup the current position as the source point
			CurrPosition		= BeamPayloadData->SourcePoint;
			CurrDrawPosition	= CurrPosition;

			// Setup the source tangent & strength
			if (bUseSource)
			{
				// The source module will have determined the proper source tangent.
				LastTangent	= BeamPayloadData->SourceTangent;
				fStrength	= BeamPayloadData->SourceStrength;
			}
			else
			{
				// We don't have a source module, so use the orientation of the emitter.
				LastTangent	= WorldToLocal.GetAxis(0);
				fStrength	= NoiseTangentStrength;
			}
			LastTangent.Normalize();
			LastTangent *= fStrength;

			// Setup the target tangent strength
			fTargetStrength	= NoiseTangentStrength;

			// Set the last draw position to the source so we don't get 'under-hang'
			LastPosition		= CurrPosition;
			LastDrawPosition	= CurrDrawPosition;

			UBOOL	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

			FVector	UseNoisePoint, CheckNoisePoint;
			FVector	NoiseDir;

			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				// Reset the texture coordinate
				fU					= 0.0f;
				LastPosition		= BeamPayloadData->SourcePoint;
				LastDrawPosition	= LastPosition;

				// Determine the current position by stepping the direct line and offsetting with the noise point. 
				CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

				if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[0] - NoisePoints[0];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[0] + NoiseDir * NoiseSpeed * *NoiseRate;
					if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[0].X) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[0].Y) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[0].Z) < NoiseLockRadius))
					{
						NoisePoints[0]	= NextNoise[0];
					}
					else
					{
						NoisePoints[0]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[0] * NoiseDistScale);

				// Determine the offset for the leading edge
				Location	= LastDrawPosition;
				EndPoint	= CurrDrawPosition;
				Right		= Location - EndPoint;
				Right.Normalize();
				LastUp		= Right ^  (Location - CameraToWorld.GetOrigin());
				LastUp.Normalize();

				if (SheetIndex)
				{
					Angle			= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
					QuatRotator		= FQuat(Right, Angle);
					WorkingLastUp	= QuatRotator.RotateVector(LastUp);
				}
				else
				{
					WorkingLastUp	= LastUp;
				}

				FLOAT	Taper	= 1.0f;

				if (TaperMethod != PEBTM_None)
				{
					check(TaperValues);
					Taper	= TaperValues[0];
				}

				LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
				LastOffset.Y	= WorkingLastUp.Y * Size.Y * Taper;
				LastOffset.Z	= WorkingLastUp.Z * Size.Z * Taper;

				// 'Lead' edge
				Vertex->Position	= Location + LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 0.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				VertexCount++;

				Vertex->Position	= Location - LastOffset;
				Vertex->OldPosition	= Location;
				Vertex->Size		= Size;
				Vertex->Tex_U		= fU;
				Vertex->Tex_V		= 1.0f;
				Vertex->Rotation	= Particle->Rotation;
				Vertex->Color		= Particle->Color;
				Vertex++;
				VertexCount++;

				fU	+= TextureIncrement;

				for (INT StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
				{
					// Determine the current position by stepping the direct line and offsetting with the noise point. 
					CurrPosition		= LastPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < NoiseLockRadius))
						{
							NoisePoints[StepIndex]	= NextNoise[StepIndex];
						}
						else
						{
							NoisePoints[StepIndex]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex] * NoiseDistScale);

					// Prep the next draw position to determine tangents
					UBOOL bTarget = FALSE;
					NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
					if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
					{
						// If we are locked, and the next step is the target point, set the draw position as such.
						// (ie, we are on the last noise point...)
						NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
						if (bTargetNoise)
						{
							if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
							{
								NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
								NoiseDir.Normalize();
								CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
								if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
								{
									NoisePoints[Frequency]	= NextNoise[Frequency];
								}
								else
								{
									NoisePoints[Frequency]	= CheckNoisePoint;
								}
							}

							NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
						}
						TargetTangent = BeamPayloadData->TargetTangent;
						fTargetStrength	= BeamPayloadData->TargetStrength;
					}
					else
					{
						// Just another noise point... offset the target to get the draw position.
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < NoiseLockRadius))
							{
								NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
							}
							else
							{
								NoisePoints[StepIndex + 1]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex + 1] * NoiseDistScale);

						TargetTangent = ((1.0f - NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					InterimDrawPosition = LastDrawPosition;
					// Tessellate between the current position and the last position
					for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						Up			= Right ^  (Location - CameraToWorld.GetOrigin());
						Up.Normalize();

						if (SheetIndex)
						{
							Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
						}

						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.Y * Taper;
						Offset.Z	= WorkingUp.Z * Size.Z * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
					LastPosition		= CurrPosition;
					LastDrawPosition	= CurrDrawPosition;
					LastTangent			= TargetTangent;
				}

				if (bLocked)
				{
					// Draw the line from the last point to the target
					CurrDrawPosition	= BeamPayloadData->TargetPoint;
					if (bTargetNoise)
					{
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
							{
								NoisePoints[Frequency]	= NextNoise[Frequency];
							}
							else
							{
								NoisePoints[Frequency]	= CheckNoisePoint;
							}
						}

						CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
					}

					if (bUseTarget)
					{
						TargetTangent = BeamPayloadData->TargetTangent;
					}
					else
					{
						NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
						TargetTangent = ((1.0f - NoiseTension) / 2.0f) * 
							(NextTargetDrawPosition - LastDrawPosition);
					}
					TargetTangent.Normalize();
					TargetTangent *= fTargetStrength;

					// Tessellate this segment
					InterimDrawPosition = LastDrawPosition;
					for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
					{
						InterpDrawPos = CubicInterp(
							LastDrawPosition, LastTangent,
							CurrDrawPosition, TargetTangent,
							InvTessFactor * (TessIndex + 1));

						Location	= InterimDrawPosition;
						EndPoint	= InterpDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();
						Up			= Right ^  (Location - CameraToWorld.GetOrigin());
						Up.Normalize();

						if (SheetIndex)
						{
							Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
							QuatRotator	= FQuat(Right, Angle);
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (TaperMethod != PEBTM_None)
						{
							check(TaperValues);
							Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
						}

						Offset.X	= WorkingUp.X * Size.X * Taper;
						Offset.Y	= WorkingUp.Y * Size.Y * Taper;
						Offset.Z	= WorkingUp.Z * Size.Z * Taper;

						// Generate the vertex
						Vertex->Position	= InterpDrawPos + Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						Vertex->Position	= InterpDrawPos - Offset;
						Vertex->OldPosition	= InterpDrawPos;
						Vertex->Size		= Size;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= Particle->Color;
						Vertex++;
						VertexCount++;

						fU	+= TextureIncrement;
						InterimDrawPosition	= InterpDrawPos;
					}
				}
				else
				if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
				{
//@todo.SAS. Re-implement partial-segment beams
/***
					if (BeamPayloadData->TravelRatio <= 1.0f)
					{
						NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
						if (BeamPayloadData->Steps == Frequency)
						{
							NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
							if (bTargetNoise)
							{
								if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
								{
									NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
									NoiseDir.Normalize();
									CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseDelta;
									if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
										(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
										(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
									{
										NoisePoints[Frequency]	= NextNoise[Frequency];
									}
									else
									{
										NoisePoints[Frequency]	= CheckNoisePoint;
									}
								}

								NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency]);
							}
							TargetTangent = BeamPayloadData->TargetTangent;
						}
						else
						{
							NextTargetDrawPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;

							INT	Steps = BeamPayloadData->Steps;

							if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
							{
								NoiseDir		= NextNoise[Steps] - NoisePoints[Steps];
								NoiseDir.Normalize();
								CheckNoisePoint	= NoisePoints[Steps] + NoiseDir * NoiseSpeed * *NoiseDelta;
								if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Steps].X) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Steps].Y) < NoiseLockRadius) &&
									(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Steps].Z) < NoiseLockRadius))
								{
									NoisePoints[Steps]	= NextNoise[Steps];
								}
								else
								{
									NoisePoints[Steps]	= CheckNoisePoint;
								}
							}

							NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Steps]);
						}

						INT	Count	= appFloor(BeamPayloadData->TravelRatio * TessFactor);
						// Tessellate this segment
						InterimDrawPosition = LastDrawPosition;
						for (INT TessIndex = 0; TessIndex < Count; TessIndex++)
						{
							InterpDrawPos = CubicInterp(
								CurrDrawPosition, LastTangent,
								NextTargetDrawPosition, TargetTangent,
								InvTessFactor * (TessIndex + 1));

							Location	= InterimDrawPosition;
							EndPoint	= InterpDrawPos;
							Right		= Location - EndPoint;
							Right.Normalize();
							Up			= Right ^  (Location - CameraToWorld.GetOrigin());
							Up.Normalize();

							if (SheetIndex)
							{
								Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
								QuatRotator	= FQuat(Right, Angle);
								WorkingUp	= QuatRotator.RotateVector(Up);
							}
							else
							{
								WorkingUp	= Up;
							}

							if (TaperMethod != PEBTM_None)
							{
								Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
							}

							Offset.X	= WorkingUp.X * Size.X * Taper;
							Offset.Y	= WorkingUp.Y * Size.Y * Taper;
							Offset.Z	= WorkingUp.Z * Size.Z * Taper;

							// Generate the vertex
							Vertex->Position	= InterpDrawPos + Offset;
							Vertex->OldPosition	= InterpDrawPos;
							Vertex->Size		= Size;
							Vertex->Tex_U		= fU;
							Vertex->Tex_V		= 0.0f;
							Vertex->Rotation	= Particle->Rotation;
							Vertex->Color		= Particle->Color;
							Vertex++;
							VertexCount++;

							Vertex->Position	= InterpDrawPos - Offset;
							Vertex->OldPosition	= InterpDrawPos;
							Vertex->Size		= Size;
							Vertex->Tex_U		= fU;
							Vertex->Tex_V		= 1.0f;
							Vertex->Rotation	= Particle->Rotation;
							Vertex->Color		= Particle->Color;
							Vertex++;
							VertexCount++;

							fU	+= TextureIncrement;
							InterimDrawPosition	= InterpDrawPos;
						}
					}
***/
				}
			}
		}
	}

	check(VertexCount <= VertexCount);

	return TrianglesToRender;
}

INT FDynamicBeam2EmitterData::FillData_InterpolatedNoise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	INT	TrianglesToRender = 0;

	check(InterpolationPoints > 0);
	check(Frequency > 0);

	FParticleSpriteVertex*			Vertex			= (FParticleSpriteVertex*)VertexData;
	FMatrix							CameraToWorld	= View->ViewMatrix.Inverse();
	
	if (Sheets <= 0)
	{
		Sheets = 1;
	}

	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	// Frequency is the number of noise points to generate, evenly distributed along the line.
	if (Frequency <= 0)
	{
		Frequency = 1;
	}

	// NoiseTessellation is the amount of tessellation that should occur between noise points.
	INT	TessFactor	= NoiseTessellation ? NoiseTessellation : 1;
	
	FLOAT	InvTessFactor	= 1.0f / TessFactor;
	FColor	Color(255,0,0);
	INT		i;

	// The last position processed
	FVector	LastPosition, LastDrawPosition, LastTangent;
	// The current position
	FVector	CurrPosition, CurrDrawPosition;
	// The target
	FVector	TargetPosition, TargetDrawPosition;
	// The next target
	FVector	NextTargetPosition, NextTargetDrawPosition, TargetTangent;
	// The interperted draw position
	FVector InterpDrawPos;
	FVector	InterimDrawPosition;

	FVector	Size;

	FLOAT	Angle;
	FQuat	QuatRotator;

	FVector Location;
	FVector EndPoint;
	FVector Right;
	FVector Up;
	FVector WorkingUp;
	FVector LastUp;
	FVector WorkingLastUp;
	FVector Offset;
	FVector LastOffset;
	FLOAT	fStrength;
	FLOAT	fTargetStrength;

	FLOAT	fU;
	FLOAT	TextureIncrement	= 1.0f / (((Frequency > 0) ? Frequency : 1) * TessFactor);	// TTP #33140/33159

	INT	 VertexCount	= 0;

	FMatrix WorldToLocal = Proxy->GetWorldToLocal();
	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	// Tessellate the beam along the noise points
	for (i = 0; i < ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * i);

		// Retrieve the beam data from the particle.
		FBeam2TypeDataPayload*	BeamPayloadData		= NULL;
		FVector*				InterpolatedPoints	= NULL;
		FLOAT*					NoiseRate			= NULL;
		FLOAT*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		FLOAT*					TaperValues			= NULL;
		FLOAT*					NoiseDistanceScale	= NULL;

		BeamPayloadData = (FBeam2TypeDataPayload*)((BYTE*)Particle + BeamDataOffset);
		if (BeamPayloadData->TriangleCount == 0)
		{
			continue;
		}
		if (InterpolatedPointsOffset != -1)
		{
			InterpolatedPoints = (FVector*)((BYTE*)Particle + InterpolatedPointsOffset);
		}
		if (NoiseRateOffset != -1)
		{
			NoiseRate = (FLOAT*)((BYTE*)Particle + NoiseRateOffset);
		}
		if (NoiseDeltaTimeOffset != -1)
		{
			NoiseDelta = (FLOAT*)((BYTE*)Particle + NoiseDeltaTimeOffset);
		}
		if (TargetNoisePointsOffset != -1)
		{
			TargetNoisePoints = (FVector*)((BYTE*)Particle + TargetNoisePointsOffset);
		}
		if (NextNoisePointsOffset != -1)
		{
			NextNoisePoints = (FVector*)((BYTE*)Particle + NextNoisePointsOffset);
		}
		if (TaperValuesOffset != -1)
		{
			TaperValues = (FLOAT*)((BYTE*)Particle + TaperValuesOffset);
		}
		if (NoiseDistanceScaleOffset != -1)
		{
			NoiseDistanceScale = (FLOAT*)((BYTE*)Particle + NoiseDistanceScaleOffset);
		}

		FLOAT NoiseDistScale = 1.0f;
		if (NoiseDistanceScale)
		{
			NoiseDistScale = *NoiseDistanceScale;
		}

		INT Freq = BEAM2_TYPEDATA_FREQUENCY(BeamPayloadData->Lock_Max_NumNoisePoints);
		FLOAT InterpStepSize = (FLOAT)(BeamPayloadData->InterpolationSteps) / (FLOAT)(BeamPayloadData->Steps);
		FLOAT InterpFraction = appFractional(InterpStepSize);
		UBOOL bInterpFractionIsZero = (Abs(InterpFraction) < KINDA_SMALL_NUMBER) ? TRUE : FALSE;
		INT InterpIndex = appTrunc(InterpStepSize);

		FVector* NoisePoints	= TargetNoisePoints;
		FVector* NextNoise		= NextNoisePoints;

		FLOAT NoiseRangeScaleFactor = NoiseRangeScale;
		//@todo. How to handle no noise points?
		// If there are no noise points, why are we in here?
		if (NoisePoints == NULL)
		{
			continue;
		}

		// Pin the size to the X component
		Size	= FVector(Particle->Size.X * Scale.X);

		// Setup the current position as the source point
		CurrPosition		= BeamPayloadData->SourcePoint;
		CurrDrawPosition	= CurrPosition;

		// Setup the source tangent & strength
		if (bUseSource)
		{
			// The source module will have determined the proper source tangent.
			LastTangent	= BeamPayloadData->SourceTangent;
			fStrength	= NoiseTangentStrength;
		}
		else
		{
			// We don't have a source module, so use the orientation of the emitter.
			LastTangent	= WorldToLocal.GetAxis(0);
			fStrength	= NoiseTangentStrength;
		}
		LastTangent *= fStrength;

		// Setup the target tangent strength
		fTargetStrength	= NoiseTangentStrength;

		// Set the last draw position to the source so we don't get 'under-hang'
		LastPosition		= CurrPosition;
		LastDrawPosition	= CurrDrawPosition;

		UBOOL	bLocked	= BEAM2_TYPEDATA_LOCKED(BeamPayloadData->Lock_Max_NumNoisePoints);

		FVector	UseNoisePoint, CheckNoisePoint;
		FVector	NoiseDir;

		for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
		{
			// Reset the texture coordinate
			fU					= 0.0f;
			LastPosition		= BeamPayloadData->SourcePoint;
			LastDrawPosition	= LastPosition;

			// Determine the current position by finding it along the interpolated path and 
			// offsetting with the noise point. 
			if (bInterpFractionIsZero)
			{
				CurrPosition = InterpolatedPoints[InterpIndex];
			}
			else
			{
				CurrPosition = 
					(InterpolatedPoints[InterpIndex + 0] * InterpFraction) + 
					(InterpolatedPoints[InterpIndex + 1] * (1.0f - InterpFraction));
			}

			if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
			{
				NoiseDir		= NextNoise[0] - NoisePoints[0];
				NoiseDir.Normalize();
				CheckNoisePoint	= NoisePoints[0] + NoiseDir * NoiseSpeed * *NoiseRate;
				if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[0].X) < NoiseLockRadius) &&
					(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[0].Y) < NoiseLockRadius) &&
					(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[0].Z) < NoiseLockRadius))
				{
					NoisePoints[0]	= NextNoise[0];
				}
				else
				{
					NoisePoints[0]	= CheckNoisePoint;
				}
			}

			CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[0] * NoiseDistScale);

			// Determine the offset for the leading edge
			Location	= LastDrawPosition;
			EndPoint	= CurrDrawPosition;
			Right		= Location - EndPoint;
			Right.Normalize();
			LastUp		= Right ^  (Location - CameraToWorld.GetOrigin());
			LastUp.Normalize();

			if (SheetIndex)
			{
				Angle			= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
				QuatRotator		= FQuat(Right, Angle);
				WorkingLastUp	= QuatRotator.RotateVector(LastUp);
			}
			else
			{
				WorkingLastUp	= LastUp;
			}

			FLOAT	Taper	= 1.0f;

			if (TaperMethod != PEBTM_None)
			{
				check(TaperValues);
				Taper	= TaperValues[0];
			}

			LastOffset.X	= WorkingLastUp.X * Size.X * Taper;
			LastOffset.Y	= WorkingLastUp.Y * Size.Y * Taper;
			LastOffset.Z	= WorkingLastUp.Z * Size.Z * Taper;

			// 'Lead' edge
			Vertex->Position	= Location + LastOffset;
			Vertex->OldPosition	= Location;
			Vertex->Size		= Size;
			Vertex->Tex_U		= fU;
			Vertex->Tex_V		= 0.0f;
			Vertex->Rotation	= Particle->Rotation;
			Vertex->Color		= Particle->Color;
			Vertex++;
			VertexCount++;

			Vertex->Position	= Location - LastOffset;
			Vertex->OldPosition	= Location;
			Vertex->Size		= Size;
			Vertex->Tex_U		= fU;
			Vertex->Tex_V		= 1.0f;
			Vertex->Rotation	= Particle->Rotation;
			Vertex->Color		= Particle->Color;
			Vertex++;
			VertexCount++;

			fU	+= TextureIncrement;

			check(InterpolatedPoints);
			for (INT StepIndex = 0; StepIndex < BeamPayloadData->Steps; StepIndex++)
			{
				// Determine the current position by finding it along the interpolated path and 
				// offsetting with the noise point. 
				if (bInterpFractionIsZero)
				{
					CurrPosition = InterpolatedPoints[StepIndex  * InterpIndex];
				}
				else
				{
					if (StepIndex == (BeamPayloadData->Steps - 1))
					{
						CurrPosition = 
							(InterpolatedPoints[StepIndex * InterpIndex] * (1.0f - InterpFraction)) + 
							(BeamPayloadData->TargetPoint * InterpFraction);
					}
					else
					{
						CurrPosition = 
							(InterpolatedPoints[StepIndex * InterpIndex + 0] * (1.0f - InterpFraction)) + 
							(InterpolatedPoints[StepIndex * InterpIndex + 1] * InterpFraction);
					}
				}


				if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
				{
					NoiseDir		= NextNoise[StepIndex] - NoisePoints[StepIndex];
					NoiseDir.Normalize();
					CheckNoisePoint	= NoisePoints[StepIndex] + NoiseDir * NoiseSpeed * *NoiseRate;
					if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex].X) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex].Y) < NoiseLockRadius) &&
						(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex].Z) < NoiseLockRadius))
					{
						NoisePoints[StepIndex]	= NextNoise[StepIndex];
					}
					else
					{
						NoisePoints[StepIndex]	= CheckNoisePoint;
					}
				}

				CurrDrawPosition	= CurrPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex] * NoiseDistScale);

				// Prep the next draw position to determine tangents
				UBOOL bTarget = FALSE;
				NextTargetPosition	= CurrPosition + BeamPayloadData->Direction * BeamPayloadData->StepSize;
				// Determine the current position by finding it along the interpolated path and 
				// offsetting with the noise point. 
				if (bInterpFractionIsZero)
				{
					if (StepIndex == (BeamPayloadData->Steps - 2))
					{
						NextTargetPosition = BeamPayloadData->TargetPoint;
					}
					else
					{
						NextTargetPosition = InterpolatedPoints[(StepIndex + 2) * InterpIndex + 0];
					}
				}
				else
				{
					if (StepIndex == (BeamPayloadData->Steps - 1))
					{
						NextTargetPosition = 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 0] * InterpFraction) + 
							(BeamPayloadData->TargetPoint * (1.0f - InterpFraction));
					}
					else
					{
						NextTargetPosition = 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 0] * InterpFraction) + 
							(InterpolatedPoints[(StepIndex + 1) * InterpIndex + 1] * (1.0f - InterpFraction));
					}
				}
				if (bLocked && ((StepIndex + 1) == BeamPayloadData->Steps))
				{
					// If we are locked, and the next step is the target point, set the draw position as such.
					// (ie, we are on the last noise point...)
					NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
					if (bTargetNoise)
					{
						if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
						{
							NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
							NoiseDir.Normalize();
							CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
							if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
								(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
							{
								NoisePoints[Frequency]	= NextNoise[Frequency];
							}
							else
							{
								NoisePoints[Frequency]	= CheckNoisePoint;
							}
						}

						NextTargetDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
					}
					TargetTangent = BeamPayloadData->TargetTangent;
					fTargetStrength	= NoiseTangentStrength;
				}
				else
				{
					// Just another noise point... offset the target to get the draw position.
					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[StepIndex + 1] - NoisePoints[StepIndex + 1];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[StepIndex + 1] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[StepIndex + 1].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[StepIndex + 1].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[StepIndex + 1].Z) < NoiseLockRadius))
						{
							NoisePoints[StepIndex + 1]	= NextNoise[StepIndex + 1];
						}
						else
						{
							NoisePoints[StepIndex + 1]	= CheckNoisePoint;
						}
					}

					NextTargetDrawPosition	= NextTargetPosition + NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[StepIndex + 1] * NoiseDistScale);

					TargetTangent = ((1.0f - NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				}
				TargetTangent = ((1.0f - NoiseTension) / 2.0f) * (NextTargetDrawPosition - LastDrawPosition);
				TargetTangent.Normalize();
				TargetTangent *= fTargetStrength;

				InterimDrawPosition = LastDrawPosition;
				// Tessellate between the current position and the last position
				for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;
					Right		= Location - EndPoint;
					Right.Normalize();
					Up			= Right ^  (Location - CameraToWorld.GetOrigin());
					Up.Normalize();

					if (SheetIndex)
					{
						Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
						QuatRotator	= FQuat(Right, Angle);
						WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[StepIndex * TessFactor + TessIndex];
					}

					Offset.X	= WorkingUp.X * Size.X * Taper;
					Offset.Y	= WorkingUp.Y * Size.Y * Taper;
					Offset.Z	= WorkingUp.Z * Size.Z * Taper;

					// Generate the vertex
					Vertex->Position	= InterpDrawPos + Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					VertexCount++;

					Vertex->Position	= InterpDrawPos - Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					VertexCount++;

					fU	+= TextureIncrement;
					InterimDrawPosition	= InterpDrawPos;
				}
				LastPosition		= CurrPosition;
				LastDrawPosition	= CurrDrawPosition;
				LastTangent			= TargetTangent;
			}

			if (bLocked)
			{
				// Draw the line from the last point to the target
				CurrDrawPosition	= BeamPayloadData->TargetPoint;
				if (bTargetNoise)
				{
					if ((NoiseLockTime >= 0.0f) && bSmoothNoise_Enabled)
					{
						NoiseDir		= NextNoise[Frequency] - NoisePoints[Frequency];
						NoiseDir.Normalize();
						CheckNoisePoint	= NoisePoints[Frequency] + NoiseDir * NoiseSpeed * *NoiseRate;
						if ((Abs<FLOAT>(CheckNoisePoint.X - NextNoise[Frequency].X) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Y - NextNoise[Frequency].Y) < NoiseLockRadius) &&
							(Abs<FLOAT>(CheckNoisePoint.Z - NextNoise[Frequency].Z) < NoiseLockRadius))
						{
							NoisePoints[Frequency]	= NextNoise[Frequency];
						}
						else
						{
							NoisePoints[Frequency]	= CheckNoisePoint;
						}
					}

					CurrDrawPosition += NoiseRangeScaleFactor * LocalToWorld.TransformNormal(NoisePoints[Frequency] * NoiseDistScale);
				}

				NextTargetDrawPosition	= BeamPayloadData->TargetPoint;
				if (bUseTarget)
				{
					TargetTangent = BeamPayloadData->TargetTangent;
				}
				else
				{
					TargetTangent = ((1.0f - NoiseTension) / 2.0f) * 
						(NextTargetDrawPosition - LastDrawPosition);
					TargetTangent.Normalize();
				}
				TargetTangent *= fTargetStrength;

				// Tessellate this segment
				InterimDrawPosition = LastDrawPosition;
				for (INT TessIndex = 0; TessIndex < TessFactor; TessIndex++)
				{
					InterpDrawPos = CubicInterp(
						LastDrawPosition, LastTangent,
						CurrDrawPosition, TargetTangent,
						InvTessFactor * (TessIndex + 1));

					Location	= InterimDrawPosition;
					EndPoint	= InterpDrawPos;
					Right		= Location - EndPoint;
					Right.Normalize();
					Up			= Right ^  (Location - CameraToWorld.GetOrigin());
					Up.Normalize();

					if (SheetIndex)
					{
						Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
						QuatRotator	= FQuat(Right, Angle);
						WorkingUp	= QuatRotator.RotateVector(Up);
					}
					else
					{
						WorkingUp	= Up;
					}

					if (TaperMethod != PEBTM_None)
					{
						check(TaperValues);
						Taper	= TaperValues[BeamPayloadData->Steps * TessFactor + TessIndex];
					}

					Offset.X	= WorkingUp.X * Size.X * Taper;
					Offset.Y	= WorkingUp.Y * Size.Y * Taper;
					Offset.Z	= WorkingUp.Z * Size.Z * Taper;

					// Generate the vertex
					Vertex->Position	= InterpDrawPos + Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					VertexCount++;

					Vertex->Position	= InterpDrawPos - Offset;
					Vertex->OldPosition	= InterpDrawPos;
					Vertex->Size		= Size;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= Particle->Color;
					Vertex++;
					VertexCount++;

					fU	+= TextureIncrement;
					InterimDrawPosition	= InterpDrawPos;
				}
			}
			else
			if (BeamPayloadData->TravelRatio > KINDA_SMALL_NUMBER)
			{
				//@todo.SAS. Re-implement partial-segment beams
			}
		}
	}

	check(VertexCount <= VertexCount);

	return TrianglesToRender;
}

///////////////////////////////////////////////////////////////////////////////
//	FDynamicTrail2EmitterData
///////////////////////////////////////////////////////////////////////////////
void FDynamicTrail2EmitterData::Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex)
{
	check(PDI);
	if ((VertexCount <= 0) || (ActiveParticleCount <= 0) || (IndexCount < 3))
	{
		return;
	}

	UBOOL bWireframe = ((View->Family->ShowFlags & SHOW_Wireframe) && !(View->Family->ShowFlags & SHOW_Materials));
	if (bWireframe == TRUE)
	{
		return;
	}

	VertexFactory->SetScreenAlignment(ScreenAlignment);
	VertexFactory->SetLockAxesFlag(LockAxisFlag);
	if (LockAxisFlag != EPAL_NONE)
	{
		FVector Up, Right;
		Proxy->GetAxisLockValues((FDynamicSpriteEmitterDataBase*)this, Up, Right);
		VertexFactory->SetLockAxes(Up, Right);
	}

	// Allocate and generate the data...
	// Determine the required particle count
	INT	TrianglesToRender	= 0;

	if (VertexData == NULL)
	{
		VertexData = (FParticleSpriteVertex*)appRealloc(
			VertexData, VertexCount * sizeof(FParticleSpriteVertex));
	}
	check(VertexData);

	INT TriCountIndex = FillIndexData(Proxy, PDI, View, DPGIndex);
	INT TriCountVertex = FillVertexData(Proxy, PDI, View, DPGIndex);

	FMeshElement Mesh;

	Mesh.IndexBuffer			= NULL;
	Mesh.VertexFactory			= VertexFactory;
	Mesh.DynamicVertexData		= VertexData;
	Mesh.DynamicVertexStride	= sizeof(FParticleSpriteVertex);
	Mesh.DynamicIndexData		= IndexData;
	Mesh.DynamicIndexStride		= IndexStride;
	Mesh.LCI					= NULL;
	if (bUseLocalSpace == TRUE)
	{
		Mesh.LocalToWorld = Proxy->GetLocalToWorld();
		Mesh.WorldToLocal = Proxy->GetWorldToLocal();
	}
	else
	{
		Mesh.LocalToWorld = FMatrix::Identity;
		Mesh.WorldToLocal = FMatrix::Identity;
	}
	Mesh.FirstIndex				= 0;
	Mesh.NumPrimitives			= TriCountIndex;
	Mesh.MinVertexIndex			= 0;
	Mesh.MaxVertexIndex			= VertexCount - 1;
	Mesh.UseDynamicData			= TRUE;
	Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
	Mesh.CastShadow				= Proxy->GetCastShadow();
	Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;

	if (bWireframe)
	{
		Mesh.MaterialRenderProxy	= Proxy->GetDeselectedWireframeMatInst();
		//Mesh.Type				= PT_LineList;
		Mesh.Type				= PT_TriangleStrip;
	}
	else
	{
		check(TriCountIndex == PrimitiveCount);

		Mesh.MaterialRenderProxy	= MaterialResource;
		Mesh.Type				= PT_TriangleStrip;
	}

	//	PDI->DrawMesh(Mesh);
	DrawRichMesh(
		PDI,
		Mesh,
		FLinearColor(1.0f, 0.0f, 0.0f),
		FLinearColor(1.0f, 1.0f, 0.0f),
		FLinearColor(1.0f, 1.0f, 1.0f),
		Proxy->GetPrimitiveSceneInfo(),
		Proxy->GetSelected()
		);
}

void FDynamicTrail2EmitterData::RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses)
{
}

INT FDynamicTrail2EmitterData::FillIndexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	INT	TrianglesToRender = 0;

	// Trail2 polygons are packed and joined as follows:
	//
	// 1--3--5--7--9-...
	// |\ |\ |\ |\ |\...
	// | \| \| \| \| ...
	// 0--2--4--6--8-...
	//
	// (ie, the 'leading' edge of polygon (n) is the trailing edge of polygon (n+1)
	//
	// NOTE: This is primed for moving to tri-strips...
	//

	INT	Sheets		= 1;
	if (TessFactor <= 0)
	{
		TessFactor = 1;
	}

	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	UBOOL bWireframe = ((View->Family->ShowFlags & SHOW_Wireframe) && !(View->Family->ShowFlags & SHOW_Materials));
	if (IndexData == NULL)
	{
/***
		if (bWireframe)
		{
			TrailData->IndexCount = 0;
			for (INT Trail = 0; Trail < TrailData->ActiveParticleCount; Trail++)
			{
				DECLARE_PARTICLE_PTR(Particle, TrailData->ParticleData + TrailData->ParticleStride * TrailData->ParticleIndices[Trail]);

				INT	CurrentOffset = TrailData->TrailDataOffset;

				FTrail2TypeDataPayload* TrailPayload = (FTrail2TypeDataPayload*)((BYTE*)Particle + CurrentOffset);
				CurrentOffset += sizeof(FTrail2TypeDataPayload);

				FLOAT* TaperValues = (FLOAT*)((BYTE*)Particle + CurrentOffset);
				CurrentOffset += sizeof(FLOAT);

				if (TRAIL_EMITTER_IS_START(TrailPayload->Flags) == FALSE)
				{
					continue;
				}

				INT Triangles = TrailPayload->TriangleCount;
				if (Triangles > 0)
				{
					TrailData->IndexCount += (4 * Triangles + 2) * Sheets;
				}
			}
		}
***/
		if ((UINT)IndexCount > 65535)
		{
			FString TemplateName = TEXT("*** UNKNOWN PSYS ***");
			UParticleSystemComponent* PSysComp = Cast<UParticleSystemComponent>(Proxy->GetPrimitiveSceneInfo()->Component);
			if (PSysComp)
			{
				if (PSysComp->Template)
				{
					TemplateName = PSysComp->Template->GetName();
				}
			}

			FString ErrorOut = FString::Printf(
				TEXT("*** PLEASE SUBMIT IMMEDIATELY ***%s")
				TEXT("Trail Index Error			- %s%s")
				TEXT("\tPosition				- %s%s")
				TEXT("\tPrimitiveCount			- %d%s")
				TEXT("\tVertexCount				- %d%s")
				TEXT("\tVertexData				- 0x%08x%s"),
				LINE_TERMINATOR,
				*TemplateName, LINE_TERMINATOR,
				*LocalToWorld.GetOrigin().ToString(), LINE_TERMINATOR,
				PrimitiveCount, LINE_TERMINATOR,
				VertexCount, LINE_TERMINATOR,
				VertexData, LINE_TERMINATOR
				);
			ErrorOut += FString::Printf(
				TEXT("\tIndexCount				- %d%s")
				TEXT("\tIndexStride				- %d%s")
				TEXT("\tIndexData				- 0x%08x%s")
				TEXT("\tVertexFactory			- 0x%08x%s"),
				IndexCount, LINE_TERMINATOR,
				IndexStride, LINE_TERMINATOR,
				IndexData, LINE_TERMINATOR,
				VertexFactory, LINE_TERMINATOR
				);
			ErrorOut += FString::Printf(
				TEXT("\tTrailDataOffset			- %d%s")
				TEXT("\tTaperValuesOffset		- %d%s")
				TEXT("\tParticleSourceOffset	- %d%s")
				TEXT("\tTrailCount				- %d%s"),
				TrailDataOffset, LINE_TERMINATOR,
				TaperValuesOffset, LINE_TERMINATOR,
				ParticleSourceOffset, LINE_TERMINATOR,
				TrailCount, LINE_TERMINATOR
				);
			ErrorOut += FString::Printf(
				TEXT("\tSheets					- %d%s")
				TEXT("\tTessFactor				- %d%s")
				TEXT("\tTessStrength			- %d%s")
				TEXT("\tTessFactorDistance		- %f%s")
				TEXT("\tActiveParticleCount		- %d%s"),
				Sheets, LINE_TERMINATOR,
				TessFactor, LINE_TERMINATOR,
				TessStrength, LINE_TERMINATOR,
				TessFactorDistance, LINE_TERMINATOR,
				ActiveParticleCount, LINE_TERMINATOR
				);

			appErrorf(*ErrorOut);
		}
		IndexData = appRealloc(IndexData, IndexCount * IndexStride);
		check(IndexData);
	}

	INT	CheckCount	= 0;

	WORD*	Index		= (WORD*)IndexData;
	WORD	VertexIndex	= 0;

	for (INT Trail = 0; Trail < ActiveParticleCount; Trail++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[Trail]);

		INT	CurrentOffset = TrailDataOffset;

		FTrail2TypeDataPayload* TrailPayload = (FTrail2TypeDataPayload*)((BYTE*)Particle + CurrentOffset);
		CurrentOffset += sizeof(FTrail2TypeDataPayload);

		FLOAT* TaperValues = (FLOAT*)((BYTE*)Particle + CurrentOffset);
		CurrentOffset += sizeof(FLOAT);

		if (TRAIL_EMITTER_IS_START(TrailPayload->Flags) == FALSE)
		{
			continue;
		}

		if (TrailPayload->TriangleCount == 0)
		{
			continue;
		}
/***
		if (bWireframe)
		{
			INT TriCount = TrailPayload->TriangleCount;
			if (TriCount > 0)
			{
				WORD StartVertexIndex	= 0;
				for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
				{
					VertexIndex = 0;

					// The 'starting' line
					TrianglesToRender += 1;
					*(Index++) = StartVertexIndex + 0;
					*(Index++) = StartVertexIndex + 1;

					// 4 lines per quad
					INT QuadCount = TriCount / 2;
					TrianglesToRender += TriCount * 2;

					for (INT i = 0; i < QuadCount; i++)
					{
						*(Index++) = StartVertexIndex + VertexIndex + 0;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 1;
						*(Index++) = StartVertexIndex + VertexIndex + 3;
						*(Index++) = StartVertexIndex + VertexIndex + 2;
						*(Index++) = StartVertexIndex + VertexIndex + 3;

						VertexIndex += 2;
					}

					StartVertexIndex += TriCount + 2;
				}
			}
		}
		else
***/
		{
			INT LocalTrianglesToRender = TrailPayload->TriangleCount;

			if (LocalTrianglesToRender > 0)
			{
				for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
				{
					// 2 triangles per tessellation factor
					if (SheetIndex == 0)
					{
						// Only need the starting two for the first sheet
						*(Index++) = VertexIndex++;	// SheetIndex + 0
						*(Index++) = VertexIndex++;	// SheetIndex + 1

						CheckCount += 2;
					}

					// Sequentially step through each triangle - 1 vertex per triangle
					for (INT i = 0; i < LocalTrianglesToRender; i++)
					{
						*(Index++) = VertexIndex++;
						CheckCount++;
						TrianglesToRender++;
					}

					// Degenerate tris
					if ((SheetIndex + 1) < Sheets)
					{
						*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
						*(Index++) = VertexIndex;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// First vertex of the next sheet
						*(Index++) = VertexIndex++;		// Second vertex of the next sheet
						TrianglesToRender += 4;
						CheckCount += 4;
					}
				}
			}
		}

		if ((Trail + 1) < TrailCount)
		{
			*(Index++) = VertexIndex - 1;	// Last vertex of the previous sheet
			*(Index++) = VertexIndex;		// First vertex of the next sheet
			*(Index++) = VertexIndex++;		// First vertex of the next sheet
			*(Index++) = VertexIndex++;		// Second vertex of the next sheet
			TrianglesToRender += 4;
			CheckCount += 4;
		}
	}

	//#if defined(_DEBUG_TRAIL_DATA_)
#if 0
	if (CheckCount > PrimitiveCount + 2)
	{
		FString DebugOut = FString::Printf(TEXT("Trail2 index buffer - CheckCount = %4d --> TriCnt + 2 = %4d"), CheckCount, PrimitiveCount + 2);
		OutputDebugString(*DebugOut);
		OutputDebugString(TEXT("\n"));
		DebugOut = FString::Printf(TEXT("                      ActiveParticles	= %4d, Sheets = %2d, TessFactor = %2d"), ParticleCount, Sheets, TessFactor);
		OutputDebugString(*DebugOut);
		OutputDebugString(TEXT("\n"));
		//		check(0);
		INT dummy = 0;
	}
#endif

	return TrianglesToRender;
}

INT FDynamicTrail2EmitterData::FillVertexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex)
{
	check(Proxy);

	INT	TrianglesToRender = 0;

	FParticleSpriteVertex*			Vertex			= (FParticleSpriteVertex*)(VertexData);
	FMatrix							CameraToWorld	= View->ViewMatrix.Inverse();
	if (TessFactor <=0 )
	{
		TessFactor = 1;
	}
	if (Sheets <= 0)
	{
		Sheets = 1;
	}

	FLOAT	InvTessFactor	= 1.0f / (FLOAT)TessFactor;
	FVector	InterpDrawPos;

	FVector	ViewOrigin	= CameraToWorld.GetOrigin();

	FVector	Offset, LastOffset;
	FLOAT	TextureIncrement;
	FLOAT	fU;
	FLOAT	Angle;
	FQuat	QuatRotator(0, 0, 0, 0);
	FVector	CurrSize, CurrPosition, CurrTangent;
	FVector EndPoint, Location, Right;
	FVector Up, WorkingUp, NextUp, WorkingNextUp;
	FVector	NextSize, NextPosition, NextTangent;
	FVector	TempDrawPos;
	FColor	CurrColor, NextColor, InterpColor;
	FColor	CurrLinearColor, NextLinearColor, InterpLinearColor, LastInterpLinearColor;

	FVector	TessDistCheck;
	INT		SegmentTessFactor;
	FLOAT	TessRatio;

	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	INT		PackedVertexCount	= 0;
	for (INT i = 0; i < ActiveParticleCount; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

		INT	CurrentOffset = TrailDataOffset;

		FTrail2TypeDataPayload* TrailPayload = (FTrail2TypeDataPayload*)((BYTE*)Particle + CurrentOffset);
		CurrentOffset += sizeof(FTrail2TypeDataPayload);

		FLOAT* TaperValues = (FLOAT*)((BYTE*)Particle + CurrentOffset);
		CurrentOffset += sizeof(FLOAT);

		// Pin the size to the X component
		CurrSize	= FVector(Particle->Size.X * Scale.X);
		CurrColor	= Particle->Color;

		if (TRAIL_EMITTER_IS_START(TrailPayload->Flags))
		{
			//@todo. This will only work for a single trail!
			TextureIncrement	= 1.0f / (TessFactor * ActiveParticleCount + 1);
			UBOOL	bFirstInSheet	= TRUE;
			for (INT SheetIndex = 0; SheetIndex < Sheets; SheetIndex++)
			{
				if (SheetIndex)
				{
					Angle		= ((FLOAT)PI / (FLOAT)Sheets) * SheetIndex;
					QuatRotator	= FQuat(Right, Angle);
				}

				fU	= 0.0f;

				// Set the current position to the source...
				/***
				if (TrailSource)
				{
				//					TrailSource->ResolveSourcePoint(Owner, *Particle, *TrailData, CurrPosition, CurrTangent);
				}
				else
				***/
				{
					FVector	Dir = LocalToWorld.GetAxis(0);
					Dir.Normalize();
					CurrTangent	=  Dir * TessStrength;
				}

				CurrPosition	= SourcePosition(TrailPayload->TrailIndex);

				NextPosition	= Particle->Location;
				NextSize		= FVector(Particle->Size.X * Scale.X);
				NextTangent		= TrailPayload->Tangent * TessStrength;
				NextColor		= Particle->Color;
				TempDrawPos		= CurrPosition;

				CurrLinearColor	= FLinearColor(CurrColor);
				NextLinearColor	= FLinearColor(NextColor);

				SegmentTessFactor	= TessFactor;
#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
				if (TrailTypeData->TessellationFactorDistance > KINDA_SMALL_NUMBER)
#else	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
				if (0)
#endif	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
				{
					TessDistCheck		= (CurrPosition - NextPosition);
					TessRatio			= TessDistCheck.Size() / TessFactorDistance;
					if (TessRatio <= 0.0f)
					{
						SegmentTessFactor	= 1;
					}
					else
					if (TessRatio < 1.0f)
					{
						SegmentTessFactor	= appTrunc((TessFactor + 1) * TessRatio);
					}
				}
				// Tessellate the current to next...
#if !defined(_TRAIL2_TESSELLATE_TO_SOURCE_)
				SegmentTessFactor = 1;
#endif	//#if !defined(_TRAIL2_TESSELLATE_TO_SOURCE_)
				InvTessFactor	= 1.0f / SegmentTessFactor;

				for (INT TessIndex = 0; TessIndex < SegmentTessFactor; TessIndex++)
				{
					InterpDrawPos = CubicInterp(
						CurrPosition, CurrTangent,
						NextPosition, NextTangent,
						InvTessFactor * (TessIndex + 1));

					InterpLinearColor	= Lerp<FLinearColor>(
						CurrLinearColor, NextLinearColor, InvTessFactor * (TessIndex + 1));
					InterpColor			= FColor(InterpLinearColor);

					EndPoint	= InterpDrawPos;
					Location	= TempDrawPos;
					Right		= Location - EndPoint;
					Right.Normalize();

					if (bFirstInSheet)
					{
						Up	= Right ^  (Location - ViewOrigin);
						Up.Normalize();
						if (SheetIndex)
						{
							WorkingUp	= QuatRotator.RotateVector(Up);
						}
						else
						{
							WorkingUp	= Up;
						}

						if (WorkingUp.IsNearlyZero())
						{
							WorkingUp	= CameraToWorld.GetAxis(2);
							WorkingUp.Normalize();
						}

						// Setup the lead verts
						Vertex->Position	= Location + WorkingUp * CurrSize;
						Vertex->OldPosition	= Location;
						Vertex->Size		= CurrSize;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= CurrColor;
						Vertex++;
						PackedVertexCount++;

						Vertex->Position	= Location - WorkingUp * CurrSize;
						Vertex->OldPosition	= Location;
						Vertex->Size		= CurrSize;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= Particle->Rotation;
						Vertex->Color		= CurrColor;
						Vertex++;
						PackedVertexCount++;

						fU	+= TextureIncrement;
						bFirstInSheet	= FALSE;
					}

					// Setup the next verts
					NextUp	= Right ^  (EndPoint - ViewOrigin);
					NextUp.Normalize();
					if (SheetIndex)
					{
						WorkingNextUp	= QuatRotator.RotateVector(NextUp);
					}
					else
					{
						WorkingNextUp	= NextUp;
					}

					if (WorkingNextUp.IsNearlyZero())
					{
						WorkingNextUp	= CameraToWorld.GetAxis(2);
						WorkingNextUp.Normalize();
					}
					Vertex->Position	= EndPoint + WorkingNextUp * NextSize;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= NextSize;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 0.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= InterpColor;
					Vertex++;
					PackedVertexCount++;

					Vertex->Position	= EndPoint - WorkingNextUp * NextSize;
					Vertex->OldPosition	= EndPoint;
					Vertex->Size		= NextSize;
					Vertex->Tex_U		= fU;
					Vertex->Tex_V		= 1.0f;
					Vertex->Rotation	= Particle->Rotation;
					Vertex->Color		= InterpColor;
					Vertex++;
					PackedVertexCount++;

					fU	+= TextureIncrement;

					TempDrawPos				= InterpDrawPos;
					LastInterpLinearColor	= InterpLinearColor;
				}

				CurrPosition	= NextPosition;
				CurrTangent		= NextTangent;
				CurrSize		= NextSize;
				CurrColor		= NextColor;

				UBOOL	bDone	= TRAIL_EMITTER_IS_ONLY(TrailPayload->Flags);

				while (!bDone)
				{
					// Grab the next particle
					INT	NextIndex	= TRAIL_EMITTER_GET_NEXT(TrailPayload->Flags);

					DECLARE_PARTICLE_PTR(NextParticle, ParticleData + ParticleStride * NextIndex);

					CurrentOffset = TrailDataOffset;

					TrailPayload = (FTrail2TypeDataPayload*)((BYTE*)NextParticle + CurrentOffset);
					CurrentOffset += sizeof(FTrail2TypeDataPayload);

					TaperValues = (FLOAT*)((BYTE*)NextParticle + CurrentOffset);
					CurrentOffset += sizeof(FLOAT);

					NextPosition	= NextParticle->Location;
					NextTangent		= TrailPayload->Tangent * TessStrength;
					NextSize		= FVector(NextParticle->Size.X * Scale.X);
					NextColor		= NextParticle->Color;

					TempDrawPos	= CurrPosition;

					CurrLinearColor	= FLinearColor(CurrColor);
					NextLinearColor	= FLinearColor(NextColor);

					SegmentTessFactor	= TessFactor;
#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
					if (TrailTypeData->TessellationFactorDistance > KINDA_SMALL_NUMBER)
#else	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
					if (0)
#endif	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
					{
						TessDistCheck		= (CurrPosition - NextPosition);
						TessRatio			= TessDistCheck.Size() / TessFactorDistance;
						if (TessRatio <= 0.0f)
						{
							SegmentTessFactor	= 1;
						}
						else
						if (TessRatio < 1.0f)
						{
							SegmentTessFactor	= appTrunc((TessFactor + 1) * TessRatio);
						}
					}
					InvTessFactor	= 1.0f / SegmentTessFactor;

					for (INT TessIndex = 0; TessIndex < SegmentTessFactor; TessIndex++)
					{
						InterpDrawPos = CubicInterp(
							CurrPosition, CurrTangent,
							NextPosition, NextTangent,
							InvTessFactor * (TessIndex + 1));

						InterpLinearColor	= Lerp<FLinearColor>(
							CurrLinearColor, NextLinearColor, InvTessFactor * (TessIndex + 1));
						InterpColor			= FColor(InterpLinearColor);

						EndPoint	= InterpDrawPos;
						Location	= TempDrawPos;
						Right		= Location - EndPoint;
						Right.Normalize();

						// Setup the next verts
						NextUp	= Right ^  (EndPoint - ViewOrigin);
						NextUp.Normalize();
						if (SheetIndex)
						{
							WorkingNextUp	= QuatRotator.RotateVector(NextUp);
						}
						else
						{
							WorkingNextUp	= NextUp;
						}

						if (WorkingNextUp.IsNearlyZero())
						{
							WorkingNextUp	= CameraToWorld.GetAxis(2);
							WorkingNextUp.Normalize();
						}
						Vertex->Position	= EndPoint + WorkingNextUp * NextSize;
						Vertex->OldPosition	= EndPoint;
						Vertex->Size		= NextSize;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 0.0f;
						Vertex->Rotation	= NextParticle->Rotation;
						Vertex->Color		= InterpColor;
						Vertex++;
						PackedVertexCount++;

						Vertex->Position	= EndPoint - WorkingNextUp * NextSize;
						Vertex->OldPosition	= EndPoint;
						Vertex->Size		= NextSize;
						Vertex->Tex_U		= fU;
						Vertex->Tex_V		= 1.0f;
						Vertex->Rotation	= NextParticle->Rotation;
						Vertex->Color		= InterpColor;
						Vertex++;
						PackedVertexCount++;

						fU	+= TextureIncrement;

						TempDrawPos	= InterpDrawPos;
					}

					CurrPosition	= NextPosition;
					CurrTangent		= NextTangent;
					CurrSize		= NextSize;
					CurrColor		= NextColor;

					if (TRAIL_EMITTER_IS_END(TrailPayload->Flags) ||
						TRAIL_EMITTER_IS_ONLY(TrailPayload->Flags))
					{
						bDone = TRUE;
					}
				}
			}
		}
	}

	return TrianglesToRender;
}

///////////////////////////////////////////////////////////////////////////////
//	ParticleSystemSceneProxy
///////////////////////////////////////////////////////////////////////////////
/** Initialization constructor. */
FParticleSystemSceneProxy::FParticleSystemSceneProxy(const UParticleSystemComponent* Component):
	  FPrimitiveSceneProxy(Component)
	, Owner(Component->GetOwner())
	, bSelected(Component->IsOwnerSelected())
	, CullDistance(Component->CachedCullDistance > 0 ? Component->CachedCullDistance : WORLD_MAX)
	, bCastShadow(Component->CastShadow)
	, MaterialViewRelevance(
		((Component->LODLevel >= 0) && (Component->LODLevel < Component->CachedViewRelevanceFlags.Num())) ?
			Component->CachedViewRelevanceFlags(Component->LODLevel) :
		((Component->LODLevel == -1) && (Component->CachedViewRelevanceFlags.Num() >= 1)) ?
			Component->CachedViewRelevanceFlags(0) :
			FMaterialViewRelevance()
		)
	, DynamicData(NULL)
	, LastDynamicData(NULL)
	, SelectedWireframeMaterialInstance(
		GEngine->WireframeMaterial->GetRenderProxy(FALSE),
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),TRUE)
		)
	, DeselectedWireframeMaterialInstance(
		GEngine->WireframeMaterial->GetRenderProxy(FALSE),
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),FALSE)
		)
	, PendingLODDistance(0.0f)
{
	LODMethod = Component->LODMethod;
}

FParticleSystemSceneProxy::~FParticleSystemSceneProxy()
{
	delete DynamicData;
	DynamicData = NULL;
}

// FPrimitiveSceneProxy interface.

/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
* @param	DPGIndex - current depth priority 
* @param	Flags - optional set of flags from EDrawDynamicElementFlags
*/
void FParticleSystemSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
	if (View->Family->ShowFlags & SHOW_Particles)
	{
		// Determine the DPG the primitive should be drawn in for this view.
		if (GetDepthPriorityGroup(View) == DPGIndex)
		{
			if (DynamicData != NULL)
			{
				for (INT Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
				{
					FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray(Index);
					if (Data == NULL)
					{
						continue;
					}
					// only allow rendering of mesh data based on static or dynamic vertex usage 
					// particles are rendered in two passes - one for static and one for dynamic
					if( (Flags&DontAllowStaticMeshElementData && !Data->HasDynamicMeshElementData()) ||
						(Flags&DontAllowDynamicMeshElementData && Data->HasDynamicMeshElementData()) )
					{
						continue;
					}

					Data->SceneProxy = this;
					Data->Render(this, PDI, View, DPGIndex);
				}
			}
			DetermineLODDistance(View);
		}

		if ((DPGIndex == SDPG_Foreground) && (View->Family->ShowFlags & SHOW_Bounds) && (View->Family->ShowFlags & SHOW_Particles) && (GIsGame || !Owner || Owner->IsSelected()))
		{
			// Draw the static mesh's bounding box and sphere.
			DrawWireBox(PDI,PrimitiveSceneInfo->Bounds.GetBox(), FColor(72,72,255),SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,1,0),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(0,1,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
		}
	}
}

void FParticleSystemSceneProxy::DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI,const FSceneView* View,const class FLightSceneInfo* Light)
{
	checkSlow(UEngine::ShadowVolumesAllowed());
}

/**
 *	Called when the rendering thread adds the proxy to the scene.
 *	This function allows for generating renderer-side resources.
 */
UBOOL FParticleSystemSceneProxy::CreateRenderThreadResources()
{
	// 
	if (DynamicData == NULL)
	{
		return FALSE;
	}

	for (INT Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
	{
		FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray(Index);
		if (Data == NULL)
		{
			continue;
		}

		switch (Data->eEmitterType)
		{
		case FDynamicEmitterDataBase::DET_Sprite:
			{
				FDynamicSpriteEmitterData* SpriteData = (FDynamicSpriteEmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (SpriteData->VertexFactory == NULL)
				{
					SpriteData->VertexFactory = new FParticleVertexFactory();
					check(SpriteData->VertexFactory);
					SpriteData->VertexFactory->Init();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_SubUV:
			{
				FDynamicSubUVEmitterData* SubUVData = (FDynamicSubUVEmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (SubUVData->VertexFactory == NULL)
				{
					SubUVData->VertexFactory = new FParticleSubUVVertexFactory();
					check(SubUVData->VertexFactory);
					SubUVData->VertexFactory->Init();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_Mesh:
			{
                FDynamicMeshEmitterData* MeshData = (FDynamicMeshEmitterData*)Data;
			}
			break;
		case FDynamicEmitterDataBase::DET_Beam2:
			{
				FDynamicBeam2EmitterData* BeamData = (FDynamicBeam2EmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (BeamData->VertexFactory == NULL)
				{
					BeamData->VertexFactory = new FParticleBeamTrailVertexFactory();
					check(BeamData->VertexFactory);
					BeamData->VertexFactory->Init();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_Trail2:
			{
				FDynamicTrail2EmitterData* TrailData = (FDynamicTrail2EmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (TrailData->VertexFactory == NULL)
				{
					TrailData->VertexFactory = new FParticleBeamTrailVertexFactory();
					check(TrailData->VertexFactory);
					TrailData->VertexFactory->Init();
				}
			}
			break;
		default:
			break;
		}
	}

	return TRUE;
}

/**
 *	Called when the rendering thread removes the dynamic data from the scene.
 */
UBOOL FParticleSystemSceneProxy::ReleaseRenderThreadResources()
{
	// 
	if (DynamicData == NULL)
	{
		return FALSE;
	}

	for (INT Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
	{
		FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray(Index);
		if (Data == NULL)
		{
			continue;
		}

		switch (Data->eEmitterType)
		{
		case FDynamicEmitterDataBase::DET_Sprite:
			{
				FDynamicSpriteEmitterData* SpriteData = (FDynamicSpriteEmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (SpriteData->VertexFactory)
				{
					SpriteData->VertexFactory->Release();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_SubUV:
			{
				FDynamicSubUVEmitterData* SubUVData = (FDynamicSubUVEmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (SubUVData->VertexFactory)
				{
					SubUVData->VertexFactory->Release();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_Mesh:
			{
				FDynamicMeshEmitterData* MeshData = (FDynamicMeshEmitterData*)Data;
			}
			break;
		case FDynamicEmitterDataBase::DET_Beam2:
			{
				FDynamicBeam2EmitterData* BeamData = (FDynamicBeam2EmitterData*)Data;
				// Create the vertex factory...
				//@todo. Cache these??
				if (BeamData->VertexFactory)
				{
					BeamData->VertexFactory->Release();
				}
			}
			break;
		case FDynamicEmitterDataBase::DET_Trail2:
			{
				FDynamicTrail2EmitterData* TrailData = (FDynamicTrail2EmitterData*)Data;
				if (TrailData->VertexFactory)
				{
					TrailData->VertexFactory->Release();
				}
			}
			break;
		default:
			break;
		}
	}

	return TRUE;
}

void FParticleSystemSceneProxy::UpdateData(FParticleDynamicData* NewDynamicData)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ParticleUpdateDataCommand,
		FParticleSystemSceneProxy*, Proxy, this,
		FParticleDynamicData*, NewDynamicData, NewDynamicData,
		{
			Proxy->UpdateData_RenderThread(NewDynamicData);
		}
		);
}

void FParticleSystemSceneProxy::UpdateData_RenderThread(FParticleDynamicData* NewDynamicData)
{
	ReleaseRenderThreadResources();
	if (DynamicData != NewDynamicData)
	{
		delete DynamicData;
	}
	DynamicData = NewDynamicData;
	CreateRenderThreadResources();
}

void FParticleSystemSceneProxy::DetermineLODDistance(const FSceneView* View)
{
	INT	LODIndex = -1;

	if (LODMethod == PARTICLESYSTEMLODMETHOD_Automatic)
	{
		// Default to the highest LOD level
		FVector	CameraPosition	= View->ViewOrigin;
		FVector	CompPosition	= LocalToWorld.GetOrigin();
		FVector	DistDiff		= CompPosition - CameraPosition;
		FLOAT	Distance		= DistDiff.Size() * View->LODDistanceFactor;
		PendingLODDistance = Distance;
	}
}

/**
 *	Retrieve the appropriate camera Up and Right vectors for LockAxis situations
 *
 *	@param	DynamicData		The emitter dynamic data the values are being retrieved for
 *	@param	CameraUp		OUTPUT - the resulting camera Up vector
 *	@param	CameraRight		OUTPUT - the resulting camera Right vector
 */
void FParticleSystemSceneProxy::GetAxisLockValues(FDynamicSpriteEmitterDataBase* DynamicData, FVector& CameraUp, FVector& CameraRight)
{
	const FMatrix& AxisLocalToWorld = DynamicData->bUseLocalSpace ? LocalToWorld: FMatrix::Identity;

	switch (DynamicData->LockAxisFlag)
	{
	case EPAL_X:
		CameraUp		=  AxisLocalToWorld.GetAxis(2);
		CameraRight	=  AxisLocalToWorld.GetAxis(1);
		break;
	case EPAL_Y:
		CameraUp		=  AxisLocalToWorld.GetAxis(2);
		CameraRight	= -AxisLocalToWorld.GetAxis(0);
		break;
	case EPAL_Z:
		CameraUp		=  AxisLocalToWorld.GetAxis(0);
		CameraRight	= -AxisLocalToWorld.GetAxis(1);
		break;
	case EPAL_NEGATIVE_X:
		CameraUp		=  AxisLocalToWorld.GetAxis(2);
		CameraRight	= -AxisLocalToWorld.GetAxis(1);
		break;
	case EPAL_NEGATIVE_Y:
		CameraUp		=  AxisLocalToWorld.GetAxis(2);
		CameraRight	=  AxisLocalToWorld.GetAxis(0);
		break;
	case EPAL_NEGATIVE_Z:
		CameraUp		=  AxisLocalToWorld.GetAxis(0);
		CameraRight	=  AxisLocalToWorld.GetAxis(1);
		break;
	}
}

/**
* @return Relevance for rendering the particle system primitive component in the given View
*/
FPrimitiveViewRelevance FParticleSystemSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	const EShowFlags ShowFlags = View->Family->ShowFlags;
	if (IsShown(View) && (ShowFlags & SHOW_Particles))
	{
		Result.bDynamicRelevance = TRUE;
		Result.SetDPG(GetDepthPriorityGroup(View),TRUE);
		if (!(View->Family->ShowFlags & SHOW_Wireframe) && (View->Family->ShowFlags & SHOW_Materials))
		{
			MaterialViewRelevance.SetPrimitiveViewRelevance(Result);
		}
		if (View->Family->ShowFlags & SHOW_Bounds)
		{
			Result.SetDPG(SDPG_Foreground,TRUE);
			Result.bOpaqueRelevance = TRUE;
		}
		// see if any of the emitters use dynamic vertex data
		if (DynamicData != NULL)
		{
			for (INT Index = 0; Index < DynamicData->DynamicEmitterDataArray.Num(); Index++)
			{
				FDynamicEmitterDataBase* Data =	DynamicData->DynamicEmitterDataArray(Index);
				if (Data == NULL)
				{
					continue;
				}
				if( Data->HasDynamicMeshElementData() )
				{
					Result.bUsesDynamicMeshElementData = TRUE;
				}
			}
		}
	}

	if (IsShadowCast(View))
	{
		Result.bShadowRelevance = TRUE;
	}

	return Result;
}

////////////////////////////////////////////////////////////////////////////////
//	ParticleSystemComponent
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UParticleSystemComponent::CreateSceneProxy()
{
	FParticleSystemSceneProxy* NewProxy = NULL;

	//@fixme EmitterInstances.Num() check should be here to avoid proxies for dead emitters but there are some edge cases where it happens for emitters that have just activated...
	if ((bIsActive == TRUE)/** && (EmitterInstances.Num() > 0)*/)
	{
		CacheViewRelevanceFlags(NULL);
		NewProxy = ::new FParticleSystemSceneProxy(this);
		check (NewProxy);
	}
	
	// 
	return NewProxy;
}

////////////////////////////////////////////////////////////////////////////////
//	Helper functions
///////////////////////////////////////////////////////////////////////////////
void PS_DumpBeamDataInformation(TCHAR* Message, 
	UParticleSystemComponent* PSysComp, FParticleSystemSceneProxy* Proxy, 
	FParticleDynamicData* NewPSDynamicData, FParticleDynamicData* OldPSDynamicData, 
	FDynamicBeam2EmitterData* NewBeamData, FDynamicBeam2EmitterData* OldBeamData)
{
#if defined(_DEBUG_BEAM_DATA_)
	INT	Spaces = 0;
	if (Message)
	{
		OutputDebugString(Message);
		Spaces = appStrlen(Message);
	}

	while (Spaces < 72)
	{
		OutputDebugString(TEXT(" "));
		Spaces++;
	}
	OutputDebugString(TEXT("    "));

	FString DebugOut = FString::Printf(TEXT("PSysComp     0x%08x - SceneProxy   0x%08x - DynamicData  0x%08x - OldDynData   0x%08x - BeamData     0x%08x - OldBeamData  0x%08x"),
		PSysComp, Proxy, NewPSDynamicData, OldPSDynamicData, NewBeamData, OldBeamData);
	OutputDebugString(*DebugOut);
	OutputDebugString(TEXT("\n"));
#endif
}

void PS_DumpTrailDataInformation(TCHAR* Message, 
	UParticleSystemComponent* PSysComp, FParticleSystemSceneProxy* Proxy, 
	FParticleDynamicData* NewPSDynamicData, FParticleDynamicData* OldPSDynamicData, 
	FDynamicTrail2EmitterData* NewTrailData, FDynamicTrail2EmitterData* OldTrailData)
{
#if defined(_DEBUG_TRAIL_DATA_)
	INT	Spaces = 0;
	if (Message)
	{
		OutputDebugString(Message);
		Spaces = appStrlen(Message);
	}

	while (Spaces < 48)
	{
		OutputDebugString(TEXT(" "));
		Spaces++;
	}
	OutputDebugString(TEXT("    "));

	FString DebugOut = FString::Printf(TEXT("PSysComp     0x%08x - SceneProxy   0x%08x - DynamicData  0x%08x - OldDynData   0x%08x - TrailData     0x%08x - OldTrailData  0x%08x"),
		PSysComp, Proxy, NewPSDynamicData, OldPSDynamicData, NewTrailData, OldTrailData);
	OutputDebugString(*DebugOut);
	OutputDebugString(TEXT("\n"));
#endif
}
