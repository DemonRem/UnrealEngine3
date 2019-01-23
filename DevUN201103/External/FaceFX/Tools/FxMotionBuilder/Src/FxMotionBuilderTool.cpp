//------------------------------------------------------------------------------
// The main module for the FaceFx Motion Builder plug-in.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxMotionBuilderTool.h"
#include "FxToolMB.h"
#include "FxMBData.h"

#include "FxToolLog.h"

#include "FxActorInstance.h"

//--- Registration defines
#define FaceFXMotionBuilderTool__CLASS	FaceFXMotionBuilderTool__CLASSNAME
#define FaceFXMotionBuilderTool__LABEL	"FaceFX"
#define FaceFXMotionBuilderTool__DESC	"A FaceFX Plugin for MotionBuidler"

//--- FiLMBOX implementation and registration
FBToolImplementation(	FaceFXMotionBuilderTool__CLASS	);
FBRegisterTool		(	FaceFXMotionBuilderTool__CLASS,
						FaceFXMotionBuilderTool__LABEL,
						FaceFXMotionBuilderTool__DESC,
						FB_DEFAULT_SDK_ICON		);	// Icon filename (default=Open Reality icon)


using namespace OC3Ent::Face;

/************************************************
 *	FiLMBOX Constructor.
 ************************************************/
bool FaceFXMotionBuilderTool::FBCreate()
{
	// Tool options
	StartSize[0] = 240;
	StartSize[1] = 400;

	// UI Management
	UICreate	();
	UIConfigure	();

	// Add tool callbacks
	OnShow.Add	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolShow		);
	OnIdle.Add	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolIdle		);
	OnResize.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolResize	);
	OnPaint.Add	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolPaint		);
	OnInput.Add	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolInput		);

	return true;
}

void FaceFXMotionBuilderTool::UICreate()
{
	int lS = 4;

	// Add regions
	AddRegion( "TabPanel",		"TabPanel",

									lS,		kFBAttachLeft,		"",				1.0,
									lS,		kFBAttachTop,		"",				1.0,
									-lS,	kFBAttachRight,		"",				1.0,
									25,		kFBAttachNone,		NULL,			1.0 );


	AddRegion( "Layout",		"Layout",
									0,		kFBAttachLeft,		"TabPanel",		1.0,
									0,		kFBAttachBottom,	"TabPanel",		1.0,
									0,		kFBAttachRight,		"TabPanel",		1.0,
									-lS,	kFBAttachBottom,	"",				1.0 );

	// Assign regions
	SetControl	("TabPanel",mTabPanel	);
	SetControl	("Layout",	mLayout[0]	);

	// Create sub-layouts
	UICreateLayout0();
	UICreateLayout1();
	UICreateLayout2();
	UICreateLayout3();
}

void FaceFXMotionBuilderTool::UICreateLayout0()
{
	int lS = 4;
	int lW = 210;
	int lH = 18;

	// Configure layout
	mLayout[0].AddRegion( "ButtonCreateActor", "ButtonCreateActor",
										lS,	kFBAttachLeft,	"",	1.0	,
										lS,	kFBAttachTop,	"",	1.0,
										lW,	kFBAttachNone,	"",	1.0,
										lH,	kFBAttachNone,	"",	1.0 );
	mLayout[0].AddRegion( "ButtonLoadActor", "ButtonLoadActor",
										0,		kFBAttachLeft,		"ButtonCreateActor",		1.0,
										lS,		kFBAttachBottom,	"ButtonCreateActor",		1.0,
										0,		kFBAttachWidth,		"ButtonCreateActor",		1.0,
										0,		kFBAttachHeight,	"ButtonCreateActor",		1.0 );
	mLayout[0].AddRegion( "ButtonSaveActor", "ButtonSaveActor",
										0,	kFBAttachLeft,		"ButtonLoadActor",	1.0	,
										lS,	kFBAttachBottom,	"ButtonLoadActor",	1.0,
										0,	kFBAttachWidth,		"ButtonLoadActor",	1.0,
										0,	kFBAttachHeight,	"ButtonLoadActor",	1.0 );
	mLayout[0].AddRegion( "ButtonAbout", "ButtonAbout",
										0,	kFBAttachLeft,		"ButtonSaveActor",	1.0	,
										lS,	kFBAttachBottom,	"ButtonSaveActor",	1.0,
										0,	kFBAttachWidth,		"ButtonSaveActor",	1.0,
										0,	kFBAttachHeight,	"ButtonSaveActor",	1.0 );

	mLayout[0].SetControl( "ButtonCreateActor", mCreateActorButton );
	mLayout[0].SetControl( "ButtonLoadActor", mLoadActorButton );
	mLayout[0].SetControl( "ButtonSaveActor", mSaveActorButton );
	mLayout[0].SetControl( "ButtonAbout", mAboutButton );
}

void FaceFXMotionBuilderTool::UICreateLayout1()
{
	int lS = 4;
	int bW = 100;
	int bH = 18;

	// Add regions
	mLayout[1].AddRegion("List",	"List",
										lS,		kFBAttachLeft,	"",				1.0,
										lS,		kFBAttachTop,	"",				1.0,
										-lS,		kFBAttachRight,	"",				1.0,
										-bH - 2 * lS,		kFBAttachBottom,	"",				1.0 );
	mLayout[1].AddRegion("Import",	"Import",
										0,		kFBAttachLeft,		"List",				1.0,
										lS,		kFBAttachBottom,	"List",				1.0,
										bW,		kFBAttachNone,		"",	1.0,
										bH,		kFBAttachNone,		"",	1.0 );
	mLayout[1].AddRegion("Export",	"Export",
										lS,		kFBAttachRight ,	"Import",				1.0,
										lS,		kFBAttachBottom,	"List",					1.0,
										bW,		kFBAttachNone,		"",	1.0,
										bH,		kFBAttachNone,		"",	1.0 );


	// Assign regions
	mLayout[1].SetControl("List", mRefBoneList );
	mLayout[1].SetControl("Import", mImportRefPoseButton );
	mLayout[1].SetControl("Export", mExportRefPoseButton );
}
void FaceFXMotionBuilderTool::UICreateLayout2()
{
	int lS = 4;
	int bW = 100;
	int bH = 18;

	// Add regions
	mLayout[2].AddRegion("List",	"List",
										lS,		kFBAttachLeft,	"",				1.0,
										lS,		kFBAttachTop,	"",				1.0,
										-lS,		kFBAttachRight,	"",				1.0,
										-2* bH - 2 * lS,		kFBAttachBottom,	"",				1.0 );
	mLayout[2].AddRegion("Import",	"Import",
										0,		kFBAttachLeft,		"List",		1.0,
										lS,		kFBAttachBottom,	"List",		1.0,
										bW,		kFBAttachNone,		"",			1.0,
										bH,		kFBAttachNone,		"",			1.0);
	mLayout[2].AddRegion("Export",	"Export",
										lS,		kFBAttachRight,		"Import",	1.0,
										lS,		kFBAttachBottom,	"List",		1.0,
										bW,		kFBAttachNone,		"",			1.0,
										bH,		kFBAttachNone,		"",			1.0 );
	mLayout[2].AddRegion("BatchImport",	"BatchImport",
										0,		kFBAttachLeft,		"Import",		1.0,
										lS,		kFBAttachBottom,	"Import",		1.0,
										bW,		kFBAttachNone,		"",			1.0,
										bH,		kFBAttachNone,		"",			1.0);
	mLayout[2].AddRegion("BatchExport",	"BatchExport",
										lS,		kFBAttachRight,		"BatchImport",	1.0,
										lS,		kFBAttachBottom,	"Import",		1.0,
										bW,	kFBAttachNone,		"",			1.0,
										bH,		kFBAttachNone,		"",			1.0 );
	// Assign regions
	mLayout[2].SetControl("List", mBonePoseList );
	mLayout[2].SetControl("Import", mImportBonePoseButton );
	mLayout[2].SetControl("Export", mExportBonePoseButton );
	mLayout[2].SetControl("BatchImport", mBatchImportBonePoseButton );
	mLayout[2].SetControl("BatchExport", mBatchExportBonePoseButton );
}
void FaceFXMotionBuilderTool::UICreateLayout3()
{
	int lS = 4;
	int tH = 20;

	int bW = 210;
	int bH = 18;

	// Add regions
	mLayout[3].AddRegion("Dropdown",	"Dropdown",
										lS,			kFBAttachLeft,	"",				1.0,
										lS,			kFBAttachTop,	"",				1.0,
										-lS,		kFBAttachRight,	"",				1.0,
										tH,			kFBAttachNone,	"",				1.0 );
	mLayout[3].AddRegion("List",	"List",
										0,			kFBAttachLeft,		"Dropdown",		1.0,
										lS,			kFBAttachBottom,	"Dropdown",		1.0,
										-lS,		kFBAttachRight,		"",			1.0,
										-2*lS -bH,	kFBAttachBottom,		"",			1.0);
	mLayout[3].AddRegion("ImportButton",	"ImportButton",
										0,			kFBAttachLeft,		"Dropdown",		1.0,
										lS,			kFBAttachBottom,	"List",		1.0,
										bW,			kFBAttachNone,		"",			1.0,
										bH,			kFBAttachNone,		"",			1.0);

	mLayout[3].SetControl("Dropdown", mAnimGroupList );
	mLayout[3].SetControl("List", mAnimList );
	mLayout[3].SetControl("ImportButton", mImportAnimationButton );
}
void FaceFXMotionBuilderTool::UIConfigure()
{
	// Configure main layout
	mTabPanel.Items.SetString	("Main~Ref Pose~Bone Pose~Anim");
	mTabPanel.OnChange.Add		( this, (FBCallback)&FaceFXMotionBuilderTool::EventTabPanelChange );

	SetBorder ("Layout", kFBStandardBorder, false,true, 1, 0,90,0);

	// Configure sub layouts
	UIConfigureLayout0();
	UIConfigureLayout1();
	UIConfigureLayout2();
	UIConfigureLayout3();
}
void FaceFXMotionBuilderTool::UIConfigureLayout0()
{
	mCreateActorButton.Caption = "Create Actor";
	mLoadActorButton.Caption = "Load Actor";
	mSaveActorButton.Caption = "Save Actor";
	mAboutButton.Caption = "About";

	// Configure button
	mCreateActorButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonCreateActor );
	mLoadActorButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonLoadActor );
	mSaveActorButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonSaveActor );
	mAboutButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonAbout );
	
}
void FaceFXMotionBuilderTool::UIConfigureLayout1()
{
	mImportRefPoseButton.Caption = "Import";
	mExportRefPoseButton.Caption = "Export";

	mRefBoneList.Style = kFBVerticalList;;

	// Configure button
	mImportRefPoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonImportRefPose );
	mExportRefPoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonExportRefPose );
}
void FaceFXMotionBuilderTool::UIConfigureLayout2()
{
	mImportBonePoseButton.Caption = "Import";
	mExportBonePoseButton.Caption = "Export";
	mBatchImportBonePoseButton.Caption = "Batch Import";
	mBatchExportBonePoseButton.Caption = "Batch Export";
	mBonePoseList.Style = kFBVerticalList;

	// Configure button
	mImportBonePoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonImportBonePose );
	mExportBonePoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonExportBonePose );
	mBatchImportBonePoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonBatchImportBonePose );
	mBatchExportBonePoseButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonBatchExportBonePose );
}
void FaceFXMotionBuilderTool::UIConfigureLayout3()
{
	mImportAnimationButton.Caption = "Import Animation";

	mAnimGroupList.Style =   kFBDropDownList;
	mAnimList.Style = kFBVerticalList;

	mImportAnimationButton.OnClick.Add( this, (FBCallback) &FaceFXMotionBuilderTool::EventButtonImportAnimation );
	mAnimGroupList.OnChange.Add(this, (FBCallback) &FaceFXMotionBuilderTool::EventListChangeAnimGroup );
}
/************************************************
 *	FiLMBOX Destruction function.
 ************************************************/
void FaceFXMotionBuilderTool::FBDestroy()
{
	// Remove tool callbacks
	OnShow.Remove	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolShow	);
	OnIdle.Remove	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolIdle	);
	OnPaint.Remove	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolPaint );
	OnInput.Remove	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolInput );
	OnResize.Remove	( this, (FBCallback) &FaceFXMotionBuilderTool::EventToolResize);

	// Free user allocated memory
}


/************************************************
 *	Button click callback.
 ************************************************/
void FaceFXMotionBuilderTool::EventButtonCreateActor	( HISender pSender, HKEvent pEvent )
{
	// Get the actor name from the user and create the actor.
	FxString actorName = "NewActor";

	char lValue[MAX_NAME_LENGTH];
	lValue[0] = '\0';
	int result = FBMessageBoxGetUserValue( "Actor Name", "Enter Actor Name", lValue, kFBPopupString, "OK", "Cancel");
	if(result == 1)
	{
		if( !FxMBData::mbInterface.CreateActor(actorName) )
		{
			FBMessageBox("FaceFX", "Failed to create actor!", "OK");
		}
		else
		{
			UpdateGUI();
		}
	}
}
void FaceFXMotionBuilderTool::EventButtonLoadActor	( HISender pSender, HKEvent pEvent )
{
	FBFilePopup		lFilePopup;

	lFilePopup.Caption	= "Select Actor File";
	lFilePopup.Filter	= "*.fxa";
	lFilePopup.Path		= mLastLoaded;

	lFilePopup.Style	= kFBFilePopupOpen;
	if(lFilePopup.Execute())
	{
		mLastLoaded = lFilePopup.Path.AsString();
		// Load the actor.
		if( !FxMBData::mbInterface.LoadActor( lFilePopup.FileName.AsString() ) )
		{
			FBMessageBox("FaceFX", "Failed to load actor!", "OK");							
		}
	}
	UpdateGUI();
}
void FaceFXMotionBuilderTool::EventButtonSaveActor	( HISender pSender, HKEvent pEvent )
{
	FBFilePopup		lFilePopup;

	lFilePopup.Caption	= "Save Actor File";
	lFilePopup.Filter	= "*.fxa";
	lFilePopup.Path		= mLastSaved;

	lFilePopup.Style	= kFBFilePopupSave;
   
	if(lFilePopup.Execute())
	{
		mLastSaved = lFilePopup.Path.AsString();
		// Save the actor.
		if( !FxMBData::mbInterface.SaveActor( lFilePopup.FileName.AsString() ) )
		{
			FBMessageBox("FaceFX", "Failed to save actor!", "OK");					
		}
	}
	UpdateGUI();
}
void FaceFXMotionBuilderTool::EventButtonAbout	( HISender pSender, HKEvent pEvent )
{
	FBMessageBox("About", "Copyright (c) 2002-2008 OC3 Entertainment, Inc.\n Contact support@oc3ent.com for help.", "OK");					
}
void FaceFXMotionBuilderTool::EventButtonImportRefPose	( HISender pSender, HKEvent pEvent )
{
	FBProgress	lProgress;
	lProgress.Caption	= "Importing Reference Pose.";
	lProgress.Text		= "Please wait...";
	FxMBData::mbInterface.ImportRefPose(GetCurrentFrame());
}
double FaceFXMotionBuilderTool::GetCurrentFrame()
{
	FBTime time = mSystem.LocalTime;
	// mSystem.FrameRate returns an incorrect value.  Must use Player control.
	FxReal fps = mPlayerControl.GetTransportFpsValue();
	return (time.GetSecondDouble()*fps)+.5f;
}

void FaceFXMotionBuilderTool::EventButtonExportRefPose	( HISender pSender, HKEvent pEvent )
{
	FBProgress	lProgress;
	lProgress.Caption	= "Calling FBGetSelectedModels. This can take a while.";
	lProgress.Text		= "Please wait...";
	// Export the ref pose from FxToolMB.  Pass in junk for frame and bone names.  We will
	// use info from selection to determine the bones.
	FxMBData::mbInterface.ExportRefPose(GetCurrentFrame(),FxArray<FxString>());
	UpdateGUI();
}
void FaceFXMotionBuilderTool::EventButtonImportBonePose	( HISender pSender, HKEvent pEvent )
{
	FxInt32 selectionIndex = mBonePoseList.ItemIndex;
	if(selectionIndex != -1)
	{
		FBProgress	lProgress;
		lProgress.Caption	= "Importing Bone Pose.";
		lProgress.Text		= "Please wait...";

		FxString poseName = mBonePoseList.Items.GetAt(selectionIndex);
		FxMBData::mbInterface.ImportBonePose(poseName, GetCurrentFrame());
		UpdateGUI();
	}
}
void FaceFXMotionBuilderTool::EventButtonExportBonePose	( HISender pSender, HKEvent pEvent )
{
	char lValue[MAX_NAME_LENGTH];
	lValue[0] = '\0';
	int result = FBMessageBoxGetUserValue( "Bone Pose Name", "Enter Bone Pose Name", lValue, kFBPopupString, "OK", "Cancel");
	if(result == 1)
	{
		FBProgress	lProgress;
		lProgress.Caption	= "Importing Bone Pose.";
		lProgress.Text		= "Please wait...";

		FxMBData::mbInterface.ExportBonePose(GetCurrentFrame(),lValue);
		mBonePoseList.ItemIndex	= mBonePoseList.Items.Find(lValue);
		UpdateGUI();
	}
}
void FaceFXMotionBuilderTool::EventButtonBatchImportBonePose	( HISender pSender, HKEvent pEvent )
{
	FBFilePopup		lFilePopup;
	lFilePopup.Filter	= "";
	lFilePopup.Caption	= "Open Batch File";
	lFilePopup.Path		= mLastBatch;
	lFilePopup.Style	= kFBFilePopupOpen;
	if(lFilePopup.Execute())
	{
		mLastBatch =lFilePopup.Path.AsString();
		FBProgress	lProgress;
		lProgress.Caption	= "Batch importing bone poses.";
		lProgress.Text		= "Please wait...";

		FxInt32 startFrame, endFrame;
		FxMBData::mbInterface.BatchImportBonePoses(lFilePopup.FileName.AsString(), startFrame, endFrame);
	}
}
void FaceFXMotionBuilderTool::EventButtonBatchExportBonePose	( HISender pSender, HKEvent pEvent )
{
	FBFilePopup		lFilePopup;
	lFilePopup.Filter	= "";
	lFilePopup.Caption	= "Open Batch File";
	lFilePopup.Style	= kFBFilePopupOpen;
	lFilePopup.Path		= mLastBatch;
	if(lFilePopup.Execute())
	{
		mLastBatch =lFilePopup.Path.AsString();
		FBProgress	lProgress;
		lProgress.Caption	= "Batch exporting bone poses.";
		lProgress.Text		= "Please wait...";

		FxMBData::mbInterface.BatchExportBonePoses(lFilePopup.FileName.AsString());
		UpdateGUI();
	}
}
void FaceFXMotionBuilderTool::EventListChangeAnimGroup( HISender pSender, HKEvent pEvent )
{
	int anim_group_index = mAnimGroupList.ItemIndex;
	FxString selectedAnimationGroup = "";
	mAnimList.Items.Clear();
	if(anim_group_index != -1)
	{
		selectedAnimationGroup = mAnimGroupList.Items.GetAt(anim_group_index);
	}
	for( FxSize i = 0; i < FxMBData::mbInterface.GetNumAnims(selectedAnimationGroup); ++i )
	{
		mAnimList.Items.Add(const_cast<FxChar*>(FxMBData::mbInterface.GetAnimName(selectedAnimationGroup, i).GetData()));
	}
}

void FaceFXMotionBuilderTool::EventButtonImportAnimation	( HISender pSender, HKEvent pEvent )
{
	int anim_group_index = mAnimGroupList.ItemIndex;
	int anim_index = mAnimList.ItemIndex;
	if(anim_group_index != -1 && anim_index != -1)
	{
		FxString selectedAnimGroup(mAnimGroupList.Items.GetAt(anim_group_index));
		FxString selectedAnim(mAnimList.Items.GetAt(anim_index));
		if( !OC3Ent::Face::FxMBData::mbInterface.ImportAnim(selectedAnimGroup, selectedAnim, 30))
		{
			FBMessageBox("Error", "Failed to import animation!", "OK");	
		}
	}
}
void FaceFXMotionBuilderTool::UpdateGUI ()
{
	int cashed_anim_group = mAnimGroupList.ItemIndex;
	int cashed_bone_pose = mBonePoseList.ItemIndex;

	mRefBoneList.Items.Clear();
	mAnimGroupList.Items.Clear();
	mAnimList.Items.Clear();
	mBonePoseList.Items.Clear();
	
	// MotionBuilder doesn't seem to be const-correct.  
	// Warning:  May be harming small babies...

	FxArray<FxString> bonePoses = FxMBData::mbInterface.GetNodesOfType("FxBonePoseNode");
	for( FxSize i = 0; i < bonePoses.Length(); ++i )
	{
		mBonePoseList.Items.Add(const_cast<FxChar*>(bonePoses[i].GetData()));
	}
	for( FxSize i = 0; i < FxMBData::mbInterface.GetNumRefBones(); ++i )
	{
		mRefBoneList.Items.Add(const_cast<FxChar*>(FxMBData::mbInterface.GetRefBoneName(i).GetData()));
	}
	for( FxSize i = 0; i < FxMBData::mbInterface.GetNumAnimGroups(); ++i )
	{
		mAnimGroupList.Items.Add(const_cast<FxChar*>(FxMBData::mbInterface.GetAnimGroupName(i).GetData()));
	}
	if( cashed_anim_group == -1 || cashed_anim_group > (int)FxMBData::mbInterface.GetNumAnimGroups())
	{
		mAnimGroupList.ItemIndex = 0;
	}
	else
	{
		mAnimGroupList.ItemIndex = cashed_anim_group;
	}
	if( cashed_bone_pose == -1 || cashed_bone_pose > (int)bonePoses.Length())
	{
		mBonePoseList.ItemIndex = -1;
	}
	else
	{
		mBonePoseList.ItemIndex = cashed_bone_pose;
	}
	EventListChangeAnimGroup(NULL, NULL);
}
void FaceFXMotionBuilderTool::EventTabPanelChange ( HISender pSender, HKEvent pEvent )
{
	switch( mTabPanel.ItemIndex )
	{
		case 0:	SetControl( "Layout", mLayout[0] );		break;
		case 1:	SetControl( "Layout", mLayout[1] );		break;
		case 2:	SetControl( "Layout", mLayout[2] );		break;
		case 3:	SetControl( "Layout", mLayout[3] );		break;
	}
}
/************************************************
 *	UI Idle callback.
 ************************************************/
void FaceFXMotionBuilderTool::EventToolIdle( HISender pSender, HKEvent pEvent )
{
}


/************************************************
 *	Handle tool activation (selection/unselection).
 ************************************************/
void FaceFXMotionBuilderTool::EventToolShow( HISender pSender, HKEvent pEvent )
{
	FBEventShow lEvent( pEvent );

	if( lEvent.Shown )
	{
		// Reset the UI here.
	}
	else
	{
	}
}


/************************************************
 *	Paint callback for tool (on expose).
 ************************************************/
void FaceFXMotionBuilderTool::EventToolPaint( HISender pSender, HKEvent pEvent )
{
}


/************************************************
 *	Tool resize callback.
 ************************************************/
void FaceFXMotionBuilderTool::EventToolResize( HISender pSender, HKEvent pEvent )
{
}


/************************************************
 *	Handle input into the tool.
 ************************************************/
void FaceFXMotionBuilderTool::EventToolInput( HISender pSender, HKEvent pEvent )
{
}


/************************************************
 *	FBX Storage.
 ************************************************/
bool FaceFXMotionBuilderTool::FbxStore	( HFBFbxObject pFbxObject, kFbxObjectStore pStoreWhat )
{
	pFbxObject->FieldWriteBegin( "FaceFXMotionBuilderToolSection" );
	{
//		pFbxObject->FieldWriteC( mButtonTest.Caption );
	}
	pFbxObject->FieldWriteEnd();

	return true;
}

/************************************************
 *	FBX Retrieval.
 ************************************************/
bool FaceFXMotionBuilderTool::FbxRetrieve( HFBFbxObject pFbxObject, kFbxObjectStore pStoreWhat )
{
	pFbxObject->FieldReadBegin( "FaceFXMotionBuilderToolSection" );
	{
//		mButtonTest.Caption = pFbxObject->FieldReadC();
	}
	pFbxObject->FieldReadEnd();

	return true;
}

