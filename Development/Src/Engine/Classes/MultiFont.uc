/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * A font class that stores multiple font pages for different resolutions
 */

class MultiFont extends Font
	native;
	
/** Holds a list of resolutions that map to a given set of font pages */
var() editinline  array<float> ResolutionTestTable;	

cpptext
{
	void Serialize( FArchive& Ar );
	virtual INT GetResolutionPageIndex(FLOAT HeightTest);
	virtual FLOAT GetScalingFactor(FLOAT HeightTest);
}
	
native function int GetResolutionTestTableIndex(float HeightTest);
	
defaultproperties
{
}
