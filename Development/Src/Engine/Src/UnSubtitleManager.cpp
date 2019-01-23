/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineSoundNodeClasses.h"
#include "UnSubtitleManager.h"
#include "Localization.h"

/** 
* A function to split a string into an array of strings that fit within a certain width
*
* @param Text			The string to word wrap
* @param Font			The font that the string will be rendered in
* @param Width			Pixel width of render window
* @param Subtitles		Output array of strings split to width
* @return				Number of lines created. -1 if there is an error.
*/

// Presize the dynamic strings to these lengths to avoid thrashing
#define PRESIZE_LINE_LENGTH		128
#define PRESIZE_WORD_LENGTH		32

INT WordWrap( const TCHAR* Text, UFont* Font, INT Width, TArray<FSubtitleCue>& Subtitles )
{
	INT				WordWidth, CurrentWidth, StringLength, CharsProcessed;
	TCHAR			Prev, Curr;
	FSubtitleCue	Line( EC_EventParm );
	FString			Word;

	check( Text );
	check( !Subtitles.Num() );

	if( !Font )
	{
		return( 0 );
	}

	StringLength = appStrlen( Text );

	// FString of a blank string does not have a valid terminating NULL TCHAR
	if( !StringLength )
	{
		return( 0 );
	}

	Line.Text.Empty( PRESIZE_LINE_LENGTH );
	CurrentWidth = 0;
	CharsProcessed = 0;

	// Create an array of lines
	Prev = 0;
	Curr = *Text;
	while( Curr && ( CharsProcessed <= StringLength ) )
	{
		// Presize the string to avoid thrashing
		Word.Empty( PRESIZE_WORD_LENGTH );

		// Add the char from the previous iteration
		if( Prev )
		{
			Word += Prev;
		}

		// Grab letters until we hit a valid word break, a string terminator, or a newline
		Curr = *Text++;
		CharsProcessed++;
		while( Curr && ( Curr != L'\n' ) && ( !Prev || !appCanBreakLineAt( Prev, Curr ) ) )
		{
			Word += Curr;
			Prev = Curr;
			Curr = *Text++;
			CharsProcessed++;
		}

		Prev = Curr;

		// Get the pixel width of the string
		WordWidth = Font->GetStringSize( *Word );

		// Check for the line being filled
		if( CurrentWidth + WordWidth > Width )
		{
			// Add the currently constructed line and reset
			Subtitles.AddItem( Line );
			Line.Text.Empty( PRESIZE_LINE_LENGTH );
			CurrentWidth = 0;
			Word.Trim();	
		}

		// Add the word to the current line
		Line.Text += Word;
		CurrentWidth += WordWidth;

		// Check for forced newline with data in it
		if( CurrentWidth && Curr == L'\n' )
		{
			Subtitles.AddItem( Line );
			Line.Text.Empty( StringLength );
			CurrentWidth = 0;
			// Don't add the newline to the string
			Prev = 0;
		}
	}

#ifdef _DEBUG
	// Bad string - processed more chars than stringlength
	check( Curr == 0 );
#endif

	// Add in the partially constructed line
	Subtitles.AddItem( Line );
	return( Subtitles.Num() );
}

/**
 * Kills all the subtitles associated with the subtitle ID.
 * This is called from AudioComponent::Stop
 */
void FSubtitleManager::KillSubtitles( PTRINT SubtitleID )
{
	ActiveSubtitles.Remove( SubtitleID );
}

/**
 * Add an array of subtitles to the active list
 * This is called from AudioComponent::Play
 * As there is only a global subtitle manager, this system will not have different subtitles for different viewports
 *
 * @param  SubTitleID - the controlling id that groups sets of subtitles together
 * @param  Priority - used to prioritize subtitles; higher values have higher priority.  Subtitles with a priority 0.0f are not displayed.
 * @param  StartTime - float of seconds that the subtitles start at
 * @param  SoundDuration - time in seconds after which the subtitles do not display
 * @param  Subtitles - TArray of lines of subtitle and time offset to play them
 */
void FSubtitleManager::QueueSubtitles( PTRINT SubtitleID, FLOAT Priority, UBOOL bManualWordWrap, FLOAT SoundDuration, TArray<FSubtitleCue>& Subtitles )
{
	check( GEngine );
	check( GWorld );

	// Do nothing if subtitles are disabled.
	if( !GEngine->bSubtitlesEnabled )
	{
		return;
	}

	// No subtitles to display
	if( !Subtitles.Num() )
	{
		return;
	}

	// Warn when a subtitle is played without its priority having been set.
	if( Priority == 0.0f )
	{
		debugf( NAME_Warning, TEXT( "Received subtitle with no priority." ) );
		return;
	}

	// No sound associated with the subtitle (used for automatic timing)
	if( SoundDuration == 0.0f )
	{
		debugf( NAME_Warning, TEXT( "Received subtitle with no sound duration." ) );
		return;
	}

	FLOAT StartTime = GWorld->GetTimeSeconds();

	// Add in the new subtitles
	FActiveSubtitle& NewSubtitle = ActiveSubtitles.Set( SubtitleID, FActiveSubtitle( 0, Priority, bManualWordWrap, Subtitles ) );

	// Resolve time offsets to absolute time
	for( INT SubtitleIndex = 0; SubtitleIndex < NewSubtitle.Subtitles.Num(); ++SubtitleIndex )
	{
		if( NewSubtitle.Subtitles( SubtitleIndex ).Time <= SoundDuration )
		{
			NewSubtitle.Subtitles( SubtitleIndex ).Time += StartTime;
		}
		else
		{
			NewSubtitle.Subtitles( SubtitleIndex ).Time = StartTime + SoundDuration;
			debugf( NAME_Warning, TEXT( "Subtitle has time offset greater than length of sound - clamping" ) );
		}
	}

	// Add on a blank at the end to clear
	FSubtitleCue* Temp = new( NewSubtitle.Subtitles ) FSubtitleCue( EC_EventParm );
	Temp->Text = FString( "" );
	Temp->Time = StartTime + SoundDuration;
}

/**
 * Draws a subtitle at the specified pixel location.
 */
#define SUBTITLE_CHAR_HEIGHT		24

void FSubtitleManager::DisplaySubtitle( FCanvas* Canvas, const TCHAR* Text, FIntRect& Parms, const FLinearColor& Color )
{
	// These should be valid in here
	check( GEngine );
	check( Canvas );

	// This can be NULL when there's an asset mixup (especially with localisation)
	if( !GEngine->MediumFont )
	{
		warnf( TEXT( "NULL GEngine->MediumFont - subtitles not rendering!" ) );
		return;
	}

	// Center up vertically
	Parms.Max.Y -= SUBTITLE_CHAR_HEIGHT;

	// Display lines up from the bottom of the region
	// Needed to add a drop shadow and doing all 4 sides was the only way to make them look correct.  If this shows up as a framerate hit we'll have to think of a different way of dealing with this.
	DrawStringCentered( Canvas, Parms.Min.X + ( Parms.Width() / 2 ) - 1, Parms.Max.Y - 1, Text, GEngine->MediumFont, FLinearColor::Black );
	DrawStringCentered( Canvas, Parms.Min.X + ( Parms.Width() / 2 ) - 1, Parms.Max.Y + 1, Text, GEngine->MediumFont, FLinearColor::Black );
	DrawStringCentered( Canvas, Parms.Min.X + ( Parms.Width() / 2 ) + 1, Parms.Max.Y + 1, Text, GEngine->MediumFont, FLinearColor::Black );
	DrawStringCentered( Canvas, Parms.Min.X + ( Parms.Width() / 2 ) + 1, Parms.Max.Y - 1, Text, GEngine->MediumFont, FLinearColor::Black );
	DrawStringCentered( Canvas, Parms.Min.X + ( Parms.Width() / 2 ), Parms.Max.Y, Text, GEngine->MediumFont, Color );
}

/**
* If any of the active subtitles need to be split into multiple lines, do so now
* - caveat - this assumes the width of the subtitle region does not change while the subtitle is active
*/
void FSubtitleManager::SplitLinesToSafeZone( FIntRect& SubtitleRegion )
{
	INT				i;
	FString			Concatenated;
	FLOAT			SoundDuration;
	FLOAT			StartTime, SecondsPerChar, Cumulative;

	for( TMap<PTRINT, FActiveSubtitle>::TIterator It( ActiveSubtitles ); It; ++It )
	{
		FActiveSubtitle& Subtitle = It.Value();
		if( Subtitle.bSplit )
		{
			continue;
		}

		// Concatenate the lines into one (in case the lines were partially manually split)
		Concatenated.Empty( 256 );

		// Set up the base data
		FSubtitleCue& Initial = Subtitle.Subtitles( 0 );
		Concatenated = Initial.Text;
		StartTime = Initial.Time;
		SoundDuration = 0.0f;
		for( i = 1; i < Subtitle.Subtitles.Num(); i++ )
		{
			FSubtitleCue& Subsequent = Subtitle.Subtitles( i );
			Concatenated += Subsequent.Text;
			// Last blank entry sets the cutoff time to the duration of the sound
			SoundDuration = Subsequent.Time - StartTime;
		}

		// Adjust concat string to use \n character instead of "/n" or "\n"
		INT SubIdx = Concatenated.InStr( TEXT("/n"), FALSE, TRUE );
		while( SubIdx >= 0 )
		{
			Concatenated = Concatenated.Left(SubIdx) + ((TCHAR)L'\n') + Concatenated.Right(Concatenated.Len() - (SubIdx+2));
			SubIdx = Concatenated.InStr( TEXT("/n"), FALSE, TRUE );
		}
		SubIdx = Concatenated.InStr( TEXT("\\n"), FALSE, TRUE );
		while( SubIdx >= 0 )
		{
			Concatenated = Concatenated.Left(SubIdx) + ((TCHAR)L'\n') + Concatenated.Right(Concatenated.Len() - (SubIdx+2));
			SubIdx = Concatenated.InStr( TEXT("\\n"), FALSE, TRUE );
		}

		// Work out a metric for the length of time a line should be displayed
		SecondsPerChar = SoundDuration / Concatenated.Len();

		// Word wrap into lines
		Subtitle.Subtitles.Empty();
		WordWrap( *Concatenated, GEngine->MediumFont, SubtitleRegion.Width(), Subtitle.Subtitles );

		// Add in the blank terminating line
		FSubtitleCue* Temp = new( Subtitle.Subtitles ) FSubtitleCue( EC_EventParm );
		Temp->Text = FString( "" );
		Temp->Time = StartTime + SoundDuration;

		// Set up the times
		Cumulative = 0.0f;
		for( i = 0; i < Subtitle.Subtitles.Num(); i++ )
		{
			FSubtitleCue& Cue = Subtitle.Subtitles( i );
			Cue.Time = StartTime + Cumulative;
			Cumulative += SecondsPerChar * Cue.Text.Len();
		}

		// Mark it as split so it doesn't happen again
		Subtitle.bSplit = TRUE;
	}
}

/**
 * Trim the SubtitleRegion to the safe 80% of the canvas 
 */
void FSubtitleManager::TrimRegionToSafeZone( FCanvas* Canvas, FIntRect& SubtitleRegion )
{
	FIntRect SafeZone;

	// Must display all text within text safe area - currently defined as 80% of the screen width and height
	SafeZone.Min.X = ( 10 * Canvas->GetRenderTarget()->GetSizeX() ) / 100;
	SafeZone.Min.Y = ( 10 * Canvas->GetRenderTarget()->GetSizeY() ) / 100;
	SafeZone.Max.X = Canvas->GetRenderTarget()->GetSizeX() - SafeZone.Min.X;
	SafeZone.Max.Y = Canvas->GetRenderTarget()->GetSizeY() - SafeZone.Min.Y;

	// Trim to safe area, but keep everything central
	if( SubtitleRegion.Min.X < SafeZone.Min.X || SubtitleRegion.Max.X > SafeZone.Max.X )
	{
		INT Delta = Max( SafeZone.Min.X - SubtitleRegion.Min.X, SubtitleRegion.Max.X - SafeZone.Max.X );
		SubtitleRegion.Min.X += Delta;
		SubtitleRegion.Max.X -= Delta;
	}

	if( SubtitleRegion.Max.Y > SafeZone.Max.Y )
	{
		SubtitleRegion.Max.Y = SafeZone.Max.Y;
	}
}

/**
 * Find the highest priority subtitle from the list of currently active ones
 */
PTRINT FSubtitleManager::FindHighestPrioritySubtitle( FLOAT CurrentTime )
{
	// Tick the available subtitles and find the highest priority one
	FLOAT HighestPriority = -1.0f;
	PTRINT HighestPriorityID = 0;

	for( TMap<PTRINT, FActiveSubtitle>::TIterator It( ActiveSubtitles ); It; ++It )
	{
		FActiveSubtitle & Subtitle = It.Value();
		
		// Remove when last entry is reached
		if( Subtitle.Index == Subtitle.Subtitles.Num() - 1 )
		{
			It.RemoveCurrent();
			continue;
		}

		if( CurrentTime >= Subtitle.Subtitles( Subtitle.Index ).Time && CurrentTime < Subtitle.Subtitles( Subtitle.Index + 1 ).Time )
		{
			if( Subtitle.Priority > HighestPriority )
			{
				HighestPriority = Subtitle.Priority;
				HighestPriorityID = It.Key();
			}
		}
		else
		{
			Subtitle.Index++;
		}
	}

	return( HighestPriorityID );
}

/**
 * Display the currently queued subtitles and cleanup after they have finished rendering
 *
 * @param  Canvas - where to render the subtitles
 * @param  CurrentTime - current world time
 */
void FSubtitleManager::DisplaySubtitles( FCanvas* Canvas, FIntRect& SubtitleRegion )
{
	check( GEngine );
	check( GWorld );
	check( Canvas );

	if( !GEngine->MediumFont )
	{
		warnf( TEXT( "NULL GEngine->MediumFont - subtitles not rendering!" ) );
		return;
	}

	// Do nothing if subtitles are disabled
	if( !GEngine->bSubtitlesEnabled )
	{
		return;
	}
	
	// Work out the safe zones
	TrimRegionToSafeZone( Canvas, SubtitleRegion );

	// If the lines have not already been split, split them to the safe zone now
	SplitLinesToSafeZone( SubtitleRegion );

	// Find the subtitle to display
	PTRINT HighestPriorityID = FindHighestPrioritySubtitle( GWorld->GetTimeSeconds() );

	// Display the highest priority subtitle
	if( HighestPriorityID )
	{
		FActiveSubtitle * Subtitle = ActiveSubtitles.Find( HighestPriorityID );
		DisplaySubtitle( Canvas, *Subtitle->Subtitles( Subtitle->Index ).Text, SubtitleRegion, FLinearColor::White );
	}
}

/**
 * Returns the singleton subtitle manager instance, creating it if necessary.
 */
FSubtitleManager* FSubtitleManager::GetSubtitleManager( void )
{
	static FSubtitleManager* SSubtitleManager = NULL;

	if( !SSubtitleManager )
	{
		SSubtitleManager = new FSubtitleManager();
	}

	return( SSubtitleManager );
}

// end
