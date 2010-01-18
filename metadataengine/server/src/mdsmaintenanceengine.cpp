/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Class for database import/export and maintenance
*
*/

// INCLUDE FILES
#include "mdsmaintenanceengine.h"
#include "mdslogger.h"
#include "mdsmanipulationengine.h"
#include "mdcserializationbuffer.h"
#include "mdsimportexport.h"
#include "mdssqliteconnection.h"
#include "mdsschema.h"
#include "mdssqldbmaintenance.h"
#include "mdsdbconnectionpool.h"
#include "mdsindexer.h"
#include "mdspreferences.h"

__USES_LOGGER

// ------------------------------------------------
// NewL
// ------------------------------------------------
//
CMdSMaintenanceEngine* CMdSMaintenanceEngine::NewL()
    {
    CMdSMaintenanceEngine* self = CMdSMaintenanceEngine::NewLC();
    CleanupStack::Pop( self );
    return self;
    }

// ------------------------------------------------
// NewLC
// ------------------------------------------------
//
CMdSMaintenanceEngine* CMdSMaintenanceEngine::NewLC()
    {
    CMdSMaintenanceEngine* self = new ( ELeave ) CMdSMaintenanceEngine();
    CleanupStack::PushL( self );
    self->ConstructL( );
    return self;
    }

// ------------------------------------------------
// Constructor
// ------------------------------------------------
//
CMdSMaintenanceEngine::CMdSMaintenanceEngine()
    {
    }

// ------------------------------------------------
// NewL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::ConstructL()
    {
    iMaintenance = CMdSSqlDbMaintenance::NewL();
    }

// ------------------------------------------------
// Destructor
// ------------------------------------------------
//
CMdSMaintenanceEngine::~CMdSMaintenanceEngine()
    {
    delete iMaintenance;
    }

// ------------------------------------------------
// InitConnectionL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::InitConnectionL()
    {
    // open the database
    MMdSDbConnectionPool::ConnectAllL();
    }

// ------------------------------------------------
// CloseDatabaseL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::CloseDatabase()
	{
	MMdSDbConnectionPool::DisconnectAll();
	}

// ------------------------------------------------
// DeleteDatabaseL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::DeleteDatabase()
	{
	CMdSMaintenanceEngine::CloseDatabase();    
	CMdSSqLiteConnection::DeleteDb( );
	}

// ------------------------------------------------
// InstallL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::InstallL( CMdSManipulationEngine& aManipulate, CMdsSchema& aSchema )
    {
    if ( !(iMaintenance->ValidateL( ) ) )
        {
        // first-time init: re-form the database completely
    	iMaintenance->CreateDatabaseL( );
		const TUint KMdSServerUid = 0x0320e65f; // temporal uid

		// try to read schema file from C drive
		TRAPD( err, ImportSchemaL( aSchema, KSchemaImportFile, KMdSServerUid) );

		if( err != KErrNone )
			{
			__LOG1( ELogAlways, "Schema reading error: %d", err );
			// if schema file is not found, try to read from rom (Z) drive
			if ( err == KErrNotFound || err == KErrPathNotFound )
				{
				TRAP( err, ImportSchemaL( aSchema, KSchemaRomImportFile, KMdSServerUid) );
				}
			if( err != KErrNone )
				{
				__LOG1( ELogAlways, "Schema reading error: %d", err );
				DeleteDatabase();
				User::Leave( err );
				}
			}

		if ( FailedImports() != 0 )
  			{
	       	User::Leave( KErrCorrupt );
       		}
		
		// try to read default import file from C drive
       	TRAP( err, ImportMetadataL( aManipulate, aSchema, KMdsDefaultImportFile ) );
       	if ( err == KErrNotFound || err == KErrPathNotFound )
       		{
       		// if default import file is not found, try to read from rom (Z) drive
       		// and ignore errors
       		TRAP_IGNORE( ImportMetadataL( aManipulate, aSchema, KMdsDefaultRomImportFile ) );
       		}
       	
       	StoreCDriveMediaIdL();
        }
    else
        {
        TRAPD( err, LoadSchemaL( aSchema ) );

        if( err != KErrNone )
        	{ 
			DeleteDatabase();
			User::Leave( err );
        	}
        }
    }

// ------------------------------------------------
// LoadSchemaL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::LoadSchemaL( CMdsSchema& aSchema )
    {
    CMdsImportExport* impSchema = CMdsImportExport::NewLC();
    impSchema->ImportSchemaFromDBL( aSchema );
    CleanupStack::PopAndDestroy( impSchema );
    aSchema.SerializeToSharedMemoryL();
    }
    
// ------------------------------------------------
// ImportSchemaL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::ImportSchemaL( CMdsSchema& aSchema, const TDesC16& aFileName, TUint32 aVendorId )
    {
    CMdsImportExport* impSchema = CMdsImportExport::NewLC();
    impSchema->ImportSchemaFromFileL( aFileName, aSchema, aVendorId );
    CleanupStack::PopAndDestroy( impSchema );

    aSchema.StoreToDBL();

	aSchema.SerializeToSharedMemoryL();
    }

// ------------------------------------------------
// ImportMetadataL
// ------------------------------------------------
//
TInt CMdSMaintenanceEngine::ImportMetadataL( CMdSManipulationEngine& aManipulate, CMdsSchema& aSchema, const TDesC16& aFileName )
    {
    CMdsImportExport* importMetadata = CMdsImportExport::NewLC();
    TInt failed = KErrNone;
    TRAPD( err, failed = importMetadata->ImportMetadataL( aManipulate.Manipulate(), aSchema, aFileName ) );
    if (err != KErrNone)
    	{
    	failed = err;
    	}
    CleanupStack::PopAndDestroy( importMetadata );
    
   	iFailedImports = failed;
    return failed;
    }
    
// ------------------------------------------------
// ExportMetadataL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::ExportMetadataL( CMdsSchema& aSchema, const TDesC16& aFileName, 
                                             CMdCSerializationBuffer& aItems )
	{
	CMdsImportExport* exporter = CMdsImportExport::NewLC();
	exporter->ExportMetadataL( aSchema, aFileName, aItems );
	CleanupStack::PopAndDestroy( exporter );
    }

// ------------------------------------------------
// StoreCDriveMediaIdL
// ------------------------------------------------
//
void CMdSMaintenanceEngine::StoreCDriveMediaIdL()
	{
	RFs fs;
    User::LeaveIfError( fs.Connect() );
    CleanupClosePushL( fs );
    TVolumeInfo volumeInfo;
    fs.Volume( volumeInfo, EDriveC );
    MMdsPreferences::InsertL( KCMediaIdKey, MMdsPreferences::EPreferenceValueSet,
    		(TUint32) volumeInfo.iUniqueID );
    
    CleanupStack::PopAndDestroy( &fs );
	}

