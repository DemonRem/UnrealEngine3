/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class Route extends Info
	placeable
	native;

cpptext
{
	void AutoFillRoute( ERouteFillAction RFA, TArray<ANavigationPoint*>& Points );
	virtual void GetNavReferences(TArray<FNavReference*> &NavRefs, UBOOL bIsRemovingLevel);
	virtual void CheckForErrors();
}

enum ERouteFillAction
{
	RFA_Overwrite,
	RFA_Add,
	RFA_Remove,
	RFA_Clear,
};
enum ERouteDirection
{
	ERD_Forward,
	ERD_Reverse,
};
enum ERouteType
{
	/** Move from beginning to end, then stop */
	ERT_Linear,
	/** Move from beginning to end and then reverse */
	ERT_Loop,
	/** Move from beginning to end, then start at beginning again */
	ERT_Circle,
};
var() ERouteType RouteType;

/** List of move targets in order */
var() array<NavReference> NavList;

defaultproperties
{
	Begin Object Name=Sprite
		Sprite=Texture2D'EngineResources.S_Route'
	End Object
	Components.Add(Sprite)

	Begin Object Class=RouteRenderingComponent Name=RouteRenderer
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(RouteRenderer)

	bStatic=TRUE
}
