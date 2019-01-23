//=============================================================================
// AccessControl.
//
// AccessControl is a helper class for GameInfo.
// The AccessControl class determines whether or not the player is allowed to
// login in the PreLogin() function, and also controls whether or not a player
// can enter as a spectator or a game administrator.
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class AccessControl extends Info
	config(Game);


var globalconfig array<string>   IPPolicies;
var	localized string          IPBanned;
var	localized string	      WrongPassword;
var	localized string          NeedPassword;
var localized string          SessionBanned;
var localized string		  KickedMsg;
var localized string          DefaultKickReason;
var localized string		  IdleKickReason;
var class<Admin> AdminClass;

var private globalconfig string AdminPassword;	    // Password to receive bAdmin privileges.
var private globalconfig string GamePassword;		    // Password to enter game.

var localized string ACDisplayText[3];
var localized string ACDescText[3];

var bool bDontAddDefaultAdmin;


function bool IsAdmin(PlayerController P)
{
	return P.PlayerReplicationInfo.bAdmin;
}

function bool SetAdminPassword(string P)
{
	AdminPassword = P;
	return true;
}

function SetGamePassword(string P)
{
	GamePassword = P;
}

function bool RequiresPassword()
{
	return GamePassword != "";
}

function Kick( string S )
{
	local Controller C;

	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (C.PlayerReplicationInfo != None && C.PlayerReplicationInfo.PlayerName ~= S)
		{
			if (PlayerController(C) != None)
			{
				KickPlayer(PlayerController(C), DefaultKickReason);
			}
			else if (C.PlayerReplicationInfo.bBot)
			{
				if (C.Pawn != None)
				{
					C.Pawn.Destroy();
				}
				if (C != None)
				{
					C.Destroy();
				}
			}
			break;
		}
	}
}

function KickBan( string S )
{
	local PlayerController P;
	local string IP;

	ForEach DynamicActors(class'PlayerController', P)
		if ( P.PlayerReplicationInfo.PlayerName~=S
			&&	(NetConnection(P.Player)!=None) )
		{
			IP = P.GetPlayerNetworkAddress();
			if( CheckIPPolicy(IP) )
			{
				IP = Left(IP, InStr(IP, ":"));
				`Log("Adding IP Ban for: "$IP);
				IPPolicies[IPPolicies.length] = "DENY," $ IP;
				SaveConfig();
			}
			P.Destroy();
			return;
		}
}

function bool KickPlayer(PlayerController C, string KickReason)
{
	// Do not kick logged admins
	if (C != None && !IsAdmin(C) && NetConnection(C.Player)!=None )
	{
	
		C.ClientWasKicked();
		
		if (C.Pawn != None)
			C.Pawn.Destroy();
		if (C != None)
			C.Destroy();
		return true;
	}
	return false;
}

function bool AdminLogin( PlayerController P, string Password )
{
	if (AdminPassword == "")
		return false;

	if (Password == AdminPassword)
	{
		`Log("Administrator logged in.");
		WorldInfo.Game.Broadcast( P, P.PlayerReplicationInfo.PlayerName$"logged in as a server administrator." );
		return true;
	}
	return false;
}

//
// Accept or reject a player on the server.
// Fails login if you set the OutError to a non-empty string.
//
event PreLogin
(
	string Options,
	string Address,
	out string OutError,
	out string FailCode,
	bool bSpectator
)
{
	// Do any name or password or name validation here.
	local string InPassword;

	OutError="";
	InPassword = WorldInfo.Game.ParseOption( Options, "Password" );

	if( (WorldInfo.NetMode != NM_Standalone) && WorldInfo.Game.AtCapacity(bSpectator) )
	{
		OutError=WorldInfo.Game.GameMessageClass.Default.MaxedOutMessage;
	}
	else if
	(	GamePassword!=""
	&&	caps(InPassword)!=caps(GamePassword)
	&&	(AdminPassword=="" || caps(InPassword)!=caps(AdminPassword)) )
	{
		if( InPassword == "" )
		{
			OutError = NeedPassword;
			FailCode = "NEEDPW";
		}
		else
		{
			OutError = WrongPassword;
			FailCode = "WRONGPW";
		}
	}

	if(!CheckIPPolicy(Address))
		OutError = IPBanned;
}


function bool CheckIPPolicy(string Address)
{
	local int i, j;
`if(`notdefined(FINAL_RELEASE))
	local int LastMatchingPolicy;
`endif
	local string Policy, Mask;
	local bool bAcceptAddress, bAcceptPolicy;

	// strip port number
	j = InStr(Address, ":");
	if(j != -1)
		Address = Left(Address, j);

	bAcceptAddress = True;
	for(i=0; i<IPPolicies.Length; i++)
	{
		j = InStr(IPPolicies[i], ",");
		if(j==-1)
			continue;
		Policy = Left(IPPolicies[i], j);
		Mask = Mid(IPPolicies[i], j+1);
		if(Policy ~= "ACCEPT")
			bAcceptPolicy = True;
			else if(Policy ~= "DENY")
			bAcceptPolicy = False;
		else
			continue;

		j = InStr(Mask, "*");
		if(j != -1)
		{
			if(Left(Mask, j) == Left(Address, j))
			{
				bAcceptAddress = bAcceptPolicy;
				`if(`notdefined(FINAL_RELEASE))
				LastMatchingPolicy = i;
				`endif
			}
		}
		else
		{
			if(Mask == Address)
			{
				bAcceptAddress = bAcceptPolicy;
				`if(`notdefined(FINAL_RELEASE))
				LastMatchingPolicy = i;
				`endif
			}
		}
	}

	if(!bAcceptAddress)
	{
		`Log("Denied connection for "$Address$" with IP policy "$IPPolicies[LastMatchingPolicy]);
	}

	return bAcceptAddress;
}

defaultproperties
{
	AdminClass=class'Engine.Admin'
}
