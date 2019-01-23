/*=============================================================================
	UnObjVer.h: Unreal object version.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Prevents incorrect files from being loaded.

#define PACKAGE_FILE_TAG			0x9E2A83C1
#define PACKAGE_FILE_TAG_SWAPPED	0xC1832A9E

// Cast to ensure that the construct cannot be used in a #if without compiler error.
// This is useful as enum vales cannot be seen by the preprocessor.
#define PREPROCESSOR_ENUM_PROTECT(a) ((unsigned int)(a))

// This is the only VER that needs to be done as #define because it is used
// with #ifdef VER_MESH_PAINT_SYSTEM
#define VER_MESH_PAINT_SYSTEM 615

//
// Package file version log:
//
// Note: The naming convention here is like for macros (#define) because
//       those have been #defines before and now became enums. This can be changed later.
//
enum EUnrealEngineObjectVersion
{
	// - Min version for content resave
	VER_CONTENT_RESAVE_AUGUST_2007_QA_BUILD				= 491,
	// - Static mesh version bump, package version bumped to ease resaving
	VER_STATICMESH_VERSION_16							= 492,
	// - Used 16 bit float UVs for skeletal meshes
	VER_USE_FLOAT16_SKELETAL_MESH_UVS					= 493,
	// - Store two tangent basis vectors instead of three to save memory (skeletal mesh vertex buffers)
	VER_SKELETAL_MESH_REMOVE_BINORMAL_TANGENT_VECTOR	= 494,
	// - Terrain collision data stored in world space.
	VER_TERRAIN_COLLISION_WORLD_SPACE					= 495,
	// - Removed DecalManager ref from UWorld
	VER_REMOVED_DECAL_MANAGER_FROM_UWORLD				= 496,
	// - Modified SpeedTree vertex factory shader parameters.
	VER_SPEEDTREE_SHADER_CHANGE							= 497,
	// - Fix height-fog pixel shader 4-layer 
	VER_HEIGHTFOG_PIXELSHADER_START_DIST_FIX			= 498,
	// - MotionBlurShader recompile (added clamping to render target extents)
	VER_MOTIONBLURSHADER_RECOMPILE_VER2					= 499,
	// - Separate pass for LDR BLEND_Modulate transparency mode
	// - Modulate preserves dest alpha (depth)
	VER_SM2_BLENDING_SHADER_FIXES						= 500,
	// - Terrain material fallback support
	VER_ADDED_TERRAIN_MATERIAL_FALLBACK					= 501,
	// - Added support for multi-column collections to UIDynamicFieldProvider
	VER_ADDED_MULTICOLUMN_SUPPORT						= 503,
	// - Serialize cached displacement values for terrain
	VER_TERRAIN_SERIALIZE_DISPLACEMENTS					= 504,
	// - Fixed bug which allowed multiple instances of a UIState class get added to style data maps
	VER_REMOVED_PREFAB_STYLE_DATA						= 505,
	// - Exposed separate horizontal and vertical texture scale for material texture lookups
	// -  Various font changes that affected serialization
	VER_FONT_FORMAT_AND_UV_TILING_CHANGES				= 506,
	// - Changed UTVehicleFactory to use a string for class reference in its defaults
	VER_UTVEHICLEFACTORY_USE_STRING_CLASS				= 507,
	// - Fixed the special 0.0f value in the velocity buffer that is used to select between background velocity or dynamic velocity
	VER_BACKGROUNDVELOCITYVALUE							= 508,
	// - Reset vehicle usage flags on some NavigationPoints that had been incorrectly set
	VER_FIXED_NAV_VEHICLE_USAGE_FLAGS					= 509,
	// - Changed Texture2DComposite to inherit from Texture instead of Texture2D.
	VER_TEXTURE2DCOMPOSITE_BASE_CHANGE					= 510,
	// - Fixed fonts serializing all members twice.
	VER_FIXED_FONTS_SERIALIZATION						= 511,
	// - Bump to break full version content to work with demo
	VER_FULL_VERSION_OF_UT3_BUMP						= 512,
	// - Throw away Atrac3 data, create MP3 data
	VER_UPGRADE_TO_MP3									= 513,
	// - 
	VER_STATICMESH_FRAGMENTINDEX						= 514,
	// - Added Draw SkelTree Manager. Added FColor to FMeshBone serialization.	
	VER_SKELMESH_DRAWSKELTREEMANAGER					= 515,
	// - Added AdditionalPackagesToCook to FPackageFileSummary	
	VER_ADDITIONAL_COOK_PACKAGE_SUMMARY					= 516,
	// - Add neighbor info to FFragmentInfo
	VER_FRAGMENT_NEIGHBOUR_INFO							= 517,
	// - Added interior fragment index
	VER_FRAGMENT_INTERIOR_INDEX							= 518,
	// - Added bCanBeDestroyed and bRootFragment
	VER_FRAGMENT_DESTROY_FLAGS							= 519,
	// - Add exterior surface normal and neighbor area info to FFragmentInfo
	VER_FRAGMENT_EXT_NORMAL_NEIGH_DIM					= 520,
	// - Add core mesh 3d offset and scale
	VER_FRACTURE_CORE_SCALE_OFFSET						= 521,
	// - Updated mp3 format to account for multichannel sounds
	VER_MP3_FORMAT_UPDATE								= 522,
	// - Moved particle SpawnRate and Burst info into their own module.
	VER_PARTICLE_SPAWN_AND_BURST_MOVE					= 523,
	// - Share modules across particle LOD levels where possible.
	VER_PARTICLE_LOD_MODULE_SHARE						= 524,
	// - Fixing up TypeData modules not getting pushed into lower LODs
	VER_PARTICLE_LOD_MODULE_TYPEDATA_FIXUP				= 525,
	// - Save off PlaneBias with FSM
	VER_FRACTURE_SAVE_PLANEBIAS							= 526,
	// - Fixing up LOD distributions... (incorrect archetypes caused during Spawn conversion)
	VER_PARTICLE_LOD_DIST_FIXUP							= 527,
	// - Added DiffusePower to material inputs
	VER_DIFFUSEPOWER									= 528,
	// - Changed default DiffusePower value
	VER_DIFFUSEPOWER_DEFAULT							= 529,
	// - Allow for '0' in the particle burst list CountLow slot...
	VER_PARTICLE_BURST_LIST_ZERO						= 530,
	// - Added AttenAllowedParameter to FModShadowMeshPixelShader
	VER_MODSHADOWMESHPIXELSHADER_ATTENALLOWED			= 531,
	// - Support for mesh simplification tool.  Static mesh version bump (added named reference to high res source mesh.)
	VER_STATICMESH_VERSION_18							= 532,
	// - Added automatic fog volume components to simplify workflow
	VER_AUTOMATIC_FOGVOLUME_COMPONENT					= 533,
	// - Added an optional array of skeletal mesh weights/bones for instancing 
	VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCES			= 534,
	// - Added an optional array of skeletal mesh weights/bones for instancing 
	VER_UNIFORM_DISTRIBUTION_BAKING_UPDATE				= 535,
	// - Replaced classes for sequences associated with PrefabInstances
	VER_FIXED_PREFAB_SEQUENCES							= 536,
	// - Changed FInputKeyAction's list of sequence actions to a list of sequence output links
	VER_MADE_INPUTKEYACTION_OUTPUT_LINKS				= 537,
	// - Moved global shaders from UShaderCache to a single global shader cache file.
	VER_GLOBAL_SHADER_FILE								= 538,
	// - Using MSEnc to encode mp3s rather than MP3Enc
	VER_MP3ENC_TO_MSENC									= 539,
	// - Fixing up LODValidity...
	VER_EMITTER_LODVALIDITY_FIX							= 540,
	// - Added optional external specification of static vertex normals.
	VER_STATICMESH_EXTERNAL_VERTEX_NORMALS				= 541,
	// - Removed 2x2 normal transform for decal materials
	VER_DECAL_MATERIAL_IDENDITY_NORMAL_XFORM			= 542,
	// - Removed FObjectExport::ComponentMap
	VER_REMOVED_COMPONENT_MAP							= 543,
	// - Fixed back uniform distributions with lock flags set to something other than NONE
	VER_LOCKED_UNIFORM_DISTRIBUTION_BAKING				= 544,
	// - Fixed Kismet sequences with illegal names
	VER_FIXED_KISMET_SEQUENCE_NAMES						= 545,
	// - Added fluid lightmap support
	VER_ADDED_FLUID_LIGHTMAPS							= 546,
	// - Fixing up LODValidity and spawn module outers...
	VER_EMITTER_LODVALIDITY_FIX2						= 547,
	// - Fixing incorrect default properties for new foliage parameters
	VER_FIX_DEFAULT_FOLIAGE_PARAMETERS					= 548,
	// - Add FSM core rotation and 'no physics' flag on chunks
	VER_FRACTURE_CORE_ROTATION_PERCHUNKPHYS				= 549,
	// - New curve auto-tangent calculations; Clamped auto tangent support
	VER_NEW_CURVE_AUTO_TANGENTS							= 550,
	// - Removed 2x2 normal transform from decal vertices 
	VER_DECAL_REMOVED_2X2_NORMAL_TRANSFORM				= 551,
	// - Updated decal vertex factories
	VER_DECAL_VERTEX_FACTORY_VER1						= 552,
	// - Updated fluid vertex factories
	VER_FLUID_VERTEX_FACTORY							= 553,
	// - Updated decal vertex factories
	VER_DECAL_VERTEX_FACTORY_VER2						= 554,
	// - Updated the fluid detail normalmap
	VER_FLUID_DETAIL_UPDATE								= 555,
	// - Fixup particle systems with incorrect distance arrays...
	VER_PARTICLE_LOD_DISTANCE_FIXUP						= 556,
	// - Added FSM build version
	VER_FRACTURE_NONCRITICAL_BUILD_VERSION				= 557,
	// - Added DynamicParameter support for particles
	VER_DYNAMICPARAMETERS_ADDED							= 558,
	// - Added travelspeed parameter to the fluid detail normalmap
	VER_FLUID_DETAIL_UPDATE2							= 559,
	// - /** replaced bAcceptsDecals,bAcceptsDecalsDuringGameplay with bAcceptsStaticDecals,bAcceptsDynamicDecals */
	VER_UPDATED_DECAL_USAGE_FLAGS						= 560,
	// - incremented DOFBloomGather pixel version; added SceneMultipler
	VER_DOFBLOOMGATHER_SCENEMULTIPLIER					= 561,
	// - Added bounced lighting settings to LightComponent
	VER_LIGHTCOMPONENT_BOUNCEDLIGHTING					= 562,
	// - Made bOverrideNormal override the full tangent basis.
	VER_OVERRIDETANGENTBASIS							= 563,
	// - Made LightComponent bounced lighting settings multiplicative with direct lighting.
	VER_BOUNCEDLIGHTING_DIRECTMODULATION				= 564,
	// - Added a shader parameter to the FDistortionApplyScreenPixelShader
	VER_DISTORTIONAPPLYPIXELSHADER_UPDATE				= 565,
	// - Reduced FStateFrame::LatentAction to WORD
	VER_REDUCED_STATEFRAME_LATENTACTION_SIZE			= 566,
	// - Added GUIDs for updating texture file cache
	VER_ADDED_TEXTURE_FILECACHE_GUIDS					= 567,
	// - Fixed scene color and scene depth usage
	VER_FIXED_SCENECOLOR_USAGE							= 568,
	// - Renamed UPrimitiveComponent::CullDistance to MaxDrawDistance
	VER_RENAMED_CULLDISTANCE							= 569,
	// - Fixing up InterpolationMethod mismatches in emitter LOD levels...
	VER_EMITTER_INTERPOLATIONMETHOD_FIXUP				= 570,
	// - Fixing up LensFlare ScreenPercentageMaps
	VER_LENSFLARE_SCREENPERCENTAGEMAP_FIXUP				= 571,
	// - Updated decal vertex factories
	VER_DECAL_VERTEX_FACTORY_VER3						= 572,
	// - Reimplemented particle LOD check distance time
	VER_PARTICLE_LOD_CHECK_DISTANCE_TIME_FIX			= 573,
	// - Decal physical material entry fixups
	VER_DECAL_PHYS_MATERIAL_ENTRY_FIXUP					= 574,
	// - Added persisitent FaceFXAnimSet to the world...
	VER_WORLD_PERSISTENT_FACEFXANIMSET					= 575,
	// - depcreated redundant editor window position
	// - Delete var - SkelControlBase: ControlPosX, ControlPosY, MaterialExpression: EditorX, EditorY
	VER_DEPRECATED_EDITOR_POSITION						= 576,
	// - moved RawAnimData serialization to native
	VER_NATIVE_RAWANIMDATA_SERIALIZATION				= 577,
	// - deprecated sound attenuation ranges
	VER_DEPRECATE_SOUND_RANGES							= 578,
	// - ambient sound update
	VER_AMBIENT_SOUND_UPDATE							= 579,
	// - new conversion required for multichannel sounds
	VER_XAUDIO2_MULTICHANNEL_UPDATE						= 580,
	// - new format stored in the XMA2 file to avoid runtime calcs
	VER_XAUDIO2_FORMAT_UPDATE							= 581,
	// - flip the normal for meshes with negative non-uniform scaling
	VER_VERTEX_FACTORY_LOCALTOWORLD_FLIP				= 582,
	// - add additional sort flags to sprite/subuv particle emitters
	VER_NEW_PARTICLE_SORT_MODES							= 583,
	// - added asset thumbnails to packages
	VER_ASSET_THUMBNAILS_IN_PACKAGES					= 584,
	// - Added Pylon list to Ulevel
	VER_PYLONLIST_IN_ULEVEL								= 585,
	// - Added local object version number to ULevel and NavMesh
	VER_NAVMESH_COVERREF								= 586,
	// - Updates and replaces several kismet objects 
	VER_CONVERT_KISMET_OBJECTS							= 587,
	// - poly height var added to polygons in navmesh
	VER_NAVMESH_POLYHEIGHT								= 588,
	// - simple element shader recompile
	VER_SIMPLE_ELEMENT_SHADER_VER0						= 589,
	// - added rectangular thumbnail support
	VER_RECTANGULAR_THUMBNAILS_IN_PACKAGES				= 590,
	// - changed default for SkeletalMeshActor.bCollideActors to FALSE
	VER_REMOVED_DEFAULT_SKELETALMESHACTOR_COLLISION		= 591,
	// - added skeletalmesh position compression saving 8 bytes
	VER_SKELETAL_MESH_SUPPORT_PACKED_POSITION			= 592,
	// - removed content tags from objects (obsolete by new asset database system)
	VER_REMOVED_LEGACY_CONTENT_TAGS						= 593,
	// - added back refs for SplineActors
	VER_ADDED_SPLINEACTOR_BACK_REFS						= 594,
	// - Changed the format of the base pose for additive animations.
	VER_NEW_BASE_POSE_ADDITIVE_ANIM_FORMAT				= 595,
	// - Fix up 'Bake and Prune' animations where their num frames doesn't match NumKeys.
	VER_FIX_BAKEANDPRUNE_NUMFRAMES						= 596,
	// - added full names to package thumbnails
	VER_CONTENT_BROWSER_FULL_NAMES						= 597,
	// - added profiling system to AnimTree previewing
	VER_ANIMTREE_PREVIEW_PROFILES						= 598,
	// - added triangle sorting options to skeletal meshes
	VER_SKELETAL_MESH_SORTING_OPTIONS					= 599,
	// - Lightmass serialization changes
	VER_INTEGRATED_LIGHTMASS							= 600,
	// - added BoneAtom quaternion math support and convert vars from Matrix
	VER_FBONEATOM_QUATERNION_TRANSFORM_SUPPORT			= 601,
	// - deprecate distributions from sound nodes
	VER_DEPRECATE_SOUND_DISTRIBUTIONS					= 602,
	// - added DontSortCategories option to classes
	VER_DONTSORTCATEGORIES_ADDED						= 603,
	// - Reintroduced lossless compression of Raw Data, and removed redundant KeyTimes array.
	VER_RAW_ANIMDATA_REDUX								= 604,
	// - Fixed bad additive animation base pose data
	VER_FIXED_BAD_ADDITIVE_DATA							= 605,
	// - Add per-poly procbuilding ruleset pointer
	VER_ADD_FPOLY_PBRULESET_POINTER						= 606,
	// - Added precomputed lighting volume to each level
	VER_GI_CHARACTER_LIGHTING							= 607,
	// - SkeletalMesh Compose now done in 3 passes as opposed to 2.
	VER_THREE_PASS_SKELMESH_COMPOSE						= 608,
	// - Added bone influence mapping data per bone break
	VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCE_MAPPING   = 609,
	// - Fix bad AnimSequences.
	VER_REMOVE_BAD_ANIMSEQ								= 610,
	// - added dual quaternion for skeletalmesh skinning
	VER_SKELETAL_MESH_DUAL_QUATERNION_SKINNING			= 611,
	// - disabled dual quaternion and need to bump this again for shader
	VER_SKELETAL_MESH_DISABLE_DQ_SKINNING				= 612,
	// - added editor data to sound classes
	VER_SOUND_CLASS_SERIALISATION_UPDATE				= 613,
	// - older maps may have improper ProcBuilding textures
	VER_NEED_TO_CLEANUP_OLD_BUILDING_TEXTURES			= 614,
	// - Mesh paint system
	VER_MESH_PAINT_SYSTEM_ENUM							= VER_MESH_PAINT_SYSTEM,
	// - Added ULightMapTexture2D::bSimpleLightmap
	VER_LIGHTMAPTEXTURE_VARIABLE						= 616,
	// - Normal shadows on the dominant light
	VER_DOMINANTLIGHT_NORMALSHADOWS						= 617,
	// - Added PlatformMeshData to mesh elements (for PS3 Edge Geometry support)
	VER_ADDED_PLATFORMMESHDATA							= 618,
	// - Disable optimizations (for now) on FSplineMeshVertexFactory to get rid of black flickering.
	VER_DISABLE_SPLINEMESH_OPTIMIZATIONS				= 619,
	// - changed makeup of FPolyReference
	VER_FPOLYREF_CHANGE									= 620,
	// - Added bsp element index to the serialized static receiver data for decals
	VER_DECAL_SERIALIZE_BSP_ELEMENT						= 621,
	// - Added option to spline deform for linear or hermite scale/roll interpolation
	VER_SPLINE_DEFORM_SMOOTH_OPTION						= 622,
	// - Added support for automatic, safe cross-level references
	VER_ADDED_CROSSLEVEL_REFERENCES						= 623,
	// - Changed lightmap encoding to only use two DXT1 textures for directional lightmaps
	VER_MAXCOMPONENT_LIGHTMAP_ENCODING					= 624,
	// - Added instanced rendering to localvertexfactory
	VER_XBOXINSTANCING									= 625,
	// - Fixing up emitter editor color issue.
	VER_FIXING_PARTICLE_EMITTEREDITORCOLOR				= 626,
	// - Added OriginalSizeX/Y to Texture2D
	VER_ADDED_TEXTURE_ORIGINAL_SIZE						= 627,
	// - Added options to generate particle normals from simple shapes
	VER_ANALYTICAL_PARTICLE_NORMALS						= 628,
	// - Spline VF recompile now that the debug flag has been removed
	VER_SPLINEVF_REMOVED_DEBUG							= 629,
	// - Fixup references to removed deprecated ParticleEmitter.SpawnRate
	VER_REMOVED_EMITTER_SPAWNRATE						= 630,
	// - Add support for static normal parameters
	VER_ADD_NORMAL_PARAMETERS							= 631,
	// - Changed UParticleSystem::bLit to be per-LOD
	VER_PARTICLE_LIT_PERLOD								= 632,
	// - Changed byte property serialization to include the enum the property uses (if any)
	VER_BYTEPROP_SERIALIZE_ENUM							= 633,
	// - Added InternalFormatLODBias
	VER_ADDED_TEXTURE_INTERNALFORMATLODBIAS				= 634,
	// - Refactoring particle event receiver classes
	VER_PARTICLE_EVENT_RECEIVER_REFACTOR				= 635,
	// - Added an explicit emissive light radius
	VER_ADDDED_EXPLICIT_EMISSIVE_LIGHT_RADIUS			= 636,
	// - Enabled Custom Thumbnails for shared thumbnail asset types
	VER_ENABLED_CUSTOM_THUMBNAILS_FOR_SHARED_TYPES		= 637,
	// - Added AnimMetaData system to AnimSequence, auto conversion of BoneControlModifiers to that new system.
	// - Fixed FQuatError, automatic animation recompression when needed.
	VER_ADDED_ANIM_METADATA_FIXED_QUATERROR				= 638,
	// - Changed UStruct serialization to include both on-disk and in-memory bytecode size
	VER_USTRUCT_SERIALIZE_ONDISK_SCRIPTSIZE				= 639,
	// - Added new ShadowmapCoordinateScaleBias shader parameter for mesh instancing
	VER_INSTANCED_MESH_SHADOWMAPS						= 640,
	// - Changed the default distance model for the SoundNodeAttenuation class from ATTENUATION_Logarithmic to ATTENUATION_Linear.
	VER_SOUNDNODEATTENUATION_DISTANCEMODEL_CHANGE		= 641,
	// - Added support for spline mesh offsetting
	VER_ADDED_SPLINE_MESH_OFFSET						= 642,
	// - Speedtree 5.0 integration
	VER_SPEEDTREE_5_INTEGRATION							= 643,
	// - Added selected object coloring to Lightmap Density rendering mode
	VER_LIGHTMAP_DENSITY_SELECTED_OBJECT				= 644,
	// - Added LightmapUVs expression
	VER_MATEXP_LIGHTMAPUVS_ADDED						= 645,
	// - Switched AnimMetadata_SkelControl to using a list.
	VER_SKELCONTROL_ANIMMETADATA_LIST					= 646,
	// - Added material vertex shader parameters
	VER_MATERIAL_EDITOR_VERTEX_SHADER					= 647,
	// - Freed up an extra shader constant in the GPU skinning shader (needed for translucent decals)
	VER_FREE_GPUSKIN_SHADER_CONSTANT					= 648,
	// - Added per-object foliage shader parameters
	VER_ADDED_FOLIAGE_PARAMETERS						= 649,
	// - Fixed hit proxy material parameters not getting serialized
	VER_FIXED_HIT_PROXY_VERTEX_OFFSET					= 650,
	// - Added general OcclusionPercentage material expression
	VER_ADDDED_OCCLUSION_PERCENTAGE_EXPRESSION			= 651,
	// - Added the ability to shadow indirect only in Lightmass
	VER_SHADOW_INDIRECT_ONLY_OPTION						= 652,
	// - Changed mesh emitter camera facing options...
	VER_MESH_EMITTER_CAMERA_FACING_OPTIONS				= 653,
	// - Replaced bSimpleLightmap with LightmapFlags in ULightMapTexture2D
	VER_LIGHTMAPFLAGS									= 654,
	// - Added the ability for script to bind DLL functions
	VER_SCRIPT_BIND_DLL_FUNCTIONS						= 655,
	// - Moved uniform expressions from being stored in the UMaterial package to the shader cache
	VER_UNIFORM_EXPRESSIONS_IN_SHADER_CACHE				= 656,
	// - Added dynamic parameter support and second uv set to beams and trails
	VER_BEAM_TRAIL_DYNAMIC_PARAMETER					= 657,
	// - Forced a recompile of vertex shaders
	VER_RECOMPILE_MATERIAL_VERTEX_SHADERS				= 658,
	// - Allow random overrides per-section in ProcBuilding meshes
	VER_PROCBUILDING_MATERIAL_OPTIONS					= 659,
	// - Changed uniform expressions to reference textures by index instead of name
	VER_UNIFORMEXPRESSION_TEXTUREINDEX					= 660,
	// - Regenerate texture array for old materials, so they match the shadercache.
	VER_UNIFORMEXPRESSION_POSTLOADFIXUP					= 661,
	// - Separated DOF and Bloom, invalidate shadercache.
	VER_SEPARATE_DOF_BLOOM								= 662,
	// - Fixed Raw Animation Compression not working properly.
	VER_FIXED_RAW_ANIM_COMPRESSION						= 663,
	// - Change AnimNotify_Trails to use SamplesPerSecond
	VER_ANIMNOTIFY_TRAIL_SAMPLEFRAMERATE				= 664,
	// - Support for attaching static decals to instanced static meshes
	VER_STATIC_DECAL_INSTANCE_INDEX						= 665,
	// - Added support for precomputed shadowmaps to lit decals
	// Teh Forbidden= ?,
	VER_DECAL_SHADOWMAPS								= 666,		
	// - Fixed malformed raw anim data
	VER_FIXED_MALFORMED_RAW_ANIM_DATA					= 667,
	// - Removed unused velocity values from AnimNotify_Trail sampled data
	VER_ANIMNOTIFY_TRAILS_REMOVED_VELOCITY				= 668,
	// - Added SpawnRate support to Ribbon emitters
	VER_RIBBON_EMITTERS_SPAWNRATE						= 669,
	// - Remove ruleset from FPoly and add 'variation name' instead
	VER_FPOLY_RULESET_VARIATIONNAME						= 670,
	// - Added PreViewTranslationParameter in FParticleInstancedMeshVertexFactoryShaderParameters
	VER_ADDED_PRE_VIEW_TRANSLATION_PARAMETER			= 671,
	// - Added shader compression functionality
	VER_SHADER_COMPRESSION								= 672,
	// - Optimized FPropertyTag to store bool properties with 1 byte on disk instead of 4
	VER_PROPERTYTAG_BOOL_OPTIMIZATION					= 673,
	// - Added iPhone cached data (PVRTC textures)
	VER_ADDED_CACHED_IPHONE_DATA						= 674,
	// - Added DOFParameters to BasePassVertexShader for translucency DoF
	VER_ADDED_BASE_PASS_VS_DOF_PARAMETERS				= 675,
	// - Added TextureUsageInfos to UPersistentCookerData for stats tracking
	VER_ADDED_TEXTURE_USAGE_INFO						= 676,
	// - Fixup for ForceFeedbackSerialization
	VER_FORCEFEEDBACKWAVERFORM_NOEXPORT_CHANGE			= 677,
	// - Changed type OverrideVertexColors from TArray<FColor> to FColorVertexBuffer * 
	VER_OVERWRITE_VERTEX_COLORS_MEM_OPTIMIZED			= 678,
	// - Changed the default usage to be SVB_LoadingAndVisibility for level streaming volumes.
	VER_STREAMINGVOLUME_USAGE_DEFAULT					= 679,
	// - Added support to serialize clothing asset properties.
	VER_APEX_CLOTHING                       			= 680,
	// - Added support to serialize destruction cached data
	VER_APEX_DESTRUCTION                      			= 681,
	// - Added spotlight dominant shadow transition handling
	VER_SPOTLIGHT_DOMINANTSHADOW_TRANSITION				= 682,
	// - Added common game type package object lists to GPCD
	VER_PREFIX_GAMETYPE_OBJECTS_IN_GPCD					= 683,
	// - Added PMap forced object lists to GPCD
	VER_PMAP_FORCED_OBJECTS_IN_GPCD						= 684,
	// - Added support for preshadows on translucency
	VER_TRANSLUCENT_PRESHADOWS							= 685,
	// - Removed shadow volume support
	VER_REMOVED_SHADOW_VOLUMES							= 686,
	// - Added procedural texture that contains parameters to fix stereo offseting.
	VER_STEREO_FIX_TEXTURE                              = 687,
	// - Bulk serialize instance data
	VER_BULKSERIALIZE_INSTANCE_DATA						= 688,
	// - Added TerrainVertexFactory TerrainLayerCoordinateOffset Parameter
	VER_ADDED_TERRAINLAYERCOORDINATEOFFSET_PARAM		= 689,
	// - Added CachedPhysConvexBSPData in ULevel for Convex BSP
	VER_CONVEX_BSP										= 690,
	// - Reduced ProbeMask in UState/FStateFrame to DWORD and removed IgnoreMask
	VER_REDUCED_PROBEMASK_REMOVED_IGNOREMASK			= 691,
	// - Changed GDO lighting defaults to be cheap
	VER_CHANGED_GDO_LIGHTING_DEFAULTS					= 692,
	// - Changed way material references are stored/handled for Matinee material parameter tracks
	VER_CHANGED_MATPARAMTRACK_MATERIAL_REFERENCES		= 693,
	// - Added bone influence mapping option per bone break
	VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCE_CUSTOM_MAPPING   = 694,
	// - Changed light shaft shaders
	VER_LIGHTSHAFT_SHADER_CHANGES						= 695,
	// - Changed GDO lighting defaults to be cheap
	VER_CHANGED_GDO_LIGHTING_DEFAULTS2					= 696,
	// - Half resolution scene and depth for DOF, Bloom, MotionBlur
	VER_HALFRESSCENEPOSTPROCESS							= 697,
	// - Changed light shaft shaders
	VER_LIGHTSHAFT_SHADER_CHANGES2						= 698,
	// - Exponential Height Fog
	VER_EXPONENTIAL_HEIGHT_FOG							= 699,
	// - Added chunks/sections when swapping to a vertex influence using IWU_FullSwap
	VER_ADDED_CHUNKS_SECTIONS_VERTEX_INFLUENCE			= 700,
	// - Downsample and blur with masking to avoid leaking
	VER_FILTERPOSTPROCESS_MASKED						= 701,
	// - Downsample and blur with masking to avoid leaking SM2
	VER_FILTERPOSTPROCESS_MASKED2						= 702,
	// - Unified downsample scene for SM2
	VER_UNIFY_HALFSCENE_RES								= 703,
	// - Added support for halfres DOF on SM2
	VER_HALFSCENE_RES_SM2								= 704,
	// - Half scene depth parameter got serialized
	VER_HALFSCENE_DEPTH_PARAM							= 705,
	// - introduced VisualizeTexture shader
	VER_VISUALIZETEXTURE								= 706,
	// - updated bink shader serialization
	VER_BINK_SHADER_SERIALIZATION_CHANGE				= 707,
	// - Added RequiredBones array to extra vertex influence structure
	VER_ADDED_REQUIRED_BONES_VERTEX_INFLUENCE			= 708,
	// - Added multiple UV channels to skeletal meshes
	VER_ADDED_MULTIPLE_UVS_TO_SKELETAL_MESH				= 709,
	// - Added ability to render and import skeletal meshes with vertex colors
	VER_ADDED_SKELETAL_MESH_VERTEX_COLORS				= 710,
	// - Removed SM2 support
	VER_REMOVED_SHADER_MODEL_2							= 711,
	// - Added bloom parameters
	VER_ADDED_BLOOM_PARAMETERS							= 712,
	// - Removed terrain displacement mapping
	VER_TERRAIN_REMOVED_DISPLACEMENTS					= 713,
	// - Added FStaticTerrainLayerWeightParameter
	VER_ADD_TERRAINLAYERWEIGHT_PARAMETERS				= 714,
	// - Added usage specification to vertex influences
	VER_ADDED_USAGE_VERTEX_INFLUENCE					= 715,
	// - Added support for camera offset particles
	VER_PARTICLE_ADDED_CAMERA_OFFSET					= 716,
	// - Merged scalar parameters
	VER_COMBINED_SCALAR_PARAMETERS						= 717,
	// - Added AngleBased SSAO option
	VER_ANGLEBASED_SSAO									= 718,
	// - AngleBasedSSAO 4x4 dither pattern instead of 2x2
	VER_ANGLEBASED_SSAO_DITHER							= 719,
	// - Resolution independent light shafts
	VER_RES_INDEPENDENT_LIGHTSHAFTS						= 720,
	// - Lightmaps on GDOs
	VER_GDO_LIGHTMAPS									= 721,
	// - MotionBlurSeperatePass
	VER_MOTIONBLUR_SEPERATE_PASS						= 722,
	// - Explicit normal support for static meshes					
	VER_STATIC_MESH_EXPLICIT_NORMALS					= 723,
	// - Fix HalfRes MotionBlur&DOF issues
	VER_HALFRES_MOTIONBLURDOF							= 724,
	// - Fix more HalfRes MotionBlur&DOF issues
	VER_HALFRES_MOTIONBLURDOF2							= 725,
	// - Fix more HalfRes MotionBlur&DOF issues
	VER_HALFRES_MOTIONBLURDOF3							= 726,
	// - Reverted HalfRes MotionBlur&DOF for now
	VER_HALFRES_MOTIONBLURDOF4							= 727,
	// - Add parameters to LandscapeVertexFactory
	VER_LANDSCAPEVERTEXFACTORY_ADDPARAMS				= 728,
	// - MotionBlurSeperatePass back in again
	VER_HALFRES_MOTIONBLURDOF5							= 729,
	// - remove SeparateBloom option (Bloom/DOF radius)
	VER_REMOVED_SEPARATEBLOOM							= 730,
	// - bump the version to prevent error message
	VER_REMOVED_SEPARATEBLOOM2							= 731,
	// - Fixed GDO FLightmapRef handling
	VER_FIXED_GDO_LIGHTMAP_REFCOUNTING					= 732,
	// - bump the version to prevent error message
	VER_HQ_DEPTHOFFIELD									= 733,
	// - Precomputed Visibility
	VER_PRECOMPUTED_VISIBILITY							= 734,
	// - sets the StartTime on MITVs to -1 when they were created with that var being transient
	VER_MITV_START_TIME_FIX_UP							= 735,
	// - refactor motionblur
	VER_REFACTOR_MOTIONBLUR								= 736,
	// - Add lightmap to LandscapeComponent
	VER_LANDSCAPECOMPONENT_LIGHTMAPS					= 737,
	// - motionblur improvements
	VER_IMPROVED_MOTIONBLUR								= 738,
	// - Non uniform precomputed visibility
	VER_NONUNIFORM_PRECOMPUTED_VISIBILITY				= 739,
	// - Object based Motion Blur scale fix
	VER_IMPROVED_MOTIONBLUR2							= 740,
	// - Object based Motion Blur scale fix
	VER_HITMASK_MIRRORING_SUPPORT						= 741,
	// - Fixed RadialBlur look
	VER_RADIALBLUR_FIX									= 743,
	// - Add Landscape vertex factory LodBias Parameter
	VER_LANDSCAPEVERTEXFACTORY_ADD_LODBIAS_PARAM		= 744,
	// - Optimized AngleBasedSSAO, better quality 
	VER_IMPROVED_ANGLEBASEDSSAO							= 746,
	// - Optimized AngleBasedSSAO 
	VER_IMPROVED_ANGLEBASEDSSAO2						= 747,
	// - New character indirect lighting controls
	VER_CHARACTER_INDIRECT_CONTROLS						= 748,
	// - Add force script defined ordering per class
	VER_FORCE_SCRIPT_DEFINED_ORDER_PER_CLASS			= 749,
	// - Optimized SSAO SmartBlur making 2 pass
	VER_OPTIMIZEDSSAO									= 750,
	// - Tonemapper now interprets negative colors as black
	VER_TONEMAPPERBEHAVIOR								= 751,
	// - Motion blur not leaking colors outside of the object bound
	VER_MOTIONBLURANTILEAK								= 752,
	// - One pass approximate lighting for translucency
	VER_ONEPASS_TRANSLUCENCY_LIGHTING					= 754,
	// - Refactor uber post processing 
	VER_REFACTOR_UBERPOSTPROCESS						= 755,
	// - Moved UField::SuperField to UStruct
	VER_MOVED_SUPERFIELD_TO_USTRUCT						= 756,
	// - Refactor uber post processing now support image grain
	VER_ADDED_IMAGEGRAIN								= 757,
	// - Added scale for ImageGrain and tone mapper
	VER_ADDED_SCALES									= 758,
	// - Changed Tonemapper scale to be before tone mapping
	VER_ADDED_SCALES2									= 759,
	// - Support AnimNodeSlot dynamic sequence node allocation on demand
	VER_ADDED_ANIMNODESLOTPOOL							= 760,
	// - Optimized UAnimSequence storage
	VER_OPTIMIZED_ANIMSEQ								= 761,
	// - Fixed Distortion effect leaking outside of the view, optimized shader
	VER_DISTORTIONEFFECT								= 762,
	// - removed Direction from cover reference
	VER_REMOVED_DIR_COVERREF							= 763,
	// - Fixed GDO's getting lighting unbuilt when Undestroyed
	VER_GDO_LIGHTING_HANDLE_UNDESTROY					= 764,
	// - Added option for per bone motion blur, made pow() for non PS3 platforms unclamped
	VER_PERBONEMOTIONBLUR								= 766,
	// - Added async texture pre-allocation to level streaming
	VER_TEXTURE_PREALLOCATION							= 767,
	// - Added property to specify bone to use for TRISORT_CustomLeftRight
	VER_ADDED_SKELETAL_MESH_SORTING_LEFTRIGHT_BONE		= 768,
	// - Added new feature: SoftEdge MotionBlur
	VER_SOFTEDGEMOTIONBLUR								= 769,
	// - Compact kDop trees for static meshes
	VER_COMPACTKDOPSTATICMESH,
	// - Compact kDop trees for static meshes
	VER_ADDED_NEW_TONEMAPPER,
	// - Refactoring UberPostProcess, LUT and grain now usable for non tone mapper cases as well
	VER_UBERPOST_REFACTOR,
	// - Refactoring UberPostProcess, removed unused parameters
	VER_UBERPOST_REFACTOR2,
	// - Added XY offset parameters to Landscape vertex factory
	VER_LANDSCAPEVERTEXFACTORY_ADD_XYOFFSET_PARAMS,
	// - Fixed clamping of non tonemapped case
	VER_FIXCLAMP_NON_TONEMAP,
	// - Update uberpostprocess with smaller DOF radius to not reach instruction limit on SM3
	VER_UBERPOSTPROCESS_UPDATE,
	// - Clamped specular pow to prevent image artifacts (see comment in shader)
	VER_CLAMP_SPECULAR_POWER,
	// - Adjusted experimental tonemapper
	VER_TONEMAPPER2ADJUST,
	// - Replaced tonemapper checkbox by combobox
	VER_TONEMAPPER_ENUM,
	// - Fix distortion effect wrong color leaking in
	VER_DISTORTIONEFFECT2,
	// - MotionBlurSoftEdge feature updated
	VER_IMPROVED_MOTIONBLUR4,
	// - MotionBlurSoftEdge fixed for Playstation3
	VER_IMPROVED_MOTIONBLUR5,
	// - Fixed translucent preshadow filtering
	VER_FIXED_TRANSLUCENT_SHADOW_FILTERING,
	// - Added vfetch sprite and subuv particle support on 360
	VER_SPRITE_SUBUV_VFETCH_SUPPORT,
	// - fixed seam in uber postprocessing
	VER_FIXSEAMINPOSTPROCESS,
	// - fixed warning with MotionBlurSkinning
	VER_MOTIONBLURSKINNING								= 787,
	// - adjustable kernel for ReferenceDOF
	VER_POSTPROCESSUPDATE,
	// - Added class group names for grouping in the editor
	VER_ADDED_CLASS_GROUPS,
	// - Bloom after motionblur for better quality
	VER_BLOOM_AFTER_MOTIONBLUR,
	// - DX11 integration
	VER_DX11_TESSELLATION,
	// - MotionBlurSoftEdge fix bias on NV 7800 cards
	VER_IMPROVED_MOTIONBLUR6,
	// - MotionBlur optimizations
	VER_IMPROVED_MOTIONBLUR7,
	// - Removed unused parameter
	VER_REMOVE_MAXBONEINFLUENCE,
	// - Optimized MotionBlurSoftEdge
	VER_IMPROVED_MOTIONBLUR8,
	// - Fixed automatic shader versioning
	VER_FIXED_AUTO_SHADER_VERSIONING,
	// - Added texture instances for non-static actors in ULevel::BuildStreamingData().
	VER_DYNAMICTEXTUREINSTANCES,
	// - Moved Guids previously stored in CoverLink (with many dups) into ULevel
	VER_COVERGUIDREFS_IN_ULEVEL,
	// - Added ability to override Colorgrading with no LUT
	VER_COLORGRADING,
	// - Fix content that lost the flag because of wrong serialization
	VER_COLORGRADING2,
	// - Added code to preserve static mesh component override vertex colors when source verts change
	VER_PRESERVE_SMC_VERT_COLORS,
	// - Added shadowing for image based reflections
	VER_IMAGE_REFLECTION_SHADOWING,
	// - changes displacement from tangent space to world space, now extra TransformVector node is required
	VER_DX11_TESSELLATION_TS_TO_WS,
	// - Added ability to keep degenerate triangles when building static mesh
	VER_KEEP_STATIC_MESH_DEGENERATES,
	// - Added shader cache priority
	VER_SHADER_CACHE_PRIORITY,
	// - Added support for 32 bit vertex indices on skeletal meshes
	VER_DWORD_SKELETAL_MESH_INDICES,
	// - Introduced DepthOfFieldType
	VER_DEPTHOFFIELD_TYPE,
	// - Fixed some serialization issues with 32 bit indices
	VER_DWORD_SKELETAL_MESH_INDICES_FIXUP,
	// - Added DoF two layer effect for simple DoF
	VER_DEPTHOFFIELD_SIMPLE_TWO_LAYER,
	// - Changed material parameter allocation for landscape
	VER_CHANGED_LANDSCAPE_MATERIAL_PARAMS,
	// - fix blue rendering
	VER_INVALIDATE_SHADERCACHE0,
	// - fix blue rendering
	VER_INVALIDATE_SHADERCACHE1,

	// -----<new versions can be added before this line>-------------------------------------------------

	// - this needs to be the last line (see note below)
	VER_AUTOMATIC_VERSION_PLUS_ONE
};

// !!
// !! NOTE: We switched from #define to enum. This means there is no longer a need to update two places and as enum is
// !!       automatically counting up each entry the system is less vulnerable to human error.
// !!       Removing lines in the used enum range need to be done with extreme care to not affect the enum counting.
// !!
// !! WARNING: Only modify this in //depot/UnrealEngine3/Development/Src/Core/Inc/UnObjVer.h on the Epic Perforce server. All 
// !! WARNING: other places should modify VER_LATEST_ENGINE_LICENSEE instead.
// !!
#define VER_LATEST_ENGINE									(PREPROCESSOR_ENUM_PROTECT(VER_AUTOMATIC_VERSION_PLUS_ONE) - 1)

#define VER_LATEST_ENGINE_LICENSEE							0

// Cooked packages loaded with an older package version are recooked
#define VER_LATEST_COOKED_PACKAGE							127
#define VER_LATEST_COOKED_PACKAGE_LICENSEE					0

// Package version about 21 months ago - May 2009 - 602 - anything older than this will be resaved and is not guaranteed to load
#define VER_DEPRECATED_PACKAGE								VER_DEPRECATE_SOUND_DISTRIBUTIONS

// Package version about 18 months ago - August 2009 - 626 - anything older than this will warn about deprecation on load
#define VER_DEPRECATED_WARN_PACKAGE							VER_FIXING_PARTICLE_EMITTEREDITORCOLOR

// Version access.

extern INT			GEngineVersion;					// Engine build number, for displaying to end users.
extern INT			GBuiltFromChangeList;			// Built from changelist, for displaying to end users.


extern INT			GEngineMinNetVersion;			// Earliest engine build that is network compatible with this one.
extern INT			GEngineNegotiationVersion;		// Base protocol version to negotiate in network play.

extern INT			GPackageFileVersion;			// The current Unrealfile version.
extern INT			GPackageFileMinVersion;			// The earliest file version that can be loaded with complete backward compatibility.
extern INT			GPackageFileLicenseeVersion;	// Licensee Version Number.

extern INT          GPackageFileCookedContentVersion;  // version of the cooked content
