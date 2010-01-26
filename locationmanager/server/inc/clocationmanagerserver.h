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
* Description:  Header file for Location Manager Server.
*
*/

#ifndef C_CLOCATIONMANAGERSERVER_H
#define C_CLOCATIONMANAGERSERVER_H


#include <e32base.h>
#include <etel3rdparty.h>
#include <centralrepository.h>
#include <locationdatatype.h>
#include <locationeventdef.h>

#include "rlocationtrail.h"
#include "rlocationobjectmanipulator.h"
#include "clocationrecord.h"
#include "ctracklog.h"
#include "mdeconstants.h"
#include "mdsutils.h"
#include "mdesession.h"
#include "mdequery.h"
#include "mderelationquery.h"
#include "locationmanagerdefs.h"


class CMdESession;

// Total number of ranges
const TUint KLocationManagerRangeCount = 1;

// Definition of the ranges of IPC numbers
const TInt KLocationManagerRanges[KLocationManagerRangeCount] = 
    {
    0,
    }; 

// Policy to implement for each of the above ranges        
const TUint8 KLocationManagerElementsIndex[KLocationManagerRangeCount] = 
    {
    0,
    };

const CPolicyServer::TPolicyElement KLocationManagerPolicyElements[] = 
    {
    { _INIT_SECURITY_POLICY_C3(ECapabilityReadUserData,
                               ECapabilityWriteUserData,
                               ECapabilityLocation),
                               CPolicyServer::EFailClient }
    };

// Package all the above together into a policy
const CPolicyServer::TPolicy KLocationManagerPolicy =
    {
    CPolicyServer::EAlwaysPass,     // all attempts should pass
    KLocationManagerRangeCount,     // number of ranges
    KLocationManagerRanges,         // ranges array
    KLocationManagerElementsIndex,  // elements<->ranges index
    KLocationManagerPolicyElements, // array of elements
    };
    
class CLocationRecord;
//class CLocationWrite;

/**
*  A server class to initialize server. 
*
*  @since S60 3.1
*/
class CLocationManagerServer : public CPolicyServer,
                               public MLocationTrailObserver,
                               public MMdESessionObserver,
                               public MMdEQueryObserver,
                               public MMdEObjectObserver,
                               public MGpxConversionObserver
    {
private:
	struct TMessageQuery
		{
		CMdERelationQuery* iQuery;
		const RMessage2& iMessage;
		
		TMessageQuery( CMdERelationQuery* aQuery, const RMessage2& aMessage ) 
			:  iQuery ( aQuery ), iMessage( aMessage ) { }
		};
    
public:
    /**
    * 2-phased constructor.
    */
    static CLocationManagerServer* NewLC();
    
    /**
    * C++ destructor.
    */ 
    virtual ~CLocationManagerServer();
  
public:
    /**
    * From CServer2, creates a new session.
    */
    CSession2* NewSessionL( const TVersion& aVersion, 
                            const RMessage2& aMessage ) const;

public: // From MMdESessionObserver
	void HandleSessionOpened(CMdESession& aSession, TInt aError);
    void HandleSessionError(CMdESession& aSession, TInt aError);
    
public: // From MMdEQueryObserver
	void HandleQueryNewResults(CMdEQuery& aQuery, TInt aFirstNewItemIndex, TInt aNewItemCount);
	void HandleQueryCompleted(CMdEQuery& aQuery, TInt aError);

public:
    /**
     * Increase session count.
     * @since S60 3.1
     * @param None.
     * @return None.
     */
    void AddSession();
    
    /**
     * Decrease session count, close server if count is zero.
     * @since S60 3.1
     * @param None.
     * @return None.
     */
    void RemoveSession();

public:
    /**
     * Starts to record locations to the trail.
     * @since S60 3.1
     * @param None.
     * @return None.
     */     
    void StartGPSPositioningL( RLocationTrail::TTrailCaptureSetting aCaptureSetting );
    
    /**
     * Stops trail recording.
     * @since S60 3.1
     * @param None.
     * @return None.
     */     
    void StopGPSPositioningL();
    
    /**
     * Gets the current state of the location trail.
     * @since S60 3.1
     * @param aState, The state of the trail is written to this param.
     * @return None.
     */     
    void GetLocationTrailState( RLocationTrail::TTrailState& aState );
    
    /**
     * Adds client's notification request to the request array.
     * @since S60 3.1
     * @param aNotifReq, asynchonous RMessage is stored to be completed
     *                   when the state of the trail changes.
     * @return None.
     */     
    void AddNotificationRequestL( const RMessage2& aNotifReq );
    
    /**
     * Add client's track log notification request to the request array.
     * @param aNotifReq, asynchonous RMessage is stored to be completed
     *                   when the state of the track log changes.
     * @return None.
     */
    void AddTrackLogNotificationRequestL( const RMessage2& aNotifReq );
    
    /**
     * Cancel notification request from client.
     * @since S60 3.1
     * @param aHandle, A handle of the request to be cancelled.
     * @return None.
     */     
    void CancelNotificationRequest( const TInt aHandle );
    
    /**
     * Get a location by time stamp.
     * @since S60 3.1
     * @param aTimeStamp, A time stamp to get corresponding location.
     * @param aPosition, Location info is written to this param.
     * @return None.
     */     
    void GetLocationByTimeL( const TTime& aTimeStamp, 
    						 TLocationData& aLocationData,
                             /*TLocality& aPosition, 
                             CTelephony::TNetworkInfoV1& aNetworkInfo,*/
                             TLocTrailState& aState );

    /**
     * Get current location.
     * @since S60 3.1
     * @param aCurrLocReq 
     * @return None.
     */     
    void RequestCurrentLocationL( const RMessage2& aCurrLocReq ); 

    /**
     * Cancel location request.
     * @since S60 3.1
     * @param aCurrLocReq 
     * @return None.
     */     
    void CancelLocationRequest( const TInt aHandle ); 
    
    /**
     * Cancel tracklog notification request.
     * @param aHandle
     * @return None.
     */
    void CancelTrackLogNotificationRequest( const TInt aHandle );

    /**
     * Get current network cell id.
     * @since S60 3.1
     * @param aCurrLocReq 
     * @return None.
     */     
    void GetCurrentNetworkInfo( CTelephony::TNetworkInfoV1& aNetworkInfo );
    
    /**
     * Create a location context object in DB and create relationships to objects
     * whose id is given in the array.
     * @param aLocationData
     * @param aObjectId
     */
	void CreateLocationObjectL( const TLocationData& aLocationData, 
    						   	   const TUint& aObjectId );
	
    /**
     * Create a location context object in DB and create relationships to objects
     * whose id is given in the array.
     * Location information is taken from location trail
     * @param aObjectId
     */
	void LocationSnapshotL( const TUint& aObjectId );
	
	/**
	 * Deletes the relationship between an object and 
	 * the location context object associated with it.
	 * @since S60 3.2
	 * @param aObjectId
	 * @return None.
	 */ 
	void RemoveLocationObjectL(TUint& aObjectId);
	void CopyLocationObjectL( TItemId aSource, const RArray<TItemId>& aTargets, TMessageQuery& aQuery );
	void CopyLocationObjectL( const TDesC& aSource, const RArray<TPtrC>& aTargets, TMessageQuery& aQuery );
	
	TBool IsSessionReady();
	
	TItemId StartTrackLogL();
	
	void StopTrackLogL();
	
	void IsTrackLogRecording( TBool &aRec );
	
	TInt GetTrackLogStatus( TBool& aRecording, TPositionSatelliteInfo& aFixQuality);
	
	TInt DeleteTrackLogL(const TDesC& aUri);
	
	TInt TrackLogName(TFileName& aFileName);
	
	void GetCaptureSetting( RLocationTrail::TTrailCaptureSetting& aCaptureSetting );
	
	void AddGpxObserver( MGpxConversionObserver* aObserver );
	
	void InitCopyLocationByIdL( const RMessage2& aMessage );
	void InitCopyLocationByURIL( const RMessage2& aMessage );

public: // from MLocationTrailObserver.
    /**
     * Callback method to get notification about trail state change.
     * @since S60 3.1
     * @param None.
     * @return None.
     */
    void LocationTrailStateChange();
    
    /**
     * Callback method to return current location.
     * @since S60 3.1
     * @param aSatelliteInfo, includes position and satellite info.
     * @param aNetworkInfo, network and cell info.
     * @param aError.
     * @return None.
     */
    void CurrentLocation( const TPositionSatelliteInfo& aSatelliteInfo,
    		const CTelephony::TNetworkInfoV1& aNetworkInfo, const TInt aError );
    
    /**
     * Callback method to notify observer of changes in GPS signal quality.
     * @param aSatelliteInfo, includes position and satellite info
     * @return None.
     */
    void GPSSignalQualityChanged( const TPositionSatelliteInfo& aSatelliteInfo );
    
public: // from MMdeObjectObserver
	/**
	 * Called to notify the observer that new objects has been
	 * added/modified/removed in the metadata engine database.
	 *
	 * @param aSession session
	 * @param aType defines if object was added/modified/remove
	 * @param aObjectIdArray IDs of added object
	 */
	void HandleObjectNotification( CMdESession& aSession, 
						TObserverNotificationType aType,
						const RArray<TItemId>& aObjectIdArray );
	
public: // from MGpxConversionObserver

	void GpxFileCreated( const TDesC& aFileName, TItemId aTagId, TReal32 aLength,
			TTime aStart, TTime aEnd );

private:    
    /**
    * C++ constructor.
    */
    CLocationManagerServer();
    
    /**
    * 2nd phase constructor.
    */
    void ConstructL();
    
    void CopyLocationL( CMdEQuery& aQuery );

    /**
     * Go through all messages in lists, complete request
     * with KErrCancel status and remove items from list.
     */ 
    void CancelRequests(RArray<RMessage2>& aMessagesList);
    
    
    void CancelCopyRequests(RArray<TMessageQuery>& aMessageList);
    
    /**
     * Create a new tracklog tag object in the database.
     * @return TItemId tag ID in the database
     */ 
    TItemId CreateTrackLogTagL();
    
    /**
     * Create a new tracklog object in the database and link tracklog tag to it.
     * @param aTagId, tracklog tag id
     */ 
    void CreateTrackLogL( TItemId aTagId, const TDesC& aUri, TReal32 aLength,
    		TTime aStart, TTime aEnd );
    
    /**
     * Start listening for tracklog tag removals.
     */
    void StartListeningTagRemovalsL();
    
    /**
     * Start listening for media object creations.
     */
    void StartListeningObjectCreationsL();
    
    /**
     * Create a relationship between mediaobject(s) and a tracklog tag.
     * @param aObjectIdArray, array of media object ids
     */
    void LinkObjectToTrackLogTagL( const RArray<TItemId>& aObjectIdArray );
    
    /**
     * Callback function for positioning stop timer.
     * @param aAny, a pointer to CLocationRecord object
     * @return Error code
     */
    static TInt PositioningStopTimeout( TAny* aAny );
    
    /**
     * Stops location trail and deletes the positioning stop timer.
     */
    void StopRecording();
    
    void CompleteNotifyRequest( TEventTypes aEventType, TInt aError );

private:
    /**
     * A class for recording and storing locations.
     * Own.
     */
    CLocationRecord* iLocationRecord;
    
    /**
     * Pointer to TrackLog
     * Own.
     */
    CTrackLog* iTrackLog;
    
    /**
     * An active scheduler wait loop for waiting a session to MdE to open.
     */
    CActiveSchedulerWait* iASW;
    
    /**
     * An array for asynchronous notification requests.
     * Own.
     */
    RArray<RMessage2> iNotifReqs;
    
    /**
     * An array for asynchronous location requests.
     * Own.
     */
    RArray<RMessage2> iLocationReqs;
    
    /**
     * An array for track log notification requests.
     * Own.
     */
    RArray<RMessage2> iTrackLogNotifyReqs;
    
    /**
     * An array for location copy requests.
     * Own.
     */
    RArray<TMessageQuery> iCopyReqs;
    
    /**
     * A session to Metadata Engine for creating and manipulating location objects.
     */
    CMdESession* iMdeSession;
    
    /** A relation query used to seach for related location objects */
    CMdERelationQuery* iRelationQuery;
    
    /**
     * A timer to stop location trail.
     * Own.
     */
    CPeriodic* iTimer;
    
    TBool iClientSwitch;    
    TInt iSessionCount;
    TBool iSessionReady;  
    
    RArray<TItemId> iTargetObjectIds;
    TItemId iTagId;
    TInt iLocManStopDelay;
    
    RLocationTrail::TTrailCaptureSetting iCaptureSetting;
    TBool iRemoveLocation;    
    };


#endif // C_CLOCATIONMANAGERSERVER_H

// End of file.
