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
* Description:  HTTP client engine, which takes of doing an Http GET request for
* the maps server.
*/

#ifndef __CLIENTENGINE_H__
#define __CLIENTENGINE_H__

#include <http/mhttptransactioncallback.h>
#include <http/mhttpauthenticationcallback.h>
#include <es_sock.h>
#include <cmmanager.h>
#include <comms-infras/cs_mobility_apiext.h>
#include <etel.h>
#include <etelmm.h>

#include "reversegeocode.h"

class RHTTPSession;
class RHTTPTransaction;

const TInt KDefaultBufferSize = 256;

/*
 *  Enumration for the different states of HTTP GET request.
 */
enum THttpStatus
    {
        EHttpSessionError =0,
        EHttpExitingApp,
        EHttpConnecting,
        EHttpTxCancelled,
        EHttpHdrReceived,
        EHttpBytesReceieved,
        EHttpBodyReceieved,
        EHttpTxCompleted,
        EHttpTxSuccess,
        EHttpTxFailed,
        EHttpConnectionFailure,
        EHttpUnknownEvent,
        EHttpMhfRunError,
        EHttpAuthNote,
        EHttpAuthFailed,
        EHttpAuthRequired,
    };

enum TMobileRoamingStatus
    {
    EMobileNotRegistered = 0x00,
    EMobileRegHomeNetwork, // home network
    EMobileNationalRoaming,
    EMobileInternationalRoaming
    };

/*
* MClientObserver
* CClientEngine passes events and responses body data with this interface.
* An instance of this class must be provided for construction of CClientEngine.
*/
class MClientObserver
    {
    public:
    /*
    * ClientEvent()
    *
    * Called when event occurs in CClientEngine.
    *
    * @params aEvent Status of the event.
    */
    virtual void ClientEvent( const THttpStatus& aEvent ) = 0;
    
    /*
    * ClientBodyReceived()
    *
    * Called when a part of the HTTP body is received.
    *
    * @param aBodyData Part of the body data received. (e.g. part of
    *         the received HTML page)
    */
    virtual void ClientBodyReceived( const TDesC8& aBodyData ) = 0;

    /*
    * Get registrer network country code
    *
    * @return current register n/w info
    */
    virtual RMobilePhone::TMobilePhoneNetworkInfoV2& GetCurrentRegisterNw() = 0;

    /*
    * UE is registered to home network?
    *
    * @return ETrue if UE is registered at home network else EFalse
    */
    virtual TBool IsRegisteredAtHomeNetwork() = 0;

    /*
    * Get home network country code
    * @param aHomeNwInfoAvailableFlag ETrue if home n/w info available else EFalse
    * @return user home n/w info
    */
    virtual const RMobilePhone::TMobilePhoneNetworkInfoV1& 
        GetHomeNetworkInfo(TBool& aHomeNwInfoAvailableFlag) = 0;
    };


/*
* Provides simple interface to HTTP Client API.
*/
NONSHARABLE_CLASS( CClientEngine ): public CActive,
                                    public MHTTPTransactionCallback,
                                    public MMobilityProtocolResp
    {

    //Internal Engine state
    enum TEngineState
    {
    EIdle = 0,
    EGet
    };

    public:
    /*
     * Create a CClientEngine object.
     *
     * @params iObserver refernce to the MClientObservers implementation
     *
     * @returns A pointer to the created instance of CClientEngine
     */
    static CClientEngine* NewL( MClientObserver& iObserver );

    /*
     * Create a CClientEngine object. This leaves the object on the clean up
     * stack.
     *
     * @params iObserver refernce to the MClientObservers implementation
     *
     * @returns A pointer to the created instance of CClientEngine
     */
    static CClientEngine* NewLC( MClientObserver& iObserver );

    /*
     * Destroy the object
     *
     */
    ~CClientEngine();

    /*
     * Starts a new HTTP GET transaction.
     *
     * @param aUri URI to get request. (e.g. http://host.org")
     */
    void IssueHTTPGetL( const TDesC8& aUri, const TConnectionOption aOption );

    /*
     * Closes currently running transaction and frees resources related to it.
     */
    void CancelTransaction();
    
	/*
	* Closes the connection
	*/
    void CloseConnection();


	/*
	* checks if silent connection is allowed
	* @return ETrue if silent connection is allowed
	*/
    TBool SilentConnectionAllowed();

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif    
    /*
     * Perform the second phase construction of a CClientEngine object.
     */
    void ConstructL();

    /*
     * Performs the first phase of two phase construction.
     * @param iObserver The observer that is notified after the HTTP transcation is over
     */
    CClientEngine( MClientObserver& iObserver );

    /*
     * Sets header value of an HTTP request.
     *
     * @param aHeaders Headers of the HTTP request
     * @param aHdrField Enumerated HTTP header field, e.g. HTTP::EUserAgent
     * @param aHdrValue New value for header field.
     */
    void SetHeaderL( RHTTPHeaders aHeaders, TInt aHdrField,
                     const TDesC8& aHdrValue );

    /*
     * Sets up the connection
     * @param aOption The connection option
     */
    void SetupConnectionL( const TConnectionOption aOption );

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif  
    //From MHTTPSessionEventCallback
    /*
     * Called by framework to notify about transaction events.
     *
     * @param aTransaction Transaction, where the event occured.
     * @param aEvent Occured event.
     */
    void MHFRunL( RHTTPTransaction aTransaction, const THTTPEvent& aEvent );

    /*
     * Called by framework to notify about transaction events.
     *
     * @param aTransaction Transaction, where the event occured.
     * @param aError Error status code.
     * @param aEvent The event that was being processed when leave occured.
     * 
     * @retuen    KErrNone, if the error was handled. Otherwise the value of aError, or
     *   some other error value. Returning error value causes causes
     *   HTTP-CORE 6 panic.
     */
    TInt MHFRunError( TInt aError, RHTTPTransaction aTransaction, const THTTPEvent& aEvent );

    
#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 
    // from MMobilityProtocolResp
    void PreferredCarrierAvailable( TAccessPointInfo aOldAPInfo,
                                    TAccessPointInfo aNewAPInfo,
                                    TBool aIsUpgrade,
                                    TBool aIsSeamless );
    void NewCarrierActive( TAccessPointInfo aNewAPInfo, TBool aIsSeamless );
    void Error( TInt aError );

	TBool IsDataConnectionAskAlwaysL();
	TMobileRoamingStatus UeRegNetworkStatus();
    TBool IsVisitorNetwork(const TMobileRoamingStatus& aRoamingStatus) const;
    TBool IsWlanOnly(const TMobileRoamingStatus& aRoamingStatus,
                const TCmGenConnSettings& aGenConnSettings) const;
    
#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 

	/**
	  * RunL
	  * This method is called on completion of the active object request
	  */

    void RunL();

	/**
	  * DoCancel
	  * Cancels any outstanding requests
	  */	
    void DoCancel();

	/**
	  * This method is called if the RunL leaves
	  *  @param aError The errcode with which it leaves
	  */	
    TInt RunError(TInt aError);

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 

	/**
	  * Submits a HTTP transaction
	  */
    void DoHTTPGetL();
    
#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 
  // declare members
    RSocketServ                 iSocketServ;
    RConnection                 iConnection;
    TUint32                     iSelectedIap;

    RHTTPSession                iSession;
    RHTTPTransaction            iTransaction;

    MClientObserver&            iObserver;  // Used for passing body data and events to UI
    TBool                       iRunning;   // ETrue, if transaction running
    TBool                       iConnectionSetupDone;
    
    TInt                        iPrevProfileId;
    
    CActiveCommsMobilityApiExt* iMobility;
    TBool                       iTransactionOpen;
    TEngineState                 iEngineState;
    HBufC8*                     iUri;
	RCmManager                  iCmManager;
    
    };

#endif // __CLIENTENGINE_H__

// End of file
