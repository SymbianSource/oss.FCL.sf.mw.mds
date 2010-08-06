/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Processor object for running harvester requests
*
*/


#include "harvesternotificationqueue.h"
#include "harvesterclientao.h"
#include "harvesterlog.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CHarvesterNotificationQueue* CHarvesterNotificationQueue::NewL()
    {
    CHarvesterNotificationQueue* self = new( ELeave )CHarvesterNotificationQueue();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::CHarvesterNotificationQueue()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CHarvesterNotificationQueue::CHarvesterNotificationQueue()
    {
    }


// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CHarvesterNotificationQueue::ConstructL()
    {
    }


// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::~CHarvesterNotificationQueue()
// Destructor.
// ---------------------------------------------------------------------------
//
CHarvesterNotificationQueue::~CHarvesterNotificationQueue()
    {
    WRITELOG( "CHarvesterNotificationQueue::~CHarvesterNotificationQueue()");
    
    Cleanup( ETrue );
    iRequests.ResetAndDestroy();
    
    WRITELOG( "CHarvesterNotificationQueue::~CHarvesterNotificationQueue() - All requests deleted");
    }


// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::AddRequestL()
// Adds new request to the queue.
// ---------------------------------------------------------------------------
//
void CHarvesterNotificationQueue::AddRequestL( CHarvesterClientAO* aRequest )
    {
    WRITELOG( "CHarvesterNotificationQueue::AddRequestL()");
    
    iRequests.AppendL( aRequest );
    }

// ---------------------------------------------------------------------------
// CHarvesterNotificationQueue::RequestComplete()
// Completes the request
// ---------------------------------------------------------------------------
//
void CHarvesterNotificationQueue::Cleanup( TBool aShutdown )
    {
    WRITELOG( "CHarvesterNotificationQueue::RequestComplete()");
    
    for( TInt i = iRequests.Count() - 1; i >=0; i-- )
        {
        if( aShutdown )
            {
            iRequests[i]->Cancel();
            }
        
        if( iRequests[i]->RequestComplete() && !iRequests[i]->IsActive() )
            {
            delete iRequests[i];
            iRequests[i] = NULL;
            iRequests.Remove( i );
            
            // correct the index so that no items are skipped
            i--;
            if(i <= -1)
                {
                i = -1;
                }
            }
        }

    if( iRequests.Count() == 0 && !aShutdown )
        {
        iRequests.Compress();
        }
    }

// ---------------------------------------------------------------------------
// SetObserver
// ---------------------------------------------------------------------------
//
void CHarvesterNotificationQueue::SetObserver( MHarvestObserver* aObserver )
    {
    WRITELOG( "CHarvesterNotificationQueue::SetObserver()" );
    
    for( TInt i = iRequests.Count() - 1; i >=0; i-- )
        {
        iRequests[i]->SetObserver( aObserver );
        }
    }

// End of file

