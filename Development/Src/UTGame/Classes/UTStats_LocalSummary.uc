/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTStats_LocalSummary extends UTStatsSummary;

const	TABSPACE=8;

function GenerateSummary()
{
	if ( bKeepLocalStatsLog )
	{
		GenerateGameStats();
		GenerateIndPlayerStat();
	}
}

/**
 * Output a header to the stats file
 */

function HeaderText(FileWriter Output, string Text, optional bool bLesser)
{
	local int i,l;

    Output.Logf("");
    if (bLesser)
    {
		Output.Logf("------------------------------------------------------");
	}
	else
	{
		Output.Logf("============================================================");
	}

	L = Len(Text);
	if ( L < 60 )
	{
		L = int( (60.0 - L) * 0.5);
		for (i=0;i<L;i++)
			Text = " "$Text;
	}
	Output.Logf(Text);
    if (bLesser)
    {
		Output.Logf("------------------------------------------------------");
	}
	else
	{
		Output.Logf("============================================================");
	}
    Output.Logf("");
}

/**
 * The function will take a string and backfill the end to fit a given number of tabs
 */

function string TFormatStr(coerce string s, int NoTabs)
{
	local int ln,x,dif,n;

	ln = len(s);
	x = NoTabs * TABSPACE;

	if (x>ln)
	{
		dif = x-ln;
		n = int( float(dif) / TABSPACE);
		if (n * tabspace < dif)
		{
			n++;
		}

		for (x=0;x<n;x++)
			s = s $ chr(9);
	}

	return s;

}

/**
 * Left Pads a number with 0's
 */

function string IndexText(int Index)
{
	local int i;
	local string s;

	s = ""$Index;
	for (i=0;i<6-len(s);i++)
	{
		S = "0"$s;
	}

	return s;
}

/****************************************************************************
  General Gameplay Stats
****************************************************************************/

function GenerateMiscGameStats(FileWriter Output)
{
	local Info.ServerResponseLine ServerData;
	local int i;
	local string ip;

    HeaderText(Output,"Unreal Tournament 2007 General Gameplay Stats");

	// Collect all of the information about the server.

	WorldInfo.Game.GetServerInfo(ServerData);
	WorldInfo.Game.GetServerDetails(ServerData);

	ip = ServerData.IP;
	if (ServerData.IP != "")
	{
		ip = " ["$ip$":"$ServerData.Port$"]";
	}

	Output.Logf("ServerName:"$CHR(9)$ServerData.ServerName@ip);
	Output.Logf("ServerID  :"$CHR(9)$ServerData.ServerID);
	Output.Logf("");
    Output.Logf("Game: "@WorldInfo.Game.GameName@"match on"@GetMapFilename());
    Output.Logf("");
    Output.Logf(":: Settings ::");
    Output.Logf("");

	for (i=0;i<ServerData.ServerInfo.Length;i++)
	{
		Output.Logf( ""$CHR(9)$ ServerData.ServerInfo[i].Key$ CHR(9)$ CHR(9)$ ServerData.ServerInfo[i].Value);
	}
    Output.Logf("");
}
function GeneratePlayerSummaries(FileWriter Output)
{
	local int StatID,i,j;
	local string temp;
	local array<int> KillTally;

	HeaderText(Output,"Player Summary");

	Output.Logf("#"$chr(9)$"Total Time"$chr(9)$"Total Score"$chr(9)$"Kills"$chr(9)$"Deaths"$chr(9)$"Name");

	for (StatID=0;StatID<PlayerStats.Length;StatID++)
	{
		Temp = ""$StatID$chr(9);
		Temp = Temp$TFormatStr(TimeStr(GetPlayerConnectTime(StatID)),2);
		Temp = Temp$TFormatStr(GetPlayerScore(StatID),2);
		Temp = Temp$TFormatStr(PlayerStats[StatID].KillStats.Length,1);
		Temp = Temp$TFormatStr(PlayerStats[StatID].DeathStats.Length - PlayerStats[StatID].NoSuicides,1);
		Temp = Temp$GetDisplayName(StatID);

		Output.Logf(Temp);
	}

	Output.Logf("");
	Output.Logf(":: Kill Matrix ::");
	Output.Logf("");

	Temp = "";

	// Generate Header

	for (i=0;i<PlayerStats.Length;i++)
	{
		Temp=Temp$chr(9)$i;
	}

	Output.Logf(Temp);
	Output.Logf("");

	KillTally.Length = PlayerStats.Length;

	for (i=0;i<PlayerStats.Length;i++)
	{
		for (j=0;j<PlayerStats[i].KillStats.Length;j++)
		{
			if ( GameStats[PlayerStats[i].KillStats[j]].InstigatorID == i )
			{
				KillTally[j] = PlayerStats[i].NoSuicides;
			}
			else if ( GameStats[PlayerStats[i].KillStats[j]].InstigatorID >= 0 )
			{
				KillTally[ GameStats[PlayerStats[i].KillStats[j]].InstigatorID ]++;
			}
		}

		Temp = ""$i;
		for (j=0;j<KillTally.Length;j++)
		{
			Temp = Temp$chr(9)$KillTally[j];
			KillTally[j] = 0;
		}

		Output.Logf(Temp);
	}

    Output.Logf("");
}



function GenerateEventSummaries(FileWriter Output, int StatID)
{
	local int EventIndex;
	local string Temp, DName1, DName2;
	local UTGame GI;

	HeaderText(Output,"Event Summary");

	GI = UTGame(WorldInfo.Game);
	if (GI!=none)
	{
		for (EventIndex=0;EventIndex<GameStats.Length;EventIndex++)
		{

			if (StatID<0 || GameStats[EventIndex].InstigatorID == StatID)
			{

				DName1 = GameStats[EventIndex].InstigatorID>=0?PlayerStats[GameStats[EventIndex].InstigatorID].DisplayName:"";
				DName2 = GameStats[EventIndex].AdditionalID>=0?PlayerStats[GameStats[EventIndex].AdditionalID].DisplayName:"";

				Temp = GI.DecodeEvent(GameStats[EventIndex].GameStatType, GameStats[EventIndex].Team, DName1, DName2, GameStats[EventIndex].AdditionalData);

				if (Temp!="")
				{
					Output.Logf(IndexText(EventIndex)$CHR(9)$TimeStr(GameStats[EventIndex].TimeStamp)$CHR(9)$CHR(9)$Temp);
				}
			}
		}
	}
	else
	{
		Output.Logf("Stats not available on gametypes who are not children of UTGAME");
	}

	Output.Logf("");
}


function GenerateGameStats()
{
	Local FileWriter Output;

	Output = Spawn(class'FileWriter');
	if (Output!=none)
	{
		if ( Output.OpenFile( GetMapFilename(), FWFT_Stats,,,true) )
		{
			GenerateMiscGameStats(Output);
			GeneratePlayerSummaries(Output);
			GenerateEventSummaries(Output,-1);
		}
		else
		{
			StatLog("Could Not Create local Game Summary!",""$self);
		}
	}
}

/****************************************************************************
  Individual Player Stats
****************************************************************************/

function GeneratePlayerStats(FileWriter Output, int StatID)
{
	local string temp;
	local int i,j,shots,hits;

	HeaderText(Output,"Player Summary");

	Output.Logf("Player Name:"$CHR(9)$GetDisplayName(StatID));
	Output.Logf("Global ID  :"$CHR(9)$PlayerStats[StatID].GlobalID);
	Output.Logf("");

	Temp = "Connect Time:"@CHR(9)$TimeStr(GetPlayerConnectTime(StatId));
	if (PlayerStats[StatID].NoConnects>1)
	{
		Temp = Temp @ "(# of Reconnects:"@(PlayerStats[StatID].NoConnects-1)$")";
	}

	Output.Logf(Temp);
	Output.Logf("Final Score:"@CHR(9)$GetPlayerScore(StatID));
	Output.Logf("# of Kills:"@CHR(9)$PlayerStats[StatID].KillStats.Length);
	Output.Logf("# of Deaths:"@CHR(9)$PlayerStats[StatID].DeathStats.Length);
	Output.Logf("# of Suicides:"@CHR(9)$PlayerStats[StatID].NoSuicides);

	shots = 0;
	hits = 0;
	for (i=0;i<PlayerStats[StatID].WeaponStats.Length;i++)
	{
		for (j=0;j<PlayerStats[StatID].WeaponStats[i].FireStats.Length;j++)
		{
			shots += PlayerStats[StatID].WeaponStats[i].FireStats[j].ShotsFired;
			hits  += PlayerStats[StatID].WeaponStats[i].FireStats[j].ShotsDamaged + PlayerStats[StatID].WeaponStats[i].FireStats[j].ShotsDirectHit;
		}
	}

	Output.Logf("# of Shots:"@CHR(9)$Shots);
	Output.logf("# of Hits:"@CHR(9)$Hits);
	Output.Logf("");
	Output.Logf("General Accuracy:"@int(float(hits)/float(Shots)*100.0)$"%");
	Output.Logf("");

	Output.Logf(":: Bonuses ::");

	for (i=0;i<PlayerStats[StatID].BonusStats.Length;i++)
	{
		Output.Logf("x"$PlayerStats[StatID].BonusStats[i].NoReceived@PlayerStats[StatID].BonusStats[i].BonusType);
	}


	HeaderText(Output,"Weapon Stats");

	for (i=0;i<PlayerStats[StatID].WeaponStats.Length;i++)
	{
		GenerateSingleWeaponStats(Output,StatID, PlayerStats[StatID].WeaponStats[i]);
	}

}

function string FireDesc(int FireMode)
{
	if (FireMode==0)
		return "Primary Fire";
	else if (FireMode==1)
		return "Alternate Fire";
	else
		return "Unknown Fire";
}

function GenerateSingleWeaponStats(FileWriter Output, int StatID, WeaponStat WeapStat)
{
	local int i, cnt, Shots;
	local float a,e;
	local array<float> Accuracy;
	local array<float> Efficiency;
	local string temp;
	local GameStat GS;
	local class<UTDamageType> DT;

   	HeaderText(Output,""$WeapStat.WeaponType,true);

	if (WeapStat.FireStats.Length < 2)
	{
		WeapStat.FireStats.Length = 2;
	}

	Accuracy.Length = WeapStat.FireStats.Length;
	Efficiency.Length = WeapStat.FireStats.Length;

    // Force Fire and Alt Fire

	Cnt = 0;
	for (i=0;i<WeapStat.FireStats.Length;i++)
	{
		Shots = WeapStat.FireStats[i].ShotsFired;
		if (Shots>0)
		{
			Accuracy[i]   = float(WeapStat.FireStats[i].ShotsDirectHit) / float(Shots) * 100.0;
			Efficiency[i] = float( (WeapStat.FireStats[i].ShotsDirectHit + WeapStat.FireStats[i].ShotsDamaged)) / float(Shots) * 100.0;
			a += Accuracy[i];
			e += Efficiency[i];
			cnt++;
		}
		else
		{
			Accuracy[i]   = 0;
			Efficiency[i] = 0;
		}
	}


	if (Cnt>0)
	{
		a = a / Cnt;
		e = e / Cnt;
	}

	Temp = "Average Accuracy:"@a$"%"$CHR(9)$"Average Efficiency:"@e$"%";
	Output.Logf(Temp);
	Output.Logf("");

	Output.Logf("FIREMODE"$Chr(9)$"SHOTS"$CHR(9)$"DIRECT"$CHR(9)$"SPLASH"$CHR(9)$"ACCURACY"$CHR(9)$"EFFICIENCY");
	Output.Logf("");

	for (i=0;i<Accuracy.Length;i++)
	{
		Temp = "";
		Temp = Temp$TFormatStr(FireDesc(i),2);
		Temp = Temp$TFormatStr(WeapStat.FireStats[i].ShotsFired,1);
		Temp = Temp$TFormatStr(WeapStat.FireStats[i].ShotsDirectHit,1);
		Temp = Temp$TFormatStr(WeapStat.FireStats[i].ShotsDamaged,1);
		Temp = Temp$TFormatStr(Accuracy[i]$"%",2);
		Temp = Temp$Efficiency[i]$"%";

		Output.Logf(Temp);
	}

	if (WeapStat.Combos>0)
	{
		Output.logf("");
		Output.logf("Combos x"$WeapStat.Combos);
		Output.logf("");
	}

	Output.logf("");
	Output.Logf(":: Kill List ::");
	Output.logf("");


	for (i=0;i<PlayerStats[StatId].KillStats.Length;i++)
	{
		GS = GameStats[ PlayerStats[StatId].KillStats[i] ];
		if (GS.AdditionalData != none)
		{
			DT = class<UTDamageType>(GS.AdditionalData);
			if (DT!=none && DT.default.DamageWeaponClass != none && DT.default.DamageWeaponClass==WeapStat.WeaponType)
			{
				Output.Logf(TimeStr(GS.TimeStamp)$Chr(9)$"Killed "@GetDisplayName(GS.InstigatorID));
			}
		}
	}

	Output.Logf("");

}

function GeneratePlayerInvStats(FileWriter Output, int StatID)
{
	local int i;
	local string temp;

	HeaderText(Output,"Inventory Stats");

	Output.Logf("ITEM TYPE"$chr(9)$chr(9)$"PICKUPS"$CHR(9)$"DROPS"$CHR(9)$"GIVES");
	Output.Logf("");

	for (i=0;i<PlayerStats[StatID].InventoryStats.Length;i++)
	{
		Temp = TFormatStr(PlayerStats[StatID].InventoryStats[i].InventoryType,3);
		Temp = Temp $ TFormatStr(PlayerStats[StatID].InventoryStats[i].NoPickups,1);
		Temp = Temp $ TFormatStr(PlayerStats[StatID].InventoryStats[i].NoDrops,1);
		Temp = Temp $ TFormatStr(PlayerStats[StatID].InventoryStats[i].NoIntentionalDrops,1);
		Output.Logf(Temp);
	}

	Output.Logf("");
}

function GenerateIndPlayerStat()
{
	Local FileWriter Output;
	Local string FName;
	local int StatID;

	for (StatID=0;StatID<PlayerStats.Length;StatID++)
	{
		FName = GetDisplayName(StatID);
		Output = Spawn(class'FileWriter');
		if (Output!=none)
		{
			if ( Output.OpenFile( FName, FWFT_Stats,,,true) )
			{
				GeneratePlayerStats(Output,StatID);
				GeneratePlayerInvStats(Output,StatID);
				GenerateEventSummaries(Output,StatID);
			}
			else
			{
				StatLog("Could Not Create local Player Stats!",""$self);
			}
		}
	}
}


defaultproperties
{
}
