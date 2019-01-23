/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialEditorMeshComponent extends StaticMeshComponent
	native;

cpptext
{
protected:
	// ActorComponent interface.
	virtual void Attach();
	virtual void Detach();
}

var transient native const pointer	MaterialEditor;
