/**
 * Base class for all Kismet related objects.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SequenceObject extends Object
	native(Sequence)
	abstract
	hidecategories(Object);

cpptext
{
	virtual void CheckForErrors() {};
	virtual void OnExport();

	/**
	 * Notification that this object has been connected to another sequence object via a link.  Called immediately after
	 * the designer creates a link between two sequence objects.
	 *
	 * @param	connObj		the object that this op was just connected to.
	 * @param	connIdx		the index of the connection that was created.  Depends on the type of sequence op that is being connected.
	 */
	virtual void OnConnect(USequenceObject *connObj,INT connIdx) {}

	// USequenceObject interface
	virtual void DrawSeqObj(FCanvas* Canvas, UBOOL bSelected, UBOOL bMouseOver, INT MouseOverConnType, INT MouseOverConnIndex, FLOAT MouseOverTime) {};
	virtual void DrawLogicLinks(FCanvas* Canvas, UBOOL bCurves, TArray<USequenceObject*> &SelectedSeqObjs, USequenceObject* MouseOverSeqObj, INT MouseOverConnType, INT MouseOverConnIndex) {};
	virtual void DrawVariableLinks(FCanvas* Canvas, UBOOL bCurves, TArray<USequenceObject*> &SelectedSeqObjs, USequenceObject* MouseOverSeqObj, INT MouseOverConnType, INT MouseOverConnIndex) {};
	virtual void OnCreated()
	{
		ObjInstanceVersion = ObjClassVersion;
	};
	virtual void OnSelected() {};

	/**
	 * Called when a copy of this object is made in the editor via cut and paste
	 */
	virtual void OnPasted(){};

	virtual FIntRect GetSeqObjBoundingBox();
	void SnapPosition(INT Gridsize, INT MaxSequenceSize);
	FString GetSeqObjFullName();
	USequence* GetRootSequence();

	FIntPoint GetTitleBarSize(FCanvas* Canvas);
	FColor GetBorderColor(UBOOL bSelected, UBOOL bMouseOver);

	void DrawTitleBar(FCanvas* Canvas, UBOOL bSelected, UBOOL bMouseOver, const FIntPoint& Pos, const FIntPoint& Size);

	virtual void UpdateObject()
	{
		// set the new instance version to match the class version
		ObjInstanceVersion = ObjClassVersion;
		MarkPackageDirty();
	}

	virtual void DrawKismetRefs( FViewport* Viewport, const FSceneView* View, FCanvas* Canvas ) {}

	virtual void PostLoad();
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/** Get the name of the class used to help out when handling events in UnrealEd.
	 * @return	String name of the helper class.
	 */
	virtual const FString GetEdHelperClassName() const
	{
		return FString( TEXT("UnrealEd.SequenceObjectHelper") );
	}

	virtual UBOOL IsPendingKill() const;

	/**
	 * Returns whether this SequenceObject can exist in a sequence without being linked to anything else (i.e. does not require
	 * another sequence object to activate it)
	 */
	virtual UBOOL IsStandalone() const { return FALSE; }

	/** called when the level that contains this sequence object is being removed/unloaded */
	virtual void CleanUp() {}

	/**
	 * Determines whether this object is contained within a UPrefab.
	 *
	 * @param	OwnerPrefab		if specified, receives a pointer to the owning prefab.
	 *
	 * @return	TRUE if this object is contained within a UPrefab; FALSE if it IS a UPrefab or isn't contained within one.
	 */
	virtual UBOOL IsAPrefabArchetype( class UObject** OwnerPrefab=NULL ) const;

	/**
	 * @return	TRUE if the object is a UPrefabInstance or part of a prefab instance.
	 */
	virtual UBOOL IsInPrefabInstance() const;
}

/** Class vs instance version, for offering updates in the Kismet editor */
var const int ObjClassVersion, ObjInstanceVersion;

/** Sequence that contains this object */
var const noimport Sequence ParentSequence;

/** Visual position of this object within a sequence */
var		int						ObjPosX, ObjPosY;

/** Text label that describes this object */
var		string					ObjName;

/**
 * Editor category for this object.  Determines which kismet submenu this object
 * should be placed in
 */
var 	string					ObjCategory;

/** Color used to draw the object */
var		color					ObjColor;

/** User editable text comment */
var()	string					ObjComment;

/** Whether or not this object is deletable. */
var		bool					bDeletable;

/** Should this object be drawn in the first pass? */
var		bool					bDrawFirst;

/** Should this object be drawn in the last pass? */
var		bool					bDrawLast;

/** Cached drawing dimensions */
var		int						DrawWidth, DrawHeight;

/** Should this object display ObjComment when activated? */
var()	bool					bOutputObjCommentToScreen;

/** Should we suppress the 'auto' comment text - values of properties flagged with the 'autocomment' metadata string. */
var()	bool					bSuppressAutoComment;

/** Writes out the specified text to a dedicated scripting log file.
 * @param LogText the text to print
 * @param bWarning true if this is a warning message.
 * 	Warning messages are also sent to the normal game log and appear onscreen if Engine's configurable bOnScreenKismetWarnings is true
 */
native final function ScriptLog(string LogText, optional bool bWarning = true);

/** Returns the current world's WorldInfo, useful for spawning actors and such. */
native final function WorldInfo GetWorldInfo();

/**
 * Determines whether this class should be displayed in the list of available ops in the level kismet editor.
 *
 * @return	TRUE if this sequence object should be available for use in the level kismet editor
 */
event bool IsValidLevelSequenceObject()
{
	return true;
}

/**
 * Determines whether this class should be displayed in the list of available ops in the UI's kismet editor.
 *
 * @param	TargetObject	the widget that this SequenceObject would be attached to.
 *
 * @return	TRUE if this sequence object should be available for use in the UI kismet editor
 */
event bool IsValidUISequenceObject( optional UIScreenObject TargetObject )
{
	return false;
}

defaultproperties
{
	bDeletable=true
	ObjClassVersion=1
	ObjName="Undefined"
	ObjColor=(R=255,G=255,B=255,A=255)
	bSuppressAutoComment=true
}
