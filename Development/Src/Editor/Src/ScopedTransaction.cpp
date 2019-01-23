/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "ScopedTransaction.h"

FScopedTransaction::FScopedTransaction(const TCHAR* SessionName)
{
	Index = GEditor->Trans->Begin( SessionName );
	check( IsOutstanding() );
}

FScopedTransaction::~FScopedTransaction()
{
	if ( IsOutstanding() )
	{
		GEditor->Trans->End();
	}
}

/**
 * Cancels the transaction.  Reentrant.
 */
void FScopedTransaction::Cancel()
{
	if ( IsOutstanding() )
	{
		GEditor->Trans->Cancel( Index );
		Index = -1;
	}
}

/**
 * @return	TRUE if the transaction is still outstanding (that is, has not been cancelled).
 */
UBOOL FScopedTransaction::IsOutstanding() const
{
	return Index >= 0;
}
