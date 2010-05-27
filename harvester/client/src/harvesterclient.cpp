/*
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Harvester Client implementation*
*/

#include <e32property.h> 

#include "harvesterclient.h"
#include "harvestercommon.h"
#include "harvesterrequestqueue.h"
#include "harvestereventobserverao.h"
#include "harvesterlog.h"
#include "harvesterclientao.h"
#include "harvestersessionwatcher.h"
#include "mdsutils.h"
#include "harvesterrequestactive.h"
#include "mdscommoninternal.h"

/** @var Message slots */
const TInt KDefaultMessageSlots = -1;  // Global pool

/* Server name */
_LIT( KHarvesterServerName, "HarvesterServer" );

/* Harvester Server process location */
_LIT( KHarvesterServerExe, "harvesterserver.exe" );

// FUNCTION PROTOTYPES
static TInt StartServer();
static TInt CreateServerProcess();


// ----------------------------------------------------------------------------------------
// RHarvesterClient
// ----------------------------------------------------------------------------------------
//
EXPORT_C RHarvesterClient::RHarvesterClient() : RSessionBase() 
    {
    WRITELOG( "RHarvesterClient::RHarvesterClient() - Constructor" );
    iHarvesterClientAO = NULL;
    iObserver = NULL;
    iHEO = NULL;
    iRequestQueue = NULL;
    }

// ----------------------------------------------------------------------------------------
// Connect
// ----------------------------------------------------------------------------------------
//
EXPORT_C TInt RHarvesterClient::Connect()
    {
    WRITELOG( "RHarvesterClient::Connect()" );

    RProperty property;
    const TInt error( property.Attach( KHarvesterPSShutdown, KShutdown, EOwnerThread ) );
    TInt value = 0;
    property.Get( value );
    property.Close();

    if ( error != KErrNone || value > KErrNone )
        {
        return KErrLocked;
        }
    
    if( iHarvesterClientAO )
    	{
    	return KErrAlreadyExists;
    	}
    
    TRAPD( err, iHarvesterClientAO = CHarvesterClientAO::NewL(*this) );
    if ( err != KErrNone )
        {
        WRITELOG( "RHarvesterClient::RHarvesterClient() - Couldn't create active object" );
        return err;
        }
    
    // request processor
    TRAP( err, iRequestQueue = CHarvesterRequestQueue::NewL() )
        {
        if ( err != KErrNone )
            {
            WRITELOG( "RHarvesterClient::RHarvesterClient() - Couldn't create harvester request queue" );
            delete iHarvesterClientAO;
            iHarvesterClientAO = NULL;
            return err;
            }
        }
    
    err = ::StartServer();

    if ( err == KErrNone || err == KErrAlreadyExists )
        {
        WRITELOG( "RHarvesterClient::Connect() - creating session" );
        err = CreateSession( KHarvesterServerName, Version(), KDefaultMessageSlots );
        }
    else
        {
        delete iHarvesterClientAO;
        iHarvesterClientAO = NULL;
        delete iRequestQueue;
        iRequestQueue = NULL;
        }

#ifdef _DEBUG
    if ( err != KErrNone )
        {
        WRITELOG1( "RHarvesterClient::Connect() - Server is not running or could not be started, error &d", err );
        }
    else
        {
        WRITELOG( "RHarvesterClient::Connect() - no errors" );
        }
    WRITELOG( "RHarvesterClient::Connect() - end" );
#endif

    iHEO = NULL;
    
    iSessionWatcher = NULL;
    
    return err;
    }

// ----------------------------------------------------------------------------------------
// Pause
// ----------------------------------------------------------------------------------------
//
EXPORT_C TInt RHarvesterClient::Pause()
    {
    WRITELOG( "RHarvesterClient::Pause() -  sending command EPauseHarvester" );
    if( iHandle )
    	{
    	return SendReceive( EPauseHarvester );
       	}
    return KErrDisconnected;
    }

// ----------------------------------------------------------------------------------------
// Resume
// ----------------------------------------------------------------------------------------
//
EXPORT_C TInt RHarvesterClient::Resume()
    {
    WRITELOG( "RHarvesterClient::Resume() -  sending command EResumeHarvester" );
    if( iHandle )
    	{
    	return SendReceive( EResumeHarvester );
    	}
    return KErrDisconnected;
    }

// ----------------------------------------------------------------------------------------
// Close
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::Close()
    {
    WRITELOG( "RHarvesterClient::Close()" );
    
    delete iSessionWatcher;
    iSessionWatcher = NULL;
    
    // cancels Harvest Complete request if it exist at server
    UnregisterHarvestComplete();
    
    WRITELOG( "RHarvesterClient::Close() - UnregisterHarvest done" );
    
    if( iRequestQueue && iRequestQueue->RequestsPending() )
        {
        iRequestQueue->Cancel();
        iRequestQueue->ForceRequests();
        }
    
    delete iRequestQueue;
    iRequestQueue = NULL;
    
    delete iHarvesterClientAO;
    iHarvesterClientAO = NULL;
    
    delete iHEO;
    iHEO = NULL;
    
    WRITELOG( "RHarvesterClient::Close() - Closing session" );
    
    RSessionBase::Close();
    }

// ----------------------------------------------------------------------------------------
// SetObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::SetObserver( MHarvestObserver* aObserver )
    {
    WRITELOG( "RHarvesterClient::SetObserver()" );

    if ( iHarvesterClientAO )
        {
        iHarvesterClientAO->SetObserver( aObserver );
        }
	iObserver = aObserver;
    }

// ----------------------------------------------------------------------------------------
// RemoveObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::RemoveObserver( MHarvestObserver* aObserver )
    {
    WRITELOG( "RHarvesterClient::RemoveObserver()" );
    
    if ( iHarvesterClientAO )
        {
        iHarvesterClientAO->RemoveObserver( aObserver );
        }
    
	if ( aObserver == iObserver )
		{
		if ( iObserver )
			{
			WRITELOG( "CHarvesterClientAO::RemoveObserver() - deleting observer" );
			iObserver = NULL;
			}
		}
    }

// ----------------------------------------------------------------------------------------
// AddHarvesterEventObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C TInt RHarvesterClient::AddHarvesterEventObserver( 
	MHarvesterEventObserver& aHarvesterEventObserver, 
	TInt aHEObserverType,
	TInt aEventInterval )
	{
	TInt err( 0 );
	if( !iHEO )
	    {
        TRAP( err, iHEO = CHarvesterEventObserverAO::NewL( *this ) );
        if ( err != KErrNone )
            {
            WRITELOG( "RHarvesterClient::RHarvesterClient() - Couldn't create harvester event observer" );
            return err;
            }
	    }
	
	TRAP(err, iHEO->AddHarvesterEventObserverL(
			aHarvesterEventObserver,
			aHEObserverType,
			aEventInterval ));
	
	return err;
	}

// ----------------------------------------------------------------------------------------
// RemoveObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C TInt RHarvesterClient::RemoveHarvesterEventObserver( MHarvesterEventObserver& aHarvesterEventObserver )
	{
    if( iHEO )
          {
          TRAPD( err, iHEO->RemoveHarvesterEventObserverL( aHarvesterEventObserver ) );
          return err;
          }
	
	return KErrNone;
	}

// ----------------------------------------------------------------------------------------
// HarvestFile
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::HarvestFile( const TDesC& aURI, RArray<TItemId>& aAlbumIds, TBool aAddLocation )
    {
    WRITELOG1( "RHarvesterClient::HarvestFile() - file %S", &aURI );
    
    HBufC8* paramBuf = NULL;
    TRAPD( err, paramBuf = SerializeArrayL( aAlbumIds ) );
    if ( err )
    	{
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot create serialized array, error: %d", err );
        if( iObserver )
            {
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), err );  
            }
    	return;
    	}
        
    CHarvesterRequestActive* harvestFileActive( NULL );
    TRAP( err, harvestFileActive = CHarvesterRequestActive::NewL( *this, iObserver, (TInt)EHarvestFile, aURI, 
                                                                                                   paramBuf, aAddLocation, iRequestQueue ) );
    if( err )
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot create harvesting request, error: %d", err );
        if( iObserver )
            {
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), err );  
            }
        return;
        }

    // send actually harvest request to server
    if( iHandle )
        {
        TRAP( err, iRequestQueue->AddRequestL( harvestFileActive ) );
        if( err && iObserver)
            {
            WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrServerBusy );
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), KErrServerBusy );  
            delete harvestFileActive;
            }
        else if( err )
            {
            WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrServerBusy );
            delete harvestFileActive;
            }
        else
            {
            WRITELOG( "RHarvesterClient::HarvestFile() - harvesting request added to queue" );
            iRequestQueue->Process();
            }
        }
    else if( iObserver )
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrDisconnected );
        iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), KErrDisconnected );  
        delete harvestFileActive;
        }
    else
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrDisconnected );
        delete harvestFileActive;
        }
    WRITELOG( "RHarvesterClient::HarvestFile() - end" );
    }

// ----------------------------------------------------------------------------------------
// HarvestFileWithUID
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::HarvestFileWithUID( const TDesC& aURI, 
                                                                                         RArray<TItemId>& aAlbumIds, 
                                                                                         TBool aAddLocation,
                                                                                         TUid /*aUid*/ )
    {
    WRITELOG1( "RHarvesterClient::HarvestFileWithUID() - file %S", &aURI );
    
    HBufC8* paramBuf = NULL;
    TRAPD( err, paramBuf = SerializeArrayL( aAlbumIds ) );
    if ( err )
        {
        WRITELOG1( "RHarvesterClient::HarvestFileWithUID() - cannot create serialized array, error: %d", err );
        if( iObserver )
            {
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), err );  
            }
        return;
        }

    CHarvesterRequestActive* harvestFileActive( NULL );
    TRAP( err, harvestFileActive = CHarvesterRequestActive::NewL( *this, iObserver, (TInt)EHarvestFile, aURI, 
                                                                                                   paramBuf, aAddLocation, iRequestQueue ) );
    if( err )
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot create harvesting request, error: %d", err );
        if( iObserver )
            {
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), err );  
            }
        return;
        }

    // send actually harvest request to server
    if( iHandle )
        {
        TRAP( err, iRequestQueue->AddRequestL( harvestFileActive ) );
        if( err && iObserver)
            {
            WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrServerBusy );
            iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), KErrServerBusy );  
            delete harvestFileActive;
            }
        else if( err )
            {
            WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrServerBusy );
            delete harvestFileActive;
            }
        else
            {
            iRequestQueue->Process();
            }
        }
    else if( iObserver )
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrDisconnected );
        iObserver->HarvestingComplete( const_cast<TDesC&>(aURI), KErrDisconnected );  
        delete harvestFileActive;
        }
    else
        {
        WRITELOG1( "RHarvesterClient::HarvestFile() - cannot not send harvest request to server, error: %d", KErrDisconnected );
        delete harvestFileActive;
        }
    }

// ----------------------------------------------------------------------------------------
// AddSessionObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::AddSessionObserverL( MHarvesterSessionObserver& aObserver  )
    {
    if( iSessionWatcher )
        {
        delete iSessionWatcher;
        iSessionWatcher = NULL;
        }
    iSessionWatcher = CHarvesterSessionWatcher::NewL( aObserver );
    }

// ----------------------------------------------------------------------------------------
// RemoveSessionObserver
// ----------------------------------------------------------------------------------------
//
EXPORT_C void RHarvesterClient::RemoveSessionObserver()
    {
    if( iSessionWatcher )
        {
        delete iSessionWatcher;
        iSessionWatcher = NULL;
        }
    }

// ----------------------------------------------------------------------------------------
// RegisterHarvestComplete
// ----------------------------------------------------------------------------------------
//
void RHarvesterClient::RegisterHarvestComplete(TDes& aURI, TRequestStatus& aStatus)
	{	
	TIpcArgs ipcArgs( &aURI );
	
	if( !iHandle )
		{
		return;
		}
	SendReceive( ERegisterHarvestComplete, ipcArgs, aStatus);
	}


// ----------------------------------------------------------------------------------------
// UnregisterHarvestComplete
// ----------------------------------------------------------------------------------------
//
void RHarvesterClient::UnregisterHarvestComplete()
	{
	if( !iHandle )
		{
		return;
		}	
	
	Send( EUnregisterHarvestComplete );
	}

// ----------------------------------------------------------------------------------------
// HarvestFile
// ----------------------------------------------------------------------------------------
//
void RHarvesterClient::HarvestFile( TInt& aService, TIpcArgs& aArgs, TRequestStatus& aStatus )
    {
    // send to server harvesting complete observer
    iHarvesterClientAO->Active();
    SendReceive( aService, aArgs, aStatus );
    }

// ----------------------------------------------------------------------------------------
// ForceHarvestFile
// ----------------------------------------------------------------------------------------
//
void RHarvesterClient::ForceHarvestFile( TInt& aService, TIpcArgs& aArgs )
    {
    // send to server harvesting complete observer
    iHarvesterClientAO->Active();
    SendReceive( aService, aArgs );
    }

// ----------------------------------------------------------------------------------------
// Version
// ----------------------------------------------------------------------------------------
//
TVersion RHarvesterClient::Version() const
    {
    WRITELOG( "RHarvesterClient::Version()" );
        
    TVersion version( KHarvesterServerMajorVersion, KHarvesterServerMinorVersion,
                              KHarvesterServerBuildVersion );
    return version;
    }

// ----------------------------------------------------------------------------------------
// StartServer
// ----------------------------------------------------------------------------------------
//
static TInt StartServer()
    {
    WRITELOG( "StartServer() - begin" );
    
    TFindServer findHarvesterServer( KHarvesterServerName );
    TFullName name;

    TInt result = findHarvesterServer.Next( name );
    if ( result == KErrNone )
        {
        WRITELOG( "StartServer() - Server allready running" );
        
        // Server already running
        return KErrNone;
        }
#ifdef _DEBUG
    else
        {
        if( result == KErrNotFound )
            {
            WRITELOG( "StartServer() - server not found running" );
            }
        else
            {
            WRITELOG1( "StartServer() error - error code: %d", result );
            }
        }
#endif
    
    result = CreateServerProcess();
    if ( result != KErrNone )
        {
        WRITELOG1( "StartServer() - creating process failed, error: %d", result );
        }
    
    WRITELOG( "StartServer() - end" ); 
    return result;
    }

// ----------------------------------------------------------------------------------------
// CreateServerProcess
// ----------------------------------------------------------------------------------------
//
static TInt CreateServerProcess()
    {
    WRITELOG( "CreateServerProcess() - begin" );
    RProcess server;
    TInt result = server.Create( KHarvesterServerExe, KNullDesC );   
    if ( result != KErrNone )
        {
        WRITELOG1( "CreateServerProcess() - failed to create server process, error: %d", result );
        return result;
        }
 
    // Process created successfully
    TRequestStatus stat( 0 );
    server.Rendezvous( stat );
    
    if ( stat != KRequestPending )
        {
        server.Kill( 0 );     // abort startup
        }
    else
        {
        server.Resume();    // logon OK - start the server
        }        
    
    User::WaitForRequest( stat ); // wait for start or death
    // we can't use the 'exit reason' if the server panicked as this
    // is the panic 'reason' and may be '0' wehich cannot be distinguished
    // from KErrNone
    result = ( server.ExitType() == EExitPanic ) ? KErrCommsBreak : stat.Int();
    server.Close();
    
    WRITELOG( "CreateServerProcess() - end" );
    
    return result;
    }

// End of file

