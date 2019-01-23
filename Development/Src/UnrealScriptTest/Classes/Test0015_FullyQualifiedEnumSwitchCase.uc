/**
 *
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0015_FullyQualifiedEnumSwitchCase extends Object;


enum SomeEnumFoo
{
  SEF_ValOne,
  SEF_ValTwo
};


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
