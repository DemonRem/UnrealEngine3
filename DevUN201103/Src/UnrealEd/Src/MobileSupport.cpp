/*=============================================================================
	MobileSupport.cpp: Editor support for mobile devices
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnConsoleSupportContainer.h"
#include <ddraw.h>

// we need to set this to 1 so ThumbnailHelpers defines the thumbnail scene that we use to flatten
#define _WANTS_MATERIAL_HELPER 1
#include "ThumbnailHelpers.h"


/**
* Class for rendering previews of material expressions in the material editor's linked object viewport.
*/
class FFlattenMaterialProxy : public FMaterial, public FMaterialRenderProxy
{
public:

	/**
	 * Constructor 
	 */
	FFlattenMaterialProxy(UMaterialInterface* InMaterialInterface, FProxyMaterialCompiler* InOverrideMaterialCompiler=NULL)
		: MaterialInterface(InMaterialInterface)
		, Material(InMaterialInterface->GetMaterial())
		, OverrideMaterialCompiler(InOverrideMaterialCompiler)
	{
		check(Material);

		CacheShaders();
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return TRUE if the shader should be compiled
	 */
	virtual UBOOL ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
	{
		if (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLocalVertexFactory"), FNAME_Find)))
		{
			// compile the shader for the local vertex factory
			return TRUE;
		}

		return FALSE;
	}

	////////////////
	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial() const
	{
		if(GetShaderMap())
		{
			return this;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(FALSE)->GetMaterial();
		}
	}

	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetTextureValue(ParameterName,OutValue,Context);
	}

	// Material properties.
	virtual INT CompileProperty(EMaterialShaderPlatform MatPlatform,EMaterialProperty Property,FMaterialCompiler* Compiler) const
	{
		UMaterial* Material = MaterialInterface->GetMaterial(MatPlatform);
		check(Material);

		if (Property == MP_SpecularColor)
		{
			// we need to tell the compiler which property this constant is for so that the code chunk is
			// put in the proper array
			Compiler->SetMaterialProperty(MP_SpecularColor);
			return Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			return MaterialInterface->GetMaterialResource(MatPlatform)->CompileProperty(MatPlatform, Property, OverrideMaterialCompiler ? OverrideMaterialCompiler : Compiler);
		}
	}

	virtual FString GetMaterialUsageDescription() const { return FString::Printf(TEXT("FFlattenMaterial %s"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL")); }
	virtual UBOOL IsTwoSided() const 
	{ 
		return Material->TwoSided == 1;
	}
	virtual UBOOL RenderTwoSidedSeparatePass() const
	{ 
		return Material->TwoSidedSeparatePass == 1;
	}
	virtual UBOOL RenderLitTranslucencyPrepass() const
	{ 
		return Material->bUseLitTranslucencyDepthPass == 1;
	}
	virtual UBOOL RenderLitTranslucencyDepthPostpass() const
	{ 
		return Material->bUseLitTranslucencyPostRenderDepthPass == 1;
	}
	virtual UBOOL CastLitTranslucencyShadowAsMasked() const
	{ 
		return Material->bCastLitTranslucencyShadowAsMasked == 1;
	}
	virtual UBOOL NeedsDepthTestDisabled() const
	{
		return Material->bDisableDepthTest == 1;
	}
	virtual UBOOL IsLightFunction() const
	{
		return Material->bUsedAsLightFunction == 1;
	}
	virtual UBOOL IsUsedWithFogVolumes() const
	{
		return FALSE;
	}
	virtual UBOOL IsSpecialEngineMaterial() const
	{
		return Material->bUsedAsSpecialEngineMaterial == 1;
	}
	virtual UBOOL IsTerrainMaterial() const
	{
		return FALSE;
	}
	virtual UBOOL IsDecalMaterial() const
	{
		return Material->bUsedWithDecals == 1;
	}
	virtual UBOOL IsWireframe() const
	{
		return FALSE;
	}
	virtual UBOOL IsDistorted() const
	{
		return Material->bUsesDistortion && !Material->bUseOneLayerDistortion; 
	}
	virtual UBOOL HasSubsurfaceScattering() const
	{
		return Material->EnableSubsurfaceScattering;
	}
	virtual UBOOL HasSeparateTranslucency() const
	{
		return Material->EnableSeparateTranslucency;
	}
	virtual UBOOL IsMasked() const
	{
		return Material->bIsMasked;
	}
	virtual enum EBlendMode GetBlendMode() const					
	{
		return (EBlendMode)Material->BlendMode; 
	}
	virtual enum EMaterialLightingModel GetLightingModel() const	
	{
		return (EMaterialLightingModel)Material->LightingModel; 
	}
	virtual FLOAT GetOpacityMaskClipValue() const
	{
		return Material->OpacityMaskClipValue; 
	}
	virtual FString GetFriendlyName() const
	{
		return FString::Printf(TEXT("FFlattenMaterialProxy %s"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL")); 
	}

	/**
	* Should shaders compiled for this material be saved to disk?
	*/
	virtual UBOOL IsPersistent() const { return FALSE; }

	const UMaterialInterface* GetMaterialInterface() const
	{
		return MaterialInterface;
	}

	friend FArchive& operator<< ( FArchive& Ar, FFlattenMaterialProxy& V )
	{
		return Ar << V.MaterialInterface;
	}

private:
	/** The material interface for this proxy */
	UMaterialInterface* MaterialInterface;

	/** The usable material for this proxy */
	UMaterial* Material;

	/** Compiler to use while compiling material expressions (can be NULL to use default compiler) */
	FProxyMaterialCompiler* OverrideMaterialCompiler;
};


class UFlattenMaterial : public UMaterialInterface
{
	DECLARE_CLASS_INTRINSIC(UFlattenMaterial, UMaterialInterface, CLASS_Transient, UnrealEd);

public:

	/**
	 * Set up this material to use another material as its master
	 */
	void Initialize(UMaterialInterface* InSourceMaterial)
	{
		SourceMaterial = InSourceMaterial;

		delete MaterialProxy;
		MaterialProxy = new FFlattenMaterialProxy(SourceMaterial);
	}

	/**
	 * Cleanup unneeded references
	 */
	void Cleanup()
	{
		SourceMaterial = NULL;

		delete MaterialProxy;
		MaterialProxy = NULL;
	}

	/**
	 * Get the material which this is an instance of.
	 * Warning - This is platform dependent!  Do not call GetMaterial(GCurrentMaterialPlatform) and save that reference,
	 * as it will be different depending on the current platform.  Instead call GetMaterial(MSP_BASE) to get the base material and save that.
	 * When getting the material for rendering/checking usage, GetMaterial(GCurrentMaterialPlatform) is fine.
	 *
	 * @param Platform - The platform to get material for.
	 */
	virtual class UMaterial* GetMaterial(EMaterialShaderPlatform Platform)
	{
		return SourceMaterial->GetMaterial(Platform);
	}

	/**
	 * Tests this material instance for dependency on a given material instance.
	 * @param	TestDependency - The material instance to test this instance for dependency upon.
	 * @return	True if the material instance is dependent on TestDependency.
	 */
	virtual UBOOL IsDependent(UMaterialInterface* TestDependency)
	{
		return SourceMaterial->IsDependent(TestDependency);
	}

	/**
	 * Returns a pointer to the FMaterialRenderProxy used for rendering.
	 *
	 * @param	Selected	specify TRUE to return an alternate material used for rendering this material when part of a selection
	 *						@note: only valid in the editor!
	 *
	 * @return	The resource to use for rendering this material instance.
	 */
	virtual FMaterialRenderProxy* GetRenderProxy(UBOOL Selected, UBOOL bHovered=FALSE) const
	{
		return MaterialProxy;
	}

	/**
	 * Returns a pointer to the physical material used by this material instance.
	 * @return The physical material.
	 */
	virtual UPhysicalMaterial* GetPhysicalMaterial() const
	{
		return SourceMaterial->GetPhysicalMaterial();
	}

	/**
	 * Gathers the textures used to render the material instance.
	 * @param OutTextures	Upon return contains the textures used to render the material instance.
	 * @param bOnlyAddOnce	Whether to add textures that are sampled multiple times uniquely or not
	 */
	virtual void GetUsedTextures(TArray<UTexture*> &OutTextures, EMaterialShaderPlatform Platform = MSP_BASE, UBOOL bAllPlatforms = FALSE)
	{
		SourceMaterial->GetUsedTextures(OutTextures, Platform, bAllPlatforms);
	}

	/**
	 * Overrides a specific texture (transient)
	 *
	 * @param InTextureToOverride The texture to override
	 * @param OverrideTexture The new texture to use
	 */
	virtual void OverrideTexture( UTexture* InTextureToOverride, UTexture* OverrideTexture ) 
	{
		SourceMaterial->OverrideTexture(InTextureToOverride, OverrideTexture);
	}

	/**
	 * Checks if the material can be used with the given usage flag.  
	 * If the flag isn't set in the editor, it will be set and the material will be recompiled with it.
	 * @param Usage - The usage flag to check
	 * @return UBOOL - TRUE if the material can be used for rendering with the given type.
	 */
	virtual UBOOL CheckMaterialUsage(EMaterialUsage Usage)
	{
		return SourceMaterial->CheckMaterialUsage(Usage);
	}

	/**
	* Allocates a new material resource
	* @return	The allocated resource
	*/
	virtual FMaterialResource* AllocateResource()
	{
		return SourceMaterial->AllocateResource();
	}

	/**
	 * Gets the static permutation resource if the instance has one
	 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
	 */
	virtual FMaterialResource* GetMaterialResource(EMaterialShaderPlatform Platform) 
	{
		return SourceMaterial->GetMaterialResource(Platform);
	}

private:
	/** The real material this flattener renders with */
	UMaterialInterface* SourceMaterial;

	/** The material proxy that can recompile the material without specular */
	FFlattenMaterialProxy* MaterialProxy;
};
IMPLEMENT_CLASS(UFlattenMaterial);


/**
 * Calculate the size that the auto-flatten texture for this material should be
 *
 * @param MaterialInterface The material to scan for texture size
 *
 * @return Size to make the auto-flattened texture
 */
INT CalcAutoFlattenTextureSize(UMaterialInterface* MaterialInterface)
{
	// static lod settings so that we only initialize them once
	static FTextureLODSettings GameTextureLODSettings;
	static UBOOL bAreLODSettingsInitialized=FALSE;
	if (!bAreLODSettingsInitialized)
	{
		// initialize LOD settings with game texture resolutions, since we don't want to use 
		// potentially bloated editor settings
		GameTextureLODSettings.Initialize(GEngineIni, TEXT("SystemSettings"));
	}

	TArray<UTexture*> MaterialTextures;
	
	MaterialInterface->GetUsedTextures(MaterialTextures);

	// find the largest texture in the list (applying it's LOD bias)
	INT MaxSize = 0;
	for (INT TexIndex = 0; TexIndex < MaterialTextures.Num(); TexIndex++)
	{
		UTexture* Texture = MaterialTextures(TexIndex);

		if (Texture == NULL)
		{
			continue;
		}

		// get the max size of the texture
		INT TexSize = 0;
		if (Texture->IsA(UTexture2D::StaticClass()))
		{
			UTexture2D* Tex2D = (UTexture2D*)Texture;
			TexSize = Max(Tex2D->SizeX, Tex2D->SizeY);
		}
		else if (Texture->IsA(UTextureCube::StaticClass()))
		{
			UTextureCube* TexCube = (UTextureCube*)Texture;
			TexSize = Max(TexCube->SizeX, TexCube->SizeY);
		}

		// bias the texture size based on LOD group
		INT BiasedSize = TexSize >> GameTextureLODSettings.CalculateLODBias(Texture);

		// finally, update the max size for the material
		MaxSize = Max<INT>(MaxSize, BiasedSize);
	}

	// if the material has no textures, then just default to 128
	if (MaxSize == 0)
	{
		MaxSize = 128;
	}

	// Make sure the texture size is a power of two
	MaxSize = appRoundUpToPowerOfTwo( MaxSize );

	// now, bias this by a global "mobile flatten bias"
	INT FlattenedTextureResolutionBias = 0;
	GConfig->GetInt(TEXT("MobileSupport"), TEXT("FlattenedTextureResolutionBias"), FlattenedTextureResolutionBias, GEngineIni);
	MaxSize >>= FlattenedTextureResolutionBias;

	// make the material at least 16x16, because PVRTC2 can't do 8x8
	MaxSize = Max(MaxSize, 16);

	// finally, return what we calculated
	return MaxSize;
}

/** 
 * Flatten the given material to a texture if it should be flattened
 *
 * @param MaterialInterface The material to potentially flatten
 * @param bReflattenAutoFlatten If the dominant texture was already an auto-flattened texture, specifying TRUE will reflatten
 */
void ConditionalFlattenMaterial(UMaterialInterface* MaterialInterface, UBOOL bReflattenAutoFlattened, const UBOOL bInForceFlatten)
{
	//if we're not force flattening, allow an early out
	if (!bInForceFlatten)
	{
		// bail out if this material should never be flattened (eg Landscape MICs)
		if (!MaterialInterface->bAutoFlattenMobile)
		{
			return;
		}

		// should we flatten materials at all?
		UBOOL bShouldFlattenMaterials = FALSE;
		GConfig->GetBool(TEXT("MobileSupport"), TEXT("bShouldFlattenMaterials"), bShouldFlattenMaterials, GEngineIni);

		// if not, return
		if (!bShouldFlattenMaterials)
		{
			return;
		}
	}

	// figure out what the name for the auto-flattened texture should be
	FString FlattenedName = FString::Printf(TEXT("%s_Flattened"), *MaterialInterface->GetName());

	// figure out if we even need to do anything
	UBOOL bShouldFlattenMaterial = bInForceFlatten;

	// calculate what size the auto-flatten texture will be, if it gets created
	INT TargetSize = -1;

	const UBOOL bHasExistingFlattenedTexture = MaterialInterface->MobileBaseTexture != NULL;

	// check to see if this is a MaterialInstance
	UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(MaterialInterface);

	// cannot auto-flatten fallbacks, and can't flatten materials with NULL material resource
	if (!MaterialInterface->IsFallbackMaterial() && MaterialInterface->GetMaterialResource())
	{
		// at this point if it has no dominant texture, we should auto-flatten
		if ( !bHasExistingFlattenedTexture )
		{
			bShouldFlattenMaterial = TRUE;
		}
		else
		{
			// if we have a dominant texture, then check if it was an previously auto-flattened texture or not
			UBOOL bIsAutoFlattenTexture = MaterialInterface->MobileBaseTexture->GetName() == FlattenedName;

			// if it's not an auto-flattened texture, then never flatten, as the user is specifying it manually
			if (bIsAutoFlattenTexture)
			{
				// if we want to force reflatten, then just do it
				if (bReflattenAutoFlattened)
				{
					bShouldFlattenMaterial = TRUE;
				}
				// if an MI was marked as needing flattening, do it now (we queue up the need to flatten because
				// otherwise editing MIC's would be very slow)
				else if (MaterialInstance && MaterialInstance->bNeedsMaterialFlattening)
				{
					bShouldFlattenMaterial = TRUE;
				}
				// otherwise, check if the pre-auto-flattened texture is "bad"
				else
				{
					// calculate what size if should be
					TargetSize = CalcAutoFlattenTextureSize(MaterialInterface);

					// get the size of the existing auto-flattened texture
					INT ExistingSize = -1;
					if (MaterialInterface->MobileBaseTexture->IsA(UTexture2D::StaticClass()))
					{
						ExistingSize = ((UTexture2D*)MaterialInterface->MobileBaseTexture)->SizeX;
					}
					else if (MaterialInterface->MobileBaseTexture->IsA(UTextureCube::StaticClass()))
					{
						ExistingSize = ((UTextureCube*)MaterialInterface->MobileBaseTexture)->SizeX;
					}
					check(ExistingSize != -1);

					// if it's the wrong size, then flatten
					if (TargetSize != ExistingSize)
					{
						bShouldFlattenMaterial = TRUE;
					}
				}
			}
		}
	}

	// no matter what happened above, reset the MI's dirty flag
	if (MaterialInstance)
	{
		MaterialInstance->bNeedsMaterialFlattening = FALSE;
	}

	if (bShouldFlattenMaterial)
	{
		// calc the size if we haven't already
		if (TargetSize == -1)
		{
			TargetSize = CalcAutoFlattenTextureSize(MaterialInterface);
		}

		// 2 copied from plane scale in thumbnail renderer
		FLOAT Scale = 2.f;

		// find a render target of the proper size
		FString RenderTargetName = FString::Printf(TEXT("FlattenRT_%d"), TargetSize);
		UTextureRenderTarget2D* RenderTarget = FindObject<UTextureRenderTarget2D>(UObject::GetTransientPackage(), *RenderTargetName);

		// if it's not created yet, create it
		if (!RenderTarget)
		{
			RenderTarget = CastChecked<UTextureRenderTarget2D>(UObject::StaticConstructObject(UTextureRenderTarget2D::StaticClass(), UObject::GetTransientPackage(), FName(*RenderTargetName), RF_Transient));
			RenderTarget->Init(TargetSize, TargetSize, PF_A8R8G8B8);
		}

		// find a flatten material
		FString FlattenMaterialName(TEXT("EditorFlattenMat"));
		UFlattenMaterial* FlattenMaterial = FindObject<UFlattenMaterial>(UObject::GetTransientPackage(), *FlattenMaterialName);

		// if it's not created yet, create it
		if (!FlattenMaterial)
		{
			FlattenMaterial = CastChecked<UFlattenMaterial>(UObject::StaticConstructObject(UFlattenMaterial::StaticClass(), UObject::GetTransientPackage(), FName(*FlattenMaterialName), RF_Transient));
		}

		// initialize it with the current material
		FlattenMaterial->Initialize(MaterialInterface);

		// set up the scene to render the "thumbnail"
		FMaterialThumbnailScene	ThumbnailScene(FlattenMaterial, TPT_Plane, 0, 0, Scale, TBT_None, FColor(0, 0, 0, 0), FColor(0, 0, 0, 0));
		// double the brightness of the light because it's coming at an angle not directly on to the surface
		// so the brightness of the resulting texture would be darker than it should be
		ThumbnailScene.SetLightBrightness(2.0f);
		FSceneViewFamilyContext ViewFamily(
			RenderTarget->GameThread_GetRenderTargetResource(),
			ThumbnailScene.GetScene(),
			((SHOW_DefaultEditor & ~(SHOW_ViewMode_Mask)) | SHOW_ViewMode_Lit) & ~(SHOW_Fog | SHOW_PostProcess),
			0, //Flattened textures should be at 0 time.
			GDeltaTime,
			0, //Flattened textures should be at 0 time.
			FALSE, FALSE, FALSE, TRUE, TRUE, 1.0f, // pass defaults in
			TRUE // write alpha
			);
		ThumbnailScene.GetView(&ViewFamily, 0, 0, TargetSize, TargetSize);

		FCanvas Canvas(RenderTarget->GameThread_GetRenderTargetResource(), NULL);
		BeginRenderingViewFamily(&Canvas,&ViewFamily);
		Canvas.Flush();

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FResolveCommand,
			FTextureRenderTargetResource*, RTResource ,RenderTarget->GameThread_GetRenderTargetResource(),
		{
			// copy the results of the scene rendering from the target surface to its texture
			RHICopyToResolveTarget(RTResource->GetRenderTargetSurface(), FALSE, FResolveParams());
		});

		// make sure we wait for the render to complete
		FlushRenderingCommands();

		// create the final rendered Texture2D, potentially fixing up alphas
		DWORD ConstructTextureFlags = CTF_Default | CTF_DeferCompression | CTF_AllowMips;

		// if the material is masked, then we need to treat 255 one the alpha as not rendered, and anything else as rendered
		// because the alpha channel will have depth written to it, and a depth of 255 means not rendered
		if (MaterialInterface->GetMaterial()->BlendMode == BLEND_Masked)
		{
			ConstructTextureFlags |= CTF_RemapAlphaAsMasked;
		}

		UTexture2D* FlattenedTexture = RenderTarget->ConstructTexture2D(MaterialInterface->GetOuter(), 
			FlattenedName, RF_Public, ConstructTextureFlags);

		// flattened textures go into new mobile group
		FlattenedTexture->LODGroup = TEXTUREGROUP_MobileFlattened;

		// assign this texture to be the dominant texture for this material
		MaterialInterface->MobileBaseTexture = FlattenedTexture;

		// If the flattened texture already existed, we don't need to do a repopulate in the CB to see it; just update the UI
		if ( bHasExistingFlattenedTexture )
		{
			GCallbackEvent->Send( FCallbackEventParameters( NULL, CALLBACK_RefreshContentBrowser, CBR_UpdateAssetListUI, FlattenedTexture ) );
		}
		// If the flattened texture didn't already exist, inform the CB of its creation
		else
		{
			GCallbackEvent->Send( FCallbackEventParameters( NULL, CALLBACK_RefreshContentBrowser, CBR_ObjectCreated|CBR_NoSync, FlattenedTexture ) );
		}

		// cleanup references to original material interface
		FlattenMaterial->Cleanup();
	}
}


/**
 * Flatten a material to a texture for mobile auto-fallback use
 */
void WxUnrealEdApp::CB_UpdateMobileFlattenedTexture(UMaterialInterface* MaterialInterface)
{
	if (MaterialInterface != NULL)
	{
		const UBOOL bReflattenAutoFlattened = TRUE;
		const UBOOL bForceFlatten = FALSE;
		// re-flatten if necessary
		ConditionalFlattenMaterial(MaterialInterface, bReflattenAutoFlattened, bForceFlatten);
	}
}

/**
 * @return TRUE if the given texture should have its mips compressed to PVRTC
 */
UBOOL ShouldCompressToPVRTC(UTexture2D* Texture)
{
	// we only convert DXT textures to PVRTC (or RGBA8 textures that will be DXT soon)
	UBOOL bIsDXT = Texture->Format >= PF_DXT1 && Texture->Format <= PF_DXT5;
	UBOOL bWillBeDXT = (Texture->Format == PF_A8R8G8B8 && Texture->DeferCompression);
	if (!(bIsDXT || bWillBeDXT))
	{
		return FALSE;
	}

	// PVRTC2 textures must be at least 16x16 (after squaring)
	UBOOL bWillBePVRTC2 = Texture->Format == PF_DXT1 && !Texture->bForcePVRTC4;
	if (bWillBePVRTC2 && (Texture->SizeX < 16 && Texture->SizeY < 16))
	{
		return FALSE;
	}

	// PVRTC4 textures must be at least 8x8 (after squaring)
	if (Texture->SizeX < 8 && Texture->SizeY < 8)
	{
		return FALSE;
	}

	// check commandline one time for options
	static UBOOL bHasCheckedInvariants = FALSE;
	static UBOOL bForcePVRTC;
	static UBOOL bForcePVRTC2;
	static UBOOL bForcePVRTC4;
	static UBOOL bCompressLightmaps;
	static UBOOL bHasConverter;
	if (!bHasCheckedInvariants)
	{
		bForcePVRTC = ParseParam(appCmdLine(), TEXT("forcepvrtc"));
		bForcePVRTC2 = ParseParam(appCmdLine(), TEXT("forcepvrtc2"));
		bForcePVRTC4 = ParseParam(appCmdLine(), TEXT("forcepvrtc4"));
		bCompressLightmaps = !ParseParam(appCmdLine(), TEXT("nopvrtclightmaps"));
		bHasConverter = GFileManager->FileSize(TEXT("..\\Redist\\ImgTec\\PVRTexTool.exe")) > 0;

		bHasCheckedInvariants = TRUE;
	}
	
	UBOOL bIsLightmap = Texture->IsA(ULightMapTexture2D::StaticClass()) || Texture->IsA(UShadowMapTexture2D::StaticClass());

	// If we're not forcing PVRTC compression, check a couple more conditions
	UBOOL bForceThisTexture = 
		bForcePVRTC ||
		(bForcePVRTC2 && bWillBePVRTC2) ||
		(bForcePVRTC4 && !bWillBePVRTC2);

	if (!bForceThisTexture)
	{
		// Don't need to convert if already converted
		if (Texture->CachedPVRTCMips.Num() == Texture->Mips.Num())
		{
			return FALSE;
		}
	}

	
	// Compress lightmaps unless 'nopvrtclightmaps' was passed in the commandline.
	// Always compress lightmaps when cooking.
 	if( bIsLightmap && !(bCompressLightmaps || GIsCooking ) )
 	{
 		return FALSE;
 	}

	// don't cache non-simple lightmaps
	if (bIsLightmap && !Texture->GetName().StartsWith("Simple"))
	{
		// make sure previously cached non-simple lightmaps are cleaned up
		Texture->CachedPVRTCMips.Empty();
		return FALSE;
	}

	// we must have the converter
	if (!bHasConverter)
	{
		return FALSE;
	}

	// skip thumbnails
	if (Texture->GetName() == TEXT("ThumbnailTexture"))
	{
		return FALSE;
	}

	// if nothing denied it, return TRUE!
	return TRUE;
}


// PVR file header format
struct FPVRHeader
{
	UINT HeaderSize;
	UINT Height;
	UINT Width;
	UINT NumMips;
	UINT Flags;
	UINT DataLength;
	UINT BitsPerPixel;
	UINT BitmaskRed;
	UINT BitmaskGreen;
	UINT BitmaskBlue;
	UINT BitmaskAlpha;
	UINT PVRTag;
	UINT NumSurfaces;
};

// We need to redeclare DDSURFACEDESC2, because it doesn't quite work as a header
// for the file since it has a pointer in it, so in 64-bit mode, it is the wrong size
struct FDDSHeader
{
	DWORD dwSize;                  // size of the DDSURFACEDESC structure
	DWORD dwFlags;                 // determines what fields are valid
	DWORD dwHeight;                // height of surface to be created
	DWORD dwWidth;                 // width of input surface
	DWORD dwLinearSize;            // Formless late-allocated optimized surface size
	DWORD dwDepth;                 // the depth if this is a volume texture 
	DWORD dwMipMapCount;           // number of mip-map levels requestde
	DWORD dwAlphaBitDepth;         // depth of alpha buffer requested
	DWORD dwReserved;              // reserved
	DWORD lpSurface;               // pointer to the associated surface memory
	QWORD ddckCKDestOverlay;       // Physical color for empty cubemap faces
	QWORD ddckCKDestBlt;           // color key for destination blt use
	QWORD ddckCKSrcOverlay;        // color key for source overlay use
	QWORD ddckCKSrcBlt;            // color key for source blt use
	DDPIXELFORMAT ddpfPixelFormat; // pixel format description of the surface
	DDSCAPS2 ddsCaps;              // direct draw surface capabilities
	DWORD dwTextureStage;          // stage in multitexture cascade
};


/**
 * Converts textures to PVRTC, using the textures source art or DXT data(if source art is not available) and caches the converted data in the texture
 *
 * @param Texture Texture to convert
 * @param bUseFastCompression If TRUE, the code will compress as fast as possible, if FALSE, it will be slow but better quality
 * @bForceCompression Forces compression of the texture
 * @param SourceArtOverride	Source art to use instead of the textures source art or to use if the texture has no source art.  This can be null 
 * @return TRUE if any work was completed (no early out was taken)
 */
UBOOL ConditionalCachePVRTCTextures(UTexture2D* Texture, UBOOL bUseFastCompression, UBOOL bForceCompression, TArray<FColor>* SourceArtOverride)
{
	// should this be compressed?
	if ( !bForceCompression && !ShouldCompressToPVRTC(Texture) )
	{
		return FALSE;
	}

	// clear any existing data
	Texture->CachedPVRTCMips.Empty();

	// cache some values
	UINT TexSizeX = Texture->SizeX;
	UINT TexSizeY = Texture->SizeY;
	UINT TexSizeSquare = Max(TexSizeX, TexSizeY);
	UINT NumColors = TexSizeX * TexSizeY;

	// the PVRTC compressor can't handle 4096 or bigger textures, so we have to go down in mips and use those
	INT FirstMipIndex = 0;
	while ((TexSizeSquare >> FirstMipIndex) > 2048)
	{
		FirstMipIndex++;
	}

	UINT FirstMipSizeX = TexSizeX >> FirstMipIndex;
	UINT FirstMipSizeY = TexSizeY >> FirstMipIndex;
	UINT FirstMipSizeSquare = TexSizeSquare >> FirstMipIndex;
	UINT FirstMipNumColors = TexSizeSquare;

	// if we need a smaller mip, but it doesn't exist, we can't proceed
	if (Texture->Mips.Num() <= FirstMipIndex)
	{
		return FALSE;
	}

	// room for raw data that will be converted
	TArray<FColor> RawData;

	UBOOL bSourceArtIsValid = FALSE;

	if( SourceArtOverride )
	{
		RawData = *SourceArtOverride;
		bSourceArtIsValid = TRUE;
	}
	// use the source art if it exists and we support the size of (ie, don't have to drop down mip levels)
	if (!bSourceArtIsValid && Texture->SourceArt.GetBulkDataSizeOnDisk() > 0 && FirstMipIndex == 0)
	{
		// cache some values
		UINT OriginalTexSizeX = Texture->OriginalSizeX;
		UINT OriginalTexSizeY = Texture->OriginalSizeY;
		UINT OriginalTexSizeSquare = Max(OriginalTexSizeX, OriginalTexSizeY);

		// size the raw data properly for the original content
		RawData.Add(OriginalTexSizeSquare * OriginalTexSizeSquare);

		// assume the decompress will work (only will fail via exception)
		bSourceArtIsValid = TRUE;
		try
		{
			// in this case, we have source art, so we need to decompress the PNG data
			FPNGHelper PNG;
			PNG.InitCompressed(Texture->SourceArt.Lock(LOCK_READ_ONLY), Texture->SourceArt.GetBulkDataSize(), OriginalTexSizeX, OriginalTexSizeY);
			appMemcpy(RawData.GetData(), PNG.GetRawData().GetData(), OriginalTexSizeX * OriginalTexSizeY * 4);
		}
		catch (...)
		{
			bSourceArtIsValid = FALSE;

			warnf(TEXT("%s had bad Source Art!"), *Texture->GetFullName());
		}
		Texture->SourceArt.Unlock();

		if (bSourceArtIsValid)
		{
			// swap red/blue for raw data (don't need this when loading a PVR file in the previous block)
			for (UINT Color = 0; Color < OriginalTexSizeX * OriginalTexSizeY; Color++)
			{
				FColor Orig = RawData(Color);
				RawData(Color) = FColor(Orig.B, Orig.G, Orig.R, Orig.A);
			}
		}
	}

	// Get the current process ID and add to the filenames.
	// In the event that there are two simultaneous builds (which should probably not be happening) compressing textures of the same name this will prevent race conditions.
	DWORD ProcessID = 0;
#if _WINDOWS
	ProcessID = GetCurrentProcessId();
#endif

	// if after the above, we didn't have valid source art, then use the DXT mip data as the source
	if (!bSourceArtIsValid)
	{
		// size the raw data properly
		RawData.Add(TexSizeSquare * TexSizeSquare);

		// fill out the file header
		FDDSHeader DDSHeader;
		appMemzero(&DDSHeader, sizeof(DDSHeader));
		DDSHeader.dwSize = sizeof(DDSHeader);
		DDSHeader.dwFlags = (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE | DDSD_MIPMAPCOUNT);
		DDSHeader.dwWidth = TexSizeX >> FirstMipIndex;
		DDSHeader.dwHeight = TexSizeY >> FirstMipIndex;
		DDSHeader.dwLinearSize = Texture->Mips(FirstMipIndex).Data.GetBulkDataSize();
		DDSHeader.dwMipMapCount = 1;
		DDSHeader.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		DDSHeader.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		DDSHeader.ddpfPixelFormat.dwFourCC = 
			Texture->Format == PF_DXT1 ? '1TXD' :
			Texture->Format == PF_DXT3 ? '3TXD' :
			'5TXD';
		DDSHeader.ddsCaps.dwCaps = DDSCAPS_TEXTURE;

		// open the .dds file
		FString InputFilePath = GSys->CachePath * FString::Printf( TEXT("%s_%d_DDSToRGB.dds"),*Texture->GetName(), ProcessID);
		FString OutputFilePath = GSys->CachePath * FString::Printf( TEXT("%s_%d_DDSToRGB.pvr"),*Texture->GetName(), ProcessID);
		
		FArchive* DDSFile = GFileManager->CreateFileWriter(*InputFilePath);

		// write out the magic tag for a DDS file
		DWORD MagicNumber = 0x20534444;
		DDSFile->Serialize(&MagicNumber, sizeof(MagicNumber));

		// write out header
		DDSFile->Serialize(&DDSHeader, sizeof(DDSHeader));

		// write out the dxt data
		void* Src = Texture->Mips(FirstMipIndex).Data.Lock(LOCK_READ_ONLY);
		DDSFile->Serialize(Src, DDSHeader.dwLinearSize);
		Texture->Mips(FirstMipIndex).Data.Unlock();

		// that's it, close the file
		delete DDSFile;

		// convert it
		FString Params = FString::Printf(TEXT("-yflip0 -fOGL8888 -i%s -o%s"), *InputFilePath, *OutputFilePath);
		void* Proc = appCreateProc(TEXT("..\\Redist\\ImgTec\\PVRTexTool.exe"), *Params, TRUE, FALSE, FALSE, NULL, -1);

		// wait for the process to complete
		INT ReturnCode;
		while (!appGetProcReturnCode(Proc, &ReturnCode))
		{
			appSleep(0.01f);
		}

		TArray<BYTE> Data;
		appLoadFileToArray(Data, *OutputFilePath);

		// process it
		FPVRHeader* Header = (FPVRHeader*)Data.GetData();
		// only copy as many colors as went into the dds file, since we may have dropped a mip
		appMemcpy(RawData.GetData(), Data.GetTypedData() + Header->HeaderSize, DDSHeader.dwWidth * DDSHeader.dwHeight * 4);
	    
		// Delete the temp files
	    GFileManager->Delete( *InputFilePath );
	    GFileManager->Delete( *OutputFilePath ); 

	}

	// at this point, RawData has RGBA8 data, possibly non square, which needs to be fixed
	if (FirstMipSizeX != FirstMipSizeY)
	{
		// calculate how many times to dup each row or column
		UINT MultX = FirstMipSizeSquare / FirstMipSizeX;
		UINT MultY = FirstMipSizeSquare / FirstMipSizeY;

		// allocate room to fill out into
		TArray<FColor> SquareRawData;
		SquareRawData.Add(FirstMipSizeSquare * FirstMipSizeSquare);

		DWORD* RectData = (DWORD*)RawData.GetData();
		DWORD* SquareData = (DWORD*)SquareRawData.GetData();

		// raw data is now at DataOffset, so we can now duplicate rows/columns into the square data
		for (UINT Y = 0; Y < FirstMipSizeY; Y++)
		{
			for (UINT X = 0; X < FirstMipSizeX; X++)
			{
				// get where the non-duplicated source color is
				DWORD SourceColor = *(RectData + Y * FirstMipSizeX + X);

				for (UINT YDup = 0; YDup < MultY; YDup++)
				{
					for (UINT XDup = 0; XDup < MultX; XDup++)
					{
						// get where to write the duplicated color to
						DWORD* DestColor = SquareData + ((Y * MultY + YDup) * FirstMipSizeSquare + (X * MultX + XDup));
						*DestColor = SourceColor;
					}
				}
			}
		}

		// now copy the new square data on top of original RawData
		RawData = SquareRawData;
	}


	// we now have square RGBA8 data, ready for PVRTC compression

	struct FPVRHeader
	{
		UINT HeaderSize;
		UINT Height;
		UINT Width;
		UINT NumMips;
		UINT Flags;
		UINT DataLength;
		UINT BitsPerPixel;
		UINT BitmaskRed;
		UINT BitmaskGreen;
		UINT BitmaskBlue;
		UINT BitmaskAlpha;
		UINT PVRTag;
		UINT NumSurfaces;
	};

	FPVRHeader PVRHeader;
	PVRHeader.HeaderSize = sizeof(PVRHeader);
	PVRHeader.Height = FirstMipSizeSquare;
	PVRHeader.Width = FirstMipSizeSquare;
	PVRHeader.NumMips = 0;
	PVRHeader.Flags = 0x12 | 0x8000; // OGL8888 | AlphaInTexture
	PVRHeader.DataLength = PVRHeader.Height * PVRHeader.Width * 4;
	PVRHeader.BitsPerPixel = 32;
	PVRHeader.BitmaskRed = 0xFF;
	PVRHeader.BitmaskGreen = 0xFF00;
	PVRHeader.BitmaskBlue = 0xFF00000;
	PVRHeader.BitmaskAlpha = 0xFF000000;
	PVRHeader.PVRTag = '!RVP';
	PVRHeader.NumSurfaces = 1;

	FString InputFilePath = GSys->CachePath * FString::Printf( TEXT("%s_%d_RGBToPVRIn.pvr"),*Texture->GetName(), ProcessID );
	FString OutputFilePath = GSys->CachePath * FString::Printf( TEXT("%s_%d_RGBToPVROut.pvr"),*Texture->GetName(), ProcessID );
	FArchive* PVRFile = GFileManager->CreateFileWriter(*InputFilePath);

	// write out header
	PVRFile->Serialize(&PVRHeader, PVRHeader.HeaderSize);

	// write out the dxt data
	PVRFile->Serialize(RawData.GetData(), PVRHeader.DataLength);

	// that's it, close the file
	delete PVRFile;

	// figure out whether or not to use 2 bits per pixel (as opposed to 4)
	UBOOL bIsPVRTC2 = Texture->Format == PF_DXT1 && !Texture->bForcePVRTC4;

	// we now have a file on disk that can be converted
	FString Params = FString::Printf(TEXT("-m %s -fOGLPVRTC%d -yflip0 -i%s -o%s"), 
		bUseFastCompression ? TEXT("-pvrtciterations1 -pvrtcfast") : TEXT("-pvrtciterations8"),
		bIsPVRTC2 ? 2 : 4, *InputFilePath, *OutputFilePath);

	// explain what's happening, (and possibly tell the cooker that we are performing a repeating slow operation and how to fix it)
	if (GIsUCC)
	{
		warnf(TEXT("%s compressing %s to PVRTC%d...%s"), 
			bUseFastCompression ? TEXT("FAST") : TEXT("SLOW"), 
			*Texture->GetFullName(), 
			bIsPVRTC2 ? 2 : 4,
			GIsCooking ? TEXT(" resave the package to fix this.") : TEXT(""));
	}
	
	void* Proc = appCreateProc(TEXT("..\\Redist\\ImgTec\\PVRTexTool.exe"), *Params, TRUE, FALSE, FALSE, NULL, -1);
	
	// wait for the process to complete
	INT ReturnCode;
	while (!appGetProcReturnCode(Proc, &ReturnCode))
	{
		appSleep(0.01f);
	}

	// did it work?
	UBOOL bConversionWasSuccessful = ReturnCode == 0;

	// if the conversion worked, open up the output file, and get the mip data from within
	if (bConversionWasSuccessful)
	{
		// read compressed data
		TArray<BYTE> PVRData;
		appLoadFileToArray(PVRData, *OutputFilePath);

		// process it
		FPVRHeader* Header = (FPVRHeader*)PVRData.GetData();
		INT FileOffset = Header->HeaderSize;

		for (INT MipLevel = 0; MipLevel < Texture->Mips.Num(); MipLevel++)
		{
			// calculate the size of the mip in bytes for PVR data
			UINT MipSizeX = Max<UINT>(TexSizeSquare >> MipLevel, 1);
			UINT MipSizeY = MipSizeX; // always square
			
			// PVRTC2 uses 8x4 blocks
			// PVRTC4 uses 4x4 blocks
			// min 2x2 blocks per mip
			UINT BlocksX = Max<UINT>(MipSizeX / (bIsPVRTC2 ? 8 : 4), 2); 
			UINT BlocksY = Max<UINT>(MipSizeY / 4, 2);
			
			// both are 8 bytes per block
			UINT MipSize = BlocksX * BlocksY * 8;

			FTexture2DMipMap* NewMipMap = new(Texture->CachedPVRTCMips) FTexture2DMipMap;

			// fill out the mip using data from the converted file
			NewMipMap->SizeX = MipSizeX;
			NewMipMap->SizeY = MipSizeY;
			NewMipMap->Data.Lock(LOCK_READ_WRITE);
			void* MipData = NewMipMap->Data.Realloc(MipSize);

			// if we skipped over any mips, don't attempt to get them out of the converted file, just
			// use black instead (and let's hope the final target won't render that large a mip)
			if (MipLevel < FirstMipIndex)
			{
				appMemzero(MipData, MipSize);
			}
			else
			{
				// copy mip data from the file data
				appMemcpy(MipData, PVRData.GetTypedData() + FileOffset, MipSize);

				// move to next mip
				FileOffset += MipSize;
			}

			NewMipMap->Data.Unlock();
		}
	}

	// Delete the temp files
	GFileManager->Delete( *InputFilePath );
	GFileManager->Delete( *OutputFilePath ); 

	return TRUE;
}

/**
 * Cook any compressed textures that don't have cached cooked data
 *
 * @param Package Package to cache textures for
 * @param bIsSilent If TRUE, don't show the slow task dialog
 * @param WorldBeingSaved	The world that is currently being saved.  NULL if no world is being saved.
 */
void PreparePackageForMobile(UPackage* Package, UBOOL bIsSilent, UWorld* WorldBeingSaved )
{
	// should we flatten materials at all?
	UBOOL bShouldFlattenMaterials = FALSE;
	GConfig->GetBool(TEXT("MobileSupport"), TEXT("bShouldFlattenMaterials"), bShouldFlattenMaterials, GEngineIni);
	if (bShouldFlattenMaterials)
	{
		checkf(GEditor != NULL, TEXT("Only the editor can flatten materials!"));

		// disable screen savers while flattening so that we don't get black textures
		GEngine->EnableScreenSaver( FALSE );

		// flatten any materials that need it
		for (TObjectIterator<UMaterialInterface> It; It; ++It)
		{
			if (It->IsIn(Package))
			{
				const UBOOL bReflattenAutoFlattened = FALSE;
				const UBOOL bForceFlatten = FALSE;
				// re-flatten if necessary, passing FALSE so that if the texture is already auto-flattened to NOT reflatten
				ConditionalFlattenMaterial(*It, bReflattenAutoFlattened, bForceFlatten);
			}
		}

		// reenable screensaver
		GEditor->EnableScreenSaver( TRUE );
	}

	// should we cache PVRTC textures at all?
	UBOOL bShouldCachePVRTCTextures = FALSE;
	GConfig->GetBool(TEXT("MobileSupport"), TEXT("bShouldCachePVRTCTextures"), bShouldCachePVRTCTextures, GEngineIni);

	if (bShouldCachePVRTCTextures)
	{
		TArray<UTexture2D*> TexturesToConvert;
		for( TObjectIterator<UTexture2D> It; It; ++It )
		{
			UTexture2D* Texture = *It;

			// any textures without cached IPhone data need cooking
			if (Texture->IsIn(Package) && ShouldCompressToPVRTC(Texture))
			{
				TexturesToConvert.AddItem(Texture);
			}
		}

		if (TexturesToConvert.Num())
		{
			if (!bIsSilent)
			{
				GWarn->BeginSlowTask(TEXT(""), TRUE);
			}
			// Create a pvr compressor to help with multithreaded compression.
			FAsyncPVRTCCompressor PVRCompressor;

			for (INT TextureIndex = 0; TextureIndex < TexturesToConvert.Num(); TextureIndex++)
			{
				UTexture2D* Texture = TexturesToConvert(TextureIndex);
				// make sure the texture is compressed if needed
				Texture->PreSave();

				// By default do not use fast PVRTC
				UBOOL bUseFastPVR = FALSE;
				if( ( Texture->IsA( ULightMapTexture2D::StaticClass() ) || Texture->IsA( UShadowMapTexture2D::StaticClass() ) ) && WorldBeingSaved )
				{
					//Use fast PVR compression if we are on a low quality lighting build and are compressing lightmaps
					bUseFastPVR = WorldBeingSaved->GetWorldInfo()->LevelLightingQuality != Quality_Production;
				}
				// Add this texture to the list of textures needing compression
				PVRCompressor.AddTexture( Texture, bUseFastPVR );
			}

			// Wait for all textures to be compressed.
			PVRCompressor.CompressTextures();

			if (!bIsSilent)
			{
				GWarn->EndSlowTask();
			}

			// mark package dirty
			Package->MarkPackageDirty(TRUE);
		}
	}
}

