/**
 * This viewport input class is used by viewports in 2D object editor windows.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class ObjectEditorViewportInput extends EditorViewportInput
	transient
	native(Private)
	config(Input);

cpptext
{
	virtual UBOOL Exec(const TCHAR* Cmd,FOutputDevice& Ar);
}


