/*=============================================================================
	UnRenderUtils.h: Rendering utility classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

//
//	FPackedNormal
//

struct FPackedNormal
{
	union
	{
		struct
		{
#if __INTEL_BYTE_ORDER__ || PS3
			BYTE	X,
					Y,
					Z,
					W;
#else
			BYTE	W,
					Z,
					Y,
					X;
#endif
		};
		DWORD		Packed;
	}				Vector;

	// Constructors.

	FPackedNormal() { Vector.Packed = 0; }
	FPackedNormal( DWORD InPacked ) { Vector.Packed = InPacked; }
	FPackedNormal(const FVector& InVector) { *this = InVector; }

	// Conversion operators.

	void operator=(const FVector& InVector);
	operator FVector() const;
	operator VectorRegister() const;

	// Equality operator.

	UBOOL operator==(const FPackedNormal& B) const
	{
		if(Vector.Packed != B.Vector.Packed)
			return 0;

		FVector	V1 = *this,
				V2 = B;

		if(Abs(V1.X - V2.X) > THRESH_NORMALS_ARE_SAME * 4.0f)
			return 0;

		if(Abs(V1.Y - V2.Y) > THRESH_NORMALS_ARE_SAME * 4.0f)
			return 0;

		if(Abs(V1.Z - V2.Z) > THRESH_NORMALS_ARE_SAME * 4.0f)
			return 0;

		return 1;
	}

	// Serializer.

	friend FArchive& operator<<(FArchive& Ar,FPackedNormal& N);
};


/**
* Constructs a basis matrix for the axis vectors and returns the sign of the determinant
*
* @param XAxis - x axis (tangent)
* @param YAxis - y axis (binormal)
* @param ZAxis - z axis (normal)
* @return sign of determinant either -1 or +1 
*/
FORCEINLINE FLOAT GetBasisDeterminantSign( const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis )
{
	FMatrix Basis(
		FPlane(XAxis,0),
		FPlane(YAxis,0),
		FPlane(ZAxis,0),
		FPlane(0,0,0,1)
		);
	return (Basis.Determinant() < 0) ? -1.0f : +1.0f;
}

/**
* Constructs a basis matrix for the axis vectors and returns the sign of the determinant
*
* @param XAxis - x axis (tangent)
* @param YAxis - y axis (binormal)
* @param ZAxis - z axis (normal)
* @return sign of determinant either 0 (-1) or +1 (255)
*/
FORCEINLINE BYTE GetBasisDeterminantSignByte( const FPackedNormal& XAxis, const FPackedNormal& YAxis, const FPackedNormal& ZAxis )
{
	return appTrunc(GetBasisDeterminantSign(XAxis,YAxis,ZAxis) * 127.5f + 127.5f);
}

//
//	FWindPointSource
//

struct FWindPointSource
{
	FVector	SourceLocation;
	FLOAT	Strength,
			Phase,
			Frequency,
			InvRadius,
			InvDuration;

	// GetWind

	FVector GetWind(const FVector& Location) const;
};

//
//	EMouseCursor
//

enum EMouseCursor
{
	MC_None,
	MC_NoChange,		// Keeps the platform client from calling setcursor so a cursor can be set elsewhere (ie using wxSetCursor).
	MC_Arrow,
	MC_Cross,
	MC_SizeAll,
	MC_SizeUpRightDownLeft,
	MC_SizeUpLeftDownRight,
	MC_SizeLeftRight,
	MC_SizeUpDown,
	MC_Hand
};

/** Enumeration of pixel format flags used for GPixelFormats.Flags */
enum EPixelFormatFlags
{
	/** Whether this format supports SRGB read, aka gamma correction on texture sampling or whether we need to do it manually (if this flag is set) */
	PF_REQUIRES_GAMMA_CORRECTION = 1
};

//
//	FPixelFormatInfo
//

struct FPixelFormatInfo
{
	const TCHAR*	Name;
	INT				BlockSizeX,
					BlockSizeY,
					BlockSizeZ,
					BlockBytes,
					NumComponents;
	/** Platform specific token, e.g. D3DFORMAT with D3DDrv										*/
	DWORD			PlatformFormat;
	/** Format specific internal flags, e.g. whether SRGB is supported with this format		*/
	DWORD			Flags;
	/** Whether the texture format is supported on the current platform/ rendering combination	*/
	UBOOL			Supported;
};

extern FPixelFormatInfo GPixelFormats[];		// Maps members of EPixelFormat to a FPixelFormatInfo describing the format.


#define NUM_DEBUG_UTIL_COLORS (32)
static const FColor DebugUtilColor[NUM_DEBUG_UTIL_COLORS] = 
{
	FColor(20,226,64),
	FColor(210,21,0),
	FColor(72,100,224),
	FColor(14,153,0),
	FColor(186,0,186),
	FColor(54,0,175),
	FColor(25,204,0),
	FColor(15,189,147),
	FColor(23,165,0),
	FColor(26,206,120),
	FColor(28,163,176),
	FColor(29,0,188),
	FColor(130,0,50),
	FColor(31,0,163),
	FColor(147,0,190),
	FColor(1,0,109),
	FColor(2,126,203),
	FColor(3,0,58),
	FColor(4,92,218),
	FColor(5,151,0),
	FColor(18,221,0),
	FColor(6,0,131),
	FColor(7,163,176),
	FColor(8,0,151),
	FColor(102,0,216),
	FColor(10,0,171),
	FColor(11,112,0),
	FColor(12,167,172),
	FColor(13,189,0),
	FColor(16,155,0),
	FColor(178,161,0),
	FColor(19,25,126)
};

//
//	CalculateImageBytes
//

extern SIZE_T CalculateImageBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format);

/**
 * Handles initialization/release for a global resource.
 */
template<class ResourceType>
class TGlobalResource : public ResourceType
{
public:
	TGlobalResource()
	{
		BeginInitResource(this);
	}

	~TGlobalResource()
	{
		//cleaning up this resource isn't necessary since this is called during static exit
		//mark it as uninitialized to pass the check in ~FRenderResource()
		ResourceType::bInitialized = FALSE;
	}
};

/** A global white texture. */
extern class FTexture* GWhiteTexture;

/** A global white cube texture. */
extern class FTexture* GWhiteTextureCube;

//
// Primitive drawing utility functions.
//

// Solid shape drawing utility functions. Not really designed for speed - more for debugging.
// These utilities functions are implemented in UnScene.cpp using GetTRI.

extern void DrawBox(class FPrimitiveDrawInterface* PDI,const FMatrix& BoxToWorld,const FVector& Radii,const FMaterialRenderProxy* MaterialRenderProxy,BYTE DepthPriority);
extern void DrawSphere(class FPrimitiveDrawInterface* PDI,const FVector& Center,const FVector& Radii,INT NumSides,INT NumRings,const FMaterialRenderProxy* MaterialRenderProxy,BYTE DepthPriority);
extern void DrawCone(class FPrimitiveDrawInterface* PDI,const FMatrix& ConeToWorld, FLOAT Angle1, FLOAT Angle2, INT NumSides, UBOOL bDrawSideLines, const FColor& SideLineColor, const FMaterialRenderProxy* MaterialRenderProxy, BYTE DepthPriority);


extern void DrawCylinder(class FPrimitiveDrawInterface* PDI,const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	FLOAT Radius, FLOAT HalfHeight, INT Sides, const class FMaterialInstance* MaterialInstance, BYTE DepthPriority);
extern void DrawChoppedCone(class FPrimitiveDrawInterface* PDI,const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	FLOAT Radius, FLOAT TopRadius, FLOAT HalfHeight, INT Sides, const class FMaterialInstance* MaterialInstance, BYTE DepthPriority);


// Line drawing utility functions.

extern void DrawWireBox(class FPrimitiveDrawInterface* PDI,const FBox& Box,FColor Color,BYTE DepthPriority);
extern void DrawCircle(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,FColor Color,FLOAT Radius,INT NumSides,BYTE DepthPriority);
extern void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FVector& Base, FColor Color, FLOAT Radius, INT NumSides, BYTE DepthPriority);
extern void DrawWireCylinder(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,FColor Color,FLOAT Radius,FLOAT HalfHeight,INT NumSides,BYTE DepthPriority);
extern void DrawWireChoppedCone(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,FColor Color,FLOAT Radius,FLOAT TopRadius,FLOAT HalfHeight,INT NumSides,BYTE DepthPriority);
extern void DrawOrientedWireBox(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z, FVector Extent, FColor Color,BYTE DepthPriority);
extern void DrawDirectionalArrow(class FPrimitiveDrawInterface* PDI,const FMatrix& ArrowToWorld,FColor InColor,FLOAT Length,FLOAT ArrowSize,BYTE DepthPriority);
extern void DrawWireStar(class FPrimitiveDrawInterface* PDI,const FVector& Position, FLOAT Size, FColor Color,BYTE DepthPriority);
extern void DrawDashedLine(class FPrimitiveDrawInterface* PDI,const FVector& Start, const FVector& End, FColor Color, FLOAT DashSize,BYTE DepthPriority);
extern void DrawWireDiamond(class FPrimitiveDrawInterface* PDI,const FMatrix& DiamondMatrix, FLOAT Size, const FColor& InColor,BYTE DepthPriority);

extern void DrawFrustumWireframe(class FPrimitiveDrawInterface* PDI,const FMatrix& WorldToFrustum,FColor Color,BYTE DepthPriority);

/** The indices for drawing a cube. */
extern const WORD GCubeIndices[12*3];

/**
 * Maps from an X,Y,Z cube vertex coordinate to the corresponding vertex index.
 */
inline const WORD GetCubeVertexIndex(UBOOL X,UBOOL Y,UBOOL Z) { return X * 4 + Y * 2 + Z; }

/**
 * Given a base color and a selection state, returns a color which accounts for the selection state.
 * @param BaseColor - The base color of the object.
 * @param bSelected - The selection state of the object.
 * @return The color to draw the object with, accounting for the selection state
 */
extern FLinearColor GetSelectionColor(const FLinearColor& BaseColor,UBOOL bSelected);

/**
 * Returns true if the given view is "rich".  Rich means that calling DrawRichMesh for the view will result in a modified draw call
 * being made.
 * A view is rich if is missing the SHOW_Materials showflag, or has any of the render mode affecting showflags.
 */
extern UBOOL IsRichView(const FSceneView* View);

/** 
 *	Returns true if the given view is showing a collision mode. 
 *	This means one of the flags SHOW_CollisionNonZeroExtent, SHOW_CollisionZeroExtent or SHOW_CollisionRigidBody is set.
 */
extern UBOOL IsCollisionView(const FSceneView* View);

/**
 * Draws a mesh, modifying the material which is used depending on the view's show flags.
 * Meshes with materials irrelevant to the pass which the mesh is being drawn for may be entirely ignored.
 *
 * @param PDI - The primitive draw interface to draw the mesh on.
 * @param Mesh - The mesh to draw.
 * @param WireframeColor - The color which is used when rendering the mesh with SHOW_Wireframe.
 * @param LevelColor - The color which is used when rendering the mesh with SHOW_LevelColoration.
 * @param PropertyColor - The color to use when rendering the mesh with SHOW_PropertyColoration.
 * @param PrimitiveInfo - The FScene information about the UPrimitiveComponent.
 * @param bSelected - True if the primitive is selected.
 */
extern void DrawRichMesh(
	FPrimitiveDrawInterface* PDI,
	const struct FMeshElement& Mesh,
	const FLinearColor& WireframeColor,
	const FLinearColor& LevelColor,
	const FLinearColor& PropertyColor,
	class FPrimitiveSceneInfo *PrimitiveInfo,
	UBOOL bSelected
	);
