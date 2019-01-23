/**
 * if( ENUM == var )
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0013_IfEnumEqualEqualVarDerivedClass extends Test0013_IfEnumEqualEqualVar
   dependson( Test0013_IfEnumEqualEqualVar );



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
`if(`notdefined(FINAL_RELEASE))
    local SomeEnumFoo AnEnum;

    AnEnum = SEF_ValOne;

    //if( SomeEnumFoo.SEF_ValOne == AnEnum ) // does not compile
    //{
    //   `log( "" );
    //}

      `log( "AnEnum: " $ AnEnum );
`endif
}
