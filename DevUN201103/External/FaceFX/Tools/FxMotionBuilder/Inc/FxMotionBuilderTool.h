//------------------------------------------------------------------------------
// The main module for the FaceFx Motion Builder plug-in.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef __FaceFXMotionBuilderTool_TOOL_H__
#define __FaceFXMotionBuilderTool_TOOL_H__

#define MAX_NAME_LENGTH 256

#include <fbsdk/fbsdk.h>
// MotionBuilder redefines new, making it impossible to use the FaceFX
// SDK unless we undef it.
#if defined(new)
	#undef new 
#endif // defined(new)

#define FaceFXMotionBuilderTool__CLASSNAME	FaceFXMotionBuilderTool
#define FaceFXMotionBuilderTool__CLASSSTR	"FaceFXMotionBuilderTool"

class FaceFXMotionBuilderTool : public FBTool
{
	//--- FiLMBOX Tool declaration.
	FBToolDeclare( FaceFXMotionBuilderTool, FBTool );

public:
	//--- FiLMBOX Construction/Destruction,
	virtual bool FBCreate();		//!< FiLMBOX Creation function.
	virtual void FBDestroy();		//!< FiLMBOX Destruction function.

	void	UICreate				();	
	void		UICreateLayout0		();
	void		UICreateLayout1		();
	void		UICreateLayout2		();
	void		UICreateLayout3		();

	void	UIConfigure				();	

	void		UIConfigureLayout0	();
	void		UIConfigureLayout1	();
	void		UIConfigureLayout2	();
	void		UIConfigureLayout3	();
	void		UIConfigureLayout4	();

	virtual bool FbxStore		( HFBFbxObject pFbxObject, kFbxObjectStore pStoreWhat );
	virtual bool FbxRetrieve	( HFBFbxObject pFbxObject, kFbxObjectStore pStoreWhat );

private:
	void		EventTabPanelChange			( HISender pSender, HKEvent pEvent );
	void		EventButtonCreateActor	( HISender pSender, HKEvent pEvent );
	void		EventButtonLoadActor	( HISender pSender, HKEvent pEvent );
	void		EventButtonSaveActor	( HISender pSender, HKEvent pEvent );
	void		EventButtonAbout	( HISender pSender, HKEvent pEvent );
	void		EventButtonImportRefPose	( HISender pSender, HKEvent pEvent );
	void		EventButtonExportRefPose	( HISender pSender, HKEvent pEvent );
	void		EventButtonImportBonePose	( HISender pSender, HKEvent pEvent );
	void		EventButtonExportBonePose	( HISender pSender, HKEvent pEvent );
	void		EventButtonBatchImportBonePose	( HISender pSender, HKEvent pEvent );
	void		EventButtonBatchExportBonePose	( HISender pSender, HKEvent pEvent );

	void		EventButtonImportAnimation	( HISender pSender, HKEvent pEvent );

	void		EventListChangeAnimGroup	( HISender pSender, HKEvent pEvent );
	void		EventToolIdle		( HISender pSender, HKEvent pEvent );
	void		EventToolShow		( HISender pSender, HKEvent pEvent );
	void		EventToolPaint		( HISender pSender, HKEvent pEvent );
	void		EventToolResize		( HISender pSender, HKEvent pEvent );
	void		EventToolInput		( HISender pSender, HKEvent pEvent );

	// Updates the GUI after the actor has changed.
	void UpdateGUI();
	double GetCurrentFrame();

private:

	FBPlayerControl mPlayerControl;
	FBSystem        mSystem;
	FBLayout	mLayout	[4 ];
	FBTabPanel	mTabPanel;

	FBString mLastBatch, mLastLoaded, mLastSaved;

	// Layout 0
	// The create actor button.
	FBButton mCreateActorButton;
	// The load actor button.
	FBButton mLoadActorButton;
	// The save actor button.
	FBButton mSaveActorButton;
	// The about button.
	FBButton mAboutButton;

	// Layout 1
	FBList				mRefBoneList;
	// The import reference pose button.
	FBButton mImportRefPoseButton;
	// The export reference pose button.
	FBButton mExportRefPoseButton;

	// Layout 2
	FBList				mBonePoseList;
	// The import bone pose button.
	FBButton mImportBonePoseButton;
	// The export bone pose button.
	FBButton mExportBonePoseButton;
	// The batch import bone poses button.
	FBButton mBatchImportBonePoseButton;
	// The batch export bone poses button.
	FBButton mBatchExportBonePoseButton;

	// Layout 3
	FBList				mAnimGroupList;
	FBList				mAnimList;
	// The import animation button.
	FBButton mImportAnimationButton;

};

#endif /* __FaceFXMotionBuilderTool_TOOL_H__ */
