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
* Description:
*
*/
// USER INCLUDE
#include "blacklistserver.h"
#include "blacklistcommon.h"
#include "blacklistsession.h"
#include "blacklistitem.h"
#include "harvesterlog.h"
#include "mdssqliteconnection.h"
#include "mdcserializationbuffer.h"

#include <pathinfo.h>

// Security policy
const TUint KServerPolicyRangeCount = 6;
// Database file without the drive letter
_LIT( KBlacklistDatabaseFile, ":[2001B2EB]blacklistdb.sq" );

const TInt KServerPolicyRanges[KServerPolicyRangeCount] =
    {
    EBlacklistAdd,
    EBlacklistRemove,
    EBlacklistRemoveFromDB,
    EGetBlacklistData,
    EBlacklistCloseDB,
    EMaxService    
    };

const TUint8 KServerPolicyElementsIndex[KServerPolicyRangeCount] =
    {
    CPolicyServer::ECustomCheck, // EBlacklistAdd   
    CPolicyServer::ECustomCheck, // EBlacklistRemove
    CPolicyServer::ECustomCheck, // EBlacklistRemoveFromDB   
    CPolicyServer::ECustomCheck, // EGetBlacklistData   
    CPolicyServer::ECustomCheck, // EBlacklistCloseDB
    CPolicyServer::ECustomCheck  // EMaxService  
    };

const CPolicyServer::TPolicy KServerPolicy =
    {
    CPolicyServer::EAlwaysPass, // specifies all connect attempts should pass
    KServerPolicyRangeCount,                   
    KServerPolicyRanges,
    KServerPolicyElementsIndex,
    NULL
    };

// ---------------------------------------------------------------------------
// CBlacklistServer::CustomSecurityCheckL()
// ---------------------------------------------------------------------------
//
CPolicyServer::TCustomResult CBlacklistServer::CustomSecurityCheckL(
        const RMessage2& aMsg, TInt& /*aAction*/, TSecurityInfo& /*aMissing*/ )
	{
    WRITELOG( "CBlacklistServer::CustomSecurityCheckL - begin" );
    
    CPolicyServer::TCustomResult securityCheckResult = EFail;
    
    switch ( aMsg.Function() )
    	{
    	case EBlacklistAdd:
    	case EBlacklistRemove:
    	case EBlacklistRemoveFromDB:
    	case EGetBlacklistData:
    	case EBlacklistCloseDB:
    	case EMaxService:
		    {
            if( aMsg.HasCapability( ECapabilityWriteDeviceData ) )
                {
                securityCheckResult = EPass;
                }
    		}
            break;
        default:
            {
            securityCheckResult = EFail;
            }
   	    }
    
    WRITELOG( "CBlacklistServer::CustomSecurityCheckL - end" );

    return securityCheckResult;
	}


// ---------------------------------------------------------------------------
// CBlacklistServer::CustomFailureActionL()
// ---------------------------------------------------------------------------
//
CPolicyServer::TCustomResult CBlacklistServer::CustomFailureActionL(
        const RMessage2& /*aMsg*/, TInt /*aAction*/, const TSecurityInfo& /*aMissing*/ )
	{
    WRITELOG( "CBlacklistServer::CustomFailureActionL" );

    // Not used
    return EFail;
	}

// ---------------------------------------------------------------------------
// CBlacklistServer::NewL()
// ---------------------------------------------------------------------------
//
CBlacklistServer* CBlacklistServer::NewL()
    {
    WRITELOG( "CBlacklistServer::NewL - begin" );

    CBlacklistServer* self = NewLC();
    CleanupStack::Pop( self );

    WRITELOG( "CBlacklistServer::NewL - end" );
    return self;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::NewLC()
// ---------------------------------------------------------------------------
//
CBlacklistServer* CBlacklistServer::NewLC()
    {
    WRITELOG( "CBlacklistServer::NewLC - begin" );

    CBlacklistServer* self = new( ELeave ) CBlacklistServer();
    CleanupStack::PushL( self );
    self->ConstructL();

    WRITELOG( "CBlacklistServer::NewLC - end" );
    return self;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::CBlacklistServer()
// ---------------------------------------------------------------------------
//
CBlacklistServer::CBlacklistServer() : 
    CPolicyServer( CActive::EPriorityStandard, KServerPolicy, ESharableSessions )
    {
    }

void CBlacklistServer::ConstructL()
    {
    WRITELOG( "CBlacklistServer::ConstructL - begin" );

    StartL( KBlacklistServerName );

    iSqLiteConnection = CMdSSqLiteConnection::NewL();
    
    WRITELOG( "CBlacklistServer::ConstructL - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::~CBlacklistServer()
// ---------------------------------------------------------------------------
//
CBlacklistServer::~CBlacklistServer()
    {
    WRITELOG( "CBlacklistServer::~CBlacklistServer - begin" );

    if ( iDatabaseOpen )
        {
        if( iSqLiteConnection )
            {
            iSqLiteConnection->CloseDb();
            }
        }
 
    iBlacklistMemoryTable.ResetAndDestroy();
    iBlacklistMemoryTable.Close();

    for( TInt i = iBufferedRemoveItems.Count() - 1; i >=0; i-- )
    	{
    	iBufferedRemoveItems[i]->Close();
    	}

    iBufferedRemoveItems.ResetAndDestroy();
    iBufferedRemoveItems.Close();  	

    for( TInt i = iBufferedAddedItems.Count() - 1; i >=0; i-- )
    	{
    	iBufferedAddedItems[i]->Close();
    	}

   	iBufferedAddedItems.ResetAndDestroy();
    iBufferedAddedItems.Close();  	
   	
   	iBlacklistServerChunk.Close();
 
    delete iSqLiteConnection;

    WRITELOG( "CBlacklistServer::~CBlacklistServer - end" );
    }

// ---------------------------------------------------------------------------
// Closes database connection.
// ---------------------------------------------------------------------------
//
void CBlacklistServer::CloseDB()
    {
    WRITELOG( "CBlacklistServer::CloseDB - begin" );

    if ( iDatabaseOpen )
        {
        iSqLiteConnection->CloseDb();
        iDatabaseOpen = EFalse;
        }

    WRITELOG( "CBlacklistServer::CloseDB - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::ExeMainL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::ExeMainL()
    {
    WRITELOG( "CBlacklistServer::ExeMainL - begin" );
    User::LeaveIfError( User::RenameThread(KBlacklistServerName) );
    // Construct active scheduler
    CActiveScheduler* activeScheduler = new ( ELeave ) CActiveScheduler;
    CleanupStack::PushL( activeScheduler );

    // Install active scheduler
    // We don't need to check whether an active scheduler is already installed
    // as this is a new thread, so there won't be one
    CActiveScheduler::Install( activeScheduler );

    // Construct our server
    CBlacklistServer::NewLC();    // Anonymous

    RProcess::Rendezvous( KErrNone );

    // Start handling requests
    CActiveScheduler::Start();

    CleanupStack::PopAndDestroy( 2, activeScheduler );  
    WRITELOG( "CBlacklistServer::ExeMainL - end" );
    }

// ---------------------------------------------------------------------------
// TInt E32Main()
// ---------------------------------------------------------------------------
//
TInt E32Main()
    {
    WRITELOG( "CBlacklistServer::E32Main - begin" );   
    __UHEAP_MARK;
    CTrapCleanup* cleanup=CTrapCleanup::New();
    TInt result = KErrNoMemory;
    if (cleanup)
        {
        TRAP(result, CBlacklistServer::ExeMainL());
        delete cleanup;
        }
    __UHEAP_MARKEND;
    WRITELOG( "CBlacklistServer::E32Main - end" );
    return result;
    }


// ---------------------------------------------------------------------------
//  CBlacklistServer::OpenDatabaseL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::OpenDatabaseL()
    {
    WRITELOG( "CBlacklistServer::OpenDatabaseL - begin" );
    
    if ( iDatabaseOpen )
        {
        return; 
        }
    HBufC* databasePath = HBufC::NewLC( KMaxFileName );
    TPtr pathPtr = databasePath->Des();
    pathPtr.Append( PathInfo::PhoneMemoryRootPath()[0] );
    pathPtr.Append( KBlacklistDatabaseFile );
    iSqLiteConnection->OpenDbL( pathPtr );
    iDatabaseOpen = ETrue; 
    CleanupStack::PopAndDestroy( databasePath );

    if ( !TableExistsL() )
        {
        CreateBlacklistTableL();
        }
    
    // Add or remove operations when DB connection was off  
    if ( iDBUpdateNeeded )
        {
        CommitBufferedItemsL();        
        }
    
    LoadDatabaseToMemoryL();
    
    // serialize it for client
    SerializeToSharedMemoryL();

    WRITELOG( "CBlacklistServer::OpenDatabaseL - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::SerializeToSharedMemoryL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::SerializeToSharedMemoryL()
	{
    WRITELOG( "CBlacklistServer::SerializeToSharedMemoryL - begin" );

	TUint32 bufferSize = 0;
	TUint32 blacklistMemoryTableCount = iBlacklistMemoryTable.Count();

    // First add size for item count
    bufferSize += CMdCSerializationBuffer::KRequiredSizeForTUint32;
	
    // Add size for items
	for ( TInt i = 0; i < blacklistMemoryTableCount; ++i )
		{
		bufferSize += iBlacklistMemoryTable[i]->RequiredBufferSize();
		}
	
	// create chunk
	iBlacklistServerChunk.Close();
	iHandle++;
	HBufC* name = HBufC::NewLC( KBlacklistChunkName.iTypeLength + KMaxUintValueLength );
   	*name = KBlacklistChunkName;
	name->Des().AppendNum( iHandle );
	User::LeaveIfError( iBlacklistServerChunk.CreateGlobal( *name, bufferSize, bufferSize ) );
	
	CMdCSerializationBuffer* buffer = CMdCSerializationBuffer::NewLC( iBlacklistServerChunk.Base(), bufferSize );
	
	// first inser table count
	buffer->InsertL( blacklistMemoryTableCount );

	// then items
	
	for ( TInt i = 0; i < blacklistMemoryTableCount; ++i )
	    {
   	    // add modified and media id field
   	    buffer->InsertL( iBlacklistMemoryTable[i]->Modified() );
   	    buffer->InsertL( iBlacklistMemoryTable[i]->MediaId() );
   	    
		// add uri
       	const TDesC& uri =  *iBlacklistMemoryTable[i]->Uri();
	    HBufC* lcUri = HBufC::NewLC( uri.Length() );
		lcUri->Des().Append( uri );
		buffer->InsertL( *lcUri );
		CleanupStack::PopAndDestroy( lcUri ); 
        lcUri = NULL;
        }
	
	CleanupStack::PopAndDestroy( 2, name );
    
	// No need to keep these items anymore
	iBlacklistMemoryTable.ResetAndDestroy();

    WRITELOG( "CBlacklistServer::SerializeToSharedMemoryL - end" );
	}

// ---------------------------------------------------------------------------
// CBlacklistServer::Handle() 
// ---------------------------------------------------------------------------
//
TInt CBlacklistServer::MemoryHandle()
    {
    WRITELOG( "CBlacklistServer::MemoryHandle" );
    return iHandle;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::LoadDatabaseToMemoryL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::LoadDatabaseToMemoryL()
    {
    WRITELOG( "CBlacklistServer::LoadDatabaseToMemoryL - begin" );

    iBlacklistMemoryTable.ResetAndDestroy();
    RMdsStatement statement;
    CleanupClosePushL( statement );
    RRowData emptyRowData;
    CleanupClosePushL( emptyRowData );

    iSqLiteConnection->ExecuteQueryL( KMdsBlacklistCache, statement, emptyRowData );

    TPtrC ptr = TPtrC( (TUint16*)0, 0 );
    TInt64 modified = 0;
    TUint32 mediaId = 0;
    RRowData result;
    CleanupClosePushL( result );
    result.AppendL( TColumn( ptr ) );
    result.AppendL( TColumn( mediaId ) );
    result.AppendL( TColumn( modified ) );
    while ( iSqLiteConnection->NextRowL( statement, result ) )
        {
        result.Column( 0 ).Get( ptr );      
        result.Column( 1 ).Get( mediaId );
        result.Column( 2 ).Get( modified );
        AddToMemoryTableL( modified, ptr, mediaId );
        result.Column( 0 ).Free();
        }
    
    CleanupStack::PopAndDestroy( &result );        
    CleanupStack::PopAndDestroy( &emptyRowData ); 
    CleanupStack::PopAndDestroy( &statement ); 

    WRITELOG( "CBlacklistServer::LoadDatabaseToMemoryL - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::CreateBlacklistTableL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::CreateBlacklistTableL()
    {
    WRITELOG( "CBlacklistServer::CreateBlacklistTableL - begin" );

    RRowData emptyRowData;
    CleanupClosePushL( emptyRowData );
    iSqLiteConnection->ExecuteL( KDropBlacklistTable, emptyRowData );
    iSqLiteConnection->ExecuteL( KCreateBlacklistTable, emptyRowData );
	CleanupStack::PopAndDestroy( &emptyRowData );

    WRITELOG( "CBlacklistServer::CreateBlacklistTableL - end" );
    }


// ---------------------------------------------------------------------------
// CBlacklistServer::AddToMemoryTableL
// ---------------------------------------------------------------------------
//
void CBlacklistServer::AddToMemoryTableL( const TInt64& aModified,
        const TDesC& aUri, const TUint32 aMediaId )
    {

    WRITELOG( "CBlacklistServer::AddToMemoryTableL - begin" );

    CBlacklistItem* item = CBlacklistItem::NewL( aModified, aUri, aMediaId );
    const TInt err = iBlacklistMemoryTable.Append( item ); // ownership is transferred
    if ( err != KErrNone )
        {
        delete item;
        }

    WRITELOG( "CBlacklistServer::AddToMemoryTableL - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::RemoveFromMemoryTable()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::RemoveFromMemoryTable( const TDesC& aUri,
        const TUint32 aMediaId )
    {

    WRITELOG( "CBlacklistServer::RemoveFromMemoryTable - begin" );

    const TInt index = GetMemoryTableIndex( aUri, aMediaId );
    if ( index >= 0 )
        {
        CBlacklistItem * item = iBlacklistMemoryTable[index];
        delete item;
        iBlacklistMemoryTable.Remove( index );
        }

    WRITELOG( "CBlacklistServer::RemoveFromMemoryTable - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::GetMemoryTableIndex()
// ---------------------------------------------------------------------------
//
TInt CBlacklistServer::GetMemoryTableIndex( const TDesC& aUri, TUint32 aMediaId )
    {
    WRITELOG( "CBlacklistServer::GetMemoryTableIndex - begin" );

    for ( TInt i( 0 ); i < iBlacklistMemoryTable.Count(); ++i )
        {
        if ( iBlacklistMemoryTable[i]->Compare( aUri, aMediaId ) )
            {
            return i;
            }
        }

    WRITELOG( "CBlacklistServer::GetMemoryTableIndex - end" );
    return KErrNotFound;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::TableExistsL()
// ---------------------------------------------------------------------------
//
TBool CBlacklistServer::TableExistsL()
    {
    WRITELOG( "CBlacklistServer::TableExistsL - begin" );

    RMdsStatement validationQuery;
    CleanupClosePushL( validationQuery );
    RRowData emptyRowData;
    CleanupClosePushL( emptyRowData );
    TRAPD( err, iSqLiteConnection->ExecuteQueryL(
    		KMdsBlacklistCheckTableExistence, validationQuery, emptyRowData ) );
	validationQuery.Close();
	CleanupStack::PopAndDestroy( &emptyRowData ); // emptyRowData
	CleanupStack::PopAndDestroy( &validationQuery ); // validationQuery

    WRITELOG( "CBlacklistServer::TableExistsL - end" );

    return ( err == KErrNone );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::NewSessionL()
// ---------------------------------------------------------------------------
//
CSession2* CBlacklistServer::NewSessionL( const TVersion& aVersion, const RMessage2& /*aMessage*/ ) const
    {
    WRITELOG( "CBlacklistServer::NewSessionL - begin" );

    // Check we are the right version
    if ( !User::QueryVersionSupported( TVersion( KBlacklistServerMajorVersion,
                                                 KBlacklistServerMinorVersion,
                                                 KBlacklistServerBuildVersion ),
                                       aVersion ) )
    	{
        User::Leave( KErrNotSupported );
    	}
    
    WRITELOG( "CBlacklistServer::NewSessionL - end" );

    return new( ELeave ) CBlacklistSession;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::RunError()
// ---------------------------------------------------------------------------
//
TInt CBlacklistServer::RunError( TInt aError )
    {
    WRITELOG1( "CBlacklistServer::RunError - begin, error %d", aError );

    // Bad descriptor implies bad client
    if ( aError == KErrBadDescriptor )
        {
        Message().Panic( KBlacklistServerName, aError );
        }
    else
        {
        Message().Complete( aError );
        }

    // Continue handling requests
    ReStart();

    WRITELOG( "CBlacklistServer::RunError - end" );

    return KErrNone;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::AddSession()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::AddSession()
    {
    WRITELOG( "CBlacklistServer::AddSession" );
    ++iSessionCount;
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::RemoveSession()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::RemoveSession()
    {
    WRITELOG( "CBlacklistServer::RemoveSession - begin" );

    --iSessionCount;
    
#ifdef _DEBUG
    TRAPD ( err, WriteAllDataToDBL() );
#else
    TRAP_IGNORE ( WriteAllDataToDBL() );
#endif
    WRITELOG1( "CBlacklistServer::RemoveSession - err %d", err  );
    
    if ( iSessionCount == 0 )
        {
        CActiveScheduler::Stop();
        }

    WRITELOG( "CBlacklistServer::RemoveSession - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::AddL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::AddL( const TUint32& mediaId, const TDesC& aUri, const TTime& aLastModifiedTime ) 
    {
    WRITELOG( "CBlacklistServer::AddL - begin" );

    AddToMemoryTableL( aLastModifiedTime.Int64(), aUri, mediaId );

    WRITELOG( "CBlacklistServer::AddL - end" );
    } 

// ---------------------------------------------------------------------------
// CBlacklistServer::RemoveL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::RemoveL( const TUint32& mediaId, const TDesC& aUri ) 
    {
    WRITELOG( "CBlacklistServer::RemoveL - begin" );

    RemoveFromMemoryTable( aUri, mediaId );

    WRITELOG( "CBlacklistServer::RemoveL - end" );
    } 
 
// ---------------------------------------------------------------------------
// CBlacklistServer::RemoveFromDBL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::RemoveFromDBL( const TDesC& aUri, const TUint32 aMediaId )
    {
    WRITELOG( "CBlacklistServer::RemoveFromDBL - begin" );
    
    RRowData* variables = new ( ELeave ) RRowData();        
    CleanupStack::PushL( variables );
    CleanupClosePushL( *variables );
 
    if ( !iDatabaseOpen )
        {
        // Add it to buffer, delete from database when DB connection opens
        
        HBufC* lcBuf = aUri.AllocLC();
    
        variables->AppendL( TColumn( lcBuf ) );     
        variables->AppendL( TColumn( aMediaId ) );    
 
        iBufferedRemoveItems.Append( variables ); // transfer ownership
        iDBUpdateNeeded = ETrue;
        CleanupStack::Pop( lcBuf );
        CleanupStack::Pop( 2, variables );
        }
    else
        {
        // Database open, commit to DB
        
        variables->AppendL( TColumn( aUri ) );     
        variables->AppendL( TColumn( aMediaId ) );    
        
        iSqLiteConnection->ExecuteL( KMdsBlacklistDelete, *variables );
        CleanupStack::PopAndDestroy( 2, variables );
        }
    
    WRITELOG( "CBlacklistServer::RemoveFromDBL - end" );
    }


// ---------------------------------------------------------------------------
// CBlacklistServer::AddDBL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::AddToDBL( const TDesC& aUri, const TUint32 aMediaId,
    const TInt64& aLastModifiedTime ) 
    {
    WRITELOG( "CBlacklistServer::AddToDBL - begin" );
  
    RRowData* rowData = new ( ELeave ) RRowData();        
    CleanupStack::PushL( rowData );
    CleanupClosePushL( *rowData );
    
    if ( !iDatabaseOpen )
        {
        // Add it to buffer, add to database when DB connection opens
        HBufC* lcBuf = aUri.AllocLC();

        rowData->AppendL( TColumn( aLastModifiedTime ) );
        rowData->AppendL( TColumn( lcBuf ) );     
        rowData->AppendL( TColumn( aMediaId ) );    

        iBufferedAddedItems.Append( rowData ); // transfer ownership
        iDBUpdateNeeded = ETrue;
        CleanupStack::Pop( lcBuf );
        CleanupStack::Pop( 2, rowData );
        }
    else
        {
        rowData->AppendL( TColumn( aLastModifiedTime ) );
        rowData->AppendL( TColumn( aUri ) );     
        rowData->AppendL( TColumn( aMediaId ) );    
       
        TRAPD( err, iSqLiteConnection->ExecuteL( KMdsBlacklistInsert, *rowData ) );
        if ( err == KSqlErrConstraint ) // is the file already blacklisted...
            {
            iSqLiteConnection->ExecuteL( KMdsBlacklistUpdate, *rowData );
            }
        CleanupStack::PopAndDestroy( 2, rowData );
        }

    WRITELOG( "CBlacklistServer::AddToDBL - end" );
    } 

// ---------------------------------------------------------------------------
// CBlacklistServer::WriteAllDataToDBL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::WriteAllDataToDBL()
    {
    WRITELOG( "CBlacklistServer::WriteAllDataToDBL - begin" );

    for ( TInt i( 0 ); i < iBlacklistMemoryTable.Count(); ++i )
        {
        AddToDBL(  *iBlacklistMemoryTable[i]->Uri(),  iBlacklistMemoryTable[i]->MediaId(), iBlacklistMemoryTable[i]->Modified() );
        }

    WRITELOG( "CBlacklistServer::WriteAllDataToDBL - end" );
    }

// ---------------------------------------------------------------------------
// CBlacklistServer::WriteAllDataToDBL()
// ---------------------------------------------------------------------------
//
void CBlacklistServer::CommitBufferedItemsL()
    {
    WRITELOG( "CBlacklistServer::CommitBufferedItemsL - begin" );
   
    // First, removed items
    const TInt removedCount( iBufferedRemoveItems.Count() );
    for ( TInt i( 0 ); i < removedCount; ++i )
        {
        iSqLiteConnection->ExecuteL( KMdsBlacklistDelete, *iBufferedRemoveItems[i] );
        iBufferedRemoveItems[i]->Column( 0 ).Free();      
        }
   
    // Second, added items
    const TInt addedCount( iBufferedAddedItems.Count() );
    for ( TInt i( 0 ); i < addedCount; ++i )
        {
        iSqLiteConnection->ExecuteL( KMdsBlacklistUpdate, *iBufferedAddedItems[i] );
        iBufferedAddedItems[i]->Column( 0 ).Free();  
        }

    WRITELOG( "CBlacklistServer::CommitBufferedItemsL - end" );
    }

// End of File
