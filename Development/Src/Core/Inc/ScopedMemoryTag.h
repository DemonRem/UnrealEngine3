/*=============================================================================
	ScopedMemoryTag.h: Scoped memory allocation tagging definitions.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** Applies a tag to all memory allocated within the scope of the object. */
class FScopedMemoryTag
{
public:

	/** Initialization constructor. */
	FScopedMemoryTag(const TCHAR* TagDescription)
	{
#if !FINAL_RELEASE
		GMalloc->PushMemoryTag(TagDescription);
#endif
	}

	/** Destructor. */
	~FScopedMemoryTag()
	{
#if !FINAL_RELEASE
		GMalloc->PopMemoryTag();
#endif
	}
};

#if FINAL_RELEASE
#define SCOPED_MEMORY_TAG(TagDescription) {}
#else
#define SCOPED_MEMORY_TAG(TagDescription) FScopedMemoryTag MemoryTag(TagDescription);
#endif
