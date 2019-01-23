/**
 * TextureRenderTarget2D
 *
 * 2D render target texture resource. This can be used as a target
 * for rendering as well as rendered as a regular 2D texture resource.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class TextureRenderTarget2D extends TextureRenderTarget
	native
	hidecategories(Object)
	hidecategories(Texture);

/** The width of the texture.												*/
var() const int SizeX;

/** The height of the texture.												*/
var() const int SizeY;

/** The format of the texture data.											*/
var const EPixelFormat Format;

/** the color the texture is cleared to */
var const LinearColor ClearColor;

/** The addressing mode to use for the X axis.								*/
var() TextureAddress AddressX;

/** The addressing mode to use for the Y axis.								*/
var() TextureAddress AddressY;

cpptext
{
	/**
	* Initialize the settings needed to create a render target texture
	* and create its resource
	* @param	InSizeX - width of the texture
	* @param	InSizeY - height of the texture
	* @param	InFormat - format of the texture
	*/
	void Init(UINT InSizeX, UINT InSizeY, EPixelFormat InFormat);

	/**
	* Utility for creating a new UTexture2D from a TextureRenderTarget2D
	* TextureRenderTarget2D must be square and a power of two size.
	* @param	Outer - Outer to use when constructing the new Texture2D.
	* @param	NewTexName	-Name of new UTexture2D object.
	* @return	New UTexture2D object.
	*/
	UTexture2D* ConstructTexture2D(UObject* Outer, const FString& NewTexName, EObjectFlags InFlags);

	// USurface interface

	/**
	* @return width of surface
	*/
	virtual FLOAT GetSurfaceWidth() const { return SizeX; }

	/**
	* @return height of surface
	*/
	virtual FLOAT GetSurfaceHeight() const { return SizeY; }

	// UTexture interface.

	/**
	* Create a new 2D render target texture resource
	* @return newly created FTextureRenderTarget2DResource
	*/
	virtual FTextureResource* CreateResource();

	/**
	* Materials should treat a render target 2D texture like a regular 2D texture resource.
	* @return EMaterialValueType for this resource
	*/
	virtual EMaterialValueType GetMaterialType();

	// UObject interface

	/**
	* Called when any property in this object is modified in UnrealEd
	* @param	PropertyThatChanged - changed property
	*/
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/**
	* Called after the object has been loaded
	*/
	virtual void PostLoad();

	// Editor thumbnail interface.

	/**
	 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
	 */
	virtual FString GetDesc();

	/**
	 * Returns detailed info to populate listview columns
	 */
	virtual FString GetDetailedDescription( INT InIndex );

	/**
	 * Serialize properties (used for backwards compatibility with main branch)
	 */
	virtual void Serialize(FArchive& Ar);
}

/** creates and initializes a new TextureRenderTarget2D with the requested settings */
static native noexport final function TextureRenderTarget2D Create(int InSizeX, int InSizeY, optional EPixelFormat InFormat = PF_A8R8G8B8, optional LinearColor InClearColor);

defaultproperties
{
	// must be a supported format
	Format=PF_A8R8G8B8

	ClearColor=(R=0.0,G=1.0,B=0.0,A=1.0)
}
