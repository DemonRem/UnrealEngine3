/**
 *
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0015_FullyQualifiedEnumSwitchCaseDerivedClass extends Test0015_FullyQualifiedEnumSwitchCase
    dependson( Test0015_FullyQualifiedEnumSwitchCase );


function CaseStatementWithFullQualifiedEnum()
{
   local SomeEnumFoo AnEnum;

   AnEnum = SEF_ValOne;

   switch( AnEnum )
   {
	case SEF_ValOne:  // Error, Bad or missing expression in 'Case'
        break;


//	case SomeEnum.SEF_ValTwo:  // Error, Bad or missing expression in 'Case'
        break;
   };


}
