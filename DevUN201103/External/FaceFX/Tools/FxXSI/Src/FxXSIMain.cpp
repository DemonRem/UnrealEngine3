#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_argument.h>
#include <xsi_command.h>
#include "FxXSIData.h"
#include "FxXSIHelper.h"
#include "FxToolLog.h"
//#include 
using namespace XSI; 


XSIPLUGINCALLBACK CStatus XSILoadPlugin( PluginRegistrar& in_reg )
{
	in_reg.PutAuthor(L"mbarrett");
	in_reg.PutName(L"FxXSI");
	in_reg.PutEmail(L"");
	in_reg.PutURL(L"");
	in_reg.PutVersion(1,0);
	
	XSI::CString helpPath = in_reg.GetOriginPath();
	helpPath += L"FxXSI.htm";
	in_reg.PutHelp(helpPath);

	//File manipulation
	in_reg.RegisterCommand(L"FaceFXOpenFXA",L"FaceFXOpenFXA");
	in_reg.RegisterCommand(L"FaceFXSaveFXA",L"FaceFXSaveFXA");
	in_reg.RegisterCommand(L"FaceFXNewFXA",L"FaceFXNewFXA");

	//Reference pose
	in_reg.RegisterCommand(L"FaceFXImportRefBonePose",L"FaceFXImportRefBonePose");
	in_reg.RegisterCommand(L"FaceFXExportRefBonePose",L"FaceFXExportRefBonePose");
	in_reg.RegisterCommand(L"FaceFXListRefBonePoses",L"FaceFXListRefBonePoses");

	//Bone pose
	in_reg.RegisterCommand(L"FaceFXImportBonePose",L"FaceFXImportBonePose");
	in_reg.RegisterCommand(L"FaceFXExportBonePose",L"FaceFXExportBonePose");
	in_reg.RegisterCommand(L"FaceFXBatchImportBonePoses",L"FaceFXBatchImportBonePoses");
	in_reg.RegisterCommand(L"FaceFXBatchExportBonePoses",L"FaceFXBatchExportBonePoses");
	//in_reg.RegisterCommand(L"FaceFXListBonePoses",L"FaceFXListBonePoses");
	// D.P. The ListBonePose function was deprecated for consistency with Max/Maya tools.
	// Use FaceFXGetNodes "FxBonePoseNode" instead.
	in_reg.RegisterCommand(L"FaceFXGetNodes",L"FaceFXGetNodes");
	

	//Animation
	in_reg.RegisterCommand(L"FaceFXImportAnimation",L"FaceFXImportAnimation");
	in_reg.RegisterCommand(L"FaceFXListAnimationGroups",L"FaceFXListAnimationGroups");
	in_reg.RegisterCommand(L"FaceFXListAnimationsFromGroup",L"FaceFXListAnimationsFromGroup");

	in_reg.RegisterCommand(L"FaceFXGetBakedCurveKeyValues",L"FaceFXGetBakedCurveKeyValues");
	in_reg.RegisterCommand(L"FaceFXGetBakedCurveKeyTimes",L"FaceFXGetBakedCurveKeyTimes");
	in_reg.RegisterCommand(L"FaceFXGetBakedCurveKeySlopeIn",L"FaceFXGetBakedCurveKeySlopeIn");
	in_reg.RegisterCommand(L"FaceFXGetBakedCurveKeySlopeOut",L"FaceFXGetBakedCurveKeySlopeOut");

	in_reg.RegisterCommand(L"FaceFXGetRawCurveKeyValues",L"FaceFXGetRawCurveKeyValues");
	in_reg.RegisterCommand(L"FaceFXGetRawCurveKeyTimes",L"FaceFXGetRawCurveKeyTimes");
	in_reg.RegisterCommand(L"FaceFXGetRawCurveKeySlopeIn",L"FaceFXGetRawCurveKeySlopeIn");
	in_reg.RegisterCommand(L"FaceFXGetRawCurveKeySlopeOut",L"FaceFXGetRawCurveKeySlopeOut");

	in_reg.RegisterCommand(L"FaceFXDeleteAllKeys",L"FaceFXDeleteAllKeys");
	in_reg.RegisterCommand(L"FaceFXInsertKey",L"FaceFXInsertKey");
	in_reg.RegisterCommand(L"FaceFXGetAnimDuration",L"FaceFXGetAnimDuration");
	in_reg.RegisterCommand(L"FaceFXIsModDeveloper",L"FaceFXIsModDeveloper");
	in_reg.RegisterCommand(L"FaceFXIsNoSave",L"FaceFXIsNoSave");

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus XSIUnloadPlugin( const PluginRegistrar& in_reg )
{
	OC3Ent::Face::FxXSIData::xsiInterface.Shutdown();
	return CStatus::OK;
}

