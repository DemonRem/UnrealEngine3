/*=============================================================================
	UnDebuggerCore.cpp: Debugger Core Logic
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "../Inc/EnginePrivate.h"
#include "UnDebuggerCore.h"
#include "UnDebuggerInterface.h"
#include "UnDelphiInterface.h"
#include "UnWTInterface.h"
#include "OpCode.h"

/*-----------------------------------------------------------------------------
	UDebuggerCore.
-----------------------------------------------------------------------------*/

static TCHAR GDebuggerIni[1024] = TEXT("");

UDebuggerCore::UDebuggerCore()
:	bIsDebugging(FALSE), 
	bIsClosing(FALSE), 
	CurrentState(NULL), 
	PendingState(NULL), 
	BreakpointManager(NULL),
	CallStack(NULL), 
	Interface(NULL),
	bAccessedNone(FALSE), 
	bBreakOnNone(FALSE), 
	bBreakASAP(FALSE), 
	bProcessDebugInfo(FALSE),
	ArrayMemberIndex(INDEX_NONE)
{
	appStrcpy( GDebuggerIni, *(appGameConfigDir() + TEXT("Debugger.ini")) );

	FFilename InterfaceDllName;
	if ( !GConfig->GetString(TEXT("DEBUGGER.DEBUGGER"), TEXT("InterfaceFilename"), InterfaceDllName, GDebuggerIni) )
	{
		InterfaceDllName = TEXT("DebuggerInterface.dll");
	}

	if ( InterfaceDllName.GetExtension() == TEXT("") )
	{
		InterfaceDllName += TEXT(".dll");
	}

	if( ParseParam(appCmdLine(),TEXT("VADEBUG")) )
	{
		Interface = new WTInterface(*InterfaceDllName);
	}
	else
	{
		Interface = new DelphiInterface(*InterfaceDllName);
	}

	if ( !Interface->Initialize( this ))
	{
		appErrorf( TEXT("Could not initialize the debugger interface!") );
	}

	DebuggerLog = new FDebuggerLog();
	GLog->AddOutputDevice( DebuggerLog );

	CallStack = new FCallStack( this );
	ChangeState(new DSIdleState(this));
	BreakpointManager = new FBreakpointManager();

	debugf( NAME_Init, TEXT("UnrealScript Debugger Core Initialized.") );

	// Init recursion limits
	FString Value;
	if ( GConfig->GetString(TEXT("DEBUGGER.RECURSION"), TEXT("OBJECTMAX"), Value, GDebuggerIni) )
		MaxObjectRecursion = appAtoi(*Value);
	else MaxObjectRecursion = 1;

	if ( GConfig->GetString(TEXT("DEBUGGER.RECURSION"),TEXT("STRUCTMAX"),Value,GDebuggerIni) )
		MaxStructRecursion = appAtoi(*Value);
	else MaxStructRecursion = INDEX_NONE;

	if ( GConfig->GetString(TEXT("DEBUGGER.RECURSION"),TEXT("CLASSMAX"),Value,GDebuggerIni) )
		MaxClassRecursion = appAtoi(*Value);
	else MaxClassRecursion = 1;

	if ( GConfig->GetString(TEXT("DEBUGGER.RECURSION"),TEXT("STATICARRAYMAX"),Value,GDebuggerIni) )
		MaxStaticArrayRecursion = appAtoi(*Value);
	else MaxStaticArrayRecursion = 2;

	if ( GConfig->GetString(TEXT("DEBUGGER.RECURSION"),TEXT("DYNAMICARRAYMAX"),Value,GDebuggerIni) )
		MaxDynamicArrayRecursion = appAtoi(*Value);
	else MaxDynamicArrayRecursion = 1;

	CurrentObjectRecursion = CurrentStructRecursion = CurrentClassRecursion = CurrentStaticArrayRecursion = CurrentDynamicArrayRecursion = 0;
}

UDebuggerCore::~UDebuggerCore()
{
	debugf( NAME_Init, TEXT("UnrealScript Debugger Core Exit.") );
	
	GConfig->SetString(TEXT("DEBUGGER.RECURSION"),TEXT("OBJECTMAX"),*appItoa(MaxObjectRecursion),GDebuggerIni);
	GConfig->SetString(TEXT("DEBUGGER.RECURSION"),TEXT("STRUCTMAX"),*appItoa(MaxStructRecursion),GDebuggerIni);
	GConfig->SetString(TEXT("DEBUGGER.RECURSION"),TEXT("CLASSMAX"),*appItoa(MaxClassRecursion),GDebuggerIni);
	GConfig->SetString(TEXT("DEBUGGER.RECURSION"),TEXT("STATICARRAYMAX"),*appItoa(MaxStaticArrayRecursion),GDebuggerIni);
	GConfig->SetString(TEXT("DEBUGGER.RECURSION"),TEXT("DYNAMICARRAYMAX"),*appItoa(MaxDynamicArrayRecursion),GDebuggerIni);
	GConfig->Flush(0, GDebuggerIni);

	if ( CurrentState )
	{
		delete CurrentState;
	}
	CurrentState = NULL;

	if(PendingState)
		delete PendingState;
	PendingState = NULL;

	if(BreakpointManager)
		delete BreakpointManager;
	BreakpointManager = NULL;
	
	if ( CallStack )
		delete CallStack;
	CallStack = NULL;
	
	if ( Interface )
	{
		Interface->Close();
		delete Interface;
	}
	Interface = NULL;

	GLog->RemoveOutputDevice( DebuggerLog );
	delete DebuggerLog;
	DebuggerLog = NULL;
}

void UDebuggerCore::Close()
{
	if ( IsDebuggerClosing() )
	{
		return;
	}

	SetDebuggerClosing(TRUE);
	if ( CallStack )
	{
		CallStack->Empty();
	}

	StackChanged(NULL);

	if ( CurrentState )
	{
		CurrentState->SetCurrentNode(NULL);
	}

	ChangeState(new DSIdleState(this));
}

void UDebuggerCore::NotifyBeginTick()
{
	EnableDebuggerProcessing(TRUE);
}

void UDebuggerCore::ProcessInput( enum EUserAction UserAction )
{
	CurrentState->HandleInput(UserAction);
}

const FStackNode* UDebuggerCore::GetCurrentNode() const
{
	const FStackNode* Node = CurrentState ? CurrentState->GetCurrentNode() : NULL;
	if ( Node == NULL )
	{
		Node = CallStack->GetTopNode();
	}

	return Node;
}

void UDebuggerCore::AddWatch(const TCHAR* watchName)
{
	FDebuggerWatch* NewWatch = new(Watches) FDebuggerWatch(ErrorDevice, watchName);

	FDebuggerState* State = GetCurrentState();

	if ( IsDebuggerActive() && State && NewWatch )
	{
		const FStackNode* Node = CallStack->Stack.IsValidIndex(CurrentStackPosition)
			? CallStack->GetNode(CurrentStackPosition)
			: State->GetCurrentNode();

		if ( Node )
		{
			NewWatch->Refresh( Node->Object, Node->StackNode );

			Interface->LockWatch(Interface->WATCH_WATCH);
			Interface->ClearAWatch(Interface->WATCH_WATCH);
			RefreshUserWatches();
			Interface->UnlockWatch(Interface->WATCH_WATCH);
		}
	}
}

void UDebuggerCore::RemoveWatch(const TCHAR* watchName)
{
	for ( INT i = 0; i < Watches.Num(); i++ )
	{
		if ( Watches(i).WatchText == watchName )
		{
			Watches.Remove(i);
			return;
		}
	}
}

void UDebuggerCore::ClearWatches()
{
	Watches.Empty();
}

#define SETPARENT(m,c,i) m.Set(c,i + 1)
#define GETPARENT(m,c)   m.FindRef(c) - 1

void UDebuggerCore::BuildParentChain( INT WatchType, TMap<UClass*,INT>& ParentChain, UClass* BaseClass, INT ParentIndex )
{
	if ( !BaseClass )
		return;

	ParentChain.Empty();
	SETPARENT(ParentChain,BaseClass,ParentIndex);

	for ( UClass* Parent = BaseClass->GetSuperClass(); Parent; Parent = Parent->GetSuperClass() )
	{
		if ( ParentChain.Find(Parent) == NULL )
		{
			ParentIndex = Interface->AddAWatch( WatchType, ParentIndex, *FString::Printf(TEXT("[[ %s ]]"), *Parent->GetName()), TEXT("[[ Base Class ]]") );
			SETPARENT(ParentChain,Parent,ParentIndex);
		}
	}
}

static UBOOL IsValidObject( UObject* Obj )
{
	if( !Obj )
		return 0;

	UObject* IndexedObject = UObject::GetIndexedObject(Obj->GetIndex());
	return ( IndexedObject != NULL && IndexedObject == Obj );
}

// Insert a given property into the watch window. 
void UDebuggerCore::PropertyToWatch( UProperty* Prop, BYTE* PropAddr, UBOOL bResetIndex, INT watch, const TCHAR* PropName )
{
	INT ParentIndex = INDEX_NONE;
	static TMap<UClass*,INT> InheritanceChain;

	if ( bResetIndex )
	{
		if ( watch == Interface->GLOBAL_WATCH )
			BuildParentChain(watch, InheritanceChain, ((UObject*)(PropAddr - Prop->Offset))->GetClass());
		else
			InheritanceChain.Empty();
	}

	ArrayMemberIndex = INDEX_NONE;
	ParentIndex = GETPARENT(InheritanceChain,Prop->GetOwnerClass());
	PropertyToWatch(Prop, PropAddr, 0, watch, ParentIndex, PropName);
}

// Extract the value of a given property at a given address
void UDebuggerCore::PropertyToWatch(UProperty* Prop, BYTE* PropAddr, INT CurrentDepth, INT watch, INT watchParent, const TCHAR* PropName )
{
	// This SHOULD be sufficient.
	FString VarName, VarValue;
	if ( ArrayMemberIndex < INDEX_NONE )
		ArrayMemberIndex = INDEX_NONE;

	if ( Prop->ArrayDim > 1 && ArrayMemberIndex < 0 )
	{
		if ( CurrentStaticArrayRecursion < MaxStaticArrayRecursion || MaxStaticArrayRecursion == INDEX_NONE )
		{
			VarName = PropName ? PropName : FString::Printf( TEXT("%s ( Static %s Array )"), *Prop->GetName(), GetShortName(Prop) );
			VarValue = FString::Printf(TEXT("%i Elements"), Prop->ArrayDim);

			INT WatchID = Interface->AddAWatch(watch, watchParent, *VarName, *VarValue);

			CurrentStaticArrayRecursion++;
			for ( INT i = 0; i < Prop->ArrayDim; i++ )
			{
				ArrayMemberIndex++;
				PropertyToWatch(Prop, PropAddr + Prop->ElementSize * i, CurrentDepth + 1, watch, WatchID);
			}
			CurrentStaticArrayRecursion--;

			ArrayMemberIndex = INDEX_NONE;
		}
		return;
	}

	VarName = PropName 
		? PropName : (ArrayMemberIndex >= 0
		? FString::Printf(TEXT("%s[%i]"), (Prop->IsA(UDelegateProperty::StaticClass()) ? *Cast<UDelegateProperty>(Prop)->Function->GetName() : *Prop->GetName()), ArrayMemberIndex)
		: (FString::Printf(TEXT("%s ( %s )"), (Prop->IsA(UDelegateProperty::StaticClass()) ? *Cast<UDelegateProperty>(Prop)->Function->GetName() : *Prop->GetName()), GetShortName(Prop))));


	if ( Prop->IsA(UStructProperty::StaticClass()) )
		VarValue = GetShortName(Prop);

	else if ( Prop->IsA(UArrayProperty::StaticClass()) )
		VarValue = FString::Printf(TEXT("%i %s %s"), ((FArray*)PropAddr)->Num(), GetShortName(Cast<UArrayProperty>(Prop)->Inner), ((FArray*)PropAddr)->Num() != 1 ? TEXT("Elements") : TEXT("Element"));

	else if ( Prop->IsA(UObjectProperty::StaticClass()) )
	{
		UObject* Obj = *(UObject**)PropAddr;
		if ( Obj )
		{
			if ( !IsValidObject(Obj) )
				VarValue = TEXT("** Destroyed **");
			else VarValue = Obj->GetName();
		}
		else VarValue = TEXT("None");
	}
	else
	{
		VarValue = TEXT("");
		Prop->ExportTextItem( VarValue, PropAddr, PropAddr, NULL, PPF_Delimited );
	}

	int ID = Interface->AddAWatch(watch, watchParent, *VarName, *VarValue);
	if ( Prop->IsA(UStructProperty::StaticClass()) && (CurrentStructRecursion < MaxStructRecursion || MaxStructRecursion == INDEX_NONE) )
	{
		INT CurrentIndex = ArrayMemberIndex;
		ArrayMemberIndex = INDEX_NONE;

		CurrentStructRecursion++;
		// Recurse every property in this struct, and copy it's value into Result;
		for( TFieldIterator<UProperty> It(Cast<UStructProperty>(Prop)->Struct); It; ++It )
		{
			if (Prop == *It) continue;

			// Special case for nested stuff, don't leave it up to VarName/VarValue since we need to recurse
			PropertyToWatch(*It, PropAddr + It->Offset, CurrentDepth + 1, watch, ID);
		}
		ArrayMemberIndex = CurrentIndex;
		CurrentStructRecursion--;
	}

	else if ( Prop->IsA(UClassProperty::StaticClass()) && (CurrentClassRecursion < MaxClassRecursion || MaxClassRecursion == INDEX_NONE) )
	{
		UClass* ClassResult = *(UClass**)PropAddr;
		if ( !ClassResult )
			return;

		INT CurrentIndex = ArrayMemberIndex, CurrentID;
		ArrayMemberIndex = INDEX_NONE;

		TMap<UClass*,INT> ParentChain;
		UClass* PropOwner = NULL;

		BuildParentChain(watch, ParentChain, ClassResult, ID);
		CurrentClassRecursion++;
		for ( TFieldIterator<UProperty> It(ClassResult); It; ++It )
		{
			if ( Prop == *It )
				continue;

			PropOwner = It->GetOwnerClass();
			if ( PropOwner == UObject::StaticClass() )
				continue;

			CurrentID = GETPARENT(ParentChain,PropOwner);
			PropertyToWatch(*It, ClassResult->GetDefaults() + It->Offset, CurrentDepth + 1, watch, CurrentID);
		}
		CurrentClassRecursion--;

		ArrayMemberIndex = CurrentIndex;
	}

	else if(Prop->IsA(UObjectProperty::StaticClass()) && (CurrentObjectRecursion < MaxObjectRecursion || MaxObjectRecursion == INDEX_NONE) )
	{
		UObject* ObjResult = *(UObject**)PropAddr;
		if ( !ObjResult )
			return;

		if ( !IsValidObject(ObjResult) )
			return;

		INT CurrentIndex = ArrayMemberIndex, CurrentID;
		ArrayMemberIndex = INDEX_NONE;

		TMap<UClass*,INT> ParentChain;

		BuildParentChain(watch, ParentChain, ObjResult->GetClass(), ID);
		CurrentObjectRecursion++;

		UClass* PropOwner = NULL;
		for( TFieldIterator<UProperty> It( ObjResult->GetClass() ); It; ++It )
		{
			if (Prop == *It) continue;

			PropOwner = It->GetOwnerClass();
			if ( PropOwner == UObject::StaticClass() ) continue;

			CurrentID = GETPARENT(ParentChain,PropOwner);
			PropertyToWatch( *It, (BYTE*)ObjResult + It->Offset, CurrentDepth + 1, watch, CurrentID );
		}
		ArrayMemberIndex = CurrentIndex;
		CurrentObjectRecursion--;
	}

	else if (Prop->IsA( UArrayProperty::StaticClass() ) && (CurrentDynamicArrayRecursion < MaxDynamicArrayRecursion || MaxDynamicArrayRecursion == INDEX_NONE) )
	{
		const INT Size		= Cast<UArrayProperty>(Prop)->Inner->ElementSize;
		FArray* Array		= ((FArray*)PropAddr);

		INT CurrentIndex = ArrayMemberIndex;
		ArrayMemberIndex = INDEX_NONE;

		CurrentDynamicArrayRecursion++;
		for ( INT i = 0; i < Array->Num(); i++ )
		{
			ArrayMemberIndex++;
			PropertyToWatch(Cast<UArrayProperty>(Prop)->Inner, (BYTE*)Array->GetData() + i * Size, CurrentDepth + 1, watch, ID );
		}

		ArrayMemberIndex = CurrentIndex;
		CurrentDynamicArrayRecursion--;
	}
}


void UDebuggerCore::SetCondition( const TCHAR* ConditionName, const TCHAR* ConditionValue )
{
	if ( GIsRequestingExit || IsDebuggerClosing() )
		return;

//	ChangeState( new DSCondition(this,ConditionName,ConditionValue,CurrentState) );
}

void UDebuggerCore::SetDataBreakpoint( const TCHAR* BreakpointName )
{
	if ( GIsRequestingExit || IsDebuggerClosing() )
		return;

	ChangeState( new DSBreakOnChange(this,BreakpointName,CurrentState) );
}

void UDebuggerCore::NotifyGC()
{
}

UBOOL UDebuggerCore::NotifyAssertionFailed( const INT LineNumber )
{
	if ( GIsRequestingExit || IsDebuggerClosing() )
		return 0;

	debugf(TEXT("Assertion failed, line %i"), LineNumber);

	ChangeState( new DSWaitForInput(this), TRUE );
	return !(GIsRequestingExit || IsDebuggerClosing());
}

UBOOL UDebuggerCore::NotifyInfiniteLoop()
{
	if ( GIsRequestingExit || IsDebuggerClosing() )
		return 0;

	debugf(TEXT("Recursion limit reached...breaking UDebugger"));

	ChangeState( new DSWaitForInput(this), TRUE );
	return !(GIsRequestingExit || IsDebuggerClosing());
}

void UDebuggerCore::ChangeStack( INT StackNodeIndex )
{
	if ( CurrentState == NULL || !CallStack->Stack.IsValidIndex(StackNodeIndex) )
		return;

	FStackNode* Node = CallStack->GetNode(CallStack->Stack.Num() - StackNodeIndex - 1);
	StackChanged(Node);
	Interface->Update(	*Node->GetClass()->GetName(),
						*Node->GetClass()->GetOuter()->GetName(),
						Node->GetLine(),
						Node->GetInfo(),
						*Node->GetObject()->GetName()
					 );

	RefreshWatch( Node );
}

void UDebuggerCore::StackChanged( const FStackNode* CurrentNode )
{
	CurrentStackPosition = CallStack->Stack.FindItemIndex(*CurrentNode);

	// For now, simply refresh user watches
	// later, we can modify this to work for all watches, allowing the ability to view values from anywhere on the callstack

	const UObject* Obj = CurrentNode ? CurrentNode->Object : NULL;
	const FFrame* Node = CurrentNode ? CurrentNode->StackNode : NULL;

	for ( INT i = 0; i < Watches.Num(); i++ )
	{
		Watches(i).Refresh(Obj, Node);
	}
}

// Update the interface
void UDebuggerCore::UpdateInterface()
{
	if ( IsDebuggerActive() && CallStack)
	{
		const FStackNode* TopNode = CallStack->GetTopNode();
		if ( !TopNode )
			return;
		
		// Get package name
		FString cName = TopNode->GetClass()->GetName(),
				pName = TopNode->GetClass()->GetOuter()->GetName();

		Interface->Update(	*cName, 
							*pName,
							TopNode->GetLine(),
							TopNode->GetInfo(),
							*TopNode->Object->GetName());
		RefreshWatch( TopNode );

		TArray<FString> StackNames;
		for(int i=0;i < CallStack->StackDepth;i++) 
		{
			const FStackNode* TestNode = CallStack->GetNode(i);
			if (TestNode && TestNode->StackNode && TestNode->StackNode->Node)
				new(StackNames) FString( TestNode->StackNode->Node->GetFullName() );
		}
		Interface->UpdateCallStack( StackNames );
	}
}

// Update the Watch ListView with all the current variables the Stack/Object contain.
void UDebuggerCore::RefreshWatch( const FStackNode* CNode )
{
	TArray<INT> foundWatchNamesIndicies;

	if ( CNode == NULL )
		return;

	Interface->LockWatch(Interface->GLOBAL_WATCH);
	Interface->LockWatch(Interface->LOCAL_WATCH);
	Interface->LockWatch(Interface->WATCH_WATCH);
	Interface->ClearAWatch(Interface->GLOBAL_WATCH);
	Interface->ClearAWatch(Interface->LOCAL_WATCH);
	Interface->ClearAWatch(Interface->WATCH_WATCH);

	UFunction* Function = Cast<UFunction>(CNode->GetFrame()->Node);
	const UObject* ContextObject = CNode->GetObject();
	UProperty* Parm;

	// Setup the local variable watch
	if ( Function )
	{
		for ( Parm = Function->PropertyLink; Parm; Parm = Parm->PropertyLinkNext )
			PropertyToWatch( Parm, CNode->GetFrame()->Locals + Parm->Offset, Parm == Function->PropertyLink, Interface->LOCAL_WATCH );
	}

	// Setup the global vars watch
	TFieldIterator<UProperty,CLASS_IsAUProperty> PropertyIt(ContextObject->GetClass());
	for( Parm = *PropertyIt; PropertyIt; ++PropertyIt )
		PropertyToWatch( *PropertyIt, (BYTE*)ContextObject + PropertyIt->Offset, *PropertyIt == Parm, Interface->GLOBAL_WATCH );

	RefreshUserWatches();

	Interface->UnlockWatch(Interface->GLOBAL_WATCH);
	Interface->UnlockWatch(Interface->LOCAL_WATCH);
	Interface->UnlockWatch(Interface->WATCH_WATCH);
}

void UDebuggerCore::RefreshUserWatches()
{
	// Fill the custom watch values from the context of the current node
	for ( INT i = 0; i < Watches.Num(); i++ )
	{
		UProperty* Prop = NULL;
		BYTE* PropAddr = NULL;
		ErrorDevice.Empty();

		FDebuggerWatch& Watch = Watches(i);

		if ( Watch.GetWatchValue((const UProperty *&) Prop, (const BYTE *&) PropAddr, ArrayMemberIndex) )
			PropertyToWatch(Prop, PropAddr, 0, Interface->WATCH_WATCH, INDEX_NONE, *Watch.WatchText);
		else Interface->AddAWatch( Interface->WATCH_WATCH, -1, *Watch.WatchText, *ErrorDevice );
	}
}

UClass* UDebuggerCore::GetStackOwnerClass( const FFrame* Stack ) const
{
	return Stack->Node->GetOwnerClass();
}


/**	 
 * Routes message to the debugger if present.
 *
 * @param	Msg		Message to route
 * @param	Event	Event type of message
 */
void FDebuggerLog::Serialize( const TCHAR* Msg, EName Event )
{
	if ( Event != NAME_Title && Event != NAME_Color )
	{
		UDebuggerCore* Debugger = (UDebuggerCore*)GDebugger;
		if ( Debugger && Debugger->Interface && Debugger->Interface->IsLoaded() )
		{
			Debugger->Interface->AddToLog( *FString::Printf(TEXT("%s: %s"), *FName::SafeString(Event), Msg) );
		}
	}
}

/*-----------------------------------------------------------------------------
	FCallStack.
-----------------------------------------------------------------------------*/

FCallStack::FCallStack( UDebuggerCore* InParent )
: Parent(InParent), StackDepth(0)
{
}

FCallStack::~FCallStack()
{
	Empty();
	Parent = NULL;
}

void FCallStack::Empty()
{
	QueuedCommands.Empty();
	Stack.Empty();
	StackDepth = 0;
}

/*-----------------------------------------------------------------------------
	FStackNode Implementation
-----------------------------------------------------------------------------*/

FStackNode::FStackNode( const UObject* Debugee, const FFrame* Stack, UClass* InClass, INT CurrentDepth, INT InLineNumber, INT InPos, BYTE InCode )
: Object(Debugee), StackNode(Stack), Class(InClass), breakpointMutex(0)
{
	Lines.AddItem(InLineNumber);
	Positions.AddItem(InPos);
	Depths.AddItem(CurrentDepth);
	OpCodes.AddItem(InCode);
}

const TCHAR* FStackNode::GetInfo() const
{
	return GetOpCodeName(OpCodes.Last());
}

void FStackNode::Show() const
{
	debugf(TEXT("Object:%s  Class:%s  Line:%i  Pos:%i  Code:%s"),
		Object ? *Object->GetName() : TEXT("NULL"),
		Class  ? *Class->GetName()  : TEXT("NULL"),
		GetLine(), GetPos(), GetInfo());
}

void FStackNode::Update( INT LineNumber, INT InputPos, BYTE OpCode, INT CurrentDepth )
{
	Lines.AddItem(LineNumber);
	Positions.AddItem(InputPos);
	Depths.AddItem(CurrentDepth);
	OpCodes.AddItem(OpCode);

	if ( OpCode == DI_ForInc || OpCode == DI_While )
	{
		// clear breakpoint mutex when beginning the next iteration of a loop
		breakpointMutex = 0;
	}
}
/*-----------------------------------------------------------------------------
	FDebuggerWatch
-----------------------------------------------------------------------------*/

static TCHAR* ParseNextName( TCHAR*& o )
{
	INT count(0);

	bool literal=false; // literal object name

	TCHAR* c = o;
	while ( c && *c )
	{
		if ( *c == '[' )
			count++;

		else if ( *c == ']')
			count--;

		else if ( count == 0 )
		{
			if ( *c == '\'' )
				literal = !literal;
			else if ( !literal )
			{
				if ( *c == '(' )
				{
					o = c;
					*o++ = 0;
				}
				else if ( *c == ')' )
					*c = 0;

				else if ( *c == '.' )
				{
					*c++ = 0;
					return c;
				}
			}
		}

		c++;
	}

	return NULL;
}

FDebuggerWatch::FDebuggerWatch(FStringOutputDevice& ErrorHandler, const TCHAR* WatchString )
: WatchText(WatchString)
{
	WatchNode = new FDebuggerWatchNode(ErrorHandler, WatchString);
}

void FDebuggerWatch::Refresh( const UObject* CurrentObject, const FFrame* CurrentFrame )
{
	if ( !CurrentObject || !CurrentFrame )
		return;

	Object = CurrentObject;
	Class = CurrentObject->GetClass();
	Function = Cast<UFunction>(CurrentFrame->Node);

	if ( WatchNode )
		WatchNode->ResetBase(Class, Object, Function, (BYTE*)Object, CurrentFrame->Locals);
}

UBOOL FDebuggerWatch::GetWatchValue( const UProperty*& OutProp, const BYTE*& OutPropAddr, INT& ArrayIndexOverride )
{
	if ( WatchNode && WatchNode->Refresh(Class, Object, (BYTE*)Object ) )
		return WatchNode->GetWatchValue(OutProp, OutPropAddr, ArrayIndexOverride);

	return 0;
}

FDebuggerWatch::~FDebuggerWatch()
{
	if ( WatchNode )
		delete WatchNode;

	WatchNode = NULL;
}

/*-----------------------------------------------------------------------------
	FDebuggerDataWatch
-----------------------------------------------------------------------------*/
FDebuggerDataWatch::FDebuggerDataWatch( FStringOutputDevice& ErrorHandler, const TCHAR* WatchString )
: FDebuggerWatch(ErrorHandler, WatchString)
{ }

void FDebuggerDataWatch::Refresh( const UObject* CurrentObject, const FFrame* CurrentFrame )
{
	// reset the current value of the watch
}

UBOOL FDebuggerDataWatch::GetWatchValue( const UProperty*& OutProp, const BYTE*& OutPropAddr, INT& ArrayIndexOverride )
{
	return 0;
}

UBOOL FDebuggerDataWatch::Modified() const
{
	check(Property);

	// TODO for arrays that have been reduced in size, this will crash

	return Property->Identical( OriginalValue, DataAddress );
}

/*-----------------------------------------------------------------------------
	FDebuggerWatchNode
-----------------------------------------------------------------------------*/
FDebuggerWatchNode::FDebuggerWatchNode( FStringOutputDevice& ErrorHandler, const TCHAR* NodeText )
: NextNode(NULL), ArrayNode(NULL), PropAddr(NULL), Property(NULL), GlobalData(NULL), Base(NULL), LocalData(NULL),
  Function(NULL), TopObject(NULL), ContextObject(NULL), TopClass(NULL), ContextClass(NULL), Error(ErrorHandler)
{
	TCHAR* Buffer = new TCHAR [ appStrlen(NodeText) + 1 ];
	appStrncpy(Buffer, NodeText, appStrlen(NodeText) + 1);

	TCHAR* NodeName = Buffer;
	TCHAR* Next = ParseNextName(NodeName);
	if ( Next )
		NextNode = new FDebuggerWatchNode(Error, Next);

	PropertyName = NodeName;

	FString ArrayDelim;
	if ( GetArrayDelimiter(PropertyName, ArrayDelim) )
		AddArrayNode(*ArrayDelim);

	delete[] Buffer;
}

FDebuggerWatchNode::~FDebuggerWatchNode()
{
	if ( NextNode )
		delete NextNode;

	if ( ArrayNode )
		delete ArrayNode;

	NextNode = NULL;
	ArrayNode = NULL;
}

UBOOL FDebuggerWatchNode::GetArrayDelimiter( FString& Test, FString& Result ) const
{
	Result = TEXT("");
	INT pos = Test.InStr(TEXT("["));
	if ( pos != INDEX_NONE )
	{
		Result = Test.Mid(pos+1);
		Test = Test.Left(pos);

		pos = Result.InStr(TEXT("]"),1);
		if ( pos != INDEX_NONE )
			Result = Result.Left(pos);
	}

	return Result.Len();
}

void FDebuggerWatchNode::AddArrayNode( const TCHAR* ArrayText )
{
	if ( !ArrayText || !(*ArrayText) )
		return;

	ArrayNode = new FDebuggerArrayNode(Error, ArrayText);
}

void FDebuggerWatchNode::ResetBase( const UClass* CurrentClass, const UObject* CurrentObject, const UFunction* CurrentFunction, const BYTE* CurrentBase, const BYTE* CurrentLocals )
{
	TopClass   = CurrentClass;
	TopObject  = CurrentObject;
	Function   = CurrentFunction;
	GlobalData = CurrentBase;
	LocalData  = CurrentLocals;

	if ( NextNode )
		NextNode->ResetBase(CurrentClass, CurrentObject, CurrentFunction, CurrentBase, CurrentLocals);

	if ( ArrayNode )
		ArrayNode->ResetBase(CurrentClass, CurrentObject, CurrentFunction, CurrentBase, CurrentLocals);
}

UBOOL FDebuggerWatchNode::Refresh( const UStruct* RelativeClass, const UObject* RelativeObject, const BYTE* Data )
{
	ContextObject = RelativeObject;
	ContextClass = RelativeClass;

	check(ContextClass);

	if ( !Data )
	{
		if ( IsDebuggerPresent() )
			appDebugBreak();
		else
			appErrorf(NAME_FriendlyError, TEXT("Corrupted data found in user watch %s (class:%s function:%s)"), *PropertyName, *ContextClass->GetName(), *Function->GetName());

		return 0;
	}

	Property = NULL;
	PropAddr = NULL;
	Base = Data;

	if ( Data == GlobalData )
	{
		// Current context is the current function - allow searching the local parameters for properties
		Property = FindField<UProperty>( const_cast<UFunction*>(Function), *PropertyName);
		if ( Property )
			PropAddr = LocalData + Property->Offset;
	}

	if ( !Property )
	{
		Property = FindField<UProperty>( const_cast<UStruct*>(ContextClass), *PropertyName);
		if ( Property )
			PropAddr = Base + Property->Offset;
	}

/*	if ( !Property )
	{
		UObject* Obj = FindObject<UObject>(ANY_PACKAGE,*PropertyName);
		if ( Obj )
		{
			ContextObject = Obj;
			ContextClass  = Obj->GetClass();
		}
	}
*/
	ArrayIndex = GetArrayIndex();

	if ( !Property )
	{
		Error.Logf(TEXT("Member '%s' couldn't be found in local or global scope '%s'"), *PropertyName, *ContextClass->GetName());
		return 0;
	}

	if ( ArrayIndex < INDEX_NONE )
		return 0;

	return 1;
}

INT FDebuggerWatchNode::GetArrayIndex() const
{
	if ( ArrayNode )
		return ArrayNode->GetArrayIndex();

	return INDEX_NONE;
}


// ArrayIndexOverride is to prevent PropertyToWatch from incorrectly interpreting individual elements of static arrays as the entire array

UBOOL FDebuggerWatchNode::GetWatchValue( const UProperty*& OutProp, const BYTE*& OutPropAddr, INT& ArrayIndexOverride ) const
{
	if ( Property == NULL )
	{
		Error.Logf(TEXT("Member '%s' couldn't be found in local or global scope '%s'"), *PropertyName, *ContextClass->GetName());
		return 0;
	}	
	else if ( PropAddr == NULL )
	{
		Error.Logf(TEXT("Member '%s' couldn't be found in local or global scope '%s'"), *PropertyName, *ContextClass->GetName());
		return 0;
	}

	const UStructProperty* StructProperty = NULL;
	const UArrayProperty* ArrayProperty = NULL;
	const UObjectProperty* ObjProperty = NULL;
	const UClassProperty* ClassProperty = ConstCast<UClassProperty>(Property);
	if ( !ClassProperty )
	{
		ObjProperty = ConstCast<UObjectProperty>(Property);
		if ( !ObjProperty )
		{
			ArrayProperty = ConstCast<UArrayProperty>(Property);
			if ( !ArrayProperty )
				StructProperty = ConstCast<UStructProperty>(Property);
		}
	}

	if ( ObjProperty )
	{
		INT Index = Max(ArrayIndex,0);

		if ( Index >= ObjProperty->ArrayDim )
		{
			Error.Logf(TEXT("Index (%i) out of bounds: %s array only has %i elements"), Index, *ObjProperty->GetName(), ObjProperty->ArrayDim);
			return 0;
		}

		const BYTE* Data = PropAddr + Max(ArrayIndex, 0) * Property->ElementSize;
		const UObject* Obj = *(UObject**)Data;

		if ( NextNode )
		{
			if ( !Obj )
			{
				Error.Logf(TEXT("Expression could not be evaluated: Value of '%s' is None"), *PropertyName);
				return 0;
			}

			if ( !NextNode->Refresh( Obj ? Obj->GetClass() : ObjProperty->PropertyClass, Obj, (BYTE*)Obj ) )
				return 0;

			return NextNode->GetWatchValue( OutProp, OutPropAddr, ArrayIndexOverride );
		}

		OutProp = Property;
		OutPropAddr = Data;
		ArrayIndexOverride = ArrayIndex;
		return 1;
	}

	else if ( ClassProperty )
	{
		INT Index = Max(ArrayIndex,0);
		if ( Index >= ClassProperty->ArrayDim )
		{
			Error.Logf(TEXT("Index (%i) out of bounds: %s array only has %i elements"), Index, *ClassProperty->GetName(), ClassProperty->ArrayDim);
			return 0;
		}

		const BYTE* Data = PropAddr + Index * Property->ElementSize;
		UClass* Cls = *(UClass**)Data;
		if ( NextNode )
		{
			if ( !Cls )
			{
				Error.Logf(TEXT("Expression couldn't be evaluated: Value of '%s' is None"), *PropertyName);
				return 0;
			}

			if ( !NextNode->Refresh(Cls ? Cls : ClassProperty->MetaClass, Cls ? Cls->GetDefaultObject() : NULL, Cls->GetDefaults()) )
				return 0;

			return NextNode->GetWatchValue( OutProp, OutPropAddr, ArrayIndexOverride );
		}

		OutProp = Property;
		OutPropAddr = Data;
		ArrayIndexOverride = ArrayIndex;
		return 1;
	}

	else if ( StructProperty )
	{
		INT Index = Max(ArrayIndex,0);
		if ( Index >= StructProperty->ArrayDim )
		{
			Error.Logf(TEXT("Index (%i) out of bounds: %s array only has %i elements"), Index, *StructProperty->GetName(), StructProperty->ArrayDim);
			return 0;
		}

		const BYTE* Data = PropAddr + Index * Property->ElementSize;
		UStruct* Struct = StructProperty->Struct;
		if ( Struct )
		{
			if ( NextNode )
			{
				if ( !NextNode->Refresh( Struct, ContextObject, Data ) )
					return 0;

                return NextNode->GetWatchValue(OutProp, OutPropAddr, ArrayIndexOverride);
			}

			OutProp = StructProperty;
			OutPropAddr = Data;
			ArrayIndexOverride = ArrayIndex;
			return 1;
		}

		Error.Logf(TEXT("No data could be found for struct '%s'"), *Property->GetName());
		return 0;
	}

	else if ( ArrayProperty )
	{
		const FArray* Array = (FArray*)PropAddr;
		if ( Array )
		{
			// If the array index is -1, then we want the entire array, not just a single element
			if ( ArrayIndex != INDEX_NONE )
			{
				if ( ArrayIndex < 0 || ArrayIndex >= Array->Num() )
				{
					Error.Logf(TEXT("Index (%i) out of bounds: %s array only has %i element%s"), ArrayIndex, *Property->GetName(), Array->Num(), Array->Num() == 1 ? TEXT("") : TEXT("s"));
					return 0;
				}

				ObjProperty = NULL;
				StructProperty = NULL;
				ClassProperty = ConstCast<UClassProperty>(ArrayProperty->Inner);
				if ( !ClassProperty )
				{
					ObjProperty = Cast<UObjectProperty>(ArrayProperty->Inner);
					if ( !ObjProperty )
                        StructProperty = ConstCast<UStructProperty>(ArrayProperty->Inner);
				}

				if ( ObjProperty )
				{
					UObject* Obj = *(UObject**)((BYTE*)Array->GetData() + ArrayIndex * ObjProperty->ElementSize);

					// object is none
					if ( NextNode )
					{
						if ( !NextNode->Refresh( Obj ? Obj->GetClass() : ObjProperty->PropertyClass, Obj, (BYTE*)Obj ) )
							return 0;

						return NextNode->GetWatchValue( OutProp, OutPropAddr, ArrayIndexOverride );
					}

					OutProp = ObjProperty;
					OutPropAddr = (BYTE*)Array->GetData() + ArrayIndex * ObjProperty->ElementSize;
					ArrayIndexOverride = ArrayIndex;
					return 1;
				}

				else if ( ClassProperty )
				{
					const BYTE* Data = ((BYTE*)Array->GetData() + ClassProperty->ElementSize * ArrayIndex);
					UClass* Cls = *(UClass**) Data;

					if ( NextNode )
					{
						if ( !NextNode->Refresh( Cls ? Cls : ClassProperty->MetaClass, Cls ? Cls->GetDefaultObject() : NULL, Cls ? Cls->GetDefaults() : NULL ) )
							return 0;

						return NextNode->GetWatchValue(OutProp, OutPropAddr, ArrayIndexOverride);
					}

					OutProp = ClassProperty;
					OutPropAddr = Data;
					ArrayIndexOverride = ArrayIndex;
					return 1;
				}

				else if ( StructProperty )
				{
					const BYTE* Data = (BYTE*)Array->GetData() + StructProperty->ElementSize * ArrayIndex;
					UStruct* Struct = StructProperty->Struct;
					if ( Struct )
					{
						if ( NextNode )
						{
							if ( !NextNode->Refresh( Struct, NULL, Data ) )
								return 0;

							return NextNode->GetWatchValue(OutProp, OutPropAddr, ArrayIndexOverride);
						}

						OutProp = StructProperty;
						OutPropAddr = Data;
						ArrayIndexOverride = ArrayIndex;
						return 1;
					}

					Error.Logf(TEXT("No data could be found for struct '%s'"), *StructProperty->GetName());
					return 0;
				}

				else 
				{
					OutProp = ArrayProperty->Inner;
					OutPropAddr = (BYTE*)Array->GetData() + OutProp->ElementSize * ArrayIndex;
					ArrayIndexOverride = ArrayIndex;
					return 1;
				}
			}
		}

		else
		{
			Error.Logf(TEXT("No data could be found for array '%s'"), *Property->GetName());
			return 0;
		}
	}

	OutProp = Property;
	OutPropAddr = PropAddr + Max(ArrayIndex,0) * Property->ElementSize;
	ArrayIndexOverride = ArrayIndex;

	return 1;
}

/*-----------------------------------------------------------------------------
	FDebuggerArrayNode
-----------------------------------------------------------------------------*/

FDebuggerArrayNode::FDebuggerArrayNode(FStringOutputDevice& ErrorHandler, const TCHAR* ArrayText )
: FDebuggerWatchNode(ErrorHandler, ArrayText)
{
}

FDebuggerArrayNode::~FDebuggerArrayNode()
{
}

void FDebuggerArrayNode::ResetBase( const UClass* CurrentClass, const UObject* CurrentObject, const UFunction* CurrentFunction, const BYTE* CurrentBase, const BYTE* CurrentLocals )
{
	Value = INDEX_NONE;

	if ( PropertyName.IsNumeric() )
	{
		Value = appAtoi(*PropertyName);
		return;
	}

	FDebuggerWatchNode::ResetBase(CurrentClass, CurrentObject, CurrentFunction, CurrentBase, CurrentLocals);
	Refresh(CurrentClass, CurrentObject, CurrentBase);
}

INT FDebuggerArrayNode::GetArrayIndex() const
{
	// if the property is simply a number, just return that
	if ( Value != INDEX_NONE && PropertyName.IsNumeric() )
		return Value;

	const UProperty* Prop = NULL;
	const BYTE* Data = NULL;

	INT dummy(0);
	if ( GetWatchValue(Prop, Data, dummy) )
	{
		FString Buffer = TEXT("");

		// Must const_cast here because ExportTextItem isn't made const even though it doesn't modify the property value.
		Prop->ExportTextItem(Buffer, const_cast<BYTE*>(Data), NULL, NULL, 0);
		Value = appAtoi(*Buffer);
	}
	else return INDEX_NONE - 1;

	return Value;
}

/*-----------------------------------------------------------------------------
	Breakpoints
-----------------------------------------------------------------------------*/

FBreakpoint::FBreakpoint( const TCHAR* InClassName, INT InLine )
{
	ClassName	= InClassName;
	Line		= InLine;
	IsEnabled	= true;
}

UBOOL FBreakpointManager::QueryBreakpoint( const TCHAR* sClassName, INT sLine )
{
	for(int i=0;i<Breakpoints.Num();i++)
	{
		FBreakpoint& Breakpoint = Breakpoints(i);
		if ( Breakpoint.IsEnabled && Breakpoint.ClassName == sClassName && Breakpoint.Line == sLine )
		{
			return 1;
		}
	}
	
	return 0;
}


void FBreakpointManager::SetBreakpoint( const TCHAR* sClassName, INT sLine )
{
	for(int i=0;i<Breakpoints.Num();i++)
	{
		if ( Breakpoints(i).ClassName == sClassName && Breakpoints(i).Line == sLine )
			return;
	}

	new(Breakpoints) FBreakpoint( sClassName, sLine );
}


void FBreakpointManager::RemoveBreakpoint( const TCHAR* sClassName, INT sLine )
{
	for( INT i=0; i<Breakpoints.Num(); i++ )
	{
		if ( Breakpoints(i).ClassName == sClassName && Breakpoints(i).Line == sLine )
		{	
			Breakpoints.Remove(i--);
		}
	}
}

/*-----------------------------------------------------------------------------
	Debugger states
-----------------------------------------------------------------------------*/

FDebuggerState::FDebuggerState(UDebuggerCore* const inDebugger)
: CurrentNode(NULL), Debugger(inDebugger), LineNumber(INDEX_NONE), EvalDepth(inDebugger->CallStack->StackDepth)
{
	const FStackNode* Node = Debugger->GetCurrentNode();
	if ( Node )	
		LineNumber = Node->GetLine();
}

FDebuggerState::~FDebuggerState()
{
}

void FDebuggerState::UpdateStackInfo( const FStackNode* CNode )
{
	if ( Debugger != NULL && ((Debugger->IsDebuggerActive() && CNode != GetCurrentNode()) || (CNode == NULL)) )
	{
		Debugger->StackChanged(CNode);
		if ( CNode == NULL )
		{
			Debugger->ActivateDebugger(FALSE);
		}
	}

	SetCurrentNode(CNode);
}

/*-----------------------------------------------------------------------------
	Constructors.
-----------------------------------------------------------------------------*/

DSIdleState::DSIdleState(UDebuggerCore* const inDebugger)
: FDebuggerState(inDebugger)
{
	Debugger->ActivateDebugger(FALSE);
}

DSWaitForInput::DSWaitForInput(UDebuggerCore* const inDebugger)
: FDebuggerState(inDebugger), bContinue(FALSE)
{
	Debugger->ActivateDebugger(TRUE);
}

DSWaitForCondition::DSWaitForCondition(UDebuggerCore* const inDebugger)
: FDebuggerState(inDebugger)
{
	Debugger->ActivateDebugger(FALSE);
}

DSBreakOnChange::DSBreakOnChange( UDebuggerCore* const inDebugger, const TCHAR* WatchText, FDebuggerState* NewState )
: DSWaitForCondition(inDebugger), SubState(NewState), Watch(NULL), bDataBreak(0)
{
	Watch = new FDebuggerDataWatch(Debugger->ErrorDevice, WatchText);
	const FStackNode* StackNode = SubState ? SubState->GetCurrentNode() : NULL;
	const UObject* Obj = StackNode ? StackNode->Object : NULL;
	const FFrame* Node = StackNode ? StackNode->StackNode : NULL;

	Watch->Refresh( Obj, Node );
}

DSBreakOnChange::~DSBreakOnChange()
{
	if ( SubState )
		delete SubState;

	SubState = NULL;
}

DSRunToCursor::DSRunToCursor( UDebuggerCore* const inDebugger )
: DSWaitForCondition(inDebugger) 
{ }


DSStepOut::DSStepOut( UDebuggerCore* const inDebugger ) 
: DSWaitForCondition(inDebugger)
{ }

DSStepInto::DSStepInto( UDebuggerCore* const inDebugger )
: DSWaitForCondition(inDebugger)
{ }

DSStepOverStack::DSStepOverStack( const UObject* inObject, UDebuggerCore* const inDebugger ) 
: DSWaitForCondition(inDebugger), EvalObject(inObject)
{
}

/*-----------------------------------------------------------------------------
	DSBreakOnChange specifics.
-----------------------------------------------------------------------------*/
void DSBreakOnChange::SetCurrentNode( const FStackNode* Node )
{
	if ( SubState )
		SubState->SetCurrentNode(Node);
	else 
		DSWaitForCondition::SetCurrentNode(Node);
}

FDebuggerState* DSBreakOnChange::GetCurrent()
{
	if ( SubState )
		return SubState->GetCurrent();

	return FDebuggerState::GetCurrent();
}

const FStackNode* DSBreakOnChange::GetCurrentNode() const
{
	if ( SubState )
		return SubState->GetCurrentNode();

	return FDebuggerState::GetCurrentNode();
}

UBOOL DSBreakOnChange::InterceptNewState( FDebuggerState* NewState )
{
	if ( !NewState )
		return 0;

	if ( SubState )
	{
		if ( SubState->InterceptNewState(NewState) )
			return 1;

		delete SubState;
	}

	SubState = NewState;
	return 1;
}

UBOOL DSBreakOnChange::InterceptOldState( FDebuggerState* OldState )
{
	if ( !OldState || !SubState || OldState == this )
		return 0;

	if ( SubState && SubState->InterceptOldState(OldState) )
		return 1;

	return OldState == SubState;
}

/*-----------------------------------------------------------------------------
	HandleInput.
-----------------------------------------------------------------------------*/
void DSBreakOnChange::HandleInput( EUserAction Action )
{
	if ( Action >= UA_MAX )
		appErrorf(NAME_FriendlyError, TEXT("Invalid UserAction received by HandleInput()!"));

	if ( Action != UA_Exit && Action != UA_None && bDataBreak )
	{
		// refresh the watch's value with the current value
	}

	bDataBreak = 0;
	if ( SubState )
		SubState->HandleInput(Action);
}

void DSWaitForInput::HandleInput( EUserAction UserInput ) 
{
	ContinueExecution();
	switch ( UserInput )
	{
	case UA_RunToCursor:
		/*CHARRANGE sel;

		RichEdit_ExGetSel (Parent->Edit.hWnd, &sel);

		if ( sel.cpMax != sel.cpMin )
		{

		//appMsgf(0,*LocalizeUnrealEd("Error_InvalidCursorPosition"));			
			return;
		}
		Parent->ChangeState( new DSRunToCursor( sel.cpMax, Parent->GetCallStack()->GetStackDepth() ) );*/
		Debugger->ActivateDebugger(FALSE);
		break;
	case UA_Exit:
		GIsRequestingExit = 1;
		Debugger->Close();
		break;
	case UA_StepInto:
		Debugger->ChangeState( new DSStepInto(Debugger) );
		break;

	case UA_StepOver:
	/*	if ( CurrentInfo != TEXT("RETURN") && CurrentInfo != TEXT("RETURNNOTHING") )
		{
			
			Debugger->ChangeState( new DSStepOver( CurrentObject,
												 CurrentClass,
												 CurrentStack, 
												 CurrentLine, 
												 CurrentPos, 
												 CurrentInfo,
												 Debugger->GetCallStack()->GetStackDepth(), Debugger ) );
			

		}*/
		debugf(TEXT("Warning: UA_StepOver currently unimplemented"));
//		Debugger->ActivateDebugger(TRUE);
		break;
	case UA_StepOverStack:
		{
			const FStackNode* Top = Debugger->CallStack->GetTopNode();
			check(Top);

			Debugger->ChangeState( new DSStepOverStack(Top->Object,Debugger) );
		}

		break;

	case UA_StepOut:
		Debugger->ChangeState( new DSStepOut(Debugger) );
		break;

	case UA_Go:
		Debugger->ChangeState( new DSIdleState(Debugger) );
		break;
	}
}

/*-----------------------------------------------------------------------------
	Process.
-----------------------------------------------------------------------------*/
void FDebuggerState::Process( UBOOL bOptional )
{
	if ( !Debugger->IsDebuggerClosing() && Debugger->GetCallStack()->GetStackDepth() > 0 && EvaluateCondition(bOptional) )
		Debugger->Break();
}

void DSWaitForInput::Process(UBOOL bOptional) 
{
	if( Debugger->IsDebuggerClosing() )
		return;

	Debugger->SetAccessedNone(FALSE);
	Debugger->SetBreakASAP(FALSE);

	Debugger->UpdateInterface();
	ContinueExecution(FALSE);

	Debugger->GetInterface()->Show();

	PumpMessages();
}

void DSWaitForCondition::Process(UBOOL bOptional) 
{
	check(Debugger);

	const FStackNode* Node = GetCurrentNode();
	check(Node);

	if ( !Debugger->IsDebuggerClosing() && Debugger->GetCallStack()->GetStackDepth() > 0 && Node->GetLine() > 0 && EvaluateCondition(bOptional) )
	{
		// Condition was MET. We now delegate control to a user-controlled state.
		if ( Node && Node->StackNode && Node->StackNode->Node && Node->StackNode->Node->IsA(UClass::StaticClass()) )
		{
			return;
		}

		Debugger->Break();
	}
// #if _DEBUG
// 	else
// 	{
// 		Node->Show();
// 	}
// #endif
}

void DSBreakOnChange::Process(UBOOL bOptional)
{
	check(Debugger);

	const FStackNode* Node = GetCurrentNode();
	check(Node);

	if ( !Debugger->IsDebuggerClosing() && Debugger->GetCallStack()->GetStackDepth() > 0 && EvaluateCondition(bOptional) )
	{
		if ( Node && Node->StackNode && Node->StackNode->Node && Node->StackNode->Node->IsA(UClass::StaticClass()) )
		{
			if ( IsDebuggerPresent() )
				appDebugBreak();

			return;
		}

		// TODO : post message box with reason for breaking the udebugger
		Debugger->Break();
		return;
	}

	if ( SubState )
		SubState->Process(bOptional);
}

void DSWaitForInput::PumpMessages() 
{
	while( !bContinue && !Debugger->IsDebuggerClosing() && !GIsRequestingExit )
	{
		if ( GEngine )
		{
			check(GEngine->Client);
			GEngine->Client->AllowMessageProcessing( FALSE );
		}
		MSG Msg;
		while( PeekMessageW(&Msg,NULL,0,0,PM_REMOVE) )
		{
			if( Msg.message == WM_QUIT )
			{
				GIsRequestingExit = 1;
				ContinueExecution();
			}

			TranslateMessage( &Msg );
			DispatchMessageW( &Msg );
		}

		if ( GEngine )
		{
			GEngine->Client->AllowMessageProcessing( TRUE );
		}
	}
}

void DSWaitForInput::ContinueExecution( UBOOL bShouldContinue/*=TRUE*/ ) 
{
	bContinue = bShouldContinue;
}

UBOOL FDebuggerState::EvaluateCondition( UBOOL bOptional )
{
	const FStackNode* Node = GetCurrentNode();
	check(Node);
	check(Debugger->GetCallStack()->GetStackDepth() > 0);
	check(Debugger);
	check(Debugger->BreakpointManager);
	check(!Debugger->IsDebuggerClosing());

	// Check if we've hit a breakpoint
	INT Line = Node->GetLine();
	if ( !Node->HasBreakpoint() && Debugger->BreakpointManager->QueryBreakpoint(*Debugger->GetStackOwnerClass(Node->StackNode)->GetPathName(), Line) )
	{
		const_cast<FStackNode*>(Node)->SetBreakpoint();
		return 1;
	}

	return 0;
}

UBOOL DSWaitForCondition::EvaluateCondition(UBOOL bOptional)
{
	return FDebuggerState::EvaluateCondition(bOptional);
}

UBOOL DSRunToCursor::EvaluateCondition(UBOOL bOptional)
{
	return DSWaitForCondition::EvaluateCondition(bOptional);
}

UBOOL DSStepOut::EvaluateCondition(UBOOL bOptional)
{
	check(Debugger->CallStack->StackDepth);
	check(!Debugger->IsDebuggerClosing());

	const FStackNode* Node = GetCurrentNode();
	check(Node);

	if ( Debugger->CallStack->StackDepth >= EvalDepth )
		return FDebuggerState::EvaluateCondition(bOptional);

	// ?! Is this the desired result?
	// This seems like it could possibly result in the udebugger skipping a function while stepping out, if the
	// opcode was DI_PrevStack when 'stepout' was received
/*	if ( bOptional )
	{
		if ( !Debugger->CallStack->StackDepth < EvalDepth - 1 )
			return DSIdleState::EvaluateCondition(bOptional);
	}
	else*/ 
	if ( Debugger->CallStack->StackDepth < EvalDepth )
		return 1;

    return DSWaitForCondition::EvaluateCondition(bOptional);
}

UBOOL DSStepInto::EvaluateCondition(UBOOL bOptional)
{
	check(Debugger->CallStack->StackDepth);
	check(!Debugger->IsDebuggerClosing());
	const FStackNode* Node = GetCurrentNode();
	check(Node);

	return Debugger->CallStack->StackDepth != EvalDepth || Node->GetLine() != LineNumber;
}

UBOOL DSStepOverStack::EvaluateCondition(UBOOL bOptional)
{
	check(Debugger->CallStack->StackDepth);
	check(!Debugger->IsDebuggerClosing());
	const FStackNode* Node = GetCurrentNode();
	check(Node);

	if ( Debugger->CallStack->StackDepth != EvalDepth || Node->GetLine() != LineNumber )
	{
		if ( Debugger->CallStack->StackDepth < EvalDepth )
			return 1;
		if ( Debugger->CallStack->StackDepth == EvalDepth )
			return !bOptional;
		return FDebuggerState::EvaluateCondition(bOptional);
	}
	return 0;
}

UBOOL DSBreakOnChange::EvaluateCondition( UBOOL bOptional )
{
	// TODO
	//first, evaluate whether our data watch has changed...if so, set a flag to indicate that we've requested the udbegger to break
	// (will be checked in HandleInput), and break
	// otherwise, just execute normal behavior
	check(Watch);
	if ( Watch->Modified() )
	{
		bDataBreak = 1;
		return 1;
	}

	if ( SubState )
		return SubState->EvaluateCondition(bOptional);

	return DSWaitForCondition::EvaluateCondition(bOptional);
}

/*-----------------------------------------------------------------------------
	Primary UDebugger methods.
-----------------------------------------------------------------------------*/
void UDebuggerCore::ChangeState( FDebuggerState* NewState, UBOOL bImmediately )
{
	if( PendingState )
	{
		delete PendingState;
	}

	PendingState = NewState ? NewState : NULL;
	if ( bImmediately && PendingState )
	{
		SetAccessedNone(FALSE);
		SetBreakASAP(FALSE);

		PendingState->UpdateStackInfo(CurrentState ? CurrentState->GetCurrentNode() : NULL);
		ProcessPendingState();
		CurrentState->Process();
	}
}

void UDebuggerCore::ProcessPendingState()
{
	if ( PendingState )
	{
		if ( CurrentState )
		{
			if ( CurrentState->InterceptNewState(PendingState) )
			{
				PendingState = NULL;
				return;
			}

			if ( !PendingState->InterceptOldState(CurrentState) )
			{
				delete CurrentState;
			}
		}

		CurrentState = PendingState;
		PendingState = NULL;
	}
}

// Main debugger entry point
void UDebuggerCore::DebugInfo( const UObject* Debugee, const FFrame* CurrentFrame, BYTE OpCode, INT LineNumber, INT InputPos )
{
	if( !IsDebuggerProcessingEnabled() )
	{
		return;
	}

	// Weird Devastation fix
	if ( CurrentFrame->Node->IsA( UClass::StaticClass() ) )
	{
		return;
	}

	UClass* OwnerClass = GetStackOwnerClass(CurrentFrame);
	if ( OwnerClass != NULL )
	{
		UPackage* OwnerPackage = OwnerClass->GetOutermost();
		checkf((OwnerPackage->PackageFlags&PKG_ContainsDebugInfo) != 0, TEXT("Package '%s' is not compiled in debug mode"), *OwnerPackage->GetName());
	}

	// Process any waiting states
	ProcessPendingState();
	checkSlow(CurrentState);

	if ( CallStack && BreakpointManager && CurrentState )
	{
		if ( IsDebuggerClosing() )
		{
			if ( Interface->IsLoaded() )
			{
				Interface->Close();
			}
		}
		else if ( !GIsRequestingExit || GIsUCC )
		{
			// Returns true if it handled updating the stack
			if ( !CallStack->UpdateStack(Debugee, CurrentFrame, LineNumber, InputPos, OpCode) )
			{
				if ( CallStack->StackDepth > 0 )
				{
					CurrentState->UpdateStackInfo( CallStack->GetTopNode() );
					if ( IsDebuggerActive() && CurrentStackPosition != CallStack->Stack.Num() - 1 )
					{
						StackChanged(CallStack->GetTopNode());
					}

					// Halt execution and wait for user input if we have a pending forced breakpoint.
					if ( (HasReceivedAccessNone() && IsBreakOnNoneEnabled()) || IsPendingImmediateBreak() )
					{
						Break();
					}
					else
					{
						// Otherwise, update the debugger's state with the currently executing stacknode, then
						// pass control to debugger state for further processing (i.e. if stepping over, are we ready to break again)
						CurrentState->Process();
					}
				}
			}
		}
	}
}

void FCallStack::StackCorruptionDetected()
{
	if ( Parent != NULL )
	{
		// disable processing for the rest of the frame
		Parent->EnableDebuggerProcessing(FALSE);
		Parent->ChangeState(new DSIdleState(Parent), TRUE);
	}

	Empty();
}

/**
 * Update the debugger callstack with the latest script frame, adding new debugger stack nodes if necessary.
 * Take into account latent stack anomolies.
 * 
 * @param	Debugee				the UObject executing script
 * @param	CurrentFrame		the FFrame corresponding to the current script being executed
 * @param	LineNumber			the source code line number (.uc) for this script frame
 * @param	InputPos			the position in the .uc file for this script frame (used for tracking which
 *								expression is active of multiple expressions on a single line such as for loops)
 * @param	OpCode				the debugger opcode that was processed.  the value must match an value from the 
 *								EDebugInfo enum (@see OpCode.h)
 *
 * @return	TRUE if the UDebugger should not be allowed to break on this opcode.  Typically, this is for opcodes
 *			which are used to manage the udebugger's callstack, such as entering a new function or leaving a function.
 *			FALSE if the UDebugger should query the current state to determine whether it should break on this opcode.
 */
UBOOL FCallStack::UpdateStack( const UObject* Debugee, const FFrame* CurrentFrame, int LineNumber, int InputPos, BYTE OpCode )
{
	checkSlow(StackDepth == Stack.Num());

	if ( StackDepth == 0 )
	{
		QueuedCommands.Empty();
	}

	FDebuggerState* CurrentState = Parent->GetCurrentState();
	UClass* OwnerClass = Parent->GetStackOwnerClass(CurrentFrame);

	switch ( OpCode )
	{
	// PrevStackLatent is encountered when script calls a latent function.  Since the thread of execution will likely
	// change after this function is called, the current stack node is removed.  Since latent functions can only be called
	// from state code, we should only receive this opcode when we have only one node on the callstack (the one corresponding
	// to the state code)
	case DI_PrevStackLatent:
		{
			if ( StackDepth != 1 )
			{
				Parent->DumpStack();
#ifdef _DEBUG
				appErrorf(NAME_FriendlyError,TEXT("PrevStackLatent received with stack depth != 1 - Object:%s Class:%s Line:%i OpCode:%s"), *Debugee->GetName(), *OwnerClass->GetName(), LineNumber, GetOpCodeName(OpCode));
#else
				warnf(NAME_Warning, TEXT("PrevStackLatent received with stack depth != 1.  Verify that all packages have been compiled in debug mode."));
				StackCorruptionDetected();
				return TRUE;
#endif
			}

			// manually insert an additional debug bytecode here so that the interface stops on the line that calls the latent
			// function, instead of ending up in some random location (which would be whichever script will be executed for the
			// next actor to be ticked).....this additional bytecode can't be inserted by the compiler because only one debug bytecode
			// will be read from the stream when the P_FINISH is called for the latent function
			UpdateStack(Debugee,CurrentFrame,LineNumber,InputPos,DI_EFP);
			CurrentState->UpdateStackInfo( &Stack.Last() );

			// force the interface to process this action, before we move on and pop the node from the stack
			CurrentState->Process(FALSE);

			// now remove the stack node for this frame
			Stack.Pop();
			StackDepth--;

			// calling Process() may have changed the debugger's state
			CurrentState = Parent->GetCurrentState();
			CurrentState->UpdateStackInfo(NULL);
			return TRUE;
		}

	// Leaving a function, state, label, etc...pop the top node off the call stack
	case DI_PrevStack:
		{
// 			UBOOL bIsStateFrame = CurrentFrame->Node->IsA(UState::StaticClass());
// 			if ( bIsStateFrame && ((FStateFrame*)CurrentFrame)->LatentAction != 0 )
// 			{
// 				// if a latent function is currently executing in the state, we don't need to pop off the
// 				return TRUE;
// 			}
			if ( StackDepth <= 0 )
			{			
				Parent->DumpStack();
#ifdef _DEBUG
				appErrorf(NAME_FriendlyError, TEXT("PrevStack received with StackDepth <= 0.  Class:%s Line:%i Object:%s OpCode:%s"), *OwnerClass->GetName(), LineNumber, *Debugee->GetName(), GetOpCodeName(OpCode));
#else
				warnf(NAME_Warning, TEXT("PrevStack received with StackDepth <= 0.  Verify that all packages have been compiled in debug mode."));
				StackCorruptionDetected();
				return TRUE;
#endif
			}

			FStackNode* CurrentTop = &Stack.Last();
			if ( CurrentTop->StackNode != CurrentFrame )
			{
				if ( !CurrentTop->StackNode->Node->IsA(UState::StaticClass()) && CurrentFrame->Node->IsA(UState::StaticClass()) )
				{
					// We've received a call to execDebugInfo() from UObject::GotoState() as a result of the state change,
					// but we were executing a function, not state code, or this is a result of pushing/popping a state.
					// Queue this prevstack to be executed once the script callstack has unwound back to the state node
					if ( ((FStateFrame*)CurrentFrame)->LatentAction == 0 )
					{
						new(QueuedCommands) StackCommand( CurrentFrame, OpCode, LineNumber );
					}
					return TRUE;
				}

#ifdef _DEBUG
				Parent->DumpStack();
				appErrorf(NAME_FriendlyError, TEXT("UDebugger CallStack inconsistency detected.  Class:%s Line:%i Object:%s OpCode:%s"), *OwnerClass->GetName(), LineNumber, *Debugee->GetName(), GetOpCodeName(OpCode));
#else
				warnf(NAME_Warning, TEXT("UDebugger CallStack inconsistency detected.  Verify that all packages have been compiled in debug mode."));
				StackCorruptionDetected();
				return TRUE;
#endif
			}

			CurrentTop = NULL;
			Stack.Pop();
			StackDepth--;

			// If we're returning to state code (StackDepth == 1 && stack node is an FStateFrame), and the current object has been marked
			// to be deleted, we'll never receive the PREVSTACK (since state code isn't executed for actors marked bDeleteMe)
			// Remove this stacknode now, but don't change the current state of the debugger (in case we were stepping into, or out of)
			if ( StackDepth == 1 )
			{
				const FFrame* ScriptFrame = Stack(0).StackNode;
				if ( ScriptFrame && ScriptFrame->Node && ScriptFrame->Node->IsA(UState::StaticClass()) && ScriptFrame->Object->IsPendingKill() )
				{
					Stack.Pop();
					StackDepth--;

					CurrentState->UpdateStackInfo(NULL);
					return TRUE;
				}
			}

			if ( StackDepth == 0 )
			{
				CurrentState->UpdateStackInfo(NULL);
			}

			else
			{
				CurrentTop = &Stack.Last();
				CurrentState->UpdateStackInfo( CurrentTop );
				CurrentState->Process(TRUE);
			}

			// prevent re-entrancy
			static bool bProcessQueue = true;

			// If we're returning to state code and we have a queued command for this state, execute it now
			if ( bProcessQueue && QueuedCommands.Num() )
			{
				// if we don't have any stack nodes in the debugger callstack, clear all QueuedCommands that wanted to remove a node
				if ( CurrentTop == NULL )
				{
					while ( QueuedCommands.Num() && QueuedCommands(0).OpCode == DI_PrevStack )
					{
						QueuedCommands.Remove(0);
					}

					if ( QueuedCommands.Num() == 0 )
					{
						return TRUE;
					}
				}

				// only process QueuedCommands once we're back to the correct point in the callstack (where the queued command's frame
				// is the same as the frame for the stack node currently at the top of the callstack)
				StackCommand FirstCommand = QueuedCommands(0);
				if ( CurrentTop == NULL || FirstCommand.Frame == CurrentTop->StackNode )
				{
					// the first QueuedCommand should now either point to a command which will create a new stack node,
					// or a command that will remove the stack node currently on top
					bProcessQueue = false;

					INT NumCommandsToExecute = QueuedCommands.Num();
					while ( NumCommandsToExecute-- > 0 )
					{
						// make sure this is a copy, since we're about to remove the StackCommand from the TArray
						StackCommand Command = QueuedCommands(0);
						QueuedCommands.Remove(0);

						INT CommandLineNumber = Command.LineNumber;
						if ( CommandLineNumber == 0 && CurrentTop != NULL )
						{
							CommandLineNumber = CurrentTop->GetLine();
						}

						Parent->ProcessPendingState();
						UpdateStack( Debugee, Command.Frame, CommandLineNumber, InputPos, Command.OpCode );
						CurrentTop = StackDepth ? &Stack.Last() : NULL;
					}

					bProcessQueue = true;
				}
			}

			return TRUE;
		}
		
	// PrevStackState indicates that we're leaving a state manually (i.e. not as the result of processing a bytecode), so
	// it isn't guaranteed that the debugger stack node corresponding to the CurrentFrame is the one on top...
	case DI_PrevStackState:
		{
			if ( CurrentFrame->Node->IsA(UState::StaticClass()) )
			{
				FStackNode& CurrentTop = Stack.Last();
				if ( !CurrentTop.StackNode->Node->IsA(UState::StaticClass()) )
				{
					UpdateStack(Debugee, CurrentFrame, CurrentTop.GetLine(), CurrentTop.GetPos(), DI_PrevStack);
				}

				else
				{
					UpdateStack( Debugee, CurrentTop.StackNode, CurrentTop.GetLine() + 1, CurrentTop.GetPos(), DI_PrevStack );
				}
				return TRUE;
			}

			goto Default;
		}

	// NewStackState is only called when returning to a previous state via PopState().  We need to
	// recreate the debugger stack node for this state node.
	case DI_NewStackState:
		{
			if ( CurrentFrame->Node->IsA(UState::StaticClass()) )
			{
				if ( StackDepth )
				{
					FStackNode& CurrentTop = Stack.Last();
					if ( !CurrentTop.StackNode->Node->IsA(UState::StaticClass()) )
					{
						// We're changing states, but we were executing a function, not state code.
						// Queue this new stack to be pushed once the script callstack has unwound from this function's node
						new(QueuedCommands) StackCommand( CurrentFrame, DI_NewStack, LineNumber );
						return TRUE;
					}
				}

				UpdateStack( Debugee, CurrentFrame, LineNumber, InputPos, DI_NewStack );
				return TRUE;
			}

			goto Default;
		}

	// Entering a new function, state, etc. as a result of a normal bytecode.
	case DI_NewStack:
		{
			FStackNode* CurrentTop = NULL;
			if (StackDepth)
			{
				CurrentTop = &Stack.Last();
			}

			if ( CurrentTop && CurrentTop->StackNode == CurrentFrame )
			{
				Parent->DumpStack();
#if _DEBUG
				appErrorf(NAME_FriendlyError,TEXT("Received duplicate NEWSTACK frame - Object:%s Class:%s Line:%i OpCode:%s"), *Debugee->GetName(), *OwnerClass->GetName(), LineNumber, GetOpCodeName(OpCode));
#else
				warnf(NAME_Warning,TEXT("Received duplicate NEWSTACK frame.  Verify that all packages have been compiled in debug mode."));
				StackCorruptionDetected();
				return TRUE;
#endif
			}

			CurrentTop = new(Stack) FStackNode( Debugee, CurrentFrame, OwnerClass, StackDepth, LineNumber, InputPos, OpCode );
			CurrentState->UpdateStackInfo( CurrentTop );
			StackDepth++;

			CurrentState->Process();

			return TRUE;
		}

	// NewStackLatent is encountered when a latent function has returned.   We need to recreate the debugger stack node for the state frame and push it
	// onto the stack.  Since latent functions can only be called from state code, we should only receive this opcode when we the callstack is empty.
	case DI_NewStackLatent:
		{
			if ( StackDepth )
			{
				Parent->DumpStack();
#ifdef _DEBUG
				appErrorf(NAME_FriendlyError,TEXT("Received LATENTNEWSTACK with stack depth - Object:%s Class:%s Line:%i OpCode:%s"), *Debugee->GetName(), *OwnerClass->GetName(), LineNumber, GetOpCodeName(OpCode));
#else
				warnf(NAME_Warning,TEXT("Received LATENTNEWSTACK with stack depth.  Verify that all packages have been compiled in debug mode."));
				StackCorruptionDetected();
				return TRUE;
#endif
			}

			FStackNode* CurrentTop = new(Stack) FStackNode(Debugee, CurrentFrame, OwnerClass, StackDepth, LineNumber,InputPos,OpCode);
			CurrentState->UpdateStackInfo( CurrentTop );
			StackDepth++;

			CurrentState->Process();
			return TRUE;
		}

	// Entering a new label, which may or may not require a new debugger stack node to be created.
	case DI_NewStackLabel:
		{
			if ( StackDepth == 0 )
			{
				// was result of a native gotostate
				FStackNode* CurrentTop = new(Stack) FStackNode(Debugee, CurrentFrame, OwnerClass, StackDepth, LineNumber,InputPos,OpCode);
				CurrentState->UpdateStackInfo( CurrentTop );
				StackDepth++;

				CurrentState->Process();
				return TRUE;
			}

			else
			{
				FStackNode* CurrentTop = &Stack.Last();
				CurrentTop->Update( LineNumber, InputPos, OpCode, StackDepth );
				return FALSE;
			}
		}
	}

	// Stack has not changed. Update the current node with line number and current opcode type
Default:
	if ( StackDepth <= 0 )
	{
		Parent->DumpStack();
#ifdef _DEBUG
		appErrorf(NAME_FriendlyError,TEXT("Received call to UpdateStack with CallStack depth of 0.  Class:%s Line:%i Object:%s OpCode:%s"), *OwnerClass->GetName(), LineNumber, *Debugee->GetName(), GetOpCodeName(OpCode));
#else
		warnf(NAME_Warning,TEXT("Received call to UpdateStack with CallStack depth of 0.  Verify that all packages have been compiled in debug mode."));
		StackCorruptionDetected();
#endif
		return TRUE;
	}

	FStackNode* CurrentTop = &Stack.Last();
	if ( CurrentTop->StackNode != CurrentFrame )
	{
		if ( !CurrentTop->StackNode->Node->IsA(UState::StaticClass()) && CurrentFrame->Node->IsA(UState::StaticClass()) )
		{
			// We've received a call to execDebugInfo() from UObject::GotoState() as a result of the state change,
			// but we were executing a function, not state code.
			// Back up the state's pointer to the EX_DebugInfo, and ignore this update.
			FFrame* HijackStack = const_cast<FFrame*>(CurrentFrame);
			while ( --HijackStack->Code && *HijackStack->Code != EX_DebugInfo );
			return TRUE;
		}

		Parent->DumpStack();
#ifdef _DEBUG
		debugf(TEXT("Debuggers says the current node is - Class:%s Line:%i Object:%s OpCode:%s"), *Parent->GetStackOwnerClass(CurrentTop->StackNode)->GetName(), CurrentTop->GetLine(), *CurrentTop->GetObject()->GetName(), CurrentTop->GetInfo());
		appErrorf(NAME_FriendlyError,TEXT("Received call to UpdateStack with stack out of sync - Class:%s Line:%i Object:%s OpCode:%s"), *OwnerClass->GetName(), LineNumber, *Debugee->GetName(), GetOpCodeName(OpCode));
#else
		warnf(NAME_Warning,TEXT("Received call to UpdateStack with stack out of sync.  Verify that all packages have been compiled in debug mode."));
		StackCorruptionDetected();
		return TRUE;
#endif
	}

	CurrentTop->Update( LineNumber, InputPos, OpCode, StackDepth );

	// Skip over FORINIT opcodes to simplify stepping into/over
	return OpCode == DI_ForInit;
}

void UDebuggerCore::Break()
{
#ifdef _DEBUG
	if ( GetCurrentNode() )
        GetCurrentNode()->Show();
#endif

	ChangeState( new DSWaitForInput(this), TRUE );

	if ( GEngine != NULL && GEngine->GameViewport != NULL && GEngine->GameViewport->Viewport != NULL )
	{
		GEngine->GameViewport->Viewport->CaptureMouseInput( FALSE );
	}
}

void UDebuggerCore::DumpStack()
{
	check(CallStack);
	debugf(TEXT("CALLSTACK DUMP - STACKDEPTH: %i"), CallStack->StackDepth);

	for ( INT i = 0; i < CallStack->Stack.Num(); i++ )
	{
		FStackNode* Node = &CallStack->Stack(i);
		if ( !Node )
		{
			debugf(TEXT("%i)  INVALID NODE"), i);
		}
		else
		{
			debugf(TEXT("%i) Class '%s'  Object '%s'  Node '%s'"),
				i, 
				Node->Class		? *Node->Class->GetName()				: TEXT("NULL"),
				Node->Object	? *Node->Object->GetFullName()			: TEXT("NULL"),
				Node->StackNode && Node->StackNode->Node
								? *Node->StackNode->Node->GetFullName() : TEXT("NULL") );

			for ( INT j = 0; j < Node->Lines.Num() && j < Node->OpCodes.Num(); j++ )
			{
				debugf(TEXT("   %i): Line '%i'  OpCode '%s'  Depth  '%i'"), j, Node->Lines(j), GetOpCodeName(Node->OpCodes(j)), Node->Depths(j));
			}
		}
	}
}

static const TCHAR* GetBoolString( UBOOL bValue )
{
	return bValue ? GTrue : GFalse;
}

/**
 * Returns a string giving the current state of the UDebugger; used for debugging purposes.
 */
FString UDebuggerCore::Describe() const
{
	FString Result = FString::Printf(
		TEXT("bIsDebugging: %s\tbProcessDebugInfo: %s\tbBreakASAP: %s\tCurrentState: %s\tPendingState: %s"),
		GetBoolString(bIsDebugging),
		GetBoolString(bProcessDebugInfo),
		GetBoolString(bBreakASAP),
		CurrentState ? *CurrentState->Describe() : TEXT("NULL"),
		PendingState ? *PendingState->Describe() : TEXT("NULL")
		);
	return Result;
}



