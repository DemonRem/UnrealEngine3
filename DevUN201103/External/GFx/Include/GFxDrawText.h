/**********************************************************************

Filename    :   GFxDrawText.h
Content     :   External text interface
Created     :   May 23, 2008
Authors     :   Artem Bolgar

Notes       :   

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDRAWTEXT_H
#define INC_GFXDRAWTEXT_H

#include "GConfig.h"
#ifndef GFC_NO_DRAWTEXT_SUPPORT
#include "GTypes.h"
#include "GRefCount.h"
#include "GTypes2DF.h"
#include "GRenderer.h"
#include "GFxLoader.h"
#include "GFxPlayerStats.h"
#include "GFxString.h"

class GFxFontManager;
class GFxFontManagerStates;
class GFxDrawTextImpl;
class GFxDrawTextManager;
class GFxTextDocView;
class GFxTextAllocator;
class GFxTextFormat;
class GFxTextParagraphFormat;

/* This class provides external text drawing functionality in GFx. User may use this
   class in conjunction with GFxDrawTextManager to draw his own text without
   loading SWF/GFX files. 
   GFxDrawTextManager::CreateText or GFxDrawTextManager::CreateHtmlText should be 
   used to create an instance of GFxDrawText:

       GPtr<GFxDrawText> ptxt = *pdm->CreateText("FHWEUHF!", GRectF(20, 20, 500, 400));

       GPtr<GFxDrawText> ptxt3 = *pdm->CreateHtmlText("<p><FONT size='20'>AB <b>singleline</b><i> CD</i>O", GRectF(20, 300, 400, 700));

   To render the text use the method Display. Note, it is necessary to call GFxDrawTextManager::BeginDisplay
   before the first call to Display and to call GFxDrawTextManager::EndDisplay after the last call to Display:

       GViewport vp(GetWidth(), GetHeight(), 0, 0, GetWidth(), GetHeight(), 0);
       pdm->BeginDisplay(vp);

       ptxt->Display();
       ptxt2->Display();
       ptxt3->Display();

       pdm->EndDisplay();
   */
class GFxDrawText : public GRefCountBaseNTS<GFxDrawText, GFxStatMV_Text_Mem>
{
    friend class GFxDrawTextManager;
public:
    typedef GRenderer::Matrix Matrix;
    typedef GRenderer::Cxform Cxform;

    // SetText sets UTF-8, UCS-2 or GString text value to the text object. The optional 
    // parameter lengthInBytes specifies number of bytes in the UTF-8 string; 
    // lengthInChars specifies number of characters in wide character string.
    // If these parameters are not specified then
    // the UTF-8 and UCS-2 strings should be null-terminated.
    virtual void SetText(const char* putf8Str, UPInt lengthInBytes = UPInt(-1)) = 0;
    virtual void SetText(const wchar_t* pstr,  UPInt lengthInChars = UPInt(-1)) = 0;
    virtual void SetText(const GString& str) = 0;
    // Returns currently set text in UTF-8 format. It returns plain text value;
    // even if HTML is used then it returns the string with all HTML tags stripped out.
    virtual GString GetText() const = 0;

    // SetHtmlText parses UTF-8, UCS-2 or GString encoded HTML and initializes the text object
    // by the parsed HTML text.
    // The optional parameter lengthInBytes specifies number of bytes in the UTF-8 string; 
    // lengthInChars specifies number of characters in wide character string.
    // If these parameters are not specified then
    // the UTF-8 and UCS-2 strings should be null-terminated.
    virtual void SetHtmlText(const char* putf8Str, UPInt lengthInBytes = UPInt(-1)) = 0;
    virtual void SetHtmlText(const wchar_t* pstr,  UPInt lengthInChars = UPInt(-1)) = 0;
    virtual void SetHtmlText(const GString& str) = 0;
    // Returns currently set text in HTML format. If plain text is used with setting formatting 
    // by calling methods, such as SetColor, SetFont, etc, then this text will be converted to 
    // appropriate HTML format by this method.
    virtual GString GetHtmlText() const = 0;

    // Sets color (R, G, B, A) to whole text or to the part of text in interval [startPos..endPos].
    // Both startPos and endPos parameters are optional.
    virtual void SetColor(GColor c, UPInt startPos = 0, UPInt endPos = UPInt(-1)) = 0;
    // Sets font to whole text or to the part of text in interval [startPos..endPos].
    // Both startPos and endPos parameters are optional.
    virtual void SetFont (const char* pfontName, UPInt startPos = 0, UPInt endPos = UPInt(-1)) = 0;
    // Sets font size to whole text or to the part of text in interval [startPos..endPos].
    // Both startPos and endPos parameters are optional.
    virtual void SetFontSize(Float fontSize, UPInt startPos = 0, UPInt endPos = UPInt(-1)) = 0;
    enum FontStyle
    {
        Normal,
        Bold,
        Italic,
        BoldItalic,
        ItalicBold = BoldItalic
    };
    // Sets font style to whole text or to the part of text in interval [startPos..endPos].
    // Both startPos and endPos parameters are optional.
    virtual void SetFontStyle(FontStyle, UPInt startPos = 0, UPInt endPos = UPInt(-1)) = 0;
    // Sets or clears underline to whole text or to the part of text in interval [startPos..endPos].
    // Both startPos and endPos parameters are optional.
    virtual void SetUnderline(bool underline, UPInt startPos = 0, UPInt endPos = UPInt(-1)) = 0;

    // Sets multiline (parameter multiline is set to true) or singleline (false) type of the text.
    virtual void SetMultiline(bool multiline) = 0;
    // Returns 'true' if the text is multiline; false otherwise.
    virtual bool IsMultiline() const = 0;

    // Turns wordwrapping on/off
    virtual void SetWordWrap(bool wordWrap) = 0;
    // Returns state of wordwrapping.
    virtual bool IsWordWrap() const = 0;

    // Sets view rectangle, coordinates are in pixels.
    virtual void    SetRect(const GRectF& viewRect) = 0;
    // Returns currently used view rectangle, coordinates are in pixels.
    virtual GRectF  GetRect() const = 0;

    // Sets transformation matrix to the text object.
    virtual void SetMatrix(const Matrix& matrix) = 0;
    // Returns the currently using transformation matrix.
    virtual Matrix GetMatrix() const = 0;

    // Sets a 3D transformation matrix to the text object.
    virtual void SetMatrix3D(const GMatrix3D& matrix) { GUNUSED(matrix); }
    // Returns the currently used 3D transformation matrix.
    virtual GMatrix3D *GetMatrix3D() const { return NULL; }

    // Set color transformation matrix to the text object.
    virtual void SetCxform(const Cxform& cx) = 0;
    // Returns the currently using color transformation matrix.
    virtual const Cxform& GetCxform() const = 0;

    // Set color of border around the text. If alpha is set to 0 then border is 
    // not drawing (default behavior).
    virtual void   SetBorderColor(const GColor& borderColor) = 0;
    
    // Returns currently set border color. If alpha is 0 then border is turned off.
    virtual GColor GetBorderColor() const = 0;

    // Set color of background under the text. If alpha is set to 0 then background is 
    // not drawing (default behavior).
    virtual void   SetBackgroundColor(const GColor& bkgColor) = 0;

    // Returns currently set background color. If alpha is 0 then background is turned off.
    virtual GColor GetBackgroundColor() const = 0;

    enum Alignment
    {
        Align_Left,
        Align_Default = Align_Left,
        Align_Right,
        Align_Center,
        Align_Justify
    };
    // Sets horizontal text alignment (right, left, center)
    virtual void        SetAlignment(Alignment) = 0;
    // Returns horizontal text alignment (right, left, center)
    virtual Alignment   GetAlignment() const = 0;

    enum VAlignment
    {
        VAlign_Top,
        VAlign_Default = VAlign_Top,
        VAlign_Center,
        VAlign_Bottom
    };
    // Sets vertical text alignment (top, bottom, center)
    virtual void        SetVAlignment(VAlignment) = 0;
    // Returns vertical text alignment (top, bottom, center)
    virtual VAlignment  GetVAlignment() const = 0;

    // Displays the text. Note, GFxDrawTextManagers BeginDisplay 
    // method should be called before the first call to this method, 
    // and EndDisplay method should be called after the last Display 
    // method call.
    virtual void        Display() = 0;

    // Sets a string value that is passed to the GRenderer::PushUserData virtual function. 
    // Developers can use this interface to pass custom text field specific data to 
    // their own renderer implementations. 
    virtual void        SetRendererString(const GString& str) = 0;
    // Returns a string value previously set by SetRendererString.
    virtual GString     GetRendererString() const   = 0;

    // Sets a float value that is passed to the GRenderer::PushUserData virtual function. 
    // Developers can use this interface to pass custom text field specific data to 
    // their own renderer implementations. 
    virtual void        SetRendererFloat(float f) = 0;
    // Returns a float value previously set by SetRendererFloat.
    virtual float       GetRendererFloat() const  = 0;

    // Sets an array or matrix of floats (maximum size 16) that is passed to the GRenderer::PushUserData virtual function. 
    // Developers can use this interface to pass custom text field specific data to 
    // their own renderer implementations.
    virtual void        SetRendererMatrix(float *m, UInt count = 16) = 0;

    enum AAMode
    {
        AA_Animation,
        AA_Readability
    };
    // Sets anti-aliasing mode - AA_Readibility for readability and AA_Animation for animation.
    // The default setting is anti-aliasing for animation.
    virtual void        SetAAMode(AAMode) =0;
    // Returns currently set anti-aliasing mode.
    virtual AAMode      GetAAMode() const =0;

    // Enumeration of supported filters
    enum FilterType
    {
        Filter_Unknown      = 0,
        Filter_DropShadow   = 1,
        Filter_Blur         = 2,
        Filter_Glow         = 3

        // not supported
        //Filter_Bevel        = 3,
        //Filter_GradientGlow = 4,
        //Filter_Convolution  = 5,
        //Filter_AdjustColor  = 6,
        //Filter_GradientBevel= 7
    };
    // Bit flags for filters
    enum FilterFlagsType
    {
        FilterFlag_KnockOut   = 0x20,
        FilterFlag_HideObject = 0x40,
        FilterFlag_FineBlur   = 0x80
    };

    // A structure defining a filter.
    struct Filter
    {
        FilterType Type;
        union
        {
            struct
            {
                Float BlurX;
                Float BlurY;
                Float Strength; // in percents
            } Blur;

            struct
            {
                Float BlurX;
                Float BlurY;
                Float Strength; // in percents
                UInt32 Color;
                UInt8 Flags;    // bit FilterFlag_*
            } Glow;

            struct
            {
                Float BlurX;
                Float BlurY;
                Float Strength; // in percents
                UInt32 Color;
                UInt8 Flags;    // bit FilterFlag_*

                Float Angle;    // in degrees
                Float Distance; // in pixels
            } DropShadow;
        };

        Filter() : Type(Filter_Unknown)  { InitByDefaultValues(); }
        Filter(FilterType ft) : Type(ft) { InitByDefaultValues(); }

        // Sets filter type. Useful, if initializing the filter as 
        // an element of array.
        void SetFilterType(FilterType ft) { Type = ft; }

        // Resets all data members to its default values.
        void InitByDefaultValues();

        // Sets "knockout" flag on
        void SetKnockOut()   { Glow.Flags |= FilterFlag_KnockOut; }
        // Clears "knockout" flag
        void ClearKnockOut() { Glow.Flags &= ~FilterFlag_KnockOut; }
        // Sets "hideobject" flag on
        void SetHideObject()   { Glow.Flags |= FilterFlag_HideObject; }
        // Clears "hideobject" flag 
        void ClearHideObject() { Glow.Flags &= ~FilterFlag_HideObject; }
    };

    // Sets filters on the text. More than one filter may be applied to one
    // text: "blur" filter might be combined with either "glow" or "dropshadow" filter.
    virtual void SetFilters(const Filter* filters, UPInt filtersCnt = 1) =0;

    // Remove all filters from the text
    virtual void ClearFilters() =0;
protected:
    virtual ~GFxDrawText() {}

    virtual GFxTextDocView* GetDocView() const = 0;
};

/* This class manages the GFxDrawText objects. GFxDrawTextManager should be used 
   for creation of GFxDrawText objects. It also may be used to measure text extents. 
   To render text, BeginDisplay should be called before the first call to GFxDrawText::Display
   and the EndDisplay method should be called after the last call to GFxDrawText::Display. 
   One GFxDrawTextManager instance may be used to manage (create, measure, render) multiple
   GFxDrawText instances.
   
   There are 3 ways to create GFxDrawTextManager: using default constructor, passing pointer on 
   GFxMovieDef, and passing pointer on GFxLoader / GFxStateBag:

   1.   GPtr<GFxDrawTextManager> pdtm = *new GFxDrawTextManager();
   2. 
        GFxLoader loader;
        ...
        GPtr<GFxMovieDef> pmd = *loader.CreateMovie("fonts.swf");
        GPtr<GFxDrawTextManager> pdtm = *new GFxDrawTextManager(pmd);
   3. 
        GFxLoader loader;
        ...
        GPtr<GFxDrawTextManager> pdtm = *new GFxDrawTextManager(&loader);

   In the case #2 all created GFxDrawText instances will be able to use all fonts defined by the loaded "fonts.swf".
   In the case #3 GFxDrawTextManager will inherit all states (log, font cache manager, etc) from the loader.
   */
class GFxDrawTextManager : public GRefCountBaseNTS<GFxDrawTextManager, GStat_Default_Mem>, public GFxStateBag
{
    friend class GFxDrawTextImpl;
public:
    struct TextParams
    {
        GColor                  TextColor;
        GFxDrawText::Alignment  HAlignment;
        GFxDrawText::VAlignment VAlignment;
        GFxDrawText::FontStyle  FontStyle;
        Float                   FontSize;
        GString                 FontName;
        bool                    Underline;
        bool                    Multiline;
        bool                    WordWrap;
		SInt					Leading;
		SInt					Indent;
		UInt					LeftMargin;
		UInt					RightMargin;

        TextParams();
    };

protected:
    class GFxDrawTextManagerImpl* pImpl;
    GMemoryHeap*                  pHeap;
	bool						  HeapOwned;

    virtual GFxStateBag* GetStateBagImpl() const;

    GFxTextAllocator*       GetTextAllocator();
    GFxFontManager*         GetFontManager();
    GFxFontManagerStates*   GetFontManagerStates();
    void                    CheckFontStatesChange();

    void SetBeginDisplayInvokedFlag(bool v = true); 
    void ClearBeginDisplayInvokedFlag();
    bool IsBeginDisplayInvokedFlagSet() const;
public:
    // Constructor. If pmovieDef is specified then the manager inherits all 
    // states from it, including fonts, font manager, font providers, font cache manager, etc.
	// Accepts a custom heap.  If no custom heap is provided, then the draw text manager heap
	// will be used.
    GFxDrawTextManager(GFxMovieDef* pmovieDef = NULL, GMemoryHeap* pheap = NULL);
    // Constructor. Copies all states from the 'ploader', plus, ResourceLib. 
    // This ctor gets a pointer to GFxLoader for compatibility reasons; in future, it might be
    // changed to a reference.
	// Accepts a custom heap.  If no custom heap is provided, then the draw text manager heap
	// will be used.
    GFxDrawTextManager(GFxLoader* ploader, GMemoryHeap* pheap = NULL);
    ~GFxDrawTextManager();

    // Sets default text parameters. If optional 'ptxtParams' parameter for CreateText is not
    // set then these default parameters will be used.
    void SetDefaultTextParams(const TextParams& params);
    // Returns currently set default text parameters.
    const TextParams& GetDefaultTextParams() const;

    // creates an empty GFxDrawText object.
    GFxDrawText* CreateText();

    // creates and initialize a GFxDrawText object. If ptxtParams specified the 
    // created instance will be initialized using it.
    GFxDrawText* CreateText(const char* putf8Str, const GRectF& viewRect, const TextParams* ptxtParams = NULL);
    GFxDrawText* CreateText(const wchar_t* pwstr, const GRectF& viewRect, const TextParams* ptxtParams = NULL);
    GFxDrawText* CreateText(const GString& str, const GRectF& viewRect, const TextParams* ptxtParams = NULL);

    // creates and initialize a GFxDrawText object using specified HTML.
    GFxDrawText* CreateHtmlText(const char* putf8Str, const GRectF& viewRect, const TextParams* ptxtParams = NULL);
    GFxDrawText* CreateHtmlText(const wchar_t* pwstr, const GRectF& viewRect, const TextParams* ptxtParams = NULL);
    GFxDrawText* CreateHtmlText(const GString& str, const GRectF& viewRect, const TextParams* ptxtParams = NULL);

    // Returns size of the text rectangle that would be necessary to render the
    // specified text using the text parameters from the txtParams parameter.
    // If WordWrap and Multiline in txtParams are set to 'true' then it uses 'width' parameter
    // as the width and calculates the height.  
    GSizeF GetTextExtent(const char* putf8Str, Float width = 0, const TextParams* ptxtParams = NULL);
    GSizeF GetTextExtent(const wchar_t* pwstr, Float width = 0, const TextParams* ptxtParams = NULL);
    GSizeF GetTextExtent(const GString& str, Float width = 0, const TextParams* ptxtParams = NULL);

    // Returns size of the text rectangle that would be necessary to render the
    // specified HTML text.  
    // If 'width' contains positive value then it assumes word wrapping is on and only 
    // the height will be calculated. HTML is treated as multiline text.
    GSizeF GetHtmlTextExtent(const char* putf8Str, Float width = 0, const TextParams* ptxtParams = NULL);
    GSizeF GetHtmlTextExtent(const wchar_t* pwstr, Float width = 0, const TextParams* ptxtParams = NULL);
    GSizeF GetHtmlTextExtent(const GString& str, Float width = 0, const TextParams* ptxtParams = NULL);

    // Begins/ends display of text objects. BeginDisplay method should be called before any call to
    // GFxDrawText::Display method. It is possible to put multiple calls to GFxDrawText::Display
    // for mutliple instances of GFxDrawText. After all text is drawn call the EndDisplay method.
    void BeginDisplay(const GViewport& vp);
    void EndDisplay();

protected:
    void            SetTextParams(GFxTextDocView* pdoc, const TextParams& txtParams,
                                  const GFxTextFormat* tfmt = NULL, const GFxTextParagraphFormat* pfmt = NULL);
    GFxTextDocView* CreateTempDoc(const TextParams& txtParams,
                                  GFxTextFormat* tfmt, GFxTextParagraphFormat *pfmt,
                                  Float width, Float height);
};
#endif //GFC_NO_DRAWTEXT_SUPPORT

#endif //INC_GFXDRAWTEXT_H

