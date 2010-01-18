/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  This is Metadata engine server file*
*/

// INCLUDE FILES
#include <e32std.h>
#include "mdsindexer.h"  
#include "mdsdbconnectionpool.h"
#include "mdssqliteconnection.h"

// ========================= MEMBER FUNCTIONS ==================================


TItemId MMdSIndexer::StartIndexL()
	{
	GetLastItemIdL();
	return iId;
	}

TItemId MMdSIndexer::GetIndex( )
	{
	return ++iId;
	}

void MMdSIndexer::RevertIndex( )
	{
	--iId;
	}

void MMdSIndexer::GetLastItemIdL()
	{
	_LIT( KGetLastMaxId, "SELECT max(seq) FROM SQLITE_SEQUENCE WHERE name!='symbian_security';" );
	CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();

	RRowData getData;
	CleanupClosePushL(getData);

	RMdsStatement selectObject;
	CleanupClosePushL(selectObject);

	//Get one row and set first column to id
	connection.ExecuteQueryL( KGetLastMaxId, selectObject, getData );

	getData.AppendL( TColumn( iId ) );
	if (connection.NextRowL(selectObject, getData))
		{
		getData.Column(0).Get( iId );
		}

	CleanupStack::PopAndDestroy(2, &getData);
	}

TItemId MMdSIndexer::GetIndexL()
	{
	_LIT( KIndexerQuery, "SELECT last_insert_rowid();" );

	CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();

	RRowData getData;
	CleanupClosePushL(getData);

	RMdsStatement selectObject;
	CleanupClosePushL(selectObject);

	TItemId id = KNoId;
	//Get one row and set first column to id
	connection.ExecuteQueryL(KIndexerQuery,selectObject,getData);
	getData.AppendL(TColumn(id));
	if (connection.NextRowL(selectObject, getData))
		{
		getData.Column(0).Get(id);
		}

	CleanupStack::PopAndDestroy(2, &getData);
	return id;
	}

TItemId MMdSIndexer::ExecuteAndGetIndexL( const TDesC &aSqlClause,
	    								 const RRowData& aRowData )
    {
    CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();

    const TInt queryResult = connection.ExecuteL( aSqlClause, aRowData );

    if (queryResult == 0)
    	{
    	return KNoId;
    	}

   	return MMdSIndexer::GetIndexL();
    }


RSQLIndex::RSQLIndex()
	: iId(MMdSIndexer::GetIndex()), iCommit(EFalse)
	{
	}

const TItemId& RSQLIndex::GetId() const
	{
	return iId;
	}

void RSQLIndex::Commit()
	{
	iCommit = ETrue;
	}

void RSQLIndex::Close()
	{
	if (!iCommit)
		{
		MMdSIndexer::RevertIndex();
		}
	}


// ---------------------------------------------------------------
// -----------------INITIALIZE STATIC DATA------------------------
// ---------------------------------------------------------------

TItemId MMdSIndexer::iId = KNoId;
