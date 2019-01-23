class UTVehicleCTFGame extends UTCTFGame
	abstract;

static function PrecacheGameAnnouncements(UTAnnouncer Announcer)
{
	Super.PrecacheGameAnnouncements(Announcer);
	class'UTVehicleCantCarryFlagMessage'.static.PrecacheGameAnnouncements(Announcer, default.Class);
}

// Returns whether a mutator should be allowed with this gametype
static function bool AllowMutator( string MutatorClassName )
{
	if ( (MutatorClassName ~= "UTGame.UTMutator_Instagib") || (MutatorClassName ~= "UTGame.UTMutator_WeaponsRespawn")
		|| (MutatorClassName ~= "UTGame.UTMutator_LowGrav") )
	{
		return false;
	}
	return Super.AllowMutator(MutatorClassName);
}

defaultproperties
{
	MapPrefix="VCTF"
	Acronym="VCTF"

	bAllowVehicles=true
	bAllowHoverboard=true
	bAllowTranslocator=false
}
