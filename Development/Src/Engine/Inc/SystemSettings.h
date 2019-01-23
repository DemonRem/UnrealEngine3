/*=============================================================================
	SystemSettings.cpp: Unreal engine HW compat scalability system.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	System settings and scalability options.
-----------------------------------------------------------------------------*/

struct FSystemSettings : public FExec
{
	/** Constructor, initializing all member variables. */
	FSystemSettings();

	/** Empty virtual destructor. */
	virtual ~FSystemSettings()
	{}

	/**
	 * Initializes system settings and included texture LOD settings.
	 *
	 * @param bSetupForEditor	Whether to initialize settings for Editor
	 */
	void Initialize( UBOOL bSetupForEditor );

	/**
	 * Exec handler implementation.
	 *
	 * @param Cmd	Command to parse
	 * @param Ar	Output device to log to
	 *
	 * @return TRUE if command was handled, FALSE otherwise
	 */
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );

	/**
	 * Scale X,Y offset/size of screen coordinates if the screen percentage is not at 100%
	 *
	 * @param X - in/out X screen offset
	 * @param Y - in/out Y screen offset
	 * @param SizeX - in/out X screen size
	 * @param SizeY - in/out Y screen size
	 */
	void ScaleScreenCoords( INT& X, INT& Y, UINT& SizeX, UINT& SizeY );

	/**
	 * Updates the lightmap type on all primitives that store references to lighmaps.
	 * This function will disallow changing bAllowDirectionalLightMaps on cooked builds.
	 */
	void UpdateLightmapType(UBOOL bOldAllowDirectionalLightMaps);

	/**
	 * Misc options.
	 */

	/** Global texture LOD settings.									*/
	FTextureLODSettings TextureLODSettings;

	/**
	 * Scalability options.
	 */

	/** Whether to allow static decals.									*/
	UBOOL	bAllowStaticDecals;
	/** Whether to allow dynamic decals.								*/
	UBOOL	bAllowDynamicDecals;

	/** Whether to allow dynamic lights.								*/
	UBOOL	bAllowDynamicLights;
	/** Whether to allow dynamic shadows.								*/
	UBOOL	bAllowDynamicShadows;
	/** Whether to allow dynamic light environments to cast shadows.	*/
	UBOOL	bAllowLightEnvironmentShadows;
	/** Whether to composte dynamic lights into light environments.		*/
	UBOOL	bUseCompositeDynamicLights;
	/** Quality bias for projected shadow buffer filtering.	 Higher values use better quality filtering.	*/
	INT		ShadowFilterQualityBias;
	/** 
	 * Whether to allow directional lightmaps, which use the material's normal and specular.	
	 */
	UBOOL	bAllowDirectionalLightMaps;

	/** Whether to allow motion blur.									*/
	UBOOL	bAllowMotionBlur;
	/** Whether to allow depth of field.								*/
	UBOOL	bAllowDepthOfField;
	/** Whether to allow bloom.											*/
	UBOOL	bAllowBloom;
	/** Whether to use high quality bloom or fast versions.				*/
	UBOOL	bUseHighQualityBloom;

	/** Whether to allow rendering of SpeedTree leaves.					*/
	UBOOL	bAllowSpeedTreeLeaves;
	/** Whether to allow rendering of SpeedTree fronds.					*/
	UBOOL	bAllowSpeedTreeFronds;

	/** If enabled, texture will only be streamed in, not out.			*/
	UBOOL	bOnlyStreamInTextures;

	/** Whether to allow rendering of LensFlares.						*/
	UBOOL	bAllowLensFlares;

	/** LOD bias for skeletal meshes.									*/
	INT		SkeletalMeshLODBias;
	/** LOD bias for particle systems.									*/
	INT		ParticleLODBias;

	/** Percentage of screen main view should take up.					*/
	FLOAT	ScreenPercentage;

	/**
	 * Current detail mode; determines whether components of actors
	 * should be updated/ ticked.
	 */
	INT		DetailMode;
	
	/**
	 * System settings.
	 */

protected:
	/** Amount of total system memory in MByte.							*/
	INT		CPUMemory;
	/** CPU Frequency in MHz											*/
	INT		CPUFrequency;
	/** Number of physical CPUs, disregarding hyper-threading.			*/
	INT		CPUCount;
	/** CPU vendor string.												*/
	FString	CPUVendorString;

	/** Supported pixel shader model * 10, so 30 for 3.0				*/
	INT		GPUShaderModel;
	/** On-board GPU memory in MByte.									*/
	INT		GPUMemory;
	/** GPU vendor id.													*/
	INT		GPUVendorId;
	/** GPU device id.													*/
	INT		GPUDeviceId;
};

/** Global scalability object. */
extern FSystemSettings GSystemSettings;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

