/*=============================================================================
	UnRenderUtils.cpp: Rendering utility classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/**
 * operator =
 */
void FPackedNormal::operator=(const FVector& InVector)
{
	// Rescale [-1..1] range to [0..255]
	VectorRegister VectorHalf255	= VectorSet( 127.5f, 127.5f, 127.5f, 127.5f );
	VectorRegister VectorToPack		= VectorLoadFloat3( &InVector );
	VectorToPack					= VectorMultiplyAdd( VectorToPack, VectorHalf255, VectorHalf255 );
	// Write out as bytes, clamped to [0..255]
	VectorStoreByte4( VectorToPack, this );
	VectorResetFloatRegisters();
}

/**
 * operator FVector
 */
FPackedNormal::operator FVector() const
{
	// Use operator VectorRegister
	VectorRegister VectorToUnpack = (VectorRegister) (*this);
	// Write to FVector and return it.
	FVector UnpackedVector;
	VectorStoreFloat3( VectorToUnpack, &UnpackedVector );
	return UnpackedVector;
}

/**
 * operator VectorRegister
 */
FPackedNormal::operator VectorRegister() const
{
	// Rescale [0..255] range to [-1..1]
	VectorRegister VectorOneOverHalf255	= VectorSet( 1 / 127.5f, 1 / 127.5f, 1 / 127.5f, 1/ 127.5f );
	VectorRegister VectorToUnpack		= VectorLoadByte4( this );
	VectorToUnpack						= VectorMultiplyAdd( VectorToUnpack, VectorOneOverHalf255, VectorNegate( VectorOne() ) );
	VectorResetFloatRegisters();
	// Return unpacked vector register.
	return VectorToUnpack;
}

//
// FPackedNormal serializer
//
FArchive& operator<<(FArchive& Ar,FPackedNormal& N)
{
#if !CONSOLE
	if ( (GCookingTarget & UE3::PLATFORM_PS3) && Ar.IsSaving() && Ar.ForceByteSwapping() )
	{
		// PS3 GPU data uses Intel byte order but the FArchive is byte-swapping. Serialize it with explicit byte order.
		Ar << N.Vector.X << N.Vector.Y << N.Vector.Z << N.Vector.W;
	}
	else
#endif
	{
		Ar << N.Vector.Packed;
	}
	return Ar;
}

//
//	Pixel format information.
//

// NOTE: If you add a new basic texture format (ie a format that could be cooked - currently PF_A32B32G32R32F through
// PF_UYVY) you MUST also update XeTools.cpp and PS3Tools.cpp to match up!
FPixelFormatInfo	GPixelFormats[] =
{
	// Name						BlockSizeX	BlockSizeY	BlockSizeZ	BlockBytes	NumComponents	PlatformFormat	Flags			Supported			EPixelFormat

	{ TEXT("unknown"),			0,			0,			0,			0,			0,				0,				0,				0			},	//	PF_Unknown
	{ TEXT("A32B32G32R32F"),	1,			1,			1,			16,			4,				0,				0,				1			},	//	PF_A32B32G32R32F
	{ TEXT("A8R8G8B8"),			1,			1,			1,			4,			4,				0,				0,				1			},	//	PF_A8R8G8B8
	{ TEXT("G8"),				1,			1,			1,			1,			1,				0,				0,				1			},	//	PF_G8
	{ TEXT("G16"),				1,			1,			1,			2,			1,				0,				0,				1			},	//	PF_G16
	{ TEXT("DXT1"),				4,			4,			1,			8,			3,				0,				0,				1			},	//	PF_DXT1
	{ TEXT("DXT3"),				4,			4,			1,			16,			4,				0,				0,				1			},	//	PF_DXT3
	{ TEXT("DXT5"),				4,			4,			1,			16,			4,				0,				0,				1			},	//	PF_DXT5
	{ TEXT("UYVY"),				2,			1,			1,			4,			4,				0,				0,				0			},	//	PF_UYVY
	{ TEXT("FloatRGB"),			1,			1,			1,			0,			3,				0,				0,				0			},	//	PF_FloatRGB
	{ TEXT("FloatRGBA"),		1,			1,			1,			0,			4,				0,				0,				0			},	//	PF_FloatRGBA
	{ TEXT("DepthStencil"),		1,			1,			1,			0,			1,				0,				0,				0			},	//	PF_DepthStencil
	{ TEXT("ShadowDepth"),		1,			1,			1,			4,			1,				0,				0,				0			},	//	PF_ShadowDepth
	{ TEXT("FilteredShadowDepth"),1,		1,			1,			4,			1,				0,				0,				0			},	//	PF_FilteredShadowDepth
	{ TEXT("R32F"),				1,			1,			1,			4,			1,				0,				0,				1			},	//	PF_R32F
	{ TEXT("FloatRGBA_FUll"),	1,			1,			1,			0,			4,				0,				0,				0			},	//	PF_FloatRGBA_Full
	{ TEXT("G16R16"),			1,			1,			1,			4,			2,				0,				0,				1			},	//	PF_G16R16
	{ TEXT("G16R16F"),			1,			1,			1,			4,			2,				0,				0,				1			},	//	PF_G16R16F
	{ TEXT("G32R32F"),			1,			1,			1,			8,			2,				0,				0,				1			},	//	PF_G32R32F
	{ TEXT("A2B10G10R10"),      1,          1,          1,          4,          4,              0,              0,              1           },  //  PF_A2B10G10R10
	{ TEXT("A16B16G16R16"),		1,			1,			1,			8,			4,				0,				0,				1			},	//	PF_A16B16G16R16
	{ TEXT("D24"),				1,			1,			1,			4,			1,				0,				0,				1			},	//	PF_D24
};

//
//	CalculateImageBytes
//

SIZE_T CalculateImageBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format)
{
	if( SizeZ > 0 )
		return (SizeX / GPixelFormats[Format].BlockSizeX) * (SizeY / GPixelFormats[Format].BlockSizeY) * (SizeZ / GPixelFormats[Format].BlockSizeZ) * GPixelFormats[Format].BlockBytes;
	else
		return (SizeX / GPixelFormats[Format].BlockSizeX) * (SizeY / GPixelFormats[Format].BlockSizeY) * GPixelFormats[Format].BlockBytes;
}

//
// FWhiteTexture implementation
//

/**
 * A solid white texture.
 */
class FWhiteTexture : public FTextureResource
{
public:

	// FResource interface.
	virtual void InitRHI()
	{
		// Create the texture RHI.  		
#if XBOX
		// make it ResolveTargetable so it is created on Xenon using the D3D functions instead of XG; otherwise
		// it shows up as a 1x1 black texture
		DWORD CreationFlags=TexCreate_ResolveTargetable;
#else
		DWORD CreationFlags=0;
#endif
		FTexture2DRHIRef Texture2D = RHICreateTexture2D(1,1,PF_A8R8G8B8,1,CreationFlags);
		TextureRHI = Texture2D;

		// Write the contents of the texture.
		UINT DestStride;
		FColor* DestBuffer = (FColor*)RHILockTexture2D(Texture2D,0,TRUE,DestStride);
		*DestBuffer = FColor(255,255,255);
		RHIUnlockTexture2D(Texture2D,0);

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer =
		{
			SF_Nearest,
			AM_Wrap,
			AM_Wrap,
			AM_Wrap
		};
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}

	/** Returns the width of the texture in pixels. */
	virtual UINT GetSizeX() const
	{
		return 1;
	}

	/** Returns the height of the texture in pixels. */
	virtual UINT GetSizeY() const
	{
		return 1;
	}
};

FTexture* GWhiteTexture = new TGlobalResource<FWhiteTexture>;

//
// FWhiteTextureCube implementation
//

/**
 * A solid white cube texture.
 */
class FWhiteTextureCube : public FTextureResource
{
public:

	// FResource interface.
	virtual void InitRHI()
	{
		// Create the texture RHI.  		
#if XBOX
		// make it ResolveTargetable so it is created on Xenon using the D3D functions instead of XG; otherwise
		// it shows up as a 1x1 black texture
		DWORD CreationFlags=TexCreate_ResolveTargetable;
#else
		DWORD CreationFlags=0;
#endif
		FTextureCubeRHIRef TextureCube = RHICreateTextureCube(1,PF_A8R8G8B8,1,CreationFlags);
		TextureRHI = TextureCube;

		// Write the contents of the texture.
		for(UINT FaceIndex = 0;FaceIndex < 6;FaceIndex++)
		{
			UINT DestStride;
			FColor* DestBuffer = (FColor*)RHILockTextureCubeFace(TextureCube,FaceIndex,0,TRUE,DestStride);
			*DestBuffer = FColor(255,255,255);
			RHIUnlockTextureCubeFace(TextureCube,FaceIndex,0);
		}

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer =
		{
			SF_Nearest,
			AM_Wrap,
			AM_Wrap,
			AM_Wrap
		};
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}

	/** Returns the width of the texture in pixels. */
	virtual UINT GetSizeX() const
	{
		return 1;
	}

	/** Returns the height of the texture in pixels. */
	virtual UINT GetSizeY() const
	{
		return 1;
	}
};

FTexture* GWhiteTextureCube = new TGlobalResource<FWhiteTextureCube>;

//
// Primitive drawing utility functions.
//

void DrawBox(FPrimitiveDrawInterface* PDI,const FMatrix& BoxToWorld,const FVector& Radii,const FMaterialRenderProxy* MaterialRenderProxy,BYTE DepthPriorityGroup)
{
	// Calculate verts for a face pointing down Z
	FVector Positions[4] =
	{
		FVector(-1, -1, +1),
		FVector(-1, +1, +1),
		FVector(+1, +1, +1),
		FVector(+1, -1, +1)
	};
	FVector2D UVs[4] =
	{
		FVector2D(0,0),
		FVector2D(0,1),
		FVector2D(1,1),
		FVector2D(1,0),
	};

	// Then rotate this face 6 times
	FRotator FaceRotations[6];
	FaceRotations[0] = FRotator(0,		0,	0);
	FaceRotations[1] = FRotator(16384,	0,	0);
	FaceRotations[2] = FRotator(-16384,	0,  0);
	FaceRotations[3] = FRotator(0,		0,	16384);
	FaceRotations[4] = FRotator(0,		0,	-16384);
	FaceRotations[5] = FRotator(32768,	0,	0);

	FDynamicMeshBuilder MeshBuilder;

	for(INT f=0; f<6; f++)
	{
		FMatrix FaceTransform = FRotationMatrix(FaceRotations[f]);

		INT VertexIndices[4];
		for(INT VertexIndex = 0;VertexIndex < 4;VertexIndex++)
		{
			VertexIndices[VertexIndex] = MeshBuilder.AddVertex(
				FaceTransform.TransformFVector( Positions[VertexIndex] ),
				UVs[VertexIndex],
				FaceTransform.TransformNormal(FVector(1,0,0)),
				FaceTransform.TransformNormal(FVector(0,1,0)),
				FaceTransform.TransformNormal(FVector(0,0,1))
				);
		}

		MeshBuilder.AddTriangle(VertexIndices[0],VertexIndices[1],VertexIndices[2]);
		MeshBuilder.AddTriangle(VertexIndices[0],VertexIndices[2],VertexIndices[3]);
	}

	MeshBuilder.Draw(PDI,FScaleMatrix(Radii) * BoxToWorld,MaterialRenderProxy,DepthPriorityGroup);
}



void DrawSphere(FPrimitiveDrawInterface* PDI,const FVector& Center,const FVector& Radii,INT NumSides,INT NumRings,const FMaterialRenderProxy* MaterialRenderProxy,BYTE DepthPriority)
{
	// Use a mesh builder to draw the sphere.
	FDynamicMeshBuilder MeshBuilder;
	{
		// The first/last arc are on top of each other.
		INT NumVerts = (NumSides+1) * (NumRings+1);
		FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)appMalloc( NumVerts * sizeof(FDynamicMeshVertex) );

		// Calculate verts for one arc
		FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)appMalloc( (NumRings+1) * sizeof(FDynamicMeshVertex) );

		for(INT i=0; i<NumRings+1; i++)
		{
			FDynamicMeshVertex* ArcVert = &ArcVerts[i];

			FLOAT angle = ((FLOAT)i/NumRings) * PI;

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
			ArcVert->Position.X = 0.0f;
			ArcVert->Position.Y = appSin(angle);
			ArcVert->Position.Z = appCos(angle);

			ArcVert->SetTangents(
				FVector(1,0,0),
				FVector(0.0f,-ArcVert->Position.Z,ArcVert->Position.Y),
				ArcVert->Position
				);

			ArcVert->TextureCoordinate.X = 0.0f;
			ArcVert->TextureCoordinate.Y = ((FLOAT)i/NumRings);
		}

		// Then rotate this arc NumSides+1 times.
		for(INT s=0; s<NumSides+1; s++)
		{
			FRotator ArcRotator(0, appTrunc(65535.f * ((FLOAT)s/NumSides)), 0);
			FRotationMatrix ArcRot( ArcRotator );
			FLOAT XTexCoord = ((FLOAT)s/NumSides);

			for(INT v=0; v<NumRings+1; v++)
			{
				INT VIx = (NumRings+1)*s + v;

				Verts[VIx].Position = ArcRot.TransformFVector( ArcVerts[v].Position );
				
				Verts[VIx].SetTangents(
					ArcRot.TransformNormal( ArcVerts[v].TangentX ),
					ArcRot.TransformNormal( ArcVerts[v].GetTangentY() ),
					ArcRot.TransformNormal( ArcVerts[v].TangentZ )
					);

				Verts[VIx].TextureCoordinate.X = XTexCoord;
				Verts[VIx].TextureCoordinate.Y = ArcVerts[v].TextureCoordinate.Y;
			}
		}

		// Add all of the vertices we generated to the mesh builder.
		for(INT VertIdx=0; VertIdx < NumVerts; VertIdx++)
		{
			MeshBuilder.AddVertex(Verts[VertIdx]);
		}
		
		// Add all of the triangles we generated to the mesh builder.
		for(INT s=0; s<NumSides; s++)
		{
			INT a0start = (s+0) * (NumRings+1);
			INT a1start = (s+1) * (NumRings+1);

			for(INT r=0; r<NumRings; r++)
			{
				MeshBuilder.AddTriangle(a0start + r + 0, a1start + r + 0, a0start + r + 1);
				MeshBuilder.AddTriangle(a1start + r + 0, a1start + r + 1, a0start + r + 1);
			}
		}

		// Free our local copy of verts and arc verts
		appFree(Verts);
		appFree(ArcVerts);
	}
	MeshBuilder.Draw(PDI, FScaleMatrix( Radii ) * FTranslationMatrix( Center ), MaterialRenderProxy, DepthPriority);
}

void DrawCone(FPrimitiveDrawInterface* PDI,const FMatrix& ConeToWorld, FLOAT Angle1, FLOAT Angle2, INT NumSides, UBOOL bDrawSideLines, const FColor& SideLineColor, const FMaterialRenderProxy* MaterialRenderProxy, BYTE DepthPriority)
{
	FLOAT ang1 = Clamp<FLOAT>(Angle1, 0.01f, (FLOAT)PI - 0.01f);
	FLOAT ang2 = Clamp<FLOAT>(Angle2, 0.01f, (FLOAT)PI - 0.01f);

	FLOAT sinX_2 = appSin(0.5f * ang1);
	FLOAT sinY_2 = appSin(0.5f * ang2);

	FLOAT sinSqX_2 = sinX_2 * sinX_2;
	FLOAT sinSqY_2 = sinY_2 * sinY_2;

	FLOAT tanX_2 = appTan(0.5f * ang1);
	FLOAT tanY_2 = appTan(0.5f * ang2);

	TArray<FVector> ConeVerts(NumSides);

	for(INT i = 0; i < NumSides; i++)
	{
		FLOAT Fraction = (FLOAT)i/(FLOAT)(NumSides);
		FLOAT thi = 2.f*PI*Fraction;
		FLOAT phi = appAtan2(appSin(thi)*sinY_2, appCos(thi)*sinX_2);
		FLOAT sinPhi = appSin(phi);
		FLOAT cosPhi = appCos(phi);
		FLOAT sinSqPhi = sinPhi*sinPhi;
		FLOAT cosSqPhi = cosPhi*cosPhi;

		FLOAT rSq, r, Sqr, alpha, beta;

		rSq = sinSqX_2*sinSqY_2/(sinSqX_2*sinSqPhi + sinSqY_2*cosSqPhi);
		r = appSqrt(rSq);
		Sqr = appSqrt(1-rSq);
		alpha = r*cosPhi;
		beta  = r*sinPhi;

		ConeVerts(i).X = (1-2*rSq);
		ConeVerts(i).Y = 2*Sqr*alpha;
		ConeVerts(i).Z = 2*Sqr*beta;
	}

	FDynamicMeshBuilder MeshBuilder;
	{
		for(INT i=0; i < NumSides; i++)
		{
			FDynamicMeshVertex V0, V1, V2;

			FVector TriTangentZ = ConeVerts((i+1)%NumSides) ^ ConeVerts( i ); // aka triangle normal
			FVector TriTangentY = ConeVerts(i);
			FVector TriTangentX = TriTangentZ ^ TriTangentY;

			V0.Position = FVector(0);
			V0.TextureCoordinate.X = 0.0f;
			V0.TextureCoordinate.Y = (FLOAT)i/NumSides;
			V0.SetTangents(TriTangentX,TriTangentY,TriTangentZ);

			V1.Position = ConeVerts(i);
			V1.TextureCoordinate.X = 1.0f;
			V1.TextureCoordinate.Y = (FLOAT)i/NumSides;
			V1.SetTangents(TriTangentX,TriTangentY,TriTangentZ);

			V2.Position = ConeVerts((i+1)%NumSides);
			V2.TextureCoordinate.X = 1.0f;
			V2.TextureCoordinate.Y = (FLOAT)((i+1)%NumSides)/NumSides;
			V2.SetTangents(TriTangentX,TriTangentY,TriTangentZ);

			const INT VertexStart = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);

			MeshBuilder.AddTriangle(VertexStart,VertexStart+1,VertexStart+2);
		}
	}
	MeshBuilder.Draw(PDI, ConeToWorld, MaterialRenderProxy, DepthPriority);


	if(bDrawSideLines)
	{
		// Draw lines down major directions
		for(INT i=0; i<4; i++)
		{
			PDI->DrawLine( ConeToWorld.GetOrigin(), ConeToWorld.TransformFVector( ConeVerts( (i*NumSides/4)%NumSides ) ), SideLineColor, DepthPriority );
		}
	}
}


void DrawCylinder(FPrimitiveDrawInterface* PDI,const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	FLOAT Radius, FLOAT HalfHeight, INT Sides, const FMaterialRenderProxy* MaterialRenderProxy, BYTE DepthPriority)
{
	const FLOAT	AngleDelta = 2.0f * PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	FLOAT TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	FDynamicMeshBuilder MeshBuilder;


	//Compute vertices for base circle.
	for(INT SideIndex = 0;SideIndex < Sides;SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * appCos(AngleDelta * (SideIndex + 1)) + YAxis * appSin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex - TopOffset;
		MeshVertex.TextureCoordinate = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
			);

		MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for(INT SideIndex = 0;SideIndex < Sides;SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * appCos(AngleDelta * (SideIndex + 1)) + YAxis * appSin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex + TopOffset;
		MeshVertex.TextureCoordinate = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
			);

		MeshBuilder.AddVertex(MeshVertex); //Add top vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}
	
	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for(INT SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		INT V0 = 0;
		INT V1 = SideIndex;
		INT V2 = (SideIndex + 1) % Sides;

		MeshBuilder.AddTriangle(V0, V1, V2); //bottom
		MeshBuilder.AddTriangle(Sides + V2, Sides + V1 , Sides + V0); //top
	}

	//Add sides.

	for(INT SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		INT V0 = SideIndex;
		INT V1 = (SideIndex + 1) % Sides;
		INT V2 = V0 + Sides;
		INT V3 = V1 + Sides;

		MeshBuilder.AddTriangle(V0, V2, V1);
		MeshBuilder.AddTriangle(V2, V3, V1);
	}

	MeshBuilder.Draw(PDI, FMatrix::Identity, MaterialRenderProxy, DepthPriority);
}


// Line drawing utility functions.

/**
 * Draws a wireframe box.
 *
 * @param	PDI				Draw interface.
 * @param	Box				The FBox to use for drawing.
 * @param	Color			Color of the box.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawWireBox(FPrimitiveDrawInterface* PDI,const FBox& Box,FColor Color,BYTE DepthPriority)
{
	FVector	B[2],P,Q;
	int i,j;

	B[0]=Box.Min;
	B[1]=Box.Max;

	for( i=0; i<2; i++ ) for( j=0; j<2; j++ )
	{
		P.X=B[i].X; Q.X=B[i].X;
		P.Y=B[j].Y; Q.Y=B[j].Y;
		P.Z=B[0].Z; Q.Z=B[1].Z;
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Y=B[i].Y; Q.Y=B[i].Y;
		P.Z=B[j].Z; Q.Z=B[j].Z;
		P.X=B[0].X; Q.X=B[1].X;
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Z=B[i].Z; Q.Z=B[i].Z;
		P.X=B[j].X; Q.X=B[j].X;
		P.Y=B[0].Y; Q.Y=B[1].Y;
		PDI->DrawLine(P,Q,Color,DepthPriority);
	}
}

/**
 * Draws a circle using lines.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the circle.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the circle.
 * @param	Radius			Radius of the circle.
 * @param	NumSides		Numbers of sides that the circle has.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawCircle(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,FColor Color,FLOAT Radius,INT NumSides,BYTE DepthPriority)
{
	const FLOAT	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	for(INT SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * appCos(AngleDelta * (SideIndex + 1)) + Y * appSin(AngleDelta * (SideIndex + 1))) * Radius;
		PDI->DrawLine(LastVertex,Vertex,Color,DepthPriority);
		LastVertex = Vertex;
	}
}

/**
 * Draws a sphere using circles.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the sphere.
 * @param	Color			Color of the sphere.
 * @param	Radius			Radius of the sphere.
 * @param	NumSides		Numbers of sides that the circle has.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FVector& Base, FColor Color, FLOAT Radius, INT NumSides, BYTE DepthPriority)
{
  DrawCircle(PDI, Base, FVector(1,0,0), FVector(0,1,0), Color, Radius, NumSides, DepthPriority);
  DrawCircle(PDI, Base, FVector(1,0,0), FVector(0,0,1), Color, Radius, NumSides, DepthPriority);
  DrawCircle(PDI, Base, FVector(0,1,0), FVector(0,0,1), Color, Radius, NumSides, DepthPriority);
}

/**
 * Draws a wireframe cylinder.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center pointer of the base of the cylinder.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the cylinder.
 * @param	Radius			Radius of the cylinder.
 * @param	HalfHeight		Half of the height of the cylinder.
 * @param	NumSides		Numbers of sides that the cylinder has.
 * @param	DepthPriority	Depth priority for the cylinder.
 */
void DrawWireCylinder(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,FColor Color,FLOAT Radius,FLOAT HalfHeight,INT NumSides,BYTE DepthPriority)
{
	const FLOAT	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	for(INT SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * appCos(AngleDelta * (SideIndex + 1)) + Y * appSin(AngleDelta * (SideIndex + 1))) * Radius;

		PDI->DrawLine(LastVertex - Z * HalfHeight,Vertex - Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex + Z * HalfHeight,Vertex + Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex - Z * HalfHeight,LastVertex + Z * HalfHeight,Color,DepthPriority);

		LastVertex = Vertex;
	}
}


/**
 * Draws a wireframe chopped cone(cylinder with independant top and bottom radius).
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center pointer of the base of the cone.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the cone.
 * @param	Radius			Radius of the cone at the bottom.
 * @param	TopRadius		Radius of the cone at the top.
 * @param	HalfHeight		Half of the height of the cone.
 * @param	NumSides		Numbers of sides that the cone has.
 * @param	DepthPriority	Depth priority for the cone.
 */
void DrawWireChoppedCone(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,FColor Color,FLOAT Radius, FLOAT TopRadius,FLOAT HalfHeight,INT NumSides,BYTE DepthPriority)
{
	const FLOAT	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;
	FVector LastTopVertex = Base + X * TopRadius;

	for(INT SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * appCos(AngleDelta * (SideIndex + 1)) + Y * appSin(AngleDelta * (SideIndex + 1))) * Radius;
		const FVector TopVertex = Base + (X * appCos(AngleDelta * (SideIndex + 1)) + Y * appSin(AngleDelta * (SideIndex + 1))) * TopRadius;	

		PDI->DrawLine(LastVertex - Z * HalfHeight,Vertex - Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastTopVertex + Z * HalfHeight,TopVertex + Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex - Z * HalfHeight,LastTopVertex + Z * HalfHeight,Color,DepthPriority);

		LastVertex = Vertex;
		LastTopVertex = TopVertex;
	}
}

/**
 * Draws an oriented box.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center point of the box.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the box.
 * @param	Extent			Vector with the half-sizes of the box.
 * @param	DepthPriority	Depth priority for the cone.
 */

void DrawOrientedWireBox(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z, FVector Extent, FColor Color,BYTE DepthPriority)
{
	FVector	B[2],P,Q;
	int i,j;

	FMatrix m(X, Y, Z, Base);
	B[0] = -Extent;
	B[1] = Extent;

	for( i=0; i<2; i++ ) for( j=0; j<2; j++ )
	{
		P.X=B[i].X; Q.X=B[i].X;
		P.Y=B[j].Y; Q.Y=B[j].Y;
		P.Z=B[0].Z; Q.Z=B[1].Z;
		P = m.TransformFVector(P); Q = m.TransformFVector(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Y=B[i].Y; Q.Y=B[i].Y;
		P.Z=B[j].Z; Q.Z=B[j].Z;
		P.X=B[0].X; Q.X=B[1].X;
		P = m.TransformFVector(P); Q = m.TransformFVector(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Z=B[i].Z; Q.Z=B[i].Z;
		P.X=B[j].X; Q.X=B[j].X;
		P.Y=B[0].Y; Q.Y=B[1].Y;
		P = m.TransformFVector(P); Q = m.TransformFVector(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);
	}
}


/**
 * Draws a directional arrow.
 *
 * @param	PDI				Draw interface.
 * @param	ArrowToWorld	Transform matrix for the arrow.
 * @param	InColor			Color of the arrow.
 * @param	Length			Length of the arrow
 * @param	ArrowSize		Size of the arrow head.
 * @param	DepthPriority	Depth priority for the arrow.
 */
void DrawDirectionalArrow(FPrimitiveDrawInterface* PDI,const FMatrix& ArrowToWorld,FColor InColor,FLOAT Length,FLOAT ArrowSize,BYTE DepthPriority)
{
	PDI->DrawLine(ArrowToWorld.TransformFVector(FVector(Length,0,0)),ArrowToWorld.TransformFVector(FVector(0,0,0)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformFVector(FVector(Length,0,0)),ArrowToWorld.TransformFVector(FVector(Length-ArrowSize,+ArrowSize,+ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformFVector(FVector(Length,0,0)),ArrowToWorld.TransformFVector(FVector(Length-ArrowSize,+ArrowSize,-ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformFVector(FVector(Length,0,0)),ArrowToWorld.TransformFVector(FVector(Length-ArrowSize,-ArrowSize,+ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformFVector(FVector(Length,0,0)),ArrowToWorld.TransformFVector(FVector(Length-ArrowSize,-ArrowSize,-ArrowSize)),InColor,DepthPriority);
}

/**
 * Draws a axis-aligned 3 line star.
 *
 * @param	PDI				Draw interface.
 * @param	Position		Position of the star.
 * @param	Size			Size of the star
 * @param	InColor			Color of the arrow.
 * @param	DepthPriority	Depth priority for the star.
 */
void DrawWireStar(FPrimitiveDrawInterface* PDI,const FVector& Position, FLOAT Size, FColor Color,BYTE DepthPriority)
{
	PDI->DrawLine(Position + Size * FVector(1,0,0), Position - Size * FVector(1,0,0), Color, DepthPriority);
	PDI->DrawLine(Position + Size * FVector(0,1,0), Position - Size * FVector(0,1,0), Color, DepthPriority);
	PDI->DrawLine(Position + Size * FVector(0,0,1), Position - Size * FVector(0,0,1), Color, DepthPriority);
}

/**
 * Draws a dashed line.
 *
 * @param	PDI				Draw interface.
 * @param	Start			Start position of the line.
 * @param	End				End position of the line.
 * @param	Color			Color of the arrow.
 * @param	DashSize		Size of each of the dashes that makes up the line.
 * @param	DepthPriority	Depth priority for the line.
 */
void DrawDashedLine(FPrimitiveDrawInterface* PDI,const FVector& Start, const FVector& End, FColor Color, FLOAT DashSize,BYTE DepthPriority)
{
	FVector LineDir = End - Start;
	FLOAT LineLeft = (End - Start).Size();
	LineDir /= LineLeft;

	while(LineLeft > 0.f)
	{
		const FVector DrawStart = End - ( LineLeft * LineDir );
		const FVector DrawEnd = DrawStart + ( Min<FLOAT>(DashSize, LineLeft) * LineDir );

		PDI->DrawLine(DrawStart, DrawEnd, Color, DepthPriority);

		LineLeft -= 2*DashSize;
	}
}

/**
 * Draws a wireframe diamond.
 *
 * @param	PDI				Draw interface.
 * @param	DiamondMatrix	Transform Matrix for the diamond.
 * @param	Size			Size of the diamond.
 * @param	InColor			Color of the diamond.
 * @param	DepthPriority	Depth priority for the diamond.
 */
void DrawWireDiamond(FPrimitiveDrawInterface* PDI,const FMatrix& DiamondMatrix, FLOAT Size, const FColor& InColor,BYTE DepthPriority)
{
	const FVector TopPoint = DiamondMatrix.TransformFVector( FVector(0,0,1) * Size );
	const FVector BottomPoint = DiamondMatrix.TransformFVector( FVector(0,0,-1) * Size );

	const FLOAT OneOverRootTwo = appSqrt(0.5f);

	FVector SquarePoints[4];
	SquarePoints[0] = DiamondMatrix.TransformFVector( FVector(1,1,0) * Size * OneOverRootTwo );
	SquarePoints[1] = DiamondMatrix.TransformFVector( FVector(1,-1,0) * Size * OneOverRootTwo );
	SquarePoints[2] = DiamondMatrix.TransformFVector( FVector(-1,-1,0) * Size * OneOverRootTwo );
	SquarePoints[3] = DiamondMatrix.TransformFVector( FVector(-1,1,0) * Size * OneOverRootTwo );

	PDI->DrawLine(TopPoint, SquarePoints[0], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[3], InColor, DepthPriority);

	PDI->DrawLine(BottomPoint, SquarePoints[0], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[3], InColor, DepthPriority);

	PDI->DrawLine(SquarePoints[0], SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[1], SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[2], SquarePoints[3], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[3], SquarePoints[0], InColor, DepthPriority);
}

const WORD GCubeIndices[12*3] =
{
	0, 2, 3,
	0, 3, 1,
	4, 5, 7,
	4, 7, 6,
	0, 1, 5,
	0, 5, 4,
	2, 6, 7,
	2, 7, 3,
	0, 4, 6,
	0, 6, 2,
	1, 3, 7,
	1, 7, 5,
};

FLinearColor GetSelectionColor(const FLinearColor& BaseColor,UBOOL bSelected)
{
	const FLOAT SelectionFactor = bSelected ? 1.0f : 0.5f;

	// Apply the selection factor in SRGB space, to match legacy behavior.
	return FLinearColor(
		appPow(appPow(BaseColor.R,1.0f / 2.2f) * SelectionFactor,2.2f),
		appPow(appPow(BaseColor.G,1.0f / 2.2f) * SelectionFactor,2.2f),
		appPow(appPow(BaseColor.B,1.0f / 2.2f) * SelectionFactor,2.2f),
		BaseColor.A
		);
}

UBOOL IsRichView(const FSceneView* View)
{
	// Flags which make the view rich when absent.
	const EShowFlags NonRichShowFlags =
		SHOW_Materials |
		SHOW_Lighting;
	if((View->Family->ShowFlags & NonRichShowFlags) != NonRichShowFlags)
	{
		return TRUE;
	}

	// Flags which make the view rich when present.
	const EShowFlags RichShowFlags =
		SHOW_Wireframe |
		SHOW_LevelColoration |
		SHOW_BSPSplit |
		SHOW_LightComplexity |
		SHOW_ShaderComplexity |
		SHOW_PropertyColoration |
		SHOW_MeshEdges | 
		SHOW_LightInfluences |
		SHOW_TextureDensity;

	if(View->Family->ShowFlags & RichShowFlags)
	{
		return TRUE;
	}

	return FALSE;
}

/** Utility for returning if a view is using one of the collision views. */
UBOOL IsCollisionView(const FSceneView* View)
{
	if(View->Family->ShowFlags & SHOW_Collision_Any)
	{
		return TRUE;
	}

	return FALSE;
}

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
void DrawRichMesh(
	FPrimitiveDrawInterface* PDI,
	const FMeshElement& Mesh,
	const FLinearColor& WireframeColor,
	const FLinearColor& LevelColor,
	const FLinearColor& PropertyColor,
	FPrimitiveSceneInfo *PrimitiveInfo,
	UBOOL bSelected
	)
{
#if FINAL_RELEASE
	// Draw the mesh unmodified.
	PDI->DrawMesh( Mesh );
#else
	const EShowFlags ShowFlags = PDI->View->Family->ShowFlags;

	if(ShowFlags & SHOW_Wireframe)
	{
		// In wireframe mode, draw the edges of the mesh with the specified wireframe color, or
		// with the level or property color if level or property coloration is enabled.
		FLinearColor BaseColor( WireframeColor );
		if ( ShowFlags & SHOW_PropertyColoration )
		{
			BaseColor = PropertyColor;
		}
		else if ( ShowFlags & SHOW_LevelColoration )
		{
			BaseColor = LevelColor;
		}

		const FColoredMaterialRenderProxy WireframeMaterialInstance(
			GEngine->WireframeMaterial->GetRenderProxy(FALSE),
			GetSelectionColor( BaseColor, bSelected )
			);
		FMeshElement ModifiedMesh = Mesh;
		ModifiedMesh.bWireframe = TRUE;
		ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
		PDI->DrawMesh(ModifiedMesh);
	}
	else if(ShowFlags & SHOW_LightComplexity)
	{
		// Don't render unlit translucency when in 'light complexity' viewmode.
		if (!Mesh.IsTranslucent() || Mesh.MaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit)
		{
			// Count the number of lights interacting with this primitive.
			INT NumLights = 0;
			FLightPrimitiveInteraction *LightList = PrimitiveInfo->LightList;
			while ( LightList )
			{
				const FLightSceneInfo* LightSceneInfo = LightList->GetLight();

				// Don't count sky lights, since they're "free".
				if ( LightSceneInfo->LightType != LightType_Sky )
				{
					// Determine the interaction type between the mesh and the light.
					FLightInteraction LightInteraction = FLightInteraction::Uncached();
					if(Mesh.LCI)
					{
						LightInteraction = Mesh.LCI->GetInteraction(LightSceneInfo);
					}

					// Don't count light-mapped or irrelevant lights.
					if(LightInteraction.GetType() != LIT_CachedIrrelevant && LightInteraction.GetType() != LIT_CachedLightMap)
					{
						NumLights++;
					}
				}
				LightList = LightList->GetNextLight();
			}

			// Get a colored material to represent the number of lights.
			NumLights = Min( NumLights, GEngine->LightComplexityColors.Num() - 1 );
			FColor Color = GEngine->LightComplexityColors(NumLights);
			FColoredMaterialRenderProxy LightComplexityMaterialInstance(
				GEngine->LevelColorationUnlitMaterial->GetRenderProxy(FALSE),
				Color );

			// Draw the mesh colored by light complexity.
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &LightComplexityMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
	}
	else if(!(ShowFlags & SHOW_Materials))
	{
		// Don't render unlit translucency when in 'lighting only' viewmode.
		if (!Mesh.IsTranslucent() || Mesh.MaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit)
		{
			// When materials aren't shown, apply the same basic material to all meshes.
			FMeshElement ModifiedMesh = Mesh;

			const FColoredMaterialRenderProxy LightingOnlyMaterialInstance(
				GEngine->LevelColorationLitMaterial->GetRenderProxy(FALSE),
				FLinearColor(.5f, .5f, .5f, 1.0f)
				);

			ModifiedMesh.MaterialRenderProxy = &LightingOnlyMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
	}
	else
	{
		if(ShowFlags & SHOW_MeshEdges)
		{
			// Draw the mesh's edges in blue, on top of the base geometry.
			FColoredMaterialRenderProxy WireframeMaterialInstance(
				GEngine->WireframeMaterial->GetRenderProxy(FALSE),
				FLinearColor(0,0,1)
				);
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.bWireframe = TRUE;
			ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
	
		// Draw lines to lights affecting this mesh if its selected.
		if(ShowFlags & SHOW_LightInfluences && bSelected)
		{
			FLightPrimitiveInteraction *LightList = PrimitiveInfo->LightList;
			while ( LightList )
			{
				const FLightSceneInfo* LightSceneInfo = LightList->GetLight();

				// Don't count sky lights, since they're "free".
				if ( LightSceneInfo->LightType != LightType_Sky )
				{
					// Determine the interaction type between the mesh and the light.
					FLightInteraction LightInteraction = FLightInteraction::Uncached();
					if(Mesh.LCI)
					{
						LightInteraction = Mesh.LCI->GetInteraction(LightSceneInfo);
					}

					// Don't count light-mapped or irrelevant lights.
					if(LightInteraction.GetType() != LIT_CachedIrrelevant)
					{
						FColor LineColor = (LightInteraction.GetType() != LIT_CachedLightMap) ? FColor(255,140,0) : FColor(0,140,255);
						PDI->DrawLine( Mesh.LocalToWorld.GetOrigin(), LightSceneInfo->LightToWorld.GetOrigin(), LineColor, SDPG_World );
					}
				}
				LightList = LightList->GetNextLight();
			}
		}

		if(ShowFlags & SHOW_PropertyColoration)
		{
			// In property coloration mode, override the mesh's material with a color that was chosen based on property value.
			const UMaterial* PropertyColorationMaterial = (ShowFlags & SHOW_ViewMode_Lit) ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;

			const FColoredMaterialRenderProxy PropertyColorationMaterialInstance(
				PropertyColorationMaterial->GetRenderProxy(FALSE),
				GetSelectionColor(PropertyColor,bSelected)
				);
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &PropertyColorationMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
		else if ( ShowFlags & SHOW_LevelColoration )
		{
			const UMaterial* LevelColorationMaterial = (ShowFlags & SHOW_ViewMode_Lit) ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;
			// Draw the mesh with level coloration.
			const FColoredMaterialRenderProxy LevelColorationMaterialInstance(
				LevelColorationMaterial->GetRenderProxy(FALSE),
				GetSelectionColor(LevelColor,bSelected)
				);
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &LevelColorationMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
		else if (	(ShowFlags & SHOW_BSPSplit) 
				&&	PrimitiveInfo->Component 
				&&	PrimitiveInfo->Component->IsA(UModelComponent::StaticClass()) )
		{
			// Determine unique color for model component.
			FLinearColor BSPSplitColor;
			FRandomStream RandomStream( PrimitiveInfo->Component->GetIndex() );
			BSPSplitColor.R = RandomStream.GetFraction();
			BSPSplitColor.G = RandomStream.GetFraction();
			BSPSplitColor.B = RandomStream.GetFraction();
			BSPSplitColor.A = 1.0f;

			// Piggy back on the level coloration material.
			const UMaterial* BSPSplitMaterial = (ShowFlags & SHOW_ViewMode_Lit) ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;
			
			// Draw BSP mesh with unique color for each model component.
			const FColoredMaterialRenderProxy BSPSplitMaterialInstance(
				BSPSplitMaterial->GetRenderProxy(FALSE),
				GetSelectionColor(BSPSplitColor,bSelected)
				);
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &BSPSplitMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
		else if( ShowFlags & SHOW_Collision_Any)
		{
			const UMaterial* LevelColorationMaterial = (ShowFlags & SHOW_ViewMode_Lit) ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;
			const FColoredMaterialRenderProxy CollisionMaterialInstance(
				LevelColorationMaterial->GetRenderProxy(bSelected),
				GetSelectionColor(LevelColor,bSelected)
				);
			FMeshElement ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &CollisionMaterialInstance;
			PDI->DrawMesh(ModifiedMesh);
		}
		else 
		{
			// Draw the mesh unmodified.
			PDI->DrawMesh( Mesh );
		}
	}
#endif
}

/** Returns the current display gamma. */
FLOAT GetDisplayGamma(void)
{
	return GEngine->Client->DisplayGamma;
}

/** Sets the current display gamma. */
void SetDisplayGamma(FLOAT Gamma)
{
	GEngine->Client->DisplayGamma = Gamma;
}

/** Saves the current display gamma. */
void SaveDisplayGamma()
{
	GEngine->Client->SaveConfig();
}

