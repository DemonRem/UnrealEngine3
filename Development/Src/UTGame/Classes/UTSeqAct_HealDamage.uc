class UTSeqAct_HealDamage extends SeqAct_HealDamage;

/** heal UTPawns up to SuperHealthMax instead of their normal max health */
var() bool bSuperHeal;

defaultproperties
{
	ObjName="Heal Damage (UT)"
	HandlerName=OnHealDamage
}
