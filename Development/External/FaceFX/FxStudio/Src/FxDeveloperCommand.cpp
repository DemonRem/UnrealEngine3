//------------------------------------------------------------------------------
// The developer command.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxDeveloperCommand.h"
#include "FxVM.h"
#include "FxSessionProxy.h"
#include "FxFaceGraphBaker.h"
#include "FxAnimSetManager.h"
#include "FxAudioFile.h"
//@todo This is included to get access to FxMsg() used in the -dumpbonepose and
//      -dumprefpose cases.  We might want to remove this later on if we 
//      actually implement the plug-in architecture.
#include "FxConsole.h"
//@todo Ditto for this.
#include "FxTearoffWindow.h"

namespace OC3Ent
{

namespace Face
{

FX_IMPLEMENT_CLASS(FxDeveloperCommand, 0, FxCommand);

FxDeveloperCommand::FxDeveloperCommand()
{
}

FxDeveloperCommand::~FxDeveloperCommand()
{
}

FxCommandSyntax FxDeveloperCommand::CreateSyntax( void )
{
	FxCommandSyntax newSyntax;
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dwm", "-debugwidgetmesssages", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dnt", "-dumpnametable", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dct", "-dumpclasstable", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dcc", "-dumpconsolecommands", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dcv", "-dumpconsolevariables", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dms", "-dumpmemorystats", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dma", "-dumpmemoryallocs", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-crash", "-crash", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-scs", "-spamcommandsystem", CAT_Size));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dbp", "-dumpbonepose", CAT_String));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-drp", "-dumprefpose", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-tb", "-testbaking", CAT_StringArray));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-dmas", "-dumpmountedanimationsets", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-da", "-dumpanims", CAT_Flag));
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-saf", "-saveaudiofile", CAT_String));
	return newSyntax;
}

FxCommandError FxDeveloperCommand::Execute( const FxCommandArgumentList& argList )
{
	if( argList.GetArgument("-debugwidgetmesssages") )
	{
		FxSessionProxy::ToggleWidgetMessageDebugging();
	}
	if( argList.GetArgument("-dumpnametable") )
	{
		FxVM::DisplayInfo("<b>----start name table----</b>");
		FxSize memSavings = 0;
		FxChar temp[32] = {0};
		FxSize numNames = FxName::GetNumNames();
		for( FxSize i = 0; i < numNames; ++i )
		{
			FxString msg("(");
			FxItoa(i, temp);
			msg = FxString::Concat(msg, temp);
			msg = FxString::Concat(msg, ") <b>");
			msg = FxString::Concat(msg, *FxName::GetNameAsString(i));
			msg = FxString::Concat(msg, "</b> <i>[");
			FxItoa(FxName::GetNumReferences(i), temp);
			msg = FxString::Concat(msg, temp);
			msg = FxString::Concat(msg, " refs]</i>");
			FxVM::DisplayInfo(msg);
			memSavings += ((FxName::GetNameAsString(i)->Length()+1)*sizeof(FxChar))*(FxName::GetNumReferences(i)-1);
		}
		memSavings -= numNames*sizeof(FxRefString*)*2;
		FxString msg("Approx. name table memory savings: <b>");
		FxItoa(memSavings, temp);
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, " bytes.</b>");
		FxVM::DisplayInfo(msg);
#ifdef FX_TRACK_NAME_STATS
		FxSize numCollisions   = 0;
		FxSize numNamesAdded   = 0;
		FxSize numProbes       = 0;
		FxSize numHashBins     = 0;
		FxSize numUsedHashBins = 0;
		FxSize memUsed         = 0;
		FxName::GetNameStats(numCollisions, numNamesAdded, numProbes, numHashBins, numUsedHashBins, memUsed);
		FxItoa(numCollisions, temp);
		msg = "Name Hash: <b>";
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " peak collisions, ");
		FxItoa(numNames, temp);
		msg = FxString::Concat(msg, "<b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " unique names, ");
		FxItoa(numNamesAdded, temp);
		msg = FxString::Concat(msg, "<b>");
        msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " peak unique names, ");
		FxItoa(numProbes, temp);
		msg = FxString::Concat(msg, "<b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " peak probes, ");
		FxItoa(numUsedHashBins, temp);
		msg = FxString::Concat(msg, "<b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " / ");
		FxItoa(numHashBins, temp);
		msg = FxString::Concat(msg, "<b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		msg = FxString::Concat(msg, " hash bins, ");
		FxItoa(memUsed, temp);
		msg = FxString::Concat(msg, "mem used (in bytes): ");
		msg = FxString::Concat(msg, "<b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
		FxVM::DisplayInfo(msg);
#endif
		FxVM::DisplayInfo("<b>----end name table----</b>");
	}
	if( argList.GetArgument("-dumpclasstable") )
	{
		FxVM::DisplayInfo("<b>----start class info----</b>");
		FxChar temp[32] = {0};
		for( FxSize i = 0; i < FxClass::GetNumClasses(); ++i )
		{
			const FxClass* pClass = FxClass::GetClass(i);
			if( pClass )
			{
				FxString derivedFrom("");
				if( pClass->GetBaseClassDesc() )
				{
					derivedFrom = FxString::Concat(" : <i>", pClass->GetBaseClassDesc()->GetName().GetAsString());
					derivedFrom = FxString::Concat(derivedFrom, "</i>");
				}
				FxString msg("<b>");
				msg = FxString::Concat(msg, pClass->GetName().GetAsString());
				msg = FxString::Concat(msg, "</b>");
				msg = FxString::Concat(msg, derivedFrom);
				FxVM::DisplayInfo(msg);
				msg.Clear();
				msg = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<i>sizeof(";
				msg = FxString::Concat(msg, pClass->GetName().GetAsString());
				msg = FxString::Concat(msg, "): <b>");
				FxItoa(pClass->GetSize(), temp);
				msg = FxString::Concat(msg, temp);
				msg = FxString::Concat(msg, " bytes</b>  version: <b>");
				FxItoa(pClass->GetCurrentVersion(), temp);
				msg = FxString::Concat(msg, temp);
				msg = FxString::Concat(msg, "</b></i>");
				FxVM::DisplayInfo(msg);
				if( pClass->GetNumChildren() )
				{
					msg.Clear();
					msg = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<i>number of children: <b>";
					FxItoa(pClass->GetNumChildren(), temp);
					msg = FxString::Concat(msg, temp);
					msg = FxString::Concat(msg, "</b></i>");
					FxVM::DisplayInfo(msg);
				}
			}
		}
		FxItoa(FxClass::GetNumClasses(), temp);
		FxString msg("Number of classes: <b>");
		msg = FxString::Concat(msg, temp);
		msg = FxString::Concat(msg, "</b>");
        FxVM::DisplayInfo(msg);
		FxVM::DisplayInfo("<b>----end class info----</b>");
	}
	if( argList.GetArgument("-dumpconsolecommands") )
	{
		FxVM::DisplayInfo("<b>----start console command info----</b>");
		FxSize numCommands = FxVM::GetNumCommands();
		FxString msg("<b><i>");
		FxChar buffer[32];
		FxItoa(numCommands, buffer);
		msg += buffer;
		msg += "</i></b> console commands:";
		FxVM::DisplayInfo(msg);
		for( FxSize i = 0; i < numCommands; ++i )
		{
			FxVM::FxCommandDesc* pCommand = FxVM::GetCommandDesc(i);
			if( pCommand )
			{
				FxVM::DisplayInfo(pCommand->CommandName.GetData());
			}
		}
		FxVM::DisplayInfo("<b>----end console command info----</b>");
	}
	if( argList.GetArgument("-dumpconsolevariables") )
	{
		FxVM::DisplayInfo("<b>----start console variable info----</b>");
		FxSize numConsoleVariables = FxVM::GetNumConsoleVariables();
		FxString msg("<b><i>");
		FxChar buffer[32];
		FxItoa(numConsoleVariables, buffer);
		msg += buffer;
		msg += "</i></b> console variables:";
		FxVM::DisplayInfo(msg);
		for( FxSize i = 0; i < numConsoleVariables; ++i )
		{
			FxConsoleVariable* pConsoleVar = FxVM::GetConsoleVariable(i);
			if( pConsoleVar )
			{
				msg = "<b>";
				msg += pConsoleVar->GetNameAsString();
				msg += "</b>  ->  default value: <b><i>";
				msg += pConsoleVar->GetDefaultValue();
				msg += "</i></b>  value: <b><i>";
				msg += pConsoleVar->GetString();
				msg += "</i></b>  flags: <b><i>";
				FxItoa(pConsoleVar->GetFlags(), buffer);
				msg += buffer;
				msg += "</i></b>";
				FxVM::DisplayInfo(msg);
			}
		}
		FxVM::DisplayInfo("<b>----end console variable info----</b>");
	}
	if( argList.GetArgument("-crash") )
	{
		FxVM::DisplayInfo("Attempting to crash with a NULL pointer dereference...");
		FxInt32* pCrashPointer = NULL;
		*pCrashPointer = 0;
	}
	if( argList.GetArgument("-spamcommandsystem") )
	{
		FxSize numCommands;
		argList.GetArgument("-spamcommandsystem", numCommands);
		for( FxSize i = 0; i < numCommands; ++i )
		{
			FxVM::ExecuteCommand("eatMem -size 1024");
		}
	}
	FxString bonePoseName;
	if( argList.GetArgument("-dumpbonepose", bonePoseName) )
	{
		FxFaceGraphNode* pNode = NULL;
		if( FxSessionProxy::GetFaceGraphNode(bonePoseName, &pNode) )
		{
			FxBonePoseNode* pBonePoseNode = FxCast<FxBonePoseNode>(pNode);
			if( pBonePoseNode )
			{
				FxString header("Bone Pose <b>");
				header = FxString::Concat(header, pBonePoseNode->GetNameAsString());
				header = FxString::Concat(header, "</b> contains <b><i>");
				FxChar buffer[32];
				FxItoa(pBonePoseNode->GetNumBones(), buffer);
				header = FxString::Concat(header, buffer);
				header = FxString::Concat(header, "</i></b> bones");
				FxVM::DisplayInfo(header);
				for( FxSize i = 0; i < pBonePoseNode->GetNumBones(); ++i )
				{
					const FxBone& poseBone = pBonePoseNode->GetBone(i);
					FxString boneInfo("&nbsp;&nbsp;&nbsp;&nbsp;");
					boneInfo = FxString::Concat(boneInfo, poseBone.GetNameAsString());
					FxVM::DisplayInfo(boneInfo);
					FxVec3 bonePos = poseBone.GetPos();
					boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;bonePos = ";
					FxMsg("%s [%f, %f, %f]", boneInfo.GetData(), bonePos.x, bonePos.y, bonePos.z);
					FxQuat boneRot = poseBone.GetRot();
					boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;boneRot = ";
					FxMsg("%s [%f, %f, %f, %f]", boneInfo.GetData(), boneRot.w, boneRot.x, boneRot.y, boneRot.z);
					FxVec3 boneScale = poseBone.GetScale();
					boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;boneScale = ";
					FxMsg("%s [%f, %f, %f]", boneInfo.GetData(), boneScale.x, boneScale.y, boneScale.z);
				}
			}
			else
			{
				FxVM::DisplayWarning("node is not a bone pose node!");
			}
		}
		else
		{
			FxVM::DisplayError("node not found!");
		}
	}
	if( argList.GetArgument("-dumprefpose") )
	{
		FxActor* pActor = NULL;
		FxSessionProxy::GetActor(&pActor);
		if( pActor )
		{
			FxMasterBoneList& masterBoneList = pActor->GetMasterBoneList();
			FxSize numRefBones = masterBoneList.GetNumRefBones();
			FxString header("<b>Reference Pose</b> contains <b><i>");
			FxChar buffer[32];
			FxItoa(numRefBones, buffer);
			header = FxString::Concat(header, buffer);
			header = FxString::Concat(header, "</i></b> bones");
			FxVM::DisplayInfo(header);
			for( FxSize i = 0; i < numRefBones; ++i )
			{
				const FxBone& refBone = masterBoneList.GetRefBone(i);
				FxString boneInfo("&nbsp;&nbsp;&nbsp;&nbsp;");
				boneInfo = FxString::Concat(boneInfo, refBone.GetNameAsString());
				FxVM::DisplayInfo(boneInfo);
				FxVec3 refPos = refBone.GetPos();
				boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;refPos = ";
				FxMsg("%s [%f, %f, %f]", boneInfo.GetData(), refPos.x, refPos.y, refPos.z);
				FxQuat refRot = refBone.GetRot();
				boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;refRot = ";
				FxMsg("%s [%f, %f, %f, %f]", boneInfo.GetData(), refRot.w, refRot.x, refRot.y, refRot.z);
				FxVec3 refScale = refBone.GetScale();
				boneInfo = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;refScale = ";
				FxMsg("%s [%f, %f, %f]", boneInfo.GetData(), refScale.x, refScale.y, refScale.z);
			}
		}
	}
	if( argList.GetArgument("-dumpmemorystats") )
	{
		FxChar buffer[64];
		FxString stats = "Current allocations: ";
		FxItoa(FxGetCurrentBytesAllocated(), buffer);
		stats += buffer;
		stats += " bytes in ";
		FxItoa(FxGetCurrentNumAllocations(), buffer);
		stats += buffer;
		stats += " unique allocations.";
		FxVM::DisplayInfo(stats);

		stats = "Peak allocations: ";
		FxItoa(FxGetPeakBytesAllocated(), buffer);
		stats += buffer;
		stats += " bytes in ";
		FxItoa(FxGetPeakNumAllocations(), buffer);
		stats += buffer;
		stats += " unique allocations.";
		FxVM::DisplayInfo(stats);

		stats = "Min allocation size: ";
		FxItoa(FxGetMinAllocationSize(), buffer);
		stats += buffer;
		stats += " bytes.";
		FxVM::DisplayInfo(stats);

		stats = "Median allocation size: ";
		FxItoa(FxGetMedianAllocationSize(), buffer);
		stats += buffer;
		stats += " bytes.";
		FxVM::DisplayInfo(stats);

		stats = "Max allocation size: ";
		FxItoa(FxGetMaxAllocationSize(), buffer);
		stats += buffer;
		stats += " bytes.";
		FxVM::DisplayInfo(stats);

		stats = "Avg allocation size: ";
		FxItoa(FxGetAvgAllocationSize(), buffer);
		stats += buffer;
		stats += " bytes.";
		FxVM::DisplayInfo(stats);
	}
	if( argList.GetArgument("-dumpmemoryallocs") )
	{
		FxChar buffer[64] = {0};
		FxSize allocNum = 0;
		FxSize allocSize = 0;
		FxChar* allocSystem = 0;
		FxAllocIterator curr = FxGetAllocStart();
		while( curr )
		{
			if( FxGetAllocInfo(curr, allocNum, allocSize, allocSystem) )
			{
				FxString allocInfo = "Allocation #";
				FxItoa(allocNum, buffer);
				allocInfo += buffer;
				allocInfo += ": ";
				FxItoa(allocSize, buffer);
				allocInfo += buffer;
				allocInfo += " bytes from ";
				allocInfo += allocSystem;

				FxVM::DisplayInfo(allocInfo);
			}

			curr = FxGetAllocNext(curr);
		}
	}
	if( argList.GetArgument("-testbaking") )
	{
		FxArray<FxString> nodesToBakeStrings;
		argList.GetArgument("-testbaking", nodesToBakeStrings);
		FxString animGroup;
		FxString anim;
		if(	FxSessionProxy::GetSelectedAnimGroup(animGroup) )
		{
			if( FxSessionProxy::GetSelectedAnim(anim) )
			{
				FxArray<FxName> nodesToBake;
				for( FxSize i = 0; i < nodesToBakeStrings.Length(); ++i )
				{
					nodesToBake.PushBack(nodesToBakeStrings[i].GetData());
				}

				FxString info = "Attempting to bake ";
				FxChar buffer[64] = {0};
				FxItoa(nodesToBake.Length(), buffer);
				info += buffer;
				info += " face graph nodes...";

				FxVM::DisplayInfo(info);

				FxActor* actor = NULL;
				FxSessionProxy::GetActor(&actor);
				FxFaceGraphBaker baker(actor);
				baker.SetAnim(animGroup.GetData(), anim.GetData());
				if( baker.Bake(nodesToBake) )
				{
					info = "Baked ";
					FxItoa(baker.GetNumResultAnimCurves(), buffer);
					info += buffer;
					info += " animation curves:";

					FxVM::DisplayInfo(info);

					for( FxSize i = 0; i < baker.GetNumResultAnimCurves(); ++i )
					{
						const FxAnimCurve& animCurve = baker.GetResultAnimCurve(i);
						info = "Curve ";
						FxItoa(i, buffer);
						info += buffer;
						info += " [";
						info += animCurve.GetNameAsString();
						info += "]: ";
						FxItoa(animCurve.GetNumKeys(), buffer);
						info += buffer;
						info += " keys.";

						FxVM::DisplayInfo(info);
					}
				}
				else
				{
					FxVM::DisplayError("Bake failed!");
				}
			}
		}
	}
	if( argList.GetArgument("-dumpmountedanimationsets") )
	{
		FxSize numMountedAnimSets = FxAnimSetManager::GetNumMountedAnimSets();
		FxString info = "Dumping ";
		FxChar buffer[64] = {0};
		FxItoa(numMountedAnimSets, buffer);
		info += buffer;
		info += " mounted animation sets...";
		FxVM::DisplayInfo(info);
		for( FxSize i = 0; i < numMountedAnimSets; ++i )
		{
			info = "[";
			FxItoa(i, buffer);
			info += buffer;
			info += "] - ";
			info += FxAnimSetManager::GetMountedAnimSetName(i);
			FxVM::DisplayInfo(info);
		}
	}
	if( argList.GetArgument("-dumpanims") )
	{
		FxActor* actor = NULL;
		FxSessionProxy::GetActor(&actor);
		if( actor )
		{
			FxSize numGroups = actor->GetNumAnimGroups();
			for( FxSize i = 0; i < numGroups; ++i )
			{
				FxAnimGroup& animGroup = actor->GetAnimGroup(i);
                FxSize numAnims = animGroup.GetNumAnims();
				FxChar buffer[64] = {0};
				FxString info = "Dumping animation group ";
				info += animGroup.GetNameAsString();
				info += " (<b>";
				FxItoa(numAnims, buffer);
				info += buffer;
				info += "</b>) animations...";
				FxVM::DisplayInfo(info);
				for( FxSize j = 0; j < numAnims; ++j )
				{
					info = "[";
					FxItoa(j, buffer);
					info += buffer;
					info += "] - ";
					info += animGroup.GetAnim(j).GetNameAsString();
					FxVM::DisplayInfo(info);
				}
			}
		}
	}
	FxString audioFilename;
	if( argList.GetArgument("-saveaudiofile", audioFilename) )
	{
		FxString selectedAnimGroup;
		FxString selectedAnim;
		if( FxSessionProxy::GetSelectedAnimGroup(selectedAnimGroup) && 
			FxSessionProxy::GetSelectedAnim(selectedAnim) )
		{
			FxAnim* pAnim = NULL;
			FxSessionProxy::GetAnim(selectedAnimGroup, selectedAnim, &pAnim);
			if( pAnim )
			{
				FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());
				if( pAnimUserData )
				{
					FxDigitalAudio* pDigitalAudio = pAnimUserData->GetDigitalAudioPointer();
					if( pDigitalAudio )
					{
						FxAudioFile waveFile(pDigitalAudio);
						if( waveFile.Save(audioFilename) )
						{
							FxVM::DisplayInfo("saved audio file!");
						}
						else
						{
							FxVM::DisplayError("failed to save audio file!");
						}
					}
					else
					{
						FxVM::DisplayError("no audio to save!");
					}
				}
				else
				{
					FxVM::DisplayError("no anim user data!");
				}
			}
			else
			{
				FxVM::DisplayError("no anim!");
			}
		}
		else
		{
			FxVM::DisplayError("no valid session!");
		}
	}
	return CE_Success;
}

FX_IMPLEMENT_CLASS(FxEatMemCommand, 0, FxCommand);

FxEatMemCommand::FxEatMemCommand()
{
	_isUndoable    = FxTrue;
	_memChunkSize  = 1;
	_pMemChunk     = NULL;
}

FxEatMemCommand::~FxEatMemCommand()
{
}

FxCommandSyntax FxEatMemCommand::CreateSyntax( void )
{
	FxCommandSyntax newSyntax;
	newSyntax.AddArgDesc(FxCommandArgumentDesc("-s", "-size", CAT_Size));
	return newSyntax;
}

FxCommandError FxEatMemCommand::Execute( const FxCommandArgumentList& argList )
{
	if( argList.GetArgument("-size", _memChunkSize) )
	{
		return Redo();
	}
	FxVM::DisplayError("-size was not specified!");
	return CE_Failure;
}

FxCommandError FxEatMemCommand::Undo( void )
{
	if( _pMemChunk )
	{
		delete [] _pMemChunk;
		_pMemChunk = NULL;
	}
	return CE_Success;
}

FxCommandError FxEatMemCommand::Redo( void )
{
	_pMemChunk = new FxByte[_memChunkSize];
	if( _pMemChunk )
	{
		FxString infoString = "ate ";
		FxChar buffer[1024] = {0};
		FxItoa(_memChunkSize, buffer);
		infoString = FxString::Concat(infoString, buffer);
		infoString = FxString::Concat(infoString, " bytes of memory!");
		FxVM::DisplayWarning(infoString);
		return CE_Success;
	}
	return CE_Failure;
}

} // namespace Face

} // namespace OC3Ent
