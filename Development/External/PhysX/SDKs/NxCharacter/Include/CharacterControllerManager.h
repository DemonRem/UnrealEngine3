#ifndef NX_CHARACTER_CONTROLLERMANAGER
#define NX_CHARACTER_CONTROLLERMANAGER

//Exclude file from docs
/** \cond */

#include "NxControllerManager.h"

//Implements the NxControllerManager interface, this class used to be called ControllerManager
class CharacterControllerManager: public NxControllerManager
{
public:
	CharacterControllerManager(NxUserAllocator* userAlloc);
	~CharacterControllerManager();

	NxU32				getNbControllers()	const;
	NxController*		getController(NxU32 index);
	Controller**		getControllers();
	NxController*		createController(NxScene* scene, const NxControllerDesc& desc);
	void				releaseController(NxController& controller);
	void				purgeControllers();
	void				updateControllers();
	void				release();

	void				printStats();
protected:
	ControllerArray*	controllers;
	NxUserAllocator*	allocator;
};

/** \endcond */

#endif //NX_CHARACTER_CONTROLLERMANAGER
