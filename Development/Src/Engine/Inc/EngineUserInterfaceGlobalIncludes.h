/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/*===========================================================================
    Class and struct declarations which are coupled to
	EngineUserInterfaceClasses.h but shouldn't be declared inside a class
===========================================================================*/

#ifndef NAMES_ONLY

#ifndef __ENGINEUSERINTERFACEGLOBALINCLUDES_H__
#define __ENGINEUSERINTERFACEGLOBALINCLUDES_H__

#ifndef VALIDATE_COMPONENT
#define VALIDATE_COMPONENT(comp) checkfSlow(comp==NULL||comp->GetOuter()==this,TEXT("Invalid #comp for %s: %s"), *GetPathName(), *comp->GetPathName())
#endif

void MigrateImageSettings( class UUITexture*& Texture, const struct FTextureCoordinates& Coordinates, const struct FUIStyleReference& Style, class UUIComp_DrawImage* DestinationComponent, const TCHAR* PropertyName );

struct HUIHitProxy : HObject
{
	DECLARE_HIT_PROXY(HUIHitProxy,HObject);

	class UUIScreenObject* UIObject;

	HUIHitProxy(class UUIScreenObject* InObject);
	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};

/**
 * FRawInputKeyEventData::ModifierKeyFlags bit values.
 * @note:	if more values are needed, the FRawInputKeyEventData::ModifierKeyFlags
 *			variable must be changed from a byte to an int
 */
enum EInputKeyModifierFlag
{
	/** Alt required */
	KEYMODIFIER_AltRequired		=	0x01,
	/** Ctrl required */
	KEYMODIFIER_CtrlRequired	=	0x02,
	/** Shift required */
	KEYMODIFIER_ShiftRequired	=	0x04,
	/** Alt excluded */
	KEYMODIFIER_AltExcluded		=	0x08,
	/** Ctrl excluded */
	KEYMODIFIER_CtrlExcluded	=	0x10,
	/** Shift excluded */
	KEYMODIFIER_ShiftExcluded	=	0x20,
	/** Pressed */
	KEYMODIFIER_Pressed			=	0x40,
	/** Released */
	KEYMODIFIER_Released		=	0x80,

	/** all */
	KEYMODIFIER_All				=	0xFF,
};

/** Which input events are supported by this key. */
enum ESupportedInputEvents
{
	SIE_Keyboard,
	SIE_MouseButton,
	SIE_PressedOnly,
	SIE_Axis,

	SIE_Number	// Must be the last element
};

/**
 * This class is used for automatically logging function entry/exit points.  It writes the log at construction and destruction.
 */
class FScopedDebugLogger
{
public:

	/** Constructors */
	FScopedDebugLogger( const TCHAR* Fmt, ... )
	: DebugString(NULL), DebugEventType(NAME_Log)
	{
		// We need to use malloc here directly as GMalloc might not be safe.
		if( GLog != NULL && !FName::SafeSuppressed(DebugEventType) )
		{
			INT		BufferSize	= 1024;
			INT		Result		= -1;

			while(Result == -1)
			{
				DebugString = (TCHAR*) appSystemRealloc( DebugString, BufferSize * sizeof(TCHAR) );
				GET_VARARGS_RESULT( DebugString, BufferSize, BufferSize-1, Fmt, Fmt, Result );
				BufferSize *= 2;
			};
			DebugString[Result] = 0;
			
			debugf(DebugEventType, TEXT("%s> >> %s"), appSpc(DebugIndent), DebugString);
		}

		DebugIndent += 2;
	}
	FScopedDebugLogger( EName EventType, const TCHAR* Fmt, ... )
	: DebugString(NULL), DebugEventType(EventType)
	{
		// We need to use malloc here directly as GMalloc might not be safe.	
		if( GLog != NULL && !FName::SafeSuppressed(DebugEventType) )
		{
			INT		BufferSize	= 1024;
			INT		Result		= -1;

			while(Result == -1)
			{
				DebugString = (TCHAR*) appSystemRealloc( DebugString, BufferSize * sizeof(TCHAR) );
				GET_VARARGS_RESULT( DebugString, BufferSize, BufferSize-1, Fmt, Fmt, Result );
				BufferSize *= 2;
			};
			DebugString[Result] = 0;
			debugf(DebugEventType, TEXT("%s> >> %s"), appSpc(DebugIndent), DebugString);
		}

		DebugIndent += 2;
	}

	/** Destructor */
	~FScopedDebugLogger()
	{
		DebugIndent -= 2;
		if( GLog != NULL && !FName::SafeSuppressed(DebugEventType) )
		{
			debugf(DebugEventType, TEXT("%s<< < %s"), appSpc(DebugIndent), DebugString);
		}

		appSystemFree( DebugString );
		DebugString = NULL;
	}

private:
	/** the formatted string that was passed to the ctor */
	TCHAR* DebugString;
	/** the logging event type to use when writing to the log */
	EName DebugEventType;
	/** how many spaces to insert at the beginning of the log message; used for easily visualizing the flow of execution */
	static INT DebugIndent;
};


#define tracef	FScopedDebugLogger

#define CANVAS_SUPPORTS_CLIP_MASK 0

#endif	// __ENGINEUSERINTERFACEGLOBALINCLUDES_H__
#endif	// NAMES_ONLY

