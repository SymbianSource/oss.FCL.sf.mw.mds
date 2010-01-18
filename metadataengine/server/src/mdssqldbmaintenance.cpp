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
* Description:  SQL database startup routines*
*/

// INCLUDE FILES
#include "mdssqldbmaintenance.h"
#include "mdccommon.h"
#include "mdspreferences.h"

// ========================= MEMBER FUNCTIONS ==================================

CMdSSqlDbMaintenance* CMdSSqlDbMaintenance::NewL()
    {
    CMdSSqlDbMaintenance* self = CMdSSqlDbMaintenance::NewLC();
    CleanupStack::Pop( self );
    return self;
    }

CMdSSqlDbMaintenance* CMdSSqlDbMaintenance::NewLC()
    {
    CMdSSqlDbMaintenance* self = new ( ELeave ) CMdSSqlDbMaintenance();
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

void CMdSSqlDbMaintenance::ConstructL( )
    {
    }

CMdSSqlDbMaintenance::CMdSSqlDbMaintenance()
    {
    }

CMdSSqlDbMaintenance::~CMdSSqlDbMaintenance()
    {
    }

TBool CMdSSqlDbMaintenance::ValidateL(  )
    {
    _LIT( KValidateTableExistence, "SELECT COUNT(*) FROM MdE_Preferences;" );

    RMdsStatement validationQuery;
    CleanupClosePushL( validationQuery );
    RRowData emptyRowData;
    CleanupClosePushL( emptyRowData );
    CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();
	TRAPD( test, connection.ExecuteQueryL( KValidateTableExistence, validationQuery, emptyRowData ) );
	CleanupStack::PopAndDestroy( 2, &validationQuery );
    return ( test == KErrNone );
    }


void CMdSSqlDbMaintenance::CreateDatabaseL()
    {
    _LIT( KCreateTblMdE_Preferences, // Table for metadata engine use
    	"CREATE TABLE MdE_Preferences(Key TEXT,Value NONE,ExtraValue LARGEINT,UNIQUE(Key,Value));");

    _LIT( KCreateTblMdS_Medias, // Table for metadata engine use
		"CREATE TABLE MdS_Medias(MediaId INTEGER PRIMARY KEY,Drive INTEGER,PresentState INTEGER,Time LARGEINT);");
    
    RRowData emptyRowData;
    CleanupClosePushL( emptyRowData );

    // Create ontology tables
    CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();

    connection.ExecuteL( KCreateTblMdE_Preferences, emptyRowData );

    connection.ExecuteL( KCreateTblMdS_Medias, emptyRowData );

    TInt majorVersion = KMdSServMajorVersionNumber;
	TInt64 minorVersion = KMdSServMinorVersionNumber;
    MMdsPreferences::InsertL( KMdsDBVersionName, MMdsPreferences::EPreferenceBothSet,
    						  majorVersion, minorVersion );

	CleanupStack::PopAndDestroy( &emptyRowData );
    }

