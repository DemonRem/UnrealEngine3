/**********************************************************************

Filename    :   GFxEvent.h
Content     :   Public interface for SWF Movie input events
Created     :   June 21, 2005
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXEVENT_H
#define INC_GFXEVENT_H

#include "GTypes.h"
#include "GRefCount.h"
#include "GArray.h"

#include "GFxPlayerStats.h"

#include <stdlib.h>
#include <string.h>

// ***** Declared Classes
class GFxKey;
class GFxEvent;
class GFxMouseEvent;
class GFxKeyEvent;
class GFxCharEvent;
class GFxKeyboardState;

// Forward declarations.
class GASStringContext;

// Key code wrapper class. This class contains enumeration
// for keyboard constants.

class GFxKey
{
public:
    // These key codes match Flash-defined values exactly.
    enum Code
    {
        VoidSymbol      = 0,

        // A through Z and numbers 0 through 9.
        A               = 65,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Num0            = 48,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,

        // Numeric keypad.
        KP_0            = 96,
        KP_1,
        KP_2,
        KP_3,
        KP_4,
        KP_5,
        KP_6,
        KP_7,
        KP_8,
        KP_9,
        KP_Multiply,
        KP_Add,
        KP_Enter,
        KP_Subtract,
        KP_Decimal,
        KP_Divide,

        // Function keys.
        F1              = 112,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,

        // Other keys.
        Backspace       = 8,
        Tab,
        Clear           = 12,
        Return,
        Shift           = 16,
        Control,
        Alt,
        Pause,
        CapsLock        = 20, // Toggle
        Escape          = 27,
        Space           = 32,
        PageUp,
        PageDown,
        End             = 35,
        Home,
        Left,
        Up,
        Right,
        Down,
        Insert          = 45,
        Delete,
        Help,
        NumLock         = 144, // Toggle
        ScrollLock      = 145, // Toggle

        Semicolon       = 186,
        Equal           = 187,
        Comma           = 188, // Platform specific?
        Minus           = 189,
        Period          = 190, // Platform specific?
        Slash           = 191,
        Bar             = 192,
        BracketLeft     = 219,
        Backslash       = 220,
        BracketRight    = 221,
        Quote           = 222,

        OEM_AX          = 0xE1,  //  'AX' key on Japanese AX kbd
        OEM_102         = 0xE2,  //  "<>" or "\|" on RT 102-key kbd.
        ICO_HELP        = 0xE3,  //  Help key on ICO
        ICO_00          = 0xE4,  //  00 key on ICO

        // Total number of keys.
        KeyCount
    };
};

// ***** Debug check support

// This expression will only be avaluated in debug mode.
#ifdef GFC_BUILD_DEBUG
#define GFC_DEBUG_EXPR(arg) arg
#define GFC_DEBUG_DECL(arg) arg;
#else
#define GFC_DEBUG_EXPR(arg)
#define GFC_DEBUG_DECL(arg)
#endif

class GFxSpecialKeysState 
{
public:
    enum
    {
        Key_ShiftPressed    = 0x01,
        Key_CtrlPressed     = 0x02,
        Key_AltPressed      = 0x04,
        Key_CapsToggled     = 0x08,
        Key_NumToggled      = 0x10,
        Key_ScrollToggled   = 0x20,

        Initialized_Bit     = 0x80,
        Initialized_Mask    = 0xFF
    };
    UInt8   States;

    GFxSpecialKeysState():States(0) {}
    GFxSpecialKeysState(UInt8 st):States(UInt8(st | Initialized_Bit)) {}

    void Reset() { States = 0; }

    bool IsShiftPressed() const { return (States & Key_ShiftPressed) != 0; }
    bool IsCtrlPressed() const  { return (States & Key_CtrlPressed) != 0; }
    bool IsAltPressed() const   { return (States & Key_AltPressed) != 0; }
    bool IsCapsToggled() const  { return (States & Key_CapsToggled) != 0; }
    bool IsNumToggled() const   { return (States & Key_NumToggled) != 0; }
    bool IsScrollToggled() const{ return (States & Key_ScrollToggled) != 0; }

    void SetShiftPressed(bool v = true)  { (v) ? States |= Key_ShiftPressed : States &= ~Key_ShiftPressed; }
    void SetCtrlPressed(bool v = true)   { (v) ? States |= Key_CtrlPressed  : States &= ~Key_CtrlPressed; }
    void SetAltPressed(bool v = true)    { (v) ? States |= Key_AltPressed   : States &= ~Key_AltPressed; }
    void SetCapsToggled(bool v = true)   { (v) ? States |= Key_CapsToggled  : States &= ~Key_CapsToggled; }
    void SetNumToggled(bool v = true)    { (v) ? States |= Key_NumToggled   : States &= ~Key_NumToggled; }
    void SetScrollToggled(bool v = true) { (v) ? States |= Key_ScrollToggled: States &= ~Key_ScrollToggled; }

    bool IsInitialized() const { return (States & Initialized_Mask) != 0; }
};

// ***** Event classes


// GFxEvent is a base class for input event notification. Objects of this type
// can be passed to GFxMovie::HandleEvent to inform the movie of various input
// actions. Events that have specific subclasses, such as GFxMouseEvent, must
// use those subclasses and NOT the base GFxEvent class.

class GFxEvent : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    enum    EventType
    {
        None,       
        // Informative events sent to the player.
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        KeyDown,
        KeyUp,
        SceneResize,
        SetFocus,
        KillFocus,

        // Action events, to be handled by user.
        DoShowMouse,
        DoHideMouse,
        DoSetMouseCursor,

        CharEvent,
        IMEEvent
    };

    // What kind of event this is.
    EventType           Type;

    // Size of class, used in debug build to verify that
    // appropriate classes are used for messages.
    GFC_DEBUG_DECL(UInt EventClassSize)


    GFxEvent(EventType eventType = None)
    {
        Type = eventType;
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxEvent));
    }
};


class   GFxMouseEvent : public GFxEvent
{
public:
    Float   x, y;
    Float   ScrollDelta;
    UInt    Button;
    UInt    MouseIndex;

    GFxMouseEvent() : GFxEvent()
    {
        Button = 0; x = 0; y = 0; ScrollDelta = 0.0f; MouseIndex = 0;
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseEvent));
    }
    // User event constructor - x, y must always be filled in.
    GFxMouseEvent(EventType eventType, UInt button,
        Float xpos, Float ypos, Float scrollVal = 0.0f, UInt mouseIdx = 0)
        : GFxEvent(eventType)
    {
        Button = button; x = xpos; y = ypos; ScrollDelta = scrollVal;
        MouseIndex = mouseIdx;
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseEvent));
    }
    GFxMouseEvent(EventType eventType, UInt mouseIdx)
        : GFxEvent(eventType)
    {
        Button = 0; x = y = 0; ScrollDelta = 0;
        MouseIndex = mouseIdx;
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseEvent));
    }
};

class   GFxMouseCursorEvent : public GFxEvent
{
public:
    enum CursorShapeType
    {
        ARROW = 0,
        HAND  = 1,
        IBEAM = 2
    };
    UInt    CursorShape;
    UInt    MouseIndex;

    GFxMouseCursorEvent() : GFxEvent(GFxEvent::DoSetMouseCursor), CursorShape(HAND), 
        MouseIndex(0)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseCursorEvent));
    }
    GFxMouseCursorEvent(CursorShapeType cursorShape, UInt mouseIndex)
        : GFxEvent(GFxEvent::DoSetMouseCursor), CursorShape(cursorShape), 
        MouseIndex(mouseIndex)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseCursorEvent));
    }
    GFxMouseCursorEvent(EventType eventType, UInt mouseIndex)
        : GFxEvent(eventType), CursorShape(ARROW), 
        MouseIndex(mouseIndex)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxMouseCursorEvent));
    }
};

class GFxKeyEvent : public GFxEvent
{
public:
    // Key, and whether it was pressed up or down
    GFxKey::Code        KeyCode;
    // ASCII code. 0, if unavailable.
    UByte               AsciiCode; // left for compatibility reasons only
    UInt                WcharCode; // left for compatibility reasons only
    // State of special keys
    GFxSpecialKeysState SpecialKeysState;
    
    // The index of the physical keyboard controller.
    UInt8               KeyboardIndex;

    GFxKeyEvent(EventType eventType = None, GFxKey::Code code = GFxKey::VoidSymbol, 
                UByte asciiCode = 0, UInt32 wcharCode = 0,
                UInt8 keyboardIndex = 0)
        : GFxEvent(eventType), KeyCode (code), AsciiCode (asciiCode), WcharCode(wcharCode),
        KeyboardIndex(keyboardIndex)
    { 
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxKeyEvent));
    }

    GFxKeyEvent(EventType eventType, GFxKey::Code code, UByte asciiCode, UInt32 wcharCode,
        GFxSpecialKeysState specialKeysState, UInt8 keyboardIndex = 0)
        : GFxEvent(eventType), KeyCode (code), AsciiCode (asciiCode), WcharCode(wcharCode),
        SpecialKeysState(specialKeysState), KeyboardIndex(keyboardIndex)
    { 
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxKeyEvent));
    }
};

class GFxCharEvent : public GFxEvent
{
public:
    UInt32              WcharCode;
    // The index of the physical keyboard controller.
    UInt8               KeyboardIndex;

    GFxCharEvent(UInt32 wcharCode, UInt8 keyboardIndex = 0)
        : GFxEvent(CharEvent), WcharCode(wcharCode), KeyboardIndex(keyboardIndex)
    { 
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxCharEvent));
    }
};

#ifndef GFC_NO_KEYBOARD_SUPPORT
// Keyboard state and queue class. 
class GFxKeyboardState : public GRefCountBaseNTS<GFxKeyboardState, GFxStatMV_Other_Mem>
{
public:
    class IListener
    {
    public:
        virtual ~IListener() {}
        virtual void OnKeyDown(GASStringContext *psc, int code, UByte ascii, 
            UInt32 wcharCode, UInt8 keyboardIndex)     = 0;
        virtual void OnKeyUp(GASStringContext *psc, int code, UByte ascii, 
            UInt32 wcharCode, UInt8 keyboardIndex)     = 0;
        virtual void Update(int code, UByte ascii, UInt32 wcharCode, 
            UInt8 keyboardIndex)                       = 0;
    };

protected:
    IListener*          pListener;

    class KeyQueue 
    {
    private:
        enum { KeyQueueSize = 100 };
        struct KeyRecord 
        {
            UInt32                          wcharCode;
            short                           code;
            GFxEvent::EventType             event;
            UByte                           ascii;
            GFxSpecialKeysState             specialKeysState;
        }               Buffer[KeyQueueSize];
        unsigned        PutIdx;
        unsigned        GetIdx;
        unsigned        Count;
    public:
        KeyQueue ();

        void Put(short code, UByte ascii, UInt32 wcharCode, GFxEvent::EventType event, GFxSpecialKeysState specialKeysState = 0);
        bool Get(short* code, UByte* ascii, UInt32* wcharCode, GFxEvent::EventType* event, GFxSpecialKeysState* specialKeysState = 0);

        GINLINE bool IsEmpty () const { return Count == 0; }

        void ResetState();

    } KeyQueue;

    UInt8               KeyboardIndex;

    UByte               Keymap[GFxKey::KeyCount / 8 + 1];   // bit-GArray
    bool                Toggled[3];
public:

    GFxKeyboardState();

    void    SetKeyboardIndex(UInt8 ki) { GASSERT(ki < GFC_MAX_KEYBOARD_SUPPORTED); KeyboardIndex = ki; }
    UInt8   GetKeyboardIndex() const   { return KeyboardIndex; }

    bool    IsKeyDown(int code) const;
    bool    IsKeyToggled(int code) const;
    void    SetKeyToggled(int code, bool toggle);
    void    SetKeyDown(int code, UByte ascii, GFxSpecialKeysState specialKeysState = 0);
    void    SetKeyUp(int code, UByte ascii, GFxSpecialKeysState specialKeysState = 0);
    void    SetChar(UInt32 wcharCode);

    bool    IsQueueEmpty() const { return KeyQueue.IsEmpty(); }
    bool    GetQueueEntry(short* code, 
                          UByte* ascii, 
                          UInt32* wcharCode, 
                          GFxEvent::EventType* event, 
                          GFxSpecialKeysState* specialKeysState = 0);

    void    CleanupListener();
    void    SetListener(IListener* listener);

    void    NotifyListeners(GASStringContext *psc, 
                            short code, 
                            UByte ascii, 
                            UInt32 wcharCode, 
                            GFxEvent::EventType event) const;
    void    UpdateListeners(short code, UByte ascii, UInt32 wcharCode);

    void    ResetState();
};
#endif //GFC_NO_KEYBOARD_SUPPORT

class GFxIMEEvent : public GFxEvent
{
public:
    enum IMEEventType
    {
        IME_Default,
        IME_PreProcessKeyboard
    } IMEEvtType;

    GFxIMEEvent(IMEEventType t = IME_Default) : GFxEvent(IMEEvent), IMEEvtType(t)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxIMEEvent));
    }
};

class GFxIMEWin32Event : public GFxIMEEvent
{
public:
    UInt  Message;  // Win32 message id (WM_<>)
    UPInt WParam;
    UPInt LParam;
    UPInt hWND;
    int   Options;

    GFxIMEWin32Event(IMEEventType t = IME_Default) : GFxIMEEvent(t), 
        Message(0), WParam(0), LParam(0), hWND(0), Options(0)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxIMEWin32Event));
    }
    GFxIMEWin32Event(IMEEventType t, UPInt hwnd, UInt win32msg, UPInt wp, UPInt lp, int options = 0) : GFxIMEEvent(t),
        Message(win32msg), WParam(wp), LParam(lp), hWND(hwnd), Options(options)
    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxIMEWin32Event));
    }
};

class GFxIMEXBox360Event : public GFxIMEEvent
{
public:
    // Unpacked XINPUT_KEYSTROKE structure
    UInt16  VirtualKey;
    wchar_t Unicode;
    UInt16  Flags;
    UInt8   UserIndex;
    UInt8   HidCode;
    void*   pLastKey;
    UInt    eType; 

    GFxIMEXBox360Event(UInt16 virtualKey, wchar_t unicode, UInt16 flags, UInt8 userIndex, UInt8 hidCode, void* plastKey,
        UInt etype) : GFxIMEEvent(IME_Default), VirtualKey(virtualKey), Unicode(unicode), Flags(flags),
        UserIndex(userIndex), HidCode(hidCode), pLastKey(plastKey), eType(etype)

    {
        GFC_DEBUG_EXPR(EventClassSize = sizeof(GFxIMEXBox360Event));
    }
};

class GFxIMEPS3Event : public GFxIMEEvent
{
public:

    GFxIMEPS3Event(): KeyCode(0), CharCode(0), Modifiers(0), LastKeyCode(0), LastCharCode(0), LastModifiers(0){};
    GFxIMEPS3Event(UInt keyCode, UInt code, UInt mods, UInt lastKeyCode, UInt lastCharCode, UInt lastMods)
    {
        KeyCode         = keyCode;
        CharCode        = code;
        Modifiers       = mods;
        LastKeyCode     = lastKeyCode;
        LastCharCode    = lastCharCode;
        LastModifiers   = lastMods;
    };
    UInt KeyCode;
    UInt CharCode;
    UInt Modifiers;
    mutable UInt LastKeyCode;
    mutable UInt LastCharCode;
    mutable UInt LastModifiers; 
};

class GFxIMEMacEvent : public GFxIMEEvent
{
public:

    GFxIMEMacEvent(): compositionString(NULL), attributes(NULL), eType(0){};
    GFxIMEMacEvent(char* compString, char* attrib, UInt etype)
    {
        compositionString   = compString;
        attributes          = attrib;
        eType               = etype;
    };

    char* compositionString;
    char* attributes;
    UInt  eType;
};

// Event to use when movie's window is getting focus. Special key state
// should be provided in order to maintain correct state of keys such as
// CapsLock, NumLock, ScrollLock.
class GFxSetFocusEvent : public GFxEvent
{
public:
    // State of special keys
    GFxSpecialKeysState SpecialKeysStates[GFC_MAX_KEYBOARD_SUPPORTED];

    GFxSetFocusEvent(GFxSpecialKeysState specialKeysState) 
        : GFxEvent(SetFocus) 
    {
        SpecialKeysStates[0] = specialKeysState;
    }

    GFxSetFocusEvent(UInt numKeyboards, GFxSpecialKeysState* specialKeysStates) 
        : GFxEvent(SetFocus) 
    {
        for (UInt i = 0, n = G_Min(UInt(GFC_MAX_KEYBOARD_SUPPORTED), numKeyboards); i < n; ++i)
            SpecialKeysStates[i] = specialKeysStates[i];
    }
};

#endif // ! INC_GFXEVENT_H
