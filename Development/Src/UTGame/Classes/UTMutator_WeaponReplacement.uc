/** replaces weapons with other weapons (including ammo) */
class UTMutator_WeaponReplacement extends UTMutator
	config(Game);

struct ReplacementInfo
{
	/** class name of the weapon we want to get rid of */
	var name OldClassName;
	/** fully qualified path of the class to replace it with */
	var string NewClassPath;
};

var config array<ReplacementInfo> WeaponsToReplace;
var config array<ReplacementInfo> AmmoToReplace;

function bool CheckReplacement(Actor Other)
{
	local UTWeaponPickupFactory WeaponPickup;
	local UTWeaponLocker Locker;
	local UTAmmoPickupFactory AmmoPickup, NewAmmo;
	local int i, Index;
	local class<UTAmmoPickupFactory> NewAmmoClass;

	WeaponPickup = UTWeaponPickupFactory(Other);
	if (WeaponPickup != None)
	{
		if (WeaponPickup.WeaponPickupClass != None)
		{
			Index = WeaponsToReplace.Find('OldClassName', WeaponPickup.WeaponPickupClass.Name);
			if (Index != INDEX_NONE)
			{
				if (WeaponsToReplace[Index].NewClassPath == "")
				{
					// replace with nothing
					return false;
				}
				WeaponPickup.WeaponPickupClass = class<UTWeapon>(DynamicLoadObject(WeaponsToReplace[Index].NewClassPath, class'Class'));
			}
		}
	}
	else
	{
		Locker = UTWeaponLocker(Other);
		if (Locker != None)
		{
			for (i = 0; i < Locker.Weapons.length; i++)
			{
				if (Locker.Weapons[i].WeaponClass != None)
				{
					Index = WeaponsToReplace.Find('OldClassName', Locker.Weapons[i].WeaponClass.Name);
					if (Index != INDEX_NONE)
					{
						if (WeaponsToReplace[Index].NewClassPath == "")
						{
							// replace with nothing
							Locker.ReplaceWeapon(i, None);
						}
						else
						{
							Locker.ReplaceWeapon(i, class<UTWeapon>(DynamicLoadObject(WeaponsToReplace[Index].NewClassPath, class'Class')));
						}
					}
				}
			}
		}
		else
		{
			AmmoPickup = UTAmmoPickupFactory(Other);
			if (AmmoPickup != None)
			{
				Index = AmmoToReplace.Find('OldClassName', AmmoPickup.Class.Name);
				if (Index != INDEX_NONE)
				{
					if (AmmoToReplace[Index].NewClassPath == "")
					{
						// replace with nothing
						return false;
					}
					NewAmmoClass = class<UTAmmoPickupFactory>(DynamicLoadObject(AmmoToReplace[Index].NewClassPath, class'Class'));
					if (NewAmmoClass == None)
					{
						// replace with nothing
						return false;
					}
					else if (NewAmmoClass.default.bStatic || NewAmmoClass.default.bNoDelete)
					{
						// transform the current ammo into the desired class
						AmmoPickup.TransformAmmoType(NewAmmoClass);
						return true;
					}
					else
					{
						// spawn the new ammo, link it to the old, then disable the old one
						NewAmmo = AmmoPickup.Spawn(NewAmmoClass);
						NewAmmo.OriginalFactory = AmmoPickup;
						AmmoPickup.ReplacementFactory = NewAmmo;
						return false;
					}
				}
			}
		}
	}

	return true;
}

defaultproperties
{
	GroupName="WEAPONMOD"
}
