/*================================================================================
	LandscapeEdMode.h: Landscape editing
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/


#ifndef __LandscapeEdMode_h__
#define __LandscapeEdMode_h__

#ifdef _MSC_VER
	#pragma once
#endif

class FLandscapeBrush
{
public:
	virtual FLOAT GetBrushExtent() = 0;
	virtual void MouseMove( FLOAT LandscapeX, FLOAT LandscapeY ) = 0;
	virtual void ApplyBrush( TMap<QWORD, FLOAT>& OutBrush, INT& OutX1, INT& OutY1, INT& OutX2, INT& OutY2 ) =0;
	virtual UBOOL InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent ) { return FALSE; }
	virtual void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime) {};
	virtual void BeginStroke(FLOAT LandscapeX, FLOAT LandscapeY, class FLandscapeTool* CurrentTool);
	virtual void EndStroke();
	virtual void EnterBrush() {}
	virtual void LeaveBrush() {}
	virtual ~FLandscapeBrush() {}
	virtual UMaterialInterface* GetBrushMaterial() { return NULL; }
	virtual const TCHAR* GetIconString() = 0;
	virtual FString GetTooltipString() = 0;
};

struct FLandscapeBrushSet
{
	FLandscapeBrushSet(FString InToolTip)
	:	ToolTip(InToolTip)
	{}

	TArray<FLandscapeBrush*> Brushes;
	FString ToolTip;
};

enum ELandscapeToolTargetType
{
	LET_Heightmap	= 0,
	LET_Weightmap	= 1,
	LET_Visibility	= 2,
	LET_Selection   = 3,
	LET_All			= 4,
};

namespace ELandscapeToolUIType
{
	enum Type
	{
		Default	= 0,
		Erosion	= 1,
		Noise	= 2,
		HydraErosion = 3,
		Smooth	= 4,
	};
};

namespace ELandscapeToolNoiseMode
{
	enum Type
	{
		Invalid = -1,
		Both = 0,
		Add,
		Sub,
	};

	inline FLOAT Conversion(Type Mode, FLOAT NoiseAmount, FLOAT OriginalValue)
	{
		switch( Mode )
		{
		case Add: // always +
			OriginalValue += NoiseAmount;
			OriginalValue /= 2.f;
			break;
		case Sub: // always -
			OriginalValue -= NoiseAmount;
			OriginalValue /= 2.f;
			break;
		case Both:
			break;
		}
		return OriginalValue;
	}
}

struct FLandscapeToolTarget
{
	ALandscape* Landscape;
	ELandscapeToolTargetType TargetType;
	FName LayerName;
	
	FLandscapeToolTarget()
	:	Landscape(NULL)
	,	TargetType(LET_Heightmap)
	,	LayerName(NAME_None)
	{}
};

/**
 * FLandscapeTool
 */
class FLandscapeTool
{
public:
	virtual UBOOL IsValidForTarget(const FLandscapeToolTarget& Target) = 0;
	virtual UBOOL BeginTool( FEditorLevelViewportClient* ViewportClient, const FLandscapeToolTarget& Target, FLOAT InHitX, FLOAT InHitY ) = 0;
	virtual void EndTool() = 0;
	virtual void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime) {};
	virtual UBOOL MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y ) = 0;
	virtual UBOOL CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY ) = 0;
	virtual void ApplyTool( FEditorLevelViewportClient* ViewportClient ) = 0;
	virtual UBOOL InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent ) { return FALSE; }
	virtual ~FLandscapeTool() {}
	virtual const TCHAR* GetIconString() = 0;
	virtual FString GetTooltipString() = 0;
	virtual ELandscapeToolUIType::Type GetUIType() { return ELandscapeToolUIType::Default; }
};

class FLandscapeToolSet
{
	TArray<FLandscapeTool*> Tools;
	FLandscapeTool*				CurrentTool;
	ELandscapeToolUIType::Type	UIType;
public:

	FLandscapeToolSet()
	:	CurrentTool(NULL),
		UIType(ELandscapeToolUIType::Default)
	{		
	}

	virtual ~FLandscapeToolSet()
	{
		for( INT ToolIdx=0;ToolIdx<Tools.Num();ToolIdx++ )
		{
			delete Tools(ToolIdx);
		}
	}

	virtual const TCHAR* GetIconString() { return Tools(0)->GetIconString(); }
	virtual FString GetTooltipString() { return Tools(0)->GetTooltipString(); }

	void AddTool(FLandscapeTool* InTool)
	{
		// Tools in one Tool Set should have same UIType...
		check(UIType == ELandscapeToolUIType::Default || UIType == InTool->GetUIType());
		UIType = InTool->GetUIType();
		Tools.AddItem(InTool);
	}

	UBOOL SetToolForTarget( const FLandscapeToolTarget& Target )
	{
		for( INT ToolIdx=0;ToolIdx<Tools.Num();ToolIdx++ )
		{
			if( Tools(ToolIdx)->IsValidForTarget(Target) )
			{
				CurrentTool = Tools(ToolIdx);
				return TRUE;
			}
		}

		return FALSE;
	}

	FLandscapeTool* GetTool()
	{
		return CurrentTool;
	}

	ELandscapeToolUIType::Type GetUIType()
	{
		return UIType;
	}
};

struct FLandscapeLayerThumbnailInfo
{
	FLandscapeLayerInfo LayerInfo;
	FObjectThumbnail LayerThumbnail;
	
	FLandscapeLayerThumbnailInfo(FLandscapeLayerInfo InLayerInfo)
	:	LayerInfo(InLayerInfo)
	{}
};

// Current user settings in Landscape UI
struct FLandscapeUISettings
{
	void Load();
	void Save();

	// Window
	void SetWindowSizePos(INT NewX, INT NewY, INT NewWidth, INT NewHeight) { WindowX = NewX; WindowY = NewY; WindowWidth = NewWidth; WindowHeight = NewHeight; }
	void GetWindowSizePos(INT& OutX, INT& OutY, INT& OutWidth, INT& OutHeight) { OutX = WindowX; OutY = WindowY; OutWidth = WindowWidth; OutHeight = WindowHeight; }

	// tool
	float GetToolStrength() { return ToolStrength; }
	void SetToolStrength(FLOAT InToolStrength) { ToolStrength = InToolStrength; }

	// for Erosion Tool
	INT GetErodeThresh() { return ErodeThresh; }
	void SetErodeThresh(FLOAT InErodeThresh) { ErodeThresh = InErodeThresh; }
	INT GetErodeIterationNum() { return ErodeIterationNum; }
	void SetErodeIterationNum(FLOAT InErodeIterationNum) { ErodeIterationNum = InErodeIterationNum; }
	INT GetErodeSurfaceThickness() { return ErodeSurfaceThickness; }
	void SetErodeSurfaceThickness(FLOAT InErodeSurfaceThickness) { ErodeThresh = InErodeSurfaceThickness; }
	ELandscapeToolNoiseMode::Type GetErosionNoiseMode() { return ErosionNoiseMode; }
	void SetErosionNoiseMode(ELandscapeToolNoiseMode::Type InErosionNoiseMode) { ErosionNoiseMode = InErosionNoiseMode; }
	FLOAT GetErosionNoiseScale() { return ErosionNoiseScale; }
	void SetErosionNoiseScale(FLOAT InErosionNoiseScale) { ErosionNoiseScale = InErosionNoiseScale; }

	// Hydra Erosion
	INT GetRainAmount() { return RainAmount; }
	void SetRainAmount(INT InRainAmount) { RainAmount = InRainAmount; }
	FLOAT GetSedimentCapacity() { return SedimentCapacity; }
	void SetSedimentCapacity(FLOAT InSedimentCapacity) { SedimentCapacity = InSedimentCapacity; }
	INT GetHErodeIterationNum() { return HErodeIterationNum; }
	void SetHErodeIterationNum(FLOAT InHErodeIterationNum) { HErodeIterationNum = InHErodeIterationNum; }
	ELandscapeToolNoiseMode::Type GetRainDistMode() { return RainDistMode; }
	void SetRainDistMode(ELandscapeToolNoiseMode::Type InRainDistMode) { RainDistMode = InRainDistMode; }
	FLOAT GetRainDistScale() { return RainDistScale; }
	void SetRainDistScale(FLOAT InRainDistScale) { RainDistScale = InRainDistScale; }
	FLOAT GetHErosionDetailScale() { return HErosionDetailScale; }
	void SetHErosionDetailScale(FLOAT InHErosionDetailScale) { HErosionDetailScale = InHErosionDetailScale; }
	UBOOL GetbHErosionDetailSmooth() { return bHErosionDetailSmooth; }
	void SetbHErosionDetailSmooth(FLOAT InbHErosionDetailSmooth) { bHErosionDetailSmooth = InbHErosionDetailSmooth; }

	// Noise Tool
	ELandscapeToolNoiseMode::Type GetNoiseMode() { return NoiseMode; }
	void SetNoiseMode(ELandscapeToolNoiseMode::Type InNoiseMode) { NoiseMode = InNoiseMode; }
	FLOAT GetNoiseScale() { return NoiseScale; }
	void SetNoiseScale(FLOAT InNoiseScale) { NoiseScale = InNoiseScale; }

	// Detail Preserving Smooth
	FLOAT GetDetailScale() { return DetailScale; }
	void SetDetailScale(FLOAT InDetailScale) { DetailScale = InDetailScale; }
	UBOOL GetbDetailSmooth() { return bDetailSmooth; }
	void SetbDetailSmooth(FLOAT InbDetailSmooth) { bDetailSmooth = InbDetailSmooth; }

	// brush
	FLOAT GetBrushRadius() { return BrushRadius; }
	void SetBrushRadius(FLOAT InBrushRadius) { BrushRadius = InBrushRadius; }
	FLOAT GetBrushFalloff() { return BrushFalloff; }
	void SetBrushFalloff(FLOAT InBrushFalloff) { BrushFalloff = InBrushFalloff; }

	FLandscapeUISettings()
	:	WindowX(-1)
	,	WindowY(-1)
	,	WindowWidth(284)
	,	WindowHeight(600)
	,	ToolStrength(0.3f)
	,	BrushRadius(2048.f)
	,	BrushFalloff(0.5f)
	,	ErodeThresh(64)
	,	ErodeIterationNum(28)
	,	ErodeSurfaceThickness(256)
	,	ErosionNoiseMode(ELandscapeToolNoiseMode::Sub)
	,	ErosionNoiseScale(60.f)
	,	RainAmount(128)
	,	SedimentCapacity(0.3f)
	,	HErodeIterationNum(75)
	,	RainDistMode(ELandscapeToolNoiseMode::Both)
	,	RainDistScale(60.f)
	,	bHErosionDetailSmooth(TRUE)
	,	HErosionDetailScale(0.01f)
	,	NoiseMode(ELandscapeToolNoiseMode::Both)
	,	NoiseScale(128.f)
	,	DetailScale(0.3f)
	,	bDetailSmooth(TRUE)
	{
	}

private:
	INT WindowX;
	INT WindowY;
	INT WindowWidth;
	INT WindowHeight;

	FLOAT ToolStrength;
	FLOAT BrushRadius;
	FLOAT BrushFalloff;

	// for Erosion Tool
	INT ErodeThresh;
	INT ErodeIterationNum;
	INT ErodeSurfaceThickness;
	ELandscapeToolNoiseMode::Type ErosionNoiseMode;
	FLOAT ErosionNoiseScale;

	// Hydra Erosion
	INT RainAmount;
	FLOAT SedimentCapacity;
	INT HErodeIterationNum;
	ELandscapeToolNoiseMode::Type RainDistMode;
	FLOAT RainDistScale;
	UBOOL bHErosionDetailSmooth;
	FLOAT HErosionDetailScale;

	// Noise Tool
	ELandscapeToolNoiseMode::Type NoiseMode;
	FLOAT NoiseScale;

	// Frequency Smooth
	UBOOL bDetailSmooth;
	FLOAT DetailScale;
};



/**
 * Landscape editor mode
 */
class FEdModeLandscape : public FEdMode
{
public:
	FLandscapeUISettings UISettings;

	FLandscapeToolSet* CurrentToolSet;
	FLandscapeBrush* CurrentBrush;
	FLandscapeToolTarget CurrentToolTarget;

	// UI setting for additional UI Tools
	INT ToolUIType;

	/** Constructor */
	FEdModeLandscape();

	/** Destructor */
	virtual ~FEdModeLandscape();

	/** FSerializableObject: Serializer */
	virtual void Serialize( FArchive &Ar );

	/** FEdMode: Called when the mode is entered */
	virtual void Enter();

	/** FEdMode: Called when the mode is exited */
	virtual void Exit();

	/** FEdMode: Called when the mouse is moved over the viewport */
	virtual UBOOL MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y );

	/**
	 * FEdMode: Called when the mouse is moved while a window input capture is in effect
	 *
	 * @param	InViewportClient	Level editor viewport client that captured the mouse input
	 * @param	InViewport			Viewport that captured the mouse input
	 * @param	InMouseX			New mouse cursor X coordinate
	 * @param	InMouseY			New mouse cursor Y coordinate
	 *
	 * @return	TRUE if input was handled
	 */
	virtual UBOOL CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY );

	/** FEdMode: Called when a mouse button is pressed */
	virtual UBOOL StartTracking();

	/** FEdMode: Called when a mouse button is released */
	virtual UBOOL EndTracking();

	/** FEdMode: Called once per frame */
	virtual void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime);

	/** FEdMode: Called when a key is pressed */
	virtual UBOOL InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent );

	/** FEdMode: Called when mouse drag input it applied */
	virtual UBOOL InputDelta( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale );

	/** FEdMode: Render elements for the landscape tool */
	virtual void Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

	/** FEdMode: Render HUD elements for this tool */
	virtual void DrawHUD( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas );

	/** FEdMode: Called when the currently selected actor has changed */
	virtual void ActorSelectionChangeNotify();

	/** FEdMode: If the EdMode is handling InputDelta (ie returning true from it), this allows a mode to indicated whether or not the Widget should also move. */
	virtual UBOOL AllowWidgetMove() { return FALSE; }

	/** FEdMode: Draw the transform widget while in this mode? */
	virtual UBOOL ShouldDrawWidget() const { return FALSE; }

	/** FEdMode: Returns true if this mode uses the transform widget */
	virtual UBOOL UsesWidget() const { return FALSE; }

	/** Forces real-time perspective viewports */
	void ForceRealTimeViewports( const UBOOL bEnable, const UBOOL bStoreCurrentState );

	/** Trace under the mouse cursor and return the landscape hit and the hit location (in landscape quad space) */
	UBOOL LandscapeMouseTrace( FEditorLevelViewportClient* ViewportClient, FLOAT& OutHitX, FLOAT& OutHitY );


	//
	// Interaction  with WPF User Interface
	//

	/** Change current tool */
	void SetCurrentTool( INT ToolIndex );

	void GetLayersAndThumbnails( TArray<FLandscapeLayerThumbnailInfo>& OutLayerTumbnailInfo );


	TArray<FLandscapeToolSet*> LandscapeToolSets;
	TArray<FLandscapeBrushSet> LandscapeBrushSets;


private:

#if WITH_MANAGED_CODE
	/** Landscape palette window */
	TScopedPointer< class FLandscapeEditWindow > LandscapeEditWindow;
#endif

	UBOOL bToolActive;
};


#endif	// __LandscapeEdMode_h__
