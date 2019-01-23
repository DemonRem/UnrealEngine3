/** used for gameplay-relevant ambient sounds (e.g. weapon loading sounds) */
class UTAmbientSoundComponent extends AudioComponent;

event OcclusionChanged(bool bNowOccluded)
{
	Super.OcclusionChanged(bNowOccluded);
}

defaultproperties
{
	OcclusionCheckInterval=1.0
	bShouldRemainActiveIfDropped=true
	bStopWhenOwnerDestroyed=true
}
