/*=============================================================================
UnWTInterface.cpp: Debugger Interface Interface
=============================================================================*/

#include "EnginePrivate.h"

#include "UnDebuggerCore.h"
#include "UnDebuggerInterface.h"
#include "UnWTInterface.h"

#pragma pack(push,8)
#include <ShellApi.h>
#pragma pack(pop)

#define UCSIDE
#include "WTDebuggerInterface.h"

WTGlobals g_WTGlobals(L"WTUnrealDebuggerUCSide");

DWORD OnCommandToUC(INT cmdId, LPCWSTR cmdStr)
{
	if(GDebugger)
		((WTInterface*)((UDebuggerCore*)GDebugger)->GetInterface())->Callback(cmdId, cmdStr);
	return 0;
}

WTInterface::WTInterface( const TCHAR* InDLLName ) 
:	hInterface(NULL),
	m_pIPC(NULL)
{
	DllName = InDLLName;
	Debugger = NULL;
	m_locked = FALSE;
}

WTInterface::~WTInterface()
{
	Close();
	UnbindDll();
	Debugger = 0;
}

int WTInterface::AddAWatch(int watch, int ParentIndex, const TCHAR* ObjectName, const TCHAR* Data)
{
	return m_pIPC->SendCommandToVS(CMD_AddWatch, watch, ParentIndex, ObjectName, Data);
// 	return VAAddAWatch(watch, ParentIndex, ObjectName, Data);
}


void WTInterface::ClearAWatch(int watch)
{
	m_pIPC->SendCommandToVS(CMD_ClearWatch, watch);
// 	VAClearAWatch(watch);
}

UBOOL WTInterface::Initialize( UDebuggerCore* DebuggerOwner )
{
	Debugger = DebuggerOwner;
	if ( !m_pIPC)
	{
		BindToDll();
		ClearAWatch( LOCAL_WATCH );
		ClearAWatch( GLOBAL_WATCH );
		ClearAWatch( WATCH_WATCH );
	}
	Show();
	return TRUE;
}

void WTInterface::Callback( INT cmdID,  LPCWSTR cmdStr )
{
	Sleep(50);

	// uncomment to log all callback mesages from the UI
	const TCHAR* command = cmdStr; // ANSI_TO_TCHAR(C);
	warnf(TEXT("Callback: %s"), command);
	warnf(TEXT("> > %s"), *Debugger->Describe());

	if(ParseCommand(&command, TEXT("addbreakpoint")))
	{		
		TCHAR className[256];
		ParseToken(command, className, 256, 0);
		TCHAR lineNumString[10];
		ParseToken(command, lineNumString, 10, 0);
		SetBreakpoint(className, appAtoi(lineNumString));

	}
	else if(ParseCommand(&command, TEXT("removebreakpoint")))
	{
		TCHAR className[256];
		ParseToken(command, className, 256, 0);
		TCHAR lineNumString[10];
		ParseToken(command, lineNumString, 10, 0);
		RemoveBreakpoint(className, appAtoi(lineNumString));
	}
	else if(ParseCommand(&command, TEXT("addwatch")))
	{
		TCHAR watchName[256];
		ParseToken(command, watchName, 256, 0);
		Debugger->AddWatch(watchName);
	}
	else if(ParseCommand(&command, TEXT("removewatch")))
	{
		TCHAR watchName[256];
		ParseToken(command, watchName, 256, 0);
		Debugger->RemoveWatch(watchName);
	}
	else if(ParseCommand(&command, TEXT("clearwatch")))
	{
		Debugger->ClearWatches();
	}
	else if ( ParseCommand(&command,TEXT("setcondition")) )
	{
		FString ConditionName, Value;
		if ( !ParseToken(command,ConditionName,1) )
		{
			debugf(TEXT("Callback error (setcondition): Couldn't parse condition name"));
			return;
		}
		if ( !ParseToken(command,Value,1) )
		{
			debugf(TEXT("Callback error (setcondition): Failed parsing condition value"));
			return;
		}

		Debugger->SetCondition(*ConditionName,*Value);
		return;
	}
	else if ( ParseCommand(&command,TEXT("getstack")) )
	{
		UpdateCallStack();
		// I am still able to get IsDebugging == 0 when it should not be,
		// I think this is caused by this asynchronous callback calling "step" before
		// the engine has finished updating the stack.
		// Is there some sort of locking we should checking for?
		// This is a hack to just reset it, when the user presses break.
		if( !Debugger->IsDebuggerActive() )
		{
			Debugger->ActivateDebugger(TRUE);
		}
		return;
	}
	else if ( ParseCommand(&command,TEXT("changestack")) )
	{
		FString StackNum;
		if ( !ParseToken(command,StackNum,1) )
		{
			debugf(TEXT("Callback error (changestack): Couldn't parse stacknum"));
			return;
		}

		Debugger->ChangeStack( appAtoi(*StackNum) );
		return;
	}
	else if (ParseCommand(&command,TEXT("setdatawatch")))
	{
		FString WatchText;
		if ( !ParseToken(command,WatchText,1) )
		{
			debugf(TEXT("Callback error (setdatawatch): Failed parsing watch text"));
			return;
		}

		Debugger->SetDataBreakpoint(*WatchText);
		return;
	}
	else if(ParseCommand(&command, TEXT("breakonnone")))
	{
		TCHAR breakValueString[5];
		ParseToken(command, breakValueString, 5, 0);
		Debugger->SetBreakOnNone(appAtoi(breakValueString));
	}
	else if(ParseCommand(&command, TEXT("break")))
	{
		if( !Debugger->IsDebuggerActive() )
		{
			Debugger->ActivateDebugger(TRUE);
		}
		Debugger->SetBreakASAP(TRUE);
	}

	else if(ParseCommand(&command, TEXT("stopdebugging")))
	{
		Debugger->Close();
		return;
	}
	else if ( ParseCommand(&command,TEXT("clearbreaks")) )
	{
		// dunno what this is supposed to be for...
		return;
	}
	else if ( Debugger->IsDebuggerActive() )
	{
		EUserAction Action = UA_None;
		if(ParseCommand(&command, TEXT("go")))
		{
			Debugger->SetBreakASAP(FALSE);
			Action = UA_Go;
		}
		else if ( ParseCommand(&command,TEXT("stepinto")) )
		{
			Action = UA_StepInto;
		}
		else if ( ParseCommand(&command,TEXT("stepover")) )
		{
			Action = UA_StepOverStack;
		}
		else if(ParseCommand(&command, TEXT("stepoutof")))
		{
			Action = UA_StepOut;
		}
		Debugger->ProcessInput(Action);
	}

	warnf(TEXT("<<< %s"), *Debugger->Describe());
}

void WTInterface::AddToLog( const TCHAR* Line )
{
	m_pIPC->SendCommandToVS(CMD_AddLineToLog, Line);
}

void WTInterface::Show()
{
	if ( Debugger->IsDebuggerActive() /*&& !Debugger->BreakASAP*/ )
		m_pIPC->SendCommandToVS(CMD_ShowDllForm);
}

void WTInterface::Close()
{
	m_pIPC->SendCommandToVS(CMD_GameEnded);
	m_pIPC->Close();
	UnbindDll();

}

void WTInterface::Hide()
{

}

void WTInterface::Update( const TCHAR* ClassName, const TCHAR* PackageName, INT LineNumber, const TCHAR* OpcodeName, const TCHAR* objectName )
{
	FString openName(PackageName);
	openName += TEXT(".");
	openName += ClassName;

	m_pIPC->SendCommandToVS(CMD_EditorLoadClass, *openName);
	m_pIPC->SendCommandToVS(CMD_EditorGotoLine, LineNumber, 1);

	m_pIPC->SendCommandToVS(CMD_EditorGotoLine, objectName);
}
void WTInterface::UpdateCallStack( TArray<FString>& StackNames )
{
	UpdateCallStack();
}
void WTInterface::UpdateCallStack()
{
	m_pIPC->SendCommandToVS(CMD_CallStackClear);
	for(int i=0;i < Debugger->CallStack->StackDepth;i++) 
	{
		const FStackNode* TestNode = Debugger->CallStack->GetNode(i);
		if (TestNode && TestNode->StackNode && TestNode->StackNode->Node)
		{
			INT line = TestNode->GetLine();
			m_pIPC->SendCommandToVS(CMD_CallStackAdd, line, 0, *TestNode->StackNode->Node->GetFullName());
		}
	}
	Show();
}

void WTInterface::SetBreakpoint( const TCHAR* ClassName, INT Line )
{
	FString upper(ClassName);
	upper = upper.ToUpper();
	Debugger->GetBreakpointManager()->SetBreakpoint( ClassName, Line );
	m_pIPC->SendCommandToVS(CMD_AddBreakpoint, Line, 0, *upper);
}

void WTInterface::RemoveBreakpoint( const TCHAR* ClassName, INT Line )
{
	FString upper(ClassName);
	upper = upper.ToUpper();
	Debugger->GetBreakpointManager()->RemoveBreakpoint( ClassName, Line );
	m_pIPC->SendCommandToVS(CMD_RemoveBreakpoint, Line, 0, *upper);
}	

void WTInterface::UpdateClassTree()
{
	ClassTree.Empty();
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* ParentClass = Cast<UClass>(It->SuperField);
		if (ParentClass)
			ClassTree.Add( ParentClass, *It );
	}

	m_pIPC->SendCommandToVS(CMD_ClearHierarchy);
	m_pIPC->SendCommandToVS(CMD_AddClassToHierarchy,  L"Core..Object" );
	RecurseClassTree( UObject::StaticClass() );
	m_pIPC->SendCommandToVS(CMD_BuildHierarchy);
}

void WTInterface::RecurseClassTree( UClass* ParentClass )
{
	TArray<UClass*> ChildClasses;
	ClassTree.MultiFind( ParentClass, ChildClasses );

	for (INT i = 0; i < ChildClasses.Num(); i++)
	{
		// Get package name
		FString FullName = ChildClasses(i)->GetFullName();
		int CutPos = FullName.InStr( TEXT(".") );

		// Extract the package name and chop off the 'Class' thing.
		FString PackageName = FullName.Left( CutPos );
		PackageName = PackageName.Right( PackageName.Len() - 6 );
		m_pIPC->SendCommandToVS(CMD_AddClassToHierarchy, ( *FString::Printf( TEXT("%s.%s.%s"), *PackageName, *ParentClass->GetName(), *ChildClasses(i)->GetName() )) );

		RecurseClassTree( ChildClasses(i) );
	}

	for ( INT i = ChildClasses.Num() - 1; i >= 0; i-- )
		ClassTree.RemovePair( ParentClass, ChildClasses(i) );
}

void WTInterface::LockWatch(int watch)
{
	m_locked = TRUE;
	m_pIPC->SendCommandToVS(CMD_LockList, watch);
}

void WTInterface::UnlockWatch(int watch)
{
	m_pIPC->SendCommandToVS(CMD_UnlockList, watch);
	m_locked = FALSE;
}

void WTInterface::BindToDll()
{
	warnf(TEXT("BINDING TO DLL (%s)"), m_pIPC ? TEXT("ALREADY BOUND!") : TEXT("GTG"));
	m_pIPC = new IPC_UC;
}

void WTInterface::UnbindDll()
{
	warnf(TEXT("UNBINDING (%s)"), m_pIPC ? TEXT("GTG") : TEXT("NOT BOUND!!"));
	if(m_pIPC)
	{
		delete m_pIPC;
		m_pIPC = NULL;
	}
}

