/*=============================================================================
	UnIn.cpp: Unreal input system.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"

IMPLEMENT_CLASS(UInput);
IMPLEMENT_CLASS(UPlayerInput);

//
//	UInput::FindButtonName - Find a button.
//
BYTE* UInput::FindButtonName( const TCHAR* ButtonName )
{
	FName Button( ButtonName, FNAME_Find );
	if( Button == NAME_None )
		return NULL;

	BYTE* Ptr = (BYTE*) NameToPtr.FindRef( Button );
	if( Ptr == NULL )
	{
		for(const UObject* Object = this;Object;Object = Object->GetOuter())
		{
			for( UProperty* Property = Object->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext )
			{
				if( (Property->PropertyFlags & CPF_Input) != 0 && Property->GetFName()==Button && Property->IsA(UByteProperty::StaticClass()) )
				{
					Ptr = (BYTE*)Object + Property->Offset;
					NameToPtr.Set( Button, Ptr );
					return Ptr;
				}
			}
		}
	}

	return Ptr;
}

//
//	UInput::FindAxisName - Find an axis.
//
FLOAT* UInput::FindAxisName( const TCHAR* ButtonName )
{
	FName Button( ButtonName, FNAME_Find );
	if( Button == NAME_None )
		return NULL;

	FLOAT* Ptr = (FLOAT*) NameToPtr.FindRef( Button );
	if( Ptr == NULL )
	{
		for(const UObject* Object = this;Object;Object = Object->GetOuter())
		{
			for ( UProperty* Property = Object->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext )
			{
				if ( (Property->PropertyFlags&CPF_Input) != 0 && Property->GetFName() == Button && Property->IsA(UFloatProperty::StaticClass()) )
				{
					Ptr = (FLOAT*) ((BYTE*) Object + Property->Offset);
					NameToPtr.Set( Button, Ptr );
					return Ptr;
				}
			}
		}
	}

	return Ptr;
}

//
//	UInput::GetBind
//
FString UInput::GetBind(FName Key) const
{
	UBOOL	Control = PressedKeys.FindItemIndex(KEY_LeftControl) != INDEX_NONE || PressedKeys.FindItemIndex(KEY_RightControl) != INDEX_NONE,
			Shift = PressedKeys.FindItemIndex(KEY_LeftShift) != INDEX_NONE || PressedKeys.FindItemIndex(KEY_RightShift) != INDEX_NONE,
			Alt = PressedKeys.FindItemIndex(KEY_LeftAlt) != INDEX_NONE || PressedKeys.FindItemIndex(KEY_RightAlt) != INDEX_NONE;

	for(INT BindIndex = Bindings.Num()-1; BindIndex >= 0; BindIndex--)
	{
		const FKeyBind&	Bind = Bindings(BindIndex);
		if(Bind.Name == Key && (!Bind.Control || Control) && (!Bind.Shift || Shift) && (!Bind.Alt || Alt))
			return Bindings(BindIndex).Command;
	}

	return TEXT("");
}

/** Returns the Name of a bind using the bind's Command as the key */
FString UInput::GetBindNameFromCommand(const FString& KeyCommand, INT* StartBindIndex) const
{
	FString NameResult;
	// init the BindIndex based on whether one was passed in and whether it has a valid index assigned to it
	INT BindIndex = (!StartBindIndex || *StartBindIndex==-1) ? Bindings.Num()-1 : *StartBindIndex;

	// sanity check
	if ( BindIndex > -1 && BindIndex < Bindings.Num() )
	{
		// find the name using the command as the key
		for( ; BindIndex >= 0; BindIndex--)
		{
			const FKeyBind&	Bind = Bindings(BindIndex);
			// the command could be one of many so just see if the string is contained within the command
			if(Bind.Command.InStr(KeyCommand) != -1)
			{
				// we found one, so store it and break
				NameResult = Bindings(BindIndex).Name.ToString();
				break;
			}
		}
	}

	// didn't find a bind so set the variables appropriately
	if ( BindIndex < 0 || BindIndex >= Bindings.Num() )
	{
		NameResult = TEXT("");
		if (StartBindIndex)
		{
			*StartBindIndex = -1;
		}
	}
	// found a bind so set the index incase one was based in
	else
	{
		if (StartBindIndex)
		{
			*StartBindIndex = BindIndex;
		}
	}

	// return the name
	return NameResult;
}

// Checks to see if InKey is pressed down

UBOOL UInput::IsPressed( FName InKey ) const
{
	return ( PressedKeys.FindItemIndex( InKey ) != INDEX_NONE );
}

UBOOL UInput::IsCtrlPressed() const
{
	return ( IsPressed( KEY_LeftControl ) || IsPressed( KEY_RightControl ) );
}

UBOOL UInput::IsShiftPressed() const
{
	return ( IsPressed( KEY_LeftShift ) || IsPressed( KEY_RightShift ) );
}

UBOOL UInput::IsAltPressed() const
{
	return ( IsPressed( KEY_LeftAlt ) || IsPressed( KEY_RightAlt ) );
}

//
//	UInput::ExecInputCommands - Execute input commands.
//

void UInput::ExecInputCommands( const TCHAR* Cmd, FOutputDevice& Ar )
{
	INT CmdLen = appStrlen(Cmd);
	TCHAR* Line = (TCHAR*)appMalloc(sizeof(TCHAR)*(CmdLen+1));

	while( ParseLine( &Cmd, Line, CmdLen+1) )		// The ParseLine function expects the full array size, including the NULL character.
	{
		const TCHAR* Str = Line;
		if(CurrentEvent == IE_Pressed || (CurrentEvent == IE_Released && ParseCommand(&Str,TEXT("OnRelease"))))
		{
			APlayerController*	Actor = Cast<APlayerController>(GetOuter());

			if(ScriptConsoleExec(Str,Ar,this))
				continue;
			else if(Exec(Str,Ar))
				continue;
			else if(Actor && Actor->Player && Actor->Player->Exec(Str,Ar))
				continue;
		}
		else
			Exec(Str,Ar);
	}

	appFree(Line);
}

//
//	UInput::Exec - Execute a command.
//
UBOOL UInput::Exec(const TCHAR* Str,FOutputDevice& Ar)
{
	TCHAR Temp[256];
	static UBOOL InAlias=0;

	if( ParseCommand( &Str, TEXT("BUTTON") ) )
	{
		// Normal button.
		BYTE* Button;
		if( ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
		{
			if	( (Button=FindButtonName(Temp))!=NULL )
			{
				if( CurrentEvent == IE_Pressed )
					*Button = 1;
				else if( CurrentEvent == IE_Released && *Button )
					*Button = 0;
			}
			else Ar.Log( TEXT("Bad Button command") );
		}
		else Ar.Log( TEXT("Bad Button command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("PULSE") ) )
	{
		// Normal button.
		BYTE* Button;
		if( ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
		{
			if	( (Button=FindButtonName(Temp))!=NULL )
			{
				if( CurrentEvent == IE_Pressed )
					*Button = 1;
			}
			else Ar.Log( TEXT("Bad Button command") );
		}
		else Ar.Log( TEXT("Bad Button command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("TOGGLE") ) )
	{
		// Toggle button.
		BYTE* Button;
		if
		(	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 )
		&&	((Button=FindButtonName(Temp))!=NULL) )
		{
			if( CurrentEvent == IE_Pressed )
				*Button ^= 0x80;
		}
		else Ar.Log( TEXT("Bad Toggle command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("AXIS") ) )
	{
		// Axis movement.
		FLOAT* Axis;
		if(	
			ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) 
		&& (Axis=FindAxisName(Temp))!=NULL )
		{
			if( CurrentEvent == IE_Axis )
			{
				FLOAT	Speed			= 1.f, 
						DeadZone		= 0.f,
						AbsoluteAxis	= 0.f;
				INT		InvertMultiplier= 1;

				Parse( Str, TEXT("SPEED=")			, Speed				);
				Parse( Str, TEXT("INVERT=")			, InvertMultiplier	);
				Parse( Str, TEXT("DEADZONE=")		, DeadZone			);
				Parse( Str, TEXT("ABSOLUTEAXIS=")	, AbsoluteAxis		);

				// Axis is expected to be in -1 .. 1 range if dead zone is used.
				if( DeadZone > 0.f && DeadZone < 1.f )
				{
					// We need to translate and scale the input to the +/- 1 range after removing the dead zone.
					if( CurrentDelta > 0 )
					{
						CurrentDelta = Max( 0.f, CurrentDelta - DeadZone ) / (1.f - DeadZone);
					}
					else
					{
						CurrentDelta = -Max( 0.f, -CurrentDelta - DeadZone ) / (1.f - DeadZone);
					}
				}
				
				// Absolute axis like joysticks need to be scaled by delta time in order to be framerate independent.
				if( AbsoluteAxis )
				{
					Speed *= CurrentDeltaTime * AbsoluteAxis;
				}

				UpdateAxisValue( Axis, Speed * InvertMultiplier * CurrentDelta );
			}
		}
		else Ar.Logf( TEXT("%s Bad Axis command"),Str );
		return 1;
	}
	else if ( ParseCommand( &Str, TEXT("COUNT") ) )
	{
		BYTE *Count;
		if
		(	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) 
		&& (Count=FindButtonName(Temp))!=NULL )
		{
			*Count += 1;
		}
		else Ar.Logf( TEXT("%s Bad Count command"),Str );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("KEYBINDING") ) && ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
	{
		FName	KeyName(Temp,FNAME_Find);
		if(KeyName != NAME_None)
		{
			for(UINT BindIndex = 0;BindIndex < (UINT)Bindings.Num();BindIndex++)
			{
				if(Bindings(BindIndex).Name == KeyName)
				{
					Ar.Logf(TEXT("%s"),*Bindings(BindIndex).Command);
					break;
				}
			}
		}

		return 1;
	}
	else if( !InAlias && ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
	{
		FName	KeyName(Temp,FNAME_Find);
		if(KeyName != NAME_None)
		{
			for(INT BindIndex = Bindings.Num() - 1; BindIndex >= 0; BindIndex--)
			{
				if(Bindings(BindIndex).Name == KeyName)
				{
					InAlias = 1;
					ExecInputCommands(*Bindings(BindIndex).Command,Ar);
					InAlias = 0;
					return 1;
				}
			}
		}
	}

	return 0;
}

//
//	UInput::InputKey
//
UBOOL UInput::InputKey(INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed,UBOOL bGamepad)
{
	switch(Event)
	{
	case IE_Pressed:
		if(PressedKeys.FindItemIndex(Key) != INDEX_NONE)
		{
			debugfSuppressed(NAME_Input, TEXT("Received pressed event for key %s that was already pressed (%s)"), *Key.ToString(), *GetFullName());
			return FALSE;
		}
		PressedKeys.AddUniqueItem(Key);
		break;

	case IE_Released:
		if(!PressedKeys.RemoveItem(Key))
		{
			debugfSuppressed(NAME_Input, TEXT("Received released event for key %s but key was never pressed (%s)"), *Key.ToString(), *GetFullName());
			return FALSE;
		}
		break;
	default:
		break;
	};

	CurrentEvent		= Event;
	CurrentDelta		= 0.0f;
	CurrentDeltaTime	= 0.0f;

	const FString Command = GetBind(Key);
	if(Command.Len())
	{
		ExecInputCommands(*Command,*GLog);
		return TRUE;
	}
	else
	{
		return Super::InputKey(ControllerId,Key,Event,AmountDepressed,bGamepad);
	}
}

//
//	UInput::InputAxis
//
UBOOL UInput::InputAxis(INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad)
{
	CurrentEvent		= IE_Axis;
	CurrentDelta		= Delta;
	CurrentDeltaTime	= DeltaTime;

	const FString Command = GetBind(Key);
	if(Command.Len())
	{
		ExecInputCommands(*Command,*GLog);
		return TRUE;
	}
	else
	{
		return Super::InputAxis(ControllerId,Key,Delta,DeltaTime, bGamepad);
	}
}

/** Directly apply delta to the axis */
void UInput::UpdateAxisValue( FLOAT* Axis, FLOAT Delta )
{
	*Axis += Delta;
}

//
//	UInput::Tick - Read input for the viewport.
//
void UInput::Tick(FLOAT DeltaTime)
{
	if(DeltaTime != -1.f)
	{
		// Update held keys with IE_Repeat.
		for(UINT PressedIndex = 0;PressedIndex < (UINT)PressedKeys.Num();PressedIndex++)
		{
			// calling InputAxis here is intentional - we just want to execute the same stuff that happens in InputAxis, even though the PressedKeys array
			// is only filled by InputKey.
			InputAxis(0,PressedKeys(PressedIndex),1,DeltaTime);
		}
	}
	else
	{
		// Initialize axis array if needed.
		if( !AxisArray.Num() )
		{
			for( UProperty* Property = GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext )
			{
				if( (Property->PropertyFlags & CPF_Input) != 0 && Property->IsA(UFloatProperty::StaticClass()) )
				{
					for ( INT ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ArrayIndex++ )
					{
						AxisArray.AddUniqueItem( (FLOAT*) ((BYTE*)this + Property->Offset + ArrayIndex * Property->ElementSize) );
					}
				}
			}
		}

		// Reset axis.
		for( INT i=0; i<AxisArray.Num(); i++ )
		{
			*AxisArray(i) = 0;
		}
	}

	Super::Tick(DeltaTime);
}

//
//	UInput::ResetInput - Reset the input system's state.
//
void UInput::ResetInput()
{
	FlushPressedKeys();

	// Reset all input variables.
	for( UProperty* Property = GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext )
	{
		if ( (Property->PropertyFlags&CPF_Input) != 0 )
		{
			for ( INT ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ArrayIndex++ )
			{
				Property->ClearValue( (BYTE*)this + Property->Offset + ArrayIndex * Property->ElementSize );
			}
		}
	}
}


/* ==========================================================================================================
	UPlayerInput
========================================================================================================== */
/**
 * Generates an IE_Released event for each key in the PressedKeys array, then clears the array.  Should be called when another
 * interaction which swallows some (but perhaps not all) input is activated.
 */
void UPlayerInput::FlushPressedKeys()
{
	APlayerController* PlayerOwner = GetOuterAPlayerController();
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOuterAPlayerController()->Player);
	if ( LocalPlayer != NULL )
	{
		TArray<FName> PressedKeyCopy = PressedKeys;
		for ( INT KeyIndex = 0; KeyIndex < PressedKeyCopy.Num(); KeyIndex++ )
		{
			FName Key = PressedKeyCopy(KeyIndex);

			// simulate a release event for this key so that the PlayerInput code can perform any cleanup required
			if ( DELEGATE_IS_SET(OnReceivedNativeInputKey) )
			{
				delegateOnReceivedNativeInputKey(LocalPlayer->ControllerId, Key, IE_Released, 0);
			}

			InputKey(LocalPlayer->ControllerId, Key, IE_Released, 0);
		}
	}

	Super::FlushPressedKeys();
}

// Catch input from gamepad so we can choose to apply aim assist or not
UBOOL UPlayerInput::InputKey( INT ControllerId, FName Key, enum EInputEvent Event, FLOAT AmountDepressed, UBOOL bGamepad )
{
	if( Key != KEY_LeftShift	&& 
		Key != KEY_RightShift	&&
		Key != KEY_LeftControl	&&
		Key	!= KEY_RightControl	&&
		Key != KEY_LeftAlt		&&
		Key	!= KEY_RightAlt		)
	{
		bUsingGamepad = bGamepad;
	}

	return Super::InputKey( ControllerId, Key, Event, AmountDepressed, bGamepad );
}

// Store key name for this axis so we can use it on update
UBOOL UPlayerInput::InputAxis( INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad )
{
	//@todo  there must be some way of directly getting the mouse sampling rate from directinput
	if ( Key == KEY_MouseX )
	{
		// calculate sampling time
		// make sure not first non-zero sample
		if ( SmoothedMouse[0] > 0 )
		{
			// not first non-zero
			MouseSamplingTotal += DeltaTime;
			MouseSamples++;
		}
	}

	LastAxisKeyName = Key;
	return Super::InputAxis( ControllerId, Key, Delta, DeltaTime, bGamepad );
}

void UPlayerInput::UpdateAxisValue( FLOAT* Axis, FLOAT Delta )
{
	if( Delta != 0.f )
	{
		// Catch input from gamepad so we can choose to apply aim assist or not
		if( LastAxisKeyName == KEY_XboxTypeS_LeftX	||
			LastAxisKeyName == KEY_XboxTypeS_LeftY	||
			LastAxisKeyName == KEY_XboxTypeS_RightX ||
			LastAxisKeyName == KEY_XboxTypeS_RightY	)
		{
			bUsingGamepad = TRUE;
		}
		else
		{
			bUsingGamepad = FALSE;
		}
	}

	Super::UpdateAxisValue( Axis, Delta );
}


/// EOF



