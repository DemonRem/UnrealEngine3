/*=============================================================================
	D3DQuery.cpp: D3D query RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI

FOcclusionQueryRHIRef RHICreateOcclusionQuery()
{
	FOcclusionQueryRHIRef OcclusionQuery;
	VERIFYD3DRESULT(GDirect3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION,OcclusionQuery.GetInitReference()));
	return OcclusionQuery;
}

UBOOL RHIGetOcclusionQueryResult(FOcclusionQueryRHIParamRef OcclusionQuery,DWORD& OutNumPixels,UBOOL bWait)
{
	while(1)
	{
		// Query occlusion query object.
		HRESULT Result = OcclusionQuery->GetData( &OutNumPixels, sizeof(OutNumPixels), D3DGETDATA_FLUSH );

		if(Result == S_FALSE)
		{
			// The query isn't finished yet...
			if(bWait)
			{
				// ...keep trying.
                continue;
			}
			else
			{
				// ...return failure.
				return FALSE;
			}
		}
		else if(Result == D3DERR_DEVICELOST)
		{
			// If the device is lost, return failure.
			GD3DDeviceLost = TRUE;
			OutNumPixels = 0;
			return FALSE;
		}
		else if(SUCCEEDED(Result))
		{
			return TRUE;
		}
	}
}

#endif