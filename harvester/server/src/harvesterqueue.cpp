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
* Description:  Harvester server harvesting queue*
*/


#include "harvesterqueue.h"
#include "harvesterao.h"
#include "harvesterlog.h"
#include "harvesterblacklist.h"
#include "mdsutils.h"

// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CHarvesterQueue* CHarvesterQueue::NewL( CHarvesterAO* aHarvesterAO,
		CHarvesterBlacklist* aBlacklist )
    {
    WRITELOG( "CHarvesterQueue::NewL()" );

    CHarvesterQueue* self = CHarvesterQueue::NewLC( aHarvesterAO, aBlacklist );
    CleanupStack::Pop( self );
    return self;	
    }

// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CHarvesterQueue* CHarvesterQueue::NewLC( CHarvesterAO* aHarvesterAO,
		CHarvesterBlacklist* aBlacklist )
    {
    WRITELOG( "CHarvesterQueue::NewLC()" );

    CHarvesterQueue* self = new ( ELeave ) CHarvesterQueue(
    		aHarvesterAO, aBlacklist );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }
        
// ---------------------------------------------------------------------------
// CHarvesterQueue
// ---------------------------------------------------------------------------
//            
CHarvesterQueue::CHarvesterQueue( CHarvesterAO* aHarvesterAO,
		CHarvesterBlacklist* aBlacklist ) : iMediaIdUtil( NULL )
    {
    WRITELOG( "CHarvesterQueue::CHarvesterQueue()" );
    iHarvesterAO = aHarvesterAO;
    iBlacklist = aBlacklist;
    }

// ---------------------------------------------------------------------------
// ~CHarvesterQueue
// ---------------------------------------------------------------------------
//            
CHarvesterQueue::~CHarvesterQueue()
    {
    WRITELOG( "CHarvesterQueue::CHarvesterQueue()" );
    iItemQueue.ResetAndDestroy();
    iItemQueue.Close();
    iFs.Close();
    RMediaIdUtil::ReleaseInstance();
    }

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CHarvesterQueue::ConstructL()
    {
    WRITELOG( "CHarvesterQueue::ConstructL()" );
    User::LeaveIfError( iFs.Connect() );
	iMediaIdUtil = &RMediaIdUtil::GetInstanceL();
    }

// ---------------------------------------------------------------------------
// ItemsInQueue
// ---------------------------------------------------------------------------
// 
TInt CHarvesterQueue::ItemsInQueue()
    {
    WRITELOG( "CHarvesterQueue::ItemsInQueue()" );

    return iItemQueue.Count();
    }

// ---------------------------------------------------------------------------
// GetNextItem
// ---------------------------------------------------------------------------
//
CHarvesterData* CHarvesterQueue::GetNextItem()
	{
    WRITELOG( "CHarvesterQueue::GetNextItem()" );
    CHarvesterData* item = NULL;
     
   	if ( iItemQueue.Count() > 0 )
        {
#ifdef _DEBUG
        WRITELOG1( "Harvester queue items = %d", iItemQueue.Count() );
#endif
        item = iItemQueue[0];
        iItemQueue.Remove( 0 );	
        }
   	else
        {
        WRITELOG( "Harvester queue items zero!" );
        iItemQueue.Compress();
        }
   	
   	WRITELOG( "CHarvesterQueue::GetNextItem, end" );
   	return item;
	}    

// ---------------------------------------------------------------------------
// Append
// Files are checked against blacklist and blacklisted files
// will not be appended.
// ---------------------------------------------------------------------------
//
void CHarvesterQueue::Append( CHarvesterData* aItem )
	{
    WRITELOG( "CHarvesterQueue::Append()" );
    TInt err = KErrNone;

    if ( iBlacklist )
        {
        TUint32 mediaId( 0 );
		err = iMediaIdUtil->GetMediaId( aItem->Uri(), mediaId );
        
        TTime time( 0 );
        if ( err == KErrNone && iBlacklist->IsBlacklisted(
        		aItem->Uri(), mediaId, time ) )
            {
            WRITELOG( "CHarvesterQueue::Append() - found a blacklisted file" );
            delete aItem;
            aItem = NULL;
            err = KErrCorrupt;
            }
        }

    if ( err != KErrCorrupt )
        {
		// check if fast harvest file and add to start of queue
    	if ( aItem->ObjectType() == EFastHarvest || aItem->Origin() == MdeConstants::Object::ECamera )
    		{
    		err = iItemQueue.Insert( aItem, 0 );
    		}
    	else
    		{
    		err = iItemQueue.Append( aItem );
    		}
    	
    	if( err != KErrNone )
			{
			delete aItem;
			aItem = NULL;
			}
        }

    if ( err != KErrNone )
        {
        WRITELOG1( "CHarvesterQueue::Append() - error: %d", err );
        delete aItem;
        }
    }

// ---------------------------------------------------------------------------
// RemoveItems
// Remove items from the queue based on them mediaid.
// ---------------------------------------------------------------------------
//
TUint CHarvesterQueue::RemoveItems( TUint32 aMediaId )
    {
#ifdef _DEBUG
    WRITELOG1( "CHarvesterQueue::RemoveItems( %d )", aMediaId);
    
    WRITELOG1( "CHarvesterQueue::RemoveItems() iItemQueue.Count() == %d", iItemQueue.Count());
#endif

    TUint removedCount(0);
    TInt err(KErrNone);
    TUint32 mediaId( 0 );
    CHarvesterData* hd = NULL;
    
    for(TInt i = iItemQueue.Count() - 1; i >=0; i--)
        {
        hd = iItemQueue[i];
        err = iMediaIdUtil->GetMediaId( hd->Uri(), mediaId );
        
        if( err == KErrNone)
            {
            if( mediaId == aMediaId)
                {
                delete hd;
                hd = NULL;
                iItemQueue.Remove( i );
                removedCount++;
                }
            else
                {
                WRITELOG2( "CHarvesterQueue::RemoveItems( ) %d != %d", mediaId, aMediaId);
                }
            }
        else
            {
            WRITELOG1( "CHarvesterQueue::RemoveItems( ) GetMediaId err == %d", err);
            }
        }
    iItemQueue.Compress();
#ifdef _DEBUG
    WRITELOG2( "CHarvesterQueue::RemoveItems() iItemQueue.Count() = %d, removedCount = %d", iItemQueue.Count(), removedCount);
#endif
    return removedCount;
    }

// ---------------------------------------------------------------------------
// MonitorEvent
// ---------------------------------------------------------------------------
//  
void CHarvesterQueue::MonitorEvent( CHarvesterData* aHarvesterData )
    {
   	Append( aHarvesterData );

    // signal to start harvest if harvester idles
    if ( !iHarvesterAO->IsServerPaused() )
        {
        iHarvesterAO->SetNextRequest( CHarvesterAO::ERequestHarvest );
        }
    }

// ---------------------------------------------------------------------------
// MonitorEvent
// ---------------------------------------------------------------------------
//  
void CHarvesterQueue::MonitorEvent(
        RPointerArray<CHarvesterData>& aHarvesterDataArray )
    {
    for( TInt i = aHarvesterDataArray.Count(); --i >= 0; )
    	{
    	MonitorEvent( aHarvesterDataArray[i] );
    	}
    
    // "clear" array after ownership of items
    // was changed for MonitorEvent-method
    aHarvesterDataArray.Reset();
    }

