/*=============================================================================
	Scene.cpp: Scene manager implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "EngineFogVolumeClasses.h"
#include "EngineMeshClasses.h"
#include "EngineFluidClasses.h"
#include "FluidSurfaceGPUSimulation.h"

#define TRACK_GAME_THREAD_RENDER_MALLOCS		0

EShowFlags SHOW_Collision_Any;
EShowFlags SHOW_ViewMode_Lit;
EShowFlags SHOW_EditorOnly_Mask;
EShowFlags SHOW_ViewMode_Wireframe;
EShowFlags SHOW_ViewMode_BrushWireframe;
EShowFlags SHOW_ViewMode_Unlit;
EShowFlags SHOW_ViewMode_LightingOnly;
EShowFlags SHOW_ViewMode_LightComplexity;
EShowFlags SHOW_ViewMode_ShaderComplexity;
EShowFlags SHOW_ViewMode_TextureDensity;
EShowFlags SHOW_ViewMode_LightMapDensity;
EShowFlags SHOW_ViewMode_LitLightmapDensity;
EShowFlags SHOW_DefaultGame;
EShowFlags SHOW_DefaultEditor;
EShowFlags SHOW_ViewMode_Mask;

// ----------------------------------------------------------------------------------------


// Enable this define to do slow checks for components being added to the wrong
// world's scene, when using PIE. This can happen if a PIE component is reattached
// while GWorld is the editor world, for example.
#define CHECK_FOR_PIE_PRIMITIVE_ATTACH_SCENE_MISMATCH	0


#if TRACK_GAME_THREAD_RENDER_MALLOCS
DECLARE_DWORD_COUNTER_STAT( TEXT( "Mallocs during proxy creation" ), STAT_GameToRendererMalloc, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Frees during proxy creation" ), STAT_GameToRendererFree, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Net memory during proxy creation" ), STAT_GameToRendererNet, STATGROUP_Memory );
#endif

DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy Total" ), STAT_GameToRendererMallocTotal, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy FParticleSystemSceneProxy" ), STAT_GameToRendererMallocPSSP, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy FSkeletalMeshSceneProxy" ), STAT_GameToRendererMallocSkMSP, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy FStaticMeshSceneProxy" ), STAT_GameToRendererMallocStMSP, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy FLensFlareSceneProxy" ), STAT_GameToRendererMallocLFSP, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy FDecalSceneProxy" ), STAT_GameToRendererMallocDecalSP, STATGROUP_Memory );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Proxy Other" ), STAT_GameToRendererMallocOther, STATGROUP_Memory );

IMPLEMENT_CLASS(UScene);


/** The motion blur info entries for the frame. Accessed on Renderthread only! */
TArray<FMotionBlurInfo> FScene::MotionBlurInfoArray;
/** Stack of indices for available entries in MotionBlurInfoArray. Accessed on Renderthread only! */
TArray<INT> FScene::MotionBlurFreeEntries;

// This function is necessary to get a defined initialization order for the global variables.
void appInitShowFlags()
{
	// The following code defines combined EShowFlag constants.

	SHOW_Collision_Any = SHOW_CollisionNonZeroExtent | SHOW_CollisionZeroExtent | SHOW_CollisionRigidBody;
	SHOW_ViewMode_Lit = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Materials | SHOW_Lighting | SHOW_PostProcess;
	SHOW_EditorOnly_Mask = SHOW_Editor | SHOW_LightRadius | SHOW_AudioRadius | SHOW_LargeVertices | SHOW_HitProxies | SHOW_NavigationNodes | SHOW_LightInfluences | SHOW_ActorTags | SHOW_PropertyColoration;
	SHOW_ViewMode_Wireframe = SHOW_InstancedStaticMeshes | SHOW_Wireframe | SHOW_BSPTriangles;
	SHOW_ViewMode_BrushWireframe = SHOW_Wireframe | SHOW_Brushes;
	SHOW_ViewMode_Unlit = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Materials | SHOW_PostProcess;
	SHOW_ViewMode_LightingOnly = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Lighting | SHOW_PostProcess;
	SHOW_ViewMode_LightComplexity = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_LightComplexity;
	SHOW_ViewMode_ShaderComplexity = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Materials | SHOW_Lighting | SHOW_ShaderComplexity | SHOW_Decals;
	SHOW_ViewMode_TextureDensity = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Materials | SHOW_TextureDensity;
	SHOW_ViewMode_LightMapDensity = SHOW_InstancedStaticMeshes | SHOW_BSPTriangles | SHOW_Materials | SHOW_LightMapDensity;
	SHOW_ViewMode_LitLightmapDensity = SHOW_InstancedStaticMeshes | SHOW_ViewMode_LightingOnly | SHOW_LightMapDensity;

	// The following ones have been split up into multiple parts (Scene.cpp compile time speedup on some compiler ~1h -> 17sec), make sure that the lines don't get longer than this

	EShowFlags SHOW_DefaultGame_Part1 = SHOW_Game | SHOW_StaticMeshes | SHOW_InstancedStaticMeshes | SHOW_SkeletalMeshes | SHOW_Terrain | SHOW_SpeedTrees | SHOW_BSPTriangles | SHOW_BSP | SHOW_Selection | SHOW_Fog;
	EShowFlags SHOW_DefaultGame_Part2 = SHOW_Particles | SHOW_Decals | SHOW_DynamicShadows | SHOW_SceneCaptureUpdates | SHOW_Sprites | SHOW_PostProcess | SHOW_ImageGrain;
	EShowFlags SHOW_DefaultGame_Part3 = SHOW_UnlitTranslucency | SHOW_ViewMode_Lit | SHOW_LensFlares | SHOW_LOD | SHOW_TranslucencyDoF | SHOW_MotionBlur | SHOW_CameraInterpolation;
	EShowFlags SHOW_DefaultGame_Part4 = SHOW_DepthOfField | SHOW_ImageReflections | SHOW_SubsurfaceScattering | SHOW_LightFunctions | SHOW_Tesselation;
	SHOW_DefaultGame = SHOW_DefaultGame_Part1 | SHOW_DefaultGame_Part2 | SHOW_DefaultGame_Part3 | SHOW_DefaultGame_Part4;

	EShowFlags SHOW_DefaultEditor_Part1 = SHOW_Editor | SHOW_StaticMeshes | SHOW_InstancedStaticMeshes | SHOW_SkeletalMeshes | SHOW_Terrain | SHOW_SpeedTrees | SHOW_BSP | SHOW_Fog | SHOW_Grid | SHOW_Selection | SHOW_Volumes;
	EShowFlags SHOW_DefaultEditor_Part2 = SHOW_NavigationNodes | SHOW_Particles | SHOW_BuilderBrush | SHOW_Decals | SHOW_DecalInfo | SHOW_LightRadius | SHOW_AudioRadius | SHOW_DynamicShadows | SHOW_SceneCaptureUpdates | SHOW_Sprites;
	EShowFlags SHOW_DefaultEditor_Part3 = SHOW_ModeWidgets | SHOW_UnlitTranslucency | SHOW_Constraints | SHOW_LensFlares | SHOW_LOD | SHOW_Splines | SHOW_TranslucencyDoF | SHOW_PostProcess;
	EShowFlags SHOW_DefaultEditor_Part4 = SHOW_DepthOfField | SHOW_ImageReflections | SHOW_SubsurfaceScattering | SHOW_LightFunctions | SHOW_Tesselation | SHOW_CameraInterpolation;
	SHOW_DefaultEditor = SHOW_DefaultEditor_Part1 | SHOW_DefaultEditor_Part2 | SHOW_DefaultEditor_Part3 | SHOW_DefaultEditor_Part4;

	EShowFlags SHOW_ViewMode_Mask_Part1 = SHOW_InstancedStaticMeshes | SHOW_Wireframe | SHOW_BSPTriangles | SHOW_Lighting | SHOW_Materials | SHOW_LightComplexity;
	EShowFlags SHOW_ViewMode_Mask_Part2 = SHOW_Brushes | SHOW_TextureDensity | SHOW_PostProcess | SHOW_ShaderComplexity | SHOW_LightMapDensity | SHOW_VertexColors;
	SHOW_ViewMode_Mask = SHOW_ViewMode_Mask_Part1 | SHOW_ViewMode_Mask_Part2;
}

void FPrimitiveOcclusionHistory::ReleaseQueries(FOcclusionQueryPool& Pool)
{
	for (INT QueryIndex = 0; QueryIndex < NumBufferedFrames; QueryIndex++)
	{
		Pool.ReleaseQuery(PendingOcclusionQuery(QueryIndex));
	}
}

/** Resets all allocations and sets a new NumChannels. */
void FLightChannelAllocator::Reset(INT InNumChannels) 
{
	NumChannels = InNumChannels;
	Lights.Empty();
	DomDirectionalLight = FLightChannelInfo(INDEX_NONE, 0);
}

/** Adds a new light allocation. */
void FLightChannelAllocator::AllocateLight(INT LightId, FLOAT Priority, UBOOL bDomDirectionalLight)
{
	if (bDomDirectionalLight)
	{
		DomDirectionalLight = FLightChannelInfo(LightId, Priority);
	}
	else
	{
		for (INT SearchIndex = 0; SearchIndex < Lights.Num(); SearchIndex++)
		{
			FLightChannelInfo& Info = Lights(SearchIndex);
			// Sort lights from lowest priority at the beginning of the array to highest
			if (Priority < Info.Priority)
			{
				Lights.InsertItem(FLightChannelInfo(LightId, Priority), SearchIndex);
				return;
			}
		}
		Lights.AddItem(FLightChannelInfo(LightId, Priority));
	}
}

/** Returns the channel index that the given light should use, or INDEX_NONE if the light was not allocated. */
INT FLightChannelAllocator::GetLightChannel(INT LightId) const
{
	// If the dominant directional light was allocated, it must be in channel 0,
	// And if both textures are being used, it must be in texture 1.
	if (DomDirectionalLight.LightId != INDEX_NONE && LightId == DomDirectionalLight.LightId)
	{
		return 0;
	}
	else
	{
		for (INT SearchIndex = 0; SearchIndex < Lights.Num(); SearchIndex++)
		{
			if (Lights(SearchIndex).LightId == LightId)
			{
				if (DomDirectionalLight.LightId == INDEX_NONE)
				{
					if (SearchIndex < NumChannels)
					{
						// The light is in texture 0
						return SearchIndex;
					}
					else
					{
						// The light is in texture 1
						return Min(SearchIndex - NumChannels, NumChannels - 1);
					}
				}
				else
				{
					if (Lights.Num() + 1 > NumChannels)
					{
						if (SearchIndex < NumChannels)
						{
							// The light is in texture 0
							return SearchIndex;
						}
						else
						{
							// The light is in texture 1
							// The dominant directional light is at index 0 in texture 1 so offset the other lights
							// Clamp the index to the last channel in texture 1, which multiple lights will share if there are too many lights (more than NumChannels * 2)
							return Min(SearchIndex + 1 - NumChannels, NumChannels - 1);
						}
					}
					else
					{
						// The dominant directional light is at index 0 so offset the other lights
						return SearchIndex + 1;
					}
				}
			}
		}
	}
	
	return INDEX_NONE;
}

/** Returns the index of the texture that the light has been allocated in, either 0 or 1. */
INT FLightChannelAllocator::GetTextureIndex(INT LightId) const
{
	// If the dominant directional light was allocated, it must be in channel 0,
	// And if both textures are being used, it must be in texture 1.
	if (DomDirectionalLight.LightId != INDEX_NONE && LightId == DomDirectionalLight.LightId)
	{
		return Lights.Num() + 1 > NumChannels ? 1 : 0;
	}
	else
	{
		for (INT SearchIndex = 0; SearchIndex < Lights.Num(); SearchIndex++)
		{
			if (Lights(SearchIndex).LightId == LightId)
			{
				return SearchIndex < NumChannels ? 0 : 1;
			}
		}
	}

	return 0;
}

/** Returns the index of the texture that the dominant directional light has been allocated to, or INDEX_NONE if it was never allocated. */
INT FLightChannelAllocator::GetDominantDirectionalLightTexture() const
{
	if (DomDirectionalLight.LightId != INDEX_NONE)
	{
		return GetTextureIndex(DomDirectionalLight.LightId);
	}
	return INDEX_NONE;
}

FSynchronizedActorVisibilityHistory::FSynchronizedActorVisibilityHistory():
	States(NULL),
		CriticalSection(NULL)
{
}

FSynchronizedActorVisibilityHistory::~FSynchronizedActorVisibilityHistory()
{
	if(CriticalSection)
	{
		GSynchronizeFactory->Destroy(CriticalSection);
		CriticalSection = NULL;
	}
	delete States;
}

void FSynchronizedActorVisibilityHistory::Init()
{
	CriticalSection = GSynchronizeFactory->CreateCriticalSection();
}

UBOOL FSynchronizedActorVisibilityHistory::GetActorVisibility(const AActor* Actor) const
{
	FScopeLock Lock(CriticalSection);
	return States ? States->GetActorVisibility(Actor) : 0;
}

void FSynchronizedActorVisibilityHistory::SetStates(FActorVisibilityHistoryInterface* NewStates)
{
	FScopeLock Lock(CriticalSection);
	delete States;
	States = NewStates;
}



/**
 * Utility function to create the inverse depth projection transform to be used
 * by the shader system.
 * @param ProjectionMatrix - used to extract the scene depth ratios
 * @param InvertZ - projection calc is affected by inverted device Z
 * @return vector containting the ratios needed to convert from device Z to world Z
 */
FVector4 CreateInvDeviceZToWorldZTransform(FMatrix const & ProjectionMatrix, UBOOL InvertZ)
{
	// The depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	//
	// The old shader code uses:
	// Z = C1 / (1 - (Z' / C2))	--- Where C1 = -B / A, C2 = A
	//
	// The new shader code uses:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	FLOAT DepthMul = ProjectionMatrix.M[2][2];
	FLOAT DepthAdd = ProjectionMatrix.M[3][2];

	// Adjust the values for an inverted Z buffer

	if (InvertZ)
	{
		DepthMul = 1.0f - DepthMul;
		DepthAdd = - DepthAdd;
	}

	return FVector4(
		DepthAdd,		// Old projection method
		DepthMul,
		1.f / DepthAdd,				// New projection method
		DepthMul / DepthAdd
		);
}

FSceneView::FSceneView(
	const FSceneViewFamily* InFamily,
	FSceneViewStateInterface* InState,
	INT InParentViewIndex,
	const FSceneViewFamily* InParentViewFamily,
	FSynchronizedActorVisibilityHistory* InActorVisibilityHistory,
	const AActor* InViewActor,
	const UPostProcessChain* InPostProcessChain,
	const FPostProcessSettings* InPostProcessSettings,
	FViewElementDrawer* InDrawer,
	FLOAT InX,
	FLOAT InY,
	FLOAT InSizeX,
	FLOAT InSizeY,
	const FMatrix& InViewMatrix,
	const FMatrix& InProjectionMatrix,
	const FLinearColor& InBackgroundColor,
	const FLinearColor& InOverlayColor,
	const FLinearColor& InColorScale,
	const TSet<UPrimitiveComponent*>& InHiddenPrimitives,
	FLOAT InLODDistanceFactor
#if !CONSOLE
	, QWORD InEditorViewBitflag
	, const FVector4& InOverrideLODViewOrigin
	, UBOOL InUseFauxOrthoViewPos
#endif
	):
	Family(InFamily),
	State(InState),
	ParentViewIndex(InParentViewIndex),
	ParentViewFamily(InParentViewFamily),
	ActorVisibilityHistory(InActorVisibilityHistory),
	ViewActor(InViewActor),
	PostProcessChain(InPostProcessChain),
	PostProcessSettings(InPostProcessSettings),
	Drawer(InDrawer),
	X(InX),
	Y(InY),
	SizeX(InSizeX),
	SizeY(InSizeY),
	FrameNumber(UINT_MAX),
	ViewMatrix(InViewMatrix),
	ProjectionMatrix(InProjectionMatrix),
	BackgroundColor(InBackgroundColor),
	OverlayColor(InOverlayColor),
	ColorScale(InColorScale),
	DiffuseOverrideParameter(FVector4(0,0,0,1)),
	SpecularOverrideParameter(FVector4(0,0,0,1)),
	HiddenPrimitives(InHiddenPrimitives),
	ScreenDoorRandomOffset( 0.0f, 0.0f ),
	LODDistanceFactor(InLODDistanceFactor),
	bUseLDRSceneColor(FALSE),
	bIsGameView(GIsGame),
	bForceLowestMassiveLOD(FALSE)
#if !CONSOLE
	, OverrideLODViewOrigin(InOverrideLODViewOrigin)
	, bAllowTranslucentPrimitivesInHitProxy( TRUE )
#endif
{
	check(SizeX > 0.0f);
	check(SizeY > 0.0f);

	// Compute the view projection matrx and its inverse.
	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

	InvProjectionMatrix = ProjectionMatrix.Inverse();

	// For precision reasons the view matrix inverse is calculated independently.
	const FMatrix InvViewMatrix = ViewMatrix.InverseSafe();
	InvViewProjectionMatrix = InvProjectionMatrix * InvViewMatrix;

	UBOOL ApplyPreViewTranslation = TRUE;

	// Calculate the view origin from the view/projection matrices.
	if(ProjectionMatrix.M[3][3] < 1.0f)
	{
		ViewOrigin = FVector4(InvViewMatrix.GetOrigin(),1);
	}
#if !CONSOLE
	else if (InUseFauxOrthoViewPos)
	{
		float DistanceToViewOrigin = InvViewMatrix.GetOrigin().Size();
		ViewOrigin = FVector4(InvViewMatrix.TransformNormal(FVector(0,0,-1)).SafeNormal()*DistanceToViewOrigin,1);		
		// to avoid issues with view dependent effect (e.g. Frensel)
		ApplyPreViewTranslation = FALSE;
		PreViewTranslation = FVector(0,0,0);
	}
#endif
	else
	{
		ViewOrigin = FVector4(InvViewMatrix.TransformNormal(FVector(0,0,-1)).SafeNormal(),0);
		// to avoid issues with view dependent effect (e.g. Frensel)
		ApplyPreViewTranslation = FALSE;
		PreViewTranslation = FVector(0,0,0);
	}


	// Compute random offset for screen door fades this frame.  We want the same random noise value
	// to be used for all primitives within the view, but different each frame.
	ScreenDoorRandomOffset = FVector2D( appRand() / 65536.0f, appRand() / 65536.0f );

	// Translate world-space so its origin is at ViewOrigin for improved precision.
	// Note that this isn't exactly right for orthogonal projections (See the above special case), but we still use ViewOrigin
	// in that case so the same value may be used in shaders for both the world-space translation and the camera's world position.
	if(ApplyPreViewTranslation)
	{
		static FVector PreViewTranslationBackup;

		extern INT GPreViewTranslation;
		if(GPreViewTranslation)
		{
			PreViewTranslation = -FVector(ViewOrigin);
			PreViewTranslationBackup = PreViewTranslation;
		}
		else
		{
			PreViewTranslation = PreViewTranslationBackup;
		}
	}
	else
	{
		PreViewTranslation = FVector(0,0,0);
	}

	// Compute a transform from view origin centered world-space to clip space.
	TranslatedViewMatrix = FTranslationMatrix(-PreViewTranslation) * ViewMatrix;
	TranslatedViewProjectionMatrix = TranslatedViewMatrix * ProjectionMatrix;
	InvTranslatedViewProjectionMatrix = TranslatedViewProjectionMatrix.InverseSafe();

	// Derive the view frustum from the view projection matrix.
	GetViewFrustumBounds(ViewFrustum,ViewProjectionMatrix,FALSE);

	// Derive the view's near clipping distance and plane.
	bHasNearClippingPlane = ViewProjectionMatrix.GetFrustumNearPlane(NearClippingPlane);
	NearClippingDistance = Abs(ProjectionMatrix.M[2][2]) > DELTA ? -ProjectionMatrix.M[3][2] / ProjectionMatrix.M[2][2] : 0.0f;

	// Determine whether the view should reverse the cull mode due to a negative determinant.  Only do this for a valid scene
	bReverseCulling = (Family && Family->Scene) ? IsNegativeFloat(ViewMatrix.Determinant()) : FALSE;

	// Setup transformation constants to be used by the graphics hardware to transform device normalized depth samples
	// into world oriented z.
	InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(ProjectionMatrix, GUsesInvertedZ);

	// Init to default values (may be modified by FSceneRenderer constructor).
	RenderTargetX = 0;
	RenderTargetY = 0;
	RenderTargetSizeX = Family->RenderTarget->GetSizeX();
	RenderTargetSizeY = Family->RenderTarget->GetSizeY();
	ScreenPositionScaleBias.Set(
			SizeX / RenderTargetSizeX / +2.0f,
			SizeY / RenderTargetSizeY / -2.0f,
			(SizeY / 2.0f + GPixelCenterOffset + RenderTargetY) / RenderTargetSizeY,
			(SizeX / 2.0f + GPixelCenterOffset + RenderTargetX) / RenderTargetSizeX
		);

#if !CONSOLE
	EditorViewBitflag = InEditorViewBitflag;
#endif

}

FVector4 FSceneView::WorldToScreen(const FVector& WorldPoint) const
{
	return ViewProjectionMatrix.TransformFVector4(FVector4(WorldPoint,1));
}

FVector FSceneView::ScreenToWorld(const FVector4& ScreenPoint) const
{
	return InvViewProjectionMatrix.TransformFVector4(ScreenPoint);
}

UBOOL FSceneView::ScreenToPixel(const FVector4& ScreenPoint,FVector2D& OutPixelLocation) const
{
	if(ScreenPoint.W > 0.0f)
	{
		FLOAT InvW = 1.0f / ScreenPoint.W;
		OutPixelLocation = FVector2D(
			X + (0.5f + ScreenPoint.X * 0.5f * InvW) * SizeX,
			Y + (0.5f - ScreenPoint.Y * 0.5f * InvW) * SizeY
			);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

FVector4 FSceneView::PixelToScreen(FLOAT InX,FLOAT InY,FLOAT Z) const
{
	return FVector4(
		-1.0f + InX / SizeX * +2.0f,
		+1.0f + InY / SizeY * -2.0f,
		Z,
		1
		);
}

/** Transforms a point from the view's world-space into pixel coordinates relative to the view's X,Y. */
UBOOL FSceneView::WorldToPixel(const FVector& WorldPoint,FVector2D& OutPixelLocation) const
{
	const FVector4 ScreenPoint = WorldToScreen(WorldPoint);
	return ScreenToPixel(ScreenPoint, OutPixelLocation);
}

/**  
 * Transforms a point from the view's world-space into the view's screen-space.  
 * Divides the resulting X, Y, Z by W before returning. 
 */
FPlane FSceneView::Project(const FVector& WorldPoint) const
{
	FPlane Result = WorldToScreen(WorldPoint);
	const FLOAT RHW = 1.0f / Result.W;

	return FPlane(Result.X * RHW,Result.Y * RHW,Result.Z * RHW,Result.W);
}

/** 
 * Transforms a point from the view's screen-space into world coordinates
 * multiplies X, Y, Z by W before transforming. 
 */
FVector FSceneView::Deproject(const FPlane& ScreenPoint) const
{
	return InvViewProjectionMatrix.TransformFVector4(FPlane(ScreenPoint.X * ScreenPoint.W,ScreenPoint.Y * ScreenPoint.W,ScreenPoint.Z * ScreenPoint.W,ScreenPoint.W));
}

/** transforms 2D screen coordinates into a 3D world-space origin and direction 
 * @param ScreenPos - screen coordinates in pixels
 * @param WorldOrigin (out) - world-space origin vector
 * @param WorldDirection (out) - world-space direction vector
 */
void FSceneView::DeprojectFVector2D(const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection)
{
	INT		X = appTrunc(ScreenPos.X),
			Y = appTrunc(ScreenPos.Y);

	// Get the eye position and direction of the mouse cursor in two stages (inverse transform projection, then inverse transform view).
	// This avoids the numerical instability that occurs when a view matrix with large translation is composed with a projection matrix
	FMatrix InverseView = ViewMatrix.Inverse();

	// The start of the raytrace is defined to be at mousex,mousey,0 in projection space
	// The end of the raytrace is at mousex, mousey, 0.5 in projection space
	FLOAT ScreenSpaceX = (X - FLOAT(SizeX / 2)) / FLOAT(SizeX / 2);
	FLOAT ScreenSpaceY = (Y - FLOAT(SizeY / 2)) / -FLOAT(SizeY / 2);
	FVector4 RayStartProjectionSpace = FVector4(ScreenSpaceX, ScreenSpaceY,    0, 1.0f);
	FVector4 RayEndProjectionSpace   = FVector4(ScreenSpaceX, ScreenSpaceY, 0.5f, 1.0f);

	// Projection (changing the W coordinate) is not handled by the FMatrix transforms that work with vectors, so multiplications
	// by the projection matrix should use homogenous coordinates (i.e. FPlane).
	FVector4 HGRayStartViewSpace = InvProjectionMatrix.TransformFVector4(RayStartProjectionSpace);
	FVector4 HGRayEndViewSpace   = InvProjectionMatrix.TransformFVector4(RayEndProjectionSpace);
	FVector RayStartViewSpace(HGRayStartViewSpace.X, HGRayStartViewSpace.Y, HGRayStartViewSpace.Z);
	FVector   RayEndViewSpace(HGRayEndViewSpace.X,   HGRayEndViewSpace.Y,   HGRayEndViewSpace.Z);
	// divide vectors by W to undo any projection and get the 3-space coordinate 
	if (HGRayStartViewSpace.W != 0.0f)
	{
		RayStartViewSpace /= HGRayStartViewSpace.W;
	}
	if (HGRayEndViewSpace.W != 0.0f)
	{
		RayEndViewSpace /= HGRayEndViewSpace.W;
	}
	FVector RayDirViewSpace = RayEndViewSpace - RayStartViewSpace;
	RayDirViewSpace = RayDirViewSpace.SafeNormal();

	// The view transform does not have projection, so we can use the standard functions that deal with vectors and normals (normals
	// are vectors that do not use the translational part of a rotation/translation)
	FVector RayStartWorldSpace = InverseView.TransformFVector(RayStartViewSpace);
	FVector RayDirWorldSpace   = InverseView.TransformNormal(RayDirViewSpace);

	// Finally, store the results in the hitcheck inputs.  The start position is the eye, and the end position
	// is the eye plus a long distance in the direction the mouse is pointing.
	out_WorldOrigin = RayStartWorldSpace;
	out_WorldDirection = RayDirWorldSpace.SafeNormal();
}

FSceneViewFamily::FSceneViewFamily(
	const FRenderTarget* InRenderTarget,
	FSceneInterface* InScene,
	EShowFlags InShowFlags,
	FLOAT InCurrentWorldTime,
	FLOAT InDeltaWorldTime,
	FLOAT InCurrentRealTime,
	UBOOL InbRealtimeUpdate,
	UBOOL InbAllowAmbientOcclusion,
	UBOOL InbDeferClear,
	UBOOL InbClearScene,
	UBOOL InbResolveScene,
	FLOAT InGammaCorrection,
	UBOOL InbWriteOpacityToAlpha)
	:
	RenderTarget(InRenderTarget),
	Scene(InScene),
	ShowFlags(InShowFlags),
	CurrentWorldTime(InCurrentWorldTime),
	DeltaWorldTime(InDeltaWorldTime),
	CurrentRealTime(InCurrentRealTime),
	bRealtimeUpdate(InbRealtimeUpdate),
	bAllowAmbientOcclusion(InbAllowAmbientOcclusion),
	bDeferClear(InbDeferClear),
	bClearScene(InbClearScene),
	bResolveScene(InbResolveScene),
	bWriteOpacityToAlpha(InbWriteOpacityToAlpha),
	GammaCorrection(InGammaCorrection)
{
#if CONSOLE
	// Console shader compilers don't set instruction count, 
	// Also various console-specific rendering paths haven't been tested with shader complexity
	check(!(ShowFlags & SHOW_ShaderComplexity));

#else

	// instead of checking GIsGame on rendering thread to see if we allow this flag to be disabled 
	// we force it on in the game thread.
	if (GIsGame)
	{
		ShowFlags |= SHOW_LOD;
	}

#endif
}

FSceneViewFamilyContext::~FSceneViewFamilyContext()
{
	// Cleanup the views allocated for this view family.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		delete Views(ViewIndex);
	}
}

FLightEnvironmentSceneInfo& FScene::GetLightEnvironmentSceneInfo(const ULightEnvironmentComponent* LightEnvironment)
{
	FLightEnvironmentSceneInfo* LightEnvironmentSceneInfo = LightEnvironments.Find(LightEnvironment);
	if(!LightEnvironmentSceneInfo)
	{
		LightEnvironmentSceneInfo = &LightEnvironments.Set(LightEnvironment,FLightEnvironmentSceneInfo());
	}
	check(LightEnvironmentSceneInfo);
	return *LightEnvironmentSceneInfo;
}

void FScene::AddPrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_AddScenePrimitiveRenderThreadTime);

	// Allocate an entry in the primitives array for the primitive, and initialize its compact scene info.
	PrimitiveSceneInfo->Id = Primitives.Add().Index;
	Primitives(PrimitiveSceneInfo->Id) = PrimitiveSceneInfo;

	// Add the primitive to its shadow parent's linked list of children.
	PrimitiveSceneInfo->LinkShadowParent();

	// Create any RenderThreadResources required.
	PrimitiveSceneInfo->Proxy->CreateRenderThreadResources();

	// Add the primitive to the scene.
	PrimitiveSceneInfo->AddToScene();
	
	if(PrimitiveSceneInfo->LightEnvironment)
	{
		// Add the primitive to the light environment's primitive list.
		FLightEnvironmentSceneInfo& LightEnvironmentSceneInfo = GetLightEnvironmentSceneInfo(PrimitiveSceneInfo->LightEnvironment);
		LightEnvironmentSceneInfo.Primitives.AddItem(PrimitiveSceneInfo);
	}
}

/**
 * Verifies that a component is added to the proper scene
 *
 * @param Component Component to verify
 * @param World World who's scene the primitive is being attached to
 */
FORCEINLINE static void VerifyProperPIEScene(UPrimitiveComponent* Component, UWorld* World)
{
#if CHECK_FOR_PIE_PRIMITIVE_ATTACH_SCENE_MISMATCH
	checkf(Component->GetOuter() == UObject::GetTransientPackage() || 
		(Component->GetOutermost()->GetName().StartsWith(PLAYWORLD_PACKAGE_PREFIX) == 
		World->GetOutermost()->GetName().StartsWith(PLAYWORLD_PACKAGE_PREFIX)),
		TEXT("The component %s was added to the wrong world's scene (due to PIE). The callstack should tell you why"), 
		*Component->GetFullName()
		);
#endif
}

void FScene::AddPrimitive(UPrimitiveComponent* Primitive)
{
	checkf(!Primitive->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Primitive->GetFullName());

#if TRACK_GAME_THREAD_RENDER_MALLOCS
	GMalloc->Exec( TEXT("BEGINTRACKINGTHREAD") );
#endif

	// Save the world transform for next time the primitive is added to the scene
	FLOAT DeltaTime = GWorld->GetTimeSeconds() - Primitive->LastSubmitTime;
	if ( DeltaTime < -0.0001f || Primitive->LastSubmitTime < 0.0001f )
	{
		// Time was reset?
		Primitive->LastSubmitTime = GWorld->GetTimeSeconds();
	}
	else if ( DeltaTime > 0.0001f )
	{
		// First call for the new frame?
		Primitive->LastSubmitTime = GWorld->GetTimeSeconds();
	}

	// Create the primitive's scene proxy.
	FPrimitiveSceneProxy* Proxy = Primitive->CreateSceneProxy();
	if(!Proxy)
	{
#if TRACK_GAME_THREAD_RENDER_MALLOCS
		GMalloc->Exec( TEXT("ENDTRACKINGTHREAD") );
#endif
		// Primitives which don't have a proxy are irrelevant to the scene manager.
		return;
	}

	// Cache the primitive's initial transform.
	Proxy->SetTransform(Primitive->LocalToWorld,Primitive->LocalToWorldDeterminant);

	// Create the primitive render info.
	FPrimitiveSceneInfo* PrimitiveSceneInfo = new FPrimitiveSceneInfo(Primitive,Proxy,this);
	Primitive->SceneInfo = PrimitiveSceneInfo;

	INC_DWORD_STAT_BY( Proxy->GetMemoryStatType(), Proxy->GetMemoryFootprint() + PrimitiveSceneInfo->GetMemoryFootprint() );
	INC_DWORD_STAT_BY( STAT_GameToRendererMallocTotal, Proxy->GetMemoryFootprint() + PrimitiveSceneInfo->GetMemoryFootprint() );

	// Verify the primitive is valid (this will compile away to a nop without CHECK_FOR_PIE_PRIMITIVE_ATTACH_SCENE_MISMATCH)
	VerifyProperPIEScene(Primitive, World);

#if !CONSOLE
	// Make sure deferred shader compiling is completed before exposing shader maps to the rendering thread
	GShaderCompilingThreadManager->FinishDeferredCompilation();
#endif

	// Send a command to the rendering thread to add the primitive to the scene.
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddPrimitiveCommand,
		FScene*,Scene,this,
		FPrimitiveSceneInfo*,PrimitiveSceneInfo,PrimitiveSceneInfo,
		{
			Scene->AddPrimitiveSceneInfo_RenderThread(PrimitiveSceneInfo);
		});

#if TRACK_GAME_THREAD_RENDER_MALLOCS
	GMalloc->Exec( TEXT("ENDTRACKINGTHREAD") );
#endif
}

/** The rendering thread side of UpdatePrimitiveTransform. */
class FUpdatePrimitiveTransformCommand
{
public:

	/** Initialization constructor. */
	FUpdatePrimitiveTransformCommand(UPrimitiveComponent* Primitive, UINT InFrameNumber):
		PrimitiveSceneInfo(Primitive->SceneInfo),
		LocalToWorld(Primitive->LocalToWorld),
		WorldToLocal(Primitive->LocalToWorld.InverseSafe()),
		Bounds(Primitive->Bounds),
		LocalToWorldDeterminant(Primitive->LocalToWorldDeterminant),
		FrameNumber(InFrameNumber)
	{}

	/** Called in the rendering thread to apply the updated transform. */
	void Apply()
	{
		// Remove the primitive from the scene at its old location
		// (note that the octree update relies on the bounds not being modified yet).
		PrimitiveSceneInfo->RemoveFromScene();

		// Update the primitive's motion blur information.
		FScene::UpdatePrimitiveMotionBlur(PrimitiveSceneInfo, LocalToWorld, FALSE, FrameNumber);		

		// Update the primitive's transform.
		PrimitiveSceneInfo->Proxy->SetTransform(LocalToWorld,LocalToWorldDeterminant);

		// Update the primitive's bounds.
		PrimitiveSceneInfo->Bounds = Bounds;

		// If the primitive has static mesh elements, it should have returned TRUE from ShouldRecreateProxyOnUpdateTransform!
		check(!PrimitiveSceneInfo->StaticMeshes.Num());

		// Re-add the primitive to the scene with the new transform.
		PrimitiveSceneInfo->AddToScene();
	}

private:
	
	FPrimitiveSceneInfo* PrimitiveSceneInfo;
	FMatrix LocalToWorld;
	FMatrix WorldToLocal;
	FBoxSphereBounds Bounds;
	FLOAT LocalToWorldDeterminant;
	UINT FrameNumber;
};

void FScene::UpdatePrimitiveTransform(UPrimitiveComponent* Primitive)
{
	// Save the world transform for next time the primitive is added to the scene
	FLOAT DeltaTime = GWorld->GetTimeSeconds() - Primitive->LastSubmitTime;
	if ( DeltaTime < -0.0001f || Primitive->LastSubmitTime < 0.0001f )
	{
		// Time was reset?
		Primitive->LastSubmitTime = GWorld->GetTimeSeconds();
	}
	else if ( DeltaTime > 0.0001f )
	{
		// First call for the new frame?
		Primitive->LastSubmitTime = GWorld->GetTimeSeconds();
	}

	if(Primitive->SceneInfo)
	{
		// Check if the primitive needs to recreate its proxy for the transform update.
		if(Primitive->ShouldRecreateProxyOnUpdateTransform())
		{
			// Re-add the primitive from scratch to recreate the primitive's proxy.
			RemovePrimitive(Primitive, TRUE);
			AddPrimitive(Primitive);
		}
		else
		{
			// Send a message to rendering thread to update the proxy's cached transforms.
			FUpdatePrimitiveTransformCommand Command(Primitive, GFrameNumber);
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				UpdateTransformCommand,
				FUpdatePrimitiveTransformCommand,Command,Command,
				{
					SCOPE_CYCLE_COUNTER(STAT_UpdatePrimitiveTransformRenderThreadTime);
					Command.Apply();
				});
		}
	}
	else
	{
		// If the primitive doesn't have a scene info object yet, it must be added from scratch.
		AddPrimitive(Primitive);
	}
}

void FScene::RemovePrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_RemoveScenePrimitiveTime);
	if(PrimitiveSceneInfo->LightEnvironment)
	{
		FLightEnvironmentSceneInfo& LightEnvironmentSceneInfo = GetLightEnvironmentSceneInfo(PrimitiveSceneInfo->LightEnvironment);

		// Remove the primitive from its light environment's primitive list.
		LightEnvironmentSceneInfo.Primitives.RemoveItem(PrimitiveSceneInfo);

		// If the light environment scene info is now empty, free it.
		if(!LightEnvironmentSceneInfo.Lights.Num() && !LightEnvironmentSceneInfo.Primitives.Num())
		{
			LightEnvironments.Remove(PrimitiveSceneInfo->LightEnvironment);
		}
	}

	// Update the primitive's motion blur information.
	// FrameNumber = 0 as for removing the paramater is not needed
	UpdatePrimitiveMotionBlur(PrimitiveSceneInfo, FMatrix::Identity, TRUE, 0);		

	Primitives.Remove(PrimitiveSceneInfo->Id);

	// Unlink the primitive from its shadow parent.
	PrimitiveSceneInfo->UnlinkShadowParent();

	// Remove the primitive from the scene.
	PrimitiveSceneInfo->RemoveFromScene();

	// Delete the primitive scene proxy.
	delete PrimitiveSceneInfo->Proxy;
	PrimitiveSceneInfo->Proxy = NULL;
}

void FScene::RemovePrimitive( UPrimitiveComponent* Primitive, UBOOL bWillReattach )
{
	FPrimitiveSceneInfo* PrimitiveSceneInfo = Primitive->SceneInfo;

	if(PrimitiveSceneInfo)
	{
		// Disassociate the primitive's render info.
		Primitive->SceneInfo = NULL;

		// Send a command to the rendering thread to remove the primitive from the scene.
		if ( !bWillReattach )
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FRemovePrimitiveCommand,
				FScene*,Scene,this,
				UPrimitiveComponent*,Primitive,Primitive,
			{
				// clear hit mask before it gets removed
				Scene->ClearHitMask( Primitive );
			});
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRemovePrimitiveCommand,
			FScene*,Scene,this,
			FPrimitiveSceneInfo*,PrimitiveSceneInfo,PrimitiveSceneInfo,
			{
				Scene->RemovePrimitiveSceneInfo_RenderThread(PrimitiveSceneInfo);
			});

		// Begin the deferred cleanup of the primitive scene info.
		BeginCleanup(PrimitiveSceneInfo);
	}
}

void FScene::AddLightSceneInfo_RenderThread(FLightSceneInfo* LightSceneInfo)
{
    SCOPE_CYCLE_COUNTER(STAT_AddSceneLightTime);

    // Add the light to the light list.
    LightSceneInfo->Id = Lights.AddItem(FLightSceneInfoCompact(LightSceneInfo));
	const FLightSceneInfoCompact& LightSceneInfoCompact = Lights(LightSceneInfo->Id);

	// Add the light to the scene.
	LightSceneInfo->AddToScene();
}

void FScene::AddLight(ULightComponent* Light)
{
	// Whether the light has entirely precomputed lighting and therefore doesn't need to be added to the world.
	UBOOL bHasEntirelyPrecomputedLighting = FALSE;	
	if( Light->UseDirectLightMap && Light->HasStaticLighting() )
	{
		if( !Light->bCanAffectDynamicPrimitivesOutsideDynamicChannel )
		{
			// The only way for this light to affect dynamic primitives is if it is in the dynamic channel.
			bHasEntirelyPrecomputedLighting = !Light->LightingChannels.Dynamic;
		}
		else
		{
			// Create a lighting channel that has all channels except BSP, Static and CompositeDynamic set.
			FLightingChannelContainer PotentiallyNonStatic;
			PotentiallyNonStatic.SetAllChannels();
			PotentiallyNonStatic.BSP				= FALSE;
			PotentiallyNonStatic.Static				= FALSE;
			PotentiallyNonStatic.CompositeDynamic	= FALSE;

			// A light with a light channel not overlapping with the "potentially non static" channel is only
			// affecting static objects
			bHasEntirelyPrecomputedLighting = !Light->LightingChannels.OverlapsWith( PotentiallyNonStatic );
		}
	}

	UBOOL bShouldAddLight = TRUE;
	// Game-only performance optimizations/ scalability options.
	if( GIsGame )
	{
		// On mobile platforms, we always want all lights available so we can use dynamic per-vertex specular for light maps primitives
#if WITH_MOBILE_RHI
		if( !GUsingMobileRHI )
#endif
		{
			// Don't add lights only affecting static objects to the world during gameplay as all the lighting
			// and shadowing is precalculated and we don't need to consider them.
			if( bHasEntirelyPrecomputedLighting )
			{
				bShouldAddLight = FALSE;
			}

			// If dynamic lights are globally disabled, only add the ones affecting light environments and ones used by cutscenes.
			if( !GSystemSettings.bAllowDynamicLights && !Light->LightEnvironment && !Light->bCanAffectDynamicPrimitivesOutsideDynamicChannel )
			{
				bShouldAddLight = FALSE;
			}
		}
	}
	
	if( bShouldAddLight )
	{
		// Create the light scene info.
		FLightSceneInfo* LightSceneInfo = Light->CreateSceneInfo();
		Light->SceneInfo = LightSceneInfo;
		check(LightSceneInfo);

		// Downgrade modulate-better shadows to modulated if specified by the level
		if (LightSceneInfo->LightShadowMode == LightShadow_ModulateBetter 
			&& !World->GetWorldInfo()->GetModulateBetterShadowsAllowed())
		{
			LightSceneInfo->LightShadowMode = LightShadow_Modulate;
		}
		
		INC_DWORD_STAT(STAT_SceneLights);

		// Send a command to the rendering thread to add the light to the scene.
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FAddLightCommand,
			FScene*,Scene,this,
			FLightSceneInfo*,LightSceneInfo,LightSceneInfo,
			{
				Scene->AddLightSceneInfo_RenderThread(LightSceneInfo);
			});
	}
}

struct FUpdateLightTransformParameters
{
	FMatrix WorldToLight;
	FMatrix LightToWorld;
	FVector4 Position;
};

void FScene::UpdateLightTransform_RenderThread(FLightSceneInfo* LightSceneInfo, const FUpdateLightTransformParameters& Parameters)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateSceneLightTime);
	if( LightSceneInfo )
	{
		// Don't remove directional lights when their transform changes as nothing in RemoveFromScene() depends on their transform
		if (!(LightSceneInfo->LightType == LightType_Directional || LightSceneInfo->LightType == LightType_DominantDirectional))
		{
			// Remove the light from the scene.
			LightSceneInfo->RemoveFromScene();
		}

		// Update the light's transform and position.
		LightSceneInfo->WorldToLight = Parameters.WorldToLight;
		LightSceneInfo->LightToWorld = Parameters.LightToWorld;
		LightSceneInfo->Position = Parameters.Position;

		// Also update the LightSceneInfoCompact
		if( LightSceneInfo->Id != INDEX_NONE )
		{
			LightSceneInfo->Scene->Lights(LightSceneInfo->Id).Init(LightSceneInfo);

			// Don't re-add directional lights when their transform changes as nothing in AddToScene() depends on their transform
			if (!(LightSceneInfo->LightType == LightType_Directional || LightSceneInfo->LightType == LightType_DominantDirectional))
			{
				// Add the light to the scene at its new location.
				LightSceneInfo->AddToScene();
			}
		}
	}
}

void FScene::UpdateLightTransform(ULightComponent* Light)
{
	FUpdateLightTransformParameters Parameters;
	Parameters.LightToWorld = Light->LightToWorld;
	Parameters.WorldToLight = Light->WorldToLight;
	Parameters.Position = Light->GetPosition();
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		UpdateLightTransform,
		FScene*,Scene,this,
		FLightSceneInfo*,LightSceneInfo,Light->SceneInfo,
		FUpdateLightTransformParameters,Parameters,Parameters,
		{
			Scene->UpdateLightTransform_RenderThread(LightSceneInfo, Parameters);
		});
}

/** 
 * Updates the color and brightness of a light which has already been added to the scene. 
 *
 * @param Light - light component to update
 */
void FScene::UpdateLightColorAndBrightness(ULightComponent* Light)
{
	FLinearColor NewColor = FLinearColor(Light->LightColor) * Light->Brightness;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		UpdateLightColorAndBrightness,
		FLightSceneInfo*,LightSceneInfo,Light->SceneInfo,
		FScene*,Scene,this,
		FLinearColor,Color,NewColor,
		{
			if( LightSceneInfo )
			{
				LightSceneInfo->Color = Color;

				// Also update the LightSceneInfoCompact
				if( LightSceneInfo->Id != INDEX_NONE )
				{
					Scene->Lights( LightSceneInfo->Id ).Color = Color;
				}
			}
		});
}

void FScene::UpdatePreviewSkyLightColor(const FLinearColor& NewColor)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdatePreviewSkyLightColor,
		FScene*,Scene,this,
		FLinearColor,Color,NewColor,
	{
		Scene->PreviewSkyLightColor = Color;
	});
}

void FScene::RemoveLightSceneInfo_RenderThread(FLightSceneInfo* LightSceneInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_RemoveSceneLightTime);

	// Remove the light from the scene.
	LightSceneInfo->RemoveFromScene();

	// Remove the light from the lights list.
	Lights.Remove(LightSceneInfo->Id);

	// Free the light scene info.
	delete LightSceneInfo;
}

void FScene::RemoveLight(ULightComponent* Light)
{
	FLightSceneInfo* LightSceneInfo = Light->SceneInfo;

	if(LightSceneInfo)
	{
		DEC_DWORD_STAT(STAT_SceneLights);

		// Disassociate the primitive's render info.
		Light->SceneInfo = NULL;

		// Send a command to the rendering thread to remove the light from the scene.
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRemoveLightCommand,
			FScene*,Scene,this,
			FLightSceneInfo*,LightSceneInfo,LightSceneInfo,
			{
				Scene->RemoveLightSceneInfo_RenderThread(LightSceneInfo);
			});
	}
}

void FScene::AddHeightFog(UHeightFogComponent* FogComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddFogCommand,
		FScene*,Scene,this,
		FHeightFogSceneInfo,HeightFogSceneInfo,FHeightFogSceneInfo(FogComponent),
		{
			// Create a FHeightFogSceneInfo for the component in the scene's fog array.
			new(Scene->Fogs) FHeightFogSceneInfo(HeightFogSceneInfo);

			// Sort the scene's fog array by height.
			Sort<USE_COMPARE_CONSTREF(FHeightFogSceneInfo,SceneCore)>(&Scene->Fogs(0),Scene->Fogs.Num());
		});
}

void FScene::RemoveHeightFog(UHeightFogComponent* FogComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveFogCommand,
		FScene*,Scene,this,
		UHeightFogComponent*,FogComponent,FogComponent,
		{
			// Remove the given component's FHeightFogSceneInfo from the scene's fog array.
			for(INT FogIndex = 0;FogIndex < Scene->Fogs.Num();FogIndex++)
			{
				if(Scene->Fogs(FogIndex).Component == FogComponent)
				{
					Scene->Fogs.Remove(FogIndex);
					break;
				}
			}
		});
}

void FScene::AddExponentialHeightFog(UExponentialHeightFogComponent* FogComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddFogCommand,
		FScene*,Scene,this,
		FExponentialHeightFogSceneInfo,HeightFogSceneInfo,FExponentialHeightFogSceneInfo(FogComponent),
		{
			// Create a FExponentialHeightFogSceneInfo for the component in the scene's fog array.
			new(Scene->ExponentialFogs) FExponentialHeightFogSceneInfo(HeightFogSceneInfo);
		});
}

void FScene::RemoveExponentialHeightFog(UExponentialHeightFogComponent* FogComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveFogCommand,
		FScene*,Scene,this,
		UExponentialHeightFogComponent*,FogComponent,FogComponent,
		{
			// Remove the given component's FExponentialHeightFogSceneInfo from the scene's fog array.
			for(INT FogIndex = 0;FogIndex < Scene->ExponentialFogs.Num();FogIndex++)
			{
				if(Scene->ExponentialFogs(FogIndex).Component == FogComponent)
				{
					Scene->ExponentialFogs.Remove(FogIndex);
					break;
				}
			}
		});
}

void FScene::AddWindSource(UWindDirectionalSourceComponent* WindComponent)
{
	FWindSourceSceneProxy* SceneProxy = WindComponent->CreateSceneProxy();
	WindComponent->SceneProxy = SceneProxy;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddWindSourceCommand,
		FScene*,Scene,this,
		FWindSourceSceneProxy*,SceneProxy,SceneProxy,
		{
			Scene->WindSources.AddItem(SceneProxy);
		});
}

void FScene::RemoveWindSource(UWindDirectionalSourceComponent* WindComponent)
{
	FWindSourceSceneProxy* SceneProxy = WindComponent->SceneProxy;
	WindComponent->SceneProxy = NULL;

	if(SceneProxy)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRemoveWindSourceCommand,
			FScene*,Scene,this,
			FWindSourceSceneProxy*,SceneProxy,SceneProxy,
			{
				Scene->WindSources.RemoveItem(SceneProxy);

				delete SceneProxy;
			});
	}
}

const TArray<FWindSourceSceneProxy*>& FScene::GetWindSources_RenderThread() const
{
	checkSlow(IsInRenderingThread());
	return WindSources;
}

FVector4 FScene::GetWindParameters(const FVector& Position) const
{
	INT NumActiveWindSources = 0;
	FVector4 AccumulatedDirectionAndSpeed(0,0,0,0);
	FLOAT TotalWeight = 0.0f;
	for (INT i = 0; i < WindSources.Num(); i++)
	{
		FVector4 CurrentDirectionAndSpeed;
		FLOAT Weight;
		const FWindSourceSceneProxy* CurrentSource = WindSources(i);
		if (CurrentSource->GetWindParameters(Position, CurrentDirectionAndSpeed, Weight))
		{
			AccumulatedDirectionAndSpeed.X += CurrentDirectionAndSpeed.X * Weight;
			AccumulatedDirectionAndSpeed.Y += CurrentDirectionAndSpeed.Y * Weight;
			AccumulatedDirectionAndSpeed.Z += CurrentDirectionAndSpeed.Z * Weight;
			AccumulatedDirectionAndSpeed.W += CurrentDirectionAndSpeed.W * Weight;
			TotalWeight += Weight;
			NumActiveWindSources++;
		}
	}

	if (TotalWeight > 0)
	{
		AccumulatedDirectionAndSpeed.X /= TotalWeight;
		AccumulatedDirectionAndSpeed.Y /= TotalWeight;
		AccumulatedDirectionAndSpeed.Z /= TotalWeight;
		AccumulatedDirectionAndSpeed.W /= TotalWeight;
	}

	// Normalize averaged direction and speed
	return NumActiveWindSources > 0 ? AccumulatedDirectionAndSpeed / NumActiveWindSources : FVector4(0,0,1,0);
}

/**
* Adds a default FFogVolumeConstantDensitySceneInfo to UPrimitiveComponent pair to the Scene's FogVolumes map.
*/
void FScene::AddFogVolume(const UPrimitiveComponent* MeshComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FAddFogVolumeCommand,
		FScene*,Scene,this,
		FFogVolumeDensitySceneInfo*,FogVolumeSceneInfo,new FFogVolumeConstantDensitySceneInfo(),
		const UPrimitiveComponent*,MeshComponent,MeshComponent,
	{
		Scene->FogVolumes.Set(MeshComponent, FogVolumeSceneInfo);
	});
}

/**
 * Adds a FFogVolumeDensitySceneInfo to UPrimitiveComponent pair to the Scene's FogVolumes map.
 */
void FScene::AddFogVolume(const UFogVolumeDensityComponent* FogVolumeComponent, const UPrimitiveComponent* MeshComponent)
{
	// Allocate the FFogVolumeDensitySceneInfo inside this FSceneInterface function so that there won't be a memory leak when using a null interface
	FFogVolumeDensitySceneInfo* FogVolumeSceneInfo = FogVolumeComponent->CreateFogVolumeDensityInfo(MeshComponent);
	if( FogVolumeSceneInfo )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FAddFogVolumeCommand,
			FScene*,Scene,this,
			FFogVolumeDensitySceneInfo*,FogVolumeSceneInfo,FogVolumeSceneInfo,
			const UPrimitiveComponent*,MeshComponent,MeshComponent,
		{
			Scene->FogVolumes.Set(MeshComponent, FogVolumeSceneInfo);
		});
	}
}

/**
* Removes an entry by UPrimitiveComponent from the Scene's FogVolumes map.
*/
void FScene::RemoveFogVolume(const UPrimitiveComponent* MeshComponent)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveFogVolumeCommand,
		FScene*,Scene,this,
		const UPrimitiveComponent*,MeshComponent,MeshComponent,
	{
		FFogVolumeDensitySceneInfo** FogVolumeInfoRef = Scene->FogVolumes.Find(MeshComponent);
		if (FogVolumeInfoRef)
		{
			delete *FogVolumeInfoRef;
		}
		Scene->FogVolumes.Remove(MeshComponent);
	});
}

/** Adds an image based reflection component to the scene. */
void FScene::AddImageReflection(const UActorComponent* Component, UTexture2D* InReflectionTexture, FLOAT ReflectionScale, const FLinearColor& InReflectionColor, UBOOL bInTwoSided, UBOOL bEnabled)
{
	check(Component && (InReflectionTexture || Component->IsA(ULightComponent::StaticClass())));
	struct FImageUpdateParams
	{
		const UActorComponent* Component;
		FImageReflectionSceneInfo* SceneInfo;
		UTexture2D* ReflectionTexture;
		FIncomingTextureArrayDataEntry* TextureEntry;
	};

	FImageUpdateParams Params;
	Params.Component = Component;
	Params.SceneInfo = new FImageReflectionSceneInfo(Component, InReflectionTexture, ReflectionScale, InReflectionColor, bInTwoSided, bEnabled);
	Params.ReflectionTexture = InReflectionTexture;
	Params.TextureEntry = InReflectionTexture ? new FIncomingTextureArrayDataEntry(InReflectionTexture) : NULL;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddReflectionCommand,
		FScene*,Scene,this,
		FImageUpdateParams,Params,Params,
	{
		if (Params.TextureEntry)
		{
			Scene->ImageReflectionTextureArray.AddTexture2D(Params.ReflectionTexture, Params.TextureEntry);
		}
		Scene->ImageReflections.Set(Params.Component, Params.SceneInfo);
	});
}

/** Updates an image based reflection's transform. */
void FScene::UpdateImageReflection(const UActorComponent* Component, UTexture2D* InReflectionTexture, FLOAT ReflectionScale, const FLinearColor& InReflectionColor, UBOOL bInTwoSided, UBOOL bEnabled)
{
	check(Component && (InReflectionTexture || Component->IsA(ULightComponent::StaticClass())));
	struct FImageUpdateParams
	{
		const UActorComponent* Component;
		FImageReflectionSceneInfo* SceneInfo;
		UTexture2D* ReflectionTexture;
	};

	FImageUpdateParams Params;
	Params.Component = Component;
	Params.SceneInfo = new FImageReflectionSceneInfo(Component, InReflectionTexture, ReflectionScale, InReflectionColor, bInTwoSided, bEnabled);
	Params.ReflectionTexture = InReflectionTexture;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddReflectionCommand,
		FScene*,Scene,this,
		FImageUpdateParams,Params,Params,
	{
		FImageReflectionSceneInfo** ReflectionSceneInfoRef = Scene->ImageReflections.Find(Params.Component);
		if (ReflectionSceneInfoRef)
		{
			check((*ReflectionSceneInfoRef)->ReflectionTexture == Params.SceneInfo->ReflectionTexture);
			delete *ReflectionSceneInfoRef;
		}
		Scene->ImageReflections.Set(Params.Component, Params.SceneInfo);
	});
}

/** Updates the image reflection texture array. */
void FScene::UpdateImageReflectionTextureArray(UTexture2D* Texture)
{
	check(Texture);
	FIncomingTextureArrayDataEntry* NewEntry = new FIncomingTextureArrayDataEntry(Texture);
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FRemoveReflectionCommand,
		FScene*,Scene,this,
		UTexture2D*,Texture,Texture,
		FIncomingTextureArrayDataEntry*,NewEntry,NewEntry,
	{
		Scene->ImageReflectionTextureArray.UpdateTexture2D(Texture, NewEntry);
	});
}

/** Removes an image based reflection component from the scene. */
void FScene::RemoveImageReflection(const UActorComponent* Component)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveReflectionCommand,
		FScene*,Scene,this,
		const UActorComponent*,Component,Component,
	{
		FImageReflectionSceneInfo** ReflectionSceneInfoRef = Scene->ImageReflections.Find(Component);
		if (ReflectionSceneInfoRef)
		{
			if ((*ReflectionSceneInfoRef)->ReflectionTexture)
			{
				Scene->ImageReflectionTextureArray.RemoveTexture2D((*ReflectionSceneInfoRef)->ReflectionTexture);
			}
			delete *ReflectionSceneInfoRef;
		}
		Scene->ImageReflections.Remove(Component);
	});
}

/** Adds an image reflection shadow plane component to the scene. */
void FScene::AddImageReflectionShadowPlane(const UActorComponent* Component, const FPlane& InPlane)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FAddReflectionShadowPlaneCommand,
		FScene*,Scene,this,
		const UActorComponent*,Component,Component,
		FPlane,Plane,InPlane,
	{
		Scene->ImageReflectionShadowPlanes.Set(Component, Plane);
	});
}

/** Removes an image reflection shadow plane component from the scene. */
void FScene::RemoveImageReflectionShadowPlane(const UActorComponent* Component)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveReflectionShadowPlaneCommand,
		FScene*,Scene,this,
		const UActorComponent*,Component,Component,
	{
		Scene->ImageReflectionShadowPlanes.Remove(Component);
	});
}

/** Sets scene image reflection environment parameters. */
void FScene::SetImageReflectionEnvironmentTexture(const UTexture2D* NewTexture, const FLinearColor& Color, FLOAT Rotation)
{
	const FVector4 ColorAndRotation(FVector(Color) * Color.A, Rotation);
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FSetEnvTextureCommand,
		FScene*,Scene,this,
		const UTexture2D*,NewTexture,NewTexture,
		FVector4,ColorAndRotation,ColorAndRotation,
	{
		Scene->ImageReflectionEnvironmentTexture = NewTexture;
		Scene->EnvironmentColor = ColorAndRotation;
		Scene->EnvironmentRotation = ColorAndRotation.W;
	});
}

/** Sets the scene in a state where it will not reallocate the image reflection texture array. */
void FScene::BeginPreventIRReallocation()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FBeginPreventIRReallocationCommand,
		FScene*,Scene,this,
	{
		Scene->ImageReflectionTextureArray.BeginPreventReallocation();
	});
}

/** Restores the scene's default state where it will reallocate the image reflection texture array as needed. */
void FScene::EndPreventIRReallocation()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FEndPreventIRReallocationCommand,
		FScene*,Scene,this,
	{
		Scene->ImageReflectionTextureArray.EndPreventReallocation();
	});
}

/**
 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
 *
 * Render thread version of function.
 * 
 * @param	Primitive				Primitive to retrieve interacting lights for
 * @param	RelevantLights	[out]	Array of lights interacting with primitive
 */
void FScene::GetRelevantLights_RenderThread( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const
{
	check( Primitive );
	check( RelevantLights );
	if( Primitive->SceneInfo )
	{
		for( const FLightPrimitiveInteraction* Interaction=Primitive->SceneInfo->LightList; Interaction; Interaction=Interaction->GetNextLight() )
		{
			RelevantLights->AddItem( Interaction->GetLight()->LightComponent );
		}
	}
}

/**
 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
 *
 * @param	Primitive				Primitive to retrieve interacting lights for
 * @param	RelevantLights	[out]	Array of lights interacting with primitive
 */
void FScene::GetRelevantLights( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const
{
	if( Primitive && RelevantLights )
	{
		// Add interacting lights to the array.
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FGetRelevantLightsCommand,
			const FScene*,Scene,this,
			UPrimitiveComponent*,Primitive,Primitive,
			TArray<const ULightComponent*>*,RelevantLights,RelevantLights,
			{
				Scene->GetRelevantLights_RenderThread( Primitive, RelevantLights );
			});

		// We need to block the main thread as the rendering thread needs to finish modifying the array before we can continue.
		FlushRenderingCommands();
	}
}

/**
 * Create the scene capture info for a capture component and add it to the scene
 * @param CaptureComponent - component to add to the scene 
 */
void FScene::AddSceneCapture(USceneCaptureComponent* CaptureComponent)
{
	check(CaptureComponent);
	check(CaptureComponent->CaptureInfo == NULL);

	// create a new scene capture probe
	FSceneCaptureProbe* SceneProbe = CaptureComponent->CreateSceneCaptureProbe();
	// add it to the scene
	if( SceneProbe )
	{
		// Create a new capture info for the component
		FCaptureSceneInfo* CaptureInfo = new FCaptureSceneInfo(CaptureComponent,SceneProbe);

        // Add the game thread copy of the scene capture info
		CaptureInfo->GameThreadId = SceneCapturesGameThread.AddItem(CaptureInfo);

		// Copy the post-process scene proxies that were created during the capture component's attach phase.
		SceneProbe->SetPostProcessProxies(CaptureComponent->PostProcessProxies);
		
		// Send a command to the rendering thread to add the capture to the scene.
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FAddSceneCaptureCommand,
			FScene*,Scene,this,
			FCaptureSceneInfo*,CaptureInfo,CaptureInfo,
		{
			// add the newly created capture info to the list of captures in the scene
			CaptureInfo->AddToScene(Scene);
		});
	}
}

/**
 * Remove the scene capture info for a capture component from the scene
 * @param CaptureComponent - component to remove from the scene 
 */
void FScene::RemoveSceneCapture(USceneCaptureComponent* CaptureComponent)
{
	check(CaptureComponent);

	FCaptureSceneInfo* CaptureInfo = CaptureComponent->CaptureInfo;
    if( CaptureInfo )
	{
		// Disassociate the component's capture info.
		CaptureComponent->CaptureInfo = NULL;

		// Remove the game thread copy of the SceneCaptureInfo
		if (CaptureInfo->GameThreadId != INDEX_NONE)
		{
			SceneCapturesGameThread.Remove(CaptureInfo->GameThreadId);
		}

		// Send a command to the rendering thread to remove the capture from the scene.
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRemoveCaptureCommand,
			FScene*,Scene,this,
			FCaptureSceneInfo*,CaptureInfo,CaptureInfo,
		{
			// render thread should be done w/ it at this point. safe to destroy 
			delete CaptureInfo;
		});
	}
}

/**
 * Adds a fluidsurface to the scene (gamethread)
 *
 * @param FluidComponent - component to add to the scene 
 */
void FScene::AddFluidSurface(UFluidSurfaceComponent* FluidComponent)
{
	FluidSurfacesGameThread.AddItem( FluidComponent );

	const FFluidGPUResource* FluidResource = FluidComponent->GetFluidGPUResource();
	if (FluidResource)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FAddFluidSurfaceCommand,
			FScene*,Scene,this,
			const UFluidSurfaceComponent*,FluidComponent,FluidComponent,
			const FFluidGPUResource*,FluidResource,FluidResource,
		{
			Scene->FluidSurfacesRenderThread.Set(FluidComponent, FluidResource);
		});
	}
}

/**
 * Removes a fluidsurface from the scene (gamethread)
 *
 * @param CaptureComponent - component to remove from the scene 
 */
void FScene::RemoveFluidSurface(UFluidSurfaceComponent* FluidComponent)
{
	FluidSurfacesGameThread.RemoveItem( FluidComponent );

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveFluidSurfaceCommand,
		FScene*,Scene,this,
		const UFluidSurfaceComponent*,FluidComponent,FluidComponent,
	{
		Scene->FluidSurfacesRenderThread.Remove(FluidComponent);
	});
}

/**
 * Retrieves a pointer to the fluidsurface container.
 * @return TSparseArray pointer, or NULL if the scene doesn't support fluidsurfaces.
 **/
const TArray<UFluidSurfaceComponent*>* FScene::GetFluidSurfaces()
{
	checkSlow(IsInGameThread());
	return &FluidSurfacesGameThread;
}

/** Returns a pointer to the first fluid surface detail normal texture in the scene. */
const FTexture2DRHIRef* FScene::GetFluidDetailNormal() const
{
	checkSlow(IsInRenderingThread());
	for (TMap<const UFluidSurfaceComponent*, const FFluidGPUResource*>::TConstIterator It(FluidSurfacesRenderThread); It; ++It)
	{
		//@todo - allow the material to specify which fluid surface to use
		return &(It.Value()->GetNormalTexture());
	}

	return NULL;
}

/** Sets the precomputed visibility handler for the scene, or NULL to clear the current one. */
void FScene::SetPrecomputedVisibility(const FPrecomputedVisibilityHandler* PrecomputedVisibilityHandler)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdatePrecomputedVisibility,
		FScene*,Scene,this,
		const FPrecomputedVisibilityHandler*,PrecomputedVisibilityHandler,PrecomputedVisibilityHandler,
	{
		Scene->PrecomputedVisibilityHandler = PrecomputedVisibilityHandler;
	});
}

/** Sets the precomputed volume distance field for the scene, or NULL to clear the current one. */
void FScene::SetPrecomputedVolumeDistanceField(const class FPrecomputedVolumeDistanceField* PrecomputedVolumeDistanceField)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdatePrecomputedVolumeDistanceField,
		FScene*,Scene,this,
		const FPrecomputedVolumeDistanceField*,PrecomputedVolumeDistanceField,PrecomputedVolumeDistanceField,
	{
		// Release the existing texture
		Scene->PrecomputedDistanceFieldVolumeTexture.ReleaseResource();
		Scene->PrecomputedDistanceFieldVolumeTexture.Data = NULL;
		if (PrecomputedVolumeDistanceField)
		{
			Scene->PrecomputedDistanceFieldVolumeTexture.Format = PF_A8R8G8B8;
			Scene->PrecomputedDistanceFieldVolumeTexture.bSRGB = FALSE;
			Scene->PrecomputedDistanceFieldVolumeTexture.SizeX = PrecomputedVolumeDistanceField->VolumeSizeX;
			Scene->PrecomputedDistanceFieldVolumeTexture.SizeY = PrecomputedVolumeDistanceField->VolumeSizeY;
			Scene->PrecomputedDistanceFieldVolumeTexture.SizeZ = PrecomputedVolumeDistanceField->VolumeSizeZ;
			Scene->PrecomputedDistanceFieldVolumeTexture.Data = (const BYTE*)PrecomputedVolumeDistanceField->Data.GetData();
			Scene->PrecomputedDistanceFieldVolumeTexture.InitResource();
			Scene->VolumeDistanceFieldMaxDistance = PrecomputedVolumeDistanceField->VolumeMaxDistance;
			Scene->VolumeDistanceFieldBox = PrecomputedVolumeDistanceField->VolumeBox;
		}
	});
}

/** 
 *	Indicates if sounds in this should be allowed to play. 
 *	By default - sound is only allowed in the GWorld scene.
 */
UBOOL FScene::AllowAudioPlayback()
{
	// Update wave instances if component is attached to current world. This means that only PIE can emit sound if a PIE session is active
	// and prevents ambient sounds from being played more than once as they will be part of the original world and the PIE world. The lesser
	// of two evils...

	return ((GWorld->Scene == this) || bAlwaysAllowAudioPlayback);
}

/**
 * @return		TRUE if hit proxies should be rendered in this scene.
 */
UBOOL FScene::RequiresHitProxies() const
{
	return (GIsEditor && !GIsCooking && bRequiresHitProxies);
}

/** 
 *	Set the primitives motion blur info
 * 
 *	@param PrimitiveSceneInfo	The primitive to add
 *	@param bRemoving			TRUE if the primitive is being removed
 *  @param FrameNumber			is only used if !bRemoving
 */
void FScene::UpdatePrimitiveMotionBlur(FPrimitiveSceneInfo* PrimitiveSceneInfo, const FMatrix& LocalToWorld, UBOOL bRemoving, UINT FrameNumber)
{
	check(PrimitiveSceneInfo && IsInRenderingThread());

	const FPrimitiveSceneProxy* Proxy = PrimitiveSceneInfo->Proxy; 
	UPrimitiveComponent* Component = PrimitiveSceneInfo->Component;

	if (Proxy != NULL && Component != NULL && Proxy->IsMovable())
	{
		// Check for validity...
		if (Component->MotionBlurInfoIndex != INDEX_NONE)
		{
			if (!MotionBlurInfoArray.IsValidIndex(Component->MotionBlurInfoIndex) || 
				// also make sure it hasn't already been freed
				MotionBlurFreeEntries.FindItemIndex(Component->MotionBlurInfoIndex) != INDEX_NONE)
			{
				// The primitive was detached and not rendered, but hung onto by the game thread.
				// So reset the MBInfoIndex.
				Component->MotionBlurInfoIndex = INDEX_NONE;
			}
			else
			{
				FMotionBlurInfo& MBInfo = MotionBlurInfoArray(Component->MotionBlurInfoIndex);
				// A different component is in this slot...
				// Make same assumption as above.
				if (MBInfo.GetComponent() != Component)
				{
					ClearMotionBlurInfoIndex(Component->MotionBlurInfoIndex);
				}
			}
		}

		if (Component->MotionBlurInfoIndex == INDEX_NONE)
		{
			// add a new entry if not removing
			if (!bRemoving)
			{
				// find an existing slot
				INT MBInfoIndex = INDEX_NONE;
				if (MotionBlurFreeEntries.Num() > 0)
				{
					// use an existing free entry
					MBInfoIndex = MotionBlurFreeEntries.Pop();
					check(MotionBlurInfoArray.IsValidIndex(MBInfoIndex));
				}
				else
				{
					// First time add
					MBInfoIndex = MotionBlurInfoArray.AddItem(FMotionBlurInfo());				
				}
				FMotionBlurInfo& MBInfo = MotionBlurInfoArray(MBInfoIndex);

				MBInfo.SetMotionBlurInfo(Component, PrimitiveSceneInfo, Proxy->GetLocalToWorld(), FrameNumber);
				Component->MotionBlurInfoIndex = MBInfoIndex;
			}
		}
		else
		{
			// Update the PrimitiveSceneInfo pointer
			check(Component->MotionBlurInfoIndex < MotionBlurInfoArray.Num());
			FMotionBlurInfo& MBInfo = MotionBlurInfoArray(Component->MotionBlurInfoIndex);
			if (bRemoving)
			{
				// When removing, set the PSI to null to indicate the primitive component pointer is invalid
				ClearMotionBlurInfoIndex(Component->MotionBlurInfoIndex);
			}
			else
			{
				// Update the PSI pointer
				MBInfo.SetMotionBlurInfo(Component, PrimitiveSceneInfo, Proxy->GetLocalToWorld(), FrameNumber);
			}			
		}
	}
}

/**
 *	Clear out the motion blur info. 
 */
void FScene::ClearMotionBlurInfo()
{
	check( IsInRenderingThread() );

	for (INT MBIndex = 0; MBIndex < MotionBlurInfoArray.Num(); MBIndex++)
	{
		FMotionBlurInfo& MBInfo = MotionBlurInfoArray(MBIndex);

		if (MBInfo.GetPrimitiveSceneInfo())
		{
			// The primitive component has not been released... clear it's MBInfoIndex
			MBInfo.GetPrimitiveSceneInfo()->Component->MotionBlurInfoIndex = INDEX_NONE;
		}
	}

	MotionBlurInfoArray.Empty();
	MotionBlurFreeEntries.Empty();
}

/**
 * Reset all the dirty bits for motion blur so they can be updated for hte next frame
 */
void FScene::ResetMotionBlurInfoDirty()
{
	check( IsInRenderingThread() );

	for (INT MBIndex = 0; MBIndex < MotionBlurInfoArray.Num(); MBIndex++)
	{
		FMotionBlurInfo& MBInfo = MotionBlurInfoArray(MBIndex);

		MBInfo.MarkNotUpdated();
	}
}

/**
 * Clear an entry in the motion blur info array
 *
 * @param MBInfoIndex index of motion blur entry to clear
 */
void FScene::ClearMotionBlurInfoIndex(INT MBInfoIndex)
{
	if (MotionBlurInfoArray.IsValidIndex(MBInfoIndex))
	{
		FMotionBlurInfo& MBInfo = MotionBlurInfoArray(MBInfoIndex);
		if (MBInfo.GetPrimitiveSceneInfo() != NULL)
		{
			// The primitive component has not been released... clear it's MBInfoIndex
			MBInfo.GetPrimitiveSceneInfo()->Component->MotionBlurInfoIndex = INDEX_NONE;
		}

		MBInfo.Invalidate();
		MotionBlurFreeEntries.AddUniqueItem(MBInfoIndex);
	}
}

/**
 * Remove motion blur infos for primitives that were not updated the last frame
 */
void FScene::ClearStaleMotionBlurInfo()
{
	check( IsInRenderingThread() );

	for (INT MBIndex = 0; MBIndex < MotionBlurInfoArray.Num(); MBIndex++)
	{
		FMotionBlurInfo& MBInfo = MotionBlurInfoArray(MBIndex);

		if (!MBInfo.IsUpdated())
		{
			ClearMotionBlurInfoIndex(MBIndex);
		}
	}
}

/** 
 *	Get the primitives motion blur info
 * 
 *	@param	PrimitiveSceneInfo	The primitive to retrieve the motion blur info for
 *	@param	MotionBlurInfo		The pointer to set to the motion blur info for the primitive (OUTPUT)
 *
 *	@return	UBOOL				TRUE if the primitive info was found and set
 */
UBOOL FScene::GetPrimitiveMotionBlurInfo(const FPrimitiveSceneInfo* PrimitiveSceneInfo, FMatrix& OutPreviousLocalToWorld, const FMotionBlurParams& MotionBlurParams)
{
	if (PrimitiveSceneInfo && PrimitiveSceneInfo->Component)
	{
		UPrimitiveComponent& Component = *(PrimitiveSceneInfo->Component);

		if (MotionBlurInfoArray.IsValidIndex(Component.MotionBlurInfoIndex))
		{
			check(IsInRenderingThread());

			FMotionBlurInfo &Info = MotionBlurInfoArray(Component.MotionBlurInfoIndex);

			UBOOL bInfoIsValid = TRUE;

			if(Info.Get(Component, OutPreviousLocalToWorld))
			{
				// the data was not updated last frame - it's outdated and we clear it
				// unless we are in "PlayersOnly" (special pause) mode
				if(Info.IsUpdated() || MotionBlurParams.bPlayersOnly)
				{
					return TRUE;
				}
			}
			else
			{
				// the slot was reused by another component - internal error?
			}

			// mismatched component so clear this entry
			ClearMotionBlurInfoIndex(Component.MotionBlurInfoIndex);
		}
	}
	return FALSE;
}

/**
 *	Add the scene captures view info to the streaming manager
 *
 *	@param	StreamingManager			The streaming manager to add the view info to.
 *	@param	View						The scene view information
 */
void FScene::AddSceneCaptureViewInformation(FStreamingManagerCollection* StreamingManager, FSceneView* View)
{
	for( TSparseArray<FCaptureSceneInfo*>::TConstIterator CaptureIt(SceneCapturesGameThread); CaptureIt; ++CaptureIt )
	{
		FCaptureSceneInfo* CaptureInfo = *CaptureIt;

		if (CaptureInfo->SceneCaptureProbe && CaptureInfo->SceneCaptureProbe->ViewInfoUpdateRequired())
		{
			FVector ProbeLocation = CaptureInfo->SceneCaptureProbe->GetProbeLocation();
			FVector DistanceToProbe = ProbeLocation - View->ViewOrigin;
			FLOAT DistSq = DistanceToProbe.SizeSquared();
			FLOAT ProbeMSUDS = CaptureInfo->SceneCaptureProbe->GetMaxStreamingUpdateDistSq() * GSystemSettings.SceneCaptureStreamingMultiplier;

			if (ProbeMSUDS > DistSq)
			{
				const UTextureRenderTarget* Target = CaptureInfo->SceneCaptureProbe->GetTextureTarget();
				if (Target)
				{
					FVector ViewLocation;
					if (CaptureInfo->SceneCaptureProbe->GetViewActorLocation(ViewLocation))
					{
						StreamingManager->AddViewInformation(
							ViewLocation,
							View->SizeX, 
							View->SizeX * View->ProjectionMatrix.M[0][0]
						);
					}
				}
			}
		}
	}
}

/**
*	Clears Hit Mask when component is detached
*
*	@param	ComponentToDetach		SkeletalMeshComponent To Detach
*/
void FScene::ClearHitMask(const UPrimitiveComponent* ComponentToDetach)
{
	// Clear this component from the list
	for( TSparseArray<FCaptureSceneInfo*>::TConstIterator CaptureIt(SceneCapturesRenderThread); CaptureIt; ++CaptureIt )
	{
		FCaptureSceneInfo* CaptureInfo = *CaptureIt;
		if ( CaptureInfo->SceneCaptureProbe )
		{
			CaptureInfo->SceneCaptureProbe->Clear(ComponentToDetach);
		}
	}
}

/**
*	Update Hit Mask when component is reattached
*
*	@param	ComponentToUpdate		SkeletalMeshComponent To Update
*/
void FScene::UpdateHitMask(const UPrimitiveComponent* ComponentToUpdate)
{
	// Update this component from the list
	for( TSparseArray<FCaptureSceneInfo*>::TConstIterator CaptureIt(SceneCapturesRenderThread); CaptureIt; ++CaptureIt )
	{
		FCaptureSceneInfo* CaptureInfo = *CaptureIt;
		if ( CaptureInfo->SceneCaptureProbe )
		{
			CaptureInfo->SceneCaptureProbe->Update(ComponentToUpdate);
		}
	}
}

void FScene::Release()
{
	// Send a command to the rendering thread to release the scene.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FReleaseCommand,
		FScene*,Scene,this,
		{
			delete Scene;
		});
}

/**
 * Dumps dynamic lighting and shadow interactions for scene to log.
 *
 * @param	bOnlyIncludeShadowCastingInteractions	Whether to only include shadow casting interactions
 */
void FScene::DumpDynamicLightShadowInteractions_RenderThread( UBOOL bOnlyIncludeShadowCastingInteractions ) const
{
	// Iterate over all primitives in scene and see whether they are statically lit/ shadowed or not.
	for( TSparseArray<FPrimitiveSceneInfo*>::TConstIterator It(Primitives); It; ++It )
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = *It;

		// Iterate over all lights this primitive is interacting with.
		FLightPrimitiveInteraction* LightPrimitiveInteraction = PrimitiveSceneInfo->LightList;
		while( LightPrimitiveInteraction )
		{
			// We only care about dynamic light/ primitive interactions that are not due to unbuilt lighting.
			if( LightPrimitiveInteraction->IsDynamic() && !LightPrimitiveInteraction->IsUncachedStaticLighting() 
			// Apply filter to only list shadowed interactions if wanted
			&& (!bOnlyIncludeShadowCastingInteractions || LightPrimitiveInteraction->HasShadow()) )
			{
				debugf(TEXT(",%s,%s"),*PrimitiveSceneInfo->Component->GetFullName(),*LightPrimitiveInteraction->GetLight()->LightComponent->GetFullName());
			}
			LightPrimitiveInteraction = LightPrimitiveInteraction->GetNextLight();
		}
	}
}

/**
 * Dumps dynamic lighting and shadow interactions for scene to log.
 *
 * @param	bInOnlyIncludeShadowCastingInteractions	Whether to only include shadow casting interactions
 */
void FScene::DumpDynamicLightShadowInteractions( UBOOL bInOnlyIncludeShadowCastingInteractions ) const
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FDumpDynamicLightShadowInteractionsCommand,
		const FScene*,Scene,this,
		UBOOL,bOnlyIncludeShadowCastingInteractions,bInOnlyIncludeShadowCastingInteractions,
		{
			Scene->DumpDynamicLightShadowInteractions_RenderThread(bOnlyIncludeShadowCastingInteractions);
		});
}

void FScene::DumpLightIteractions( FOutputDevice& Ar ) const
{
	Ar.Logf( TEXT( "DumpLightIteractions" ) );
	Ar.Logf( TEXT( "LightsNum: %d" ), Lights.Num() );
	// if want to print out all of the lights
	for( TSparseArray<FLightSceneInfoCompact>::TConstIterator It(Lights); It; ++It )
	{
		const FLightSceneInfoCompact& LightInfo = *It;

		const FString ToPrint = FString::Printf( TEXT( "LevelName: %s GetLightName: %s" ), *LightInfo.LightSceneInfo->LevelName.ToString(), *LightInfo.LightSceneInfo->GetLightName().ToString()  );
		Ar.Logf( *ToPrint );
	}
}

/**
 * Creates a unique radial blur proxy for the given component and adds it to the scene.
 *
 * @param RadialBlurComponent - component being added to the scene
 */
void FScene::AddRadialBlur(URadialBlurComponent* RadialBlurComponent)
{
	if (RadialBlurComponent != NULL &&
		RadialBlurComponent->bEnabled &&
		GSystemSettings.bAllowRadialBlur)
	{
		FRadialBlurSceneProxy* RadialBlurSceneProxy = new FRadialBlurSceneProxy(RadialBlurComponent);

		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			AddRadialBlurCmd,
			URadialBlurComponent*,RadialBlurComponent,RadialBlurComponent,
			FRadialBlurSceneProxy*,RadialBlurSceneProxy,RadialBlurSceneProxy,
			FScene*,Scene,this,
		{
			FRadialBlurSceneProxy** FoundIt = Scene->RadialBlurInfos.Find(RadialBlurComponent);
			if (FoundIt)
			{
				delete *FoundIt;
			}
			Scene->RadialBlurInfos.Set(RadialBlurComponent,RadialBlurSceneProxy);
		});
	}
}

/**
 * Removes the radial blur proxy for the given component from the scene.
 *
 * @param RadialBlurComponent - component being added to the scene
 */
void FScene::RemoveRadialBlur(URadialBlurComponent* RadialBlurComponent)
{
	if (RadialBlurComponent != NULL)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			AddRadialBlurCmd,
			URadialBlurComponent*,RadialBlurComponent,RadialBlurComponent,
			FScene*,Scene,this,
		{
			FRadialBlurSceneProxy** FoundIt = Scene->RadialBlurInfos.Find(RadialBlurComponent);
			if (FoundIt)
			{
				delete *FoundIt;
				Scene->RadialBlurInfos.Remove(RadialBlurComponent);
			}
		});
	}
}

/**
 * Exports the scene.
 *
 * @param	Ar		The Archive used for exporting.
 **/
void FScene::Export( FArchive& Ar ) const
{
	
}

/**
 * Dummy NULL scene interface used by dedicated servers.
 */
class FNULLSceneInterface : public FSceneInterface
{
public:
	FNULLSceneInterface( UWorld* InWorld )
		:	World( InWorld )
	{}

	virtual void AddPrimitive(UPrimitiveComponent* Primitive){}
	virtual void RemovePrimitive(UPrimitiveComponent* Primitive, UBOOL bWillReattach){}

	/** Updates the transform of a primitive which has already been added to the scene. */
	virtual void UpdatePrimitiveTransform(UPrimitiveComponent* Primitive){}

	virtual void AddLight(ULightComponent* Light){}
	virtual void RemoveLight(ULightComponent* Light){}

	/** Updates the transform of a light which has already been added to the scene. */
	virtual void UpdateLightTransform(ULightComponent* Light){}
	virtual void UpdateLightColorAndBrightness(ULightComponent* Light){}

	virtual void AddSceneCapture(USceneCaptureComponent* CaptureComponent){}
	virtual void RemoveSceneCapture(USceneCaptureComponent* CaptureComponent){}

	virtual void AddFluidSurface(UFluidSurfaceComponent* FluidComponent) {}
	virtual void RemoveFluidSurface(UFluidSurfaceComponent* FluidComponent) {}
	virtual const TArray<UFluidSurfaceComponent*>* GetFluidSurfaces() { return NULL; }

	virtual void AddHeightFog(class UHeightFogComponent* FogComponent){}
	virtual void RemoveHeightFog(class UHeightFogComponent* FogComponent){}

	virtual void AddExponentialHeightFog(class UExponentialHeightFogComponent* FogComponent){}
	virtual void RemoveExponentialHeightFog(class UExponentialHeightFogComponent* FogComponent){}

	virtual void AddFogVolume(const UPrimitiveComponent* MeshComponent){};
	virtual void AddFogVolume(const UFogVolumeDensityComponent* FogVolumeComponent, const UPrimitiveComponent* MeshComponent){};
	virtual void RemoveFogVolume(const UPrimitiveComponent* MeshComponent){};

	virtual void AddWindSource(class UWindDirectionalSourceComponent* WindComponent) {}
	virtual void RemoveWindSource(class UWindDirectionalSourceComponent* WindComponent) {}
	virtual const TArray<class FWindSourceSceneProxy*>& GetWindSources_RenderThread() const
	{
		static TArray<class FWindSourceSceneProxy*> NullWindSources;
		return NullWindSources;
	}
	virtual FVector4 GetWindParameters(const FVector& Position) const { return FVector4(0,0,1,0); }
	virtual void AddRadialBlur(class URadialBlurComponent* RadialBlurComponent) {}
	virtual void RemoveRadialBlur(class URadialBlurComponent* RadialBlurComponent) {}

	virtual void Release(){}

	/**
	 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
	 *
	 * @param	Primitive				Primitive to retrieve interacting lights for
	 * @param	RelevantLights	[out]	Array of lights interacting with primitive
	 */
	virtual void GetRelevantLights( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const {}

	/** Indicates if sounds in this should be allowed to play. */
	virtual UBOOL AllowAudioPlayback() 
	{
		return FALSE;
	}

	/**
	 * @return		TRUE if hit proxies should be rendered in this scene.
	 */
	virtual UBOOL RequiresHitProxies() const 
	{
		return FALSE;
	}

	// Accessors.
	virtual class UWorld* GetWorld() const
	{
		return World;
	}

	/**
	* Return the scene to be used for rendering
	*/
	virtual class FScene* GetRenderScene()
	{
		return NULL;
	}

private:
	UWorld* World;
};

/**
 * Allocates a new instance of the private FScene implementation for the given world.
 * @param World - An optional world to associate with the scene.
 * @param bAlwaysAllowAudioPlayback - Indicate that audio in this scene should always be played, even if its not the GWorld.
 * @param bInRequiresHitProxies- Indicates that hit proxies should be rendered in the scene.
 */
FSceneInterface* AllocateScene(UWorld* World, UBOOL bInAlwaysAllowAudioPlayback, UBOOL bInRequiresHitProxies)
{
	FSceneInterface* SceneInterface = NULL;

	// Create a full fledged scene if we have something to render.
	if( GIsClient && !GIsUCC )
	{
		FScene* Scene = new FScene;
		Scene->World = World;
		Scene->bAlwaysAllowAudioPlayback = bInAlwaysAllowAudioPlayback;
		Scene->bRequiresHitProxies = bInRequiresHitProxies;
		SceneInterface = Scene;
	}
	// And fall back to a dummy/ NULL implementation for commandlets and dedicated server.
	else
	{
		SceneInterface = new FNULLSceneInterface( World );
	}

	return SceneInterface;
}

FSceneViewStateInterface* AllocateViewState()
{
	return new FSceneViewState();
}

FPrimitiveSceneProxy* Scene_GetProxyFromInfo(FPrimitiveSceneInfo* Info)
{
	return Info ? Info->Proxy : NULL;
}

/** Maps the no light-map case to the appropriate base pass draw list. */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FNoLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FNoLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassNoLightMapDrawList[DrawType];
}

/** Maps the directional vertex light-map case to the appropriate base pass draw list. */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FDirectionalVertexLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FDirectionalVertexLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassDirectionalVertexLightMapDrawList[DrawType];
}

/** Maps the simple vertex light-map case to the appropriate base pass draw list. */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FSimpleVertexLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FSimpleVertexLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassSimpleVertexLightMapDrawList[DrawType];
}

/** Maps the directional light-map texture case to the appropriate base pass draw list. */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FDirectionalLightMapTexturePolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FDirectionalLightMapTexturePolicy>(EBasePassDrawListType DrawType)
{
	return BasePassDirectionalLightMapTextureDrawList[DrawType];
}

/** Maps the simple light-map texture case to the appropriate base pass draw list. */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FSimpleLightMapTexturePolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FSimpleLightMapTexturePolicy>(EBasePassDrawListType DrawType)
{
	return BasePassSimpleLightMapTextureDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FDirectionalLightLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FDirectionalLightLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassDirectionalLightDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FDynamicallyShadowedMultiTypeLightLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FDynamicallyShadowedMultiTypeLightLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassDynamicallyShadowedDynamicLightDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FSHLightAndMultiTypeLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FSHLightAndMultiTypeLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassSHLightAndDynamicLightDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FSHLightLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FSHLightLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassSHLightDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FShadowedDynamicLightDirectionalVertexLightMapPolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FShadowedDynamicLightDirectionalVertexLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassShadowedDynamicLightDirectionalVertexLightMapDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FShadowedDynamicLightDirectionalLightMapTexturePolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FShadowedDynamicLightDirectionalLightMapTexturePolicy>(EBasePassDrawListType DrawType)
{
	return BasePassShadowedDynamicLightDirectionalLightMapTextureDrawList[DrawType];
}

template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FDistanceFieldShadowedDynamicLightDirectionalLightMapTexturePolicy,FNoDensityPolicy> >& FDepthPriorityGroup::GetBasePassDrawList<FDistanceFieldShadowedDynamicLightDirectionalLightMapTexturePolicy>(EBasePassDrawListType DrawType)
{
	return BasePassDistanceFieldShadowedDynamicLightDirectionalLightMapTextureDrawList[DrawType];
}

/*-----------------------------------------------------------------------------
	Stat declarations.
-----------------------------------------------------------------------------*/

// The purpose of the SceneUpdate stat group is to show where rendering thread time is going from a high level outside of RenderViewFamily.
// It should only contain stats that are likely to track a lot of time in a typical scene, not edge case stats that are rarely non-zero.
// Use a more detailed profiler (like an instruction trace or sampling capture on Xbox 360) to track down where time is going in more detail.

DECLARE_STATS_GROUP(TEXT("SceneUpdate"),STATGROUP_SceneUpdate);

DECLARE_CYCLE_STAT(TEXT("   Update particles"),STAT_ParticleUpdateRTTime,STATGROUP_SceneUpdate);
DECLARE_CYCLE_STAT(TEXT("   Update Influence Weights"),STAT_InfluenceWeightsUpdateRTTime,STATGROUP_SceneUpdate);
DECLARE_CYCLE_STAT(TEXT("   Update GPU Skin"),STAT_GPUSkinUpdateRTTime,STATGROUP_SceneUpdate);

DECLARE_CYCLE_STAT(TEXT("   RemoveLight"),STAT_RemoveSceneLightTime,STATGROUP_SceneUpdate);
DECLARE_CYCLE_STAT(TEXT("   UpdateLight"),STAT_UpdateSceneLightTime,STATGROUP_SceneUpdate);
DECLARE_CYCLE_STAT(TEXT("   AddLight"),STAT_AddSceneLightTime,STATGROUP_SceneUpdate);

DECLARE_CYCLE_STAT(TEXT("   RemovePrimitive"),STAT_RemoveScenePrimitiveTime,STATGROUP_SceneUpdate);
DECLARE_CYCLE_STAT(TEXT("   AddPrimitive"),STAT_AddScenePrimitiveRenderThreadTime,STATGROUP_SceneUpdate);

DECLARE_CYCLE_STAT(TEXT("   UpdatePrimitiveTransform"),STAT_UpdatePrimitiveTransformRenderThreadTime,STATGROUP_SceneUpdate);

// Update RenderViewFamily_RenderThread if more stats are added to STATGROUP_SceneUpdate so that STAT_TotalRTUpdateTime stays up to date
DECLARE_CYCLE_STAT(TEXT("Total RT Update"),STAT_TotalRTUpdateTime,STATGROUP_SceneUpdate);
