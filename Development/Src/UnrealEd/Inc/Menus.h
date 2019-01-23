/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MENUS_H__
#define __MENUS_H__

/**
 * Baseclass for menus created when right clicking in the main editor viewports.
 */
class WxMainContextMenuBase : public wxMenu
{	
public:
	wxMenu* ActorFactoryMenu;
	wxMenu* ActorFactoryMenuAdv;
	wxMenu* RecentClassesMenu;

	WxMainContextMenuBase();

	void AppendAddActorMenu();
};

/**
 * Main level viewport context menu.
 */
class WxMainContextMenu : public WxMainContextMenuBase
{
public:
	WxMainContextMenu();

private:
	wxMenu* OrderMenu;
	wxMenu* PolygonsMenu;
	wxMenu* CSGMenu;
	wxMenu* SolidityMenu;
	wxMenu* SelectMenu;
	wxMenu* AlignMenu;
	wxMenu* PivotMenu;
	wxMenu* TransformMenu;
	wxMenu* VolumeMenu;
	wxMenu* PathMenu;
	wxMenu* ComplexPathMenu;
};

/**
 * Main level viewport context menu for BSP surfaces.
 */
class WxMainContextSurfaceMenu : public WxMainContextMenuBase
{
public:
	WxMainContextSurfaceMenu();

private:
	wxMenu* SelectSurfMenu;
	wxMenu* ExtrudeMenu;
	wxMenu* AlignmentMenu;
};

/**
 * Main level viewport context menu for cover slots.
 */
class WxMainContextCoverSlotMenu : public WxMainContextMenuBase
{
public:
	WxMainContextCoverSlotMenu(class ACoverLink *Link, struct FCoverSlot &Slot);
};

#endif // __MENUS_H__
