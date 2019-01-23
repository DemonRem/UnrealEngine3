/**
 * Copyright 2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// GenericBrowserType_SoundWave: SoundWaves
//=============================================================================

class GenericBrowserType_SpeechRecognition
	extends GenericBrowserType
	native;

cpptext
{
	virtual void Init();
	virtual void InvokeCustomCommand( INT InCommand, UObject* InObject );
}
	
defaultproperties
{
	Description="Speech Recognition"
}
