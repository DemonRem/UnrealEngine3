/**
 *  To run this do the following:  run UnrealScriptTest.Test0011_Commandlet
 *
 * Part of the unrealscript execution and compilation regression framework.
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 */

class Test0011_Commandlet extends TestCommandletBase;
`include(Core/Globals.uci)

event int Main( string Parms )
{
	PerformBoolOrderTest();

	return 0;
}


final function PerformBoolOrderTest()
{
	local Test0011_NativeObjectBoolOrder TestObj;

	TestObj = new(self) class'Test0011_NativeObjectBoolOrder';

	TestObj.PerformBoolOrderTest();
}

DefaultProperties
{
	LogToConsole=true
}
