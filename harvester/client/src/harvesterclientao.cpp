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
#include "harvesternotificationqueue.h"
#include "harvestercommon.h"
#include "harvesterlog.h"
#include "mdsutils.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "harvesterclientaoTraces.h"
#endif


// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CHarvesterClientAO* CHarvesterClientAO::NewL( RHarvesterClient& aHarvesterClient, 
        CHarvesterNotificationQueue* aNotificationQueue )
	{
    WRITELOG( "CHarvesterClientAO::NewL()" );
    OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_NEWL, "CHarvesterClientAO::NewL" );
    
	CHarvesterClientAO* self = new (ELeave) CHarvesterClientAO( aHarvesterClient, aNotificationQueue );
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
    OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_CHARVESTERCLIENTAO, "CHarvesterClientAO::~CHarvesterClientAO" );
    
    WRITELOG( "CHarvesterClientAO::~CHarvesterClientAO()" );
    Cancel();
    
    delete iURI;
    iURI = NULL;
 	}

// ---------------------------------------------------------------------------
// CHarvesterClientAO
// First-phase C++ constructor
// ---------------------------------------------------------------------------
//
CHarvesterClientAO::CHarvesterClientAO( RHarvesterClient& aHarvesterClient, 
        CHarvesterNotificationQueue* aNotificationQueue )
    : CActive( CActive::EPriorityStandard ), 
    iObserver( NULL ),
    iHarvesterClient( aHarvesterClient ),
    iNotificationQueue( aNotificationQueue ),
    iURI( NULL ),
    iRequestComplete( EFalse )
  	{
    OstTrace0( TRACE_NORMAL, DUP1_CHARVESTERCLIENTAO_CHARVESTERCLIENTAO, "CHarvesterClientAO::CHarvesterClientAO" );
    
    WRITELOG( "CHarvesterClientAO::CHarvesterClientAO()" );
	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::ConstructL() // second-phase constructor
	{
    WRITELOG( "CHarvesterClientAO::ConstructL()" );   
    OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_CONSTRUCTL, "CHarvesterClientAO::ConstructL" );
    
    CActiveScheduler::Add( this );
	}

// ---------------------------------------------------------------------------
// SetObserver
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::SetObserver( MHarvestObserver* aObserver )
	{
	WRITELOG( "CHarvesterClientAO::SetObserver()" );
	OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_SETOBSERVER, "CHarvesterClientAO::SetObserver" );
	
	iObserver = aObserver;
	}

// ---------------------------------------------------------------------------
// DoCancel
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::DoCancel()
	{
	WRITELOG( "CHarvesterClientAO::DoCancel()" );
	OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_DOCANCEL, "CHarvesterClientAO::DoCancel" );
	iRequestComplete = ETrue;
	}
	
// ---------------------------------------------------------------------------
// Active
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::Active( TDesC& aUri )
	{	
    WRITELOG( "CHarvesterClientAO::Active()");
    if( iObserver && !IsActive() )
        {
        delete iURI;
        iURI = NULL;
        iURI = aUri.Alloc();
        if( iURI )
            {
            TPtr16 uri( iURI->Des() );
            iHarvesterClient.RegisterHarvestComplete( uri, iStatus );
            SetActive();            
            }
        else if( iObserver )
            {
            iObserver->HarvestingComplete( aUri, KErrCompletion );
            iRequestComplete = ETrue;
            }
        }
	}

// ---------------------------------------------------------------------------
// RunL
// ---------------------------------------------------------------------------
//
void CHarvesterClientAO::RunL()
	{
	WRITELOG( "CHarvesterClientAO::RunL()" );
	OstTrace0( TRACE_NORMAL, CHARVESTERCLIENTAO_RUNL, "CHarvesterClientAO::RunL" );

	iNotificationQueue->Cleanup( EFalse );
	
	const TInt status = iStatus.Int();
	
    if ( status < KErrNone )
        {
        WRITELOG1( "CHarvesterClientAO::RunL() - Error occured while harvesting, error:%d", status );
        }

	// Callback to client process
	if ( iObserver && iURI )
		{
		WRITELOG( "CHarvesterClientAO::RunL() - Request complete - calling callback" );
		TPtrC16 uri( iURI->Des() );
		iObserver->HarvestingComplete( uri, status );
		}
	
    delete iURI;
    iURI = NULL;
	iRequestComplete = ETrue;
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
    
    iNotificationQueue->Cleanup( EFalse );
    iRequestComplete = ETrue;
    
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// RequestComplete
// ---------------------------------------------------------------------------
//  
TBool CHarvesterClientAO::RequestComplete()
    {
    return iRequestComplete;
    }

