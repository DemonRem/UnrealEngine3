#ifndef NXQUICKLOAD_H
#define NXQUICKLOAD_H

#include "NxConvexMesh.h"

/*
	NxQuickLoadAllocator

	You must use this allocator if you are going to use the NxQuickLoad library.
	It wraps a user-defined allocator.
 */

class NxQuickLoadAllocator : public NxUserAllocator
{
public:

					NxQuickLoadAllocator( NxUserAllocator & allocator, NxU32 alignment = 4 );
					~NxQuickLoadAllocator();

	bool			allocateTempSpace( NxU32 size );

	NxU32			readTempHighMark( bool clear );

	// NxUserAllocator interface
	virtual void*	mallocDEBUG( size_t size, const char* fileName, int line );
	virtual void*	mallocDEBUG( size_t size, const char* fileName, int line, const char* className, NxMemoryType type );
	virtual void*	malloc( size_t size );
	virtual void*	malloc( size_t size, NxMemoryType type );
	virtual void*	realloc( void* memory, size_t size );
	virtual void	free( void* memory );

private:

	NxUserAllocator*	m_allocator;
};

bool NxQuickLoadInit( NxPhysicsSDK * sdk );

bool NxCookQLConvexMesh(const NxConvexMeshDesc& desc_, NxStream& stream);

NxConvexMesh * NxCreateQLConvexMesh(NxStream& stream);

void NxReleaseQLConvexMesh(NxConvexMesh& mesh);

#define TEMP_QL_CODE	0

// Temporary functions
#if TEMP_QL_CODE

void Track( bool track );
void Mark( const char * notes );

#else

#define Track( track ) do {} while( false )
#define Mark( notes ) do {} while( false )

#endif	// #if TEMP_QL_CODE


#endif // #ifndef NXQUICKLOAD_H
