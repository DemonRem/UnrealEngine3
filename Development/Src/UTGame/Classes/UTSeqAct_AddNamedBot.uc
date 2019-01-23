class UTSeqAct_AddNamedBot extends SequenceAction;

/** name of bot to spawn */
var() string BotName;
/** reference to bot controller so Kismet can work with it further */
var UTBot SpawnedBot;

event Activated()
{
	local UTGame Game;

	Game = UTGame(GetWorldInfo().Game);
	if (Game != None)
	{
		SpawnedBot = Game.AddNamedBot(BotName);
		if (SpawnedBot != None && SpawnedBot.Pawn == None)
		{
			Game.RestartPlayer(SpawnedBot);
		}
	}
}

defaultproperties
{
	ObjCategory="AI"
	ObjName="Add Named Bot"
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Bot",bWriteable=true,PropertyName=SpawnedBot)
}
