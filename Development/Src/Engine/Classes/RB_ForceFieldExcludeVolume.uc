//=============================================================================
// RB_ForceFieldExcludeVolume:  a bounding volume which removes the effect of a physical force field.
// Currently: RB_CylindricalForceActor
//=============================================================================
class RB_ForceFieldExcludeVolume extends Volume
	native(Physics)
	placeable;

/** Used to identify which force fields this excluder applies to, ie if the channel ID matches then the excluder 
excludes this force field*/
var()	int ForceFieldChannel;

defaultproperties
{
	Begin Object Name=BrushComponent0
		CollideActors=true
		BlockActors=false
		BlockZeroExtent=false
		BlockNonZeroExtent=false
		BlockRigidBody=false
	End Object
	
	bCollideActors=True
	ForceFieldChannel=1
}
