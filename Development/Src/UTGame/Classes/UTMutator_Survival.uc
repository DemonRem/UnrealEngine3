class UTMutator_Survival extends UTMutator;

function PostBeginPlay()
{
	local UTDuelGame Game;

	Super.PostBeginPlay();

	Game = UTDuelGame(WorldInfo.Game);
	if (Game == None)
	{
		`Warn("Survival mutator only works in Duel");
		Destroy();
	}
	else
	{
		Game.bRotateQueueEachKill = true;
		Game.NumRounds = 1;
	}
}
