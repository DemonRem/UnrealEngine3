/**
 *
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved
 **/

class Test0014_FullyQualifiedEnumFunctionDerivedClass extends Test0014_FullyQualifiedEnumFunction
   dependson( Test0014_FullyQualifiedEnumFunction );



function FullyQualifiedEnum( SomeEnumFoo Val )
{
}


function Caller()
{
   FullyQualifiedEnum( SEF_ValOne );
   FullyQualifiedEnum( SomeEnumFoo.SEF_ValOne );
}


