/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Harvester client active object
*
*/


#include "harvesterclientao.h"
#include "harvestercommon.h"
#include "harvesterlog.h"
#include "mdsutils.h"


// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CHarvesterClientAO* CHarvesterClientAO::NewL( RHarvesterClient& aHarvesterClient )
	{
    WRITELOG( "CHarvesterClientAO::NewL()" );
	CHarvesterClientAO* self = new (ELeave) CHarvesterClientAO( aHarvesterClient );
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// ~CHarvesterClientAO
// ---------------------------------------------------------------------------
//
CHarvesterClientAO::~CHarvesterClientAO() // destruct
	{   
    WRITELOG( "CHarvesterClientAO::~CHarvesterClientAO()" );
    Cancel();
 	}

// ---------------------------------------------------------------------------
// CHarvesterClientAO
// First-phase C++ constructor
// ---------------------------------------------------------------------------
//
CHarvesterClientAO::CHarvesterClientAO( RHarvesterClient& aHarvesterClient )
    : CActive( CActive::EPriorityStandard ), 
    iObserver( NULL ),
    iHarvesterClient( aHarvesterClient )
  	{
    WRITELOG( "CHarvesterClientAO::CHarvesterClientAO()" );
	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::ConstructL() // second-phase constructor
	{
    WRITELOG( "CHarvesterClientAO::ConstructL()" );    
    CActiveScheduler::Add( this );
	}

// ---------------------------------------------------------------------------
// SetObserver
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::SetObserver( MHarvestObserver* aObserver )
	{
	WRITELOG( "CHarvesterClientAO::SetObserver()" );
	iObserver = aObserver;
	}

// ---------------------------------------------------------------------------
// RemoveObserver
// ---------------------------------------------------------------------------
//	
void CHarvesterClientAO::RemoveObserver( MHarvestObserver* aObserver )
	{
	WRITELOG( "CHarvesterClientAO::RemoveObserver()" );
	if ( aObserver == iObserver )
		{
		if ( iObserver )
			{
			WRITELOG( "CHarvesterClientAO::RemoveObserver() - deleting observer" );
			iObserver = NULL;
			}
		}
	}

// ---------------------------------------------------------------------------
// DoCancel
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::DoCancel()
	{
	WRITELOG( "CHarvesterClientAO::DoCancel()" );
	}
	
// ---------------------------------------------------------------------------
// Active
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::Active()
	{	
	if (!IsActive())
		{
		iHarvesterClient.RegisterHarvestComplete(iURI, iStatus);
		SetActive();
		}
	}

// ---------------------------------------------------------------------------
// RunL
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::RunL()
	{
	WRITELOG( "CHarvesterClientAO::RunL()" );

	const TInt status = iStatus.Int();
	
    if ( status < KErrNone )
        {
        WRITELOG1( "CHarvesterClientAO::RunL() - Error occured while harvesting, error:%d", status );
        }

	// Callback to client process
	if ( iObserver )
		{
		WRITELOG( "CHarvesterClientAO::RunL() - ECompleteRequest - calling callback" );
		iObserver->HarvestingComplete( iURI, status );
		}
	
	// if the request was not canceled or server is not terminated, Activating AO again
	if ( status != KErrCancel && status != KErrServerTerminated )
		{
		Active();
		}
	}
	
// ---------------------------------------------------------------------------
// RunError
// ---------------------------------------------------------------------------
//	
#ifdef _DEBUG
TInt CHarvesterClientAO::RunError( TInt aError )
#else
TInt CHarvesterClientAO::RunError( TInt )
#endif
    {
    WRITELOG1( "CHarvesterClientAO::RunError(), errorcode: %d", aError );
    
    return KErrNone;
    }
