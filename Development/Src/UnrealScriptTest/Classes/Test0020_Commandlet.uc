/**
 *  To run this do the following:  run UnrealScriptTest.Test0020_Commandlet
 *
 * Part of the unrealscript execution and compilation regression framework.
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class Test0020_Commandlet extends TestCommandletBase;
`include(Core/Globals.uci)

event int Main( string Parms )
{
	PerformStructSerializationTest();

	return 0;
}


final function PerformStructSerializationTest()
{
	local Test0020_StructDefaults TestObj;

	TestObj = new(self) class'Test0020_StructDefaults';

	TestObj.LogPropertyValues();
}

DefaultProperties
{
	LogToConsole=True
}
