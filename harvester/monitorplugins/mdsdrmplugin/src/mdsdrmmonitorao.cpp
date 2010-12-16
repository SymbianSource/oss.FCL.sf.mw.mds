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
* Description: 
*/

#include "mdsdrmmonitorao.h"
#include "harvesterlog.h"
#include "harvestercommon.h"

#include <caf/caf.h>

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::NewL()
// ---------------------------------------------------------------------------
//
CMdsDrmMonitorAO* CMdsDrmMonitorAO::NewL()
    {
    WRITELOG( "CMdsDrmMonitorAO::NewL" );

    CMdsDrmMonitorAO* self = new (ELeave) CMdsDrmMonitorAO;
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::ConstructL()
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::ConstructL()
    {
    WRITELOG( "CMdsDrmMonitorAO::ConstructL" );
    
    CActiveScheduler::Add( this );
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::~CMdsDrmMonitorAO()
// ---------------------------------------------------------------------------
//
CMdsDrmMonitorAO::~CMdsDrmMonitorAO()
    {
    WRITELOG( "CMdsDrmMonitorAO::~CMdsDrmMonitorAO" );
    
    Cancel();
    
    iEventArray.ResetAndDestroy();
    iEventArray.Close();
    
    delete iObjectQuery;
    iObjectQuery = NULL;
    
    delete iOriginPropertyDef;
    iOriginPropertyDef = NULL;
    
    iHdArray.ResetAndDestroy();
    iHdArray.Close();
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::~CMdsDrmMonitorAO()
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::HandleQueryNewResults( CMdEQuery& /*aQuery*/,
                                              TInt /*aFirstNewItemIndex*/,
                                              TInt /*aNewItemCount*/ )
    {
    // Do nothing
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::~CMdsDrmMonitorAO()
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::HandleQueryCompleted( CMdEQuery& aQuery, TInt aError )
    {
    if( aError )
        {
        if( iEventArray.Count() )
            {
            SetNextRequest( ERequestQuery );
            }
        else
            {
            SetNextRequest( ERequestIdle );
            }
        return;
        }
    
    CMdEObjectQuery& objectQuery = (CMdEObjectQuery&)aQuery;  
    const TInt count( objectQuery.Count() );
    if( count == 0 )
        {
        WRITELOG( "CMdsDrmMonitorAO::HandleQueryCompleted - no pending items found" );
        iEventArray.ResetAndDestroy();
        SetNextRequest( ERequestIdle );
        return;
        }
    
    WRITELOG1( "CMdsDrmMonitorAO::HandleQueryCompleted - pending items found, count %d", count );
    
    for( TInt i = iEventArray.Count() - 1; i >=0; i-- )
        {
        HBufC8* contentID = iEventArray[i];
        iEventArray[i] = NULL;
        iEventArray.Remove( 0 );
        
        // Convert 8 bit data to 16 bit.
        TBufC<ContentAccess::KMaxCafUniqueId> rightsCid;
        TPtr cidPtr( rightsCid.Des() );
        cidPtr.Copy( contentID->Des() );
        
        WRITELOG1( "CMdsDrmMonitorAO::HandleQueryCompleted - checking event for content ID %S", &cidPtr );
        
        for( TInt y = count - 1; y >=0; y-- )
            {
            CMdEObject& rObject = objectQuery.Result(y);

            CMdEProperty* prop = NULL;
            TBool matches( EFalse );
            rObject.Property( *iContentIDPropertyDef, prop );
            if ( prop )
                {
                HBufC* contentIdBuf = NULL;
                TRAPD( err, contentIdBuf = prop->TextValueL().Alloc() );
#ifdef _DEBUG
                TBuf<256> debug;
                debug.Copy( contentIdBuf->Des() );
                WRITELOG1( "CMdsDrmMonitorAO::HandleQueryCompleted - checking against item content ID %S", &debug );
#endif
                if( !err && contentIdBuf->Des().CompareF( cidPtr ) == 0 )
                    {
                    matches = ETrue;
                    }
                delete contentIdBuf;
                contentIdBuf = NULL;
                }
            else
                {
                WRITELOG( "CMdsDrmMonitorAO::HandleQueryCompleted - no property found from MDE object" );
                continue;
                }

            if( matches )
                {
                HBufC* fileName = NULL;
                fileName = rObject.Uri().Alloc();
                if( !fileName )
                    {
                    continue;
                    }
                CHarvesterData* hd = NULL;
                TRAPD( err, hd = CHarvesterData::NewL( fileName ) );
                if( err )
                    {
                    delete fileName;
                    fileName = NULL;
                    continue;
                    }

                CMdEProperty* prop = NULL;
                TUint8 originVal( MdeConstants::Object::EOther );
                rObject.Property( *iOriginPropertyDef, prop );
                if ( prop )
                    {
                    TRAP( err, originVal = prop->Uint8ValueL() );
                    if( err )
                        {
                        originVal = MdeConstants::Object::EOther;
                        }
                    }

                hd->SetOrigin( (TOrigin)originVal );
                hd->SetEventType( EHarvesterEdit );
                hd->SetObjectType( EFastHarvest );
                
                if( iHdArray.Append( hd ) != KErrNone )
                    {
                    delete hd;
                    hd = NULL;
                    }                  
                }
            }
        
        delete contentID;
        contentID = NULL;
        }
    
    if( iObserver )
        {
        WRITELOG1( "CMdsDrmMonitorAO::HandleQueryCompleted - sending monitor event to harvester, item count %d", iHdArray.Count() );
        iObserver->MonitorEvent( iHdArray );
        iHdArray.Reset();
        }
    else
        {
        iHdArray.ResetAndDestroy();
        }
    
    SetNextRequest( ERequestIdle );
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::SetNextRequest
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::SetNextRequest( TRequest aRequest )
    {
    WRITELOG( "CMdsDrmMonitorAO::SetNextRequest" );
    
    iNextRequest = aRequest;
            
    if ( !IsActive() )
        {
        iStatus = KRequestPending;
        SetActive();
        TRequestStatus* ptrStatus = &iStatus;
        User::RequestComplete( ptrStatus, KErrNone );
        }    
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::Setup
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::Setup( MMonitorPluginObserver& aObserver,
                              CMdESession* aSession )
    {
    iSession = aSession;
    iObserver = &aObserver;
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::AddToQueue
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::AddToQueue( HBufC8* aContentID )
    {    
    WRITELOG( "CMdsDrmMonitorAO::AddToQueue" );
    if( iEventArray.Append( aContentID ) != KErrNone )
        {
        delete aContentID;
        aContentID = NULL;
        return;
        }
    
    if ( iNextRequest == ERequestIdle )
        {
        SetNextRequest( ERequestQuery );
        }
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::DoQueryL
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::DoQueryL()
    {
    WRITELOG( "CMdsDrmMonitorAO::DoQueryL" );
    if( !iSession )
        {
        if( iEventArray.Count() )
            {
            SetNextRequest( ERequestQuery );
            }
        else
            {
            SetNextRequest( ERequestIdle );
            }
        return;
        }
    
    // Clear the old query
    delete iObjectQuery;
    iObjectQuery = NULL;

    CMdENamespaceDef& defaultNamespaceDef = iSession->GetDefaultNamespaceDefL();
    CMdEObjectDef& mediaObjDef = defaultNamespaceDef.GetObjectDefL( 
        MdeConstants::MediaObject::KMediaObject );
    CMdEObjectDef& videoObjDef = defaultNamespaceDef.GetObjectDefL( 
        MdeConstants::Video::KVideoObject );
    CMdEObjectDef& imageObjDef = defaultNamespaceDef.GetObjectDefL( 
        MdeConstants::Image::KImageObject );

    // object defs where to search
    RPointerArray<CMdEObjectDef>* objectDefs = 
        new (ELeave) RPointerArray<CMdEObjectDef>( 2 );
    objectDefs->Append( &videoObjDef );
    objectDefs->Append( &imageObjDef );

    // query media object properties from image and video objects
    iObjectQuery = iSession->NewObjectQueryL( mediaObjDef, objectDefs, this );

    CMdELogicCondition& rootCond = iObjectQuery->Conditions();

    CMdEPropertyDef& rightsPropDef = mediaObjDef.GetPropertyDefL( 
        MdeConstants::MediaObject::KRightsStatus );

    // Must be pending for drm rights
    rootCond.AddPropertyConditionL( rightsPropDef, TMdEUintEqual( MdeConstants::MediaObject::ENoRights ) );
    
    CMdEPropertyDef& contentIDPropDef = mediaObjDef.GetPropertyDefL( 
        MdeConstants::MediaObject::KContentID );
    
    // property filters
    if( !iOriginPropertyDef )
        {
        CMdEObjectDef& objDef = defaultNamespaceDef.GetObjectDefL( MdeConstants::Object::KBaseObject );
        iOriginPropertyDef = &objDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
        }
    
    // property filters
    if( !iContentIDPropertyDef )
        {
        CMdEObjectDef& objDef = defaultNamespaceDef.GetObjectDefL( MdeConstants::MediaObject::KMediaObject );
        iContentIDPropertyDef = &objDef.GetPropertyDefL( MdeConstants::MediaObject::KContentID );
        }
    
    iObjectQuery->AddPropertyFilterL( iOriginPropertyDef );
    iObjectQuery->AddPropertyFilterL( iContentIDPropertyDef );
    
    iObjectQuery->FindL();
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::RunL()
// From CActive
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::RunL()
    {
    WRITELOG( "CMdsDrmMonitorAO::RunL" );

    switch( iNextRequest )
        {
        case ERequestIdle:
            {
            iHdArray.Compress();
            iEventArray.Compress();
            break;
            }

        case ERequestQuery:
            {
            DoQueryL();
            break;
            }
        }
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::RunError()
// From CActive
// ---------------------------------------------------------------------------
//
#ifdef _DEBUG
TInt CMdsDrmMonitorAO::RunError( TInt aError )
#else
TInt CMdsDrmMonitorAO::RunError( TInt /*aError*/ )
#endif
    {
#ifdef _DEBUG
    WRITELOG1( "CMdsDrmMonitorAO::RunError %d", aError );
#endif
    if( iNextRequest == ERequestQuery && iEventArray.Count())
        {
        SetNextRequest( ERequestQuery );
        }
    else
        {
        SetNextRequest( ERequestIdle );
        }

    return KErrNone;
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::DoCancel()
// From CActive
// ---------------------------------------------------------------------------
//
void CMdsDrmMonitorAO::DoCancel()
    {
    WRITELOG( "CMdsDrmMonitorAO::DoCancel" );
    if( iObjectQuery )
        {
        iObjectQuery->Cancel();
        }
    }

// ---------------------------------------------------------------------------
// CMdsDrmMonitorAO::CMdsDrmMonitorAO()
// Constructor
// ---------------------------------------------------------------------------
//
CMdsDrmMonitorAO::CMdsDrmMonitorAO() : CActive( KHarvesterPriorityMonitorPlugin ), 
                                       iObserver( NULL ),
                                       iSession( NULL ),
                                       iNextRequest( ERequestIdle ),
                                       iObjectQuery( NULL ),
                                       iOriginPropertyDef( NULL )
    {
    }

