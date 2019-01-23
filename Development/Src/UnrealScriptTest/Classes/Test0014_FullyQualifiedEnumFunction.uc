/**
 *
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0014_FullyQualifiedEnumFunction extends Object;


enum SomeEnumFoo
{
  SEF_ValOne,
  SEF_ValTwo
};



function FullyQualifiedEnum( SomeEnumFoo Val )
{
}


function Caller()
{
   FullyQualifiedEnum( SEF_ValOne );
   FullyQualifiedEnum( SomeEnumFoo.SEF_ValOne );
}


