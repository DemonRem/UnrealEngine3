/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// CascadeOptions
//
// A configuration class that holds information for the setup of Cascade.
// Supplied so that the editor 'remembers' the last setup the user had.
//=============================================================================
class CascadeOptions extends Object	
	hidecategories(Object)
	config(Editor)
	native;	

var(Options)		config bool			bShowModuleDump;
var(Options)		config color		BackgroundColor;
var(Options)		config bool			bUseSubMenus;
var(Options)		config bool			bUseSpaceBarReset;
var(Options)		config color		Empty_Background;
var(Options)		config color		Emitter_Background;
var(Options)		config color		Emitter_Unselected;
var(Options)		config color		Emitter_Selected;
var(Options)		config color		ModuleColor_General_Unselected;
var(Options)		config color		ModuleColor_General_Selected;
var(Options)		config color		ModuleColor_TypeData_Unselected;
var(Options)		config color		ModuleColor_TypeData_Selected;
var(Options)		config color		ModuleColor_Beam_Unselected;
var(Options)		config color		ModuleColor_Beam_Selected;
var(Options)		config color		ModuleColor_Trail_Unselected;
var(Options)		config color		ModuleColor_Trail_Selected;

var(Options)		config bool			bShowGrid;
var(Options)		config color		GridColor_Hi;
var(Options)		config color		GridColor_Low;
var(Options)		config float		GridPerspectiveSize;

var(Options)		config bool			bShowParticleCounts;
var(Options)		config bool			bShowParticleTimes;
var(Options)		config bool			bShowParticleDistance;

var(Options)		config bool			bShowFloor;
var(Options)		config string		FloorMesh;
var(Options)		config vector		FloorPosition;
var(Options)		config rotator		FloorRotation;
var(Options)		config float		FloorScale;
var(Options)		config vector		FloorScale3D;

var(Options)		config string		PostProcessChainName;

var(Options)		config int			ShowPPFlags;

defaultproperties
{
}
