/*=============================================================================
	D3DViewport.h: D3D viewport RHI definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if USE_D3D_RHI

typedef TD3DRef<class FD3DViewport> FViewportRHIRef;

class FD3DViewport : public FD3DRefCountedObject
{
public:

	FD3DViewport(void* InWindowHandle,UINT InSizeX,UINT InSizeY,UBOOL bInIsFullscreen);
	~FD3DViewport();

	void Resize(UINT InSizeX,UINT InSizeY,UBOOL bInIsFullscreen);

	// Accessors.
	void* GetWindowHandle() const { return WindowHandle; }
	UINT GetSizeX() const { return SizeX; }
	UINT GetSizeY() const { return SizeY; }
	UBOOL IsFullscreen() const { return bIsFullscreen; }

	/**
	 * Returns a supported screen resolution that most closely matches input.
	 * @param Width - Input: Desired resolution width in pixels. Output: A width that the platform supports.
	 * @param Height - Input: Desired resolution height in pixels. Output: A height that the platform supports.
	 */
	static void GetSupportedResolution( UINT &Width, UINT &Height );

private:
	void* WindowHandle;
	UINT SizeX;
	UINT SizeY;
	UBOOL bIsFullscreen;
};

typedef FD3DViewport* FViewportRHIParamRef;

//
// Globals.
//

extern TD3DRef<IDirect3DDevice9> GDirect3DDevice;
extern UBOOL GIsRHIInitialized;

#endif

