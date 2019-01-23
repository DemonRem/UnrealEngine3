/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 * Actor placed in level and used to preview different custom character combinations.
 * Does not use skeletal mesh merging, so changing parts is quick.
 */

class UTCustomChar_Preview extends Actor
	dependson(UTCustomChar_Data)
	placeable
	native;

var()	SkeletalMeshComponent	AnimComp;
var()	SkeletalMeshComponent	HeadComp;
var()	SkeletalMeshComponent	HelmetComp;
var()	SkeletalMeshComponent	FacemaskComp;
var()	SkeletalMeshComponent	GogglesComp;
var()	SkeletalMeshComponent	TorsoComp;
var()	SkeletalMeshComponent	LShoPadComp;
var()	SkeletalMeshComponent	RShoPadComp;
var()	SkeletalMeshComponent	ArmsComp;
var()	SkeletalMeshComponent	ThighsComp;
var()	SkeletalMeshComponent	BootsComp;

/** Info about character currently being used. */
var()	UTCustomChar_Data.CharacterInfo	Character;

var()	UTCustomChar_Data.CustomCharMergeState MergeState;

/**  */
var()	SkeletalMeshActor		TestMergeActor;
var()	MaterialInstanceConstant		TestMIC;
var()	MaterialInstanceConstant		TestMIC2;
var()	MaterialInstanceConstant		TestMIC3;

var		UTCharFamilyAssetStore	FamilyAssets;

cpptext
{
	/** Utility for getting the component for a particular part of the preview character. */
	USkeletalMeshComponent* GetPartComponent(BYTE Part);
}

/** Set a particular part of the preview character, based on its short ID. */
native function SetPart(UTCustomChar_Data.ECharPart InPart, string InPartID);

/** Set the entire preview character to a specified profile. */
native function SetCharacter(string InFaction, string InCharID);

/** Sets the preview character's data to the specified char data struct. */
native function SetCharacterData(UTCustomChar_Data.CustomCharData InCharData);

/** Notification when the character data has changed so the preview actor can update the data in-game. */
native function NotifyCharacterDataChanged();

event PostBeginPlay()
{
	Super.PostBeginPlay();

	if(TestMergeActor != None)
	{
		//FamilyAssets = class'UTCustomChar_Data'.static.LoadFamilyAssets("TWIF", FALSE);
		FamilyAssets = class'UTCustomChar_Data'.static.LoadFamilyAssets("IRNM", FALSE, FALSE);
		//SetCharacter(class'UTCustomChar_Data'.default.Characters[4].Faction, class'UTCustomChar_Data'.default.Characters[4].CharID);
		SetCharacter(class'UTCustomChar_Data'.default.Characters[0].Faction, class'UTCustomChar_Data'.default.Characters[0].CharID);
	}
}

/** Handler for 'SetCustomCharPart' kismet action **/
event OnSetCustomCharPart(UTSeqAct_SetCustomCharPart Action)
{
	//`log("SET PART:"@Action.Part@Action.PartID);
	SetPart(Action.Part, Action.PartID);
}

/** Handler for 'SetCustomCharCharacter' kismet action **/
event OnSetCustomCharProfile(UTSeqAct_SetCustomCharProfile Action)
{
	//`log("SET PROFILE:"@Action.Faction@Action.CharName);
	SetCharacter(Action.Faction, Action.CharID);
}

/** For testing merging - when toggled - create new merged skeletal mesh and assign to place SkeletalMeshActor.*/
event OnToggle(SeqAct_Toggle Action)
{
	//local CustomCharData CharData;

	if(TestMergeActor != None && FamilyAssets != None && !MergeState.bMergeInProgress)
	{
		// See if all assets required are loaded.
		if(FamilyAssets.NumPendingPackages == 0)
		{
			//CharData = class'UTCustomChar_Data'.static.MakeRandomCharData();
			MergeState = class'UTCustomChar_Data'.static.StartCustomCharMerge(Character.CharData, "VRed", "SK1", None, CCTR_Normal);
		}
		else
		{
			`log("Cannot merge yet - "$FamilyAssets.NumPendingPackages$" packages pending.");
		}
	}
}

event Tick(float DeltaTime)
{
	local SkeletalMesh NewMesh;
	local Texture2D HeadTex, BodyTex;
	local Texture PortraitTex;

	if(MergeState.bMergeInProgress)
	{
		HeadTex = MergeState.HeadTextures[0];
		BodyTex = MergeState.BodyTextures[0];
		NewMesh = class'UTCustomChar_Data'.static.FinishCustomCharMerge(MergeState);
		if(NewMesh != None)
		{
			PortraitTex = class'UTCustomChar_Data'.static.MakeCharPortraitTexture(NewMesh, class'UTCustomChar_Data'.default.PortraitSetup);

			TestMergeActor.SkeletalMeshComponent.SetSkeletalMesh(NewMesh);

			if(TestMIC != None)
			{
				TestMIC.SetTextureParameterValue('TestTex', HeadTex);
			}

			if(TestMIC2 != None)
			{
				TestMIC2.SetTextureParameterValue('TestTex', BodyTex);
			}

			if(TestMIC3 != None)
			{
				TestMIC3.SetTextureParameterValue('TestTex', PortraitTex);
			}
		}
	}
}

defaultproperties
{
	Begin Object Class=AnimNodeSequence Name=AnimNodeSeq0
		AnimSeqName="idle_ready_rif"
		bLooping=true
		bPlaying=true
	End Object

	Begin Object Class=SkeletalMeshComponent Name=MyAnimComp
		HiddenGame=true
		SkeletalMesh=SkeletalMesh'CH_IronGuard_Male.Mesh.SK_CH_IronGuard_MaleA'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		Animations=AnimNodeSeq0
	End Object
	AnimComp=MyAnimComp
	Components.Add(MyAnimComp)

	Begin Object Class=SkeletalMeshComponent Name=MyTorsoComp
		ParentAnimComponent=MyAnimComp
	End Object
	TorsoComp=MyTorsoComp
	Components.Add(MyTorsoComp)

	Begin Object Class=SkeletalMeshComponent Name=MyHeadComp
		ParentAnimComponent=MyAnimComp
	End Object
	HeadComp=MyHeadComp
	Components.Add(MyHeadComp)

	Begin Object Class=SkeletalMeshComponent Name=MyHelmetComp
		ParentAnimComponent=MyAnimComp
	End Object
	HelmetComp=MyHelmetComp
	Components.Add(MyHelmetComp)

	Begin Object Class=SkeletalMeshComponent Name=MyFacemaskComp
		ParentAnimComponent=MyAnimComp
	End Object
	FacemaskComp=MyFacemaskComp
	Components.Add(MyFacemaskComp)

	Begin Object Class=SkeletalMeshComponent Name=MyGogglesComp
		ParentAnimComponent=MyAnimComp
	End Object
	GogglesComp=MyGogglesComp
	Components.Add(MyGogglesComp)

	Begin Object Class=SkeletalMeshComponent Name=MyLShoPadComp
		ParentAnimComponent=MyAnimComp
	End Object
	LShoPadComp=MyLShoPadComp
	Components.Add(MyLShoPadComp)

	Begin Object Class=SkeletalMeshComponent Name=MyRShoPadComp
		ParentAnimComponent=MyAnimComp
	End Object
	RShoPadComp=MyRShoPadComp
	Components.Add(MyRShoPadComp)

	Begin Object Class=SkeletalMeshComponent Name=MyArmsComp
		ParentAnimComponent=MyAnimComp
	End Object
	ArmsComp=MyArmsComp
	Components.Add(MyArmsComp)

	Begin Object Class=SkeletalMeshComponent Name=MyThighsComp
		ParentAnimComponent=MyAnimComp
	End Object
	ThighsComp=MyThighsComp
	Components.Add(MyThighsComp)

	Begin Object Class=SkeletalMeshComponent Name=MyBootsComp
		ParentAnimComponent=MyAnimComp
	End Object
	BootsComp=MyBootsComp
	Components.Add(MyBootsComp)
}