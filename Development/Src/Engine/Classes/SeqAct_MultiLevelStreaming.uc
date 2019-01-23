/**
 * SeqAct_MultiLevelStreaming
 *
 * Kismet action exposing loading and unloading of multiple levels at once.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_MultiLevelStreaming extends SeqAct_LevelStreamingBase
	native(Sequence);

struct native LevelStreamingNameCombo
{
	/** Cached LevelStreaming object that is going to be loaded/ unloaded on request.	*/
	var		const LevelStreaming		Level;
	/** LevelStreaming object name.														*/
	var()	const Name					LevelName;
};

/** Array of levels to load/ unload														*/
var() array<LevelStreamingNameCombo>	Levels;

cpptext
{
	void Activated();
	UBOOL UpdateOp(FLOAT DeltaTime);
};

defaultproperties
{
	ObjName="Stream Multiple Levels"
}
