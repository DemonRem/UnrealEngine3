/*================================================================================
	LandscapeEdMode.cpp: Landscape editing mode
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#include "UnrealEd.h"
#include "UnObjectTools.h"
#include "LandscapeEdMode.h"
#include "ScopedTransaction.h"
#include "EngineTerrainClasses.h"
#include "LandscapeEdit.h"
#include "LandscapeRender.h"

#if WITH_MANAGED_CODE
#include "LandscapeEditWindowShared.h"
#endif

//
// FLandscapeHeightCache
//
template<class Accessor, typename AccessorType>
struct TLandscapeEditCache
{
	TLandscapeEditCache(Accessor& InDataAccess)
	:	DataAccess(InDataAccess)
	,	Valid(FALSE)
	{
	}

	void CacheData( INT X1, INT Y1, INT X2, INT Y2 )
	{
		if( !Valid )
		{
			CachedX1 = X1;
			CachedY1 = Y1;
			CachedX2 = X2;
			CachedY2 = Y2;

			DataAccess.GetData( CachedX1, CachedY1, CachedX2, CachedY2, CachedData );
			Valid = TRUE;
		}
		else
		{
			// Extend the cache area if needed
			if( X1 < CachedX1 )
			{
				DataAccess.GetData( X1, Min<INT>(Y1,CachedY1), CachedX1-1, Max<INT>(Y2,CachedY2), CachedData );
			}

			if( X2 > CachedX2 )
			{
				DataAccess.GetData( CachedX2+1, Min<INT>(Y1,CachedY1), X2, Max<INT>(Y2,CachedY2), CachedData );
			}

			if( Y1 < CachedY1 )
			{
				DataAccess.GetData( CachedX1, Y1, CachedX2, CachedY1-1, CachedData );
			}

			if( Y2 > CachedY2 )
			{
				DataAccess.GetData( CachedX1, CachedY2+1, CachedX2, Y2, CachedData );
			}

			// update
			CachedX1 = Min<INT>(X1,CachedX1);
			CachedY1 = Min<INT>(Y1,CachedY1);
			CachedX2 = Max<INT>(X2,CachedX2);
			CachedY2 = Max<INT>(Y2,CachedY2);
		}	
	}

	AccessorType* GetValueRef(INT LandscapeX, INT LandscapeY)
	{
		return CachedData.Find(ALandscape::MakeKey(LandscapeX,LandscapeY));
	}

	void SetValue(INT LandscapeX, INT LandscapeY, AccessorType Value)
	{
		CachedData.Set(ALandscape::MakeKey(LandscapeX,LandscapeY), Value);
	}

	void GetCachedData(INT X1, INT Y1, INT X2, INT Y2, TArray<AccessorType>& OutData)
	{
		INT NumSamples = (1+X2-X1)*(1+Y2-Y1);
		OutData.Empty(NumSamples);
		OutData.Add(NumSamples);

		for( INT Y=Y1;Y<=Y2;Y++ )
		{
			for( INT X=X1;X<=X2;X++ )
			{
				AccessorType* Ptr = GetValueRef(X,Y);
				if( Ptr )
				{
					OutData((X-X1) + (Y-Y1)*(1+X2-X1)) = *Ptr;
				}
			}
		}
	}

	void SetCachedData(INT X1, INT Y1, INT X2, INT Y2, TArray<AccessorType>& Data)
	{
		// Update cache
		for( INT Y=Y1;Y<=Y2;Y++ )
		{
			for( INT X=X1;X<=X2;X++ )
			{
				SetValue( X, Y, Data((X-X1) + (Y-Y1)*(1+X2-X1)) );
			}
		}

		// Update real data
		DataAccess.SetData( X1, Y1, X2, Y2, &Data(0) );
	}

	void Flush()
	{
		DataAccess.Flush();
	}

protected:
	Accessor& DataAccess;
private:
	TMap<QWORD, AccessorType> CachedData;
	UBOOL Valid;

	INT CachedX1;
	INT CachedY1;
	INT CachedX2;
	INT CachedY2;
};

//
// FHeightmapAccessor
//
struct FHeightmapAccessor
{
	FHeightmapAccessor( ALandscape* InLandscape )
	{
		LandscapeEdit = new FLandscapeEditDataInterface(InLandscape);
	}

	// accessors
	void GetData(INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, WORD>& Data)
	{
		LandscapeEdit->GetHeightData( X1, Y1, X2, Y2, Data);
	}

	void SetData(INT X1, INT Y1, INT X2, INT Y2, const WORD* Data )
	{
		LandscapeEdit->SetHeightData( X1, Y1, X2, Y2, Data, 0, TRUE);
		LandscapeEdit->GetComponentsInRegion(X1, Y1, X2, Y2, ChangedComponents);
	}

	void Flush()
	{
		LandscapeEdit->Flush();
	}

	virtual ~FHeightmapAccessor()
	{
		delete LandscapeEdit;
		LandscapeEdit = NULL;

		// Update the bounds for the components we edited
		for(TSet<ULandscapeComponent*>::TConstIterator It(ChangedComponents);It;++It)
		{
			(*It)->UpdateCachedBounds();
			(*It)->ConditionalUpdateTransform();
		}
	}

private:
	FLandscapeEditDataInterface* LandscapeEdit;
	TSet<ULandscapeComponent*> ChangedComponents;
};

struct FLandscapeHeightCache : public TLandscapeEditCache<FHeightmapAccessor,WORD>
{
	typedef WORD DataType;
	static WORD ClampValue( INT Value ) { return Clamp(Value, 0, 65535); }

	FHeightmapAccessor HeightmapAccessor;

	FLandscapeHeightCache(const FLandscapeToolTarget& InTarget)
	:	HeightmapAccessor(InTarget.Landscape)
	,	TLandscapeEditCache(HeightmapAccessor)
	{
	}
};

//
// FAlphamapAccessor
//
struct FAlphamapAccessor
{
	FAlphamapAccessor( ALandscape* InLandscape, FName InLayerName )
		:	LandscapeEdit(InLandscape)
		,	LayerName(InLayerName)
		,	bBlendWeight(TRUE)
	{
		// should be no Layer change during FAlphamapAccessor lifetime...
		if (InLandscape && LayerName != NAME_None)
		{
			for (INT LayerIdx = 0; LayerIdx < InLandscape->LayerInfos.Num(); LayerIdx++)
	{
				if (InLandscape->LayerInfos(LayerIdx).LayerName == LayerName)
				{
					bBlendWeight = !InLandscape->LayerInfos(LayerIdx).bNoWeightBlend;
					break;
				}
			}
		}
	}

	void GetData(INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, BYTE>& Data)
	{
		LandscapeEdit.GetWeightData(LayerName, X1, Y1, X2, Y2, Data);
	}

	void SetData(INT X1, INT Y1, INT X2, INT Y2, const BYTE* Data )
	{
		LandscapeEdit.SetAlphaData(LayerName, X1, Y1, X2, Y2, Data, 0, bBlendWeight);
	}

	void Flush()
	{
		LandscapeEdit.Flush();
	}

private:
	FLandscapeEditDataInterface LandscapeEdit;
	FName LayerName;
	BOOL bBlendWeight;
};

struct FLandscapeAlphaCache : public TLandscapeEditCache<FAlphamapAccessor,BYTE>
{
	typedef BYTE DataType;
	static BYTE ClampValue( INT Value ) { return Clamp(Value, 0, 255); }

	FAlphamapAccessor AlphamapAccessor;

	FLandscapeAlphaCache(const FLandscapeToolTarget& InTarget)
		:	AlphamapAccessor(InTarget.Landscape, InTarget.LayerName)
		,	TLandscapeEditCache(AlphamapAccessor)
	{
	}
};

//
// FFullWeightmapAccessor
//
struct FFullWeightmapAccessor
{
	FFullWeightmapAccessor( ALandscape* InLandscape)
		:	LandscapeEdit(InLandscape)
	{
	}
	void GetData(INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, TArray<BYTE>>& Data)
	{
		LandscapeEdit.GetWeightData(NAME_None, X1, Y1, X2, Y2, Data);
	}

	void SetData(INT X1, INT Y1, INT X2, INT Y2, const BYTE* Data)
	{
		LandscapeEdit.SetAlphaData(NAME_None, X1, Y1, X2, Y2, Data, 0, FALSE);
	}

	void Flush()
	{
		LandscapeEdit.Flush();
	}

private:
	FLandscapeEditDataInterface LandscapeEdit;
};

struct FLandscapeFullWeightCache : public TLandscapeEditCache<FFullWeightmapAccessor,TArray<BYTE>>
{
	typedef TArray<BYTE> DataType;

	FFullWeightmapAccessor WeightmapAccessor;

	FLandscapeFullWeightCache(const FLandscapeToolTarget& InTarget)
		:	WeightmapAccessor(InTarget.Landscape)
		,	TLandscapeEditCache(WeightmapAccessor)
	{
	}

	// Only for all weight case... the accessor type should be TArray<BYTE>
	void GetCachedData(INT X1, INT Y1, INT X2, INT Y2, TArray<BYTE>& OutData, INT ArraySize)
	{
		INT NumSamples = (1+X2-X1)*(1+Y2-Y1) * ArraySize;
		OutData.Empty(NumSamples);
		OutData.Add(NumSamples);

		for( INT Y=Y1;Y<=Y2;Y++ )
		{
			for( INT X=X1;X<=X2;X++ )
			{
				TArray<BYTE>* Ptr = GetValueRef(X,Y);
				if( Ptr )
				{
					for (INT Z = 0; Z < ArraySize; Z++)
					{
						OutData(( (X-X1) + (Y-Y1)*(1+X2-X1)) * ArraySize + Z) = (*Ptr)(Z);
					}
				}
			}
		}
	}

	// Only for all weight case... the accessor type should be TArray<BYTE>
	void SetCachedData(INT X1, INT Y1, INT X2, INT Y2, TArray<BYTE>& Data, INT ArraySize)
	{
		// Update cache
		for( INT Y=Y1;Y<=Y2;Y++ )
		{
			for( INT X=X1;X<=X2;X++ )
			{
				TArray<BYTE> Value;
				Value.Empty(ArraySize);
				Value.Add(ArraySize);
				for ( INT Z=0; Z < ArraySize; Z++)
				{
					Value(Z) = Data( ((X-X1) + (Y-Y1)*(1+X2-X1)) * ArraySize + Z);
				}
				SetValue( X, Y, Value );
			}
		}

		// Update real data
		DataAccess.SetData( X1, Y1, X2, Y2, &Data(0) );
	}
};

// 
// FLandscapeBrush
//

void FLandscapeBrush::BeginStroke(FLOAT LandscapeX, FLOAT LandscapeY, class FLandscapeTool* CurrentTool)
{
	GEditor->BeginTransaction( *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeMode_EditTransaction"), CurrentTool->GetIconString())) );
}

void FLandscapeBrush::EndStroke()
{
	GEditor->EndTransaction();
}

// 
// FLandscapeBrushCircle
//

class FLandscapeBrushCircle : public FLandscapeBrush
{
	FVector2D LastMousePosition;
	ALandscape* LastLandscape;

	TSet<ULandscapeComponent*> BrushMaterialComponents;
protected:
	UMaterialInstanceConstant* BrushMaterial;
	virtual FLOAT CalculateFalloff( FLOAT Distance, FLOAT Radius, FLOAT Falloff ) = 0;
public:
	class FEdModeLandscape* EdMode;

	FLandscapeBrushCircle(class FEdModeLandscape* InEdMode)
	:	EdMode(InEdMode)
	{
		BrushMaterial = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass());
		BrushMaterial->AddToRoot();
	}

	virtual ~FLandscapeBrushCircle()
	{
		BrushMaterial->RemoveFromRoot();
	}

	void LeaveBrush()
	{
		for( TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It )
		{
			if( (*It)->EditToolRenderData != NULL )
			{
				(*It)->EditToolRenderData->Update(NULL);
			}
		}
		BrushMaterialComponents.Empty();
	}

	void BeginStroke(FLOAT LandscapeX, FLOAT LandscapeY, class FLandscapeTool* CurrentTool)
	{
		FLandscapeBrush::BeginStroke(LandscapeX,LandscapeY,CurrentTool);
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime)
	{
		ALandscape* Landscape = EdMode->CurrentToolTarget.Landscape;
		if( Landscape )
		{
			FLOAT ScaleXY = Landscape->DrawScale3D.X * Landscape->DrawScale;
			FLOAT Radius = (1.f - EdMode->UISettings.GetBrushFalloff()) * EdMode->UISettings.GetBrushRadius() / ScaleXY;
			FLOAT Falloff = EdMode->UISettings.GetBrushFalloff() * EdMode->UISettings.GetBrushRadius() / ScaleXY;

			// Set params for brush material.
			FVector WorldLocation = FVector(LastMousePosition.X,LastMousePosition.Y,0) * ScaleXY + Landscape->Location;
			BrushMaterial->SetScalarParameterValue(FName(TEXT("WorldRadius")), ScaleXY*Radius);
			BrushMaterial->SetScalarParameterValue(FName(TEXT("WorldFalloff")), ScaleXY*Falloff);
			BrushMaterial->SetVectorParameterValue(FName(TEXT("WorldPosition")), FLinearColor(WorldLocation.X,WorldLocation.Y,WorldLocation.Z,0));

			// Set brush material.
			INT X1 = appFloor(LastMousePosition.X - (Radius+Falloff));
			INT Y1 = appFloor(LastMousePosition.Y - (Radius+Falloff));
			INT X2 = appCeil(LastMousePosition.X + (Radius+Falloff));
			INT Y2 = appCeil(LastMousePosition.Y + (Radius+Falloff));

			TSet<ULandscapeComponent*> NewComponents;
			Landscape->GetComponentsInRegion(X1,Y1,X2,Y2,NewComponents);

			// Set brush material for components in new region
			for( TSet<ULandscapeComponent*>::TIterator It(NewComponents); It; ++It )
			{
				if( (*It)->EditToolRenderData != NULL )
				{
					(*It)->EditToolRenderData->Update(BrushMaterial);
				}
			}

			// Remove the material from any old components that are no longer in the region
			TSet<ULandscapeComponent*> RemovedComponents = BrushMaterialComponents.Difference(NewComponents);
			for ( TSet<ULandscapeComponent*>::TIterator It(RemovedComponents); It; ++It )
			{
				if( (*It)->EditToolRenderData != NULL )
				{
					(*It)->EditToolRenderData->Update(NULL);
				}
			}

			BrushMaterialComponents = NewComponents;	
		}
	}

	void MouseMove(FLOAT LandscapeX, FLOAT LandscapeY)
	{
		FVector2D NewMousePosition = FVector2D(LandscapeX, LandscapeY);
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	UBOOL InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent )
	{
		UBOOL bUpdate = FALSE;
#if 0
		if( InEvent == IE_Pressed || IE_Repeat )
		{
			if( InKey == KEY_LeftBracket )
			{
				if( IsShiftDown(InViewport) )
				{
					Falloff = Max<FLOAT>(Falloff - 0.5f, 0.f);
				}
				else
				{
					Radius = Max<FLOAT>(Radius - 0.5f, 0.f);
				}
				bUpdate = TRUE;
			}
			if( InKey == KEY_RightBracket )
			{
				if( IsShiftDown(InViewport) )
				{
					Falloff += 0.5f;
				}
				else
				{
					Radius += 0.5f;
				}
				bUpdate = TRUE;
			}
		}

		if( bUpdate )
		{
			ALandscape* Landscape = EdMode->CurrentToolTarget.Landscape;
			FLOAT ScaleXY = Landscape->DrawScale3D.X * Landscape->DrawScale;
			BrushMaterial->SetScalarParameterValue(FName(TEXT("WorldRadius")), ScaleXY*Radius);
			BrushMaterial->SetScalarParameterValue(FName(TEXT("WorldFalloff")), ScaleXY*Falloff);
		}
#endif

		return bUpdate;
	}

	FLOAT GetBrushExtent()
	{
		ALandscape* Landscape = EdMode->CurrentToolTarget.Landscape;
		FLOAT ScaleXY = Landscape->DrawScale3D.X * Landscape->DrawScale;

		return 2.f * EdMode->UISettings.GetBrushRadius() / ScaleXY;
	}

	void ApplyBrush( TMap<QWORD, FLOAT>& OutBrush, INT& X1, INT& Y1, INT& X2, INT& Y2 )
	{
		ALandscape* Landscape = EdMode->CurrentToolTarget.Landscape;
		FLOAT ScaleXY = Landscape->DrawScale3D.X * Landscape->DrawScale;
		
		FLOAT Radius = (1.f - EdMode->UISettings.GetBrushFalloff()) * EdMode->UISettings.GetBrushRadius() / ScaleXY;
		FLOAT Falloff = EdMode->UISettings.GetBrushFalloff() * EdMode->UISettings.GetBrushRadius() / ScaleXY;

		X1 = appFloor(LastMousePosition.X - (Radius+Falloff));
		Y1 = appFloor(LastMousePosition.Y - (Radius+Falloff));
		X2 = appCeil(LastMousePosition.X + (Radius+Falloff));
		Y2 = appCeil(LastMousePosition.Y + (Radius+Falloff));

		for( INT Y=Y1;Y<=Y2;Y++ )
		{
			for( INT X=X1;X<=X2;X++ )
			{
				QWORD VertexKey = ALandscape::MakeKey(X,Y);

				// Distance from mouse
				FLOAT MouseDist = appSqrt(Square(LastMousePosition.X-(FLOAT)X) + Square(LastMousePosition.Y-(FLOAT)Y));

				FLOAT PaintAmount = CalculateFalloff(MouseDist, Radius, Falloff);

				if( PaintAmount > 0.f )
				{
					// Set the brush value for this vertex
					OutBrush.Set(VertexKey, PaintAmount);
				}
			}
		}
	}
};

class FLandscapeBrushCircle_Linear : public FLandscapeBrushCircle
{
public:
	FLandscapeBrushCircle_Linear(class FEdModeLandscape* InEdMode)
	:	FLandscapeBrushCircle(InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Linear = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("EditorLandscapeResources.CircleBrushMaterial_Linear"), NULL, LOAD_None, NULL);
		BrushMaterial->SetParent(CircleBrushMaterial_Linear);
	}


	const TCHAR* GetIconString() { return TEXT("Circle_linear"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Brush_Falloff_Linear"); };

protected:
	virtual FLOAT CalculateFalloff( FLOAT Distance, FLOAT Radius, FLOAT Falloff )
	{
		return Distance < Radius ? 1.f : 
			Falloff > 0.f ? Max<FLOAT>(0.f, 1.f - (Distance - Radius) / Falloff) : 
			0.f;
	}
};

class FLandscapeBrushCircle_Smooth : public FLandscapeBrushCircle_Linear
{
public:
	FLandscapeBrushCircle_Smooth(class FEdModeLandscape* InEdMode)
	:	FLandscapeBrushCircle_Linear(InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Smooth = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("EditorLandscapeResources.CircleBrushMaterial_Smooth"), NULL, LOAD_None, NULL);
		BrushMaterial->SetParent(CircleBrushMaterial_Smooth);
	}

	const TCHAR* GetIconString() { return TEXT("Circle_smooth"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Brush_Falloff_Smooth"); };

protected:
	virtual FLOAT CalculateFalloff( FLOAT Distance, FLOAT Radius, FLOAT Falloff )
	{
		FLOAT y = FLandscapeBrushCircle_Linear::CalculateFalloff(Distance, Radius, Falloff);
		// Smooth-step it
		return y*y*(3-2*y);
	}
};

class FLandscapeBrushCircle_Spherical : public FLandscapeBrushCircle
{
public:
	FLandscapeBrushCircle_Spherical(class FEdModeLandscape* InEdMode)
	:	FLandscapeBrushCircle(InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Spherical = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("EditorLandscapeResources.CircleBrushMaterial_Spherical"), NULL, LOAD_None, NULL);
		BrushMaterial->SetParent(CircleBrushMaterial_Spherical);
	}

	const TCHAR* GetIconString() { return TEXT("Circle_spherical"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Brush_Falloff_Spherical"); };

protected:
	virtual FLOAT CalculateFalloff( FLOAT Distance, FLOAT Radius, FLOAT Falloff )
	{
		if( Distance <= Radius )
		{
			return 1.f;
		}

		if( Distance > Radius + Falloff )
		{
			return 0.f;
		}

		// Elliptical falloff
		return appSqrt( 1.f - Square((Distance - Radius) / Falloff) );
	}
};

class FLandscapeBrushCircle_Tip : public FLandscapeBrushCircle
{
public:
	FLandscapeBrushCircle_Tip(class FEdModeLandscape* InEdMode)
	:	FLandscapeBrushCircle(InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Tip = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("EditorLandscapeResources.CircleBrushMaterial_Tip"), NULL, LOAD_None, NULL);
		BrushMaterial->SetParent(CircleBrushMaterial_Tip);
	}

	const TCHAR* GetIconString() { return TEXT("Circle_tip"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Brush_Falloff_Tip"); };

protected:
	virtual FLOAT CalculateFalloff( FLOAT Distance, FLOAT Radius, FLOAT Falloff )
	{
		if( Distance <= Radius )
		{
			return 1.f;
		}

		if( Distance > Radius + Falloff )
		{
			return 0.f;
		}

		// inverse elliptical falloff
		return 1.f - appSqrt( 1.f - Square((Falloff + Radius - Distance) / Falloff) );
	}
};


struct FHeightmapToolTarget
{
	typedef FLandscapeHeightCache CacheClass;
	enum EToolTargetType { TargetType = LET_Heightmap };

	static FLOAT StrengthMultiplier(ALandscape* Landscape)
	{
		return 10.f * Landscape->DrawScale * Landscape->DrawScale3D.X / (Landscape->DrawScale3D.Z / 128.f);
	}
};


struct FWeightmapToolTarget
{
	typedef FLandscapeAlphaCache CacheClass;
	enum EToolTargetType { TargetType = LET_Weightmap };

	static FLOAT StrengthMultiplier(ALandscape* Landscape)
	{
		return 255.f;
	}
};

// 
// FLandscapeToolPaintBase
//
template<class ToolTarget>
class FLandscapeToolPaintBase : public FLandscapeTool
{
private:
	FLOAT PrevHitX;
	FLOAT PrevHitY;
protected:
	FLOAT PaintDistance;

public:
	FLandscapeToolPaintBase(FEdModeLandscape* InEdMode)
	:	EdMode(InEdMode)
	,	bToolActive(FALSE)
	,	Landscape(NULL)
	,	Cache(NULL)
	{
	}

	virtual UBOOL IsValidForTarget(const FLandscapeToolTarget& Target)
	{
		return Target.TargetType == ToolTarget::TargetType;
	}

	virtual UBOOL BeginTool( FEditorLevelViewportClient* ViewportClient, const FLandscapeToolTarget& InTarget, FLOAT InHitX, FLOAT InHitY )
	{
		bToolActive = TRUE;

		Landscape = InTarget.Landscape;
		EdMode->CurrentBrush->BeginStroke(InHitX, InHitY, this);
		PaintDistance = 0;
		PrevHitX = InHitX;
		PrevHitY = InHitY;

		Cache = new ToolTarget::CacheClass(InTarget);

		ApplyTool(ViewportClient);

		return TRUE;
	}

	virtual void EndTool()
	{
		delete Cache;
		Landscape = NULL;
		bToolActive = FALSE;
		EdMode->CurrentBrush->EndStroke();
	}

	virtual UBOOL MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y )
	{
		FLOAT HitX, HitY;
		if( EdMode->LandscapeMouseTrace(ViewportClient, HitX, HitY)  )
		{
			PaintDistance += appSqrt(Square(PrevHitX - HitX) + Square(PrevHitY - HitY));
			PrevHitX = HitX;
			PrevHitY = HitY;

			if( EdMode->CurrentBrush )
			{
				// Move brush to current location
				EdMode->CurrentBrush->MouseMove(HitX, HitY);
			}

			if( bToolActive )
			{
				// Apply tool
				ApplyTool(ViewportClient);
			}
		}

		return TRUE;
	}	

	virtual UBOOL CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY )
	{
		return MouseMove(InViewportClient,InViewport,InMouseX,InMouseY);
	}

	virtual void ApplyTool(FEditorLevelViewportClient* ViewportClient) = 0;

protected:
	class FEdModeLandscape* EdMode;
	UBOOL bToolActive;
	class ALandscape* Landscape;

	typename ToolTarget::CacheClass* Cache;
};


// 
// FLandscapeToolPaint
//
template<class ToolTarget>
class FLandscapeToolPaint : public FLandscapeToolPaintBase<ToolTarget>
{
	struct FBrushHistoryInfo
	{
		FLOAT PaintDistance;
		typename ToolTarget::CacheClass::DataType OriginalValue;
		typename ToolTarget::CacheClass::DataType PaintAmount;
		
		FBrushHistoryInfo(FLOAT InPaintDistance, typename ToolTarget::CacheClass::DataType InOriginalValue, typename ToolTarget::CacheClass::DataType InPaintAmount)
		:	PaintDistance(InPaintDistance)
		,	OriginalValue(InOriginalValue)
		,	PaintAmount(InPaintAmount)
		{}
	};
	
	TMap<QWORD, FBrushHistoryInfo> BrushHistory;

public:
	FLandscapeToolPaint(class FEdModeLandscape* InEdMode)
		:	FLandscapeToolPaintBase(InEdMode)
	{}

	virtual const TCHAR* GetIconString() { return TEXT("Paint"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Paint"); };

	virtual void ApplyTool(FEditorLevelViewportClient* ViewportClient)
	{
		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		Cache->CacheData(X1,Y1,X2,Y2);

		TArray<ToolTarget::CacheClass::DataType> Data;
		Cache->GetCachedData(X1,Y1,X2,Y2,Data);

		// Invert when holding Shift
		UBOOL bInvert = IsShiftDown(ViewportClient->Viewport);

		// Apply the brush	
		for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
		{
			INT X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			// Value before we apply our painting
			ToolTarget::CacheClass::DataType OriginalValue = Data((X-X1) + (Y-Y1)*(1+X2-X1));
			ToolTarget::CacheClass::DataType PaintAmount = appRound( It.Value() * EdMode->UISettings.GetToolStrength() * Pressure * ToolTarget::StrengthMultiplier(Landscape) );

			FBrushHistoryInfo* History = BrushHistory.Find(It.Key());
			if( History )
			{
				FLOAT BrushExtent = EdMode->CurrentBrush->GetBrushExtent();
				FLOAT HistoryDistance = PaintDistance - History->PaintDistance;
				if( HistoryDistance < 5.f * BrushExtent )
				{
					FLOAT Alpha = Clamp<FLOAT>( (HistoryDistance - BrushExtent) / (5.f * BrushExtent), 0.f, 1.f);
					OriginalValue = Lerp( History->OriginalValue, OriginalValue, Alpha );
					PaintAmount = Max(PaintAmount, History->PaintAmount);
				}
			}

			if( bInvert )
			{
				PaintAmount = OriginalValue - ToolTarget::CacheClass::ClampValue( OriginalValue - PaintAmount );
				Data((X-X1) + (Y-Y1)*(1+X2-X1)) = OriginalValue - PaintAmount;
			}
			else
			{
				PaintAmount = ToolTarget::CacheClass::ClampValue( OriginalValue + PaintAmount ) - OriginalValue;
				Data((X-X1) + (Y-Y1)*(1+X2-X1)) = OriginalValue + PaintAmount;
			}

			// Save the original and paint amount values we used
			BrushHistory.Set(It.Key(), FBrushHistoryInfo(PaintDistance, OriginalValue, PaintAmount));
		}

		Cache->SetCachedData(X1,Y1,X2,Y2,Data);
		Cache->Flush();
	}

	virtual void EndTool()
	{
		BrushHistory.Empty();
		FLandscapeToolPaintBase::EndTool();
	}

};

#if WITH_KISSFFT
#include "tools/kiss_fftnd.h" // Kiss FFT for Real component...
#endif

template<typename DataType>
inline void LowPassFilter(INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, FLOAT>& BrushInfo, TArray<DataType>& Data, const FLOAT DetailScale, const FLOAT ApplyRatio = 1.f)
{
#if WITH_KISSFFT
	// Low-pass filter
	INT FFTWidth = X2-X1-1;
	INT FFTHeight = Y2-Y1-1;

	const int NDims = 2;
	const INT Dims[NDims] = {FFTHeight-FFTHeight%2, FFTWidth-FFTWidth%2};
	kiss_fftnd_cfg stf = kiss_fftnd_alloc(Dims, NDims, 0, NULL, NULL),
					sti = kiss_fftnd_alloc(Dims, NDims, 1, NULL, NULL);

	kiss_fft_cpx *buf = (kiss_fft_cpx *)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * Dims[0] * Dims[1]);
	kiss_fft_cpx *out = (kiss_fft_cpx *)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * Dims[0] * Dims[1]);

	for (int X = X1+1; X <= X2-1-FFTWidth%2; X++)
	{
		for (int Y = Y1+1; Y <= Y2-1-FFTHeight%2; Y++)
		{
			buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r = Data((X-X1) + (Y-Y1)*(1+X2-X1));
			buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].i = 0;
		}
	}

	// Forward FFT
	kiss_fftnd(stf, buf, out);

	INT CenterPos[2] = {Dims[0]>>1, Dims[1]>>1};
	for (int Y = 0; Y < Dims[0]; Y++)
	{
		FLOAT DistFromCenter = 0.f;
		for (int X = 0; X < Dims[1]; X++)
		{
			if (Y < CenterPos[0])
			{
				if (X < CenterPos[1])
				{
					// 1
					DistFromCenter = X*X + Y*Y;
				}
				else
				{
					// 2
					DistFromCenter = (X-Dims[1])*(X-Dims[1]) + Y*Y;
				}
			}
			else
			{
				if (X < CenterPos[1])
				{
					// 3
					DistFromCenter = X*X + (Y-Dims[0])*(Y-Dims[0]);
				}
				else
				{
					// 4
					DistFromCenter = (X-Dims[1])*(X-Dims[1]) + (Y-Dims[0])*(Y-Dims[0]);
				}
			}
			// High frequency removal
			FLOAT Ratio = 1.f - DetailScale;
			FLOAT Dist = Min<FLOAT>((Dims[0]*Ratio)*(Dims[0]*Ratio), (Dims[1]*Ratio)*(Dims[1]*Ratio));
			FLOAT Filter = 1.0 / (1.0 + DistFromCenter/Dist);
			out[X+Y*Dims[1]].r *= Filter;
			out[X+Y*Dims[1]].i *= Filter;
		}
	}

	// Inverse FFT
	kiss_fftnd(sti, out, buf);

	FLOAT Scale = Dims[0] * Dims[1];
	for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
	{
		INT X, Y;
		ALandscape::UnpackKey(It.Key(), X, Y);

		if (It.Value() > 0.f)
		{
			Data((X-X1) + (Y-Y1)*(1+X2-X1)) = Lerp((FLOAT)Data((X-X1) + (Y-Y1)*(1+X2-X1)), buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r / Scale, It.Value() * ApplyRatio);
				//buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r / Scale;
		}
	}

	// Free FFT allocation
	KISS_FFT_FREE(stf);
	KISS_FFT_FREE(sti);
	KISS_FFT_FREE(buf);
	KISS_FFT_FREE(out);
#endif
}


// 
// FLandscapeToolSmooth
//
template<class ToolTarget>
class FLandscapeToolSmooth : public FLandscapeToolPaintBase<ToolTarget>
{
public:
	FLandscapeToolSmooth(class FEdModeLandscape* InEdMode)
		:	FLandscapeToolPaintBase(InEdMode)
	{}

	virtual ELandscapeToolUIType::Type GetUIType() { return ELandscapeToolUIType::Smooth; }

	virtual const TCHAR* GetIconString() { return TEXT("Smooth"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Smooth"); };

	virtual void ApplyTool(FEditorLevelViewportClient* ViewportClient)
	{
		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		Cache->CacheData(X1,Y1,X2,Y2);

		TArray<ToolTarget::CacheClass::DataType> Data;
		Cache->GetCachedData(X1,Y1,X2,Y2,Data);

		// Apply the brush
		if (EdMode->UISettings.GetbDetailSmooth())
		{
			LowPassFilter<ToolTarget::CacheClass::DataType>(X1, Y1, X2, Y2, BrushInfo, Data, EdMode->UISettings.GetDetailScale(), EdMode->UISettings.GetToolStrength() * Pressure);
		}
		else
		{
			for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
			{
				INT X, Y;
				ALandscape::UnpackKey(It.Key(), X, Y);

				if( It.Value() > 0.f )
				{
					// 3x3 filter
					INT FilterValue = 0;
					for( INT y=Y-1;y<=Y+1;y++ )
					{
						for( INT x=X-1;x<=X+1;x++ )
						{
							FilterValue += Data((x-X1) + (y-Y1)*(1+X2-X1));
						}
					}
					FilterValue /= 9;

					INT HeightDataIndex = (X-X1) + (Y-Y1)*(1+X2-X1);
					Data(HeightDataIndex) = Lerp( Data(HeightDataIndex), (ToolTarget::CacheClass::DataType)FilterValue, It.Value() * EdMode->UISettings.GetToolStrength() * Pressure );
				}	
			}
		}

		Cache->SetCachedData(X1,Y1,X2,Y2,Data);
		Cache->Flush();
	}
};

//
// FLandscapeToolFlatten
//
template<class ToolTarget>
class FLandscapeToolFlatten : public FLandscapeToolPaintBase<ToolTarget>
{
	UBOOL bInitializedFlattenHeight;
	INT FlattenHeightX;
	INT FlattenHeightY;
	typename ToolTarget::CacheClass::DataType FlattenHeight;

public:
	FLandscapeToolFlatten(class FEdModeLandscape* InEdMode)
	:	FLandscapeToolPaintBase(InEdMode)
	,	bInitializedFlattenHeight(FALSE)
	{}

	virtual const TCHAR* GetIconString() { return TEXT("Flatten"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Flatten"); };

	virtual UBOOL BeginTool( FEditorLevelViewportClient* ViewportClient, const FLandscapeToolTarget& InTarget, FLOAT InHitX, FLOAT InHitY )
	{
		bInitializedFlattenHeight = FALSE;
		FlattenHeightX = InHitX;
		FlattenHeightY = InHitY;
		return FLandscapeToolPaintBase::BeginTool(ViewportClient, InTarget, InHitX, InHitY);
	}

	virtual void EndTool()
	{
		bInitializedFlattenHeight = FALSE;
		FLandscapeToolPaintBase::EndTool();
	}

	virtual void ApplyTool(FEditorLevelViewportClient* ViewportClient)
	{
		if( !bInitializedFlattenHeight && ToolTarget::TargetType != LET_Weightmap)
		{
			Cache->CacheData(FlattenHeightX,FlattenHeightY,FlattenHeightX,FlattenHeightY);
			ToolTarget::CacheClass::DataType* FlattenHeightPtr = Cache->GetValueRef(FlattenHeightX,FlattenHeightY);
			check(FlattenHeightPtr);
			FlattenHeight = *FlattenHeightPtr;
			bInitializedFlattenHeight = TRUE;
		}

		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		Cache->CacheData(X1,Y1,X2,Y2);

		TArray<ToolTarget::CacheClass::DataType> HeightData;
		Cache->GetCachedData(X1,Y1,X2,Y2,HeightData);

		// Apply the brush
		for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
		{
			INT X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			if( It.Value() > 0.f )
			{
				INT HeightDataIndex = (X-X1) + (Y-Y1)*(1+X2-X1);
				if( ToolTarget::TargetType == LET_Weightmap )
				{
					// Weightmap painting always flattens to solid color, so the strength slider specifies the flatten amount.

					HeightData(HeightDataIndex) = 255.f * EdMode->UISettings.GetToolStrength() * Pressure * It.Value();
				}
				else
				{
					HeightData(HeightDataIndex) = Lerp( HeightData(HeightDataIndex), FlattenHeight, It.Value() * EdMode->UISettings.GetToolStrength() * Pressure );
				}
			}	
		}

		Cache->SetCachedData(X1,Y1,X2,Y2,HeightData);
		Cache->Flush();
	}
};

//
// FLandscapeToolErosion
//
class FLandscapeToolErosion : public FLandscapeTool
{
public:
	FLandscapeToolErosion(class FEdModeLandscape* InEdMode)
		:EdMode(InEdMode)
		,	bToolActive(FALSE)
		,	Landscape(NULL)
		,	HeightCache(NULL)
		,	WeightCache(NULL)
		,	bWeightApplied(FALSE)
	{}

	virtual ELandscapeToolUIType::Type GetUIType() { return ELandscapeToolUIType::Erosion; }

	virtual const TCHAR* GetIconString() { return TEXT("Erosion"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Erosion"); };

	virtual UBOOL IsValidForTarget(const FLandscapeToolTarget& Target)
	{
		return TRUE; // erosion applied to all...
	}

	virtual UBOOL BeginTool( FEditorLevelViewportClient* ViewportClient, const FLandscapeToolTarget& InTarget, FLOAT InHitX, FLOAT InHitY )
	{
		bToolActive = TRUE;

		Landscape = InTarget.Landscape;
		EdMode->CurrentBrush->BeginStroke(InHitX, InHitY, this);

		HeightCache = new FLandscapeHeightCache(InTarget);
		WeightCache = new FLandscapeFullWeightCache(InTarget);

		bWeightApplied = InTarget.TargetType != LET_Heightmap;

		ApplyTool(ViewportClient);

		return TRUE;
	}

	virtual void EndTool()
	{
		delete HeightCache;
		delete WeightCache;
		Landscape = NULL;
		bToolActive = FALSE;
		EdMode->CurrentBrush->EndStroke();
	}

	virtual UBOOL MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y )
	{
		FLOAT HitX, HitY;
		if( EdMode->LandscapeMouseTrace(ViewportClient, HitX, HitY)  )
		{
			if( EdMode->CurrentBrush )
			{
				// Move brush to current location
				EdMode->CurrentBrush->MouseMove(HitX, HitY);
			}

			if( bToolActive )
			{
				// Apply tool
				ApplyTool(ViewportClient);
			}
		}

		return TRUE;
	}	

	virtual UBOOL CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY )
	{
		return MouseMove(InViewportClient,InViewport,InMouseX,InMouseY);
	}

	virtual void ApplyTool( FEditorLevelViewportClient* ViewportClient )
	{
		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		const INT NeighborNum = 4;
		const INT Iteration = EdMode->UISettings.GetErodeIterationNum();
		const INT Thickness = EdMode->UISettings.GetErodeSurfaceThickness();
		const INT LayerNum = Landscape->LayerInfos.Num();

		HeightCache->CacheData(X1,Y1,X2,Y2);
		TArray<WORD> HeightData;
		HeightCache->GetCachedData(X1,Y1,X2,Y2,HeightData);

		TArray<BYTE> WeightDatas; // Weight*Layers...
		WeightCache->CacheData(X1,Y1,X2,Y2);
		WeightCache->GetCachedData(X1,Y1,X2,Y2, WeightDatas, LayerNum);	

		// Invert when holding Shift
		UBOOL bInvert = IsShiftDown(ViewportClient->Viewport);

		// Apply the brush	
		WORD Thresh = EdMode->UISettings.GetErodeThresh();
		INT WeightMoveThresh = Min<INT>(Max<INT>(Thickness >> 2, Thresh), Thickness >> 1);

		DWORD SlopeTotal;
		WORD SlopeMax;
		FLOAT TotalHeightDiff;
		FLOAT TotalWeight;

		TArray<FLOAT> CenterWeight;
		CenterWeight.Empty(LayerNum);
		CenterWeight.Add(LayerNum);
		TArray<FLOAT> NeighborWeight;
		NeighborWeight.Empty(NeighborNum*LayerNum);
		NeighborWeight.Add(NeighborNum*LayerNum);

		UBOOL bHasChanged = FALSE;
		for (INT i = 0; i < Iteration; i++)
		{
			bHasChanged = FALSE;
			for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
			{
				INT X, Y;
				ALandscape::UnpackKey(It.Key(), X, Y);

				if( It.Value() > 0.f )
				{
					INT Center = (X-X1) + (Y-Y1)*(1+X2-X1);
					INT Neighbor[NeighborNum] = {(X-1-X1) + (Y-Y1)*(1+X2-X1), (X+1-X1) + (Y-Y1)*(1+X2-X1), (X-X1) + (Y-1-Y1)*(1+X2-X1), (X-X1) + (Y+1-Y1)*(1+X2-X1)};
/*
					INT Neighbor[NeighborNum] = {
													(X-1-X1) + (Y-Y1)*(1+X2-X1), (X+1-X1) + (Y-Y1)*(1+X2-X1), (X-X1) + (Y-1-Y1)*(1+X2-X1), (X-X1) + (Y+1-Y1)*(1+X2-X1)
													,(X-1-X1) + (Y-1-Y1)*(1+X2-X1), (X+1-X1) + (Y+1-Y1)*(1+X2-X1), (X+1-X1) + (Y-1-Y1)*(1+X2-X1), (X-1-X1) + (Y+1-Y1)*(1+X2-X1)
												};
*/
					SlopeTotal = 0;
					SlopeMax = bInvert ? 0 : Thresh;

					for (INT Idx = 0; Idx < NeighborNum; Idx++)
					{
						if (HeightData(Center) > HeightData(Neighbor[Idx]))
						{
							WORD Slope = HeightData(Center) - HeightData(Neighbor[Idx]);
							if (bInvert ^ (Slope*It.Value() > Thresh))
							{
								SlopeTotal += Slope;
								if (SlopeMax < Slope)
								{
									SlopeMax = Slope;
								}
							}
						}
					}

					if (SlopeTotal > 0)
					{
						FLOAT Softness = 1.f;
						for (INT Idx = 0; Idx < LayerNum; Idx++)
						{
							BYTE Weight = WeightDatas(Center*LayerNum + Idx);
							Softness -= (FLOAT)(Weight) / 255.f * Landscape->LayerInfos(Idx).Hardness;
						}
						if (Softness > 0.f)
						{
							//Softness = Clamp<FLOAT>(Softness, 0.f, 1.f);
							TotalHeightDiff = 0;
							INT WeightTransfer = Min<INT>(WeightMoveThresh, (bInvert ? (Thresh - SlopeMax) : (SlopeMax - Thresh)));
							for (INT Idx = 0; Idx < NeighborNum; Idx++)
							{
								TotalWeight = 0.f;
								if (HeightData(Center) > HeightData(Neighbor[Idx]))
								{
									WORD Slope = HeightData(Center) - HeightData(Neighbor[Idx]);
									if (bInvert ^ (Slope > Thresh))
									{
										FLOAT WeightDiff = Softness * EdMode->UISettings.GetToolStrength() * Pressure * ((FLOAT)Slope / SlopeTotal) * It.Value();
										//WORD HeightDiff = (WORD)((SlopeMax - Thresh) * WeightDiff);
										FLOAT HeightDiff = ((bInvert ? (Thresh - SlopeMax) : (SlopeMax - Thresh)) * WeightDiff);
										HeightData(Neighbor[Idx]) += HeightDiff;
										TotalHeightDiff += HeightDiff;

										if (bWeightApplied)
										{
											for (INT LayerIdx = 0; LayerIdx < LayerNum; LayerIdx++)
											{
												FLOAT CenterWeight = (FLOAT)(WeightDatas(Center*LayerNum + LayerIdx)) / 255.f;
												FLOAT Weight = (FLOAT)(WeightDatas(Neighbor[Idx]*LayerNum + LayerIdx)) / 255.f;
												NeighborWeight(Idx*LayerNum + LayerIdx) = Weight*(FLOAT)Thickness + CenterWeight*WeightDiff*WeightTransfer; // transferred + original...
												TotalWeight += NeighborWeight(Idx*LayerNum + LayerIdx);
											}
											// Need to normalize weight...
											for (INT LayerIdx = 0; LayerIdx < LayerNum; LayerIdx++)
											{
												WeightDatas(Neighbor[Idx]*LayerNum + LayerIdx) = (BYTE)(255.f * NeighborWeight(Idx*LayerNum + LayerIdx) / TotalWeight);
											}
										}
									}
								}
							}

							HeightData(Center) -= TotalHeightDiff;

							if (bWeightApplied)
							{
								TotalWeight = 0.f;
								FLOAT WeightDiff = Softness * EdMode->UISettings.GetToolStrength() * Pressure * It.Value();

								for (INT LayerIdx = 0; LayerIdx < LayerNum; LayerIdx++)
								{
									FLOAT Weight = (FLOAT)(WeightDatas(Center*LayerNum + LayerIdx)) / 255.f;
									CenterWeight(LayerIdx) = Weight*Thickness - Weight*WeightDiff*WeightTransfer;
									TotalWeight += CenterWeight(LayerIdx);
								}
								// Need to normalize weight...
								for (INT LayerIdx = 0; LayerIdx < LayerNum; LayerIdx++)
								{
									WeightDatas(Center*LayerNum + LayerIdx) = (BYTE)(255.f * CenterWeight(LayerIdx) / TotalWeight);
								}
							}

							bHasChanged = TRUE;
						} // if Softness > 0.f
					} // if SlopeTotal > 0
				}
			}
			if (!bHasChanged)
			{
				break;
			}
		}

		// Make some noise...
		for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
		{
			INT X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			if( It.Value() > 0.f )
			{
				FNoiseParameter NoiseParam(0, EdMode->UISettings.GetErosionNoiseScale(), It.Value() * Thresh);
				FLOAT PaintAmount = ELandscapeToolNoiseMode::Conversion(EdMode->UISettings.GetErosionNoiseMode(), NoiseParam.NoiseAmount, NoiseParam.Sample(X, Y));
				HeightData((X-X1) + (Y-Y1)*(1+X2-X1)) += PaintAmount;
			}
		}

		HeightCache->SetCachedData(X1,Y1,X2,Y2,HeightData);
		HeightCache->Flush();
		if (bWeightApplied)
		{
			WeightCache->SetCachedData(X1,Y1,X2,Y2,WeightDatas, LayerNum);
			WeightCache->Flush();
		}
	}

protected:
	class FEdModeLandscape* EdMode;
	class ALandscape* Landscape;

	FLandscapeHeightCache* HeightCache;
	FLandscapeFullWeightCache* WeightCache;

	UBOOL bToolActive;
	UBOOL bWeightApplied;
};

//
// FLandscapeToolHydraErosion
//
class FLandscapeToolHydraErosion : public FLandscapeToolErosion
{
public:
	FLandscapeToolHydraErosion(class FEdModeLandscape* InEdMode)
		: FLandscapeToolErosion(InEdMode)
	{}

	virtual ELandscapeToolUIType::Type GetUIType() { return ELandscapeToolUIType::HydraErosion; }

	virtual const TCHAR* GetIconString() { return TEXT("HydraulicErosion"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_HydraErosion"); };

	virtual void ApplyTool( FEditorLevelViewportClient* ViewportClient )
	{
		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		const INT NeighborNum = 8;
		const INT LayerNum = Landscape->LayerInfos.Num();

		const INT Iteration = EdMode->UISettings.GetHErodeIterationNum();
		const WORD RainAmount = EdMode->UISettings.GetRainAmount();
		const FLOAT DissolvingRatio = 0.07 * EdMode->UISettings.GetToolStrength() * Pressure;  //0.01;
		const FLOAT EvaporateRatio = 0.5;
		const FLOAT SedimentCapacity = 0.10 * EdMode->UISettings.GetSedimentCapacity(); //DissolvingRatio; //0.01;

		HeightCache->CacheData(X1,Y1,X2,Y2);
		TArray<WORD> HeightData;
		HeightCache->GetCachedData(X1,Y1,X2,Y2,HeightData);
/*
		TArray<BYTE> WeightDatas; // Weight*Layers...
		WeightCache->CacheData(X1,Y1,X2,Y2);
		WeightCache->GetCachedData(X1,Y1,X2,Y2, WeightDatas, LayerNum);	
*/
		// Invert when holding Shift
		UBOOL bInvert = IsShiftDown(ViewportClient->Viewport);

		// Apply the brush
		TArray<WORD> WaterData;
		WaterData.Empty((1+X2-X1)*(1+Y2-Y1));
		WaterData.AddZeroed((1+X2-X1)*(1+Y2-Y1));
		TArray<WORD> SedimentData;
		SedimentData.Empty((1+X2-X1)*(1+Y2-Y1));
		SedimentData.AddZeroed((1+X2-X1)*(1+Y2-Y1));

		UBOOL bWaterExist = TRUE;
		UINT TotalHeightDiff;
		UINT TotalAltitudeDiff;
		UINT AltitudeDiff[NeighborNum];
		UINT TotalWaterDiff;
		UINT WaterDiff[NeighborNum];
		UINT TotalSedimentDiff;
		FLOAT AverageAltitude;

		// It's raining men!
		// Only initial raining works better...
		FNoiseParameter NoiseParam(0, EdMode->UISettings.GetRainDistScale(), RainAmount);
		for (int X = X1; X < X2; X++)
		{
			for (int Y = Y1; Y < Y2; Y++)
			{
				FLOAT PaintAmount = ELandscapeToolNoiseMode::Conversion(EdMode->UISettings.GetRainDistMode(), NoiseParam.NoiseAmount, NoiseParam.Sample(X, Y));
				if (PaintAmount > 0) // Raining only for positive region...
					WaterData((X-X1) + (Y-Y1)*(1+X2-X1)) += PaintAmount;
			}
		}

		for (INT i = 0; i < Iteration; i++)
		{
			bWaterExist = FALSE;
			for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
			{
				INT X, Y;
				ALandscape::UnpackKey(It.Key(), X, Y);

				if( It.Value() > 0.f)
				{
					INT Center = (X-X1) + (Y-Y1)*(1+X2-X1);
					//INT Neighbor[NeighborNum] = {(X-1-X1) + (Y-Y1)*(1+X2-X1), (X+1-X1) + (Y-Y1)*(1+X2-X1), (X-X1) + (Y-1-Y1)*(1+X2-X1), (X-X1) + (Y+1-Y1)*(1+X2-X1)};

					INT Neighbor[NeighborNum] = {
						(X-1-X1) + (Y-Y1)*(1+X2-X1), (X+1-X1) + (Y-Y1)*(1+X2-X1), (X-X1) + (Y-1-Y1)*(1+X2-X1), (X-X1) + (Y+1-Y1)*(1+X2-X1)
						,(X-1-X1) + (Y-1-Y1)*(1+X2-X1), (X+1-X1) + (Y+1-Y1)*(1+X2-X1), (X+1-X1) + (Y-1-Y1)*(1+X2-X1), (X-1-X1) + (Y+1-Y1)*(1+X2-X1)
					};

					// Dissolving...				
					FLOAT DissolvedAmount = DissolvingRatio * WaterData(Center) * It.Value();
					if (DissolvedAmount > 0 && HeightData(Center) >= DissolvedAmount)
					{
						HeightData(Center) -= DissolvedAmount;
						SedimentData(Center) += DissolvedAmount;
					}

					TotalHeightDiff = 0;
					TotalAltitudeDiff = 0;
					TotalWaterDiff = 0;
					TotalSedimentDiff = 0;

					UINT Altitude = HeightData(Center)+WaterData(Center);
					AverageAltitude = 0;
					UINT LowerNeighbor = 0;
					for (INT Idx = 0; Idx < NeighborNum; Idx++)
					{
						UINT NeighborAltitude = HeightData(Neighbor[Idx])+WaterData(Neighbor[Idx]);
						if (Altitude > NeighborAltitude)
						{
							AltitudeDiff[Idx] = Altitude - NeighborAltitude;
							TotalAltitudeDiff += AltitudeDiff[Idx];
							LowerNeighbor++;
							AverageAltitude += NeighborAltitude;
							if (HeightData(Center) > HeightData(Neighbor[Idx]))
								TotalHeightDiff += HeightData(Center) - HeightData(Neighbor[Idx]);
						}
						else
						{
							AltitudeDiff[Idx] = 0;
						}
					}

					// Water Transfer
					if (LowerNeighbor > 0)
					{
						AverageAltitude /= (LowerNeighbor);
						// This is not mathematically correct, but makes good result
						if (TotalHeightDiff)
							AverageAltitude *= (1.f - 0.1 * EdMode->UISettings.GetToolStrength() * Pressure);
							//AverageAltitude -= 4000.f * EdMode->UISettings.GetToolStrength();

						UINT WaterTransfer = Min<UINT>(WaterData(Center), Altitude - (UINT)AverageAltitude) * It.Value();

						for (INT Idx = 0; Idx < NeighborNum; Idx++)
						{
							if (AltitudeDiff[Idx] > 0)
							{
								WaterDiff[Idx] = (UINT)(WaterTransfer * (FLOAT)AltitudeDiff[Idx] / TotalAltitudeDiff);
								WaterData(Neighbor[Idx]) += WaterDiff[Idx];
								TotalWaterDiff += WaterDiff[Idx];
								UINT SedimentDiff = (UINT)(SedimentData(Center) * (FLOAT)WaterDiff[Idx] / WaterData(Center));
								SedimentData(Neighbor[Idx]) += SedimentDiff;
								TotalSedimentDiff += SedimentDiff;
							}
						}

						WaterData(Center) -= TotalWaterDiff;
						SedimentData(Center) -= TotalSedimentDiff;
					}

					// evaporation
					if (WaterData(Center) > 0)
					{
						bWaterExist = TRUE;
						WaterData(Center) = (WORD)(WaterData(Center) * (1.f - EvaporateRatio));
						FLOAT SedimentCap = SedimentCapacity*WaterData(Center);
						FLOAT SedimentDiff = SedimentData(Center) - SedimentCap;
						if (SedimentDiff > 0)
						{
							SedimentData(Center) -= SedimentDiff;
							HeightData(Center) = Clamp<WORD>(HeightData(Center)+SedimentDiff, 0, 65535);
						}
					}
				}	
			}

			if (!bWaterExist) 
			{
				break;
			}
		}

		if (EdMode->UISettings.GetbHErosionDetailSmooth())
			LowPassFilter<WORD>(X1, Y1, X2, Y2, BrushInfo, HeightData, EdMode->UISettings.GetHErosionDetailScale(), 1.f);

		HeightCache->SetCachedData(X1,Y1,X2,Y2,HeightData);
		HeightCache->Flush();
		/*
		if (bWeightApplied)
		{
			WeightCache->SetCachedData(X1,Y1,X2,Y2,WeightDatas, LayerNum);
			WeightCache->Flush();
		}
		*/
	}
};

// 
// FLandscapeToolNoise
//
template<class ToolTarget>
class FLandscapeToolNoise : public FLandscapeToolPaintBase<ToolTarget>
{
public:
	FLandscapeToolNoise(class FEdModeLandscape* InEdMode)
		:	FLandscapeToolPaintBase(InEdMode)
	{}

	virtual ELandscapeToolUIType::Type GetUIType() { return ELandscapeToolUIType::Noise; }

	virtual const TCHAR* GetIconString() { return TEXT("Noise"); }
	virtual FString GetTooltipString() { return LocalizeUnrealEd("LandscapeMode_Noise"); };

	virtual void ApplyTool(FEditorLevelViewportClient* ViewportClient)
	{
		// Get list of verts to update
		TMap<QWORD, FLOAT> BrushInfo;
		INT X1, Y1, X2, Y2;
		EdMode->CurrentBrush->ApplyBrush(BrushInfo, X1, Y1, X2, Y2);

		// Tablet pressure
		FLOAT Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		Cache->CacheData(X1,Y1,X2,Y2);
		TArray<ToolTarget::CacheClass::DataType> Data;
		Cache->GetCachedData(X1,Y1,X2,Y2,Data);

		// Apply the brush
		for( TMap<QWORD, FLOAT>::TIterator It(BrushInfo); It; ++It )
		{
			INT X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			if( It.Value() > 0.f )
			{
				FLOAT OriginalValue = Data((X-X1) + (Y-Y1)*(1+X2-X1));
				FNoiseParameter NoiseParam(0, EdMode->UISettings.GetNoiseScale(), It.Value() * EdMode->UISettings.GetToolStrength() * Pressure * ToolTarget::StrengthMultiplier(Landscape));
				FLOAT PaintAmount = ELandscapeToolNoiseMode::Conversion(EdMode->UISettings.GetNoiseMode(), NoiseParam.NoiseAmount, NoiseParam.Sample(X, Y)); //NoiseParam.Sample(X, Y);
				Data((X-X1) + (Y-Y1)*(1+X2-X1)) = ToolTarget::CacheClass::ClampValue(OriginalValue + PaintAmount);
			}	
		}

		Cache->SetCachedData(X1,Y1,X2,Y2,Data);
		Cache->Flush();
	}
};

//
// FEdModeLandscape
//

/** Constructor */
FEdModeLandscape::FEdModeLandscape() 
:	FEdMode()
,	bToolActive(FALSE)
{
	ID = EM_Landscape;
	Desc = TEXT( "Landscape" );

	// Initialize tools.
	FLandscapeToolSet* ToolSet_Paint = new FLandscapeToolSet;
	ToolSet_Paint->AddTool(new FLandscapeToolPaint<FHeightmapToolTarget>(this));
	ToolSet_Paint->AddTool(new FLandscapeToolPaint<FWeightmapToolTarget>(this));
	LandscapeToolSets.AddItem(ToolSet_Paint);

	FLandscapeToolSet* ToolSet_Smooth = new FLandscapeToolSet;
	ToolSet_Smooth->AddTool(new FLandscapeToolSmooth<FHeightmapToolTarget>(this));
	ToolSet_Smooth->AddTool(new FLandscapeToolSmooth<FWeightmapToolTarget>(this));
	LandscapeToolSets.AddItem(ToolSet_Smooth);

	FLandscapeToolSet* ToolSet_Flatten = new FLandscapeToolSet;
	ToolSet_Flatten->AddTool(new FLandscapeToolFlatten<FHeightmapToolTarget>(this));
	ToolSet_Flatten->AddTool(new FLandscapeToolFlatten<FWeightmapToolTarget>(this));
	LandscapeToolSets.AddItem(ToolSet_Flatten);

	FLandscapeToolSet* ToolSet_Erosion = new FLandscapeToolSet;
	ToolSet_Erosion->AddTool(new FLandscapeToolErosion(this));
	LandscapeToolSets.AddItem(ToolSet_Erosion);

	FLandscapeToolSet* ToolSet_HydraErosion = new FLandscapeToolSet;
	ToolSet_HydraErosion->AddTool(new FLandscapeToolHydraErosion(this));
	LandscapeToolSets.AddItem(ToolSet_HydraErosion);

	FLandscapeToolSet* ToolSet_Noise = new FLandscapeToolSet;
	ToolSet_Noise->AddTool(new FLandscapeToolNoise<FHeightmapToolTarget>(this));
	ToolSet_Noise->AddTool(new FLandscapeToolNoise<FWeightmapToolTarget>(this));
	LandscapeToolSets.AddItem(ToolSet_Noise);

	CurrentToolSet = NULL;
	ToolUIType = ELandscapeToolUIType::Default;

	// Initialize brushes

	FLandscapeBrushSet* BrushSet; 
	BrushSet = new(LandscapeBrushSets) FLandscapeBrushSet(LocalizeUnrealEd("LandscapeMode_Brush_Circle"));
	BrushSet->Brushes.AddItem(new FLandscapeBrushCircle_Smooth(this));
	BrushSet->Brushes.AddItem(new FLandscapeBrushCircle_Linear(this));
	BrushSet->Brushes.AddItem(new FLandscapeBrushCircle_Spherical(this));
	BrushSet->Brushes.AddItem(new FLandscapeBrushCircle_Tip(this));

	CurrentBrush = LandscapeBrushSets(0).Brushes(0);

	CurrentToolTarget.Landscape = NULL;
	CurrentToolTarget.TargetType = LET_Heightmap;
	CurrentToolTarget.LayerName = NAME_None;
}


/** Destructor */
FEdModeLandscape::~FEdModeLandscape()
{
	// Save UI settings to config file
	UISettings.Save();

	delete CurrentBrush;

	// Destroy tools.
	for( INT ToolIdx=0;ToolIdx<LandscapeToolSets.Num();ToolIdx++ )
	{
		delete LandscapeToolSets(ToolIdx);
	}
	LandscapeToolSets.Empty();

	// Destroy brushes
	for( INT BrushSetIdx=0;BrushSetIdx<LandscapeBrushSets.Num();BrushSetIdx++ )
	{
		FLandscapeBrushSet& BrushSet = LandscapeBrushSets(BrushSetIdx);

		for( INT BrushIdx=0;BrushIdx < BrushSet.Brushes.Num();BrushIdx++ )
		{
			delete BrushSet.Brushes(BrushIdx);
		}
	}
	LandscapeBrushSets.Empty();
}



/** FSerializableObject: Serializer */
void FEdModeLandscape::Serialize( FArchive &Ar )
{
	// Call parent implementation
	FEdMode::Serialize( Ar );
}

/** FEdMode: Called when the mode is entered */
void FEdModeLandscape::Enter()
{
	// Call parent implementation
	FEdMode::Enter();

	// Load UI settings from config file
	UISettings.Load();

#if WITH_MANAGED_CODE
	// Create the mesh paint window
	HWND EditorFrameWindowHandle = (HWND)GApp->EditorFrame->GetHandle();
	LandscapeEditWindow.Reset( FLandscapeEditWindow::CreateLandscapeEditWindow( this, EditorFrameWindowHandle ) );
	check( LandscapeEditWindow.IsValid() );
#endif

	// Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
	// user exits this mode.
	const UBOOL bWantRealTime = TRUE;
	const UBOOL bRememberCurrentState = TRUE;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	CurrentBrush->EnterBrush();

	SetCurrentTool(0);
}


/** FEdMode: Called when the mode is exited */
void FEdModeLandscape::Exit()
{
	// Restore real-time viewport state if we changed it
	const UBOOL bWantRealTime = FALSE;
	const UBOOL bRememberCurrentState = FALSE;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	// Save any settings that may have changed
#if WITH_MANAGED_CODE
	if( LandscapeEditWindow.IsValid() )
	{
		LandscapeEditWindow->SaveWindowSettings();
	}

	// Kill the mesh paint window
	LandscapeEditWindow.Reset();
#endif

	CurrentBrush->LeaveBrush();

	// Save UI settings to config file
	UISettings.Save();
	GLandscapeViewMode = ELandscapeViewMode::Normal;

	// Call parent implementation
	FEdMode::Exit();
}

/** FEdMode: Called once per frame */
void FEdModeLandscape::Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime)
{
	FEdMode::Tick(ViewportClient,DeltaTime);

	if( CurrentToolSet && CurrentToolSet->GetTool() )
	{
		CurrentToolSet->GetTool()->Tick(ViewportClient,DeltaTime);
	}
	if( CurrentBrush )
	{
		CurrentBrush->Tick(ViewportClient,DeltaTime);
	}
}


/** FEdMode: Called when the mouse is moved over the viewport */
UBOOL FEdModeLandscape::MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT MouseX, INT MouseY )
{
	UBOOL Result = FALSE;
	if( CurrentToolSet && CurrentToolSet->GetTool() )
	{
		Result = CurrentToolSet && CurrentToolSet->GetTool()->MouseMove(ViewportClient, Viewport, MouseX, MouseY);
		ViewportClient->Invalidate( FALSE, FALSE );
	}
	return Result;
}

/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	TRUE if input was handled
 */
UBOOL FEdModeLandscape::CapturedMouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT MouseX, INT MouseY )
{
	UBOOL Result = FALSE;
	if( CurrentToolSet && CurrentToolSet->GetTool() )
	{
		Result = CurrentToolSet && CurrentToolSet->GetTool()->CapturedMouseMove(ViewportClient, Viewport, MouseX, MouseY);
		ViewportClient->Invalidate( FALSE, FALSE );
	}
	return Result;
}


/** FEdMode: Called when a mouse button is pressed */
UBOOL FEdModeLandscape::StartTracking()
{
	return TRUE;
}



/** FEdMode: Called when the a mouse button is released */
UBOOL FEdModeLandscape::EndTracking()
{
	return TRUE;
}

/** Trace under the mouse cursor and return the landscape hit and the hit location (in landscape quad space) */
UBOOL FEdModeLandscape::LandscapeMouseTrace( FEditorLevelViewportClient* ViewportClient, FLOAT& OutHitX, FLOAT& OutHitY )
{
	INT MouseX = ViewportClient->Viewport->GetMouseX();
	INT MouseY = ViewportClient->Viewport->GetMouseY();

	// Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily(
		ViewportClient->Viewport, ViewportClient->GetScene(),
		ViewportClient->ShowFlags,
		GWorld->GetTimeSeconds(),
		GWorld->GetDeltaSeconds(),
		GWorld->GetRealTimeSeconds(),
		ViewportClient->IsRealtime()
		);
	FSceneView* View = ViewportClient->CalcSceneView( &ViewFamily );
	FViewportCursorLocation MouseViewportRay( View, ViewportClient, MouseX, MouseY );

	FVector Start = MouseViewportRay.GetOrigin();
	FVector End = Start + WORLD_MAX * MouseViewportRay.GetDirection();

	FMemMark		Mark(GMainThreadMemStack);
	FCheckResult*	FirstHit	= NULL;
	DWORD			TraceFlags	= TRACE_Terrain|TRACE_TerrainIgnoreHoles;

	FirstHit	= GWorld->MultiLineCheck(GMainThreadMemStack, End, Start, FVector(0.f,0.f,0.f), TraceFlags, NULL);
	for( FCheckResult* Test = FirstHit; Test; Test = Test->GetNext() )
	{
		ULandscapeHeightfieldCollisionComponent* CollisionComponent = Cast<ULandscapeHeightfieldCollisionComponent>(Test->Component);
		if( CollisionComponent )
		{
			ALandscape* HitLandscape = CastChecked<ALandscape>(CollisionComponent->GetOuter());		

			if( HitLandscape && HitLandscape == CurrentToolTarget.Landscape )
			{
				FVector DrawScale = HitLandscape->DrawScale3D * HitLandscape->DrawScale;

				OutHitX = (Test->Location.X-HitLandscape->Location.X) / DrawScale.X;
				OutHitY = (Test->Location.Y-HitLandscape->Location.Y) / DrawScale.X;

				Mark.Pop();

				return TRUE;
			}
		}
	}

	Mark.Pop();
	return FALSE;
}


/** FEdMode: Called when a key is pressed */
UBOOL FEdModeLandscape::InputKey( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, FName Key, EInputEvent Event )
{
	if( Key == KEY_LeftMouseButton )
	{
		if( Event == IE_Pressed && (IsCtrlDown(Viewport) || (Viewport->IsPenActive() && Viewport->GetTabletPressure() > 0.f)) )
		{
			if( CurrentToolSet )
			{				
				if( CurrentToolSet->SetToolForTarget(CurrentToolTarget) )
				{
					FLOAT HitX, HitY;
					if( LandscapeMouseTrace(ViewportClient, HitX, HitY) )
					{
						bToolActive = CurrentToolSet->GetTool()->BeginTool(ViewportClient, CurrentToolTarget, HitX, HitY);
					}
				}
			}
			return TRUE;
		}
	}

	if( Key == KEY_LeftMouseButton || Key==KEY_LeftControl || Key==KEY_RightControl )
	{
		if( Event == IE_Released && CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
		{
			CurrentToolSet->GetTool()->EndTool();
			bToolActive = FALSE;
			return TRUE;
		}
	}

	// Prev tool 
	if( Event == IE_Pressed && Key == KEY_Comma )
	{
		if( CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
		{
			CurrentToolSet->GetTool()->EndTool();
			bToolActive = FALSE;
		}

		INT NewToolIndex = LandscapeToolSets.FindItemIndex(CurrentToolSet) - 1;
		SetCurrentTool( LandscapeToolSets.IsValidIndex(NewToolIndex) ? NewToolIndex : LandscapeToolSets.Num()-1 );

		return TRUE;
	}

	// Next tool 
	if( Event == IE_Pressed && Key == KEY_Period )
	{
		if( CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
		{
			CurrentToolSet->GetTool()->EndTool();
			bToolActive = FALSE;
		}

		INT NewToolIndex = LandscapeToolSets.FindItemIndex(CurrentToolSet) + 1;
		SetCurrentTool( LandscapeToolSets.IsValidIndex(NewToolIndex) ? NewToolIndex : 0 );

		return TRUE;
	}

	if( CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->InputKey(ViewportClient, Viewport, Key, Event)==TRUE )
	{
		return TRUE;
	}

	if( CurrentBrush && CurrentBrush->InputKey(ViewportClient, Viewport, Key, Event)==TRUE )
	{
		return TRUE;
	}

	return FALSE;
}

void FEdModeLandscape::SetCurrentTool( INT ToolIndex )
{
	ToolIndex = LandscapeToolSets.IsValidIndex(ToolIndex) ? ToolIndex : 0;
	CurrentToolSet = LandscapeToolSets( ToolIndex );
	CurrentToolSet->SetToolForTarget( CurrentToolTarget );

#if WITH_MANAGED_CODE
	if( LandscapeEditWindow.IsValid() )
	{
		LandscapeEditWindow->NotifyCurrentToolChanged(ToolIndex);
	}
#endif

	ToolUIType = CurrentToolSet->GetUIType();
}

void FEdModeLandscape::GetLayersAndThumbnails( TArray<FLandscapeLayerThumbnailInfo>& OutLayerTumbnailInfo )
{
	OutLayerTumbnailInfo.Empty();
	
	if( CurrentToolTarget.Landscape )
	{
		UTexture2D* ThumbnailWeightmap = NULL;
		UTexture2D* ThumbnailHeightmap = NULL;

		for( INT LayerIdx=0;LayerIdx<CurrentToolTarget.Landscape->LayerInfos.Num(); LayerIdx++ )
		{
			FLandscapeLayerInfo& LayerInfo = CurrentToolTarget.Landscape->LayerInfos(LayerIdx);
			FName LayerName = LayerInfo.LayerName;

			FLandscapeLayerThumbnailInfo* ThumbnailInfo = new(OutLayerTumbnailInfo) FLandscapeLayerThumbnailInfo(LayerInfo);

			if( LayerInfo.ThumbnailMIC == NULL )
			{
				if( ThumbnailWeightmap == NULL )
				{
					ThumbnailWeightmap = LoadObject<UTexture2D>(NULL, TEXT("EditorLandscapeResources.LandscapeThumbnailWeightmap"), NULL, LOAD_None, NULL);
				}
				if( ThumbnailHeightmap == NULL )
				{
					ThumbnailHeightmap = LoadObject<UTexture2D>(NULL, TEXT("EditorLandscapeResources.LandscapeThumbnailHeightmap"), NULL, LOAD_None, NULL);
				}

				// Construct Thumbnail MIC
				LayerInfo.ThumbnailMIC = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), CurrentToolTarget.Landscape->GetOutermost(), NAME_None, RF_Public|RF_Standalone);
				LayerInfo.ThumbnailMIC->SetParent(CurrentToolTarget.Landscape->LandscapeMaterial);
				FStaticParameterSet StaticParameters;
				LayerInfo.ThumbnailMIC->GetStaticParameterValues(&StaticParameters);

				for( INT LayerParameterIdx=0;LayerParameterIdx<StaticParameters.TerrainLayerWeightParameters.Num();LayerParameterIdx++ )
				{
					FStaticTerrainLayerWeightParameter& LayerParameter = StaticParameters.TerrainLayerWeightParameters(LayerParameterIdx);
					if( LayerParameter.ParameterName == LayerName )
					{
						LayerParameter.WeightmapIndex = 0;
						LayerParameter.bOverride = TRUE;
					}
					else
					{
						LayerParameter.WeightmapIndex = INDEX_NONE;
					}
				}

				LayerInfo.ThumbnailMIC->SetStaticParameterValues(&StaticParameters);
				LayerInfo.ThumbnailMIC->InitResources();
				LayerInfo.ThumbnailMIC->UpdateStaticPermutation();
				LayerInfo.ThumbnailMIC->SetTextureParameterValue(FName("Weightmap0"), ThumbnailWeightmap); 
				LayerInfo.ThumbnailMIC->SetTextureParameterValue(FName("Heightmap"), ThumbnailHeightmap);
			}

			const FObjectThumbnail* LayerThumbnail = ThumbnailTools::FindCachedThumbnail( LayerInfo.ThumbnailMIC->GetFullName() );

			// If we didn't find it in memory, OR if the thumbnail needs to be refreshed...
			if( LayerThumbnail == NULL || LayerThumbnail->IsEmpty() || LayerThumbnail->IsDirty() )
			{
				// Generate a thumbnail
				LayerThumbnail = ThumbnailTools::GenerateThumbnailForObject( LayerInfo.ThumbnailMIC );
			}

			if( LayerThumbnail != NULL )
			{
				ThumbnailInfo->LayerThumbnail = *LayerThumbnail;
			}
		}
	}
}

/** FEdMode: Called when mouse drag input it applied */
UBOOL FEdModeLandscape::InputDelta( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale )
{
	return FALSE;
}



/** FEdMode: Render the mesh paint tool */
void FEdModeLandscape::Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	/** Call parent implementation */
	FEdMode::Render( View, Viewport, PDI );

}



/** FEdMode: Render HUD elements for this tool */
void FEdModeLandscape::DrawHUD( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{

}



/** FEdMode: Called when the currently selected actor has changed */
void FEdModeLandscape::ActorSelectionChangeNotify()
{
}



/** Forces real-time perspective viewports */
void FEdModeLandscape::ForceRealTimeViewports( const UBOOL bEnable, const UBOOL bStoreCurrentState )
{
	// Force perspective views to be real-time
	for( INT CurViewportIndex = 0; CurViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount(); ++CurViewportIndex )
	{
		WxLevelViewportWindow* CurLevelViewportWindow =
			GApp->EditorFrame->ViewportConfigData->AccessViewport( CurViewportIndex ).ViewportWindow;
		if( CurLevelViewportWindow != NULL )
		{
			if( CurLevelViewportWindow->ViewportType == LVT_Perspective )
			{				
				if( bEnable )
				{
					CurLevelViewportWindow->SetRealtime( bEnable, bStoreCurrentState );
				}
				else
				{
					CurLevelViewportWindow->RestoreRealtime(TRUE);
				}
			}
		}
	}
}

/** Load UI settings from ini file */
void FLandscapeUISettings::Load()
{
	FString WindowPositionString;
	if( GConfig->GetString( TEXT("LandscapeEdit"), TEXT("WindowPosition"), WindowPositionString, GEditorUserSettingsIni ) )
	{
		TArray<FString> PositionValues;
		if( WindowPositionString.ParseIntoArray( &PositionValues, TEXT( "," ), TRUE ) == 4 )
		{
			WindowX = appAtoi( *PositionValues(0) );
			WindowY = appAtoi( *PositionValues(1) );
			WindowWidth = appAtoi( *PositionValues(2) );
			WindowHeight = appAtoi( *PositionValues(3) );
		}
	}

	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("ToolStrength"), ToolStrength, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("BrushRadius"), BrushRadius, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("BrushFalloff"), BrushFalloff, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("ErodeThresh"), ErodeThresh, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("ErodeIterationNum"), ErodeIterationNum, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("ErodeSurfaceThickness"), ErodeSurfaceThickness, GEditorUserSettingsIni );
	INT InErosionNoiseMode = ELandscapeToolNoiseMode::Sub; 
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("ErosionNoiseMode"), InErosionNoiseMode, GEditorUserSettingsIni );
	ErosionNoiseMode = (ELandscapeToolNoiseMode::Type)InErosionNoiseMode;
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("ErosionNoiseScale"), ErosionNoiseScale, GEditorUserSettingsIni );

	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("RainAmount"), RainAmount, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("SedimentCapacity"), SedimentCapacity, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("HErodeIterationNum"), HErodeIterationNum, GEditorUserSettingsIni );
	INT InRainDistMode = ELandscapeToolNoiseMode::Both; 
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("RainDistNoiseMode"), InRainDistMode, GEditorUserSettingsIni );
	RainDistMode = (ELandscapeToolNoiseMode::Type)InRainDistMode;
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("RainDistScale"), RainDistScale, GEditorUserSettingsIni );
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("HErosionDetailScale"), HErosionDetailScale, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("LandscapeEdit"), TEXT("bHErosionDetailSmooth"), bHErosionDetailSmooth, GEditorUserSettingsIni );

	INT InNoiseMode = ELandscapeToolNoiseMode::Both; 
	GConfig->GetInt( TEXT("LandscapeEdit"), TEXT("NoiseMode"), InNoiseMode, GEditorUserSettingsIni );
	NoiseMode = (ELandscapeToolNoiseMode::Type)InNoiseMode;
	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("NoiseScale"), NoiseScale, GEditorUserSettingsIni );

	GConfig->GetFloat( TEXT("LandscapeEdit"), TEXT("DetailScale"), DetailScale, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("LandscapeEdit"), TEXT("bDetailSmooth"), bDetailSmooth, GEditorUserSettingsIni );
}

/** Save UI settings to ini file */
void FLandscapeUISettings::Save()
{
	FString WindowPositionString = FString::Printf(TEXT("%d,%d,%d,%d"), WindowX, WindowY, WindowWidth, WindowHeight );
	GConfig->SetString( TEXT("LandscapeEdit"), TEXT("WindowPosition"), *WindowPositionString, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("ToolStrength"), ToolStrength, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("BrushRadius"), BrushRadius, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("BrushFalloff"), BrushFalloff, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("ErodeThresh"), ErodeThresh, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("ErodeIterationNum"), ErodeIterationNum, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("ErodeSurfaceThickness"), ErodeSurfaceThickness, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("ErosionNoiseMode"), (INT)ErosionNoiseMode, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("ErosionNoiseScale"), ErosionNoiseScale, GEditorUserSettingsIni );

	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("RainAmount"), RainAmount, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("SedimentCapacity"), SedimentCapacity, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("HErodeIterationNum"), ErodeIterationNum, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("RainDistMode"), (INT)RainDistMode, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("RainDistScale"), RainDistScale, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("HErosionDetailScale"), HErosionDetailScale, GEditorUserSettingsIni );
	GConfig->SetBool( TEXT("LandscapeEdit"), TEXT("bHErosionDetailSmooth"), bHErosionDetailSmooth, GEditorUserSettingsIni );

	GConfig->SetInt( TEXT("LandscapeEdit"), TEXT("NoiseMode"), (INT)NoiseMode, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("NoiseScale"), NoiseScale, GEditorUserSettingsIni );
	GConfig->SetFloat( TEXT("LandscapeEdit"), TEXT("DetailScale"), DetailScale, GEditorUserSettingsIni );
	GConfig->SetBool( TEXT("LandscapeEdit"), TEXT("bDetailSmooth"), bDetailSmooth, GEditorUserSettingsIni );
}