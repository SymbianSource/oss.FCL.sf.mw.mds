/*
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Notifier engine / server side*
*/

#include "mdsnotifier.h"

#include "mdcresult.h"
#include "mdcitem.h"
#include "mdsserversession.h"
#include "mdsnotifycomparator.h"
#include "mdslogger.h"
#include "mdcserializationbuffer.h"
#include "mdccommon.pan"

__USES_LOGGER

// ------------------------------------------------
// NewL
// ------------------------------------------------
//
CMdSNotifier* CMdSNotifier::NewL()
    {
    CMdSNotifier* that = CMdSNotifier::NewLC();
    CleanupStack::Pop( that );
    return that;
    }


// ------------------------------------------------
// NewLC
// ------------------------------------------------
//
CMdSNotifier* CMdSNotifier::NewLC()
    {
    CMdSNotifier* that = new(ELeave) CMdSNotifier();
    CleanupStack::PushL( that );
    that->ConstructL();
    return that;
    }


// ------------------------------------------------
// Constructor
// ------------------------------------------------
//
CMdSNotifier::CMdSNotifier()
    {
    
    }

// ------------------------------------------------
// ConstructL
// ------------------------------------------------
//
void CMdSNotifier::ConstructL()
    {
    iComparator = CMdSNotifyComparator::NewL();
    }

// ------------------------------------------------
// Destructor
// ------------------------------------------------
//
CMdSNotifier::~CMdSNotifier()
    {
    delete iComparator;
    
    const TInt count = iEntries.Count();
    
    for ( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];
            
        if ( e.iSerializedCondition )
            {
            delete e.iSerializedCondition;
            e.iSerializedCondition = NULL;
            }
        if ( e.iDataBuffer )
            {
            delete e.iDataBuffer;
            e.iDataBuffer = NULL;
            }
        }
    
    iEntries.Close();
    }

// ------------------------------------------------
// Constructor
// ------------------------------------------------
//
CMdSNotifier::TEntry::TEntry( TInt aId,
    TConditionType aType,
    CMdCSerializationBuffer* aSerializedBuffer,
    TDefId aNamespaceDefId, 
    CMdSServerSession& aSession, 
    TBool aConfidential)
    : iId( aId )
    , iType( aType )
    , iNamespaceDefId(aNamespaceDefId)
    , iSerializedCondition( aSerializedBuffer )
    , iSession( aSession )
    , iConfidential(aConfidential)    
    {
    iDataBuffer = NULL;
    iRemoteSizeMsgSlot = KErrNotFound;
    }

// ------------------------------------------------
// TriggerL completes the client message and sends the data size
// ------------------------------------------------
//
void CMdSNotifier::TEntry::TriggerL(
	TUint32 aCompleteCode,
    const RArray<TItemId>& aIdArray )
    {
    const TInt remoteSizeMsgSlot = iRemoteSizeMsgSlot;
    iRemoteSizeMsgSlot = KErrNotFound;

    __ASSERT_DEBUG( !iDataBuffer, MMdCCommon::Panic( KErrCorrupt ) );

    if(aIdArray.Count())
		{
		iDataBuffer = CopyToBufferL( aIdArray );
	    iSession.SizeToRemoteL( iMessage, remoteSizeMsgSlot, iDataBuffer->Size());
		}
	else
		{
	    iSession.SizeToRemoteL( iMessage, remoteSizeMsgSlot, 0);
		}

    __LOG2( ELogServer, "<- Notify trigger %d (%d)", iId, aCompleteCode );
    iMessage.Complete( aCompleteCode );
    }

// ------------------------------------------------
// TriggerRelationItemsL completes the client
//                  message and sends the data size
// ------------------------------------------------
//
void CMdSNotifier::TEntry::TriggerRelationItemsL(
	TUint32 aCompleteCode,
	CMdCSerializationBuffer& aBuffer,
	const RArray<TItemId>& aRelationIdArray)
    {
    const TInt remoteSizeMsgSlot = iRemoteSizeMsgSlot;
    iRemoteSizeMsgSlot = KErrNotFound;

    __ASSERT_DEBUG( !iDataBuffer, MMdCCommon::Panic( KErrCorrupt ) );

    if(aRelationIdArray.Count())
		{
		iDataBuffer = CopyItemsToBufferL( aBuffer, aRelationIdArray );
	    iSession.SizeToRemoteL( iMessage, remoteSizeMsgSlot, iDataBuffer->Size());
		}
	else
		{
	    iSession.SizeToRemoteL( iMessage, remoteSizeMsgSlot, 0);
		}

    __LOG2( ELogServer, "<- Notify trigger %d (%d)", iId, aCompleteCode );
    iMessage.Complete( aCompleteCode );
    }

// ------------------------------------------------
// TriggerSchemaL sends a schema notification
// ------------------------------------------------
//
void CMdSNotifier::TEntry::TriggerSchema()
    {
    iRemoteSizeMsgSlot = KErrNotFound;
    iMessage.Complete( ESchemaModify );
    }

// ------------------------------------------------
// TriggerError send a error message to the client
// ------------------------------------------------
//
void CMdSNotifier::TEntry::TriggerError( TInt aErrorCode )
    {
    iRemoteSizeMsgSlot = KErrNotFound;
	delete iDataBuffer;
    iDataBuffer = NULL;
    __LOG2( ELogServer, "<- Notify trigger %d (%d)", iId, aErrorCode );

    if( !iMessage.IsNull() )
    	{
    	iMessage.Complete( aErrorCode );
    	}
    }

// ------------------------------------------------
// CopyToBufferL copies id to buffer
// ------------------------------------------------
//
CMdCSerializationBuffer* CMdSNotifier::TEntry::CopyToBufferL(const RArray<TItemId>& aIdArray)
	{
	// IDs are always stored in object ID, 
	// even if those are actually relation or event IDs

	const TUint32 count = aIdArray.Count();

	CMdCSerializationBuffer* buffer = CMdCSerializationBuffer::NewLC(
			sizeof( TMdCItemIds )
			+ count * CMdCSerializationBuffer::KRequiredSizeForTItemId );

	TMdCItemIds itemIds;
	itemIds.iNamespaceDefId = NamespaceDefId();
	itemIds.iObjectIds.iPtr.iCount = count;
	itemIds.iObjectIds.iPtr.iOffset = sizeof(TMdCItemIds);
	itemIds.SerializeL( *buffer );

	for( TInt i = 0; i < count; ++i )
		{
		buffer->InsertL( aIdArray[i] );
		}

	CleanupStack::Pop( buffer );
	return buffer;	
	}

// ------------------------------------------------
// CopyItemsToBufferL copies relation items to buffer
// ------------------------------------------------
//
CMdCSerializationBuffer* CMdSNotifier::TEntry::CopyItemsToBufferL(
		CMdCSerializationBuffer& aRelationItemsBuffer, 
		const RArray<TItemId>& aIdArray)
	{
	const TUint32 count = aIdArray.Count();
	aRelationItemsBuffer.PositionL( KNoOffset );
	const TMdCItems& items = TMdCItems::GetFromBufferL( aRelationItemsBuffer );

	CMdCSerializationBuffer* buffer = NULL;
	if ( items.iRelations.iPtr.iCount == count )
		{
		buffer = CMdCSerializationBuffer::NewLC( aRelationItemsBuffer );
		}
	else
		{
		buffer = CMdCSerializationBuffer::NewLC( sizeof(TMdCItems)
				+ count * sizeof(TMdCRelation) );

		TMdCItems returnItems;
		returnItems.iNamespaceDefId = items.iNamespaceDefId;
		returnItems.iRelations.iPtr.iCount = count;
		returnItems.iRelations.iPtr.iOffset = sizeof(TMdCItems);
		buffer->PositionL( sizeof(TMdCItems) );

		for( TInt i = 0; i < items.iRelations.iPtr.iCount; ++i )
			{
			TMdCRelation relation;
			relation.DeserializeL( aRelationItemsBuffer );

			if ( aIdArray.Find( relation.iId ) >= 0 )
				{
				relation.SerializeL( *buffer );
				}
			}
		buffer->PositionL( KNoOffset );
		returnItems.SerializeL( *buffer );
		}
	
	CleanupStack::Pop( buffer );
	return buffer;
	}

// ------------------------------------------------
// CacheL caches the notification
// ------------------------------------------------
//
void CMdSNotifier::TEntry::CacheL(TUint32 aCompleteCode, const RArray<TItemId>& aIdArray )
    {
    if ( aIdArray.Count() <= 0 )
    	{
    	return;
    	}

    CMdCSerializationBuffer* data = CopyToBufferL( aIdArray );
    iSession.CacheNotificationL( iId, aCompleteCode, data );
    }

// ------------------------------------------------
// CacheRelationItemsL caches the notification
// ------------------------------------------------
//
void CMdSNotifier::TEntry::CacheRelationItemsL(TUint32 aCompleteCode,
		CMdCSerializationBuffer& aBuffer, 
		const RArray<TItemId>& aRelationIdArray )
    {
    CMdCSerializationBuffer* data = CopyItemsToBufferL( aBuffer, 
    		aRelationIdArray );
    iSession.CacheNotificationL(iId, aCompleteCode, data);
    }

// ------------------------------------------------
// CacheL for schema mods
// ------------------------------------------------
//
void CMdSNotifier::TEntry::CacheL(TUint32 aCompleteCode)
    {
    iSession.CacheNotificationL(iId, aCompleteCode, NULL);
    }
    
// ------------------------------------------------
// TriggerCachedL triggers a previously cached notification
// ------------------------------------------------
//
void CMdSNotifier::TEntry::TriggerCachedL(TUint32 aCompleteCode, 
		CMdCSerializationBuffer* aData)
    {
    const TInt remoteSizeMsgSlot = iRemoteSizeMsgSlot;
    iRemoteSizeMsgSlot = KErrNotFound;

    __ASSERT_DEBUG( !iDataBuffer, MMdCCommon::Panic( KErrCorrupt ) );

    if( aData )
    	{
    	iSession.SizeToRemoteL( iMessage, remoteSizeMsgSlot, aData->Size());
    	}

	iDataBuffer = aData;

    __LOG2( ELogServer, "<- Notify trigger %d (%d)", iId, aCompleteCode );
    iMessage.Complete( aCompleteCode );
    }

// ------------------------------------------------
// SetupForCallback
// ------------------------------------------------
//
void CMdSNotifier::TEntry::SetupForCallback(
    RMessage2 aMessage, TInt aRemoteSizeMsgSlot )
    {
    __ASSERT_DEBUG( !IsPending(), MMdCCommon::Panic( KErrCorrupt ) );
    iMessage = aMessage;
    iRemoteSizeMsgSlot = aRemoteSizeMsgSlot;
    }

// ------------------------------------------------
// GetDataBuffer
// ------------------------------------------------
//
CMdCSerializationBuffer* CMdSNotifier::TEntry::GetDataBuffer()
    {
    CMdCSerializationBuffer* data = iDataBuffer;
    iDataBuffer = NULL;
    return data;
    }

// ------------------------------------------------
// CreateEntry creates a new notifier entry
// ------------------------------------------------
//
CMdSNotifier::TEntry& CMdSNotifier::CreateEntryL( TInt aId,
    TConditionType aType, CMdCSerializationBuffer* aSerializedBuffer,
    TDefId aNamespaceDefId, CMdSServerSession& aSession, TBool aConfidential )
    {

    User::LeaveIfError( iEntries.Append(
        TEntry( aId, aType, aSerializedBuffer, aNamespaceDefId, aSession, aConfidential ) ) );
    return iEntries[ iEntries.Count() - 1 ];
    }

// ------------------------------------------------
// FindEntry
// ------------------------------------------------
//
CMdSNotifier::TEntry& CMdSNotifier::FindEntryL( TInt aId )
    {
    CMdSNotifier::TEntry* entry = NULL;
    
    const TInt count = iEntries.Count();
    
    for ( TInt i = 0; i < count; ++i )
        {
        if ( iEntries[i].iId == aId )
            {
            entry = &iEntries[i];
            break;
            }
        }

    if( !entry )
    	{
    	User::Leave( KErrNotFound );
    	}
    
    return *entry;
    }

// ------------------------------------------------
// RemoveEntryL
// ------------------------------------------------
//
void CMdSNotifier::RemoveEntryL( TInt aId )
    {
    const TInt count = iEntries.Count();
    
    for ( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];
        if ( e.iId == aId )
            {
            if ( e.IsPending() )
                {
                e.TriggerError( KErrCancel );
                }
            
            if ( e.iSerializedCondition )
            	{
            	delete e.iSerializedCondition;
            	e.iSerializedCondition = NULL;
            	}
            if ( e.iDataBuffer )
            	{
            	delete e.iDataBuffer;
            	e.iDataBuffer = NULL;
            	}
            iEntries.Remove( i );
            return;
            }
        }
    User::Leave( KErrNotFound );
    }

// ------------------------------------------------
// RemoveEntriesBySession
// ------------------------------------------------
//
void CMdSNotifier::RemoveEntriesBySession(
    const CMdSServerSession& aSession )
    {
    const TInt count = iEntries.Count();
    
    for ( TInt i = count; --i >= 0; )
        {
        TEntry& e = iEntries[i];
        if ( &e.iSession == &aSession ) // pointer comparision
            {
            if ( e.IsPending() )
                {
                e.TriggerError( KErrCancel );
                }
            
            delete e.iSerializedCondition;
            delete e.iDataBuffer;
            iEntries.Remove( i );
            }
        }
    }

// ------------------------------------------------
// NotifyAdded
// ------------------------------------------------
//
void CMdSNotifier::NotifyAddedL(CMdCSerializationBuffer& aSerializedItems, 
							    CMdCSerializationBuffer& aSerializedItemIds)
    {
    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        if ( ! (e.iType & ( EObjectNotifyAdd | ERelationNotifyAdd | EEventNotifyAdd ) ) )
        	{
        	continue;
        	}
        
        RArray<TItemId> matchingItemIdArray;
   		CleanupClosePushL( matchingItemIdArray );

		aSerializedItems.PositionL( KNoOffset );
		aSerializedItemIds.PositionL( KNoOffset );

		const TBool someMatches = iComparator->MatchL( e.NamespaceDefId(), e.iType, e.Condition(), 
												 aSerializedItems, aSerializedItemIds, 
												 matchingItemIdArray,
												 e.AllowConfidential() );

        if( someMatches ) // check if there is some matches
            {
            if( e.IsPending() )
            	{
            	// match found. trigger notifier entry !
	            TRAPD( err, e.TriggerL( EObjectNotifyAdd | ERelationNotifyAdd | EEventNotifyAdd,
	            		matchingItemIdArray ) );
	            if( err != KErrNone )
	            	{
	            	e.TriggerError( err );
	            	}
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( EObjectNotifyAdd | ERelationNotifyAdd | EEventNotifyAdd,
            			matchingItemIdArray ) );
            	}
            }

   		CleanupStack::PopAndDestroy( &matchingItemIdArray );
        }
    }

// ------------------------------------------------
// NotifyRemoved
// ------------------------------------------------
//
void CMdSNotifier::NotifyRemovedL(CMdCSerializationBuffer& aSerializedItemIds, 
								  TBool aItemIsConfidential)
    {
	aSerializedItemIds.PositionL( KNoOffset );

	const TMdCItemIds& itemIds = TMdCItemIds::GetFromBufferL( aSerializedItemIds );

    RArray<TItemId> objectIdArray;
	CleanupClosePushL( objectIdArray );
    RArray<TItemId> eventIdArray;
	CleanupClosePushL( eventIdArray );
    RArray<TItemId> relationIdArray;
	CleanupClosePushL( relationIdArray );

    //get removed item IDs
	if( itemIds.iObjectIds.iPtr.iCount > 0 )
		{
		aSerializedItemIds.PositionL( itemIds.iObjectIds.iPtr.iOffset );

    	objectIdArray.ReserveL( itemIds.iObjectIds.iPtr.iCount );
    	for( TUint32 i = 0; i < itemIds.iObjectIds.iPtr.iCount; i++ )
    		{
    		TItemId objectId;
    		aSerializedItemIds.ReceiveL( objectId );
    		if ( objectId != KNoId )
    			{
    			objectIdArray.Append( objectId );
    			}
    		}
		}
	if( itemIds.iEventIds.iPtr.iCount > 0 )
		{
		aSerializedItemIds.PositionL( itemIds.iEventIds.iPtr.iOffset );

    	eventIdArray.ReserveL( itemIds.iEventIds.iPtr.iCount );
    	for( TUint32 i = 0; i < itemIds.iEventIds.iPtr.iCount; i++ )
    		{
    		TItemId eventId;
    		aSerializedItemIds.ReceiveL( eventId );
    		if ( eventId != KNoId )
    			{
    			eventIdArray.Append( eventId );
    			}
    		}
		}
	if( itemIds.iRelationIds.iPtr.iCount > 0 )
		{
		aSerializedItemIds.PositionL( itemIds.iRelationIds.iPtr.iOffset );

    	relationIdArray.ReserveL( itemIds.iRelationIds.iPtr.iCount );
    	for( TUint32 i = 0; i < itemIds.iRelationIds.iPtr.iCount; i++ )
    		{
    		TItemId relationId;
    		aSerializedItemIds.ReceiveL( relationId );
    		if ( relationId != KNoId )
    			{
    			relationIdArray.Append( relationId );
    			}
    		}
		}

	const TInt objectCount( objectIdArray.Count() );
	const TInt eventCount( eventIdArray.Count() );
	const TInt relationCount( relationIdArray.Count() );
	if( objectCount != 0 
			|| eventCount != 0 
			|| relationCount != 0 )
		{
		const TInt entriesCount = iEntries.Count();
	    for( TInt i=0; i < entriesCount; ++i )
	        {
	        TEntry& e = iEntries[i];
	        
	        // if namespace definition IDs don't match skip listener entry
	        if( e.NamespaceDefId() != itemIds.iNamespaceDefId )
	        	{
	        	continue;
	        	}

	        if(aItemIsConfidential && !e.AllowConfidential())
	        	{
	        	continue;	
	        	}

	        if( e.iType & EObjectNotifyRemove && objectCount > 0 )
	            {
	            // collect matching object IDs
	            RArray<TItemId> matchingObjectIdArray;
				CleanupClosePushL( matchingObjectIdArray );
	
	            const TBool allMatches = iComparator->MatchObjectIdsL( e.Condition(),
	            		objectIdArray, matchingObjectIdArray );
	
				// check is there any matches
				if( allMatches || matchingObjectIdArray.Count() > 0 )
	            	{
	            	if(e.IsPending())
	            		{
		            	// Match found. Trigger notifier entry.
		            	TInt err( KErrNone );
		            	
		            	if( allMatches )
		            		{
		            		// all matches so send whole object ID array
		            		TRAP( err, e.TriggerL( EObjectNotifyRemove, 
		            				objectIdArray ) );
		            		}
		            	else
		            		{
		            		TRAP( err, e.TriggerL( EObjectNotifyRemove, 
		            				matchingObjectIdArray ) );
		            		}
	
		            	if( err != KErrNone )
			            	{
			            	e.TriggerError( err );
		    	        	}
	            		}
	            	else
	            		{
						if( allMatches )
		            		{
		            		// all matches so send whole object ID array
	            			TRAP_IGNORE( e.CacheL( EObjectNotifyRemove, 
	            					objectIdArray ) );
		            		}
		            	else
		            		{
		            		TRAP_IGNORE( e.CacheL( EObjectNotifyRemove, 
		            				matchingObjectIdArray ) );
		            		}
	            		}
	            	}
	
				CleanupStack::PopAndDestroy( &matchingObjectIdArray );
				}
	        else if( ( e.iType & EEventNotifyRemove ) 
	        		&& eventCount > 0 )
            	{
				// event condition can't contain ID conditions, 
            	// so get all IDs
	        	if(e.IsPending())
	        		{
	            	// Match found. Trigger notifier entry.
	            	TRAPD( err, e.TriggerL( EEventNotifyRemove, 
	            			eventIdArray ) );
	            	if( err != KErrNone )
		            	{
		            	e.TriggerError( err );
	    	        	}
	        		}
	        	else
	        		{
	        		TRAP_IGNORE( e.CacheL( EEventNotifyRemove, 
	        				eventIdArray ) );
	        		}
            	}
	        else if( ( e.iType & ERelationNotifyRemove ) 
	        		&& relationCount > 0 )
            	{
	            // relation condition can't contain ID conditions, 
            	// so get all IDs
	        	if(e.IsPending())
	        		{
	            	// Match found. Trigger notifier entry.
	            	TRAPD( err, e.TriggerL( ERelationNotifyRemove, 
	            			relationIdArray ) );
	            	if( err != KErrNone )
		            	{
		            	e.TriggerError( err );
	    	        	}
	        		}
	        	else
	        		{
	        		TRAP_IGNORE( e.CacheL( ERelationNotifyRemove, 
	        				relationIdArray ) );
	        		}
            	}
	        }
		}
	CleanupStack::PopAndDestroy( 3, &objectIdArray ); // relationIdArray, eventIdArray, objectIdArray
    }

// ------------------------------------------------
// NotifyModified
// ------------------------------------------------
//
void CMdSNotifier::NotifyModifiedL(CMdCSerializationBuffer& aSerializedItems, 
							       CMdCSerializationBuffer& aSerializedItemIds)
    {
    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        if ( ! (e.iType & ( EObjectNotifyModify | ERelationNotifyModify ) ) )
        	{
        	continue;
        	}
  
        RArray<TItemId> matchingObjectIdArray;
		CleanupClosePushL( matchingObjectIdArray );

		aSerializedItems.PositionL( KNoOffset );
		aSerializedItemIds.PositionL( KNoOffset );

		const TBool someMatches = iComparator->MatchL( e.NamespaceDefId(), 
				e.iType, e.Condition(), aSerializedItems, aSerializedItemIds, 
				matchingObjectIdArray, e.AllowConfidential() );

        if( someMatches ) // check if there is some matches
            {
            if( e.IsPending() )
            	{
            	// match found. trigger notifier entry !
	            TRAPD( err, e.TriggerL( EObjectNotifyModify | ERelationNotifyModify,
	            		matchingObjectIdArray ) );
	            if( err != KErrNone )
	            	{
	            	e.TriggerError( err );
	            	}
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( EObjectNotifyModify | ERelationNotifyModify,
            			matchingObjectIdArray ) );
            	}
            }

		CleanupStack::PopAndDestroy( &matchingObjectIdArray );
        }
    }

// ------------------------------------------------
// NotifyModified
// ------------------------------------------------
//
void CMdSNotifier::NotifyModifiedL(const RArray<TItemId>& aObjectIds)
	{
	if (aObjectIds.Count() == 0)
    	{
    	return;
    	}

    const TInt count = iEntries.Count();

    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        if( e.iType & EObjectNotifyModify )
            {
            // collect matching object IDs
            RArray<TItemId> matchingObjectIdArray;
			CleanupClosePushL( matchingObjectIdArray );

                const TBool allMatches = iComparator->MatchObjectIdsL( e.Condition(),
            		aObjectIds, matchingObjectIdArray );

			// check is there any matches
			if( allMatches || matchingObjectIdArray.Count() > 0 )
            	{
            	if(e.IsPending())
            		{
	            	// Match found. Trigger notifier entry.
	            	TInt err( KErrNone );

	            	if( allMatches )
	            		{
	            		// all matches so send whole object ID array
	            		TRAP( err, e.TriggerL( EObjectNotifyModify, 
	            				aObjectIds ) );
	            		}
	            	else
	            		{
	            		TRAP( err, e.TriggerL( EObjectNotifyModify, 
	            				matchingObjectIdArray ) );
	            		}

	            	if( err != KErrNone )
		            	{
		            	e.TriggerError( err );
	    	        	}
            		}
            	else
            		{
					if( allMatches )
	            		{
	            		// all matches so send whole object ID array
            			TRAP_IGNORE( e.CacheL( EObjectNotifyModify, 
            					aObjectIds ) );
	            		}
	            	else
	            		{
	            		TRAP_IGNORE( e.CacheL( EObjectNotifyModify, 
	            				matchingObjectIdArray ) );
	            		}
            		}
            	}

			CleanupStack::PopAndDestroy( &matchingObjectIdArray );
            }
        }
	}

// ------------------------------------------------
// NotifyRemoved
// ------------------------------------------------
//
void CMdSNotifier::NotifyRemovedL(const RArray<TItemId>& aItemIdArray)
	{
    const TInt entriesCount( iEntries.Count() );
    for( TInt i=0; i<entriesCount; ++i )
        {
        TEntry& e = iEntries[i];

        if( e.iType & EObjectNotifyRemove )
        	{
            if( e.IsPending() )
            	{
	            TRAPD( err, e.TriggerL( EObjectNotifyRemove, aItemIdArray ) );
	            if( err != KErrNone )
	            	{
	            	e.TriggerError( err );
	            	}
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( EObjectNotifyRemove, aItemIdArray ) );
            	}
        	}
        }
	}

// ------------------------------------------------
// NotifyObjectPresent
// ------------------------------------------------
//
void CMdSNotifier::NotifyObjectPresent(TBool aPresent, const RArray<TItemId>& aObjectIds)
    {
    if (aObjectIds.Count() == 0)
    	{
    	return;
    	}

    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        // No condition matching, object present changes
        // are always notified to object present observers
        if( e.iType & ( EObjectNotifyPresent | EObjectNotifyNotPresent )  )
            {
            const TMdSObserverNotificationType objectState = 
            	aPresent ? EObjectNotifyPresent : EObjectNotifyNotPresent;

            if( e.IsPending() )
            	{
            	// match found. trigger notifier entry !
	            TRAPD( err, e.TriggerL( objectState, aObjectIds ) );
	            if( err != KErrNone )
	            	{
	            	e.TriggerError( err );
	            	}
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( objectState, aObjectIds ) );
            	}
            }
        }
    }

// ------------------------------------------------
// NotifyRelationPresent
// ------------------------------------------------
//
void CMdSNotifier::NotifyRelationPresent(TBool aPresent, const RArray<TItemId>& aRelationIds)
    {
    if (aRelationIds.Count() == 0)
    	{
    	return;
    	}

    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        // No condition matching, relation present changes
        // are always notified to relation present observers
        if( e.iType & ( ERelationNotifyPresent | ERelationNotifyNotPresent ) )
            {
            const TMdSObserverNotificationType relationState = 
            	aPresent ? ERelationNotifyPresent : ERelationNotifyNotPresent;

            if( e.IsPending() )
            	{
            	// match found. trigger notifier entry !
	            TRAPD( err, e.TriggerL( relationState, aRelationIds ) );
	            if( err != KErrNone )
	            	{
	            	e.TriggerError( err );
	            	}
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( relationState, aRelationIds ) );
            	}
            }
        }
    }


// ------------------------------------------------
// NotifySchemaAdded
// ------------------------------------------------
//
void CMdSNotifier::NotifySchemaAddedL()
    {
    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        TEntry& e = iEntries[i];

        // No condition matching, schema additions 
        // are always notified to schema observers
        if( e.iType == ESchemaModify )
            {
            if( e.IsPending() )
            	{
	            // match found. trigger notifier entry
	            e.TriggerSchema();
            	}
            else
            	{
            	TRAP_IGNORE( e.CacheL( ESchemaModify ) );
            	}
            }
        }
    }


// ------------------------------------------------
// CheckForNotifier
// ------------------------------------------------
//
TBool CMdSNotifier::CheckForNotifier( TUint32 aNotifyTypes )
    {
    const TInt count = iEntries.Count();
    
    for( TInt i = 0; i < count; ++i )
        {
        if ( iEntries[i].iType & aNotifyTypes )
        	{
        	return ETrue;
        	}
        }
    return EFalse;
    }

void CMdSNotifier::NotifyRemovedRelationItemsL( 
		CMdCSerializationBuffer& aBuffer )
	{
	aBuffer.PositionL( KNoOffset );

	const TMdCItems& items = TMdCItems::GetFromBufferL( aBuffer );

	if( items.iRelations.iPtr.iCount )
		{
		const TInt entriesCount = iEntries.Count();
	    for( TInt i = 0; i < entriesCount; ++i )
	        {
	        TEntry& e = iEntries[i];
	        
	        // if namespace definition IDs don't match skip listener entry
	        if( e.NamespaceDefId() != items.iNamespaceDefId )
	        	{
	        	continue;
	        	}
	        
	        if( e.iType & ERelationItemNotifyRemove )
            	{
            	aBuffer.PositionL( items.iRelations.iPtr.iOffset );
	            // check relations condition
            	RArray<TItemId> matchedRelations;
            	CleanupClosePushL( matchedRelations );
            	TBool matches = iComparator->MatchRelationItemsL( 
            			e.Condition(), aBuffer, matchedRelations );

            	if ( matches )
	        		{
	        		if(e.IsPending())
	        			{
		            	// Match found. Trigger notifier entry.
		            	TRAPD( err, e.TriggerRelationItemsL( 
		            			ERelationItemNotifyRemove, aBuffer, 
		            			matchedRelations ) );
		            	if( err != KErrNone )
			            	{
			            	e.TriggerError( err );
		    	        	}
		        		}
		        	else
		        		{
		        		TRAP_IGNORE( e.CacheRelationItemsL( 
		        				ERelationItemNotifyRemove, aBuffer, 
		        				matchedRelations ) );
		        		}
	        		}
            	CleanupStack::PopAndDestroy( &matchedRelations );
            	}
	        }
		}

	}

