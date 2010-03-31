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
* Description:  An interface to Location Manager server
*
*/

#include <f32file.h>
#include <s32mem.h>
#include <data_caging_path_literals.hrh>

#include <rlocationmanager.h>
#include <locationeventdef.h>
#include "locationmanagerdefs.h"
#include "locationmanagerdebug.h"

// --------------------------------------------------------------------------
// LaunchServer
// Launches the server.
// --------------------------------------------------------------------------
//
TInt LaunchServer()
    {
    LOG( "RLocationManager::LaunchServer begin" );
    TParse parser;
    parser.Set( KLocServerFileName, &KDC_PROGRAMS_DIR, NULL );

    // DLL launch
    RProcess server;
    const TInt ret = server.Create( parser.FullName(), KNullDesC );

    if ( ret != KErrNone )  // Loading failed.
        {
        return ret;
        }
    
    TRequestStatus status( KErrNone );
    server.Rendezvous( status );

    if ( status != KRequestPending )
        {
        LOG( "RLocationManager::LaunchServer Failed" );
        server.Kill( 0 );     // Abort startup.
        server.Close();
        return KErrGeneral;
        }
    else
        {
        server.Resume();    // Logon OK - start the server.
        }
        
    User::WaitForRequest( status );
    server.Close();
    LOG( "RLocationManager::LaunchServer end" );
    return status.Int();    
    }

// --------------------------------------------------------------------------
// RLocationManager::RLocationManager
// C++ Constructor
// --------------------------------------------------------------------------
//
EXPORT_C RLocationManager::RLocationManager()
    {
    }

// --------------------------------------------------------------------------
// RLocationManager::Connect
// Creates connection to server
// --------------------------------------------------------------------------
//
EXPORT_C TInt RLocationManager::Connect()
    {
    LOG( "RLocationManager::Connect(), begin" );
    TInt ret = CreateSession( KLocServerName, Version(), KSessionSlotCount);
    if ( ret != KErrNone )
        {
        ret = LaunchServer();
        if ( ret == KErrNone )
            {
            ret = CreateSession( KLocServerName, Version() );    
            }       
        }
    LOG( "RLocationManager::Connect(), end" );
    return ret;
    }
    
// --------------------------------------------------------------------------
// RLocationManager::Close
// --------------------------------------------------------------------------
//      
EXPORT_C void RLocationManager::Close()
    {
    LOG( "RLocationManager::Close(), begin" );
    // close session    
    RSessionBase::Close();
    LOG( "RLocationManager::Close(), end" );
    }

// --------------------------------------------------------------------------
// RLocationManager::Version
// Returns the version of Location Manager.
// --------------------------------------------------------------------------
//      
TVersion RLocationManager::Version() const
    {
    TVersion version( KLocationManagerServerMajor, 
                              KLocationManagerServerMinor, 
                              KLocationManagerServerBuild );
    return version;
    }

// --------------------------------------------------------------------------
// RLocationManager::CompleteRequest
// Completes an asynchronous request with an error code.
// --------------------------------------------------------------------------
//      
void RLocationManager::CompleteRequest( TRequestStatus& aStatus, TInt aError )
	{
    TRequestStatus* status = &aStatus;
    User::RequestComplete( status, aError );
	}


//End of File
