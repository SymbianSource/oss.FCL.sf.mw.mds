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
#include "blacklistclient.h"
#include "blacklistcommon.h"
#include "mdcserializationbuffer.h"
#include "blacklistitem.h"
#include "harvesterlog.h"


// ---------------------------------------------------------------------------
// RBlacklistClient::RBlacklistClient()
// ---------------------------------------------------------------------------
//
EXPORT_C RBlacklistClient::RBlacklistClient() : RSessionBase (),
    iSessionOk( EFalse )
    {
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::~RBlacklistClient()
// ---------------------------------------------------------------------------
//
EXPORT_C RBlacklistClient::~RBlacklistClient()
    {
    WRITELOG( "CBlacklistServer::~RBlacklistClient - begin" );
    
    RSessionBase::Close(); 
    iBlacklistMemoryTable.ResetAndDestroy();
    iBlacklistMemoryTable.Close();
	iBlacklistChunk.Close();

    WRITELOG( "CBlacklistServer::~RBlacklistClient - end" );
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::Connect()
// ---------------------------------------------------------------------------
//
EXPORT_C TInt RBlacklistClient::Connect()
    {
    WRITELOG( "CBlacklistServer::Connect - begin" );

    TInt retryCount = 2;
    TInt error = KErrNone;
    
    iSessionOk = EFalse;
    
    while ( retryCount )
        {
        // try create session, if ok, then break out and return KErrNone
        error = CreateSession( KBlacklistServerName, Version() );
        if( error != KErrNotFound && error != KErrServerTerminated )
            {
            iSessionOk = ETrue;
            break;
            }
        
        // Cannot create session, start server
        error = StartServer();
        
        if ( error != KErrNone && error != KErrAlreadyExists )
            {
            break;
            }
       
        --retryCount;
        }
    
    WRITELOG( "CBlacklistServer::Connect - end" );

    return error;
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::Version()
// ---------------------------------------------------------------------------
//
TVersion RBlacklistClient::Version() const
    {
    WRITELOG( "CBlacklistServer::Version - begin" );

    return TVersion( KBlacklistServerMajorVersion, KBlacklistServerMinorVersion,
    	KBlacklistServerBuildVersion );

    }

// ---------------------------------------------------------------------------
// RBlacklistClient::StartServer()
// ---------------------------------------------------------------------------
//
TInt RBlacklistClient::StartServer()
    {
    WRITELOG( "CBlacklistServer::CustomSecurityCheckL - begin" );

    const TUidType serverUid = ( KNullUid, KNullUid, KUidKBlacklistServer );

    RProcess server;
    TInt error = server.Create( KBlacklistServerExe, KNullDesC );
    if( error != KErrNone )
        {
        return error;
        }
    
    // start server and wait for signal before proceeding    
    TRequestStatus status;
    server.Rendezvous( status );
    if ( status.Int() != KRequestPending )
        {
        server.Kill( 0 );
        }
    else
        {
        server.Resume();
        }

    User::WaitForRequest( status );
    error = server.ExitType() == EExitPanic ? KErrGeneral : status.Int();
    server.Close();

    WRITELOG( "CBlacklistServer::Version - end" );

    return error;
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::RemoveFromDBL()
// ---------------------------------------------------------------------------
//
void RBlacklistClient::RemoveFromDBL( const TDesC& aUri, TUint32 aMediaId ) const
    {
    WRITELOG( "CBlacklistServer::RemoveFromDBL - begin" );

    TPckgBuf<TUint32> mediaIdPckg( aMediaId );
    
    TIpcArgs ipcArgs;
    ipcArgs.Set( 1, &aUri );
    ipcArgs.Set( 2, &mediaIdPckg );
 
    const TInt err = SendReceive( EBlacklistRemoveFromDB, ipcArgs );
    User::LeaveIfError( err );

    WRITELOG( "CBlacklistServer::RemoveFromDBL - end" );
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::DoLoadBlacklistL()
// ---------------------------------------------------------------------------
//
void RBlacklistClient::DoLoadBlacklistL( TInt& aHandle ) const
    {
    WRITELOG( "CBlacklistServer::DoLoadBlacklistL - begin" );

    TPckgBuf<TInt> handleBuf;
    TIpcArgs ipcArgs;
    ipcArgs.Set( 1, &handleBuf );
    const TInt err = SendReceive( EGetBlacklistData, ipcArgs );
    User::LeaveIfError( err );
    aHandle = handleBuf();

    WRITELOG( "CBlacklistServer::DoLoadBlacklistL - end" );
    } 

// ---------------------------------------------------------------------------
// RBlacklistClient::LoadBlacklistL()
// ---------------------------------------------------------------------------
//
EXPORT_C void RBlacklistClient::LoadBlacklistL()
    {
    WRITELOG( "CBlacklistServer::LoadBlacklistL - begin" );

    if ( !iSessionOk )
        {
        return;
        }

    // delete old data
    if( iBlacklistMemoryTable.Count() > 0 )
        {
        iBlacklistMemoryTable.ResetAndDestroy();
        }
    
    // Get handle to data
    TInt handle( 0 );
    DoLoadBlacklistL( handle );
    
   	// create memory chunk
   	HBufC*  name = HBufC::NewLC( 32 );
	*name = KBlacklistChunkName;
	name->Des().AppendNum( handle );
	iBlacklistChunk.Close();

	User::LeaveIfError ( iBlacklistChunk.OpenGlobal( *name, ETrue ) );

	CMdCSerializationBuffer* buffer = CMdCSerializationBuffer::NewLC( iBlacklistChunk.Base(), iBlacklistChunk.Size() );

	if ( buffer->Size() == 0 )
	    {
	    User::Leave( KErrNotFound );
	    }
	
	// First get list count
	TUint32 listCount ( 0 );
	buffer->ReceiveL( listCount );
	
	TInt64 modified ( 0 );
	TUint32 mediaId ( 0 );
    HBufC* uri = NULL;
    
    for( TInt i( 0 ); i < listCount; i++ )
        {
        // get modified and media id
	    buffer->ReceiveL( modified );
	    buffer->ReceiveL( mediaId );
        
        //Get uri
        uri = buffer->ReceiveDes16L();
       	CleanupStack::PushL( uri );
       	AddToMemoryTableL( modified, *uri, mediaId );
       	CleanupStack::PopAndDestroy( uri );
       	uri = NULL;
        }
	
	CleanupStack::PopAndDestroy( buffer );
	CleanupStack::PopAndDestroy( name );

    WRITELOG( "CBlacklistServer::LoadBlacklistL - end" );
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::AddToMemoryTableL()
// ---------------------------------------------------------------------------
//
void RBlacklistClient::AddToMemoryTableL( const TInt64& aModified,
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
// RBlacklistClient::RemoveFromMemoryTableL()
// ---------------------------------------------------------------------------
//
void RBlacklistClient::RemoveFromMemoryTableL( const TDesC& aUri, const TUint32 aMediaId )
    {
    WRITELOG( "CBlacklistServer::RemoveFromMemoryTableL - begin" );

    const TInt index = GetListIndex( aUri, aMediaId );
    if ( index >= 0 )
        {
        CBlacklistItem* item = iBlacklistMemoryTable[index];
        delete item;
        iBlacklistMemoryTable.Remove( index );
        }

    WRITELOG( "CBlacklistServer::RemoveFromMemoryTableL - end" );
    }
 

// ---------------------------------------------------------------------------
// RBlacklistClient::IsBlacklistedL()
// ---------------------------------------------------------------------------
//
EXPORT_C TBool RBlacklistClient::IsBlacklistedL( const TDesC& aUri, TUint32 aMediaId, TTime aLastModifiedTime )
    {
    WRITELOG( "CBlacklistServer::IsBlacklistedL - begin" );

    const TInt index = GetListIndex( aUri, aMediaId );
    if ( index >= 0 )
        {
        TInt64 modified( 0 );
        modified = iBlacklistMemoryTable[index]->Modified();
        
        if( modified > 0 )
            {
            if ( modified == aLastModifiedTime.Int64() )
                {
                WRITELOG( "CBlacklistServer::IsBlacklistedL - file is blacklisted, modification time is different" );
                return ETrue;
                }
            else
                {
                // file might be different, so remove from blacklist
                // and act like it wasn't found
                RemoveFromMemoryTableL( aUri, aMediaId );
                
                // Remove from server DB
                RemoveFromDBL( aUri, aMediaId );
                }
            }
        else
            {
            WRITELOG( "CBlacklistServer::IsBlacklistedL - file is blacklisted, no modification time found" );
            return ETrue;
            }
        
        }
   
    WRITELOG( "CBlacklistServer::IsBlacklistedL - end" );
    return EFalse;
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::GetListIndex()
// ---------------------------------------------------------------------------
//
TInt RBlacklistClient::GetListIndex( const TDesC& aUri, TUint32 aMediaId )
    {
    WRITELOG( "CBlacklistServer::GetListIndex - begin" );

    for ( TInt i( 0 ); i < iBlacklistMemoryTable.Count(); ++i )
        {
        if ( iBlacklistMemoryTable[i]->Compare( aUri, aMediaId ) )
            {
            return i;
            }
        }

    WRITELOG( "CBlacklistServer::GetListIndex - end" );

    return KErrNotFound;
    }


// ---------------------------------------------------------------------------
// RBlacklistClient::AddL()
// ---------------------------------------------------------------------------
//
EXPORT_C void RBlacklistClient::AddL( const TDesC& aUri, TUint32 aMediaId, TTime aLastModifiedTime ) const
    {
    WRITELOG( "CBlacklistServer::AddL - begin" );

    TPckgC<TUint32> mediaIdPckg( aMediaId );
    TPckgC<TTime> lastModifiedTimePckg( aLastModifiedTime );

	TIpcArgs ipcArgs;
    ipcArgs.Set( 0, &mediaIdPckg );
    ipcArgs.Set( 1, &aUri );
    ipcArgs.Set( 2, &lastModifiedTimePckg );
 
    const TInt err = SendReceive( EBlacklistAdd, ipcArgs );
    User::LeaveIfError( err );

    WRITELOG( "CBlacklistServer::AddL - end" );
    }


// ---------------------------------------------------------------------------
// RBlacklistClient::RemoveL()
// ---------------------------------------------------------------------------
//
EXPORT_C void RBlacklistClient::RemoveL( const TDesC& aUri, TUint32 aMediaId ) const
    {
    WRITELOG( "CBlacklistServer::RemoveL - begin" );

    TPckgBuf<TUint32> mediaIdPckg( aMediaId );
    
    TIpcArgs ipcArgs;
    ipcArgs.Set( 1, &aUri );
    ipcArgs.Set( 2, &mediaIdPckg );
 
    const TInt err = SendReceive( EBlacklistRemove, ipcArgs );
    User::LeaveIfError( err );

    WRITELOG( "CBlacklistServer::RemoveL - end" );
    }

// ---------------------------------------------------------------------------
// RBlacklistClient::CloseDBL()
// ---------------------------------------------------------------------------
//
EXPORT_C void RBlacklistClient::CloseDBL()
    {
    WRITELOG( "CBlacklistServer::CloseDBL - begin" );

    if ( !iSessionOk )
        {
        User::Leave( KErrDisconnected );
        }
    else
        {
        Send( EBlacklistCloseDB );
        }

    WRITELOG( "CBlacklistServer::CloseDBL - end" );
    }


// End of File
