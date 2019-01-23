/**
 * ExamplePawn
 * Demo pawn demonstrating animating character.
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class ExamplePawn extends GamePawn
	config(Game);

/*
UDebugger tests
state A
{
Begin:
	`log("here i am in state A");
	while ( true )
	{
		Sleep(0.1f);
	}
}

state B
{
LabelA:
	Sleep(0.1f);
	Goto('LabelA');
}


state C
{
Begin:
	`log("here i am in state c");
}

exec function StartTest()
{
	GotoState('A');
}

exec function FinishTest()
{
	PushState('C');
}

exec function PopIt()
{
	PopState();
	`log("Popped state C");
}

state SomeState
{
}


exec function TestPushState()
{
	PushState('SomeState');

	`log("Pushed SomeState");
}

exec function TestPopState()
{
	PopState();

	`log("Popped SomeState");
}



// TTPRO #16702
exec function Test16702()
{
	GotoState('Chase');
}

function GotoAttackState()
{
	GotoState( 'Attack' );
}

function SetMoveGoal()
{
	GotoMoveToGoalState();
}

function GotoMoveToGoalState()
{
	PushState( 'MoveToGoalAttack' );
}

state Chase
{
Begin:
	SetMoveGoal();

	while ( true )
	{
		Sleep( 0.2f );

		GotoAttackState();
		//GotoState( 'Attack' );
	}
}

state MoveToGoal
{
	function BeginState(Name PreviousStateName)
	{
		SetTimers();
	}

	function SetTimers()
	{
		SetTimer(0.1f, true, 'MoveToGoalTimerUpdate' );
	}

AbortMove:
	PopState();

Begin:
	SetTimers();
}

state MoveToGoalAttack extends MoveToGoal
{
	function MoveToGoalTimerUpdate()
	{
		GotoState( GetStateName(), 'AbortMove', false, true );
	}
}

*/

defaultproperties
{
	Components.Remove(Sprite)

	Begin Object Class=SkeletalMeshComponent Name=DemoPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'COG_Grunt.COG_Grunt_AMesh'
		PhysicsAsset=PhysicsAsset'COG_Grunt.COG_Grunt_AMesh_Physics'
		AnimSets(0)=AnimSet'COG_Grunt.COG_Grunt_BasicAnims'
		AnimTreeTemplate=AnimTree'COG_Grunt.Grunt_AnimTree'
		bOwnerNoSee=false
	End Object
	Mesh=DemoPawnSkeletalMeshComponent
	Components.Add(DemoPawnSkeletalMeshComponent)

	bStasis=false
}
