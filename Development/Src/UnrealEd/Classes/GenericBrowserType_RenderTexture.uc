/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// GenericBrowserType_RenderTexture: Render target textures
//=============================================================================

class GenericBrowserType_RenderTexture
	extends GenericBrowserType_Texture
	native;

cpptext
{
	virtual void Init();
	virtual void InvokeCustomCommand( INT InCommand, UObject* InObject );
}
	
defaultproperties
{
	Description="RenderToTexture"
}
