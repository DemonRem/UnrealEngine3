/**
 * Part of the unrealscript execution and compilation regression framework.
 * General script compilation and VM regression.
 *
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class Test0016_PropertyAssignment extends Object;
`include(Core/Globals.uci)

var string TestGlobalString;

var property TestPropertyAssignment;

DefaultProperties
{
	TestPropertyAssignment=TestGlobalString
}
