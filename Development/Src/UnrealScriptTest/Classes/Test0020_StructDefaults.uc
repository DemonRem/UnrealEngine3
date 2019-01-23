/**
 * Performs various tests for struct defaults
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/
class Test0020_StructDefaults extends TestClassBase
	editinlinenew;
`include(Core/Globals.uci)

struct TestNestedStruct
{
	var()	int		NestedStructInt;

structdefaultproperties
{
	NestedStructInt=100
}

};

struct TestStruct
{
	var() int TestInt;
	var() float TestFloat;
	var() TestNestedStruct TestMemberStruct;
	var() TestNestedStruct TestMemberStruct2;

structdefaultproperties
{
	TestInt=8
	TestFloat=2.0f
	TestMemberStruct2=(NestedStructInt=300)
}
};

`define IncludeMemberStructs RemoveThisWordToDisable

`if(`IncludeMemberStructs)
var() TestStruct	StructVar_NoDefaults;
var() TestStruct	StructVar_ClassDefaults;
`endif

var() int			ClassIntVar;
var() string		ClassStringVar;

final function LogPropertyValues( optional TestStruct ParameterStruct )
{
`if(`notdefined(FINAL_RELEASE))
	local TestStruct LocalTestStruct;

	`log(`showvar(ClassIntVar));
	`log(`showvar(ClassStringVar));

	`log(`showvar(LocalTestStruct.TestInt));
	`log(`showvar(LocalTestStruct.TestFloat));
	`log(`showvar(LocalTestStruct.TestMemberStruct.NestedStructInt));
	`log(`showvar(LocalTestStruct.TestMemberStruct2.NestedStructInt));

	`log(`showvar(ParameterStruct.TestInt));
	`log(`showvar(ParameterStruct.TestFloat));
	`log(`showvar(ParameterStruct.TestMemberStruct.NestedStructInt));
	`log(`showvar(ParameterStruct.TestMemberStruct2.NestedStructInt));

`if(`IncludeMemberStructs)
	`log(`showvar(StructVar_NoDefaults.TestInt));
	`log(`showvar(StructVar_NoDefaults.TestFloat));
	`log(`showvar(StructVar_NoDefaults.TestMemberStruct.NestedStructInt));
	`log(`showvar(StructVar_NoDefaults.TestMemberStruct2.NestedStructInt));

	`log(`showvar(StructVar_ClassDefaults.TestInt));
	`log(`showvar(StructVar_ClassDefaults.TestFloat));
	`log(`showvar(StructVar_ClassDefaults.TestMemberStruct.NestedStructInt));
	`log(`showvar(StructVar_ClassDefaults.TestMemberStruct2.NestedStructInt));
`endif
`endif
}

DefaultProperties
{
	ClassIntVar=16
	ClassStringVar="ClassStringDefault"
`if(`IncludeMemberStructs)
	StructVar_ClassDefaults=(TestInt=16,TestFloat=10.f)
`endif
}
