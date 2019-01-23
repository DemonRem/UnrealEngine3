//------------------------------------------------------------------------------
// This class implements a FaceFx actor.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxActor.h"
#include "FxUtil.h"
#include "FxActorInstance.h"
#include "FxArchiveStoreNull.h"
#include "FxArchiveStoreFile.h"
#include "FxArchiveStoreFileFast.h"
#include "FxArchiveStoreMemory.h"
#include "FxArchiveStoreMemoryNoCopy.h"

namespace OC3Ent
{

namespace Face
{

FxList<FxActor*>* FxActor::_pActorList = NULL;
FxBool FxActor::_shouldFreeActorMemory = FxTrue;
FxName FxActor::NewActor;

#define kCurrentFxActorVersion 2

FX_IMPLEMENT_CLASS(FxActor, kCurrentFxActorVersion, FxNamedObject)

FxActor::FxActor()
	: _shouldClientRelink(FxFalse)
	, _isOpenInStudio(FxFalse)
{
	SetName(NewActor);
	_defaultAnimGroup.SetName(FxAnimGroup::Default);
}

FxActor::~FxActor()
{
}

void FX_CALL FxActor::Startup( void )
{
	if( !_pActorList )
	{
		_pActorList = static_cast<FxList<FxActor*>*>(FxAlloc(sizeof(FxList<FxActor*>), "ActorList"));
		FxDefaultConstruct(_pActorList);
	}
}

void FX_CALL FxActor::Shutdown( void )
{
	FlushActors();
	FxDelete(_pActorList);
	_pActorList = NULL;
}

void FxActor::Link( void )
{
	_linkBones();
	_linkAnims();
}

void FxActor::SetShouldClientRelink( FxBool shouldClientRelink )
{
	_shouldClientRelink = shouldClientRelink;
}

void FxActor::SetIsOpenInStudio( FxBool isOpenInStudio )
{
	_isOpenInStudio = isOpenInStudio;
}

FxBool FxActor::AddAnim( const FxName& group, const FxAnim& anim )
{
	// Add the group if needed - if not, nothing happens.
	AddAnimGroup(group);
	FxSize groupIndex = FindAnimGroup(group);
	if( groupIndex != FxInvalidIndex )
	{
		FxAnimGroup& animGroup = GetAnimGroup(groupIndex);
		animGroup.AddAnim(anim);
		animGroup.Link(_faceGraph);
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxActor::RemoveAnim( const FxName& group, const FxName& anim )
{
	FxSize groupIndex = FindAnimGroup( group );
	if( groupIndex != FxInvalidIndex )
	{
		FxAnimGroup& animGroup = GetAnimGroup(groupIndex);
		return animGroup.RemoveAnim(anim);
	}
	return FxFalse;
}

FxBool FxActor::AddAnimGroup( const FxName& name )
{
	if( FindAnimGroup(name) == FxInvalidIndex )
	{
		FxAnimGroup newGroup;
		newGroup.SetName(name);
		_animGroups.PushBack(newGroup);
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxActor::RemoveAnimGroup( const FxName& name )
{
	FxSize groupIndex = FindAnimGroup(name);
	if( groupIndex != FxInvalidIndex &&
		groupIndex > 0               &&
		groupIndex <= _animGroups.Length() )
	{
		_animGroups.Remove(groupIndex - 1);
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxActor::MountAnimSet( const FxAnimSet& animSet, 
							  FxMountUnmountExtendedInfo* pExtendedInfo )
{
	const FxAnimGroup& animGroup = animSet.GetAnimGroup();
	const FxName& animGroupName  = animGroup.GetName();
	if( FxInvalidIndex == FindAnimGroup(animGroupName) )
	{
		_mountedAnimGroups.PushBack(animGroup);
		_mountedAnimGroups.Back().Link(_faceGraph);
		if( pExtendedInfo )
		{
			*pExtendedInfo = MUE_None;
		}
		return FxTrue;
	}
	if( pExtendedInfo )
	{
		*pExtendedInfo = MUE_Duplicate;
	}
	return FxFalse;
}

FxBool FxActor::UnmountAnimSet( const FxName& animSetName, FxBool bStopPlayback, 
							    FxMountUnmountExtendedInfo* pExtendedInfo )
{
	FxList<FxAnimGroup>::Iterator magIter    = _mountedAnimGroups.Begin();
	FxList<FxAnimGroup>::Iterator magEndIter = _mountedAnimGroups.End();
	for( ; magIter != magEndIter; ++magIter )
	{
		if( magIter->GetName() == animSetName )
		{
			FxList<FxActorInstance*>::Iterator curr = _instanceList.Begin();
			FxList<FxActorInstance*>::Iterator end  = _instanceList.End();
			for( ; curr != end; ++curr )
			{
				FxAnimPlayer& animPlayer = (*curr)->GetAnimPlayer();
				if( animPlayer.IsPlaying() && animPlayer.GetCurrentAnimGroupName() == animSetName )
				{
					if( bStopPlayback )
					{
						animPlayer.Stop();
					}
					else
					{
						if( pExtendedInfo )
						{
							*pExtendedInfo = MUE_CurrentlyPlayingTryLater;
						}
						return FxFalse;
					}
				}
			}
			_mountedAnimGroups.Remove(magIter);
			if( pExtendedInfo )
			{
				*pExtendedInfo = MUE_None;
			}
			return FxTrue;
		}
	}
	if( pExtendedInfo )
	{
		*pExtendedInfo = MUE_InvalidMountedAnimSet;
	}
	return FxFalse;
}

FxBool FxActor::ImportAnimSet( const FxAnimSet& animSet )
{
	const FxAnimGroup& animGroup = animSet.GetAnimGroup();
	const FxName& animGroupName  = animGroup.GetName();
	if( FxInvalidIndex == FindAnimGroup(animGroupName) )
	{
		_animGroups.PushBack(animGroup);
		_animGroups.Back().Link(_faceGraph);
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxActor::ExportAnimSet( const FxName& animGroupName, FxAnimSet& exportedAnimSet )
{
	FxSize animGroupIndex = FindAnimGroup(animGroupName);
	if( FxInvalidIndex != animGroupIndex && 
		animGroupIndex > 0               &&
		animGroupIndex <= _animGroups.Length() )
	{
		FxAnimGroup& animGroup = GetAnimGroup(animGroupIndex);
		exportedAnimSet.SetOwningActorName(GetName());
		exportedAnimSet.SetName(animGroup.GetName());
		exportedAnimSet.SetAnimGroup(animGroup);
		return RemoveAnimGroup(animGroupName);
	}
	return FxFalse;
}

FxActor* FX_CALL FxActor::FindActor( const FxName& name )
{
	if( _pActorList )
	{
		FxList<FxActor*>::Iterator curr = _pActorList->Begin();
		FxList<FxActor*>::Iterator end  = _pActorList->End();
		for( ; curr != end; ++curr )
		{
			if( (*curr)->GetName() == name )
			{
				return (*curr);
			}
		}
	}
	return NULL;
}

void FX_CALL FxActor::SetShouldFreeActorMemory( FxBool shouldFreeActorMemory )
{
	_shouldFreeActorMemory = shouldFreeActorMemory;
}

FxBool FX_CALL FxActor::GetShouldFreeActorMemory( void )
{
	return _shouldFreeActorMemory;
}

FxBool FX_CALL FxActor::AddActor( FxActor* actor )
{
	if( !_pActorList )
	{
		FxAssert(!"Attempt to add actor before FxActor::Startup() called!");
	}
	else
	{
		if( actor )
		{
			if( FindActor(actor->GetName()) == NULL )
			{
				_pActorList->PushBack(actor);
				return FxTrue;
			}
		}
	}
	return FxFalse;
}

FxBool FX_CALL FxActor::RemoveActor( FxActor* actor )
{
	if( _pActorList && actor )
	{
		FxList<FxActor*>::Iterator pos = _pActorList->Find(actor);
		if( pos != _pActorList->End() )
		{
			if( _shouldFreeActorMemory )
			{
				FxActor* pActorToDelete = *pos;
				FxSize size = pActorToDelete->GetClassDesc()->GetSize();
				pActorToDelete->~FxActor();
				FxFree(pActorToDelete, size);
				pActorToDelete = NULL;
			}
			_pActorList->Remove(pos);
			return FxTrue;
		}
	}
	return FxFalse;
}

void FX_CALL FxActor::FlushActors( void )
{
	if( _pActorList )
	{
		if( _shouldFreeActorMemory )
		{
			FxList<FxActor*>::Iterator curr = _pActorList->Begin();
			FxList<FxActor*>::Iterator end  = _pActorList->End();
			for( ; curr != end; ++curr )
			{
				FxActor* pActorToDelete = *curr;
				FxSize size = pActorToDelete->GetClassDesc()->GetSize();
				pActorToDelete->~FxActor();
				FxFree(pActorToDelete, size);
				pActorToDelete = NULL;
			}
		}
		_pActorList->Clear();
	}
}

FxBool FxActor::AddRegister( const FxName& name )
{
	// First make sure this register isn't already available.
	FxSize numRegisterDefs = _registerDefs.Length();
	for( FxSize i = 0; i < numRegisterDefs; ++i )
	{
		if( _registerDefs[i]->GetName() == name )
		{
			return FxFalse;
		}
	}
	// Find the node in the face graph and add it as a register.
	FxFaceGraphNode* node = _faceGraph.FindNode(name);
	if( node )
	{
		_registerDefs.PushBack(node);
		// Update all the instances of this actor with the new register.
		FxList<FxActorInstance*>::Iterator curr = _instanceList.Begin();
		FxList<FxActorInstance*>::Iterator end  = _instanceList.End();
		for( ; curr != end; ++curr )
		{
			(*curr)->_addRegister(_registerDefs.Back());
		}
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxActor::RemoveRegister( const FxName& name )
{
	// First make sure this register is available.
	FxSize numRegisterDefs = _registerDefs.Length();
	for( FxSize i = 0; i < numRegisterDefs; ++i )
	{
		if( _registerDefs[i]->GetName() == name )
		{
			// Remove it from all instances.
			FxList<FxActorInstance*>::Iterator curr = _instanceList.Begin();
			FxList<FxActorInstance*>::Iterator end  = _instanceList.End();
			for( ; curr != end; ++curr )
			{
				(*curr)->_removeRegister(_registerDefs[i]);
			}
			// Remove it from this list.
			_registerDefs.Remove(i);
			return FxTrue;
		}
	}
	return FxFalse;
}

void FxActor::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxActor);
	arc << version;

	arc << _masterBoneList << _faceGraph;

	if( version >= 2 )
	{
		arc << _defaultAnimGroup << _animGroups;
	}
	else
	{
		arc << _animGroups;
		// Set up the new animation storage format.
		FxSize numAnimGroups = _animGroups.Length();
		for( FxSize i = 0; i < numAnimGroups; ++i )
		{
			if( _animGroups[i].GetName() == FxAnimGroup::Default )
			{
				_defaultAnimGroup = _animGroups[i];
				_animGroups.Remove(i);
				break;
			}
		}
	}
	
	FxSize registerDefsLength = _registerDefs.Length();
	arc << registerDefsLength;

	if( arc.IsSaving() )
	{
		for( FxSize i = 0; i < registerDefsLength; ++i )
		{
			arc << const_cast<FxName&>(_registerDefs[i]->GetName());
		}
	}
	else
	{
		_registerDefs.Clear();
		_registerDefs.Reserve(registerDefsLength);
		for( FxSize i = 0; i < registerDefsLength; ++i )
		{
			FxName registerDefName;
			arc << registerDefName;
			AddRegister(registerDefName);
		}
		// If there are mounted animation sets the only way to
		// unmount them is to call FxActor::UnmountAnimSet(), but if any
		// of them were linked to the Face Graph the links need to be 
		// cleared and recreated later.
		if( _mountedAnimGroups.Length() )
		{
			FxList<FxAnimGroup>::Iterator magIter    = _mountedAnimGroups.Begin();
			FxList<FxAnimGroup>::Iterator magEndIter = _mountedAnimGroups.End();
			for( ; magIter != magEndIter; ++magIter )
			{
				magIter->Unlink();
			}
		}
		// Link up the bones.
		_linkBones();
		// Animation linking is deferred until the animation is played for
		// the first time (as of FaceFX 1.6).
	}

	if( (arc.IsLoading() && version >= 1) || arc.IsSaving() )
	{
		arc << _phonemeMap;
	}
}

FX_INLINE void FxActor::_linkBones( void )
{
	_masterBoneList.Link(_faceGraph);
}

FX_INLINE void FxActor::_linkAnims( void )
{
	_defaultAnimGroup.Link(_faceGraph);
	FxSize numGroups = _animGroups.Length();
	for( FxSize i = 0; i < numGroups; ++i )
	{
		_animGroups[i].Link(_faceGraph);
	}
	FxList<FxAnimGroup>::Iterator magIter    = _mountedAnimGroups.Begin();
	FxList<FxAnimGroup>::Iterator magEndIter = _mountedAnimGroups.End();
	for( ; magIter != magEndIter; ++magIter )
	{
		magIter->Link(_faceGraph);
	}
}

FxBool FX_CALL FxLoadActorFromFile( FxActor& actor, const FxChar* filename, const FxBool bUseFastMethod,
						            void(FX_CALL *callbackFunction)(FxReal), FxReal updateFrequency )
{
	FxArchiveStore* pStore = bUseFastMethod ? FxArchiveStoreFileFast::Create(filename)
		                                    : FxArchiveStoreFile::Create(filename);
	FxArchive actorArchive(pStore, FxArchive::AM_LinearLoad);
	actorArchive.Open();
	if( actorArchive.IsValid() )
	{
		actorArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
		actorArchive << actor;
		return FxTrue;
	}
	return FxFalse;
}

FxBool FX_CALL FxSaveActorToFile( FxActor& actor, const FxChar* filename,
						          FxArchive::FxArchiveByteOrder byteOrder,
						          void(FX_CALL *callbackFunction)(FxReal), FxReal updateFrequency )
{
	FxArchive directoryCreater(FxArchiveStoreNull::Create(), FxArchive::AM_CreateDirectory);
	directoryCreater.Open();
	directoryCreater << actor;
	FxArchive actorArchive(FxArchiveStoreFile::Create(filename), FxArchive::AM_Save, byteOrder);
	actorArchive.Open();
	if( actorArchive.IsValid() )
	{
		actorArchive.SetInternalDataState(directoryCreater.GetInternalData());
		actorArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
		actorArchive << actor;
		return FxTrue;
	}
	return FxFalse;
}

FxBool FX_CALL FxLoadActorFromMemory( FxActor& actor, const FxByte* pMemory, const FxSize numBytes,
							          void(FX_CALL *callbackFunction)(FxReal), FxReal updateFrequency )
{
	if( pMemory )
	{
		FxArchive actorArchive(FxArchiveStoreMemoryNoCopy::Create(pMemory, numBytes), FxArchive::AM_LinearLoad);
		actorArchive.Open();
		if( actorArchive.IsValid() )
		{
			actorArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
			actorArchive << actor;
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FX_CALL FxSaveActorToMemory( FxActor& actor, FxByte*& pMemory, FxSize& numBytes,
						            FxArchive::FxArchiveByteOrder byteOrder,
						            void(FX_CALL *callbackFunction)(FxReal), FxReal updateFrequency )
{
	FxAssert(pMemory == NULL);
	if( !pMemory )
	{
		FxArchive directoryCreater(FxArchiveStoreNull::Create(), FxArchive::AM_CreateDirectory);
		directoryCreater.Open();
		directoryCreater << actor;
		FxArchiveStoreMemory* pStore = FxArchiveStoreMemory::Create(NULL, 0);
		FxArchive actorArchive(pStore, FxArchive::AM_Save, byteOrder);
		actorArchive.Open();
		if( actorArchive.IsValid() )
		{
			actorArchive.SetInternalDataState(directoryCreater.GetInternalData());
			actorArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
			actorArchive << actor;
			numBytes = pStore->GetSize();
			pMemory = static_cast<FxByte*>(FxAlloc(numBytes, "FxSaveActorToMemory"));
			FxMemcpy(pMemory, pStore->GetMemory(), numBytes);
			return FxTrue;
		}
	}
	return FxFalse;
}

} // namespace Face

} // namespace OC3Ent
