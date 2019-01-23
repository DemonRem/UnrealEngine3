class UTSeqEvent_GameEnded extends SequenceEvent;

/** The winner of the game. In Deathmatch, the player with the final kill; in other gametypes, the home base of the winning team */
var Actor Winner;

event Activated()
{
	local UTGame Game;

	Game = UTGame(GetWorldInfo().Game);
	if (Game != None)
	{
		Winner = Game.EndGameFocus;
	}
}

defaultproperties
{
	ObjName="Game Ended"
	bPlayerOnly=false
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Winning Player/Base",bWriteable=true,PropertyName=Winner)
}
