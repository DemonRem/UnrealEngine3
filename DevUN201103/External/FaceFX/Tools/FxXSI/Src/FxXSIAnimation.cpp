#include <xsi_ref.h>
#include <xsi_value.h>
#include <xsi_status.h>
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_decl.h>
#include <xsi_plugin.h>
#include <xsi_pluginitem.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include "FxXSIHelper.h"
#include "FxXSIData.h"
#include <xsi_value.h>
#include <xsi_string.h>
#include <xsi_projectitem.h>
using namespace XSI;
//

XSIPLUGINCALLBACK CStatus FaceFXImportAnimation_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());

	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"framerate",30.0f);

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXImportAnimation_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	float frameRate = (float)argValue[2];

	OC3Ent::Face::FxXSIData::xsiInterface.ImportAnim(animationGroupNameStr, animationNameStr,frameRate);

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXListAnimationGroups_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());

	Application app;	
	cmd.EnableReturnValue(true);
	cmd.SetFlag(siNoLogging,true);


	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXListAnimationGroups_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray animGroupNames;
	OC3Ent::Face::FxSize numAnimGroups = OC3Ent::Face::FxXSIData::xsiInterface.GetNumAnimGroups();
	for( OC3Ent::Face::FxSize i = 0; i < numAnimGroups; ++i )
	{

		CString strTemp;
		strTemp.PutAsciiString(OC3Ent::Face::FxXSIData::xsiInterface.GetAnimGroupName(i).GetData());
		animGroupNames.Add(CValue(strTemp));
	}
	CValue retValue(animGroupNames);
	ctxt.PutAttribute(L"ReturnValue",retValue);
	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXListAnimationsFromGroup_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());

	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	cmd.EnableReturnValue(true);
	cmd.SetFlag(siNoLogging,true);


	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXListAnimationsFromGroup_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());
	CValueArray animationNames;
	OC3Ent::Face::FxSize numAnimations = OC3Ent::Face::FxXSIData::xsiInterface.GetNumAnims(animationGroupNameStr);
	for( OC3Ent::Face::FxSize i = 0; i < numAnimations; ++i )
	{
		CString strTemp;
		strTemp.PutAsciiString(OC3Ent::Face::FxXSIData::xsiInterface.GetAnimName(animationGroupNameStr,i).GetData());
		animationNames.Add(CValue(strTemp));
	}
	CValue retValue(animationNames);
	ctxt.PutAttribute(L"ReturnValue",retValue);
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeyValues_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeyValues_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetBakedCurveKeyValues(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeyTimes_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeyTimes_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetBakedCurveKeyTimes(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeySlopeIn_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeySlopeIn_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetBakedCurveKeySlopeIn(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}



XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeySlopeOut_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetBakedCurveKeySlopeOut_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetBakedCurveKeySlopeOut(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeyValues_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeyValues_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetRawCurveKeyValues(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeyTimes_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeyTimes_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetRawCurveKeyTimes(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeySlopeIn_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeySlopeIn_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetRawCurveKeySlopeIn(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}



XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeySlopeOut_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetRawCurveKeySlopeOut_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxArray<OC3Ent::Face::FxReal> values = OC3Ent::Face::FxXSIData::xsiInterface.GetRawCurveKeySlopeOut(animationGroupNameStr, animationNameStr,curveNameStr);

	CValueArray returnValues;
	OC3Ent::Face::FxSize numValues = values.Length();
	for( OC3Ent::Face::FxSize i = 0; i < numValues; ++i )
	{
		returnValues.Add(CValue(values[i]));
	}
	CValue retValue(returnValues);
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FaceFXGetAnimDuration_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	return CStatus::OK;
}
XSIPLUGINCALLBACK CStatus FaceFXGetAnimDuration_Execute( CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	OC3Ent::Face::FxReal duration = OC3Ent::Face::FxXSIData::xsiInterface.GetAnimDuration(animationGroupNameStr, animationNameStr);

	CValue retValue(duration);
	ctxt.PutAttribute(L"ReturnValue",retValue);
	return CStatus::OK;
}
XSIPLUGINCALLBACK CStatus FaceFXInsertKey_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	args.Add( L"time");
	args.Add( L"vale");
	args.Add( L"inSlope");
	args.Add( L"inSlope");
	return CStatus::OK;
}
XSIPLUGINCALLBACK CStatus FaceFXInsertKey_Execute( const CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	CValue time = (CValue)argValue[3];
	CValue value = (CValue)argValue[4];
	CValue slopeIn = (CValue)argValue[5];
	CValue slopeOut = (CValue)argValue[6];
	
	OC3Ent::Face::FxSize index =  OC3Ent::Face::FxXSIData::xsiInterface.InsertKey(animationGroupNameStr, animationNameStr, curveNameStr, time, value, slopeIn, slopeOut);

	CValue retValue(static_cast<int>(index));
	ctxt.PutAttribute(L"ReturnValue",retValue);

	return CStatus::OK;
}
XSIPLUGINCALLBACK CStatus FaceFXDeleteAllKeys_Init( const CRef& in_context )
{
	Context ctx(in_context);
	Command cmd(ctx.GetSource());
	cmd.EnableReturnValue(true);
	Application app;
	ArgumentArray args = cmd.GetArguments();
	args.Add( L"animationgroupname");
	args.Add( L"animationname");
	args.Add( L"curvename");
	return CStatus::OK;
}
XSIPLUGINCALLBACK CStatus FaceFXDeleteAllKeys_Execute( const CRef& in_context )
{
	//Temp
	OC3Ent::Face::FxXSIData::xsiInterface.Startup();
	Application app;
	Context ctxt(in_context);	
	CValueArray argValue = (CValueArray)ctxt.GetAttribute(L"Arguments");
	CString animationGroupName = (CString)argValue[0];
	OC3Ent::Face::FxString animationGroupNameStr(animationGroupName.GetAsciiString());

	CString animationName = (CString)argValue[1];
	OC3Ent::Face::FxString animationNameStr(animationName.GetAsciiString());

	CString curveName = (CString)argValue[2];
	OC3Ent::Face::FxString curveNameStr(curveName.GetAsciiString());

	OC3Ent::Face::FxBool returnValue =  OC3Ent::Face::FxXSIData::xsiInterface.DeleteAllKeys(animationGroupNameStr, animationNameStr, curveNameStr);
	CValue retValue(returnValue);
	ctxt.PutAttribute(L"ReturnValue",retValue);
	return CStatus::OK;
}

