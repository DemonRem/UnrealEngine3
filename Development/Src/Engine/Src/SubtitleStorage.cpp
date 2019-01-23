/*=============================================================================
	SubtitleStorage.cpp: Utility for loading subtitle text
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "SubtitleStorage.h"

FSubtitleStorage::FSubtitleStorage()
: ActiveMovie(-1),
  ActiveTip(-1)
{
}

FSubtitleStorage::~FSubtitleStorage()
{
}

void FSubtitleStorage::Load( FString const& Filename )
{
	Movies.Empty();
	Add( Filename );
}

void FSubtitleStorage::Add( FString const& Filename )
{
	FFilename Path = Filename;

	if (GFileManager->FileSize(*Path) == -1)
	{
		return;
	}

	TArray<BYTE> Storage;
	if (!appLoadFileToArray(Storage, *Path))
	{
		return;
	}

	FSubtitleMovie Movie;

	Movie.MovieName = Path.GetBaseFilename();
	Movie.RandomSelect = FALSE;

	UBOOL bFirstLine = TRUE;
	UBOOL bIsDone = FALSE;

	UINT TimeScale = 1000; // By default the values are specified in ms.

	ANSICHAR* Ptr = reinterpret_cast<ANSICHAR*>(Storage.GetData());

	while (!bIsDone)
	{
		// Advance past new line characters
		while (*Ptr=='\r' || *Ptr=='\n')
		{
			Ptr++;
		}

		// Store the location of the first character of this line
		ANSICHAR* Start = Ptr;

		// Advance the char pointer until we hit a newline character
		while (*Ptr && *Ptr!='\r' && *Ptr!='\n')
		{
			Ptr++;
		}

		// If this is the end of the file, we're done
		if (*Ptr == 0)
		{
			bIsDone = 1;
		}

		// Terminate the current line, and advance the pointer to the next character in the stream
		*Ptr++ = 0;

		if ( bFirstLine )
		{
			// parse the line of text
			INT Matched = appSSCANF(ANSI_TO_TCHAR(Start), TEXT("%u"), &TimeScale);
			if ( Matched )
				bFirstLine = FALSE;
		}
		else
		{
			UINT StartTime, StopTime;

			TCHAR Subtitle[1024];
			
			appMemzero(Subtitle, sizeof(Subtitle));

			if (strlen(Start) < ARRAY_COUNT(Subtitle))
			{
				// parse the line of text
				appSSCANF(ANSI_TO_TCHAR(Start), TEXT("%u,%u, %s"), &StartTime, &StopTime, Subtitle);
				if (Subtitle[0])
				{
					FSubtitleKeyFrame KeyFrame;
					KeyFrame.StartTime = StartTime * 1000 / TimeScale;
					KeyFrame.StopTime  = StopTime * 1000 / TimeScale;
					KeyFrame.Subtitle  = FString(Subtitle);

					Movie.KeyFrames.Push(KeyFrame);

					// If start and end times are both zero - enable random selection
					if( StartTime == 0 && StopTime == 0 )
					{
						Movie.RandomSelect = TRUE;
					}
				}
			}
		}
	}

	Movies.Push(Movie);
}

void FSubtitleStorage::ActivateMovie( FString const& MovieName )
{
	FString LookupName = FFilename(MovieName).GetBaseFilename();

	for (INT MovieIndex = 0; MovieIndex < Movies.Num(); ++MovieIndex)
	{
		FSubtitleMovie const& Movie = Movies(MovieIndex);
		if (LookupName == Movie.MovieName)
		{
			ActiveMovie = MovieIndex;
			ActiveTip = ( appRand() * Movie.KeyFrames.Num() ) / RAND_MAX;
			return;
		}
	}

	ActiveTip = -1;
	ActiveMovie = -1;
}

FString FSubtitleStorage::LookupSubtitle( UINT Time ) const
{
	if ((ActiveMovie != -1) && (ActiveMovie < Movies.Num()))
	{
		FSubtitleMovie const& Movie = Movies(ActiveMovie);

		if( Movie.RandomSelect )
		{
			if( ActiveTip > -1 )
			{
				FSubtitleKeyFrame const& KeyFrame = Movie.KeyFrames( ActiveTip );
				return KeyFrame.Subtitle;
			}
		}
		else
		{
			for (INT FrameIndex = 0; FrameIndex < Movie.KeyFrames.Num(); ++FrameIndex)
			{
				FSubtitleKeyFrame const& KeyFrame = Movie.KeyFrames(FrameIndex);
				if (KeyFrame.StartTime > Time)
				{
					break;
				}

				if (KeyFrame.StartTime <= Time && Time <= KeyFrame.StopTime)
				{
					return KeyFrame.Subtitle;
				}
			}
		}
	}

	return FString();
}



