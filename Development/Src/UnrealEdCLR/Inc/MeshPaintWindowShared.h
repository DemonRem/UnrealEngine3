/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MeshPaintWindowShared_h__
#define __MeshPaintWindowShared_h__

#ifdef _MSC_VER
	#pragma once
#endif


// NOTE: This file is included as MANAGED for CLR .cpp files, but UNMANAGED for the rest of the app

#include "InteropShared.h"



#ifdef __cplusplus_cli


// ... MANAGED ONLY definitions go here ...

ref class MMeshPaintWindow;
ref class MWPFFrame;


#else // #ifdef __cplusplus_cli


// ... NATIVE ONLY definitions go here ...


#endif // #else




/**
 * Mesh Paint window class (shared between native and managed code)
 */
class FMeshPaintWindow
	: public FCallbackEventDevice
{

public:

	/** Static: Allocate and initialize mesh paint window */
	static FMeshPaintWindow* CreateMeshPaintWindow( class FEdModeMeshPaint* InMeshPaintSystem );


public:

	/** Constructor */
	FMeshPaintWindow();

	/** Destructor */
	virtual ~FMeshPaintWindow();

	/** Initialize the mesh paint window */
	UBOOL InitMeshPaintWindow( FEdModeMeshPaint* InMeshPaintSystem );

	/** Refresh all properties */
	void RefreshAllProperties();

	/** Saves window settings to the Mesh Paint settings structure */
	void SaveWindowSettings();

	/** Returns true if the mouse cursor is over the mesh paint window */
	UBOOL IsMouseOverWindow();

	/** FCallbackEventDevice: Called when a parameterless global event we've registered for is fired */
	void Send( ECallbackEventType Event );


protected:

	/** Managed reference to the actual window control */
	//AutoGCRoot( MMeshPaintWindow^ ) WindowControl;
	AutoGCRoot( MWPFFrame^ ) MeshPaintFrame;
	AutoGCRoot( MMeshPaintWindow^ ) MeshPaintPanel;

	FPickColorStruct PaintColorStruct;
	FPickColorStruct EraseColorStruct;
};



#endif	// __MeshPaintWindowShared_h__