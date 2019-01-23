/** client side event triggered by power core when it wants to play its destruction effect
 * use this to make a level specific effect instead of the default
 */
class UTSeqEvent_PowerCoreDestructionEffect extends SequenceEvent;

/** skeletal mesh actor the power core spawns (for e.g. matinee control) */
var SkeletalMeshActorSpawnable MeshActor;

defaultproperties
{
	bPlayerOnly=false
	bClientSideOnly=true
	ObjName="Play Core Destruction"
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Skeletal Mesh Actor",bWriteable=true,PropertyName=MeshActor)
}
