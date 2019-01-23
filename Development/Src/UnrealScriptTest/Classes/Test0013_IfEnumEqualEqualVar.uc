/**
 * if( ENUM == var )
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0013_IfEnumEqualEqualVar extends Object;


enum SomeEnumFoo
{
	SEF_ValOne,
	SEF_ValTwo
};


function EnumEqualEqualVar()
{
`if(`notdefined(FINAL_RELEASE))
    local SomeEnumFoo AnEnum;

    AnEnum = SEF_ValOne;

 //   if( SEF_ValOne == AnEnum )  // does not compile
 //   {
 //      `log( "" );
 //   }

      `log( "AnEnum: " $ AnEnum );
`endif
}



function EnumTypeEnumEqualEqualVar()
{
    local SomeEnumFoo AnEnum;

    AnEnum = SEF_ValOne;

    if( SomeEnumFoo.SEF_ValOne == AnEnum )
    {
       `log( "" );
    }

      `log( "AnEnum: " $ AnEnum );
}
