/**
 * if( var == ENUM )
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0012_IfVarEqualEqualEnum extends Object;


enum SomeEnumFoo
{
	SEF_ValOne,
	SEF_ValTwo
};



function VarEqualEqualEnum()
{
    local SomeEnumFoo AnEnum;

    AnEnum = SEF_ValOne;

    if( AnEnum == SEF_ValOne )  // compiles
    {
       `log( "" );
    }

}

function VarEqualEqualEnumTypeEnum()
{
    local SomeEnumFoo AnEnum;

    AnEnum = SEF_ValOne;

    if( AnEnum == SomeEnumFoo.SEF_ValOne )  // compiles
    {
       `log( "" );
    }

}
