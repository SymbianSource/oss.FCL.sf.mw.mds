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

#ifndef C_CLOCATIONRECORD_H
#define C_CLOCATIONRECORD_H

#include <e32base.h>
#include <e32property.h>
#include <lbs.h>

#include "rlocationtrail.h"
#include "locationdatatype.h"
#include "cnetworkinfo.h"
#include "cpositioninfo.h"
#include "rlocationobjectmanipulator.h"

#include "mdccommon.h"
#include "mdesession.h"
#include "mdenamespacedef.h"
#include "mdeobjectdef.h"
#include "mdepropertydef.h"
#include "mderelation.h"
#include "mdequery.h"
#include "locationremappingao.h"

typedef RLocationTrail::TTrailState TLocTrailState;

class CTelephony;
class TPositionSatelliteInfo;

/**
*  An observer interface, which is used for getting notification when the 
*  location trail's state changes.    
*
*  @since S60 3.1
*/
class MLocationTrailObserver
    {
public:    
    /**
     * This method is used to notify about location trail state changes.
     */
    virtual void LocationTrailStateChange() = 0;
    
    virtual void CurrentLocation( const TPositionSatelliteInfo& aSatelliteInfo, 
    							  const CTelephony::TNetworkInfoV1& aNetworkInfo,
                                  const TInt aError ) = 0;
    
    virtual void GPSSignalQualityChanged( const TPositionSatelliteInfo& aSatelliteInfo )  = 0;
    
    /**
     * Callback method to notify observer that during waiting for positioning stop timeout remap is done.
     */
    virtual void RemapedCompleted() = 0;
    
    /**
     * Returns if in ETrialStopping state server waits for positioning stop timeout
     * @returns <code>ETrue</code> if server is waiting for positioning stop timeout
     *          <code>EFalse</code>, otherwise.
     */
    virtual TBool WaitForPositioningStopTimeout() = 0;
    };

/**
* Location trail item class.
*/
class TLocationTrailItem
    {
    public:
    	TLocationData	iLocationData; // Location info & network info
        TTime           iTimeStamp;  // Time stamp.
        TLocTrailState  iTrailState; // Trail state for this item.
    };
    
class MLocationAddObserver
	{
public:
	/**
	 * This method is used to notify about new locations added to location trail
	 */
	virtual void LocationAdded( const TLocationTrailItem& aTrailItem, 
								const TPositionSatelliteInfo& aSatellites ) = 0;
	};    

/**
 *  Location trail collects location information periodically and stores them
 *  to an array. Stored locations may be searched by time stamp to get 
 *  a location, which corresponds to certain time.  
 *
 *  @since S60 3.1
 */
class CLocationRecord : public CBase,
                        public MNetworkInfoObserver,
                        public MPositionInfoObserver,
                        public MMdEQueryObserver
    {
public:  
    /**
     * 2-phased constructor.
     * @since S60 3.1
     */
    IMPORT_C static CLocationRecord* NewL();

    /**
     * C++ destructor.
     * @since S60 3.1
     */    
    IMPORT_C virtual ~CLocationRecord();

    
public:
    /**
     * Returns the current state of the location trail.
     * @since S60 3.1
     * @param aState, The current state is written to this variable.
     * @return None.
     */    
    IMPORT_C void LocationTrailState( TLocTrailState& aState );
    
    /**
     * Starts collecting locations from Location Acquisition API.
     * @since S60 3.1
     * @param None.
     * @return None.
     */    
    IMPORT_C void StartL( RLocationTrail::TTrailCaptureSetting aCaptureSetting );
    
    /**
     * Stops collecting locations.
     * @since S60 3.1
     * @param None.
     * @return None.
     */    
    IMPORT_C void Stop();    

    /**
     * Returns the location info, which is nearest to the given time.
     * @since S60 3.1
     * @param aTime, A time stamp to get corresponding location.
     * @param aPosition, Location info is written to this param.
     * @param 
     * @return None.
     */
    IMPORT_C void GetLocationByTimeL( const TTime aTime,
    								  TLocationData& aLocationData,
                                      /*TLocality& aPosition,
                                      CTelephony::TNetworkInfoV1& aNetworkInfo,*/
                                      TLocTrailState& aState );

    /**
     * Request location info. The result is returned by calllback method.
     * @since S60 3.1
     * @param None.
     * @return None.
     */
    IMPORT_C void RequestLocationL();

    /**
     * Cancel request for location info. 
     * @since S60 3.1
     * @param None.
     * @return None.
     */    
    IMPORT_C void CancelLocationRequest();
        
    /**
     * Get network cell id.
     * @since S60 3.1
     * @param aCellId, Network cell is written into this param.
     * @return None.
     */
    IMPORT_C void GetNetworkInfo( CTelephony::TNetworkInfoV1& aNetworkInfo );
    
    /**
     * Set observer for notifying state changes.
     * @since S60 3.1
     * @param aObserver, An interface to notify about state changes.
     * @return None.
     */
    IMPORT_C void SetObserver( MLocationTrailObserver* aObserver );
    
    /**
     * Set observer (TrackLog) for notifying new locations in location trail
     */
    IMPORT_C void SetAddObserver( MLocationAddObserver* aObserver );    
    
    static TInt UpdateNetworkInfo( TAny* aAny );
    
    IMPORT_C void CreateLocationObjectL( const TLocationData& aLocationData,
    		const TUint& aObjectId );
    
    IMPORT_C void LocationSnapshotL( const TUint& aObjectId );
    
    TItemId DoCreateLocationL( const TLocationData& aLocationData );
    
    TItemId CreateRelationL( const TUint& aObjectId, const TUint& aLocationId );
    
    IMPORT_C void SetMdeSession( CMdESession* aSession );
    
    IMPORT_C void SetStateToStopping();
    
    TTime GetMdeObjectTimeL( TItemId aObjectId );
    
    IMPORT_C TBool RemappingNeeded();

public: // from MNetworkInfoObserver.
    /**
     * 
     * @since S60 3.1
     * @param 
     * @return 
     */
    void NetworkInfo( const CTelephony::TNetworkInfoV1 &aNetworkInfo, TInt aError );
    
public: // from MPositionInfoObserver    
    /**
     * 
     * @since S60 3.1
     * @param 
     * @return  
     */
    void Position( const TPositionInfo& aPositionInfo, const TInt aError );
    
    
public: // From MMdEQueryObserver

	void HandleQueryNewResults(CMdEQuery& aQuery, TInt aFirstNewItemIndex, 
			TInt aNewItemCount);
	
	void HandleQueryCompleted(CMdEQuery& aQuery, TInt aError);
	
private:    
    /**
     * Stores the location info into the array.
     */
    void StoreLocation( /*const TPosition& aPosition, const TCourse& aCourse,*/ 
    		const TPositionSatelliteInfo& aSatelliteInfo );
    
    /**
     * Changes the current state. New state is published in P&S and
     * possible observer is notified.
     */    
    void SetCurrentState( TLocTrailState aState );
    
    /**
     * Returns the requested location via callback method, if the location
     * is valid. Otherwise new location value is requested until the value
     * is succesful, or the time out limit has been reached.
     * 
     */
    void HandleLocationRequest( const TPositionSatelliteInfo& aSatelliteInfo /*TLocality& aPosition*/, 
                                const TInt aError );
    /**
     * C++ constructor.
     */  
    CLocationRecord();
    
    /**
     * 2nd phase constructor.
     */
    void ConstructL();
    
    /**
     * Read interval value from Central repository
     * @param aKey, Key to item
     * @param aValue, Read value
     */ 
    void ReadCenRepValueL(TInt aKey, TInt& aValue);
    
    TBool CheckGPSFix( const TPositionSatelliteInfo& aSatelliteInfo );
    
    void StartTimerL();

private:
	/**
	 * A session to Metadata Engine for creating and manipulating location objects.
	 */
	CMdESession* iMdeSession;
	
    /**
     * An observer interface to notify about state changes.
     * Not own.
     */
    MLocationTrailObserver* iObserver;
    
    /**
     * An observer interface to notify about new locations.
     * Not own.
     */
    MLocationAddObserver* iAddObserver;    

    /**
     * An array to collect location values.
     * Own.
     */
    RArray<TLocationTrailItem> iTrail;

    /**
     * P&S key property.
     * Own.
     */
    RProperty iProperty;

    /**
     * Active class to get network information.
     * Own.
     */
    CNetworkInfo* iNetworkInfo;
    
    /**
     * Active class to get position information.
     * Own.
     */
    CPositionInfo* iPositionInfo;
    
	/**
	 * Class which handles database remapping operations
	 */ 
	CLocationRemappingAO* iRemapper;
    
    CActiveSchedulerWait   iWait;
    
    /**
     * Timer for capturing network info only
     */
    CPeriodic*			   iNetworkInfoTimer;

    TLocTrailState         iState;    
    TLocationTrailItem     iNewItem;
    RLocationTrail::TTrailCaptureSetting   iTrailCaptureSetting;
    CTelephony::TNetworkInfoV1		   iNetwork;

    TInt                   iMaxTrailSize;
    TInt                   iLocationCounter;
    
    /*
     * Interval value for location trail
     */
    TInt iInterval;
    
    TBool                  iRequestCurrentLoc;
    TBool                  iTrailStarted;
    
    TUint				   iLastNumberOfSatellitesUsed;
    TReal32				   iLastHDOP;
    TReal32				   iLastVDOP;
    TBool				   iLastGPSFixState;
    
    TInt                   iLocationDelta;
    TLocationData          iLastLocation;
    TItemId                iLastLocationId;
    
    TItemId iObjectId;               
    TLocationData iLocationData;
    
    /**
     * This query object is used to find existing locations
     */
    CMdEObjectQuery* iLocationQuery;
    };

#endif // C_CLOCATIONRECORD_H 

// End of file.
