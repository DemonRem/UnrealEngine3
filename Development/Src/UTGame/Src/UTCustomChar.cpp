/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UTGame.h"
#include "UnSkeletalMeshMerge.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(AUTCustomChar_Preview);
IMPLEMENT_CLASS(UUTCustomChar_Data);
IMPLEMENT_CLASS(UUTCharFamilyAssetStore);
IMPLEMENT_CLASS(UUTFamilyInfo);

static void HelmetAllowsFaceParts(const TArray<FCustomCharPart>& Parts, const FString& InFamilyID, const FString& InHelmetID, UBOOL& bOutNoFacemask, UBOOL& bOutNoGoggles)
{
	bOutNoGoggles = FALSE;
	bOutNoFacemask = FALSE;

	for(INT i=0; i<Parts.Num(); i++)
	{
		const FCustomCharPart& PartInfo = Parts(i);
		if( PartInfo.FamilyID == InFamilyID &&
			PartInfo.Part == PART_Helmet &&
			PartInfo.PartID == InHelmetID )
		{			
			bOutNoGoggles = PartInfo.bNoGoggles;
			bOutNoFacemask = PartInfo.bNoFacemask;
		}
	}
}

FString GetPartIDFromData(const FCustomCharData& InCharData, BYTE InPart)
{
	switch(InPart)
	{
	case(PART_Head): return InCharData.HeadID;
	case(PART_Helmet): return InCharData.HelmetID;
	case(PART_Facemask): return InCharData.FacemaskID;
	case(PART_Goggles): return InCharData.GogglesID;
	case(PART_Torso): return InCharData.TorsoID;
	case(PART_ShoPad): return InCharData.ShoPadID;
	case(PART_Arms): return InCharData.ArmsID;
	case(PART_Thighs): return InCharData.ThighsID;
	case(PART_Boots): return InCharData.BootsID;
	}
	return FString(TEXT(""));
}

//////////////////////////////////////////////////////////////////////////
// AUTCustomChar_Preview
//////////////////////////////////////////////////////////////////////////

/** 
 *	Utility for getting the component for a particular part of the preview character. 
 *	Passing in PART_ShoPad is invalid, as there are two components for that.
 */
USkeletalMeshComponent* AUTCustomChar_Preview::GetPartComponent(BYTE Part)
{
	if(Part == PART_Head)
	{
		return HeadComp;
	}
	else if(Part == PART_Helmet)
	{
		return HelmetComp;
	}
	else if(Part == PART_Facemask)
	{
		return FacemaskComp;
	}
	else if(Part == PART_Goggles)
	{
		return GogglesComp;
	}
	else if(Part == PART_Torso)
	{
		return TorsoComp;
	}
	else if(Part == PART_Arms)
	{
		return ArmsComp;
	}
	else if(Part == PART_Thighs)
	{
		return ThighsComp;
	}
	else if(Part == PART_Boots)
	{
		return BootsComp;
	}
	else
	{
		appErrorf(TEXT("Invalid Part"));
		return NULL;
	}
}

/** Util for taking 'generic' shoulder pad name and making it left or right. */
FString GetShoulderObjName(const FString& ShoPadName, UBOOL bLefShoulder)
{
	INT SidePos = ShoPadName.InStr(TEXT("XShoPad"));
	if(SidePos == INDEX_NONE)
	{
		debugf(TEXT("Not Valid Shoulder Name: %s"), *ShoPadName);
		return FString(TEXT(""));
	}

	FString OutName = ShoPadName;
	if(bLefShoulder)
	{
		OutName[SidePos] = TCHAR('L');
	}
	else
	{
		OutName[SidePos] = TCHAR('R');
	}
	return OutName;
}

/** Set a particular part of the preview character, based on its short ID. */
void AUTCustomChar_Preview::SetPart(BYTE InPart, const FString& InPartID)
{
	// Find the object name for the part we want.
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());

	// Have to do some special work for shoulder pads - two skeletal meshes.
	if(InPart == PART_ShoPad)
	{
		check(LShoPadComp);
		check(RShoPadComp);

		// Left Shoulder
		USkeletalMesh* LeftSkelMesh = Data->FindPartSkelMesh(Character.CharData.FamilyID, PART_ShoPad, InPartID, TRUE, Character.CharData.BasedOnCharID);
		if(LeftSkelMesh)
		{
			LShoPadComp->SetSkeletalMesh(LeftSkelMesh, TRUE);
		}
		LShoPadComp->SetHiddenGame(!Character.CharData.bHasLeftShoPad);

		// Right Shoulder
		USkeletalMesh* RightSkelMesh = Data->FindPartSkelMesh(Character.CharData.FamilyID, PART_ShoPad, InPartID, FALSE, Character.CharData.BasedOnCharID);
		if(RightSkelMesh)
		{
			RShoPadComp->SetSkeletalMesh(RightSkelMesh, TRUE);
		}
		RShoPadComp->SetHiddenGame(!Character.CharData.bHasRightShoPad);
	}
	else
	{
		USkeletalMeshComponent* Comp = GetPartComponent(InPart);
		check(Comp);

		USkeletalMesh* SkelMesh = Data->FindPartSkelMesh(Character.CharData.FamilyID, InPart, InPartID, FALSE, Character.CharData.BasedOnCharID);
		if(SkelMesh)
		{
			// Get the component for that part, and assign the new skel mesh.
			Comp->SetSkeletalMesh(SkelMesh, TRUE);
			Comp->SetHiddenGame(false);
		}
		else
		{
			Comp->SetHiddenGame(true);
		}
	}

	// Save the string of this part into the slot in the character info, so its up to date.
	if(InPart == PART_Head)
	{
		Character.CharData.HeadID = InPartID;
	}
	else if(InPart == PART_Helmet)
	{
		Character.CharData.HelmetID = InPartID;
	}
	else if(InPart == PART_Facemask)
	{
		Character.CharData.FacemaskID = InPartID;
	}
	else if(InPart == PART_Goggles)
	{
		Character.CharData.GogglesID = InPartID;
	}
	else if(InPart == PART_Torso)
	{
		Character.CharData.TorsoID = InPartID;
	}
	else if(InPart == PART_ShoPad)
	{
		Character.CharData.ShoPadID = InPartID;
	}
	else if(InPart == PART_Arms)
	{
		Character.CharData.ArmsID = InPartID;
	}
	else if(InPart == PART_Thighs)
	{
		Character.CharData.ThighsID = InPartID;
	}
	else if(InPart == PART_Boots)
	{
		Character.CharData.BootsID = InPartID;
	}


	// Handle helmet possibly blocking display of goggles and/or facemask
	if(InPart == PART_Facemask || InPart == PART_Goggles)
	{
		UBOOL bNoFacemask = FALSE;
		UBOOL bNoGoggles = FALSE;
		HelmetAllowsFaceParts(Data->Parts, Character.CharData.FamilyID, Character.CharData.HelmetID, bNoFacemask, bNoGoggles );

		if(bNoFacemask)
		{
			FacemaskComp->SetHiddenGame(true);
		}

		if(bNoGoggles)
		{
			GogglesComp->SetHiddenGame(true);
		}
	}
	// When helmet itself changes - just re-set the facemask and goggle parts, so they re-check the helmet and possbily get hidden.
	else if(InPart == PART_Helmet)
	{
		SetPart(PART_Facemask, Character.CharData.FacemaskID);
		SetPart(PART_Goggles, Character.CharData.GogglesID);
	}
}

/** Set the entire preview character to a specified profile. */
void AUTCustomChar_Preview::SetCharacter(const FString& InFaction, const FString& InCharID)
{
	// Find the object name for the part we want.
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());
	FCharacterInfo NewCharacter = Data->FindCharacter(InFaction, InCharID);

	// If we found a profile, apply it to this preview actor.
	if(NewCharacter.CharName != FString(TEXT("")))
	{
		Character.Faction = NewCharacter.Faction;

		FCustomCharData& NewData = NewCharacter.CharData;
		Character.CharData.BasedOnCharID = NewCharacter.CharID;
		SetCharacterData(NewData);
	}
	else
	{
		debugf(TEXT("No Character '%s' in Faction '%s' found"), *InCharID, *InFaction);
	}
}

/** Sets the preview character's data to the specified char data struct. */
void AUTCustomChar_Preview::SetCharacterData(FCustomCharData InCharData)
{
	Character.CharData.FamilyID = InCharData.FamilyID;

	// Find the object name for the part we want.
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());
	UClass* FamilyInfoClass = Data->FindFamilyInfo(Character.CharData.FamilyID);
	if(FamilyInfoClass && FamilyInfoClass->IsChildOf(UUTFamilyInfo::StaticClass()))
	{
		UUTFamilyInfo* FamilyInfo = CastChecked<UUTFamilyInfo>(FamilyInfoClass->GetDefaultObject());
		check(FamilyInfo->MasterSkeleton);
		check(AnimComp);
		AnimComp->AnimSets = FamilyInfo->AnimSets;
		AnimComp->SetSkeletalMesh(FamilyInfo->MasterSkeleton);
	}

	// Set each part based on the new profile
	SetPart(PART_Head, InCharData.HeadID);
	SetPart(PART_Helmet, InCharData.HelmetID);
	SetPart(PART_Facemask, InCharData.FacemaskID);
	SetPart(PART_Goggles, InCharData.GogglesID);

	SetPart(PART_Torso, InCharData.TorsoID);

	Character.CharData.bHasLeftShoPad = InCharData.bHasLeftShoPad;
	Character.CharData.bHasRightShoPad = InCharData.bHasRightShoPad;
	SetPart(PART_ShoPad, InCharData.ShoPadID);

	SetPart(PART_Arms, InCharData.ArmsID);
	SetPart(PART_Thighs, InCharData.ThighsID);
	SetPart(PART_Boots, InCharData.BootsID);
}



/** Notification when the character data has changed so the preview actor can update the data in-game. */
void AUTCustomChar_Preview::NotifyCharacterDataChanged()
{
	//@todo: This is probably not the best way to do this, reset all the parts for now.
	SetPart(PART_Head, Character.CharData.HeadID);
	SetPart(PART_Helmet, Character.CharData.HelmetID);
	SetPart(PART_Facemask, Character.CharData.FacemaskID);
	SetPart(PART_Goggles, Character.CharData.GogglesID);
	SetPart(PART_Torso, Character.CharData.TorsoID);
	SetPart(PART_ShoPad, Character.CharData.ShoPadID);
	SetPart(PART_Arms, Character.CharData.ArmsID);
	SetPart(PART_Thighs, Character.CharData.ThighsID);
	SetPart(PART_Boots, Character.CharData.BootsID);
}

//////////////////////////////////////////////////////////////////////////
// UUTCustomChar_Data
//////////////////////////////////////////////////////////////////////////

/** Saves the character data to an INI file. */
#if 0
void UUTCustomChar_Data::TempSaveDataToINI(const FString& Data)
{

#if !CONSOLE
	GConfig->SetString(TEXT("TempData"), TEXT("CustomChar"), *Data, GGameIni);
	GConfig->Flush(FALSE);
#endif

}
#endif

/** Loads the character data from an INI file. */
#if 0
UBOOL UUTCustomChar_Data::TempLoadDataFromINI(FString &OutData)
{
	UBOOL bResult = FALSE;

#if !CONSOLE
	bResult = GConfig->GetString(TEXT("TempData"), TEXT("CustomChar"), OutData, GGameIni);
#endif

	return bResult;
}
#endif

/** Given a family, part and ID string, give the SkeletalMesh object name to use. */
FString UUTCustomChar_Data::FindPartObjName(const FString& InFamilyID, BYTE InPart, const FString& InPartID)
{
	//debugf(TEXT("Checking %d Parts for %s"), Parts.Num(), *InPartID);

	// Linear search over parts in database, looking for one that matches
	for(INT i=0; i<Parts.Num(); i++)
	{
		FCustomCharPart& PartInfo = Parts(i);
		if( PartInfo.FamilyID == InFamilyID &&
			PartInfo.Part == InPart &&
			PartInfo.PartID == InPartID )
		{
			// Found a match - return its object name.
			return PartInfo.ObjectName;
		}
	}

	// No match - return empty string.
	return FString(TEXT(""));
}

/** */
USkeletalMesh* UUTCustomChar_Data::FindPartSkelMesh(const FString& InFamilyID, BYTE InPart, const FString& InPartId, UBOOL bLeftShoPad, const FString& InBasedOnCharID)
{
	// No string in - no mesh out.
	if(InPartId == FString(TEXT("")) || InPartId == FString(TEXT("NONE")))
	{
		return NULL;
	}

	FString PartName = FindPartObjName(InFamilyID, InPart, InPartId);

	// If we didn't get a part obj name - it might be that we are missing the part package - so just fall back to the 'based on' char.
	if(PartName == FString(TEXT("")))
	{
		debugf(TEXT("Cannot find PartID '%s' - Falling back to BasedOnChar."), *InPartId);
		UClass* FamilyClass = FindFamilyInfo(InFamilyID);
		if(FamilyClass)
		{
			UUTFamilyInfo* FamilyInfo = CastChecked<UUTFamilyInfo>(FamilyClass->GetDefaultObject());
			FCharacterInfo BasedOnCharInfo = FindCharacter(FamilyInfo->Faction, InBasedOnCharID);
			if(BasedOnCharInfo.CharData.FamilyID == InFamilyID)
			{
				FString BaseOnCharPartID = GetPartIDFromData(BasedOnCharInfo.CharData, InPart);
				PartName = FindPartObjName(InFamilyID, InPart, BaseOnCharPartID);
			}
		}
	}

	if(PartName != FString(TEXT("")))
	{
		if(InPart == PART_ShoPad)
		{
			PartName = GetShoulderObjName(PartName, bLeftShoPad);
		}

		// Assume objects are loaded - try and find it now.
		USkeletalMesh* PartSkelMesh = FindObject<USkeletalMesh>(ANY_PACKAGE, *PartName);
		if(PartSkelMesh)
		{
			return PartSkelMesh;
		}
		else
		{
			debugf(TEXT("FindPartSkelMesh: Failed to find/load part: %s"), *PartName);
		}
	}
	else
	{
		debugf(TEXT("FindPartSkelMesh: Failed to find Family:%s Part:%d PartID:%s"), *InFamilyID, InPart, *InPartId);
	}

	return NULL;
}

/** Given a faction and character name, find the profile that defines all its parts. */
FCharacterInfo UUTCustomChar_Data::FindCharacter(const FString& InFaction, const FString& InCharID)
{
	//debugf(TEXT("Checking %d Profiles for %s"), Profiles.Num(), *InCharName);

	for(INT i=0; i<Characters.Num(); i++)
	{
		FCharacterInfo& TestChar = Characters(i);
		if( TestChar.Faction == InFaction &&
			TestChar.CharID == InCharID )
		{
			return TestChar;
		}
	}

	// No match - return empty profile.
	FCharacterInfo EmptyChar;
	appMemzero(&EmptyChar, sizeof(FCharacterInfo));
	return EmptyChar;
}

/** Find the info class for a particular family */
UClass* UUTCustomChar_Data::FindFamilyInfo(const FString& InFamilyID)
{
	for(INT i=0; i<Families.Num(); i++)
	{
		UClass* InfoClass = Families(i);
		if(InfoClass)
		{
			UUTFamilyInfo* Info = CastChecked<UUTFamilyInfo>(InfoClass->GetDefaultObject());
			if(Info->FamilyID == InFamilyID)
			{
				return Families(i);
			}
		}
	}

	return NULL;
}

/** Callback executed when each asset package has finished async loading. */
static void AsyncLoadFamilyAssetsCompletionCallback(UObject* Package, void* InAssetStore)
{
	UUTCharFamilyAssetStore* Store = (UUTCharFamilyAssetStore*)InAssetStore;

	debugf(TEXT("Family Asset Package Loaded: %s"), *Package->GetName());
	(Store->NumPendingPackages)--;

	// Add all objects within this package to the asset store, to stop them from being GC'd
	for( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Obj = *It;
		if (Obj->IsIn(Package))
		{
			Store->FamilyAssets.AddItem(Obj);
		}
	}

	// If we finished loading, print how long it took.
	if(GWorld && Store->NumPendingPackages == 0)
	{
		debugf(TEXT("CONSTRUCTIONING: LoadFamilyAsset (%s) Took: %f secs"), *Store->FamilyID, GWorld->GetRealTimeSeconds() - Store->StartLoadTime);
	}
}

/** 
 *	This function initiates async loading of all the assets for a particular custom character family. 
 *	When UUTCharFamilyAssetStore.NumPendingPackages hits zero, all assets are in place and you can begin creating merged characters.
 *	@param bBlocking	If true, game will block until all assets are loaded.
 *	@param bArms		Load package containing arm mesh for this family
 */
UUTCharFamilyAssetStore* UUTCustomChar_Data::LoadFamilyAssets(const FString& InFamilyID, UBOOL bBlocking, UBOOL bArms)
{
	// First find all packages required by parts of this family
	TArray<FString> FamilyPackageNames;

	if(bArms)
	{
		if(InFamilyID == TEXT("") || InFamilyID == TEXT("NONE"))
		{
			FamilyPackageNames.AddItem(DefaultArmMeshPackageName);
			FamilyPackageNames.AddItem(DefaultArmSkinPackageName);
		}
		else
		{
			UClass* FamilyInfoClass = FindFamilyInfo(InFamilyID);
			if(FamilyInfoClass)
			{
				UUTFamilyInfo* FamilyInfo = CastChecked<UUTFamilyInfo>(FamilyInfoClass->GetDefaultObject());
				FamilyPackageNames.AddItem(FamilyInfo->ArmMeshPackageName);
				FamilyPackageNames.AddItem(FamilyInfo->ArmSkinPackageName);
			}
		}
	}
	else
	{
		for(INT i=0; i<Parts.Num(); i++)
		{
			FCustomCharPart& Part = Parts(i);
			if(Part.FamilyID == InFamilyID)
			{
				INT PackageDotPos = Part.ObjectName.InStr(TEXT("."));
				FString PackageName = Part.ObjectName.Left(PackageDotPos);
				if(PackageName.Len() > 0)
				{
					FamilyPackageNames.AddUniqueItem(PackageName);
				}
			}
		}
	}

	// Check there are some packages to load.
	if(FamilyPackageNames.Num() == 0)
	{
		debugf(TEXT("LoadFamilyAssets : No packages found for Family: %s"), *InFamilyID);
		return NULL;
	}

	// Create asset store object
	UUTCharFamilyAssetStore* NewStore = ConstructObject<UUTCharFamilyAssetStore>(UUTCharFamilyAssetStore::StaticClass());
	NewStore->FamilyID = InFamilyID;
	NewStore->NumPendingPackages = FamilyPackageNames.Num();
	NewStore->StartLoadTime = GWorld->GetRealTimeSeconds();

	//
	debugf(TEXT("Begin Async loading packages for Family '%s':"), *InFamilyID);
	for(INT i=0; i<FamilyPackageNames.Num(); i++)
	{
		debugf(TEXT("- %s"), *(FamilyPackageNames(i)));
		UObject::LoadPackageAsync( FamilyPackageNames(i), AsyncLoadFamilyAssetsCompletionCallback, NewStore);
	}

	// If desired, block now until package is loaded.
	if(bBlocking)
	{
		UObject::FlushAsyncLoading();
	}

	return NewStore;
}

/** Util for finding and replacing a substring within another string. */
FString ReplaceSubstring(const FString& InString, const FString& Substring, const FString& ReplaceWith)
{
	INT SubstringPos = InString.InStr(Substring);
	if(SubstringPos == INDEX_NONE)
	{
		return TEXT("");
	}

	FString PreSubstring = InString.Left(SubstringPos);
	FString PostSubstring = InString.Mid(SubstringPos+4);
	FString WithSubstring = PreSubstring + ReplaceWith + PostSubstring;

	return WithSubstring;
}

/** Util that takes the currently assigned texture, and looks for a variant for a particular team and skin color. */
static UTexture2D* FindTeamSkinVariant(UTexture2D* InTex, const FString& Team, const FString& Skin)
{
	// Handle null Texture2D
	if(!InTex)
	{
		return NULL;
	}

	FString InTexName = InTex->GetPathName();
	
	// First, look for V01 and replace with desired team
	FString WithTeam = ReplaceSubstring(InTexName, TEXT("_V01"), FString(TEXT("_")) + Team);
	if(WithTeam.Len() > 0)
	{
		// Then try looking for SK1 and replacing with skin name
		FString WithTeamAndSkin = ReplaceSubstring(WithTeam, TEXT("_SK1"), FString(TEXT("_")) + Skin);
		if(WithTeamAndSkin.Len() > 0)
		{
			// Find that object
			//debugf(TEXT("Looking For: %s"), *WithTeamAndSkin);
			UTexture2D* Tex1 = FindObject<UTexture2D>(ANY_PACKAGE, *WithTeamAndSkin);
			// If successful, use this texture
			if(Tex1)
			{
				return Tex1;
			}
		}

		// Couldn't replace or find - try looking with just Team replaced
		//debugf(TEXT("Looking For: %s"), *WithTeam);
		UTexture2D* Tex2 = FindObject<UTexture2D>(ANY_PACKAGE, *WithTeam);
		if(Tex2)
		{
			return Tex2;
		}
	}

	// If V01 not found, try just replacing SK1 with skin name
	FString WithSkin = ReplaceSubstring(InTexName, TEXT("_SK1"), FString(TEXT("_")) + Skin);
	if(WithSkin.Len() > 0)
	{
		//debugf(TEXT("Looking For: %s"), *WithSkin);
		UTexture2D* Tex3 = FindObject<UTexture2D>(ANY_PACKAGE, *WithSkin);
		if(Tex3)
		{
			return Tex3;
		}
	}

	// Couldn't find any useful variation
	return InTex;
}

/** Util for checking texture is correct, and marking it to stream in if so. */
static UTexture2D* ValidatePartTexture(UTexture* InTex)
{
	UTexture2D* TempTex2D = Cast<UTexture2D>(InTex);
	if(!TempTex2D)
	{
		return NULL;
	}

	// Check texture is in correct texture group.
	if( TempTex2D->LODGroup != TEXTUREGROUP_Character && 
		TempTex2D->LODGroup != TEXTUREGROUP_CharacterNormalMap && 
		TempTex2D->LODGroup != TEXTUREGROUP_CharacterSpecular )
	{
		debugf(TEXT("Part Texture: '%s' in incorrect TEXTUREGROUP (%d)"), *TempTex2D->GetPathName(), TempTex2D->LODGroup);
		return NULL;
	}

	// Mark texture to stream in - need this before we can composite them.
	TempTex2D->bForceMiplevelsToBeResident = TRUE;

	return TempTex2D;
}

/** 
 *	Utility for finding all UTexture2Ds used by a particular mesh and setting them in the supplied 'regions'. 
 *	Returns parent MaterialInterface used for this part.
 */
static void AddPartTextures( USkeletalMesh* PartMesh, 
							  FSourceTexture2DRegion& Diffuse, 
							  FSourceTexture2DRegion& Normal, 
							  FSourceTexture2DRegion& Specular, 
							  FSourceTexture2DRegion& SpecPower, 
							  FSourceTexture2DRegion& EmMask,
							  const FString& TeamString,
							  const FString& SkinString,
							  INT SectionIndex )
{
	// Check we have a mesh
	if(!PartMesh)
	{
		return;
	}

	// Check it has a material applied
	if(PartMesh->Materials.Num() == 0)
	{
		debugf(TEXT("Part '%s' has no Material set."), *PartMesh->GetPathName());
		return;
	}

	// Check it has just one material (section)
	if(SectionIndex >= PartMesh->Materials.Num())
	{
		debugf(TEXT("Part '%s' does not contain section %d."), *PartMesh->GetPathName(), SectionIndex);
		SectionIndex = 0;
	}

	// Check it is a MaterialInstanceConstant
	UMaterialInstanceConstant* PartMIC = Cast<UMaterialInstanceConstant>(PartMesh->Materials(SectionIndex));
	if(!PartMIC)
	{
		debugf(TEXT("Part '%s' not using MIC."), *PartMesh->GetPathName());
		return;
	}

	// Read the parameters from the MIC for each type of texture.
	UBOOL bTexOK = TRUE;

	UTexture* TempTex = NULL;
	bTexOK |= PartMIC->GetTextureParameterValue(FName(TEXT("Char_Diffuse")), TempTex);
	Diffuse.Texture2D = ValidatePartTexture( FindTeamSkinVariant( Cast<UTexture2D>(TempTex), TeamString, SkinString ) );

	TempTex = NULL;
	bTexOK |= PartMIC->GetTextureParameterValue(FName(TEXT("Char_Normal")), TempTex);
	Normal.Texture2D = ValidatePartTexture(TempTex);

	TempTex = NULL;
	bTexOK |= PartMIC->GetTextureParameterValue(FName(TEXT("Char_Specular")), TempTex);
	Specular.Texture2D = ValidatePartTexture( FindTeamSkinVariant( Cast<UTexture2D>(TempTex), TeamString, SkinString ) );

	TempTex = NULL;
	bTexOK |= PartMIC->GetTextureParameterValue(FName(TEXT("Char_SpecPower")), TempTex);
	SpecPower.Texture2D = ValidatePartTexture(TempTex);

	TempTex = NULL;
	bTexOK |= PartMIC->GetTextureParameterValue(FName(TEXT("Char_Emissive")), TempTex);
	EmMask.Texture2D = ValidatePartTexture(TempTex);

	// Output warning if any parameters were missing.
	if(!bTexOK)
	{
		debugf(TEXT("Problems finding all required TextureParameters from '%s'"), *PartMIC->GetPathName());
	}

	return;
}

/** 
 *	Does 2 things:
 *	1) Remove any regions with NULL texture
 *	2) scale (shift) all source region sizes/offsets to match new texture size. 
 */
void FixRegions(TArray<FSourceTexture2DRegion>& Regions, const FString& Area, INT TexType)
{
	// Remove regions with no texture
	for(INT i=Regions.Num()-1; i>=0; i--)
	{
		 if(!Regions(i).Texture2D)
		 {
			Regions.Remove(i);
		 }
	}

	// Bail out if no regions left
	if(Regions.Num() == 0)
	{
		debugf(TEXT("No Textures Found For: %s Texture: %d"), *Area, TexType);
		return;
	}

	// Grab first texture to use as basis
	UTexture2D* Tex = Regions(0).Texture2D;

	// Texture co-ords are based around 2048 textures. 
	// Find how far we need to shift sizes to get from 2048 to this texture size.
	INT ShiftBy = 0;
	INT TestSize = 2048;
	while(TestSize > Tex->SizeX)
	{
		ShiftBy++;
		TestSize = TestSize >> 1;
	}

	for(INT i=0; i<Regions.Num(); i++)
	{
		FSourceTexture2DRegion& Region = Regions(i);
		Region.OffsetX = Region.OffsetX >> ShiftBy;
		Region.OffsetY = Region.OffsetY >> ShiftBy;
		Region.SizeX = Region.SizeX >> ShiftBy;
		Region.SizeY = Region.SizeY >> ShiftBy;
	}
}

/** Must match size of HeadTextures and BodyTextures array. */
static const INT NumCharTextures = 5;

// Textures array: Diffuse, Normal, Specular, SpecPower, EmMask
static const TCHAR* TextureNames[] = {TEXT("D01"), TEXT("N01"), TEXT("S01"), TEXT("SP01"), TEXT("E01")};

FCustomCharMergeState UUTCustomChar_Data::StartCustomCharMerge(FCustomCharData InCharData, const FString& InTeamString, const FString& InSkinString, USkeletalMesh* UseMesh, BYTE TextureRes)
{
	FCustomCharMergeState NewMergeState;
	appMemzero(&NewMergeState, sizeof(FCustomCharMergeState));

	// First save the profile/postfix into we are using for this new character.
	NewMergeState.CharData = InCharData;
	NewMergeState.TeamString = InTeamString;
	NewMergeState.SkinString = InSkinString;
	NewMergeState.UseMesh = UseMesh;

	// If a mesh is supplied, use its outer to create the textures we need as well.
	UObject* TexturePackage = INVALID_OBJECT;
	EObjectFlags TextureFlags = 0;
	if(NewMergeState.UseMesh)
	{		
		TexturePackage = CastChecked<UPackage>(NewMergeState.UseMesh->GetOuter());
		TextureFlags = RF_Public|RF_Transactional|RF_Standalone;
	}

	// Use special rules for Krall
	if(NewMergeState.CharData.FamilyID == FString(TEXT("KRAM")))
	{
		NewMergeState.bUseKrallRules = TRUE;
	}

	check(HeadRegions.Num() == 9);
	check(BodyRegions.Num() == 5);

	// First up we need to create the 'pending' UTexture2DComposite's, and being streaming all the input textures in.
	for(INT i=0; i<NumCharTextures; i++)
	{
		FName HeadTexName = NAME_None;
		FName BodyTexName = NAME_None;
		if(NewMergeState.UseMesh)
		{
			HeadTexName = FName( *FString::Printf(TEXT("%s_HEAD_%s"), *NewMergeState.UseMesh->GetName(), TextureNames[i]) );
			BodyTexName = FName( *FString::Printf(TEXT("%s_BODY_%s"), *NewMergeState.UseMesh->GetName(), TextureNames[i]) );
		}

		NewMergeState.HeadTextures[i] = ConstructObject<UTexture2DComposite>(UTexture2DComposite::StaticClass(), TexturePackage, HeadTexName, TextureFlags);
		NewMergeState.HeadTextures[i]->SourceRegions = HeadRegions;

		NewMergeState.BodyTextures[i] = ConstructObject<UTexture2DComposite>(UTexture2DComposite::StaticClass(), TexturePackage, BodyTexName, TextureFlags);
		NewMergeState.BodyTextures[i]->SourceRegions = BodyRegions;
	}

	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());
	USkeletalMesh* PartMesh = NULL;

	// Textures array: Diffuse, Normal, Specular, SpecPower, EmMask

	// HEAD: 0- head, 1- eyes, 2- teeth, 3- stump, 4- helmet or hair, 5- facemask, 6- eyewear, 7- kneepad/chest-skin, 8- arm-skin
	// BODY: 0- chest, 1- thighs, 2- shoulder pads, 3- arms, 4- boots 


	///// HEAD TEXTURES
	// Head
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Head, NewMergeState.CharData.HeadID, FALSE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(0), 
		NewMergeState.HeadTextures[1]->SourceRegions(0), 
		NewMergeState.HeadTextures[2]->SourceRegions(0),
		NewMergeState.HeadTextures[3]->SourceRegions(0), 
		NewMergeState.HeadTextures[4]->SourceRegions(0),
		InTeamString, InSkinString, 0);
	// Eyes
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(1), 
		NewMergeState.HeadTextures[1]->SourceRegions(1), 
		NewMergeState.HeadTextures[2]->SourceRegions(1), 
		NewMergeState.HeadTextures[3]->SourceRegions(1), 
		NewMergeState.HeadTextures[4]->SourceRegions(1),
		InTeamString, InSkinString, 0);
	// Teeth
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(2), 
		NewMergeState.HeadTextures[1]->SourceRegions(2), 
		NewMergeState.HeadTextures[2]->SourceRegions(2), 
		NewMergeState.HeadTextures[3]->SourceRegions(2), 
		NewMergeState.HeadTextures[4]->SourceRegions(2),
		InTeamString, InSkinString, 0);
	// Stump
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(3), 
		NewMergeState.HeadTextures[1]->SourceRegions(3), 
		NewMergeState.HeadTextures[2]->SourceRegions(3), 
		NewMergeState.HeadTextures[3]->SourceRegions(3), 
		NewMergeState.HeadTextures[4]->SourceRegions(3),
		InTeamString, InSkinString, 0);
	// Helmet
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Helmet, NewMergeState.CharData.HelmetID, FALSE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(4), 
		NewMergeState.HeadTextures[1]->SourceRegions(4), 
		NewMergeState.HeadTextures[2]->SourceRegions(4), 
		NewMergeState.HeadTextures[3]->SourceRegions(4), 
		NewMergeState.HeadTextures[4]->SourceRegions(4),
		InTeamString, InSkinString, 0);
	// Facemask
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Facemask, NewMergeState.CharData.FacemaskID, FALSE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(5), 
		NewMergeState.HeadTextures[1]->SourceRegions(5), 
		NewMergeState.HeadTextures[2]->SourceRegions(5), 
		NewMergeState.HeadTextures[3]->SourceRegions(5), 
		NewMergeState.HeadTextures[4]->SourceRegions(5),
		InTeamString, InSkinString, 0);
	// Goggles
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Goggles, NewMergeState.CharData.GogglesID, FALSE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.HeadTextures[0]->SourceRegions(6), 
		NewMergeState.HeadTextures[1]->SourceRegions(6), 
		NewMergeState.HeadTextures[2]->SourceRegions(6), 
		NewMergeState.HeadTextures[3]->SourceRegions(6), 
		NewMergeState.HeadTextures[4]->SourceRegions(6),
		InTeamString, InSkinString, 0);
	// Kneepad
	if(NewMergeState.bUseKrallRules)
	{
		PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Boots, NewMergeState.CharData.BootsID, FALSE, NewMergeState.CharData.BasedOnCharID);
		AddPartTextures(PartMesh, 
			NewMergeState.HeadTextures[0]->SourceRegions(7), 
			NewMergeState.HeadTextures[1]->SourceRegions(7), 
			NewMergeState.HeadTextures[2]->SourceRegions(7), 
			NewMergeState.HeadTextures[3]->SourceRegions(7), 
			NewMergeState.HeadTextures[4]->SourceRegions(7),
			InTeamString, InSkinString, 0);
	}

	///// BODY TEXTURES
	// Chest
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Torso, NewMergeState.CharData.TorsoID, FALSE, NewMergeState.CharData.BasedOnCharID);
	if(PartMesh->Materials.Num() == 1)
	{
		AddPartTextures(PartMesh, 
			NewMergeState.BodyTextures[0]->SourceRegions(0), 
			NewMergeState.BodyTextures[1]->SourceRegions(0), 
			NewMergeState.BodyTextures[2]->SourceRegions(0),
			NewMergeState.BodyTextures[3]->SourceRegions(0),
			NewMergeState.BodyTextures[4]->SourceRegions(0),
			InTeamString, InSkinString, 0);
	}
	else if(PartMesh->Materials.Num() == 2)
	{
		if(NewMergeState.bUseKrallRules)
		{
			debugf(TEXT("Error! Krall cannot have two section on torso mesh! (%s)"), *PartMesh->GetPathName());
		}

		AddPartTextures(PartMesh, 
			NewMergeState.HeadTextures[0]->SourceRegions(7), 
			NewMergeState.HeadTextures[1]->SourceRegions(7), 
			NewMergeState.HeadTextures[2]->SourceRegions(7),
			NewMergeState.HeadTextures[3]->SourceRegions(7),
			NewMergeState.HeadTextures[4]->SourceRegions(7),
			InTeamString, InSkinString, 0);

		AddPartTextures(PartMesh, 
			NewMergeState.BodyTextures[0]->SourceRegions(0), 
			NewMergeState.BodyTextures[1]->SourceRegions(0), 
			NewMergeState.BodyTextures[2]->SourceRegions(0),
			NewMergeState.BodyTextures[3]->SourceRegions(0),
			NewMergeState.BodyTextures[4]->SourceRegions(0),
			InTeamString, InSkinString, 1);
	}

	// Thighs
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Thighs, NewMergeState.CharData.ThighsID, FALSE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.BodyTextures[0]->SourceRegions(1), 
		NewMergeState.BodyTextures[1]->SourceRegions(1), 
		NewMergeState.BodyTextures[2]->SourceRegions(1),
		NewMergeState.BodyTextures[3]->SourceRegions(1),
		NewMergeState.BodyTextures[4]->SourceRegions(1),
		InTeamString, InSkinString, 0);
	// Boots
	// Special case for Krall - grab boot texture from thigh mesh as well (no boots)
	if(!NewMergeState.bUseKrallRules)
	{
		PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Boots, NewMergeState.CharData.BootsID, FALSE, NewMergeState.CharData.BasedOnCharID);
	}
	AddPartTextures(PartMesh, 
		NewMergeState.BodyTextures[0]->SourceRegions(4), 
		NewMergeState.BodyTextures[1]->SourceRegions(4), 
		NewMergeState.BodyTextures[2]->SourceRegions(4),
		NewMergeState.BodyTextures[3]->SourceRegions(4),
		NewMergeState.BodyTextures[4]->SourceRegions(4),
		InTeamString, InSkinString, 0);
	// Pads
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_ShoPad, NewMergeState.CharData.ShoPadID, TRUE, NewMergeState.CharData.BasedOnCharID);
	AddPartTextures(PartMesh, 
		NewMergeState.BodyTextures[0]->SourceRegions(2), 
		NewMergeState.BodyTextures[1]->SourceRegions(2), 
		NewMergeState.BodyTextures[2]->SourceRegions(2),
		NewMergeState.BodyTextures[3]->SourceRegions(2),
		NewMergeState.BodyTextures[4]->SourceRegions(2),
		InTeamString, InSkinString, 0);
	// Arms
	PartMesh = Data->FindPartSkelMesh(NewMergeState.CharData.FamilyID, PART_Arms, NewMergeState.CharData.ArmsID, FALSE, NewMergeState.CharData.BasedOnCharID);
	if( PartMesh != NULL )
	{
		if(PartMesh->Materials.Num() == 1)
		{
			AddPartTextures(PartMesh, 
				NewMergeState.BodyTextures[0]->SourceRegions(3), 
				NewMergeState.BodyTextures[1]->SourceRegions(3), 
				NewMergeState.BodyTextures[2]->SourceRegions(3),
				NewMergeState.BodyTextures[3]->SourceRegions(3),
				NewMergeState.BodyTextures[4]->SourceRegions(3),
				InTeamString, InSkinString, 0);
		}
		else if(PartMesh->Materials.Num() == 2)
		{
			AddPartTextures(PartMesh, 
				NewMergeState.HeadTextures[0]->SourceRegions(8), 
				NewMergeState.HeadTextures[1]->SourceRegions(8), 
				NewMergeState.HeadTextures[2]->SourceRegions(8),
				NewMergeState.HeadTextures[3]->SourceRegions(8),
				NewMergeState.HeadTextures[4]->SourceRegions(8),
				InTeamString, InSkinString, 0);

			AddPartTextures(PartMesh, 
				NewMergeState.BodyTextures[0]->SourceRegions(3), 
				NewMergeState.BodyTextures[1]->SourceRegions(3), 
				NewMergeState.BodyTextures[2]->SourceRegions(3),
				NewMergeState.BodyTextures[3]->SourceRegions(3),
				NewMergeState.BodyTextures[4]->SourceRegions(3),
				InTeamString, InSkinString, 1);
		}
	}
	else
	{
		warnf( TEXT( "Arms not found for FamilyID: %s" ), *NewMergeState.CharData.FamilyID );
	}

	for(INT i=0; i<NumCharTextures; i++)
	{
		// Remove NULL textures and adjust source region positions to take into account different size source textures.
		FixRegions(NewMergeState.HeadTextures[i]->SourceRegions, TEXT("HEAD"), i);
		FixRegions(NewMergeState.BodyTextures[i]->SourceRegions, TEXT("BODY"), i);

		if(TextureRes == CCTR_Normal)
		{
			// Set max texture size in composite textures to 'normal'
			NewMergeState.HeadTextures[i]->MaxTextureSize = HeadMaxTexSize[i];		
			NewMergeState.BodyTextures[i]->MaxTextureSize = BodyMaxTexSize[i];
		}
		else if(TextureRes == CCTR_Self)
		{
			// Set max texture size in composite textures to 'self' quality
			NewMergeState.HeadTextures[i]->MaxTextureSize = SelfHeadMaxTexSize[i];		
			NewMergeState.BodyTextures[i]->MaxTextureSize = SelfBodyMaxTexSize[i];
		}
		// If neither - then don't limit texture size.
	}

	// Set flag to indicate merge in progress
	NewMergeState.bMergeInProgress = TRUE;

	return NewMergeState;
}

static void AddMeshIfNotNull(TArray<USkeletalMesh*>& PartMeshes, USkeletalMesh* Mesh, TArray<FSkelMeshMergeSectionMapping>& Sections, INT SectionIndex)
{
	if(Mesh)
	{
		check(Mesh->LODModels.Num()>0);
		FStaticLODModel& SrcLODModel = Mesh->LODModels(0);

		// Check each LOD has the same number of sections.
		INT BaseNumSections = SrcLODModel.Sections.Num();
		for(INT i=1; i<Mesh->LODModels.Num(); i++)
		{
			if(Mesh->LODModels(i).Sections.Num() != BaseNumSections)
			{
				debugf(TEXT("ERROR: Mesh '%s' has LODs with different numbers of sections! Ignoring."), *Mesh->GetPathName());
				return;
			}
		}

		PartMeshes.AddItem(Mesh);

		// create a new entry for mapping the source mesh sections to the merged mesh sections
		FSkelMeshMergeSectionMapping* SectionMapping = new(Sections) FSkelMeshMergeSectionMapping;

		// Special case - when two sections 0 always is head and 1 is always body
		if(SrcLODModel.Sections.Num() == 2)
		{
			SectionMapping->SectionIDs.AddItem( Clamp<INT>(SrcLODModel.Sections(0).MaterialIndex, 0, 1) );
			SectionMapping->SectionIDs.AddItem( Clamp<INT>(SrcLODModel.Sections(1).MaterialIndex, 0, 1) );
		}
		// If not 2 sections, all go into same target section.
		else
		{
			// We only support 1 or 2 sections
			if(SrcLODModel.Sections.Num() != 1)
			{
				debugf(TEXT("Source mesh can only have 1 or 2 sections. (%s)"), *Mesh->GetPathName());
			}

			for( INT Idx=0; Idx < SrcLODModel.Sections.Num(); Idx++ )
			{
				SectionMapping->SectionIDs.AddItem(SectionIndex);
			}
		}
	}
}

static void UnMarkForceStreamFromRegions(UTexture2DComposite* CompTex)
{
	if(CompTex)
	{
		for(INT i=0; i<CompTex->SourceRegions.Num(); i++)
		{
			UTexture2D* Tex2D = CompTex->SourceRegions(i).Texture2D;
			if(Tex2D)
			{
				Tex2D->bForceMiplevelsToBeResident = FALSE;
			}
		}
	}
}

void UUTCustomChar_Data::ResetCustomCharMerge(FCustomCharMergeState& MergeState)
{
	// Allow textures we used for compositing to stream out again.
	for(INT i=0; i<NumCharTextures; i++)
	{
		UnMarkForceStreamFromRegions(MergeState.HeadTextures[i]);
		if(MergeState.HeadTextures[i])
		{
			MergeState.HeadTextures[i]->ClearFlags( RF_Standalone ); // Make sure texture will be GC'd
		}
		MergeState.HeadTextures[i] = NULL;

		UnMarkForceStreamFromRegions(MergeState.BodyTextures[i]);
		if(MergeState.BodyTextures[i])
		{
			MergeState.BodyTextures[i]->ClearFlags( RF_Standalone );
		}
		MergeState.BodyTextures[i] = NULL;
	}

	FCustomCharData EmptyData(EC_EventParm);
	MergeState.CharData = EmptyData;

	MergeState.bMergeInProgress = FALSE;
	MergeState.bUseKrallRules = FALSE;
}

USkeletalMesh* UUTCustomChar_Data::FinishCustomCharMerge(FCustomCharMergeState& MergeState)
{
	// Check we have actually started  merge
	if(!MergeState.bMergeInProgress)
	{
		debugf(TEXT("FinishCustomCharMerge called before StartCustomCharMerge"));
		return NULL;
	}

	// Iterate over all pending composite textures, and bail out if any are not streamed in
	for(INT i=0; i<NumCharTextures; i++)
	{
		if(	!MergeState.HeadTextures[i]->SourceTexturesFullyStreamedIn() ||
			!MergeState.BodyTextures[i]->SourceTexturesFullyStreamedIn() )
		{
			return NULL;
		}
	}

	DOUBLE StartComposite = appSeconds();
	for(INT i=0; i<NumCharTextures; i++)
	{
		// Do actual compositing work
		MergeState.HeadTextures[i]->UpdateCompositeTextue(0);
		MergeState.BodyTextures[i]->UpdateCompositeTextue(0);
	}

	DOUBLE EndComposite = appSeconds();
	debugf(TEXT("Composite took:%3.2fms"), (EndComposite - StartComposite) * 1000.f);

	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());

	// Array to store all the SkeletalMeshes we want to merge.
	TArray<USkeletalMesh*> PartMeshes;
	// Array to indicate which section each mesh should be merged in to
	TArray<FSkelMeshMergeSectionMapping> Sections;

	USkeletalMesh* PartMesh = NULL;

	// Find out if the helmet rules out goggles and/or facemask
	UBOOL bNoFacemask = FALSE;
	UBOOL bNoGoggles = FALSE;
	HelmetAllowsFaceParts(Data->Parts, MergeState.CharData.FamilyID, MergeState.CharData.HelmetID, bNoFacemask, bNoGoggles );

	// HEAD
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Head, MergeState.CharData.HeadID, FALSE, MergeState.CharData.BasedOnCharID);
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 0);

	// HELMET
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Helmet, MergeState.CharData.HelmetID, FALSE, MergeState.CharData.BasedOnCharID);
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 0);

	// FACEMASK
	if(!bNoFacemask)
	{
		PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Facemask, MergeState.CharData.FacemaskID, FALSE, MergeState.CharData.BasedOnCharID);
		AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 0);
	}

	// GOGGLES
	if(!bNoGoggles)
	{
		PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Goggles, MergeState.CharData.GogglesID, FALSE, MergeState.CharData.BasedOnCharID);
		AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 0);
	}

	// TORSO
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Torso, MergeState.CharData.TorsoID, FALSE, MergeState.CharData.BasedOnCharID);
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 1);

	// SHOULDER PADS
	if(MergeState.CharData.bHasLeftShoPad)
	{
		PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_ShoPad, MergeState.CharData.ShoPadID, TRUE, MergeState.CharData.BasedOnCharID);
		AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 1);
	}

	if(MergeState.CharData.bHasRightShoPad)
	{
		PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_ShoPad, MergeState.CharData.ShoPadID, FALSE, MergeState.CharData.BasedOnCharID);
		AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 1);
	}

	// ARMS
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Arms, MergeState.CharData.ArmsID, FALSE, MergeState.CharData.BasedOnCharID);
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 1);

	// THIGHS
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Thighs, MergeState.CharData.ThighsID, FALSE, MergeState.CharData.BasedOnCharID);
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, 1);

	// BOOTS
	PartMesh = Data->FindPartSkelMesh(MergeState.CharData.FamilyID, PART_Boots, MergeState.CharData.BootsID, FALSE, MergeState.CharData.BasedOnCharID);
	// This is kneepads for Krall, which are part of head section
	AddMeshIfNotNull(PartMeshes, PartMesh, Sections, MergeState.bUseKrallRules ? 0 : 1);


	// First, try using mesh that was passed in.
	USkeletalMesh* NewMesh = MergeState.UseMesh;
	if(!NewMesh)
	{
		// Create new SkeletalMesh in the transient package.
		NewMesh = ConstructObject<USkeletalMesh>(USkeletalMesh::StaticClass());
	}
	check(NewMesh);

	DOUBLE StartMerge = appSeconds();
	FSkeletalMeshMerge Merge(NewMesh, PartMeshes, Sections);
	UBOOL bSuccess = Merge.DoMerge();
	DOUBLE EndMerge = appSeconds();
	debugf(TEXT("Merge took:%3.2fms"), (EndMerge - StartMerge) * 1000.f);

	// Check new mesh looks good
	if(NewMesh->LODModels.Num() == 0 || NewMesh->LODModels(0).Sections.Num() == 0)
	{
		debugf(TEXT("New Mesh has zero LODModels or Sections. Failed."));
		bSuccess = FALSE;
	}

	if(bSuccess)
	{
		UClass* FamilyInfoClass = FindFamilyInfo(MergeState.CharData.FamilyID);
		if(FamilyInfoClass && FamilyInfoClass->IsChildOf(UUTFamilyInfo::StaticClass()))
		{
			UUTFamilyInfo* FamilyInfo = CastChecked<UUTFamilyInfo>(FamilyInfoClass->GetDefaultObject());
			if(FamilyInfo->BaseMICParent && FamilyInfo->BioDeathMICParent)
			{
				INT NumSections = NewMesh->LODModels(0).Sections.Num();
				if(NumSections != 2)
				{
					debugf(TEXT("FinishCustomCharMerge: WARNING: Resulting Mesh has %d sections - should have 2."), NumSections);
				}
				else
				{					
					// If a mesh is supplied, use its outer to create the textures we need as well.
					UObject* TexturePackage = INVALID_OBJECT;
					EObjectFlags TextureFlags = 0;
					FName HeadMICName = NAME_None;
					FName BodyMICName = NAME_None;
					FName HeadBioMICName = NAME_None;
					FName BodyBioMICName = NAME_None;

					if(MergeState.UseMesh)
					{		
						TexturePackage = CastChecked<UPackage>(MergeState.UseMesh->GetOuter());
						TextureFlags = RF_Public|RF_Transactional|RF_Standalone;

						HeadMICName = FName( *FString::Printf(TEXT("%s_HEAD_MIC"), *MergeState.UseMesh->GetName()) );
						BodyMICName = FName( *FString::Printf(TEXT("%s_BODY_MIC"), *MergeState.UseMesh->GetName()) );
						HeadBioMICName = FName( *FString::Printf(TEXT("%s_HEAD_BIO_MIC"), *MergeState.UseMesh->GetName()) );
						BodyBioMICName = FName( *FString::Printf(TEXT("%s_BODY_BIO_MIC"), *MergeState.UseMesh->GetName()) );
					}

					// Glow colour
					FLinearColor EColor = FamilyInfo->NonTeamEmissiveColor;
					FLinearColor TeamColor = FamilyInfo->NonTeamTintColor;
					FLOAT TintAmount = 1.f;
					if(MergeState.TeamString == FString("VRed"))
					{
						EColor		= FLinearColor(10.f,0.2f,0.2f,0.f);
						TeamColor	= FLinearColor(8.0f,0.1f,0.1f,0.f);
					}
					else if(MergeState.TeamString == FString("VBlue"))
					{
						EColor		= FLinearColor(1.f,1.f,10.f,0.f);
						TeamColor	= FLinearColor(0.5f,1.f,10.f,0.f);
					}

					// DEFAULT
					// Create new MIC for head, and set new textures.
					MergeState.DefaultHeadMIC = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), TexturePackage, HeadMICName, TextureFlags);
					MergeState.DefaultHeadMIC->SetParent(FamilyInfo->BaseMICParent);
					MergeState.DefaultHeadMIC->SetTextureParameterValue(FName(TEXT("Char_Diffuse")), MergeState.HeadTextures[0]);
					MergeState.DefaultHeadMIC->SetTextureParameterValue(FName(TEXT("Char_Normal")), MergeState.HeadTextures[1]);
					MergeState.DefaultHeadMIC->SetTextureParameterValue(FName(TEXT("Char_Specular")), MergeState.HeadTextures[2]);
					MergeState.DefaultHeadMIC->SetTextureParameterValue(FName(TEXT("Char_SpecPower")), MergeState.HeadTextures[3]);
					MergeState.DefaultHeadMIC->SetTextureParameterValue(FName(TEXT("Char_Emissive")), MergeState.HeadTextures[4]);
					MergeState.DefaultHeadMIC->SetVectorParameterValue(FName(TEXT("Char_Emissive_Color")), EColor);
					MergeState.DefaultHeadMIC->SetVectorParameterValue(FName(TEXT("Char_TeamColor")), TeamColor);
					MergeState.DefaultHeadMIC->SetScalarParameterValue(FName(TEXT("Char_DistSaturateSwitch")), TintAmount);

					// Create new MIC for body, and set new textures.
					MergeState.DefaultBodyMIC = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), TexturePackage, BodyMICName, TextureFlags);
					MergeState.DefaultBodyMIC->SetParent(FamilyInfo->BaseMICParent);
					MergeState.DefaultBodyMIC->SetTextureParameterValue(FName(TEXT("Char_Diffuse")), MergeState.BodyTextures[0]);
					MergeState.DefaultBodyMIC->SetTextureParameterValue(FName(TEXT("Char_Normal")), MergeState.BodyTextures[1]);
					MergeState.DefaultBodyMIC->SetTextureParameterValue(FName(TEXT("Char_Specular")), MergeState.BodyTextures[2]);
					MergeState.DefaultBodyMIC->SetTextureParameterValue(FName(TEXT("Char_SpecPower")), MergeState.BodyTextures[3]);
					MergeState.DefaultBodyMIC->SetTextureParameterValue(FName(TEXT("Char_Emissive")), MergeState.BodyTextures[4]);
					MergeState.DefaultBodyMIC->SetVectorParameterValue(FName(TEXT("Char_Emissive_Color")), EColor);
					MergeState.DefaultBodyMIC->SetVectorParameterValue(FName(TEXT("Char_TeamColor")), TeamColor);
					MergeState.DefaultBodyMIC->SetScalarParameterValue(FName(TEXT("Char_DistSaturateSwitch")), TintAmount);

					// Apply new MICs to new SkeletalMesh
					check(NewMesh->Materials.Num() == 2);
					NewMesh->Materials(0) = MergeState.DefaultHeadMIC;
					NewMesh->Materials(1) = MergeState.DefaultBodyMIC;

					// Set distance for LOD transitions. TODO: Should be in ini?
					check(NewMesh->LODInfo.Num() == NewMesh->LODModels.Num());
					if(NewMesh->LODModels.Num() >= 2)
					{
						NewMesh->LODInfo(1).DisplayFactor = Data->LOD1DisplayFactor;
					}

					if(NewMesh->LODModels.Num() >= 3)
					{
						NewMesh->LODInfo(2).DisplayFactor = Data->LOD2DisplayFactor;
					}
				}
			}
		}
	}
	else
	{
		debugf(TEXT("CreateCustomCharMesh: Problem combining parts."));
		NewMesh = NULL; // New mesh will be GC'd in time.
	}

	// Reset merge state when we're done - set bMergeInProgress to false.
	ResetCustomCharMerge(MergeState);

	return NewMesh;
}

static const INT PortraitTexSize = 256;

FSceneView* CalcPortraitSceneView(FSceneViewFamily* ViewFamily, FLOAT FOV)
{
	FMatrix ViewMatrix = FMatrix::Identity;	

	INT X = 0;
	INT Y = 0;
	UINT SizeX = PortraitTexSize;
	UINT SizeY = PortraitTexSize;

	ViewMatrix = ViewMatrix * FMatrix(
			FPlane(0,	0,	1,	0),
			FPlane(1,	0,	0,	0),
			FPlane(0,	1,	0,	0),
			FPlane(0,	0,	0,	1));


	FMatrix ProjectionMatrix = FPerspectiveMatrix(
		FOV * (FLOAT)PI / 360.0f,
		SizeX,
		SizeY,
		NEAR_CLIPPING_PLANE
		);

	FSceneView* View = new FSceneView(
		ViewFamily,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		X,
		Y,
		SizeX,
		SizeY,
		ViewMatrix,
		ProjectionMatrix,
		FLinearColor::Black,
		FLinearColor(0,0,0,0),
		FLinearColor::White,
		TArray<FPrimitiveSceneInfo*>()
		);
	ViewFamily->Views.AddItem(View);
	return View;
}

/** Utility for creating a random player character. */
UTexture* UUTCustomChar_Data::MakeCharPortraitTexture(USkeletalMesh* CharMesh, FCharPortraitSetup Setup)
{
	// Scene we will use for rendering this mesh to a texture.
	// The destructor of FPreviewScene removes all the components from it.
	FPreviewScene CharScene(Setup.DirLightRot, Setup.SkyBrightness, Setup.DirLightBrightness, FALSE);

	CharScene.RemoveComponent(CharScene.SkyLight);
	CharScene.SkyLight->LightColor = Setup.SkyColor;
	CharScene.SkyLight->LowerBrightness = Setup.SkyLowerBrightness;
	CharScene.SkyLight->LowerColor = Setup.SkyLowerColor;
	CharScene.AddComponent(CharScene.SkyLight, FMatrix::Identity);

	CharScene.RemoveComponent(CharScene.DirectionalLight);
	CharScene.DirectionalLight->LightColor = Setup.DirLightColor;
	CharScene.AddComponent(CharScene.DirectionalLight, FRotationMatrix(Setup.DirLightRot));

	CharScene.bStopAllAudioOnDestroy = FALSE;

	// Add the mesh
	USkeletalMeshComponent* PreviewSkelComp = ConstructObject<USkeletalMeshComponent>(USkeletalMeshComponent::StaticClass());
	PreviewSkelComp->SetSkeletalMesh(CharMesh);
	CharScene.AddComponent(PreviewSkelComp,FRotationMatrix(Setup.MeshRot));

	FVector UseOffset = Setup.MeshOffset;
	if(Setup.CenterOnBone != NAME_None)
	{
		INT BoneIndex = PreviewSkelComp->MatchRefBone(Setup.CenterOnBone);
		if(BoneIndex != INDEX_NONE)
		{
			FMatrix BoneTM = PreviewSkelComp->GetBoneMatrix(BoneIndex);
			UseOffset -= BoneTM.GetOrigin();
		}
	}

	FMatrix NewTM = FRotationMatrix(Setup.MeshRot);
	NewTM.SetOrigin(UseOffset);
	CharScene.RemoveComponent(PreviewSkelComp);
	CharScene.AddComponent(PreviewSkelComp, NewTM);

	// Create the output texture we'll render to
	UTextureRenderTarget2D* PortraitTex = ConstructObject<UTextureRenderTarget2D>(UTextureRenderTarget2D::StaticClass());
	check(PortraitTex);
	PortraitTex->bUpdateImmediate = TRUE;
	PortraitTex->Init(256,256,PF_A8R8G8B8);

	// TEMP: Block on scene being rendered.
	FlushRenderingCommands();

	// Create view family
	FSceneViewFamilyContext CharSceneViewFamily(
		PortraitTex->GetRenderTargetResource(),
		CharScene.GetScene(),
		(SHOW_DefaultGame & ~SHOW_PostProcess),
		appSeconds(),
		appSeconds(), 
		NULL);

	// Calculate scene view and add to ViewFamily
	CalcPortraitSceneView(&CharSceneViewFamily, Setup.CamFOV);

	// Make a Canvas
	FCanvas Canvas(PortraitTex->GetRenderTargetResource(), NULL);

	FViewport* UseViewport = GEngine->GetAViewport();
	if(!UseViewport)
	{
		appErrorf(TEXT("MakeCharPortraitTexture - Couldn't find FViewport!"));
	}

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		BeginDrawingCommand,
		FViewport*,Viewport,UseViewport,
	{
		RHIBeginDrawingViewport(Viewport->GetViewportRHI());
	}); 

	// Send a command to render this scene.
	BeginRenderingViewFamily(&Canvas, &CharSceneViewFamily);

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		EndDrawingCommand,
		FViewport*,Viewport,UseViewport,
		FRenderTarget*,RenderTarget,PortraitTex->GetRenderTargetResource(),
	{
		RHIEndDrawingViewport(Viewport->GetViewportRHI(), FALSE, FALSE);
		RHICopyToResolveTarget(RenderTarget->GetRenderTargetSurface(), TRUE);
	});

	// TEMP: Block on scene being rendered.
	FlushRenderingCommands();

	// Return the texture we created.
	return PortraitTex;
}


/** Util for find a random part ID within a given faction and type. */
static FString FindRandomPartID(const TArray<FCustomCharPart>& InParts, const FString& InFamilyID, BYTE InPart)
{
	TArray<FString> FilteredPartIDs;

	for(INT i=0; i<InParts.Num(); i++)
	{
		const FCustomCharPart& Part = InParts(i);
		if( Part.FamilyID == InFamilyID &&
			Part.Part == InPart)
		{
			FilteredPartIDs.AddItem(Part.PartID);
		}
	}

	// If no parts found- return empty string.
	FString OutString = TEXT("");
	if(FilteredPartIDs.Num() > 0)
	{
		INT RandomPartIdx = (appRand() % FilteredPartIDs.Num());
		OutString = FilteredPartIDs(RandomPartIdx);
	}

	//debugf(TEXT("%s %d %s"), *InFamilyID, InPart, *OutString);
	return OutString;
}

/** Utility to randomly generate a new character. */
FCustomCharData UUTCustomChar_Data::MakeRandomCharData()
{
	// First pick a random character to base data on.
	INT BaseCharIdx = (appRand() % Characters.Num());
	FCharacterInfo& BaseChar = Characters(BaseCharIdx);

	FCustomCharData NewData;
	appMemzero(&NewData, sizeof(FCustomCharData));

	NewData.BasedOnCharID = BaseChar.CharID;
	NewData.FamilyID = BaseChar.CharData.FamilyID;

	// Head always comes from base character
	NewData.HeadID = BaseChar.CharData.HeadID;

	// Pick random parts for the rest of the body
	NewData.HelmetID	= FindRandomPartID(Parts, NewData.FamilyID, PART_Helmet);
	NewData.FacemaskID	= FindRandomPartID(Parts, NewData.FamilyID, PART_Facemask);
	NewData.GogglesID	= FindRandomPartID(Parts, NewData.FamilyID, PART_Goggles);
	NewData.TorsoID		= FindRandomPartID(Parts, NewData.FamilyID, PART_Torso);
	NewData.ShoPadID	= FindRandomPartID(Parts, NewData.FamilyID, PART_ShoPad);
	NewData.ArmsID		= FindRandomPartID(Parts, NewData.FamilyID, PART_Arms);
	NewData.ThighsID	= FindRandomPartID(Parts, NewData.FamilyID, PART_Thighs);
	NewData.BootsID		= FindRandomPartID(Parts, NewData.FamilyID, PART_Boots);

	// Pick whether shoulder pads are on randomly.
	NewData.bHasLeftShoPad = (appFrand() < 0.5f);
	NewData.bHasRightShoPad = (appFrand() < 0.5f);

#if 0
	//////////////////////////////////////////////////////////////////////////
	FString Tmp = CharDataToString(NewData);
	debugf(TEXT("Tmp= %s"), *Tmp);
	FCustomCharData Tmp2 = CharDataFromString(Tmp);
	check(Tmp2.BasedOnCharID == NewData.BasedOnCharID);
	check(Tmp2.FamilyID == NewData.FamilyID);
	check(Tmp2.HeadID == NewData.HeadID);
	check(Tmp2.HelmetID == NewData.HelmetID);
	check(Tmp2.FacemaskID == NewData.FacemaskID);
	check(Tmp2.GogglesID == NewData.GogglesID);
	check(Tmp2.TorsoID == NewData.TorsoID);
	check(Tmp2.ShoPadID == NewData.ShoPadID);
	check(Tmp2.ArmsID == NewData.ArmsID);
	check(Tmp2.ThighsID == NewData.ThighsID);
	check(Tmp2.BootsID == NewData.BootsID);
	check(Tmp2.bHasLeftShoPad == NewData.bHasLeftShoPad);
	check(Tmp2.bHasRightShoPad == NewData.bHasRightShoPad);
	//////////////////////////////////////////////////////////////////////////
#endif

	return NewData;
}

static FString MakeNoneIfEmpty(const FString& InString)
{
	if(InString.Len() == 0)
	{
		return FString(TEXT("NONE"));
	}
	else
	{
		return InString;
	}
}

/** Utility for converting custom char data into one string */
FString UUTCustomChar_Data::CharDataToString(FCustomCharData InCharData)
{
	FString OutString = FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"),
		*MakeNoneIfEmpty(InCharData.BasedOnCharID),
		*MakeNoneIfEmpty(InCharData.FamilyID),
		*MakeNoneIfEmpty(InCharData.HeadID),
		*MakeNoneIfEmpty(InCharData.HelmetID),
		*MakeNoneIfEmpty(InCharData.FacemaskID),
		*MakeNoneIfEmpty(InCharData.GogglesID),
		*MakeNoneIfEmpty(InCharData.TorsoID),
		*MakeNoneIfEmpty(InCharData.ShoPadID),
		*MakeNoneIfEmpty(InCharData.ArmsID),
		*MakeNoneIfEmpty(InCharData.ThighsID),
		*MakeNoneIfEmpty(InCharData.BootsID),
		(InCharData.bHasLeftShoPad?TEXT("T"):TEXT("F")),
		(InCharData.bHasRightShoPad?TEXT("T"):TEXT("F")) );

	return OutString;
}

/** Utility for converting a string into a custom char data. */
FCustomCharData UUTCustomChar_Data::CharDataFromString(const FString& InString)
{
	FCustomCharData NewData;
	appMemzero(&NewData, sizeof(FCustomCharData));

	TArray<FString> PartStrings;
	InString.ParseIntoArrayWS(&PartStrings, TEXT(","));
	if(PartStrings.Num() == 13)
	{
		NewData.BasedOnCharID	= MakeNoneIfEmpty(PartStrings(0));
		NewData.FamilyID		= MakeNoneIfEmpty(PartStrings(1));
		NewData.HeadID			= MakeNoneIfEmpty(PartStrings(2));
		NewData.HelmetID		= MakeNoneIfEmpty(PartStrings(3));
		NewData.FacemaskID		= MakeNoneIfEmpty(PartStrings(4));
		NewData.GogglesID		= MakeNoneIfEmpty(PartStrings(5));
		NewData.TorsoID			= MakeNoneIfEmpty(PartStrings(6));
		NewData.ShoPadID		= MakeNoneIfEmpty(PartStrings(7));
		NewData.ArmsID			= MakeNoneIfEmpty(PartStrings(8));
		NewData.ThighsID		= MakeNoneIfEmpty(PartStrings(9));
		NewData.BootsID			= MakeNoneIfEmpty(PartStrings(10));
		NewData.bHasLeftShoPad	= (PartStrings(11).InStr(TEXT("T")) != -1);
		NewData.bHasRightShoPad = (PartStrings(12).InStr(TEXT("T")) != -1);
	}
	else
	{
		debugf(TEXT("CharDataFromString: Invalid string - %d elements found, expected 13"), PartStrings.Num());
	}

	return NewData;
}
