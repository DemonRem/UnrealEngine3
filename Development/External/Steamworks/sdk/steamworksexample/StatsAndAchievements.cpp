//========= Copyright � 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Class for tracking stats and achievements
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "StatsAndAchievements.h"
#include <math.h>
#include "steam/isteamuserstats.h"
#include "SpaceWarClient.h"

#define ACHDISP_FONT_HEIGHT 20
#define ACHDISP_COLUMN_WIDTH 340
#define ACHDISP_CENTER_SPACING 40
#define ACHDISP_VERT_SPACING 10
#define ACHDISP_IMG_SIZE 64
#define ACHDISP_IMG_PAD 10

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }

Achievement_t g_rgAchievements[] = 
{
	_ACH_ID( ACH_WIN_ONE_GAME, "Winner" ),
	_ACH_ID( ACH_WIN_100_GAMES, "Champion" ),
	_ACH_ID( ACH_TRAVEL_FAR_ACCUM, "Interstellar" ),
	_ACH_ID( ACH_TRAVEL_FAR_SINGLE, "Orbiter" ),
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#pragma warning( push )
//  warning C4355: 'this' : used in base member initializer list
//  This is OK because it's warning on setting up the Steam callbacks, they won't use this until after construction is done
#pragma warning( disable : 4355 ) 
CStatsAndAchievements::CStatsAndAchievements( CGameEngine *pGameEngine )
	: 
	m_pGameEngine( pGameEngine ), 
	m_pSteamUser( NULL ),
	m_pSteamUserStats( NULL ),
	m_GameID( SteamUtils()->GetAppID() ),
	m_CallbackUserStatsReceived( this, &CStatsAndAchievements::OnUserStatsReceived ),
	m_CallbackUserStatsStored( this, &CStatsAndAchievements::OnUserStatsStored ),
	m_CallbackAchievementStored( this, &CStatsAndAchievements::OnAchievementStored )
{
	m_pSteamUser = SteamUser();
	m_pSteamUserStats = SteamUserStats();

	m_bRequestedStats = false;
	m_bStatsValid = false;
	m_bStoreStats = false;

	m_flGameFeetTraveled = 0;

	m_nTotalGamesPlayed = 0;
	m_nTotalNumWins = 0;
	m_nTotalNumLosses = 0;
	m_flTotalFeetTraveled = 0;
	m_flMaxFeetTraveled = 0;

	m_flAverageSpeed = 0;

	m_hDisplayFont = pGameEngine->HCreateFont( ACHDISP_FONT_HEIGHT, FW_MEDIUM, false, "Arial" );
	if ( !m_hDisplayFont )
		OutputDebugString( "Stats font was not created properly, text won't draw\n" );
}
#pragma warning( pop )

//-----------------------------------------------------------------------------
// Purpose: Run a frame for the CStatsAndAchievements
//-----------------------------------------------------------------------------
void CStatsAndAchievements::RunFrame()
{
	if ( !m_bRequestedStats )
	{
		// Is Steam Loaded? if no, can't get stats, done
		if ( NULL == m_pSteamUserStats || NULL == m_pSteamUser )
		{
			m_bRequestedStats = true;
			return;
		}

		// If yes, are we connected to the back end? if no, wait
		if ( !m_pSteamUser->BLoggedOn() )
		{
			return;
		}

		// If yes, request our stats
		bool bSuccess = m_pSteamUserStats->RequestCurrentStats( );
		
		// This function should only return false if we weren't logged in, and we already checked that.
		// But handle it being false again anyway, just ask again later.
		m_bRequestedStats = bSuccess;
	}

	if ( !m_bStatsValid )
		return;

	// Get info from sources

	// Evaluate achievements
	for ( int iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch )
	{
		EvaluateAchievement( g_rgAchievements[iAch] );
	}

	// Store stats
	StoreStatsIfNecessary();
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate distance traveled
//-----------------------------------------------------------------------------
void CStatsAndAchievements::AddDistanceTraveled( float flDistance )
{
	m_flGameFeetTraveled += SpaceWarClient()->PixelsToFeet( flDistance );
}

//-----------------------------------------------------------------------------
// Purpose: Game state has changed
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnGameStateChange( EClientGameState eNewState )
{
	if ( !m_bStatsValid )
		return;

	switch ( eNewState )
	{
	case k_EClientStatsAchievements:
	case k_EClientGameStartServer:
	case k_EClientGameMenu:
	case k_EClientGameQuitMenu:
	case k_EClientGameExiting:
	case k_EClientGameInstructions:
	case k_EClientGameConnecting:
	case k_EClientGameConnectionFailure:
	default:
		break;
	case k_EClientGameActive:
		// Reset per-game stats
		m_flGameFeetTraveled = 0;
		m_ulTickCountGameStart = m_pGameEngine->GetGameTickCount();
		break;
	case k_EClientFindInternetServers:
		break;	
	case k_EClientGameWinner:
		if ( SpaceWarClient()->BLocalPlayerWonLastGame() )
			m_nTotalNumWins++;
		else
			m_nTotalNumLosses++;
		// fall through
	case k_EClientGameDraw:

		// Tally games
		m_nTotalGamesPlayed++;

		// Accumulate distances
		m_flTotalFeetTraveled += m_flGameFeetTraveled;

		// New max?
		if ( m_flGameFeetTraveled > m_flMaxFeetTraveled )
			m_flMaxFeetTraveled = m_flGameFeetTraveled;

		// Calc game duration
		m_flGameDurationSeconds = ( m_pGameEngine->GetGameTickCount() - m_ulTickCountGameStart ) / 1000.0;

		// We want to update stats the next frame.
		m_bStoreStats = true;

		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: see if we should unlock this achievement
//-----------------------------------------------------------------------------
void CStatsAndAchievements::EvaluateAchievement( Achievement_t &achievement )
{
	// Already have it?
	if ( achievement.m_bAchieved )
		return;

	switch ( achievement.m_eAchievementID )
	{
	case ACH_WIN_ONE_GAME:
		if ( m_nTotalNumWins )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_WIN_100_GAMES:
		if ( m_nTotalNumWins >= 100 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TRAVEL_FAR_ACCUM:
		if ( m_flTotalFeetTraveled >= 5280 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TRAVEL_FAR_SINGLE:
		if ( m_flGameFeetTraveled > 500 )
		{
			UnlockAchievement( achievement );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Unlock this achievement
//-----------------------------------------------------------------------------
void CStatsAndAchievements::UnlockAchievement( Achievement_t &achievement )
{
	achievement.m_bAchieved = true;

	// the icon may change once it's unlocked
	achievement.m_iIconImage = 0;

	// mark it down
	m_pSteamUserStats->SetAchievement( achievement.m_pchAchievementID );

	// Store stats end of frame
	m_bStoreStats = true;
}

//-----------------------------------------------------------------------------
// Purpose: Store stats in the Steam database
//-----------------------------------------------------------------------------
void CStatsAndAchievements::StoreStatsIfNecessary()
{
	if ( m_bStoreStats )
	{
		// already set any achievements in UnlockAchievement

		// set stats
		m_pSteamUserStats->SetStat( "NumGames", m_nTotalGamesPlayed );
		m_pSteamUserStats->SetStat( "NumWins", m_nTotalNumWins );
		m_pSteamUserStats->SetStat( "NumLosses", m_nTotalNumLosses );
		m_pSteamUserStats->SetStat( "FeetTraveled", m_flTotalFeetTraveled );
		m_pSteamUserStats->SetStat( "MaxFeetTraveled", m_flMaxFeetTraveled );
		// Update average feet / second stat
		m_pSteamUserStats->UpdateAvgRateStat( "AverageSpeed", m_flGameFeetTraveled, m_flGameDurationSeconds );
		// The averaged result is calculated for us
		m_pSteamUserStats->GetStat( "AverageSpeed", &m_flAverageSpeed );

		bool bSuccess = m_pSteamUserStats->StoreStats();
		// If this failed, we never sent anything to the server, try
		// again later.
		m_bStoreStats = !bSuccess;
	}
}


//-----------------------------------------------------------------------------
// Purpose: We have stats data from Steam. It is authoritative, so update
//			our data with those results now.
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	if ( !m_pSteamUserStats )
		return;

	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			OutputDebugString( "Received stats and achievements from Steam\n" );

			m_bStatsValid = true;

			// load achievements
			for ( int iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch )
			{
				Achievement_t &ach = g_rgAchievements[iAch];
				m_pSteamUserStats->GetAchievement( ach.m_pchAchievementID, &ach.m_bAchieved );
				_snprintf( ach.m_rgchName, sizeof( ach.m_rgchName ), "%s", 
					m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "name" ) );
				_snprintf( ach.m_rgchDescription, sizeof( ach.m_rgchDescription ), "%s", 
					m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "desc" ) );			
			}

			// load stats
			m_pSteamUserStats->GetStat( "NumGames", &m_nTotalGamesPlayed );
			m_pSteamUserStats->GetStat( "NumWins", &m_nTotalNumWins );
			m_pSteamUserStats->GetStat( "NumLosses", &m_nTotalNumLosses );
			m_pSteamUserStats->GetStat( "FeetTraveled", &m_flTotalFeetTraveled );
			m_pSteamUserStats->GetStat( "MaxFeetTraveled", &m_flMaxFeetTraveled );
			m_pSteamUserStats->GetStat( "AverageSpeed", &m_flAverageSpeed );
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult );
			OutputDebugString( buffer );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Our stats data was stored!
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			OutputDebugString( "StoreStats - success\n" );
		}
		else if ( k_EResultInvalidParam == pCallback->m_eResult )
		{
			// One or more stats we set broke a constraint. They've been reverted,
			// and we should re-iterate the values now to keep in sync.
			OutputDebugString( "StoreStats - some failed to validate\n" );
			// Fake up a callback here so that we re-load the values.
			UserStatsReceived_t callback;
			callback.m_eResult = k_EResultOK;
			callback.m_nGameID = m_GameID.ToUint64();
			OnUserStatsReceived( &callback );
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "StoreStats - failed, %d\n", pCallback->m_eResult );
			OutputDebugString( buffer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: An achievement was stored
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( 0 == pCallback->m_nMaxProgress )
		{
			char buffer[128];
			_snprintf( buffer, 128, "Achievement '%s' unlocked!", pCallback->m_rgchAchievementName );
			OutputDebugString( buffer );
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "Achievement '%s' progress callback, (%d,%d)\n", 
				pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress );
			OutputDebugString( buffer );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Display the user's stats and achievements
//-----------------------------------------------------------------------------
void CStatsAndAchievements::Render()
{
	const int32 width = m_pGameEngine->GetViewportWidth();
	const int32 height = m_pGameEngine->GetViewportHeight();

	const int32 pxColumn1Left = width / 2 - ACHDISP_COLUMN_WIDTH - ACHDISP_CENTER_SPACING / 2;
	const int32 pxColumn2Left = width / 2 + ACHDISP_CENTER_SPACING / 2;

	RECT rect;

	if ( !m_bStatsValid )
	{
		rect.top = 0;
		rect.bottom = m_pGameEngine->GetViewportHeight();
		rect.left = 0;
		rect.right = width;

		char rgchBuffer[256];
		_snprintf( rgchBuffer, sizeof( rgchBuffer), "Unable to retrieve data from Steam\n" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );


		rect.left = 0;
		rect.right = width;
		rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.7);
		rect.bottom = m_pGameEngine->GetViewportHeight();

		_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Press ESC to return to the Main Menu" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_TOP, rgchBuffer );

	}
	else
	{
		// COLUMN 1
		// Achievements above the midline 
		int32 pxVertOffset = height / 2 - 2 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );
		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		rect.left = pxColumn1Left;
		rect.right = rect.left + ACHDISP_COLUMN_WIDTH;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[0] );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[1] );

		// Stats below the midline
		pxVertOffset = height / 2 + ACHDISP_VERT_SPACING;

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;
		
		DrawStatInfo( rect, "Games Played", static_cast<float>( m_nTotalGamesPlayed ) );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Games Won", static_cast<float>( m_nTotalNumWins ) );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Games Lost", static_cast<float>( m_nTotalNumLosses ) );

		// COLUMN 2
		// Achievements above the midline
		pxVertOffset = height / 2 - 2 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );
		
		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		rect.left = pxColumn2Left;
		rect.right = rect.left + ACHDISP_COLUMN_WIDTH;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[2] );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[3] );

		// Stats below the midline
		pxVertOffset = height / 2 + ACHDISP_VERT_SPACING;

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Feet Traveled", m_flTotalFeetTraveled );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Max Feet Traveled", m_flMaxFeetTraveled );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Average Inches / Second", m_flAverageSpeed * 12.0f );
		
		// Footer
		rect.left = 0;
		rect.right = width;
		rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.7);
		rect.bottom = m_pGameEngine->GetViewportHeight();

		char rgchBuffer[256];
		_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Press ESC to return to the Main Menu" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_TOP, rgchBuffer );

	}
}

void CStatsAndAchievements::DrawAchievementInfo( RECT &rect, Achievement_t &ach )
{
	if ( ach.m_iIconImage == 0 )
	{
		ach.m_iIconImage = m_pSteamUserStats->GetAchievementIcon( ach.m_pchAchievementID );
	}

	HGAMETEXTURE hTexture = SpaceWarClient()->GetSteamImageAsTexture( ach.m_iIconImage );

	// don't modify the caller's rect, they may use it later to locate something else
	RECT rect2 = rect;

	// could still be zero if the image isn't downloaded yet
	if (hTexture )
	{
		m_pGameEngine->BDrawTexturedQuad( (float)rect2.left, (float)rect2.top, (float)rect2.left+ACHDISP_IMG_SIZE, (float)rect2.bottom, 
			0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );

		rect2.left += ACHDISP_IMG_SIZE + ACHDISP_IMG_PAD;
	}

	// todo: divide up so can draw image
	char rgchBuffer[256];
	_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s: %s\n%s", 
		ach.m_rgchName,
		ach.m_bAchieved ? "Unlocked" : "Locked",
		ach.m_rgchDescription );

	m_pGameEngine->BDrawString( m_hDisplayFont, rect2, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_LEFT|DT_VCENTER, rgchBuffer );
}

void CStatsAndAchievements::DrawStatInfo( RECT &rect, const char *pchName, float flValue )
{
	// todo: divide up so can draw image
	char rgchBuffer[256];
	_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s: %.1f", pchName, flValue );
	m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_LEFT|DT_VCENTER, rgchBuffer );
}