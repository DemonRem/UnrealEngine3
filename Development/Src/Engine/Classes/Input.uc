//=============================================================================
// Input
// Object that maps key events to key bindings
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class Input extends Interaction
	native(UserInterface)
	config(Input)
    transient;

struct native KeyBind
{
	var config name		Name;
	var config string	Command;
	var config bool		Control,
						Shift,
						Alt;
structcpptext
{
	FKeyBind()
	: Name()
	, Control(0)
	, Shift(0)
	, Alt(0)
	{}
}
};

var config array<KeyBind>				Bindings;

/** list of keys which this interaction handled a pressed event for */
var const array<name>					PressedKeys;

var const EInputEvent					CurrentEvent;
var const float							CurrentDelta;
var const float							CurrentDeltaTime;

var native const Map{FName,void*}		NameToPtr;
var native const init array<pointer>	AxisArray{FLOAT};

cpptext
{
	// UInteraction interface.

	virtual UBOOL InputKey(INT ControllerId, FName Key, enum EInputEvent Event, FLOAT AmountDepressed = 1.f, UBOOL bGamepad = FALSE );
	virtual UBOOL InputAxis(INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad=FALSE);
	virtual void Tick(FLOAT DeltaTime);
	UBOOL IsPressed( FName InKey ) const;
	UBOOL IsCtrlPressed() const;
	UBOOL IsShiftPressed() const;
	UBOOL IsAltPressed() const;

	// UInput interface.
	virtual UBOOL Exec(const TCHAR* Cmd,FOutputDevice& Ar);
	virtual void ResetInput();

	/**
	 * Clears the PressedKeys array.  Should be called when another interaction which swallows some (but perhaps not all) input is activated.
	 */
	virtual void FlushPressedKeys()
	{
		PressedKeys.Empty();
	}

	// Protected.

	BYTE* FindButtonName(const TCHAR* ButtonName);
	FLOAT* FindAxisName(const TCHAR* ButtonName);
	FString GetBind(FName Key) const;
	/**
	 * Returns the Name of a bind using the bind's Command as the key
	 * StartBind Index is where the search will start from and where the index result will be stored
	 *   -- If you don't where to start your search from (as the list will search backwards), set the StartBindIndex to -1 before passing it in
	 */
	FString GetBindNameFromCommand(const FString& KeyCommand, INT* StartBindIndex = NULL ) const;
	void ExecInputCommands(const TCHAR* Cmd,class FOutputDevice& Ar);
	virtual void UpdateAxisValue( FLOAT* Axis, FLOAT Delta );
}

exec function SetBind(name BindName,string Command)
{
	local KeyBind	NewBind;
	local int		BindIndex;

	for(BindIndex = Bindings.Length-1;BindIndex >= 0;BindIndex--)
		if(Bindings[BindIndex].Name == BindName)
		{
			Bindings[BindIndex].Command = Command;
			SaveConfig();
			return;
		}

	NewBind.Name = BindName;
	NewBind.Command = Command;
	Bindings[Bindings.Length] = NewBind;
	SaveConfig();
}
