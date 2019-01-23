/*=============================================================================
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "D3DDrvPrivate.h"
#include "D3DMeshUtils.h"

#if USE_D3D_RHI

#define SAFE_RELEASE(x) { if ((x)) { (x)->Release(); (x) = NULL; } }

/**
* ConstructD3DVertexElement - acts as a constructor for D3DVERTEXELEMENT9
*/
D3DVERTEXELEMENT9 FD3DMeshUtilities :: ConstructD3DVertexElement(WORD Stream, WORD Offset, BYTE Type, BYTE Method, BYTE Usage, BYTE UsageIndex)
{
	D3DVERTEXELEMENT9 newElement;
	newElement.Stream = Stream;
	newElement.Offset = Offset;
	newElement.Type = Type;
	newElement.Method = Method;
	newElement.Usage = Usage;
	newElement.UsageIndex = UsageIndex;
	return newElement;
}


/**
* Creates a D3DXMESH from a FStaticMeshRenderData
* @param SourceMesh Mesh to extract from
* @param D3DMesh Mesh to create
* @return Boolean representing success or failure
*/
UBOOL FD3DMeshUtilities :: ConvertUMeshToD3DXMesh(FStaticMeshRenderData& SourceMesh, LPD3DXMESH& D3DMesh)
{
	TArray<D3DVERTEXELEMENT9> VertexElements;
	VertexElements.Push(ConstructD3DVertexElement(0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0));
	VertexElements.Push(ConstructD3DVertexElement(0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0));
	VertexElements.Push(ConstructD3DVertexElement(0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1));
	VertexElements.Push(ConstructD3DVertexElement(0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2));
	VertexElements.Push(ConstructD3DVertexElement(0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3));
	VertexElements.Push(ConstructD3DVertexElement(0, 44, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0));
	VertexElements.Push(ConstructD3DVertexElement(0, 48, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1));
	VertexElements.Push(ConstructD3DVertexElement(0xFF,0,D3DDECLTYPE_UNUSED,0,0,0));

	// This code uses the raw triangles. Needs welding, etc.
	INT NumFaces = SourceMesh.RawTriangles.GetElementCount();
	INT NumVertices = SourceMesh.RawTriangles.GetElementCount()*3;
	// Create mesh for source data
	if (FAILED(D3DXCreateMesh(NumFaces,NumVertices,D3DXMESH_SYSTEMMEM,(D3DVERTEXELEMENT9 *)VertexElements.GetData(),GDirect3DDevice,&D3DMesh) ) )
	{
		warnf(TEXT("D3DXCreateMesh() Failed!"));
		return FALSE;
	}

	// Fill D3DMesh mesh
	FUtilVertex* D3DVertices;
	WORD*		 D3DIndices;
	DWORD*		 D3DAttributes;
	D3DMesh->LockVertexBuffer(0,(LPVOID*)&D3DVertices);
	D3DMesh->LockIndexBuffer(0,(LPVOID*)&D3DIndices);
	D3DMesh->LockAttributeBuffer(0, &D3DAttributes);

	// Build straight indices
	for(INT I=0;I<SourceMesh.RawTriangles.GetElementCount();I++)
	{
		D3DIndices[I*3 + 0] = I*3 + 0;
		D3DIndices[I*3 + 1] = I*3 + 1;
		D3DIndices[I*3 + 2] = I*3 + 2;
	}

	UINT NumVerts = 0;
	const FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) SourceMesh.RawTriangles.Lock(LOCK_READ_ONLY);
	for(INT I=0;I<SourceMesh.RawTriangles.GetElementCount();I++)
	{
		const FStaticMeshTriangle& Tri = RawTriangleData[I];
		D3DAttributes[I] = Tri.MaterialIndex;
		for(INT J=0;J<3;J++)
		{
			D3DVertices[NumVerts].Position = Tri.Vertices[J];
			D3DVertices[NumVerts].Color = Tri.Colors[J];
			//store the smoothing mask per vertex since there is only one per-face attribute that is already being used (materialIndex)
			D3DVertices[NumVerts].SmoothingMask = FColor(Tri.SmoothingMask);
			for(INT UVIndex = 0; UVIndex < Tri.NumUVs; UVIndex++)
			{
				D3DVertices[NumVerts].UVs[UVIndex] = Tri.UVs[J][UVIndex];
			}
			NumVerts++;
		}
	}
	SourceMesh.RawTriangles.Unlock();

	D3DMesh->UnlockIndexBuffer();
	D3DMesh->UnlockVertexBuffer();
	D3DMesh->UnlockAttributeBuffer();

	return TRUE;
}

/**
* Creates a FStaticMeshRenderData from a D3DXMesh
* @param DestMesh Destination mesh to extract to
* @param NumUVs Number of UVs
* @param Elements Elements array
* @return Boolean representing success or failure
*/
UBOOL FD3DMeshUtilities :: ConvertD3DXMeshToUMesh(
	LPD3DXMESH& D3DMesh, 									  
	FStaticMeshRenderData& DestMesh, 				  
	INT NumUVs, 
	TArray<FStaticMeshElement>& Elements, 
	UBOOL bClearDestElements)
{

	//DestMesh.RawTriangles.RemoveBulkData();
	//DestMesh.Elements.Empty();

	// Extract simplified data to LOD
	FUtilVertex* D3DVertices;
	WORD*		 D3DIndices;
	DWORD*		 D3DAttributes;
	D3DMesh->LockVertexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DVertices);
	D3DMesh->LockIndexBuffer(D3DLOCK_READONLY,(LPVOID*)&D3DIndices);
	D3DMesh->LockAttributeBuffer(D3DLOCK_READONLY,&D3DAttributes);

	DestMesh.RawTriangles.Lock(LOCK_READ_WRITE);
	FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) DestMesh.RawTriangles.Realloc( D3DMesh->GetNumFaces() );

	for(UINT I=0;I<D3DMesh->GetNumFaces();I++)
	{
		FStaticMeshTriangle& Tri = RawTriangleData[I];
		// Copy smoothing mask and index from any vertex into this triangle
		Tri.SmoothingMask = D3DVertices[D3DIndices[I*3+0]].SmoothingMask.DWColor();
		Tri.MaterialIndex = D3DAttributes[I];

		Tri.NumUVs = NumUVs;
		for(int UVs=0;UVs<Tri.NumUVs;UVs++)
		{
			Tri.UVs[0][UVs] = D3DVertices[D3DIndices[I*3+0]].UVs[UVs];
			Tri.UVs[1][UVs] = D3DVertices[D3DIndices[I*3+1]].UVs[UVs];
			Tri.UVs[2][UVs] = D3DVertices[D3DIndices[I*3+2]].UVs[UVs];
		}

		for(INT K=0;K<3;K++)
		{
			Tri.Vertices[K]   = D3DVertices[D3DIndices[I*3+K]].Position;
			Tri.Colors[K]   = D3DVertices[D3DIndices[I*3+K]].Color;
		}
	}

	DestMesh.RawTriangles.Unlock();

	DestMesh.Elements = Elements;
	if (bClearDestElements)
	{
		for(INT I=0;I<DestMesh.Elements.Num();I++)
		{
			DestMesh.Elements(I).MaxVertexIndex = 0;
			DestMesh.Elements(I).MinVertexIndex = 0;
			DestMesh.Elements(I).NumTriangles   = 0;
			DestMesh.Elements(I).FirstIndex     = 0;
		}
	}

	D3DMesh->UnlockIndexBuffer();
	D3DMesh->UnlockVertexBuffer();
	D3DMesh->UnlockAttributeBuffer();

	//done with the D3DXMesh
	SAFE_RELEASE(D3DMesh)

	return TRUE;
}

/**
* Generates a simplified LOD from a static mesh
* @param StaticMesh - The input/output mesh
* @param DesiredLOD - The LOD level to generate the simplified mesh for
* @param DesiredTriangles - The desired triangle count for the LOD. Resulting triangle count may not be desired triangle count.
* @return TRUE if successful
*/
UBOOL FD3DMeshUtilities :: GenerateLOD(UStaticMesh* StaticMesh, INT DesiredLOD, INT DesiredTriangles)
{ 

	FStaticMeshRenderData& BaseLOD = StaticMesh->LODModels(0);
	LPD3DXMESH SourceMesh;
	if (!ConvertUMeshToD3DXMesh(BaseLOD,SourceMesh))
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't convert to a D3DXMesh."));
		return FALSE;
	}

	DWORD * Adjacency = new DWORD[SourceMesh->GetNumFaces() * 3];

	if (FAILED(SourceMesh->GenerateAdjacency(1e-6f,Adjacency)))
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't generate adjacency info."));
		return FALSE;
	}
	//Clean the mesh, which allows it to be simplified more effectively.  
	LPD3DXMESH TempMesh;

	DWORD* NewAdjacency = new DWORD[SourceMesh->GetNumFaces() * 3];


	if( FAILED(D3DXCleanMesh( D3DXCLEAN_SIMPLIFICATION, SourceMesh, Adjacency, &TempMesh, 
		NewAdjacency, NULL ) ) )
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't clean mesh."));
		return FALSE;
	}
	delete [] Adjacency;

	SAFE_RELEASE(SourceMesh)
	
	SourceMesh = TempMesh;

	// Perform a weld to try and remove excess vertices.
	// Weld the mesh using all epsilons of 1e-6

	if( FAILED(D3DXWeldVertices( SourceMesh, 0, NULL,
		NewAdjacency,NewAdjacency, NULL, NULL ) ) )
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't weld vertices."));
		return FALSE;
	}

	D3DXATTRIBUTEWEIGHTS d3daw;
	appMemzero( &d3daw, sizeof(D3DXATTRIBUTEWEIGHTS) );
	d3daw.Position = 1.0f;
	d3daw.Boundary = 10000.0f;
	d3daw.Normal   = 1.0f;

	//Use the D3DX simplify functionality
	LPD3DXMESH NewMesh;

	if( FAILED(D3DXSimplifyMesh( TempMesh, NewAdjacency, &d3daw, NULL, DesiredTriangles , D3DXMESHSIMP_FACE, &NewMesh )))
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't simplify mesh."));
		return FALSE;
	}

	UINT NewTriCount = NewMesh->GetNumFaces();

	SAFE_RELEASE(TempMesh);

	SourceMesh = NewMesh;

	delete [] NewAdjacency;

	// Add dummy LODs if the LOD being inserted is not the next one in the array
	while(StaticMesh->LODModels.Num() <= DesiredLOD)
	{
		// This LOD will be a new structure
		if(StaticMesh->LODModels.Num() <= DesiredLOD)
		{
			new(StaticMesh->LODModels) FStaticMeshRenderData();
		}
		// Any others will be dummies
		else
		{
			StaticMesh->LODModels.AddRawItem(0); 
		}
	}

	// Add dummy LODs if the LOD being inserted is not the next one in the array
	while(StaticMesh->LODInfo.Num() <= DesiredLOD)
	{
		StaticMesh->LODInfo.AddItem(FStaticMeshLODInfo());
	}

	FStaticMeshRenderData& NewLOD = StaticMesh->LODModels(DesiredLOD);
	if (!ConvertD3DXMeshToUMesh(SourceMesh,NewLOD,BaseLOD.VertexBuffer.GetNumTexCoords(),BaseLOD.Elements, TRUE))
	{
		appDebugMessagef(TEXT("GenerateLOD failed, couldn't convert the simplified D3DXMesh back to a UStaticMesh."));
		return FALSE;
	}
	// Re-build the static mesh
	// todo - only rebuild the new LOD

	StaticMesh->Build();

	return TRUE; 
}

/**
* Generates a unique UV layout for a static mesh
* @param StaticMesh - The input/output mesh
* @param LODIndex - The LOD level
* @param TexCoordIndex - Index of the uv channel to overwrite or create
* @param MaxDesiredStretch - The amount of stretching allowed. 0 means no stretching is allowed, 1 means any amount of stretching can be used. 
* @param ChartsGenerated - Number of charts generated by the uv unwrap algorithm.
* @return TRUE if successful
*/
UBOOL FD3DMeshUtilities :: GenerateUVs(UStaticMesh* StaticMesh, UINT LODIndex, UINT TexCoordIndex, FLOAT &MaxDesiredStretch, UINT &ChartsGenerated)
{
	FStaticMeshRenderData& LOD = StaticMesh->LODModels(LODIndex);
	LPD3DXMESH SourceMesh;
	//convert from UStaticMesh to ID3DXMesh
	if (!ConvertUMeshToD3DXMesh(LOD,SourceMesh))
	{
		appDebugMessagef(TEXT("GenerateUVs failed, couldn't convert to a D3DXMesh."));
		return FALSE;
	}

	DWORD * Adjacency = new DWORD[SourceMesh->GetNumFaces() * 3];
	//generate adjacency info for the mesh, which is needed later
	if (FAILED(SourceMesh->GenerateAdjacency(1e-6f,Adjacency)))
	{
		appDebugMessagef(TEXT("GenerateUVs failed, couldn't generate adjacency info."));
		return FALSE;
	}
	
	LPD3DXMESH TempMesh;
	//clean the mesh
	DWORD* CleanedAdjacency = new DWORD[SourceMesh->GetNumFaces() * 3];
	if( FAILED(D3DXCleanMesh( D3DXCLEAN_SIMPLIFICATION, SourceMesh, Adjacency, &TempMesh, 
		CleanedAdjacency, NULL ) ) )
	{
		appDebugMessagef(TEXT("GenerateUVs failed, couldn't clean mesh."));
		return FALSE;
	}
	delete [] Adjacency;

	SAFE_RELEASE(SourceMesh);
	SourceMesh = TempMesh;

	// Input parameters
	float	 Maxcharts = 0;
	float	Gutter = 2.0f;
	
	//generate unique uv's
	//size params are only used to determine units of Gutter parameter
	HRESULT hr = D3DXUVAtlasCreate(SourceMesh, Maxcharts, MaxDesiredStretch, 256.0f, 256.0f, Gutter, 
		TexCoordIndex, CleanedAdjacency, NULL, NULL, NULL, 0.0f, NULL, D3DXUVATLAS_DEFAULT, &TempMesh, NULL,
		NULL, &MaxDesiredStretch, &ChartsGenerated);

	if (FAILED(hr))
	{
		warnf(*FString::Printf(TEXT("D3DXUVAtlasCreate() returned %u with MaxDesiredStretch=%.2f, TexCoordIndex=%u."), hr, MaxDesiredStretch, TexCoordIndex));
		appDebugMessagef(TEXT("GenerateUVs failed, D3DXUVAtlasCreate failed."));
		return FALSE;
	}

	SAFE_RELEASE(SourceMesh);
	SourceMesh = TempMesh;
	delete [] CleanedAdjacency;

	INT NewNumTexCoords = LOD.VertexBuffer.GetNumTexCoords();
	//if the selected index doesn't exist yet, create it
	if (TexCoordIndex == LOD.VertexBuffer.GetNumTexCoords())
	{
		NewNumTexCoords++;
	}

	//convert back to UStaticMesh
	if (!ConvertD3DXMeshToUMesh(SourceMesh, LOD, NewNumTexCoords, LOD.Elements, FALSE))
	{
		appDebugMessagef(TEXT("GenerateUVs failed, couldn't convert the simplified D3DXMesh back to a UStaticMesh."));
		return FALSE;
	}
	// Re-build the static mesh
	// todo - only rebuild the altered LOD
	StaticMesh->Build();

	return TRUE; 

}


#endif // USE_D3D_RHI

