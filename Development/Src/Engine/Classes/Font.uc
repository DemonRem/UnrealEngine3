/**
 * Copyright 1998-2007 Epic Games, Inc.
 *
 * A font object, containing information about a set of glyphs.
 * The glyph bitmaps are stored in the contained textures, while
 * the font database only contains the coordinates of the individual
 * glyph.
 */
class Font extends Object
	hidecategories(object)
	native;

// This is the character that RemapChar will return if the specified character doesn't exist in the font
const NULLCHARACTER = 127;

struct native FontCharacter
{
	var() int StartU;
	var() int StartV;
	var() int USize;
	var() int VSize;
	var() editconst BYTE TextureIndex;

	structcpptext
	{
		// Serializer.
		friend FArchive& operator<<( FArchive& Ar, FFontCharacter& Ch )
		{
			return Ar << Ch.StartU << Ch.StartV << Ch.USize << Ch.VSize << Ch.TextureIndex;
		}
	}
};

var() editinline array<FontCharacter> Characters;

//NOTE: Do not expose this to the editor as it has nasty crash potential
var array<Texture2D> Textures;

var private const native Map{WORD,WORD} CharRemap;

var int IsRemapped;
var() int Kerning;

cpptext
{
	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
	 *
	 * @return		Size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual INT GetResourceSize();

	// UFont interface
	FORCEINLINE TCHAR RemapChar(TCHAR CharCode) const
	{
		const WORD UCode = ToUnicode(CharCode);
		if ( IsRemapped )
		{
			// currently, fonts are only remapped if they contain Unicode characters.
			// For remapped fonts, all characters in the CharRemap map are valid, so
			// if the characters exists in the map, it's safe to use - otherwise, return
			// the null character (an empty square on windows)
			const WORD* FontChar = CharRemap.Find(UCode);
			if ( FontChar == NULL )
				return UCONST_NULLCHARACTER;

			return (TCHAR)*FontChar;
		}

		// Otherwise, our Characters array will contains 256 members, and is
		// a one-to-one mapping of character codes to array indexes, though
		// not every character is a valid character.
		if ( UCode >= Characters.Num() )
			return UCONST_NULLCHARACTER;

		// If the character's size is 0, it's non-printable or otherwise unsupported by
		// the font.  Return the default null character (an empty square on windows).
		if ( Characters(UCode).VSize == 0 )
			return UCONST_NULLCHARACTER;

		return CharCode;
	}

	FORCEINLINE void GetCharSize(TCHAR InCh, FLOAT& Width, FLOAT& Height, INT ResolutionPageIndex=0) const
	{
		Width = Height = 0.f;

		const INT Ch = (TCHARU)RemapChar(InCh) + ResolutionPageIndex;
		if( Ch < Characters.Num() )
		{
			const FFontCharacter& Char = Characters(Ch);
			if( Char.TextureIndex < Textures.Num() && Textures(Char.TextureIndex) != NULL )
			{
				Width = Char.USize;
				Height = Char.VSize;
			}
		}
	}

	FORCEINLINE INT GetStringSize( const TCHAR *Text, INT ResolutionPageIndex = 0 ) const
	{
		FLOAT	Width, Height, Total;

		Total = 0.0f;
		while( *Text )
		{
			GetCharSize( *Text, Width, Height, ResolutionPageIndex );
			Total += Width;
			Text++;
		}

		return( appCeil( Total ) );
	}

	// UObject interface
	
	/**
	* Serialize the object struct with the given archive
	*
	* @param Ar - archive to serialize with
	*/
	virtual void Serialize( FArchive& Ar );
	
	/**
	* Called after object and all its dependencies have been serialized.
	*/
	virtual void PostLoad();

}

/**
 * @Returns the resolution page index to use given a height test.
 */
native function int GetResolutionPageIndex(float HeightTest);

/**
 * @Returns the scaling factor for a given HeightTest.
 */
native function float GetScalingFactor(float HeightTest);

/**
 * @Returns the Maximum Height required by this font.
 */
native function float GetMaxCharHeight();

/**
 * @Returns the length of a string given this font
 */
native function float StrWidth(string Text);

