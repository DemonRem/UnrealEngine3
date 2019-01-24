/**********************************************************************

Filename    :   GFxUIEngine.h
Content     :   Movie manager / GFxMovieView access wrapper for UE3

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxEngine_h
#define GFxEngine_h

#if WITH_GFx

class FViewport;
class FGFxRenderer;
class FGFxRenderTarget;
class FGFxImageCreateInfo;
class FGFxImageCreator;
class UGFxInteraction;
class UGFxMoviePlayer;
// GFx built-in classes
class GFxMovieView;
class GFxMovieDef;
class GFxLoader;
class GFile;
class GImageInfoBase;

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GTypes.h"
#include "GRefCount.h"
#include "GRenderer.h"
#include "GFxPlayer.h"
#include "GFile.h"
#include "GFxLoader.h"
#include "GFxFontLib.h"
#include "GFxUIClasses.h"

#if WITH_GFx_IME
    #if defined(WIN32)
        #include "GFxIMEManager.h"
    #else
        #undef WITH_GFx_IME
    #endif
#endif

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

static const float                           DEFAULT_CURVE_ERROR = 2.0f;
static const GFxRenderConfig::RenderFlagType DEFAULT_STROKE_TYPE = GFxRenderConfig::RF_StrokeNormal;

#define PACKAGE_PATH     "/ package/"
#define PACKAGE_PATH_U   TEXT("/ package/")
#define PACKAGE_PATH_LEN 10
#define GAMEDIR_PATH     "gamedir://"
#define GAMEDIR_PATH_LEN 10

inline const char *IsPackagePath(const char *ppath)
{
	if (!strncmp(ppath, PACKAGE_PATH, PACKAGE_PATH_LEN))
		return ppath + PACKAGE_PATH_LEN;
	else
		return NULL;
}

/**
* Info re: the movie which was loaded.
*/
class FGFxMovie
{
public:
    FString             FileName;
	GFxMovieInfo        Info;
	GPtr<GFxMovieDef>   pDef;
	GPtr<GFxMovieView>  pView;
	DOUBLE              LastTime;
    UBOOL               Playing;
	UBOOL		        fVisible;
	UBOOL		        fUpdate;
    UBOOL               fViewportSet;
	UBOOL				bCanReceiveFocus;
	UBOOL               bCanReceiveInput;
    INT                 TimingMode;
    UGFxMoviePlayer*          pUMovie;
    UTextureRenderTarget2D* pRenderTexture;
    FRenderCommandFence RenderCmdFence;
};

struct FGFxLocalPlayerState
{
	class FGFxMovie*	FocusedMovie;
};


//=============================================================================
// FGFxEngine  -- A class responsible for playing scaleform movies as well as 
//    managing resources required to play these movies such as the Renderer and 
//    file loader
//=============================================================================

class FGFxEngineBase
{
protected:
    GFxSystem System;

public:    
    FGFxEngineBase();
    virtual ~FGFxEngineBase();
};

class FGFxEngine : public FGFxEngineBase
{
    /** Default constructor, allocates the required resources */
    FGFxEngine();

#if WITH_GFx_FULLSCREEN_MOVIE
    friend class FFullScreenMovieGFx;
#endif

public:
	static const int INVALID_LEVEL_NUM = -1;

    static FGFxEngine* GetEngine();

    /** Virtual destructor for subclassing, cleans up the allocated resources */
	virtual ~FGFxEngine();

	/** Set the viewport for use with HUD (overlay) movies */
	void        SetRenderViewport(FViewport* inViewport);
    FViewport*  GetRenderViewport() { return HudViewport; }

	/**
	 * Called once a frame to update the played movie's state.
	 * @param	DeltaTime - The time since the last frame.
	 */
	void        Tick(FLOAT DeltaTime);

	/** Render all the scaleform movies that are currently playing */
	void        RenderUI(UBOOL bRenderToSceneColor, INT DPG=SDPG_Foreground);
    void        RenderTextures();

    /** Get the next DPG that will have a Flash movie over it. */
    INT         GetNextMovieDPG(INT FirstDPG);

    /** Loads a flash movie, it does not start playing */
    FGFxMovie*  LoadMovie(const TCHAR* Path, UBOOL fInitFirstFrame = TRUE);

	/** Start playing a flash movie */
	void        StartScene(FGFxMovie* Movie, UTextureRenderTarget2D* RenderTexture = NULL, UBOOL fVisible = TRUE, UBOOL fUpdate = TRUE);

    /** Stops the playback of the topmost scene on the stack and unloads */
    void        CloseTopmostScene();

	/** Stops the playback of any scene currently playing, cleans up resources allocated to it if requested */
	void        CloseScene(FGFxMovie* pMovie, UBOOL fDelete);

    /** Stops and unloads all movies (used at end of map) */
    void        NotifyGameSessionEnded();

    /** Delete any movies scheduled for deletion by Close. */
    void        DeleteQueuedMovies(UBOOL wait = TRUE);

	/** Closes all movies */
	void		CloseAllMovies(BOOL bOnlyCloseOnLevelChangeMovies = FALSE);

	/** Closes all texturemovies */
	void		CloseAllTextureMovies();

    FGFxMovie*  GetFocusMovie(INT ControllerId);

	/**Sets whether or not a movie is allowed to receive focus.  Defaults to true*/
	void SetMovieCanReceiveFocus(FGFxMovie* InMovie, UBOOL bCanReceiveFocus);
	/**Sets whether or not a movie is allowed to receive input.  Defaults to true*/
	void SetMovieCanReceiveInput(FGFxMovie* InMovie, UBOOL bCanReceiveInput);

	// Helper function to insert a movie into a list of movies in priority order while removing any existing reference to the same movie
	void InsertMovieIntoList(FGFxMovie* Movie, TArray<FGFxMovie*>* List);
	// Insert movie into OpenMovies and DPG in priority order
	void InsertMovie(FGFxMovie* Movie, BYTE DPG);

#if WITH_GFx_IME
    GFxIMEManager* GetIMEManager() { return pImeManager; }
#endif

    /** Returns the topmost movie from the stack */
    FGFxMovie*    GetTopmostMovie() const;
	
	/** Returns the number of movies in the open movie list */
	INT GetNumOpenMovies() const;
	/** Returns an open movie from the open movie list at index Idx */
	FGFxMovie* GetOpenMovie(INT Idx) const;
    /** Returns the number of all movies */
    INT GetNumAllMovies() const;

	FGFxRenderer* GetRenderer()  { return pRenderer.GetPtr(); }

    GFxMovieDef*  LoadMovieDef(const TCHAR* Path, GFxMovieInfo& Info);

    UBOOL InputKey(INT ControllerId, FName key, EInputEvent event);
    UBOOL InputChar(INT ControllerId, TCHAR ch);
    UBOOL InputAxis(INT ControllerId, FName key, Float delta, Float DeltaTime, UBOOL bGamepad);
    void FlushPlayerInput(TSet<NAME_INDEX>* Keys = NULL);

    /** Replaces characters in an FString with alternate chars. Put here instead of FString for now, to avoid
    messing too much with the engine */
    static inline INT ReplaceCharsInFString(FString& strInput, TCHAR const * pCharsToReplace, TCHAR const pReplacement); 

    // translate GFxLoader path to Unreal path or return false if filesystem path
    static UBOOL GetPackagePath(const char *ppath, FFilename& pkgpath);

    static inline void ConvertUPropToGFx(UProperty* up, BYTE* paddr, GFxValue& val, GFxMovieView* Movie, bool overwrite = 0);
    static inline void ConvertGFxToUProp(UProperty* up, BYTE* paddr, const GFxValue& val, UGFxMoviePlayer* Movie);

	/**
	 * Given a relative path that may contain tokens such as /./ and ../, collapse these
	 * relative tokens into a an absolute reference.
	 *
	 * @param InString A relative path that may contain relative tokens.
	 *
	 * @return A path with all relative tokens collapsed
	 */
	static FFilename CollapseRelativePath(const FFilename& InString);

    FRenderCommandFence  RenderCmdFence;
#if WITH_GFx_FULLSCREEN_MOVIE
    class FFullScreenMovieGFx* FullscreenMovie;
#endif
    SInt                 GameHasRendered;

	// Currently playing movies
	TArray<FGFxMovie*> OpenMovies;
	// All movies, including not currently playing (inactive) movies
	TArray<FGFxMovie*> AllMovies;

	//Current GFx State for all Local Players
	TArray<FGFxLocalPlayerState> PlayerStates;

	//Helper function to translate ControllerId's to LocalPlayer Indexes
	INT GetLocalPlayerIndexFromControllerID(INT ControllerId);

	//Helper function to translate ControllerId's to that controller's focused movie
	FGFxMovie* GetFocusedMovieFromControllerID(INT ControllerId);

	//Function that determines what the focusable movies should be for all of the local players.
	void ReevaluateFocus();

	//Add a player state struct to the PlayerStates array
	void AddPlayerState();

	//Remove the player state struct from the PlayerStates array at index Index
	void RemovePlayerState(INT Index);

	//Reevaluate all movies sizes for a potential change
	void ReevaluateSizes();

private:
    
	//Reevaluate the specific size for a movie
	void SetMovieSize(FGFxMovie* Movie);
    
	/** Disallow copy and assignment */
	FGFxEngine& operator=(const FGFxEngine&);
	FGFxEngine(const FGFxEngine&);

    // Used for loading up swf resources
    GFxLoader          Loader;

    // Currently playing movies.
    FViewport*         HudViewport;
    GPtr<FGFxRenderTarget> HudRenderTarget;
    GPtr<FGFxRenderTarget> SceneColorRT;
    TArray<FGFxMovie*> DPGOpenMovies[SDPG_PostProcess+1];	
    // Movies to delete at start of next frame
    TArray<FGFxMovie*> DeleteMovies;
    // Movies to render to textures
    TArray<FGFxMovie*> TextureMovies;

    // Implementation of the renderer used by all the scaleform movies.
	GPtr<FGFxRenderer> pRenderer;

    //< Maximum amount of error in curve segments when tessellating.
    float              CurveError;				

    // mapping of Unreal keys (and mouse buttons) to GFx keys
    struct UGFxInput
    {
        GFxKey::Code    Key;
        int             MouseButton;

        UGFxInput (GFxKey::Code k) : Key(k), MouseButton(-1) { }
        UGFxInput (GFxKey::Code k, int mb) : Key(k), MouseButton(mb) { }
    };
    struct UGFxInputAxis
    {
        GFxKey::Code    Key1, Key2;

        UGFxInputAxis (GFxKey::Code a, GFxKey::Code b) : Key1(a), Key2(b) { }
    };
    TMap<NAME_INDEX,UGFxInput>                      KeyCodes, KeyMap;
    FName                                           AxisMouseEmulation[2]; // X, Y
    UBOOL                                           AxisMouseInvert[2];
    TMap<FName,struct FUIAxisEmulationDefinition>	AxisEmulationDefinitions;
    FLOAT                                           AxisRepeatDelay;
    FLOAT                                           UIJoystickDeadZone;

    FIntPoint                            MousePos;

    // send keyboard input to this movie (mouse goes to all visible movies if any FocusMovie is set)
    //FGFxMovie*          pFocusMovie;
    // if false, pass input through to game even when pFocusMovie is set.
    //UBOOL               CaptureInput;

    // keys to ignore because they were pressed before input capture began
    TMap<INT,TArray<FName> > InitialPressedKeys;

    FUIAxisEmulationData	 AxisInputEmulation[4];
    FName                    AxisInputEmulationLastKey[4];

private:	
    // Handle input for a movie.
    UBOOL InputKey(INT ControllerId, FGFxMovie* pFocusMovie, FName ukey, EInputEvent uevent);
    inline UBOOL IsKeyCaptured(FName ukey);

#ifndef GFX_NO_LOCALIZATION
    UBOOL               FontlibInitialized;
    GPtr<GFxFontLib>    pFontLib;
    GPtr<GFxFontMap>    pFontMap;
    GPtr<GFxTranslator> pTranslator;

    void InitLocalization();
    void InitFontlib();
#endif

#if WITH_GFx_IME
    GPtr<GFxIMEManager> pImeManager;
#endif

    static void InitGFxLoaderCommon(GFxLoader& Loader);

    UGFxInput* GetInputKey(FName Key)
    {
        return KeyMap.Find(Key.GetIndex());
    }
};

inline INT FGFxEngine::ReplaceCharsInFString(FString& strInput, TCHAR const * pCharsToReplace, TCHAR const pReplacement)
{
	INT num = 0 ;
	TCHAR* pData = (TCHAR*)strInput.GetCharArray().GetTypedData();
	TCHAR const *pChars;
	while (*pData != TEXT('\0'))
	{
		pChars = pCharsToReplace;
		while (*pChars != TEXT('\0'))
		{
			if (*pData == *pChars)
			{
				*pData = pReplacement; num++ ;
				break;
			}
			++pChars;
		}
		++pData;
	}
	return num ; 
}

inline void FGFxEngine::ConvertUPropToGFx(UProperty* up, BYTE* paddr, GFxValue& val, GFxMovieView* Movie, bool overwrite)
{
    if (up->ArrayDim > 1 && Movie)
    {
        Movie->CreateArray(&val);
        for (int i = 0; i < up->ArrayDim; i++)
        {
            GFxValue elem;
            ConvertUPropToGFx(up, paddr + i * up->ElementSize, elem, NULL);
            val.PushBack(elem);
        }
    }

    UBoolProperty* upbool = ExactCast<UBoolProperty>(up);
    if (upbool)
    {
		INT *TempValue = reinterpret_cast<INT*>(paddr);
		val.SetBoolean( (*TempValue & upbool->BitMask) ? TRUE : FALSE );
    }
    else if (Cast<UArrayProperty>(up) && Movie)
    {
        UArrayProperty* Array = (UArrayProperty*)up;
        FScriptArray*   Data = (FScriptArray*)paddr;
        BYTE* pelements = (BYTE*)Data->GetData();

        if (overwrite)
        {
            if (!val.IsArray())
                return;

            for (int i = 0; i < Data->Num(); i++)
            {
                GFxValue elem;
                val.GetElement(i, &elem);
                ConvertUPropToGFx(Array->Inner, pelements + i * Array->Inner->ElementSize, elem, Movie, 1);
                val.SetElement(i, elem);
            }
        }
        else
        {
            Movie->CreateArray(&val);

            for (int i = 0; i < Data->Num(); i++)
            {
                GFxValue elem;
                ConvertUPropToGFx(Array->Inner, pelements + i * Array->Inner->ElementSize, elem, Movie);
                val.PushBack(elem);
            }
        }
    }
    else if (Cast<UStructProperty>(up) && Movie)
    {
        UStructProperty* Struct = (UStructProperty*) up;

        if (overwrite)
        {
            if (!val.IsObject())
                return;

            for( TFieldIterator<UProperty> It(Struct->Struct); It; ++It )
            {
                GFxValue elem;
                FTCHARToUTF8 fname (*It->GetName());
                if (val.GetMember(fname, &elem))
                    ConvertUPropToGFx(*It, paddr + It->Offset, elem, Movie);
                else
                {
                    ConvertUPropToGFx(*It, paddr + It->Offset, elem, Movie);
                    val.SetMember(fname, elem);
                }
            }
        }
        else
        {
            Movie->CreateObject(&val);

            for( TFieldIterator<UProperty> It(Struct->Struct); It; ++It )
            {
                GFxValue elem;
                ConvertUPropToGFx(*It, paddr + It->Offset, elem, Movie);
                val.SetMember(FTCHARToUTF8(*It->GetName()), elem);
            }
        }
    }
    else
    {
        UPropertyValue v;
        up->GetPropertyValue(paddr, v);
        if (Cast<UByteProperty>(up))
            val.SetNumber(v.ByteValue);
        else if (Cast<UIntProperty>(up))
            val.SetNumber(v.IntValue);
        else if (Cast<UFloatProperty>(up))
            val.SetNumber(v.FloatValue);
        else if (Cast<UStrProperty>(up))
            val.SetStringW(**v.StringValue);
        else if (Cast<UObjectProperty>(up))
        {
            UObjectProperty* objp = Cast<UObjectProperty>(up);
            if (objp->PropertyClass->IsChildOf(UGFxObject::StaticClass()))
                val = *((GFxValue*)((UGFxObject*)v.ObjectValue)->Value);
            else
                val.SetNull();
        }
        else if (Cast<UBoolProperty>(up))
            val.SetBoolean(v.BoolValue ? 1 : 0);
        else
            val.SetUndefined();
    }
}

inline void FGFxEngine::ConvertGFxToUProp(UProperty* up, BYTE* paddr, const GFxValue& val, UGFxMoviePlayer* Movie)
{
    UPropertyValue v;

    if (up->ArrayDim > 1 && val.IsArray())
    {
        UInt count = up->ArrayDim;
        if (val.GetArraySize() < count)
            count = val.GetArraySize();
        GFxValue elem;
        for (UInt i = 0; i < count; i++)
        {
            val.GetElement(i, &elem);
            ConvertGFxToUProp(up, paddr + i * up->ElementSize, elem, Movie);
        }
    }
    else if (val.GetType() == GFxValue::VT_Boolean && Cast<UBoolProperty>(up))
    {
        v.BoolValue = val.GetBool();
        up->SetPropertyValue(paddr, v);
    }
    else if (val.GetType() == GFxValue::VT_Number)
    {
        if (Cast<UByteProperty>(up))
            v.ByteValue = (BYTE)val.GetNumber();
        else if (Cast<UIntProperty>(up))
            v.IntValue = (INT)val.GetNumber();
        else if (Cast<UFloatProperty>(up))
            v.FloatValue = val.GetNumber();
        else
            return;
        up->SetPropertyValue(paddr, v);
    }
    else if (Cast<UStrProperty>(up))
    {
        if (val.GetType() == GFxValue::VT_String)
        {
            FString str(val.GetString());
            v.StringValue = &str;
            up->SetPropertyValue(paddr, v);
        }
        else if (val.GetType() == GFxValue::VT_StringW)
        {
            FString str(val.GetStringW());
            v.StringValue = &str;
            up->SetPropertyValue(paddr, v);
        }
        else
        {
            FString str;
            v.StringValue = &str;
            up->SetPropertyValue(paddr, v);
        }
    }
    else if (Cast<UArrayProperty>(up) && val.IsArray())
    {
        UArrayProperty* Array = (UArrayProperty*)up;
        FScriptArray*   Data = (FScriptArray*)paddr;
        Data->Empty(0,Array->Inner->ElementSize);
        Data->AddZeroed(val.GetArraySize(), Array->Inner->ElementSize);
        BYTE* pelements = (BYTE*)Data->GetData();
        GFxValue elem;
        for (UInt i = 0; i < val.GetArraySize(); i++)
        {
            val.GetElement(i, &elem);
            ConvertGFxToUProp(Array->Inner, pelements + i * Array->Inner->ElementSize, elem, Movie);
        }
    }
    else if (Cast<UStructProperty>(up) && val.IsObject())
    {
        class ObjVisitor : public GFxValue::ObjectVisitor
        {
            UGFxMoviePlayer*       Movie;
            BYTE*            paddr;
            UStructProperty* Struct;

        public:
            ObjVisitor(UGFxMoviePlayer* m, BYTE* p, UStructProperty* s) : Movie(m), paddr(p), Struct(s) {}
            void    Visit(const char* name, const GFxValue& val)
            {
                FName fname (FUTF8ToTCHAR(name),FNAME_Find);
                for( TFieldIterator<UProperty> It(Struct->Struct); It; ++It )
                    if (It->GetFName() == fname)
                    {
                        ConvertGFxToUProp(*It, paddr + It->Offset, val, Movie);
                        break;
                    }
            }
        };

        ObjVisitor visitor(Movie, paddr, (UStructProperty*) up);
        val.VisitMembers(&visitor);
    }
    else if (Cast<UObjectProperty>(up))
    {
        UObjectProperty* objp = Cast<UObjectProperty>(up);
        if (objp->PropertyClass->IsChildOf(UGFxObject::StaticClass()))
        {
            v.ObjectValue = Movie->CreateValueAddRef(&val, objp->PropertyClass);
            up->SetPropertyValue(paddr, v);
        }
    }
}

extern FGFxEngine* GGFxEngine;
extern UGFxEngine* GGFxGCManager;

#define UGALLOC(s) GALLOC(s, GStat_Default_Mem)

// Helper for dynamic arrays of GFxValue on stack.
struct FAutoGFxValueArray
{
    UInt Count;
    GFxValue *Array;

    FAutoGFxValueArray(UInt count, void *p) : Count(count), Array((GFxValue*)p)
    {
        for (UInt i = 0; i < Count; i++)
            new(&Array[i]) GFxValue;
    }
    ~FAutoGFxValueArray()
    {
        for (UInt i = 0; i < Count; i++)
            Array[i].~GFxValue();
    }
    operator GFxValue* () { return Array; }
    GFxValue& operator[] (UInt i) { return Array[i]; }
    GFxValue& operator[] (int i) { return Array[i]; }
};
#define AutoGFxValueArray(name,count) FAutoGFxValueArray name (count,appAlloca((count) * sizeof(GFxValue)))

#endif // WITH_GFx

#endif // GFxEngine_h
