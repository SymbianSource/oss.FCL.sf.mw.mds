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
* Description:  A class for recording and storing locations.
*
*/

#include <e32cmn.h> 
#include <lbserrors.h>
#include <lbssatellite.h>

#include "rlocationtrail.h"
#include "clocationrecord.h"
#include "cnetworkinfo.h"
#include "locationmanagerdebug.h"
#include "locationtraildefs.h"
#include "locationtrailpskeys.h"
#include "mdeconstants.h"
#include <centralrepository.h>


using namespace MdeConstants;

// --------------------------------------------------------------------------
// CLocationRecord::NewL
// --------------------------------------------------------------------------
//
EXPORT_C CLocationRecord* CLocationRecord::NewL()
    {
    CLocationRecord* self = new (ELeave) CLocationRecord();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }
        
// --------------------------------------------------------------------------
// CLocationRecord::CLocationRecord
// --------------------------------------------------------------------------
//  
CLocationRecord::CLocationRecord()
    : iNetworkInfoTimer( NULL ),
    iState( RLocationTrail::ETrailStopped ),
    iTrailCaptureSetting( RLocationTrail::ECaptureAll ),
    iLocationCounter( 0 ),
    iRequestCurrentLoc( EFalse ),
    iTrailStarted( EFalse ),
    iLastGPSFixState( EFalse ),
    iLastLocationId( 0 )
    {
    iMaxTrailSize = KMaxTrailLength / KUpdateInterval;
    }

// --------------------------------------------------------------------------
// CLocationRecord::ConstructL
// --------------------------------------------------------------------------
//    
void CLocationRecord::ConstructL()
    {
    const TInt KMillion = 1000000;
    TInt err = iProperty.Define( KPSUidLocationTrail, KLocationTrailState, RProperty::EInt );
    if ( err != KErrNone && err != KErrAlreadyExists )
        {
        User::Leave( err );
        }
    User::LeaveIfError( iProperty.Set( KPSUidLocationTrail,
        KLocationTrailState, (TInt) RLocationTrail::ETrailStopped ) ); 

    iNetworkInfo = CNetworkInfo::NewL( this );
    iPositionInfo = CPositionInfo::NewL( this );
	iRemapper = CLocationRemappingAO::NewL();
    iNetworkInfoTimer = CPeriodic::NewL( CActive::EPriorityStandard );
    
    TInt interval( 0 );
    TRAP(err, ReadCenRepValueL(KIntervalKey, interval));
    LOG1("CLocationManagerServer::ConstructL, cenrep interval value:%d", interval);
    
    if (interval == 0 || err != KErrNone )
    	{
        LOG1("CLocationManagerServer::ConstructL, cenrep interval err:%d", err);
    	iInterval = KUpdateInterval;
    	}
    else 
    	{
    	iInterval = interval * KMillion;
    	}

    TRAP(err, ReadCenRepValueL(KLocationDeltaKey, iLocationDelta));
    LOG1("CLocationManagerServer::ConstructL, location delta value:%d", iLocationDelta);
    
    if (iLocationDelta == 0)
    	{
        LOG1("CLocationManagerServer::ConstructL, location delta err:%d", err);
        iLocationDelta = KLocationDelta;
    	}

    }
    
// --------------------------------------------------------------------------
// CLocationRecord::~CLocationRecord
// --------------------------------------------------------------------------
//    
EXPORT_C CLocationRecord::~CLocationRecord()
    {
    Stop();
    iProperty.Delete( KPSUidLocationTrail, KLocationTrailState );
    iProperty.Close();
    iTrail.Close();
    
    delete iNetworkInfo;
    delete iPositionInfo;
    delete iNetworkInfoTimer;
	if (iRemapper)
		{
		iRemapper->StopRemapping();
		delete iRemapper;
		}
    }

// --------------------------------------------------------------------------
// CLocationRecord::CurrentState
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::LocationTrailState( TLocTrailState& aState )
    {
    aState = iState;
    }

// --------------------------------------------------------------------------
// CLocationRecord::StartL
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::StartL( RLocationTrail::TTrailCaptureSetting aCaptureSetting )
    {
    LOG( "CLocationRecord::StartL(), begin" );
    iTrailCaptureSetting = aCaptureSetting;
    if ( aCaptureSetting == RLocationTrail::ECaptureAll && !iPositionInfo->IsActive() )
        {
        if( iState == RLocationTrail::ETrailStopped  )
            {
            iTrail.Reset();
            }
        iPositionInfo->StartL( aCaptureSetting, iInterval );
        }
    else if ( aCaptureSetting == RLocationTrail::ECaptureNetworkInfo )
    	{
    	// Update and store network info in location trail immediately.
    	// Timer will trigger the update again later.
    	UpdateNetworkInfo( this );
    	
        if ( iNetworkInfoTimer && iNetworkInfoTimer->IsActive() )
        	{
        	iNetworkInfoTimer->Cancel();
        	}
        	
        StartTimerL();
    	}
    
    iLastLocationId = 0;
    
    SetCurrentState( RLocationTrail::ETrailStarting );
    
    iTrailStarted = ETrue;
    LOG( "CLocationRecord::StartL(), end" );
    }

// --------------------------------------------------------------------------
// CLocationRecord::Stop
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::Stop()
    {
    LOG( "CLocationRecord::StopL(), begin" );
    iPositionInfo->Stop();
    iTrailStarted = EFalse;
    
    if ( iNetworkInfoTimer && iNetworkInfoTimer->IsActive() )
    	{
    	iNetworkInfoTimer->Cancel();
    	}

    if ( iRemapper )
    	{
    	iRemapper->ResetQueue();
    	}
    SetCurrentState( RLocationTrail::ETrailStopped );
    LOG( "CLocationRecord::StopL(), end" );
    }

// --------------------------------------------------------------------------
// CLocationRecord::SetStateToStop
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::SetStateToStopping()
	{
	SetCurrentState( RLocationTrail::ETrailStopping );
	}

// --------------------------------------------------------------------------
// CLocationRecord::GetLocationByTimeL
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::GetLocationByTimeL( const TTime aTime, 
												   TLocationData& aLocationData,
                                                   TLocTrailState& aState ) 
    {
    LOG( "CLocationRecord::GetLocationByTimeL(), begin" );
    TInt posFound( EFalse );

#ifdef _DEBUG
    _LIT( KDateTimeFormat, "%Y/%M/%D %H:%T:%S" );
    const TInt DateTimeStrMaxLength = 20;
    LOG1( "CLocationRecord::GetLocationByTimeL - aTime: %Ld", aTime.Int64() );
    TBuf<DateTimeStrMaxLength> str1;
    aTime.FormatL( str1, KDateTimeFormat );
    LOG1( "CLocationRecord::GetLocationByTimeL - aTime: %S", &str1 );
#endif

    TTimeIntervalSeconds interval;
    TTimeIntervalSeconds nextInterval;
    for ( TInt i(iTrail.Count()-1) ; i >= 0 && !posFound ; i-- )
        {
        TInt err = iTrail[i].iTimeStamp.SecondsFrom( aTime, interval );
        
        TInt timeDiff = Abs( interval.Int() );

#ifdef _DEBUG
        LOG1( "CLocationRecord::GetLocationByTimeL - Trail timestamp: %Ld", iTrail[i].iTimeStamp.Int64() );
        TBuf<DateTimeStrMaxLength> str;
        iTrail[i].iTimeStamp.FormatL( str, KDateTimeFormat );
        LOG1( "CLocationRecord::GetLocationByTimeL - Trail timestamp: %S", &str );
        LOG1( "CLocationRecord::GetLocationByTimeL - timeDiff: %d", timeDiff );
#endif

        if ( err == KErrNone && timeDiff <= KMaximumIntervalSeconds )
            {
            // The nearest time is in iTrail[i] or in iTrail[i-1].
            if ( i > 0 )
                {
                iTrail[i-1].iTimeStamp.SecondsFrom( aTime, nextInterval );
                
                TInt nextDiff = Abs( nextInterval.Int() );
                    
                if ( nextDiff < timeDiff )
                    {
                    aLocationData = iTrail[i-1].iLocationData;
                    aState = iTrail[i-1].iTrailState;
                    }
                else
                    {
                    aLocationData = iTrail[i].iLocationData;
                    aState = iTrail[i].iTrailState;
                    }                    
                }            
            else
                {
                aLocationData = iTrail[i].iLocationData;
                aState = iTrail[i].iTrailState;
                }
            posFound = ETrue;
            }
        }
    if ( !posFound )
        {
        User::Leave( KErrNotFound );
        }
    LOG( "CLocationRecord::GetLocationByTimeL(), end" );
    }
    
// --------------------------------------------------------------------------
// CLocationRecord::RequestLocationL
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::RequestLocationL()
    {
    iRequestCurrentLoc = ETrue;
    if ( iTrailCaptureSetting != RLocationTrail::ECaptureNetworkInfo &&
    	!iPositionInfo->IsActive() )
        {
        iPositionInfo->StartL( iTrailCaptureSetting, iInterval );
        }
    else if ( iTrailCaptureSetting == RLocationTrail::ECaptureNetworkInfo )
    	{
    	TPositionSatelliteInfo posInfo;
    	CTelephony::TNetworkInfoV1 network = CTelephony::TNetworkInfoV1();
    	GetNetworkInfo( network );
       	iObserver->CurrentLocation( posInfo, network, KErrNone );
        iRequestCurrentLoc = EFalse;
    	}
    }

// --------------------------------------------------------------------------
// CLocationRecord::CancelLocationRequest
// --------------------------------------------------------------------------
//    
EXPORT_C void CLocationRecord::CancelLocationRequest()
    {
    iRequestCurrentLoc = EFalse;
    if ( !iTrailStarted )
        {
        iPositionInfo->Stop();
        }
    }
        
        
// --------------------------------------------------------------------------
// CLocationRecord::GetNetworkInfo
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::GetNetworkInfo( CTelephony::TNetworkInfoV1& aNetworkInfo ) 
    {
    LOG("CLocationRecord::GetNetworkInfo");

    aNetworkInfo = iNetwork;
    }
    
// --------------------------------------------------------------------------
// CLocationRecord::SetObserver
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::SetObserver( MLocationTrailObserver* aObserver) 
    {
    iObserver = aObserver;
    }

// --------------------------------------------------------------------------
// CLocationRecord::SetAddObserver
// --------------------------------------------------------------------------
//
EXPORT_C void CLocationRecord::SetAddObserver( MLocationAddObserver* aObserver)
    {
    iAddObserver = aObserver;
    }

// --------------------------------------------------------------------------
// From MNetworkInfoObserver.
// CLocationRecord::Position
// --------------------------------------------------------------------------
//    
void CLocationRecord::Position( const TPositionInfo& aPositionInfo,
                                const TInt aError  )
    {    
    const TPositionSatelliteInfo& positionSatelliteInfo = 
    	static_cast<const TPositionSatelliteInfo&>(aPositionInfo);

    if ( iRequestCurrentLoc )
        {
        HandleLocationRequest( positionSatelliteInfo, aError );
        }
    if( iState == RLocationTrail::ETrailStopped )
    	{
    	LOG("CLocationRecord::Position - trail stopped");
    	return;
    	}
    
    if ( !iTrailStarted )
        {
        iPositionInfo->NextPosition();
        return;
        }
    switch ( aError )
        {
        case KPositionPartialUpdate: // fall through
        case KPositionQualityLoss: 
            {
            // Location is stored, even if it may not be valid.
            StoreLocation( positionSatelliteInfo ); 
            LOG("CLocationRecord::Position - partial update");
            if ( iState != RLocationTrail::EWaitingGPSData && 
            	 iState != RLocationTrail::ETrailStopping ) 
                {
                SetCurrentState( RLocationTrail::EWaitingGPSData );
            	LOG("CLocationRecord::Position trail waiting for gps");
                }
            break;
            }
        case KErrNone:
            {
            StoreLocation( positionSatelliteInfo );
            LOG("CLocationRecord::Position - good GPS coordinates");
            if ( iState != RLocationTrail::ETrailStarted ) 
                {
                if ( iRemapper )
                	{
                	LOG("CLocationRecord::Position start remapping");
                	iLastLocationId = 0;
                	TBool createLocation = iRemapper->CheckQueue();
                	if( createLocation )
                		{
                		TRAP_IGNORE(	
                		TItemId locationId = DoCreateLocationL( iNewItem.iLocationData );
                		iRemapper->UpdateRelationsL( locationId );
                		)
                		}
               		iRemapper->StartRemappingObjects( iNewItem.iLocationData );

                    if( iObserver->WaitForPositioningStopTimeout() && !RemappingNeeded() )                                
               		    {                                                
                        iObserver->RemapedCompleted();
                        return;
               		    }
               		
                	}
                if ( iState != RLocationTrail::ETrailStopping )
                	{
                    SetCurrentState( RLocationTrail::ETrailStarted );
                	LOG("CLocationRecord::Position trail started");
                	}
                }
            break;
            }
        default:
            {
            StoreLocation( positionSatelliteInfo );
            LOG1("CLocationRecord::Position - searching GPS, aError %d", aError );
            if ( iState != RLocationTrail::ESearchingGPS &&
               	 iState != RLocationTrail::ETrailStopping ) 
                {
                SetCurrentState( RLocationTrail::ESearchingGPS );
            	LOG("CLocationRecord::Position trail searching gps");
                }
            break;
            }      
        }
    TBool fixState = CheckGPSFix( positionSatelliteInfo );
    LOG1( "CLocationRecord::Position fixState %d", fixState );
    LOG1( "CLocationRecord::Position iLastGPSFixState %d", iLastGPSFixState );
    
    if ( iObserver && iLastGPSFixState != fixState )
    	{
    	LOG("CLocationRecord::Position quality changed");
    	iObserver->GPSSignalQualityChanged( positionSatelliteInfo );
    	}
    
   	iLastGPSFixState = fixState;
    
    iPositionInfo->NextPosition();
    }

TBool CLocationRecord::CheckGPSFix( const TPositionSatelliteInfo& aSatelliteInfo )
	{
	TPosition position;
	aSatelliteInfo.GetPosition( position );
	LOG1( "CLocationRecord::CheckGPSFix latitude %f", position.Latitude() );
	LOG1( "CLocationRecord::CheckGPSFix longitude %f", position.Longitude() );
	TBool ret = ( Math::IsNaN(position.Latitude()) || Math::IsNaN(position.Longitude()) ) 
		? EFalse : ETrue;
   	return ret;
	}
    
// --------------------------------------------------------------------------
// From MPositionerObserver.
// CLocationRecord::NetworkInfo
// --------------------------------------------------------------------------
//    
void CLocationRecord::NetworkInfo( const CTelephony::TNetworkInfoV1 &aNetworkInfo, 
		const TInt aError )
    {
    LOG("CLocationRecord::NetworkInfo");
    if ( aError == KErrNone )
        {
        LOG("CLocationRecord::NetworkInfo - KErrNone");
        iNetwork = aNetworkInfo;
        if (iNetwork.iAccess == CTelephony::ENetworkAccessUtran)
        	{
        	iNetwork.iLocationAreaCode = 0;
        	}
        if ( iState == RLocationTrail::ETrailStarting && iTrailStarted )
        	{
        	SetCurrentState( RLocationTrail::ETrailStarted );
        	}
        }
    else
        {
        LOG1("CLocationRecord::NetworkInfo - %d", aError );
        iNetwork = CTelephony::TNetworkInfoV1();
        iNetwork.iAreaKnown = EFalse;
        iNetwork.iAccess = CTelephony::ENetworkAccessUnknown;
        iNetwork.iCellId = 0;
        iNetwork.iLocationAreaCode = 0;
        iNetwork.iCountryCode.Zero();
        iNetwork.iNetworkId.Zero();
        }
    }

// --------------------------------------------------------------------------
// CLocationRecord::StoreLocationL
// --------------------------------------------------------------------------
//    
void CLocationRecord::StoreLocation( const TPositionSatelliteInfo& aSatelliteInfo )
    {
    aSatelliteInfo.GetPosition( iNewItem.iLocationData.iPosition );
    aSatelliteInfo.GetCourse( iNewItem.iLocationData.iCourse );
    iNewItem.iLocationData.iSatellites = aSatelliteInfo.NumSatellitesUsed();
    iNewItem.iLocationData.iQuality = aSatelliteInfo.HorizontalDoP();
    
    // Network info
    GetNetworkInfo( iNewItem.iLocationData.iNetworkInfo );
    // Get Universal time
    iNewItem.iTimeStamp.UniversalTime();
    iNewItem.iTrailState = iState;
    
    TInt error = iTrail.Append( iNewItem );
    
    // If appending an item to the trail fails because of OOM, remove oldest trail items
    // until the new item fits or there's only one item left in the trail.
    while ( error == KErrNoMemory && iTrail.Count() > 1 )
		{
		LOG("CLocationRecord::StoreLocation - Out of memory! Shortening trail!");
		iTrail.Remove( 0 );
		error = iTrail.Append( iNewItem );
		}
    
    if ( iTrail.Count() > iMaxTrailSize )
        {
        iTrail.Remove( 0 );
        }
    
    if( iAddObserver )
    	{
    	iAddObserver->LocationAdded( iNewItem, aSatelliteInfo );
    	}
    }
    
// --------------------------------------------------------------------------
// CLocationRecord::SetCurrentState
// --------------------------------------------------------------------------
//        
void CLocationRecord::SetCurrentState( TLocTrailState aState )    
    {
    LOG1( "CLocationRecord::SetCurrentState(), begin, state:%d", aState );
    iState = aState;
    iProperty.Set( KPSUidLocationTrail, KLocationTrailState, (TInt) aState );
    if ( iObserver )
        {
        iObserver->LocationTrailStateChange();
        }
    LOG( "CLocationRecord::SetCurrentState(), end" );
    }
    
// --------------------------------------------------------------------------
// CLocationRecord::HandleLocationRequest
// --------------------------------------------------------------------------
//
void CLocationRecord::HandleLocationRequest( const TPositionSatelliteInfo& aSatelliteInfo, 
                                             const TInt aError )    
    {
	CTelephony::TNetworkInfoV1 network = CTelephony::TNetworkInfoV1();
    if ( aError == KErrNone )
        {
       	GetNetworkInfo( network );
        iObserver->CurrentLocation( aSatelliteInfo, network, aError );
        iRequestCurrentLoc = EFalse;
        if ( !iTrailStarted )
            {
            iPositionInfo->Stop();
            }
        }
    else
        {
        iLocationCounter++;
        if ( iLocationCounter > KCurrentLocTimeoutCount )
            {
            iObserver->CurrentLocation( aSatelliteInfo, network, KErrTimedOut );
            iRequestCurrentLoc = EFalse;
            iLocationCounter = 0;
            if ( !iTrailStarted )
                {
                iPositionInfo->Stop();
                }
            }       
        }    
    }

TInt CLocationRecord::UpdateNetworkInfo( TAny* aAny )
	{
	TPositionSatelliteInfo nullPositionInfo;
	CLocationRecord* self = STATIC_CAST( CLocationRecord*, aAny );
	self->StoreLocation( nullPositionInfo );
	return KErrNone;
	}


EXPORT_C void CLocationRecord::CreateLocationObjectL( const TLocationData& aLocationData,
		const TUint& aObjectId )
	{
	TItemId locationId = DoCreateLocationL( aLocationData );
	CreateRelationL( aObjectId, locationId );
	}


EXPORT_C void CLocationRecord::LocationSnapshotL( const TUint& aObjectId )
	{
	LOG("CLocationRecord::LocationSnapshotL");
	
	TBool previousMatch = EFalse;
	CMdENamespaceDef& namespaceDef = iMdeSession->GetDefaultNamespaceDefL();

	// get locationdata from trail with object time
	TTime timestamp = GetMdeObjectTimeL( aObjectId );
	TLocationData locationData;
	TLocTrailState state;
	GetLocationByTimeL( timestamp, locationData, state );
	
	iObjectId = aObjectId;
	iLocationData = locationData;

	// capture only network data
	if ( iTrailCaptureSetting == RLocationTrail::ECaptureNetworkInfo )
		{
		CTelephony::TNetworkInfoV1* net = &locationData.iNetworkInfo;
		
		if ( net->iCellId == 0 && 
				net->iLocationAreaCode == 0 &&
				net->iCountryCode.Length() == 0 &&
				net->iNetworkId.Length() == 0 )
			{
			// nothing to do
			LOG("CLocationRecord::LocationSnapshotL - no network info available");
			}
		else if ( iLastLocationId != 0 )
			{
			CTelephony::TNetworkInfoV1* lastnet = &iLastLocation.iNetworkInfo;
			
			// compare to previous network info
			TItemId locationId = iLastLocationId;
			if ( lastnet->iCellId != net->iCellId ||
					lastnet->iLocationAreaCode != net->iLocationAreaCode ||
					lastnet->iCountryCode != net->iCountryCode ||
					lastnet->iNetworkId != net->iNetworkId )
				{
				LOG("CLocationRecord::LocationSnapshotL - network info changed");
				locationId = DoCreateLocationL( locationData );
				}
			CreateRelationL( aObjectId, locationId );
			}
		else 
			{
			// new location
			TItemId locationId = DoCreateLocationL( locationData );
			CreateRelationL( aObjectId, locationId );
			}
		return;
		}
	
	// coordinates empty (will be remapped)
	if ( Math::IsNaN( locationData.iPosition.Latitude() ) && 
			Math::IsNaN( locationData.iPosition.Longitude() ))
		{
		TRemapItem remapItem;
		remapItem.iObjectId = aObjectId;
		remapItem.iTime = timestamp;
		
		CTelephony::TNetworkInfoV1* net = &locationData.iNetworkInfo;

		// no network info (offline mode + no GPS fix)
		if ( net->iCellId == 0 && 
				net->iLocationAreaCode == 0 &&
				net->iCountryCode.Length() == 0 &&
				net->iNetworkId.Length() == 0 )
			{
			LOG("CLocationRecord::LocationSnapshotL - empty remap item created");
			}
		// check match for last created locationobject
		else if ( iLastLocationId != 0 )
			{
			TItemId locationId;
			CTelephony::TNetworkInfoV1* lastnet = &iLastLocation.iNetworkInfo;

			// networkinfo changed from last location
			if ( lastnet->iCellId != net->iCellId ||
					lastnet->iLocationAreaCode != net->iLocationAreaCode ||
					lastnet->iCountryCode != net->iCountryCode ||
					lastnet->iNetworkId != net->iNetworkId )
				{
				LOG("CLocationRecord::LocationSnapshotL - remap with new network info");
				locationId = DoCreateLocationL( locationData );
				}		
			else
				{
				LOG("CLocationRecord::LocationSnapshotL - remap with previous network info");
				locationId = iLastLocationId;
				}
			TItemId relationId = CreateRelationL( aObjectId, locationId );
			remapItem.iLocationId = locationId;
			remapItem.iRelationId = relationId;
			}
		else
			{
			// new location with only network data
			TItemId locationId = DoCreateLocationL( locationData );
			TItemId relationId = CreateRelationL( aObjectId, locationId );
			remapItem.iLocationId = locationId;
			remapItem.iRelationId = relationId;
			}
		iRemapper->Append( remapItem );
		return;
		}
		
	// valid coordinates found
	if ( iLastLocationId != 0 )
		{
		CTelephony::TNetworkInfoV1* net = &locationData.iNetworkInfo;
		CTelephony::TNetworkInfoV1* lastnet = &iLastLocation.iNetworkInfo;
		
		// first check if networkinfo matches last created location
		if ( lastnet->iCellId == net->iCellId &&
				lastnet->iLocationAreaCode == net->iLocationAreaCode &&
				lastnet->iCountryCode == net->iCountryCode &&
				lastnet->iNetworkId == net->iNetworkId )
			{
			LOG("CLocationRecord::LocationSnapshotL - network info matches");
			
			// if both locations have valid coordinates, calculate distance between points
			if ( !Math::IsNaN( iLastLocation.iPosition.Latitude() ) && 
					!Math::IsNaN( iLastLocation.iPosition.Longitude() ) && 
					!Math::IsNaN( locationData.iPosition.Latitude() ) && 
					!Math::IsNaN( locationData.iPosition.Longitude() ))
				{
				TReal32 distance;
				TInt err = locationData.iPosition.Distance(iLastLocation.iPosition, distance);
				
				if ( distance < iLocationDelta )
					{
					LOG("CLocationRecord::LocationSnapshotL - location close to the previous one");
					previousMatch = ETrue;
					CreateRelationL( aObjectId, iLastLocationId );
					LOG("CLocationRecord::CreateLocationObjectL - last location matched");
					}
				}
			}
		}
	
	// last location did not match, find existing one from DB
	if( !previousMatch )
		{
		LOG("CLocationRecord::LocationSnapshotL - query location");
		const TReal64 KMeterInDegrees = 0.000009;
		const TReal64 KPi = 3.14159265358979;
		const TReal32 K180Degrees = 180.0;
	
		TReal64 latitude = locationData.iPosition.Latitude();
		TReal64 longitude = locationData.iPosition.Longitude();
		// calculate distance in degrees
		TReal64 cosine;
		Math::Cos(cosine, locationData.iPosition.Latitude() * KPi / K180Degrees );
		TReal64 latDelta = iLocationDelta * KMeterInDegrees;
		TReal64 lonDelta = latDelta * cosine;
		
		CMdEObjectDef& locationObjectDef = namespaceDef.GetObjectDefL( Location::KLocationObject );
		
		CMdEPropertyDef& latitudeDef = locationObjectDef.GetPropertyDefL(
				Location::KLatitudeProperty );
		CMdEPropertyDef& longitudeDef = locationObjectDef.GetPropertyDefL(
				Location::KLongitudeProperty );
		CMdEPropertyDef& cellIdDef = locationObjectDef.GetPropertyDefL(
				Location::KCellIdProperty );
		CMdEPropertyDef& locationCodeDef = locationObjectDef.GetPropertyDefL( 
				Location::KLocationAreaCodeProperty );
		CMdEPropertyDef& countryCodeDef = locationObjectDef.GetPropertyDefL( 
				Location::KCountryCodeProperty );
		CMdEPropertyDef& networkCodeDef = locationObjectDef.GetPropertyDefL( 
				Location::KNetworkCodeProperty );
		
		iLocationQuery = iMdeSession->NewObjectQueryL( namespaceDef, locationObjectDef, this );
		CMdELogicCondition& cond = iLocationQuery->Conditions();
		cond.SetOperator( ELogicConditionOperatorAnd );
		
		LOG1( "CLocationRecord::LocationSnapshotL latitude: %f", latitude);
		LOG1( "CLocationRecord::LocationSnapshotL latdelta: %f", latDelta);
		LOG1( "CLocationRecord::LocationSnapshotL longitude: %f", longitude);
		LOG1( "CLocationRecord::LocationSnapshotL londelta: %f", lonDelta);
		
		cond.AddPropertyConditionL( latitudeDef, 
				TMdERealBetween( latitude - latDelta, latitude + latDelta ));
		cond.AddPropertyConditionL( longitudeDef, 
				TMdERealBetween( longitude - lonDelta, longitude + lonDelta ));
		cond.AddPropertyConditionL( cellIdDef, 
				TMdEUintEqual( locationData.iNetworkInfo.iCellId) );
		cond.AddPropertyConditionL( locationCodeDef, 
				TMdEUintEqual( locationData.iNetworkInfo.iLocationAreaCode) );
		cond.AddPropertyConditionL( countryCodeDef, ETextPropertyConditionCompareEquals,
				locationData.iNetworkInfo.iCountryCode );
		cond.AddPropertyConditionL( networkCodeDef, ETextPropertyConditionCompareEquals,
				locationData.iNetworkInfo.iNetworkId );
		
		iLocationQuery->FindL();			
		}
	}

	
TItemId CLocationRecord::DoCreateLocationL( const TLocationData& aLocationData ) 
	{
	LOG("CLocationRecord::DoCreateLocationL - start");
	TItemId locationObjectId;
	
	CMdENamespaceDef& namespaceDef = iMdeSession->GetDefaultNamespaceDefL();

	CMdEObjectDef& locationObjectDef = namespaceDef.GetObjectDefL( Location::KLocationObject );
	
	// required object properties
	CMdEPropertyDef& creationDef = locationObjectDef.GetPropertyDefL(
			Object::KCreationDateProperty );
	CMdEPropertyDef& modifiedDef = locationObjectDef.GetPropertyDefL(
			Object::KLastModifiedDateProperty );
	CMdEPropertyDef& sizeDef = locationObjectDef.GetPropertyDefL(
			Object::KSizeProperty );
	CMdEPropertyDef& itemTypeDef = locationObjectDef.GetPropertyDefL(
			Object::KItemTypeProperty );
	CMdEPropertyDef& offSetDef = locationObjectDef.GetPropertyDefL( 
			Object::KTimeOffsetProperty );

	// location related properties
	CMdEPropertyDef& cellIdDef = locationObjectDef.GetPropertyDefL(
			Location::KCellIdProperty );
	CMdEPropertyDef& latitudeDef = locationObjectDef.GetPropertyDefL(
			Location::KLatitudeProperty );
	CMdEPropertyDef& longitudeDef = locationObjectDef.GetPropertyDefL(
			Location::KLongitudeProperty );
	CMdEPropertyDef& altitudeDef = locationObjectDef.GetPropertyDefL(
			Location::KAltitudeProperty );

	CMdEPropertyDef& directionDef = locationObjectDef.GetPropertyDefL(
			Location::KDirectionProperty );
	CMdEPropertyDef& speedDef = locationObjectDef.GetPropertyDefL( 
			Location::KSpeedProperty );
	CMdEPropertyDef& locationCodeDef = locationObjectDef.GetPropertyDefL( 
			Location::KLocationAreaCodeProperty );
	CMdEPropertyDef& countryCodeDef = locationObjectDef.GetPropertyDefL( 
			Location::KCountryCodeProperty );
	CMdEPropertyDef& networkCodeDef = locationObjectDef.GetPropertyDefL( 
			Location::KNetworkCodeProperty );
	CMdEPropertyDef& qualityDef = locationObjectDef.GetPropertyDefL( 
			Location::KQualityProperty );

	// location object
	CMdEObject* locationObject = NULL;

	locationObject = iMdeSession->NewObjectL( locationObjectDef, Object::KAutomaticUri );
	CleanupStack::PushL( locationObject );

	TTime timestamp( 0 );
	timestamp.UniversalTime();

	TTimeIntervalSeconds timeOffset = User::UTCOffset();
	TTime localTime = timestamp + timeOffset;
	
	// required object properties
	locationObject->AddTimePropertyL( creationDef, localTime );
	locationObject->AddTimePropertyL( modifiedDef, timestamp );
	locationObject->AddUint32PropertyL( sizeDef, 0 ); // always zero size for location objects
	locationObject->AddTextPropertyL( itemTypeDef, Location::KLocationItemType );
	locationObject->AddInt16PropertyL( offSetDef, timeOffset.Int() / 60 );
	
	LOG1( "CLocationRecord::DoCreateLocationL - location created with stamp: %Ld", timestamp.Int64() );
	
	// location related properties
	if ( !Math::IsNaN( aLocationData.iPosition.Latitude() ) && 
		 !Math::IsNaN( aLocationData.iPosition.Longitude() ))
		{
		locationObject->AddReal64PropertyL( latitudeDef, aLocationData.iPosition.Latitude() );
		locationObject->AddReal64PropertyL( longitudeDef, aLocationData.iPosition.Longitude() );
		}
	if ( !Math::IsNaN( aLocationData.iPosition.Altitude() ) )
		{
		locationObject->AddReal64PropertyL( altitudeDef, aLocationData.iPosition.Altitude() );
		}
	if ( !Math::IsNaN( aLocationData.iCourse.Course() ) )
		{
		locationObject->AddReal32PropertyL( directionDef, aLocationData.iCourse.Course() );
		}
	if ( !Math::IsNaN( aLocationData.iCourse.Speed() ) )
		{
		locationObject->AddReal32PropertyL( speedDef, aLocationData.iCourse.Speed() );
		}
	if ( !Math::IsNaN( aLocationData.iQuality ) )
		{
		locationObject->AddReal32PropertyL( qualityDef, aLocationData.iQuality );
		}

	// network related properties
	if ( aLocationData.iNetworkInfo.iAreaKnown )
		{
		if ( aLocationData.iNetworkInfo.iAccess != CTelephony::ENetworkAccessUnknown )
			{
			locationObject->AddUint32PropertyL( cellIdDef, aLocationData.iNetworkInfo.iCellId );
			
			}
		if ( aLocationData.iNetworkInfo.iLocationAreaCode != 0 &&
			aLocationData.iNetworkInfo.iAccess != CTelephony::ENetworkAccessUnknown )
			{
			locationObject->AddUint32PropertyL( locationCodeDef, 
					aLocationData.iNetworkInfo.iLocationAreaCode );
			
			}
		if ( aLocationData.iNetworkInfo.iCountryCode.Length() > 0 )
			{
			locationObject->AddTextPropertyL( countryCodeDef, 
					aLocationData.iNetworkInfo.iCountryCode );
			
			}
		if ( aLocationData.iNetworkInfo.iNetworkId.Length() > 0 )
			{
			locationObject->AddTextPropertyL(networkCodeDef, aLocationData.iNetworkInfo.iNetworkId);
			
			}
		}

	// Add the location object to the database.
	locationObjectId = iMdeSession->AddObjectL( *locationObject );

	iLastLocationId = locationObjectId;
	iLastLocation = aLocationData;

	CleanupStack::PopAndDestroy( locationObject );
	
	LOG("CLocationRecord::DoCreateLocationL - end");
	
	return locationObjectId;
	}


TItemId CLocationRecord::CreateRelationL( const TUint& aObjectId, const TUint& aLocationId )
	{ 
	LOG("CLocationRecord::CreateRelationL - start");
	CMdENamespaceDef& namespaceDef = iMdeSession->GetDefaultNamespaceDefL();
	
	// "contains" relation definition
	CMdERelationDef& containsRelDef = namespaceDef.GetRelationDefL( 
			Relations::KContainsLocation );

	CMdERelation* relationObject = iMdeSession->NewRelationLC( containsRelDef,
			aObjectId, aLocationId, 0 );
	if ( !relationObject )
		{
		User::Leave( KErrBadHandle );
		}
	TItemId relationId = iMdeSession->AddRelationL( *relationObject );

	CleanupStack::PopAndDestroy( relationObject );
	LOG("CLocationRecord::CreateRelationL - end");
	
	return relationId; 
	}

// --------------------------------------------------------------------------
// CLocationManagerServer::ReadCenRepValueL
// --------------------------------------------------------------------------
//
void CLocationRecord::ReadCenRepValueL(TInt aKey, TInt& aValue)
	{
	LOG( "CLocationRecord::::ReadCenRepValueL(), begin" );
	CRepository* repository;
	repository = CRepository::NewLC( KRepositoryUid );
	User::LeaveIfError(repository->Get( aKey, aValue));
	CleanupStack::PopAndDestroy(repository);
    LOG( "CLocationRecord::::ReadCenRepValueL(), end" );   
	}

void CLocationRecord::HandleQueryNewResults(CMdEQuery& /*aQuery*/, TInt /*aFirstNewItemIndex*/, 
		TInt /*aNewItemCount*/)
	{
	}

void CLocationRecord::HandleQueryCompleted(CMdEQuery& aQuery, TInt aError)
    {
    LOG("CLocationRecord::HandleQueryCompleted - start");
    const TInt count = aQuery.Count();
    LOG1("CLocationRecord::HandleQueryCompleted count: %d", count);

    CMdENamespaceDef* namespaceDef = NULL;

    TRAP_IGNORE( namespaceDef = &iMdeSession->GetDefaultNamespaceDefL() );
    if ( namespaceDef )
        {
        CMdEObjectDef* locationObjectDef = NULL;

        TRAP_IGNORE( locationObjectDef = &namespaceDef->GetObjectDefL( Location::KLocationObject ) );
        if ( locationObjectDef )
        	{
        	CMdEPropertyDef* latitudeDef = NULL;
        	CMdEPropertyDef* longitudeDef = NULL;
        	CMdEPropertyDef* altitudeDef = NULL;
        	
            TRAP_IGNORE( 
            		latitudeDef = &locationObjectDef->GetPropertyDefL(
            				Location::KLatitudeProperty );
            		longitudeDef = &locationObjectDef->GetPropertyDefL(	
            				Location::KLongitudeProperty );
            		altitudeDef = &locationObjectDef->GetPropertyDefL( 
            				Location::KAltitudeProperty );
            		);

            if( latitudeDef && longitudeDef && altitudeDef )
            	{
	            TBool created = EFalse;
	            for ( TInt i = 0; i < count; i++ )
	                {
	                LOG1("CLocationRecord::HandleQueryCompleted check item: %d", i);
	                CMdEItem& item = aQuery.ResultItem(i);
	                CMdEObject& locationObject = static_cast<CMdEObject&>(item);
	
	                CMdEProperty* latProp = NULL;
	                CMdEProperty* lonProp = NULL; 
	                CMdEProperty* altProp = NULL;
	
	                locationObject.Property( *latitudeDef, latProp, 0 );
	                locationObject.Property( *longitudeDef, lonProp, 0 );
	                locationObject.Property( *altitudeDef, altProp, 0 );
	
	                if ( latProp && lonProp )
	                    {
	                    TReal32 distance;
	                    TCoordinate newCoords;
	                    if ( altProp )
	                        {
	                        TRAP_IGNORE( newCoords = TCoordinate( latProp->Real64ValueL(), lonProp->Real64ValueL(), (TReal32)altProp->Real64ValueL() ) );
	                        }
	                    else
	                        {
	                        TRAP_IGNORE( newCoords = TCoordinate( latProp->Real64ValueL(), lonProp->Real64ValueL() ) );
	                        }
	                    
	                    const TInt err = iLocationData.iPosition.Distance(newCoords, distance);
	                    
	                    if ( distance < iLocationDelta )
	                        {
	                        LOG("CLocationRecord::HandleQueryCompleted - match found in db");
	                        TRAPD( err, CreateRelationL( iObjectId, locationObject.Id() ) );
	                        if( err == KErrNone)
	                            {
	                            created = ETrue;
	                            i = count;
	                            }
	                        else
	                            {
	                            aError = err;
	                            }
	                        }
	                    }
	                }

	            if ( !created && aError == KErrNone )
	                {
	                LOG("CLocationRecord::HandleQueryCompleted - no match found in db, create new");
	                TInt locationId( 0 );
	                TRAPD( err, locationId = DoCreateLocationL( iLocationData ) );
	                LOG1("CLocationRecord::HandleQueryCompleted - DoCreateLocationL err: %d", err);
	                if( err == KErrNone )
	                    {
	                    TRAP( err, CreateRelationL( iObjectId, locationId ));
	                    LOG1("CLocationRecord::HandleQueryCompleted - CreateRelationL err: %d", err);
	                    }
	                }
            	}
            }
        }

    LOG("CLocationRecord::HandleQueryCompleted - end");
    }

EXPORT_C void CLocationRecord::SetMdeSession( CMdESession* aSession )
	{
	iMdeSession = aSession;
	TRAPD(err, iRemapper->InitialiseL( aSession ));
	if( err != KErrNone )
		{
		delete iRemapper;
		iRemapper = NULL;
		}
	}

void CLocationRecord::StartTimerL()
	{
	LOG("CLocationRecord::StartTimerL");
	
	if( !iNetworkInfoTimer->IsActive() )
	    {
	    iNetworkInfoTimer->Start( iInterval, iInterval, TCallBack( UpdateNetworkInfo, this ) );
	    }
	}

TTime CLocationRecord::GetMdeObjectTimeL( TItemId aObjectId ) 
    {
    CMdENamespaceDef& namespaceDef = iMdeSession->GetDefaultNamespaceDefL();

    CMdEObjectDef& objectDef = namespaceDef.GetObjectDefL( Object::KBaseObject );
    CMdEPropertyDef& timeDef = objectDef.GetPropertyDefL( Object::KLastModifiedDateProperty );

    CMdEObject* object = NULL;
    CMdEProperty* property = NULL;
    
    object = iMdeSession->GetObjectL( aObjectId );
    CleanupStack::PushL( object );
    object->Property( timeDef, property, 0 );
    if ( !property )
        {
        User::Leave( KErrNotFound );
        }
    
    const TTime timeValue( property->TimeValueL() );
    CleanupStack::PopAndDestroy( object );
    return timeValue;
    }

EXPORT_C TBool CLocationRecord::RemappingNeeded()
	{
	return iRemapper->ItemsInQueue();
	}

// End of file
