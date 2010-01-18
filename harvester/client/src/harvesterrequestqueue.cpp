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


#include "harvesterrequestqueue.h"
#include "harvesterrequestactive.h"
#include "harvesterlog.h"

// maximum number of active client requests ( 18 burst images + 1 video )
const TUint KMaxClientRequests = 19;

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CHarvesterRequestQueue* CHarvesterRequestQueue::NewL()
    {
    CHarvesterRequestQueue* self = new( ELeave )CHarvesterRequestQueue();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::CHarvesterRequestQueue()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CHarvesterRequestQueue::CHarvesterRequestQueue(): CActive( CActive::EPriorityStandard )
    {
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::ConstructL()
    {
    iActiveRequests = 0;
    iShutdown = EFalse;
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::~CHarvesterRequestQueue()
// Destructor.
// ---------------------------------------------------------------------------
//
CHarvesterRequestQueue::~CHarvesterRequestQueue()
    {
    WRITELOG( "CHarvesterRequestQueue::~CHarvesterRequestQueue()");
    
    Cancel();
    iRequests.ResetAndDestroy();
    
    WRITELOG( "CHarvesterRequestQueue::~CHarvesterRequestQueue() - All requests deleted");
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::Process()
// Activates next request if possible.
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::Process()
    {
    WRITELOG( "CHarvesterRequestQueue::Process()");

    if ( !IsActive() && !iShutdown && (iActiveRequests < KMaxClientRequests) )
        {  
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::AddRequestL()
// Adds new request to the queue.
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::AddRequestL( CHarvesterRequestActive* aRequest )
    {
    WRITELOG( "CHarvesterRequestQueue::AddRequestL()");
    
    iRequests.AppendL( aRequest );
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::RunL()
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::RunL()
    {
    WRITELOG2( "CHarvesterRequestQueue::RunL() begin - requests: %d, active requests: %d",
               iRequests.Count(), iActiveRequests );
    
    CHarvesterRequestActive* selectedRequest = NULL;
    
    for ( TInt i = 0; i < iRequests.Count(); i++ )
        {
        CHarvesterRequestActive* request = iRequests[i];
        
        // remove completed tasks    
        if ( request->RequestCompleted() && !request->IsActive() )
            {
            // delete completed task
            delete request;
            iRequests.Remove( i );
            
            // correct the index so that no tasks are skipped
            i--;
            if(i <= -1)
                {
                i = -1;
                }
            
            WRITELOG( "CHarvesterRequestQueue::RunL() - removing completed request");
            }
        // select priorised task
        else
            {
            // not yet activated
            if( !request->IsActive() )
                {
                selectedRequest = request;
                }
            }
        }

    // activate selected
    if ( selectedRequest )
        {
        iActiveRequests++;
        selectedRequest->Start();
        }
    
    WRITELOG2( "CHarvesterRequestQueue::RunL() end - requests: %d, active requests: %d",
            iRequests.Count(), iActiveRequests );
    
    // continue processing if needed
    if ( !iShutdown && (iActiveRequests < KMaxClientRequests)  && 
            (iRequests.Count() > iActiveRequests))
        {  
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    else
    	{
    	iRequests.Compress();
    	}
    }

// ---------------------------------------------------------------------------
// Handle any error from RunL
// ---------------------------------------------------------------------------
//
TInt CHarvesterRequestQueue::RunError( TInt aError )
    {
    WRITELOG1( "CHarvesterRequestQueue::RunError() - %d", aError);
    
    if (aError)
        {
        iActiveRequests--;
        if(iActiveRequests <= -1)
            {
            iActiveRequests = 0;
            }
        }
    
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::DoCancel()
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::DoCancel()
    {
    WRITELOG( "CHarvesterRequestQueue::DoCancel()");
    
    for( TInt i(0); i < iRequests.Count(); i++ )
        {
        iRequests[i]->Cancel();
        }
    
    iShutdown = ETrue;
    }


// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::RequestComplete()
// Completes the request
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::RequestComplete()
    {
    WRITELOG( "CHarvesterRequestQueue::RequestComplete()");
 
    iActiveRequests--;
    if(iActiveRequests <= -1)
        {
        iActiveRequests = 0;
        }
    
    WRITELOG1( "CHarvesterRequestQueue::RequestComplete() - active requests: %d", iActiveRequests );
    
    // clean completed request and possibly process next
    if ( !IsActive() && !iShutdown )
        {  
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    }

// ---------------------------------------------------------------------------
// CHarvesterRequestQueue::ForceRequests()
// ---------------------------------------------------------------------------
//
void CHarvesterRequestQueue::ForceRequests()
    {
    WRITELOG( "CHarvesterRequestQueue::ForceRequests()");
    
    Cancel();
    
    for ( TInt i = 0; i < iRequests.Count(); i++ )
        {
        CHarvesterRequestActive* request = iRequests[i];
        // remove completed tasks    
        if ( request->RequestCompleted() )
            {
            // delete completed task
            delete request;
            iRequests.Remove( i );
        
            // correct the index so that no tasks are skipped
            i--;
            if(i <= -1)
                {
                i = -1;
                }
            }
        else
            {
            iRequests[i]->ForceHarvest();
            }
        }
    }

// End of file
