/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


//=============================================================================
// EditorEngine: The UnrealEd subsystem.
// This is a built-in Unreal class and it shouldn't be modified.
//=============================================================================
class EditorEngine extends Engine
	native
	config(Engine)
	noexport
	transient;

// Objects.
var const model       TempModel;
var const transbuffer Trans;
var const textbuffer  Results;
var const array<pointer>  ActorProperties;
var const pointer     LevelProperties;

// Textures.
var const texture2D Bad, Bkgnd, BkgndHi, BadHighlight, MaterialArrow, MaterialBackdrop;

// Audio
var const transient SoundCue			PreviewSoundCue;
var const transient AudioComponent		PreviewAudioComponent;

// Used in UnrealEd for showing materials
var staticmesh	TexPropCube;
var staticmesh	TexPropSphere;
var staticmesh	TexPropPlane;
var staticmesh	TexPropCylinder;

// Toggles.
var const bool bFastRebuild, bBootstrapping, bIsImportingT3D;

// Other variables.
var const int TerrainEditBrush, ClickFlags;
var const config float MovementSpeed;
var const package ParentContext;
var const vector ClickLocation;
var const plane ClickPlane;
var const vector MouseMovement;
var const native array<pointer> ViewportClients;

/** Distance to far clipping plane for perspective viewports.  If <= NEAR_CLIPPING_PLANE, far plane is at infinity. */
var const float FarClippingPlane;

// BEGIN FEditorConstraints
var					noexport const	pointer	ConstraintsVtbl;

// Grid.
var(Grid)			noexport config bool	GridEnabled;
var(Grid)			noexport config bool	SnapScaleEnabled;
var(Grid)			noexport config bool	SnapVertices;
var(Grid)			noexport config int		ScaleGridSize;		// Integer percentage amount to snap scaling to.
var(Grid)			noexport config float	SnapDistance;	
var(Grid)			noexport config float	GridSizes[11];		// FEditorConstraints::MAX_GRID_SIZES = 11 in native code
var(Grid)			noexport config int		CurrentGridSz;		// Index into GridSizes
// Rotation grid.
var(RotationGrid)	noexport config bool	RotGridEnabled;
var(RotationGrid)	noexport config rotator RotGridSize;
// END FEditorConstraints


// Advanced.
var(Advanced) config bool UseSizingBox;
var(Advanced) config bool UseAxisIndicator;
var(Advanced) config float FOVAngle;
var(Advanced) config bool GodMode;

/** The location to autosave to. */
var(Advanced) config string AutoSaveDir;

var(Advanced) config bool InvertwidgetZAxis;
var(Advanced) config string GameCommandLine;

/** the list of package names to compile when building scripts */
var(Advanced) globalconfig array<string> EditPackages;

/** the base directory to use for finding .uc files to compile*/
var(Advanced) config string EditPackagesInPath;

/** the directory to save compiled .u files to */
var(Advanced) config string EditPackagesOutPath;

/** the directory to save compiled .u files to when script is compiled with the -FINAL_RELEASE switch */
var(Advanced) config string FRScriptOutputPath;

/** If TRUE, always show the terrain in the overhead 2D view. */
var(Advanced) config bool AlwaysShowTerrain;

/** If TRUE, use the gizmo for rotating actors. */
var(Advanced) config bool UseActorRotationGizmo;

/** If TRUE, show translucent marker polygons on the builder brush and volumes. */
var(Advanced) config bool bShowBrushMarkerPolys;

/** If TRUE, use Maya camera controls. */
var(Advanced) config bool bUseMayaCameraControls;

/** If TRUE, parts of prefabs cannot be individually selected/edited. */
var(Advanced) config bool bPrefabsLocked;

var			  config string HeightMapExportClassName;

/** Array of actor factories created at editor startup and used by context menu etc. */
var const array<ActorFactory> ActorFactories;

/** The name of the file currently being opened in the editor. "" if no file is being opened. */
var string	UserOpenedFile;

///////////////////////////////
// "Play From Here" properties

/** Additional per-user/per-game options set in the .ini file. Should be in the form "?option1=X?option2?option3=Y"					*/
var(Advanced) config string InEditorGameURLOptions;
/** A pointer to a UWorld that is the duplicated/saved-loaded to be played in with "Play From Here" 								*/
var const World PlayWorld;
/** An optional location for the starting location for "Play From Here"																*/
var const vector PlayWorldLocation;
/** An optional rotation for the starting location for "Play From Here"																*/
var const rotator PlayWorldRotation;
/** Has a request for "Play From Here" been made?													 								*/
var const bool bIsPlayWorldQueued;
/** Did the request include the optional location and rotation?										 								*/
var const bool bHasPlayWorldPlacement;
/** Cache of the world package's dirty flag, so that it can be restored after Play Form Here. */
var const bool bWorldPackageWasDirty;
/** Where did the person want to play? Where to play the game - -1 means in editor, 0 or more is an index into the GConsoleSupportContainer	*/
var const int	PlayWorldDestination;

// possible object propagators
var const pointer InEditorPropagator;
var const pointer RemotePropagator;

var bool bIsPushingView;
var const transient bool bColorPickModeEnabled;
var const transient bool bDecalUpdateRequested;

/** Temporary render target that can be used by the editor. */
var const transient TextureRenderTarget2D ScratchRenderTarget;

/**
 *	Display StreamingBounds for textures
 */
var const transient Texture2D StreamingBoundsTexture;

defaultproperties
{
     Bad=Texture2D'EditorResources.Bad'
     Bkgnd=Texture2D'EditorResources.Bkgnd'
     BkgndHi=Texture2D'EditorResources.BkgndHi'
	 MaterialArrow=Texture2D'EditorResources.MaterialArrow'
	 MaterialBackdrop=Texture2D'EditorResources.MaterialBackdrop'
	 BadHighlight=Texture2D'EditorResources.BadHighlight'
	 TexPropCube=StaticMesh'EditorMeshes.TexPropCube'
	 TexPropSphere=StaticMesh'EditorMeshes.TexPropSphere'
	 TexPropPlane=StaticMesh'EditorMeshes.TexPropPlane'
	 TexPropCylinder=StaticMesh'EditorMeshes.TexPropCylinder'
}
