/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// CheatManager
// Object within playercontroller that manages "cheat" commands
// only spawned in single player mode
//=============================================================================

class UTCheatManager extends CheatManager within PlayerController
	native;

var class<LocalMessage> LMC;

exec function LM( string MessageClassName )
{
	LMC = class<LocalMessage>(DynamicLoadObject(MessageClassName, class'Class'));
}

exec function LMS( int switch )
{
	ReceiveLocalizedMessage(LMC, switch, PlayerReplicationInfo, PlayerReplicationInfo);
}

/** Summon a vehicle */
exec function SummonV( string ClassName )
{
	local class<actor> NewClass;
	local vector SpawnLoc;

	`log( "Fabricate " $ ClassName );
	NewClass = class<actor>( DynamicLoadObject( "UTGameContent.UTVehicle_"$ClassName, class'Class' ) );
	if ( NewClass == None )
	{
		NewClass = class<actor>( DynamicLoadObject( "UTGameContent.UTVehicle_"$ClassName$"_Content", class'Class' ) );
	}
	if( NewClass!=None )
	{
		if ( Pawn != None )
			SpawnLoc = Pawn.Location;
		else
			SpawnLoc = Location;
		Spawn( NewClass,,,SpawnLoc + 72 * Vector(Rotation) + vect(0,0,1) * 15 );
	}
}

// @TODO FIXMESTEVE - temp cheat until all weapons final
exec function FullyLoaded()
{
	local bool bTranslocatorBanned;
	local UTVehicleFactory VF;
	if( WorldInfo.Netmode!=NM_Standalone )
		return;

	GiveWeapon("UTGameContent.UTWeap_Avril_Content");
	GiveWeapon("UTGame.UTWeap_BioRifle");
	GiveWeapon("UTGame.UTWeap_FlakCannon");
	GiveWeapon("UTGame.UTWeap_LinkGun");
	GiveWeapon("UTGameContent.UTWeap_Redeemer_Content");
	GiveWeapon("UTGame.UTWeap_RocketLauncher");
	GiveWeapon("UTGame.UTWeap_ShockRifle");
	GiveWeapon("UTGame.UTWeap_SniperRifle");
	GiveWeapon("UTGame.UTWeap_Stinger");
	GiveWeapon("UTGame.UTWeap_InstagibRifle");
	bTranslocatorBanned = false;
	ForEach WorldInfo.AllNavigationPoints(class'UTVehicleFactory', VF)
	{
		bTranslocatorBanned = true;
	}
	if(!bTranslocatorBanned)
	{
		GiveWeapon("UTGameContent.UTWeap_Translocator_Content");
	}

    AllAmmo();
}

/* AllWeapons
	Give player all available weapons
*/
exec function AllWeapons()
{
	local bool bTranslocatorBanned;
	local UTVehicleFactory VF;
	if( (WorldInfo.NetMode!=NM_Standalone) || (Pawn == None) )
		return;

	GiveWeapon("UTGameContent.UTWeap_Avril_Content");
	GiveWeapon("UTGame.UTWeap_BioRifle");
	GiveWeapon("UTGame.UTWeap_FlakCannon");
	GiveWeapon("UTGame.UTWeap_LinkGun");
	GiveWeapon("UTGameContent.UTWeap_Redeemer_Content");
	GiveWeapon("UTGame.UTWeap_RocketLauncher");
	GiveWeapon("UTGame.UTWeap_ShockRifle");
	GiveWeapon("UTGame.UTWeap_SniperRifle");
	GiveWeapon("UTGame.UTWeap_Stinger");
	bTranslocatorBanned = false;
	ForEach WorldInfo.AllNavigationPoints(class'UTVehicleFactory', VF)
	{
		bTranslocatorBanned = true;
	}
	if(!bTranslocatorBanned)
	{
		GiveWeapon("UTGameContent.UTWeap_Translocator_Content");
	}
}

exec function DoubleUp()
{
	local UTWeap_Enforcer MyEnforcer;

	MyEnforcer = UTWeap_Enforcer(Pawn.FindInventoryType(class'UTWeap_Enforcer'));
	MyEnforcer.DenyPickupQuery(class'UTWeap_Enforcer', None);
}

exec function PhysicsGun()
{
	if (Pawn != None)
	{
		GiveWeapon("UTGameContent.UTWeap_PhysicsGun");
	}
}

/* AllAmmo
	Sets maximum ammo on all weapons
*/
exec function AllAmmo()
{
	if ( (Pawn != None) && (UTInventoryManager(Pawn.InvManager) != None) )
	{
		UTInventoryManager(Pawn.InvManager).AllAmmo(true);
		UTInventoryManager(Pawn.InvManager).bInfiniteAmmo = true;
	}
}

exec function Invisible(bool B)
{
	Pawn.SetHidden(B);

	if ( UTPawn(Pawn) != None )
		UTPawn(Pawn).bIsInvisible = B;
}

exec function FreeCamera()
{
		UTPlayerController(Outer).bFreeCamera = !UTPlayerController(Outer).bFreeCamera;
		UTPlayerController(Outer).SetBehindView(UTPlayerController(Outer).bFreeCamera);
}

exec function ViewBot()
{
	local Controller first;
	local bool bFound;
	local AIController C;

	foreach WorldInfo.AllControllers(class'AIController', C)
	{
		if (C.Pawn != None && C.PlayerReplicationInfo != None)
		{
			if (bFound || first == None)
			{
				first = C;
				if (bFound)
				{
					break;
				}
			}
			if (C.PlayerReplicationInfo == RealViewTarget)
			{
				bFound = true;
			}
		}
	}

	if ( first != None )
	{
		SetViewTarget(first);
		UTPlayerController(Outer).SetBehindView(true);
		UTPlayerController(Outer).bFreeCamera = true;
		FixFOV();
	}
	else
		ViewSelf(true);
}

exec function KillBadGuys()
{
	local playercontroller PC;
	local UTPawn p;

	PC = UTPlayerController(Outer);

	if (PC!=none)
	{
		ForEach DynamicActors(class'UTPawn', P)
		{
			if ( !WorldInfo.GRI.OnSameTeam(P,PC) )
			{
				P.TakeDamage(20000,P.Controller, P.Location, Vect(0,0,0),class'UTDmgType_Rocket');
			}
		}
	}
}

exec function VehicleZoom()
{
    if (UTVehicle(Pawn) != None)
	UTVehicle(Pawn).bUnlimitedCameraDistance = !UTVehicle(Pawn).bUnlimitedCameraDistance;
}

exec function RBGrav(float NewGravityScaling)
{
	WorldInfo.RBPhysicsGravityScaling = NewGravityScaling;
}

/** allows suiciding with a specific damagetype and health value for testing death effects */
exec function SuicideBy(string Type, optional int DeathHealth)
{
	local class<DamageType> DamageType;

	if (Pawn != None)
	{
		if (InStr(Type, ".") == -1)
		{
			Type = "UTGame." $ Type;
		}
		DamageType = class<DamageType>(DynamicLoadObject(Type, class'Class'));
		if (DamageType != None)
		{
			Pawn.Health = DeathHealth;
			if (Pawn.IsA('UTPawn'))
			{
				UTPawn(Pawn).AccumulateDamage = -DeathHealth;
				UTPawn(Pawn).AccumulationTime = WorldInfo.TimeSeconds;
			}
			Pawn.Died(Outer, DamageType, Pawn.Location);
		}
	}
}

exec function EditWeapon(string WhichWeapon)
{
	local utweapon Weapon;
	local array<string> weaps;
	local string s;
	local int i;
	if (WhichWeapon != "")
	{
		ConsoleCommand("Editactor class="$WhichWeapon);
	}
	else
	{
		foreach AllActors(class'UTWeapon',Weapon)
		{
			s = ""$Weapon.Class;
			if ( Weaps.Find(s) < 0 )
			{
				Weaps.Length = Weaps.Length + 1;
				Weaps[Weaps.Length-1] = s;
			}
		}

		for (i=0;i<Weaps.Length;i++)
		{
			`log("Weapon"@i@"="@Weaps[i]);
		}
	}
}

