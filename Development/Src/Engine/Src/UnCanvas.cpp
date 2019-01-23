/*=============================================================================
	UnCanvas.cpp: Unreal canvas rendering.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "ScenePrivate.h"
#include "TileRendering.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(UCanvas);

FCanvas::FCanvas(FRenderTarget* InRenderTarget,FHitProxyConsumer* InHitProxyConsumer)
:	RenderTarget(InRenderTarget)
,	bRenderTargetDirty(FALSE)
,	HitProxyConsumer(InHitProxyConsumer)
{
	check(RenderTarget);
	// Push the viewport transform onto the stack.  Default to using a 2D projection. 
	new(TransformStack) FMatrix( CalcBaseTransform2D(RenderTarget->GetSizeX(),RenderTarget->GetSizeY()) );
	// init alpha to 1
	AlphaModulate=1.0;

	// init sort key to 0
	PushDepthSortKey(0);
}

/**
* Replace the base (ie. TransformStack(0)) transform for the canvas with the given matrix
*
* @param Transform - The transform to use for the base
*/
void FCanvas::SetBaseTransform(const FMatrix& Transform)
{
	// set the base transform
	if( TransformStack.Num() > 0 )
	{
		TransformStack(0) = Transform;
	}
	else
	{
		new(TransformStack) FMatrix(Transform);
	}
}

/**
* Generate a 2D projection for the canvas. Use this if you only want to transform in 2D on the XY plane
*
* @param ViewSizeX - Viewport width
* @param ViewSizeY - Viewport height
* @return Matrix for canvas projection
*/
FMatrix FCanvas::CalcBaseTransform2D(UINT ViewSizeX, UINT ViewSizeY)
{
	return 
		FTranslationMatrix(FVector(-GPixelCenterOffset,-GPixelCenterOffset,0)) *
		FMatrix(
			FPlane(	1.0f / (ViewSizeX / 2.0f),	0.0,										0.0f,	0.0f	),
			FPlane(	0.0f,						-1.0f / (ViewSizeY / 2.0f),					0.0f,	0.0f	),
			FPlane(	0.0f,						0.0f,										1.0f,	0.0f	),
			FPlane(	-1.0f,						1.0f,										0.0f,	1.0f	)
			);
}

/**
* Generate a 3D projection for the canvas. Use this if you want to transform in 3D 
*
* @param ViewSizeX - Viewport width
* @param ViewSizeY - Viewport height
* @param fFOV - Field of view for the projection
* @param NearPlane - Distance to the near clip plane
* @return Matrix for canvas projection
*/
FMatrix FCanvas::CalcBaseTransform3D(UINT ViewSizeX, UINT ViewSizeY, FLOAT fFOV, FLOAT NearPlane)
{
	FMatrix ViewMat(CalcViewMatrix(ViewSizeX,ViewSizeY,fFOV));
	FMatrix ProjMat(CalcProjectionMatrix(ViewSizeX,ViewSizeY,fFOV,NearPlane));
	return ViewMat * ProjMat;
}

/**
* Generate a view matrix for the canvas. Used for CalcBaseTransform3D
*
* @param ViewSizeX - Viewport width
* @param ViewSizeY - Viewport height
* @param fFOV - Field of view for the projection
* @return Matrix for canvas view orientation
*/
FMatrix FCanvas::CalcViewMatrix(UINT ViewSizeX, UINT ViewSizeY, FLOAT fFOV)
{
	// convert FOV to randians
	FLOAT FOVRad = fFOV * (FLOAT)PI / 360.0f;
	// move camera back enough so that the canvas items being rendered are at the same screen extents as regular canvas 2d rendering	
	FTranslationMatrix CamOffsetMat(-FVector(0,0,-appTan(FOVRad)*ViewSizeX/2));
	// adjust so that canvas items render as if they start at [0,0] upper left corner of screen 
	// and extend to the lower right corner [ViewSizeX,ViewSizeY]. 
	FMatrix OrientCanvasMat(
		FPlane(	1.0f,				0.0f,				0.0f,	0.0f	),
		FPlane(	0.0f,				-1.0f,				0.0f,	0.0f	),
		FPlane(	0.0f,				0.0f,				1.0f,	0.0f	),
		FPlane(	ViewSizeX * -0.5f,	ViewSizeY * 0.5f,	0.0f, 1.0f		)
		);
	return 
		// also apply screen offset to align to pixel centers
		FTranslationMatrix(FVector(-GPixelCenterOffset,-GPixelCenterOffset,0)) * 
		OrientCanvasMat * 
		CamOffsetMat;
}

/**
* Generate a projection matrix for the canvas. Used for CalcBaseTransform3D
*
* @param ViewSizeX - Viewport width
* @param ViewSizeY - Viewport height
* @param fFOV - Field of view for the projection
* @param NearPlane - Distance to the near clip plane
* @return Matrix for canvas projection
*/
FMatrix FCanvas::CalcProjectionMatrix(UINT ViewSizeX, UINT ViewSizeY, FLOAT fFOV, FLOAT NearPlane)
{
	// convert FOV to randians
	FLOAT FOVRad = fFOV * (FLOAT)PI / 360.0f;
	// project based on the FOV and near plane given
	return FPerspectiveMatrix(
		FOVRad,
		ViewSizeX,
		ViewSizeY,
		NearPlane
		);
}

FCanvas::~FCanvas()
{
	Flush();
}

/**
* Base interface for canvas items which can be batched for rendering
*/
class FCanvasBaseRenderItem
{
public:
	/**
	* Renders the canvas item
	*
	* @param Canvas - canvas currently being rendered
	* @return TRUE if anything rendered
	*/
	virtual UBOOL Render( const FCanvas* Canvas ) =0;
	/**
	* FCanvasBatchedElementRenderItem instance accessor
	*
	* @return FCanvasBatchedElementRenderItem instance
	*/
	virtual class FCanvasBatchedElementRenderItem* GetCanvasBatchedElementRenderItem() { return NULL; }
	/**
	* FCanvasTileRendererItem instance accessor
	*
	* @return FCanvasTileRendererItem instance
	*/
	virtual class FCanvasTileRendererItem* GetCanvasTileRendererItem() { return NULL; }
};

/**
* Info needed to render a batched element set
*/
class FCanvasBatchedElementRenderItem : public FCanvasBaseRenderItem
{
public:
	/** 
	* Init constructor 
	*/
	FCanvasBatchedElementRenderItem(
		const FTexture* InTexture=NULL,
		EBlendMode InBlendMode=BLEND_MAX,
		FCanvas::EElementType InElementType=FCanvas::ET_MAX,
		const FMatrix& InTransform=FMatrix::Identity )
		// this data is deleted after rendering has completed
		: Data( new FRenderData(InTexture,InBlendMode,InElementType,InTransform) )
	{}

	/**
	* FCanvasBatchedElementRenderItem instance accessor
	*
	* @return this instance
	*/
	virtual class FCanvasBatchedElementRenderItem* GetCanvasBatchedElementRenderItem() 
	{ 
		return this; 
	}
	
	/**
	* Renders the canvas item. 
	* Iterates over all batched elements and draws them with their own transforms
	*
	* @param Canvas - canvas currently being rendered
	* @return TRUE if anything rendered
	*/
	virtual UBOOL Render( const FCanvas* Canvas )
	{	
		checkSlow(Data);
		UBOOL bDirty=FALSE;		
		if( Data->BatchedElements.HasPrimsToDraw() )
		{
			bDirty = TRUE;

			// this allows us to use FCanvas operations from the rendering thread (ie, render subtitles
			// on top of a movie that is rendered completely in rendering thread)
			if (IsInRenderingThread())
			{
				SCOPED_DRAW_EVENT(EventFrozenText)(DEC_SCENE_ITEMS,TEXT("UI Canvas From RenderThread"));
				// render context for RHI commands
				FCommandContextRHI* Context = RHIGetGlobalContext();
				// current render target set for the canvas
				const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
				// Set the RHI render target.
				RHISetRenderTarget(Context,CanvasRenderTarget->GetRenderTargetSurface(), FSurfaceRHIRef());
				// set viewport to RT size
				RHISetViewport(Context,0,0,0.0f,CanvasRenderTarget->GetSizeX(),CanvasRenderTarget->GetSizeY(),1.0f);
				// draw batched items
				Data->BatchedElements.Draw(
					Context,
					Data->Transform,
					CanvasRenderTarget->GetSizeX(),
					CanvasRenderTarget->GetSizeY(),
					Canvas->IsHitTesting(),
					1.0f / CanvasRenderTarget->GetDisplayGamma()
					);
				// delete data since we're done rendering it
				delete Data;
			}
			else
			{
				// Render the batched elements.
				struct FBatchedDrawParameters
				{
					FRenderData* RenderData;
					UINT ViewportSizeX;
					UINT ViewportSizeY;
					BITFIELD bHitTesting : 1;
					FLOAT DisplayGamma;
					const FRenderTarget* CanvasRenderTarget;
				};
				// current render target set for the canvas
				const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
				// all the parameters needed for rendering
				FBatchedDrawParameters DrawParameters =
				{
					Data,
					CanvasRenderTarget->GetSizeX(),
					CanvasRenderTarget->GetSizeY(),
					Canvas->IsHitTesting(),
					CanvasRenderTarget->GetDisplayGamma(),
					CanvasRenderTarget
				};
				ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
					BatchedDrawCommand,
					FBatchedDrawParameters,Parameters,DrawParameters,
				{
					SCOPED_DRAW_EVENT(EventFrozenText)(DEC_SCENE_ITEMS,TEXT("UI Canvas From GameThread"));

					FCommandContextRHI* Context = RHIGetGlobalContext();
					// Set the RHI render target.
					RHISetRenderTarget(Context,Parameters.CanvasRenderTarget->GetRenderTargetSurface(), FSurfaceRHIRef());
					// set viewport to RT size
					RHISetViewport(Context,0,0,0.0f,Parameters.CanvasRenderTarget->GetSizeX(),Parameters.CanvasRenderTarget->GetSizeY(),1.0f);
					// draw batched items
					Parameters.RenderData->BatchedElements.Draw(
						Context,
						Parameters.RenderData->Transform,
						Parameters.ViewportSizeX,
						Parameters.ViewportSizeY,
						Parameters.bHitTesting,
						1.0f / Parameters.DisplayGamma
						);
					delete Parameters.RenderData;
				});
			}
		}
		Data = NULL;
		return bDirty;
	}

	/**
	* Determine if this is a matching set by comparing texture,blendmode,elementype,transform. All must match
	*
	* @param InTexture - texture resource for the item being rendered
	* @param InBlendMode - current alpha blend mode 
	* @param InElementType - type of item being rendered: triangle,line,etc
	* @param InTransform - the transform for the item being rendered
	* @return TRUE if the parameters match this render item
	*/
	UBOOL IsMatch( const FTexture* InTexture, EBlendMode InBlendMode, FCanvas::EElementType InElementType, const FMatrix& InTransform )
	{
		return(	Data->Texture == InTexture &&
				Data->BlendMode == InBlendMode &&
				Data->ElementType == InElementType &&
				Data->Transform == InTransform );
	}

	/**
	* Accessor for the batched elements. This can be used for adding triangles and primitives to the batched elements
	*
	* @return pointer to batched elements struct
	*/
	FBatchedElements* GetBatchedElements()
	{
		return &Data->BatchedElements;
	}

private:
	class FRenderData
	{
	public:
		/**
		* Init constructor
		*/
		FRenderData(
			const FTexture* InTexture=NULL,
			EBlendMode InBlendMode=BLEND_MAX,
			FCanvas::EElementType InElementType=FCanvas::ET_MAX,
			const FMatrix& InTransform=FMatrix::Identity )
			:	Texture(InTexture)
			,	BlendMode(InBlendMode)
			,	ElementType(InElementType)
			,	Transform(InTransform)
		{}
		/** Current batched elements, destroyed once rendering completes. */
		FBatchedElements BatchedElements;
		/** Current texture being used for batching, set to NULL if it hasn't been used yet. */
		const FTexture* Texture;
		/** Current blend mode being used for batching, set to BLEND_MAX if it hasn't been used yet. */
		EBlendMode BlendMode;
		/** Current element type being used for batching, set to ET_MAX if it hasn't been used yet. */
		FCanvas::EElementType ElementType;
		/** Transform used to render including projection */
		FMatrix Transform;
	};
	/**
	* Render data which is allocated when a new FCanvasBatchedElementRenderItem is added for rendering.
	* This data is only freed on the rendering thread once the item has finished rendering
	*/
	FRenderData* Data;		
};

/**
* Info needed to render a single FTileRenderer
*/
class FCanvasTileRendererItem : public FCanvasBaseRenderItem
{
public:
	/** 
	* Init constructor 
	*/
	FCanvasTileRendererItem( 
		FLOAT InX=0.f,
		FLOAT InY=0.f,
		FLOAT InSizeX=0.f,
		FLOAT InSizeY=0.f,
		FLOAT InU=0.f,
		FLOAT InV=0.f,
		FLOAT InSizeU=0.f,
		FLOAT InSizeV=0.f,
		const FMaterialRenderProxy* InMaterialRenderProxy=NULL,
		const FMatrix& InTransform=FMatrix::Identity )
		// this data is deleted after rendering has completed
		:	Data(new FRenderData(InX,InY,InSizeX,InSizeY,InU,InV,InSizeU,InSizeV,InMaterialRenderProxy,InTransform))	
	{}

	/**
	* FCanvasTileRendererItem instance accessor
	*
	* @return this instance
	*/
	virtual class FCanvasTileRendererItem* GetCanvasTileRendererItem() 
	{ 
		return this; 
	}

	/**
	* Renders the canvas item. 
	* Iterates over each tile to be rendered and draws it with its own transforms
	*
	* @param Canvas - canvas currently being rendered
	* @return TRUE if anything rendered
	*/
	virtual UBOOL Render( const FCanvas* Canvas )
	{
		checkSlow(Data);
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		FSceneViewFamily* ViewFamily = new FSceneViewFamily(
			CanvasRenderTarget,
			NULL,
			SHOW_DefaultGame,
			GWorld->GetTimeSeconds(),
			GWorld->GetRealTimeSeconds(),
			NULL,FALSE,TRUE,TRUE,
			CanvasRenderTarget->GetDisplayGamma()
			);

		// make a temporary view
		FViewInfo* View = new FViewInfo(ViewFamily, 
			NULL, 
			NULL, 
			NULL, 
			NULL, 
			NULL, 
			NULL, 
			0, 
			0, 
			CanvasRenderTarget->GetSizeX(), 
			CanvasRenderTarget->GetSizeY(), 
			FMatrix::Identity, 
			Data->Transform, 
			FLinearColor::Black, 
			FLinearColor::White, 
			FLinearColor::White, 
			TArray<FPrimitiveSceneInfo*>()
			);
		// Render the batched elements.
		if( IsInRenderingThread() )
		{
			FTileRenderer TileRenderer;
			FCommandContextRHI* GlobalContext = RHIGetGlobalContext();
			TileRenderer.DrawTile(
				GlobalContext, 
				*View, 
				Data->MaterialRenderProxy, 
				Data->X, Data->Y, Data->SizeX, Data->SizeY, 
				Data->U, Data->V, Data->SizeU, Data->SizeV,
				Canvas->IsHitTesting(), Canvas->GetHitProxyId()
				);

			delete View->Family;
			delete View;
			delete Data;
		}
		else
		{
			struct FDrawTileParameters
			{
				FViewInfo* View;
				FRenderData* RenderData;
				UBOOL bIsHitTesting;
				FHitProxyId HitProxyId;
			};
			FDrawTileParameters DrawTileParameters =
			{
				View,
				Data,
				Canvas->IsHitTesting(),
				Canvas->GetHitProxyId()
			};
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				DrawTileCommand,
				FDrawTileParameters,Parameters,DrawTileParameters,
			{
				FTileRenderer TileRenderer;
				FCommandContextRHI* GlobalContext = RHIGetGlobalContext();
				TileRenderer.DrawTile(
					GlobalContext, 
					*Parameters.View, 
					Parameters.RenderData->MaterialRenderProxy, 
					Parameters.RenderData->X, Parameters.RenderData->Y, Parameters.RenderData->SizeX, Parameters.RenderData->SizeY, 
					Parameters.RenderData->U, Parameters.RenderData->V, Parameters.RenderData->SizeU, Parameters.RenderData->SizeV,
					Parameters.bIsHitTesting, Parameters.HitProxyId
					);

				delete Parameters.View->Family;
				delete Parameters.View;
				delete Parameters.RenderData;
			});
		}

		return TRUE;
	}

	/**
	* Determine if this is a matching set by comparing material,transform. All must match
	*
	* @param IInMaterialRenderProxy - material proxy resource for the item being rendered
	* @param InTransform - the transform for the item being rendered
	* @return TRUE if the parameters match this render item
	*/
	UBOOL IsMatch( const FMaterialRenderProxy* InMaterialRenderProxy, const FMatrix& InTransform )
	{
		return( Data->MaterialRenderProxy == InMaterialRenderProxy && 
			Data->Transform == InTransform );
	};

private:
	class FRenderData
	{
	public:
		FRenderData(
			FLOAT InX=0.f,
			FLOAT InY=0.f,
			FLOAT InSizeX=0.f,
			FLOAT InSizeY=0.f,
			FLOAT InU=0.f,
			FLOAT InV=0.f,
			FLOAT InSizeU=0.f,
			FLOAT InSizeV=0.f,
			const FMaterialRenderProxy* InMaterialRenderProxy=NULL,
			const FMatrix& InTransform=FMatrix::Identity )
			:	X(InX),Y(InY),SizeX(InSizeX),SizeY(InSizeY)
			,	U(InU),V(InV),SizeU(InSizeU),SizeV(InSizeV)
			,	MaterialRenderProxy(InMaterialRenderProxy)
			,	Transform(InTransform)
		{}
		FLOAT X,Y;
		FLOAT SizeX,SizeY;
		FLOAT U,V;
		FLOAT SizeU,SizeV;
		const FMaterialRenderProxy* MaterialRenderProxy;
		FMatrix Transform;
	};
	/**
	* Render data which is allocated when a new FCanvasTileRendererItem is added for rendering.
	* This data is only freed on the rendering thread once the item has finished rendering
	*/
	FRenderData* Data;		
};

/**
* Get the sort element for the given sort key. Allocates a new entry if one does not exist
*
* @param DepthSortKey - the key used to find the sort element entry
* @return sort element entry
*/
FCanvas::FCanvasSortElement& FCanvas::GetSortElement(INT DepthSortKey)
{
	// find the FCanvasSortElement array entry based on the sortkey
	INT ElementIdx = INDEX_NONE;
	INT* ElementIdxFromMap = SortedElementLookupMap.Find(DepthSortKey);
	if( ElementIdxFromMap )
	{
		ElementIdx = *ElementIdxFromMap;
		checkSlow( SortedElements.IsValidIndex(ElementIdx) );
	}	
	// if it doesn't exist then add a new entry (no duplicates allowed)
	else
	{
		new(SortedElements) FCanvasSortElement(DepthSortKey);
		ElementIdx = SortedElements.Num()-1;
		// keep track of newly added array index for later lookup
		SortedElementLookupMap.Set( DepthSortKey, ElementIdx );
	}
	return SortedElements(ElementIdx);
}

/**
* Returns a FBatchedElements pointer to be used for adding vertices and primitives for rendering.
* Adds a new render item to the sort element entry based on the current sort key.
*
* @param InElementType - Type of element we are going to draw.
* @param InTexture - New texture that will be set.
* @param InBlendMode - New blendmode that will be set.	* 
* @return Returns a pointer to a FBatchedElements object.
*/
FBatchedElements* FCanvas::GetBatchedElements(EElementType InElementType, const FTexture* InTexture, EBlendMode InBlendMode)
{
	// get sort element based on the current sort key from top of sort key stack
	FCanvasSortElement& SortElement = FCanvas::GetSortElement(TopDepthSortKey());	
	
	// find a batch to use 
	FCanvasBatchedElementRenderItem* RenderBatch = NULL;
	// try to use the current top entry in the render batch array
	if( SortElement.RenderBatchArray.Num() > 0 )
	{
		checkSlow( SortElement.RenderBatchArray.Last() );
		RenderBatch = SortElement.RenderBatchArray.Last()->GetCanvasBatchedElementRenderItem();
	}
	// get the current transform matrix from top of transform stack
	const FMatrix& Transform = GetFullTransform();
	// if a matching entry for this batch doesn't exist then allocate a new entry
	if( RenderBatch == NULL ||		
		!RenderBatch->IsMatch(InTexture,InBlendMode,InElementType,Transform) )
	{
		RenderBatch = new FCanvasBatchedElementRenderItem( InTexture,InBlendMode,InElementType,Transform );
		SortElement.RenderBatchArray.AddItem(RenderBatch);
	}
	return RenderBatch->GetBatchedElements();
}


/**
* Generates a new FCanvasTileRendererItem for the current sortkey and adds it to the sortelement list of itmes to render
*/
void FCanvas::AddTileRenderItem(FLOAT X,FLOAT Y,FLOAT SizeX,FLOAT SizeY,FLOAT U,FLOAT V,FLOAT SizeU,FLOAT SizeV,const FMaterialRenderProxy* MaterialRenderProxy )
{
	// get sort element based on the current sort key from top of sort key stack
	FCanvasSortElement& SortElement = FCanvas::GetSortElement(TopDepthSortKey());
	// get the current transform matrix from top of transform stack
	const FMatrix& Transform = GetFullTransform();
#if 0
	// find a batch to use 
	FCanvasTileRendererItem* RenderBatch = NULL;
	// try to use the current top entry in the render batch array
	if( SortElement.RenderBatchArray.Num() > 0 )
	{
		check( SortElement.RenderBatchArray.Last() );
		RenderBatch = SortElement.RenderBatchArray.Last()->GetCanvasBatchedElementRenderItem();
	}
	if( RenderBatch == NULL ||		
		!RenderBatch->IsMatch(MaterialRenderProxy,Transform) )
	{
		RenderBatch = new FCanvasTileRendererItem( X,Y,SizeX,SizeY,U,V,SizeU,SizeV,MaterialRenderProxy,Transform );
		SortElement.RenderBatchArray.AddItem(RenderBatch);
	}
#else
	// always add a new entry for now
	SortElement.RenderBatchArray.AddItem( new FCanvasTileRendererItem( X,Y,SizeX,SizeY,U,V,SizeU,SizeV,MaterialRenderProxy,Transform ) );
#endif
}

/** 
* Sends a message to the rendering thread to draw the batched elements. 
*/
void FCanvas::Flush()
{
	check(GetRenderTarget());

	// sort the array of FCanvasSortElement entries so that higher sort keys render first (back-to-front)
	Sort<USE_COMPARE_CONSTREF(FCanvasSortElement,UnCanvas)>( &SortedElements(0), SortedElements.Num() );

	// iterate over the FCanvasSortElements in sorted order and render all the batched items for each entry
	UBOOL bDirty = FALSE;
	for( INT Idx=0; Idx < SortedElements.Num(); Idx++ )
	{
		FCanvasSortElement& SortElement = SortedElements(Idx);
		for( INT BatchIdx=0; BatchIdx < SortElement.RenderBatchArray.Num(); BatchIdx++ )
		{
			FCanvasBaseRenderItem* RenderItem = SortElement.RenderBatchArray(BatchIdx);
			if( RenderItem )
			{
				bDirty |= RenderItem->Render(this);
				delete RenderItem;
			}			
		}
		SortElement.RenderBatchArray.Empty();
	}
	// empty the array of FCanvasSortElement entries after finished with rendering	
	SortedElements.Empty();
	SortedElementLookupMap.Empty();
	
	// mark current render target as dirty since we are drawing to it
	SetRenderTargetDirty(bDirty);
}

void FCanvas::PushRelativeTransform(const FMatrix& Transform)
{
	INT PreviousTopIndex = TransformStack.Num() - 1;
#if 0
	static UBOOL DEBUG_NoRotation=1;
	if( DEBUG_NoRotation )
	{
		FMatrix TransformNoRotation(FMatrix::Identity);
		TransformNoRotation.SetOrigin(Transform.GetOrigin());
		new(TransformStack) FMatrix(TransformNoRotation * TransformStack(PreviousTopIndex));
	}
	else
#endif
	{
		new(TransformStack) FMatrix(Transform * TransformStack(PreviousTopIndex));
	}
}

void FCanvas::PushAbsoluteTransform(const FMatrix& Transform) 
{
	new(TransformStack) FMatrix(Transform * TransformStack(0));
}

void FCanvas::PopTransform()
{
	TransformStack.Pop();
}

void FCanvas::SetHitProxy(HHitProxy* HitProxy)
{
	// Change the current hit proxy.
	CurrentHitProxy = HitProxy;

	if(HitProxyConsumer && HitProxy)
	{
		// Notify the hit proxy consumer of the new hit proxy.
		HitProxyConsumer->AddHitProxy(HitProxy);
	}
}

void FCanvas::SetRenderTarget(FRenderTarget* NewRenderTarget)
{
	if( RenderTarget != NewRenderTarget )
	{
		// flush whenever we swap render targets
		if( RenderTarget )
		{
			Flush();

			// resolve the current render target if it is dirty
			if( bRenderTargetDirty )
			{
				if( IsInRenderingThread() )
				{
					RHICopyToResolveTarget(RenderTarget->GetRenderTargetSurface(),TRUE);					
				}
				else
				{
					ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
						ResolveCanvasRTCommand,
						FRenderTarget*,CanvasRenderTarget,RenderTarget,
					{
						RHICopyToResolveTarget(CanvasRenderTarget->GetRenderTargetSurface(),TRUE);
					});
				}
				SetRenderTargetDirty(FALSE);
			}
		}
		// Change the current render target.
		RenderTarget = NewRenderTarget;
	}
}

void Clear(FCanvas* Canvas,const FLinearColor& Color)
{
	// desired display gamma space
	const FLOAT DisplayGamma = (GEngine && GEngine->Client) ? GEngine->Client->DisplayGamma : 2.2f;
	// render target gamma space expected
	FLOAT RenderTargetGamma = DisplayGamma;
	if( Canvas->GetRenderTarget() )
	{
		RenderTargetGamma = Canvas->GetRenderTarget()->GetDisplayGamma();
	}
	// assume that the clear color specified is in 2.2 gamma space
	// so convert to the render target's color space 
	FLinearColor GammaCorrectedColor(Color);
	GammaCorrectedColor.R = appPow(Clamp<FLOAT>(GammaCorrectedColor.R,0.0f,1.0f), DisplayGamma / RenderTargetGamma);
	GammaCorrectedColor.G = appPow(Clamp<FLOAT>(GammaCorrectedColor.G,0.0f,1.0f), DisplayGamma / RenderTargetGamma);
	GammaCorrectedColor.B = appPow(Clamp<FLOAT>(GammaCorrectedColor.B,0.0f,1.0f), DisplayGamma / RenderTargetGamma);

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ClearCommand,
		FColor,Color,GammaCorrectedColor,
		FRenderTarget*,CanvasRenderTarget,Canvas->GetRenderTarget(),
		{
			FCommandContextRHI* Context = RHIGetGlobalContext();
			if( CanvasRenderTarget )
			{
				RHISetRenderTarget(Context,CanvasRenderTarget->GetRenderTargetSurface(),FSurfaceRHIRef());
				RHISetViewport(Context,0,0,0.0f,CanvasRenderTarget->GetSizeX(),CanvasRenderTarget->GetSizeY(),1.0f);
			}
			RHIClear(Context,TRUE,Color,FALSE,0.0f,FALSE,0);
		});
}

/**
 *	Draws a line.
 *
 * @param	Canvas		Drawing canvas.
 * @param	StartPos	Starting position for the line.
 * @param	EndPos		Ending position for the line.
 * @param	Color		Color for the line.
 */
void DrawLine(FCanvas* Canvas,const FVector& StartPos,const FVector& EndPos,const FLinearColor& Color)
{
	FBatchedElements* BatchedElements = Canvas->GetBatchedElements(FCanvas::ET_Line);
	FHitProxyId HitProxyId = Canvas->GetHitProxyId();

	BatchedElements->AddLine(StartPos,EndPos,Color,HitProxyId);
}

/**
 *	Draws a 2D line.
 *
 * @param	Canvas		Drawing canvas.
 * @param	StartPos	Starting position for the line.
 * @param	EndPos		Ending position for the line.
 * @param	Color		Color for the line.
 */
void DrawLine2D(FCanvas* Canvas,const FVector2D& StartPos,const FVector2D& EndPos,const FLinearColor& Color)
{
	FBatchedElements* BatchedElements = Canvas->GetBatchedElements(FCanvas::ET_Line);
	FHitProxyId HitProxyId = Canvas->GetHitProxyId();

	BatchedElements->AddLine(FVector(StartPos.X,StartPos.Y,0),FVector(EndPos.X,EndPos.Y,0),Color,HitProxyId);
}

void DrawBox2D(FCanvas* Canvas,const FVector2D& StartPos,const FVector2D& EndPos,const FLinearColor& Color)
{
	DrawLine2D(Canvas,FVector2D(StartPos.X,StartPos.Y),FVector2D(StartPos.X,EndPos.Y),Color);
	DrawLine2D(Canvas,FVector2D(StartPos.X,EndPos.Y),FVector2D(EndPos.X,EndPos.Y),Color);
	DrawLine2D(Canvas,FVector2D(EndPos.X,EndPos.Y),FVector2D(EndPos.X,StartPos.Y),Color);
	DrawLine2D(Canvas,FVector2D(EndPos.X,StartPos.Y),FVector2D(StartPos.X,StartPos.Y),Color);
}

void DrawTile(
	FCanvas* Canvas,
	FLOAT X,
	FLOAT Y,
	FLOAT SizeX,
	FLOAT SizeY,
	FLOAT U,
	FLOAT V,
	FLOAT SizeU,
	FLOAT SizeV,
	const FLinearColor& Color,
	const FTexture* Texture,
	UBOOL AlphaBlend
	)
{

	FLinearColor ActualColor = Color;
	ActualColor.A *= Canvas->AlphaModulate;

	const FTexture* FinalTexture = Texture ? Texture : GWhiteTexture;
	const EBlendMode BlendMode = AlphaBlend ? BLEND_Translucent : BLEND_Opaque;
	FBatchedElements* BatchedElements = Canvas->GetBatchedElements(FCanvas::ET_Triangle, FinalTexture, BlendMode);	
	FHitProxyId HitProxyId = Canvas->GetHitProxyId();

	INT V00 = BatchedElements->AddVertex(FVector4(X,		Y,			0,1),FVector2D(U,			V),			ActualColor,HitProxyId);
	INT V10 = BatchedElements->AddVertex(FVector4(X + SizeX,Y,			0,1),FVector2D(U + SizeU,	V),			ActualColor,HitProxyId);
	INT V01 = BatchedElements->AddVertex(FVector4(X,		Y + SizeY,	0,1),FVector2D(U,			V + SizeV),	ActualColor,HitProxyId);
	INT V11 = BatchedElements->AddVertex(FVector4(X + SizeX,Y + SizeY,	0,1),FVector2D(U + SizeU,	V + SizeV),	ActualColor,HitProxyId);

	BatchedElements->AddTriangle(V00,V10,V11,FinalTexture,BlendMode);
	BatchedElements->AddTriangle(V00,V11,V01,FinalTexture,BlendMode);
}

void DrawTile(
	FCanvas* Canvas,
	FLOAT X,
	FLOAT Y,
	FLOAT SizeX,
	FLOAT SizeY,
	FLOAT U,
	FLOAT V,
	FLOAT SizeU,
	FLOAT SizeV,
	const FMaterialRenderProxy* MaterialRenderProxy
	)
{
	// add a new FTileRenderItem entry. These get rendered and freed when the canvas is flushed
	Canvas->AddTileRenderItem(X,Y,SizeX,SizeY,U,V,SizeU,SizeV,MaterialRenderProxy);
	//@todo sz - shouldn't be needed and it messes up sorting
	Canvas->Flush();
}

void DrawTriangle2D(
	FCanvas* Canvas,
	const FVector2D& Position0,
	const FVector2D& TexCoord0,
	const FVector2D& Position1,
	const FVector2D& TexCoord1,
	const FVector2D& Position2,
	const FVector2D& TexCoord2,
	const FLinearColor& Color,
	const FTexture* Texture,
	UBOOL AlphaBlend
	)
{
	const EBlendMode BlendMode = AlphaBlend ? BLEND_Translucent : BLEND_Opaque;
	const FTexture* FinalTexture = Texture ? Texture : GWhiteTexture;
	FBatchedElements* BatchedElements = Canvas->GetBatchedElements(FCanvas::ET_Triangle, FinalTexture, BlendMode);
	FHitProxyId HitProxyId = Canvas->GetHitProxyId();

	INT V0 = BatchedElements->AddVertex(FVector4(Position0.X,Position0.Y,0,1),TexCoord0,Color,HitProxyId);
	INT V1 = BatchedElements->AddVertex(FVector4(Position1.X,Position1.Y,0,1),TexCoord1,Color,HitProxyId);
	INT V2 = BatchedElements->AddVertex(FVector4(Position2.X,Position2.Y,0,1),TexCoord2,Color,HitProxyId);

	BatchedElements->AddTriangle(V0,V1,V2,FinalTexture, BlendMode);
}

void DrawTriangle2D(
	FCanvas* Canvas,
	const FVector2D& Position0,
	const FVector2D& TexCoord0,
	const FVector2D& Position1,
	const FVector2D& TexCoord1,
	const FVector2D& Position2,
	const FVector2D& TexCoord2,
	const FMaterialRenderProxy* MaterialRenderProxy
	)
{}

INT DrawStringCentered(FCanvas* Canvas,FLOAT StartX,FLOAT StartY,const TCHAR* Text,class UFont* Font,const FLinearColor& Color)
{
	INT XL, YL;
	StringSize( Font, XL, YL, Text );

	return DrawString(Canvas, StartX-(XL/2), StartY, Text, Font, Color );
}

INT DrawString(FCanvas* Canvas,FLOAT StartX,FLOAT StartY,const TCHAR* Text,class UFont* Font,const FLinearColor& Color,FLOAT XScale, FLOAT YScale)
{
	if(Font == NULL || Text == NULL)
	{
		return FALSE;
	}

	// Get the scaling and resolution information from the font.
	const FLOAT FontResolutionTest = Canvas->GetRenderTarget()->GetSizeY();
	const INT PageIndex = Font->GetResolutionPageIndex(FontResolutionTest);
	const FLOAT FontScale = Font->GetScalingFactor(FontResolutionTest);

	// apply the font's internal scale to the desired scaling
	XScale *= FontScale;
	YScale *= FontScale;

	// the position of the first character must always fall at a pixel boundary or the text will look fuzzy even at 1.0 scale 
	StartX = appTrunc(StartX);
	StartY = appTrunc(StartY);

	// Draw all characters in string.
	FLOAT LineX = 0;
	for( INT i=0; Text[i]; i++ )
	{
		INT Ch = (TCHARU)Font->RemapChar(Text[i]);

		// Process character if it's valid.
		if( Ch < Font->Characters.Num() )
		{
			FFontCharacter& Char = Font->Characters(Ch + PageIndex);
			UTexture2D* Tex;
			if( Font->Textures.IsValidIndex(Char.TextureIndex) && (Tex=Font->Textures(Char.TextureIndex))!=NULL )
			{
				const FLOAT X      = LineX + StartX;
				const FLOAT Y      = StartY;
				const FLOAT CU     = Char.StartU;
				const FLOAT CV     = Char.StartV;
				const FLOAT CUSize = Char.USize;
				const FLOAT CVSize = Char.VSize;
				const FLOAT ScaledSizeU = CUSize * XScale;
				const FLOAT ScaledSizeV = CVSize * YScale;

				// this allows us to draw half of a character if it's off the screen to the left
				// but it might not be necessary
// 				if( X < 0.f )
// 				{
// 					CU-=X;
// 					CUSize+=X;
// 					X=0;
// 				}
// 				if( Y < 0.f )
// 				{
// 					CV-=Y;
// 					CVSize+=Y;
// 					Y=0;
// 				}

				// Draw.
				DrawTile(
					Canvas,
					X,
					Y,
					ScaledSizeU,
					ScaledSizeV,
					CU		/ (FLOAT)Tex->SizeX,
					CV		/ (FLOAT)Tex->SizeY,
					CUSize	/ (FLOAT)Tex->SizeX,
					CVSize	/ (FLOAT)Tex->SizeY,
					Color,
					Tex->Resource
					);


				// Update the current rendering position
				LineX += ScaledSizeU;

				// if we have another non-whitespace character to render, add the font's kerning.
				if ( Text[i+1] && !appIsWhitespace(Text[i+1]) )
				{
					LineX += XScale * Font->Kerning;
				}
			}
		}
	}

	return appTrunc(LineX);
}

INT DrawShadowedString(FCanvas* Canvas,FLOAT StartX,FLOAT StartY,const TCHAR* Text,class UFont* Font,const FLinearColor& Color)
{
	// Draw a shadow of the text offset by 1 pixel in X and Y.
	DrawString(Canvas,StartX + 1,StartY + 1,Text,Font,FLinearColor::Black);

	// Draw the text.
	return DrawString(Canvas,StartX,StartY,Text,Font,Color);
}

/**
* Render string using both a font and a material. The material should have a font exposed as a 
* parameter so that the correct font page can be set based on the character being drawn.
*
* @param Canvas - valid canvas for rendering tiles
* @param StartX - starting X screen position
* @param StartY - starting Y screen position
* @param XScale - scale of text rendering in X direction
* @param YScale - scale of text rendering in Y direction
* @param Text - string of text to be rendered
* @param Font - font containing texture pages of character glyphs
* @param MatInst - material with a font parameter
* @param FontParam - name of the font parameter in the material
* @return total size in pixels of text drawn
*/
INT DrawStringMat(FCanvas* Canvas,FLOAT StartX,FLOAT StartY,FLOAT XScale,FLOAT YScale,const TCHAR* Text,class UFont* Font,UMaterialInterface* MatInst,const TCHAR* FontParam)
{
	checkSlow(Canvas);

	INT Result = 0;	
	if( Font && Text )
	{
		if( MatInst )
		{
			// check for valid font parameter name
			UFont* TempFont;
			INT TempFontPage;
			if( !FontParam || 
				!MatInst->GetFontParameterValue(FName(FontParam),TempFont,TempFontPage) )
			{
				//debugf(NAME_Warning,TEXT("Invalid font parameter name [%s]"),FontParam ? FontParam : TEXT("NULL"));
				Result = DrawString(Canvas,StartX,StartY,Text,Font,FLinearColor(0,1,0,1),XScale,YScale);
			}
			else
			{
				// Get the scaling and resolution information from the font.
				const FLOAT FontResolutionTest = Canvas->GetRenderTarget()->GetSizeY();
				const INT PageIndex = Font->GetResolutionPageIndex(FontResolutionTest);
				const FLOAT FontScale = Font->GetScalingFactor(FontResolutionTest);

				// apply the font's internal scale to the desired scaling
				XScale *= FontScale;
				YScale *= FontScale;

				// create a FFontMaterialRenderProxy for each font page
				TArray<FFontMaterialRenderProxy> FontMats;
				for( INT FontPage=0; FontPage < Font->Textures.Num(); FontPage++ )
				{
					new(FontMats) FFontMaterialRenderProxy(MatInst->GetRenderProxy(FALSE),Font,FontPage,FName(FontParam));
				}
				// Draw all characters in string.
				FLOAT LineX = 0;
				for( INT i=0; Text[i]; i++ )
				{
					// unicode mapping of text
					INT Ch = (TCHARU)Font->RemapChar(Text[i]);
					// Process character if it's valid.
					if( Ch < Font->Characters.Num() )
					{
						UTexture2D* Tex = NULL;
						FFontCharacter& Char = Font->Characters(Ch);
						// only render fonts with a valid texture page
						if( Font->Textures.IsValidIndex(Char.TextureIndex) && 
							(Tex=Font->Textures(Char.TextureIndex)) != NULL )
						{
							const FLOAT X			= LineX + StartX;
							const FLOAT Y			= StartY;
							const FLOAT CU			= Char.StartU;
							const FLOAT CV			= Char.StartV;
							const FLOAT CUSize		= Char.USize;
							const FLOAT CVSize		= Char.VSize;
							const FLOAT ScaledSizeU	= CUSize * XScale;
							const FLOAT ScaledSizeV	= CVSize * YScale;
							
							// Draw using the font material instance
							DrawTile(
								Canvas,
								X,
								Y,
								ScaledSizeU,
								ScaledSizeU,
								CU		/ (FLOAT)Tex->SizeX,
								CV		/ (FLOAT)Tex->SizeY,
								CUSize	/ (FLOAT)Tex->SizeX,
								CVSize	/ (FLOAT)Tex->SizeY,
								&FontMats(Char.TextureIndex)
								);

							// Update the current rendering position
							LineX += ScaledSizeU;

							// if we have another non-whitespace character to render, add the font's kerning.
							if ( Text[i+1] && !appIsWhitespace(Text[i+1]) )
							{
								LineX += XScale * Font->Kerning;
							}
						}
					}
				}
				// return the resulting line position
				Result = appTrunc(LineX);
			}
		}
		else
		{
			// fallback to just using the font texture without a material
			Result = DrawString(Canvas,StartX,StartY,Text,Font,FLinearColor(0,1,0,1),XScale,YScale);
		}
	}

	return Result;
}

//@fixme - this doesn't work correctly for scaled or multifonts because we don't have access to an FCanvas.  There are
// a million places that call this function, which makes it a huge task to change it's signature.  When this signature
// *IS* changed, it should be changed to take a single struct parameter.
// hmmm, maybe a GCanvas?  >:)
void StringSize(UFont* Font,INT& XL,INT& YL,const TCHAR* Format,...)
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), ARRAY_COUNT(Text)-1, Format, Format );

#if 0

	// Get the scaling and resolution information from the font.
	const FLOAT FontResolutionTest = Canvas->GetRenderTarget()->GetSizeY();
	const INT PageIndex = Font->GetResolutionPageIndex(FontResolutionTest);
	const FLOAT FontScale = Font->GetScalingFactor(FontResolutionTest);

	// apply the font's internal scale to the desired scaling
	XScale *= FontScale;
	YScale *= FontScale;
#else
	const FLOAT XScale = 1.f, YScale = 1.f;
#endif

	// this functionality has been moved to a static function in UIString
	FRenderParameters Parameters(Font,XScale,YScale);
	UUIString::StringSize(Parameters, Text);

	XL = appTrunc(Parameters.DrawXL);
	YL = appTrunc(Parameters.DrawYL);
}

/*-----------------------------------------------------------------------------
	UCanvas object functions.
-----------------------------------------------------------------------------*/

void UCanvas::Init()
{
}

void UCanvas::Update()
{
	// Call UnrealScript to reset.
	eventReset();

	// Copy size parameters from viewport.
	ClipX = SizeX;
	ClipY = SizeY;

}

/*-----------------------------------------------------------------------------
	UCanvas scaled sprites.
-----------------------------------------------------------------------------*/

//
// Draw arbitrary aligned rectangle.
//
void UCanvas::DrawTile
(
	UTexture2D*			Tex,
	FLOAT				X,
	FLOAT				Y,
	FLOAT				XL,
	FLOAT				YL,
	FLOAT				U,
	FLOAT				V,
	FLOAT				UL,
	FLOAT				VL,
	const FLinearColor&	Color
)
{
    if ( !Canvas || !Tex ) 
        return;

	FLOAT MyClipX = OrgX + ClipX;
	FLOAT MyClipY = OrgY + ClipY;
	FLOAT w = X + XL > MyClipX ? MyClipX - X : XL;
	FLOAT h = Y + YL > MyClipY ? MyClipY - Y : YL;
	if (XL > 0.f &&
		YL > 0.f)
	{
		::DrawTile(
			Canvas,
			(X),
			(Y),
			(w),
			(h),
			U/Tex->SizeX,
			V/Tex->SizeY,
			UL/Tex->SizeX * w/XL,
			VL/Tex->SizeY * h/YL,
			Color,
			Tex->Resource
			);		
	}
}

void UCanvas::DrawMaterialTile(
	UMaterialInterface*	Material,
	FLOAT				X,
	FLOAT				Y,
	FLOAT				XL,
	FLOAT				YL,
	FLOAT				U,
	FLOAT				V,
	FLOAT				UL,
	FLOAT				VL
	)
{
    if ( !Canvas || !Material ) 
        return;

	::DrawTile(
		Canvas,
		(X),
		(Y),
		(XL),
		(YL),
		U,
		V,
		UL,
		VL,
		Material->GetRenderProxy(0)
		);
}

void UCanvas::ClippedStrLen( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Text )
{
	XL = 0;
	YL = 0;
	if (Font != NULL)
	{
		FRenderParameters Parameters(Font,ScaleX,ScaleY);
		UUIString::StringSize(Parameters, Text);

		XL = appTrunc(Parameters.DrawXL);
		YL = appTrunc(Parameters.DrawYL);
		/*
		for( INT i=0; Text[i]; i++)
		{
			INT W, H;
			Font->GetCharSize( Text[i], W, H );
			if (Text[i + 1])
			{
				W = appTrunc((FLOAT)(W / *+ SpaceX + Font->Kerning* /) * ScaleX);
			}
			else
			{
				W = appTrunc((FLOAT)(W) * ScaleX);
			}
			H = appTrunc((FLOAT)(H) * ScaleY);
			XL += W;
			if(YL < H)
			{
				YL = H;	
			}
		}
		*/
	}
}

//
// Calculate the size of a string built from a font, word wrapped
// to a specified region.
//
void VARARGS UCanvas::WrappedStrLenf( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Fmt, ... ) 
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), ARRAY_COUNT(Text)-1, Fmt, Fmt );

	WrappedPrint( 0, XL, YL, Font, ScaleX, ScaleY, 0, Text ); 
}

//
// Compute size and optionally print text with word wrap.
//!!For the next generation, redesign to ignore CurX,CurY.
//
void UCanvas::WrappedPrint( UBOOL Draw, INT& out_XL, INT& out_YL, UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Text ) 
{
	// FIXME: Wrapped Print is screwed which kills the hud.. fix later

	if( ClipX<0 || ClipY<0 )
		return;
	check(Font);

	
	FLOAT FontResolutionTest = Canvas->GetRenderTarget()->GetSizeY();
	INT ResolutionPageIndex = Font->GetResolutionPageIndex( FontResolutionTest );
	FLOAT FontScale = Font->GetScalingFactor( FontResolutionTest );

	// Process each word until the current line overflows.
	FLOAT XL=0.f, YL=0.f;
	do
	{
		INT iCleanWordEnd=0, iTestWord;
		FLOAT TestXL= CurX, CleanXL=0;
		FLOAT TestYL=0,    CleanYL=0;
		UBOOL GotWord=0;
		for( iTestWord=0; Text[iTestWord]!=0 && Text[iTestWord]!='\n'; )
		{
			FLOAT ChW, ChH;
			Font->GetCharSize(Text[iTestWord], ChW, ChH, ResolutionPageIndex);
			TestXL              += (ChW + SpaceX /*+ Font->Kerning*/) * ScaleX * FontScale; 
			TestYL               = Max(TestYL, (ChH + SpaceY) * ScaleY * FontScale);
			if( TestXL>ClipX )
				break;
			iTestWord++;
			UBOOL WordBreak = Text[iTestWord]==' ' || Text[iTestWord]=='\n' || Text[iTestWord]==0;
			if( WordBreak || !GotWord )
			{
				iCleanWordEnd = iTestWord;
				CleanXL       = TestXL;
				CleanYL       = TestYL;
				GotWord       = GotWord || WordBreak;
			}
		}
		if( iCleanWordEnd==0 )
			break;

		// Sucessfully split this line, now draw it.
		if (Draw && CurY < ClipY && CurY + CleanYL > 0)
		{
			FString TextLine(Text);
			FLOAT LineX = Center ? CurX+(ClipX-CleanXL)/2 : CurX;
			LineX += DrawString(Canvas,OrgX + LineX, OrgY + CurY,*(TextLine.Left(iCleanWordEnd)),Font,DrawColor, ScaleX, ScaleY);
			CurX = LineX;
		}

		// Update position.
		CurX  = 0;
		CurY += CleanYL;
		YL   += CleanYL;
		XL    = Max(XL,CleanXL);
		Text += iCleanWordEnd;

		// Skip whitespace after word wrap.
		while( *Text==' '|| *Text=='\n' )
			Text++;
	}
	while( *Text );

	out_XL = appTrunc(XL);
	out_YL = appTrunc(YL);
}


/*-----------------------------------------------------------------------------
	UCanvas natives.
-----------------------------------------------------------------------------*/
void UCanvas::execDrawTile( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UTexture2D,Tex);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;
	if( !Tex )
		return;

	DrawTile
	(
		Tex,
		OrgX+CurX,
		OrgY+CurY,
		XL,
		YL,
		U,
		V,
		UL,
		VL,
		DrawColor
	);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
}
IMPLEMENT_FUNCTION( UCanvas, 466, execDrawTile );

void UCanvas::execDrawColorizedTile( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UTexture2D,Tex);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_GET_STRUCT(FLinearColor,DrawColor);
	P_FINISH;
	if( !Tex )
		return;

	DrawTile
	(
		Tex,
		OrgX+CurX,
		OrgY+CurY,
		XL,
		YL,
		U,
		V,
		UL,
		VL,
		DrawColor
	);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
}
IMPLEMENT_FUNCTION( UCanvas, INDEX_NONE, execDrawColorizedTile );


void UCanvas::execDrawMaterialTile( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UMaterialInterface,Material);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT_OPTX(U,0.f);
	P_GET_FLOAT_OPTX(V,0.f);
	P_GET_FLOAT_OPTX(UL,1.f);
	P_GET_FLOAT_OPTX(VL,1.f);
	P_FINISH;
	if(!Material)
		return;
	DrawMaterialTile
	(
		Material,
		OrgX+CurX,
		OrgY+CurY,
		XL,
		YL,
		U,
		V,
		UL,
		VL
	);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
}
IMPLEMENT_FUNCTION( UCanvas, INDEX_NONE, execDrawMaterialTile );

void UCanvas::execDrawMaterialTileClipped( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UMaterialInterface,Material);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT_OPTX(U,0.f);
	P_GET_FLOAT_OPTX(V,0.f);
	P_GET_FLOAT_OPTX(UL,1.f);
	P_GET_FLOAT_OPTX(VL,1.f);
	P_FINISH;

	if ( !Material )
	{
		return;
	}

	if( CurX<0 )
		{FLOAT C=CurX*UL/XL; U-=C; UL+=C; XL+=CurX; CurX=0;}
	if( CurY<0 )
		{FLOAT C=CurY*VL/YL; V-=C; VL+=C; YL+=CurY; CurY=0;}
	if( XL>ClipX-CurX )
		{UL+=(ClipX-CurX-XL)*UL/XL; XL=ClipX-CurX;}
	if( YL>ClipY-CurY )
		{VL+=(ClipY-CurY-YL)*VL/YL; YL=ClipY-CurY;}

	DrawMaterialTile
	(
		Material,
		OrgX+CurX,
		OrgY+CurY,
		XL,
		YL,
		U,
		V,
		UL,
		VL
	);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
}
IMPLEMENT_FUNCTION( UCanvas, INDEX_NONE, execDrawMaterialTileClipped );

void UCanvas::execDrawText( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(InText);
	P_GET_UBOOL_OPTX(CR,1);
	P_GET_FLOAT_OPTX(XScale,1.0f);
	P_GET_FLOAT_OPTX(YScale,1.0f);
	P_FINISH;

	if( !Font )
	{
		Stack.Logf( NAME_Warning, TEXT("DrawText: No font") ); 
		return;
	}
	INT		XL		= 0;
	INT		YL		= 0; 
	FLOAT	OldCurX	= CurX;
	FLOAT	OldCurY	= CurY;
	WrappedPrint( 1, XL, YL, Font, XScale, YScale, bCenter, *InText ); 
    
	CurX += XL;
	CurYL = Max(CurYL,(FLOAT)YL);
	if( CR )
	{
		CurX	= OldCurX;
		CurY	= OldCurY + CurYL;
		CurYL	= 0;
	}

}
IMPLEMENT_FUNCTION( UCanvas, 465, execDrawText );

void UCanvas::execDrawTextClipped( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(InText);
	P_GET_UBOOL_OPTX(CheckHotKey, 0);
	P_GET_FLOAT_OPTX(XScale,1.0f);
	P_GET_FLOAT_OPTX(YScale,1.0f);
	P_FINISH;

	if( !Font )
	{
		Stack.Logf( TEXT("DrawTextClipped: No font") ); 
		return;
	}

	check(Font);

	DrawString(Canvas,appTrunc(OrgX + CurX), appTrunc(OrgY + CurY), *InText, Font, DrawColor, XScale, YScale);

}
IMPLEMENT_FUNCTION( UCanvas, 469, execDrawTextClipped );

void UCanvas::execStrLen( FFrame& Stack, RESULT_DECL ) // wrapped 
{
	P_GET_STR(InText);
	P_GET_FLOAT_REF(XL);
	P_GET_FLOAT_REF(YL);
	P_FINISH;

	INT XLi, YLi;
	INT OldCurX, OldCurY;

	OldCurX = appTrunc(CurX);
	OldCurY = appTrunc(CurY);
	CurX = 0;
	CurY = 0;

	WrappedStrLenf( Font, 1.f, 1.f, XLi, YLi, TEXT("%s"), *InText );

	CurY = OldCurY;
	CurX = OldCurX;
	XL = XLi;
	YL = YLi;

}
IMPLEMENT_FUNCTION( UCanvas, 464, execStrLen );

void UCanvas::execTextSize( FFrame& Stack, RESULT_DECL ) // clipped
{
	P_GET_STR(InText);
	P_GET_FLOAT_REF(XL);
	P_GET_FLOAT_REF(YL);
	P_FINISH;

	INT XLi, YLi;

	if( !Font )
	{
		Stack.Logf( TEXT("TextSize: No font") ); 
		return;
	}

	ClippedStrLen( Font, 1.f, 1.f, XLi, YLi, *InText );

	XL = XLi;
	YL = YLi;

}
IMPLEMENT_FUNCTION( UCanvas, 470, execTextSize );

void UCanvas::execProject( FFrame& Stack, RESULT_DECL )
{
	P_GET_VECTOR(Location);
	P_FINISH;

	FPlane V(0,0,0,0);

	if (SceneView!=NULL)
		V = SceneView->Project(Location);

	FVector resultVec(V);
	resultVec.X = (ClipX/2.f) + (resultVec.X*(ClipX/2.f));
	resultVec.Y *= -1.f;
	resultVec.Y = (ClipY/2.f) + (resultVec.Y*(ClipY/2.f));
	*(FVector*)Result =	resultVec;

}
IMPLEMENT_FUNCTION( UCanvas, -1, execProject);

void UCanvas::execDrawTileClipped( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UTexture2D,Tex);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;

	if( !Tex )
	{
		Stack.Logf( TEXT("DrawTileClipped: Missing Material") );
		return;
	}


	// Clip to ClipX and ClipY
	if( XL > 0 && YL > 0 )
	{		
		if( CurX<0 )
			{FLOAT C=CurX*UL/XL; U-=C; UL+=C; XL+=CurX; CurX=0;}
		if( CurY<0 )
			{FLOAT C=CurY*VL/YL; V-=C; VL+=C; YL+=CurY; CurY=0;}
		if( XL>ClipX-CurX )
			{UL+=(ClipX-CurX-XL)*UL/XL; XL=ClipX-CurX;}
		if( YL>ClipY-CurY )
			{VL+=(ClipY-CurY-YL)*VL/YL; YL=ClipY-CurY;}
	
		DrawTile
		(
			Tex,
			OrgX+CurX,
			OrgY+CurY,
			XL,
			YL,
			U,
			V,
			UL,
			VL,
			DrawColor
		);

		CurX += XL + SpaceX;
		CurYL = Max(CurYL,YL);
	}

}
IMPLEMENT_FUNCTION( UCanvas, 468, execDrawTileClipped );


void UCanvas::execDrawTileStretched( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UTexture2D,Tex);
	P_GET_FLOAT(AWidth);
	P_GET_FLOAT(AHeight);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_GET_STRUCT(FLinearColor,DrawColor);
	P_GET_UBOOL_OPTX(bStretchHorizontally,TRUE);
	P_GET_UBOOL_OPTX(bStretchVertically,TRUE);
	P_GET_FLOAT_OPTX(ScalingFactor,1.0);
	P_FINISH;

	DrawTileStretched(Tex,CurX, CurY, AWidth, AHeight, U, V, UL, VL, DrawColor,bStretchHorizontally,bStretchVertically,ScalingFactor);

}
IMPLEMENT_FUNCTION( UCanvas, -1, execDrawTileStretched );

void UCanvas::DrawTileStretched(UTexture2D* Tex, FLOAT Left, FLOAT Top, FLOAT AWidth, FLOAT AHeight, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLinearColor DrawColor, UBOOL bStretchHorizontally,UBOOL bStretchVertically, FLOAT ScalingFactor)
{
	// Offset for the origin.
	Left += OrgX;
	Top += OrgY;

	// Get the midpoints of the texture
	FLOAT TexWidth = UL * 0.5;
	FLOAT TexHeight = VL * 0.5;

	// FullXxxxx holds the final width/height of the whole image (Screen Pixels)
	FLOAT FullWidth = Abs<FLOAT>(AWidth);
	FLOAT FullHeight = Abs<FLOAT>(AHeight);

	// this represents the size of the regions that will be filled (Screen Pixels).  Make sure to 
	// account for the scaling factor
	FLOAT ScreenFillWidth = UL > 0 ? FullWidth - (UL * ScalingFactor) : FullWidth + (UL * ScalingFactor); 
	FLOAT ScreenFillHeight = VL > 0 ? FullHeight - (VL* ScalingFactor) : FullHeight + (VL* ScalingFactor); 

	// Amount of space to tile from in (image pixels)
	FLOAT TexTileWidth, TexTileHeight;	

	FLOAT AbsTexWidth = Abs<FLOAT>(TexWidth);
	FLOAT AbsTexHeight = Abs<FLOAT>(TexHeight);

	// Top and Bottom
	if( bStretchHorizontally && ScreenFillWidth > 0 )
	{
		// Need to stretch material horizontally
		TexTileWidth = TexWidth;

		// Figure out how much of the source image do we need to display
		if( !(bStretchVertically && ScreenFillHeight > 0) )
		{
			// if we're not stretching the vertical orientation, we should scale it
			TexTileHeight = FullHeight * 0.5;
		}
		else
		{
			TexTileHeight = TexHeight;
		}

		// draw the upper middle stretched portion of the image
		DrawTile(Tex,	Left + AbsTexWidth,	Top,							ScreenFillWidth,	Abs<FLOAT>(TexTileHeight),	U + TexWidth,	V,					1,			TexHeight,	DrawColor);
		// draw the lower middle stretched portion of the image
		DrawTile(Tex,	Left + AbsTexWidth,	Top + AHeight - AbsTexHeight,	ScreenFillWidth,	Abs<FLOAT>(TexTileHeight),	U + TexWidth,	V + VL - TexHeight,	1,			TexHeight,	DrawColor);
	}
	else
	{
		TexTileWidth = FullWidth * 0.5f;
	}

	// Left and Right
	if( bStretchVertically && ScreenFillHeight > 0 )
	{
		// Need to stretch material vertically
		TexTileHeight = TexHeight;

		if ( !(bStretchHorizontally && ScreenFillWidth > 0) )
		{
			// if we're not stretching the horizontal orientation, scale it
			TexTileWidth = FullWidth * 0.5f;
		}
		else
		{
			TexTileWidth = TexWidth;
		}

		// draw the left middle stretched portion of the image
		DrawTile(Tex,	Left,							Top + AbsTexHeight,	Abs<FLOAT>(TexTileWidth),	ScreenFillHeight,	U,					V + TexTileHeight,	TexWidth,	1,	DrawColor);
		// draw the right middle stretched portion of the image
		DrawTile(Tex,	Left + AWidth - AbsTexWidth,	Top + AbsTexHeight,	Abs<FLOAT>(TexTileWidth),	ScreenFillHeight,	U + UL - TexWidth,	V + TexTileHeight,	TexWidth,	1, DrawColor);
	}
	else
	{
		TexTileHeight = FullHeight * 0.5f;
	}

	// If we had to stretch the material both ways, repeat the middle pixels of the image to fill the gap
	if( (bStretchHorizontally && ScreenFillWidth > 0) && (bStretchVertically && ScreenFillHeight > 0) )
	{
		DrawTile(Tex,Left + AbsTexWidth, Top + AbsTexHeight, ScreenFillWidth, ScreenFillHeight, U+TexWidth, V+TexHeight, 1, 1, DrawColor);
	}

	FLOAT AbsTexTileWidth = Abs<FLOAT>(TexTileWidth);
	FLOAT AbsTexTileHeight = Abs<FLOAT>(TexTileHeight);

	// Draw the 4 corners - each quadrant is scaled if its area is smaller than the destination area
	//topleft
	DrawTile(Tex,	Left,							Top,							AbsTexTileWidth, AbsTexTileHeight,	U,						V,						TexTileWidth, TexTileHeight, DrawColor);
	//topright
	DrawTile(Tex,	Left + AWidth - AbsTexWidth,	Top,							AbsTexTileWidth, AbsTexTileHeight,	U + UL - TexTileWidth,	V,						TexTileWidth, TexTileHeight, DrawColor);
	//bottomleft
	DrawTile(Tex,	Left,							Top + AHeight - AbsTexHeight,	AbsTexTileWidth, AbsTexTileHeight,	U,						V + VL - TexTileHeight,	TexTileWidth, TexTileHeight, DrawColor);
	//bottomright
	DrawTile(Tex,	Left + AWidth - AbsTexWidth,	Top + AHeight - AbsTexHeight,	AbsTexTileWidth, AbsTexTileHeight,	U + UL - TexTileWidth,	V + VL - TexTileHeight,	TexTileWidth, TexTileHeight, DrawColor);
}

void UCanvas::execDrawRotatedTile(FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UTexture2D,Tex);
	P_GET_ROTATOR(Rotation);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;

	if(!Tex)
	{
		return;
	}

	// Figure out where we are drawing
	FVector Position( OrgX + CurX, OrgY + CurY, 0 );

	// Anchor is the center of the tile
	FVector AnchorPos( XL * 0.5, YL * 0.5, 0 );

	FRotationMatrix RotMatrix( Rotation );
	FMatrix TransformMatrix = FTranslationMatrix(-AnchorPos) * RotMatrix * FTranslationMatrix(AnchorPos);

	// translate the matrix back to origin, apply the rotation matrix, then transform back to the current position
 	FMatrix FinalTransform = FTranslationMatrix(-Position) * TransformMatrix * FTranslationMatrix(Position);

	Canvas->PushRelativeTransform(FinalTransform);
	DrawTile(Tex,OrgX+CurX,OrgY+CurY,XL,YL,U,V,UL,VL,DrawColor);
	Canvas->PopTransform();
}
IMPLEMENT_FUNCTION( UCanvas, -1, execDrawRotatedTile);

void UCanvas::execDrawRotatedMaterialTile(FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UMaterialInterface,Material);
	P_GET_ROTATOR(Rotation);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT_OPTX(U,0.f);
	P_GET_FLOAT_OPTX(V,0.f);
	P_GET_FLOAT_OPTX(UL,0.f);
	P_GET_FLOAT_OPTX(VL,0.f);
	P_FINISH;

	if(!Material)
	{
		return;
	}

	UMaterial* Mat = Material->GetMaterial();

	if (UL <= 0.0f)
	{
		UL = 1.0f;
	}
	if (VL <= 0.0f) 
	{
		VL = 1.0f;
	}

	// Figure out where we are drawing
	FVector Position( OrgX + CurX, OrgY + CurY, 0 );

	// Anchor is the center of the tile
	FVector AnchorPos( XL * 0.5, YL * 0.5, 0 );

	FRotationMatrix RotMatrix( Rotation );
	FMatrix TransformMatrix;
	TransformMatrix = FTranslationMatrix(-AnchorPos) * RotMatrix * FTranslationMatrix(AnchorPos);

	// translate the matrix back to origin, apply the rotation matrix, then transform back to the current position
 	FMatrix FinalTransform = FTranslationMatrix(-Position) * TransformMatrix * FTranslationMatrix(Position);

	Canvas->PushRelativeTransform(FinalTransform);
	DrawMaterialTile(Material,OrgX+CurX,OrgY+CurY,XL,YL,U,V,UL,VL);
	Canvas->PopTransform();

}
IMPLEMENT_FUNCTION( UCanvas, -1, execDrawRotatedMaterialTile);

void UCanvas::execDraw2DLine(FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT(X1);
	P_GET_FLOAT(Y1);
	P_GET_FLOAT(X2);
	P_GET_FLOAT(Y2);
	P_GET_STRUCT(FColor,LineColor);
	P_FINISH;

	X1+= OrgX;
	X2+= OrgX;
	Y1+= OrgY;
	Y2+= OrgY;

	DrawLine2D(Canvas, FVector2D(X1, Y1), FVector2D(X2, Y2), LineColor);
}

IMPLEMENT_FUNCTION(UCanvas, -1, execDraw2DLine);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
