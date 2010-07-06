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
* Description:  Implementaion class for the CReverseGeocode class.
*/
#ifndef _INTERNALREVERSEGEOCODE_H_
#define _INTERNALREVERSEGEOCODE_H_

#include <coemain.h>

#include "reversegeocode.h"
#include "xmlhandler.h"
#include "clientengine.h"
#include "connectiontimerhandler.h"


/*
 * Internal implementation class for the CReverseGeocode class.
 */
NONSHARABLE_CLASS( CInternalReverseGeocode ) : public CReverseGeocode, 
                    public MClientObserver, 
                    public MXmlHandlerObserver,
                    public MConnectionTimeoutHandlerInterface
    {
public:
    /**
        * Factory function to create the instance
        * @param aObserver The observer instance that is to be notified when reverse geocoding is over
        * @return A pointer to the CInternalReverseGeocode instance
        */
    static CInternalReverseGeocode* NewL( MReverseGeocodeObserver& aObserver );

    /**
        * Gets the address for the given geo-coordinaates.
        * @param aLocality  The locality information
        *		   aOption    The connection option whether 	its silent or not
        */			
    virtual void GetAddressByCoordinateL( TLocality aLocality, const TConnectionOption aOption );

    /**
        * Checks if a silent connection is allowed
        * @return ETrue If silentconnection is allowed
        */	
    virtual TBool SilentConnectionAllowed();
    
    
    /**
        * Helper function to get the appropriate language for the request.
        * @param aLanguage  The language for the request 
        */    
    void GetLanguageForTheRequest( TDes8& aLanguage );


    /**
        * destructor
        *
        */	
    ~CInternalReverseGeocode();

    // MConnectionTimeoutHandlerInterface
    /**
        * Closes the connection once it times out
        * @param aErrorCode The Error code
        */    
    void HandleTimedoutEvent(TInt aErrorCode);
    
protected:
   
    /**
        * Second phase construction
        */    
    void ConstructL();
        
    
    /**
        * First phase construction.
        * @param aObserver The observer instance that is to be notified when reverse geocoding is over
        */    
    CInternalReverseGeocode( MReverseGeocodeObserver& aObserver );
        
    //From MClientObserver
    /**
        * callback which notifies progess of HTTP request
        * @param aEvent The Httpstatus
        */    
    void ClientEvent( const THttpStatus& aEvent );
	
    /**
        * callback through which the HTTP body data is recieved. 
        * @param aBodyData The body recieved
        */	
    void ClientBodyReceived(const TDesC8& aBodyData);

    /*
    * Get registrer network country code
    *
    * @return current register n/w info
    */
    RMobilePhone::TMobilePhoneNetworkInfoV2& GetCurrentRegisterNw();

    //From MXmlHandlerObserver    
    /**
        * callback which notifys the completion of parsing.
        * @param aError The err code
        * 		   aAddressInfo The address info obtained after parsing
        */    
    void OnParseCompletedL( TInt aError, MAddressInfo& aAddressInfo ); 
    
    /*
    * UE is registered to home network?
    *
    * @return ETrue if UE is registered at home network else EFalse
    */
    TBool IsRegisteredAtHomeNetwork();


    /*
    * Get home network country code
    * @param aHomeNwInfoAvailableFlag ETrue if home n/w info available else EFalse
    * @return user home n/w info
    */
    const RMobilePhone::TMobilePhoneNetworkInfoV1& 
        GetHomeNetworkInfo(TBool& aHomeNwInfoAvailableFlag);

private:

    /**
        * Starts the timer
        */		
    void StartTimer();

    /**
        * Closes the http connection and notifies the observer
        */	
    void CloseConnection();

private:
    CXmlHandler *iXmlHandler;
    CClientEngine *iClientEngine;
    CInternalAddressInfo *iAddressInfo;
    HBufC8* iXMLBuf;
    MReverseGeocodeObserver& iObserver;
    CConnectionTimerHandler*      iTimer;
	
	// Optimize the buffer len..??
    TBuf8<KMaxFileName> iQueryString;
    TBuf8<KMaxFileName> iAuthCode;
    TBuf8<KMaxFileName> iRefURL;
    TBuf8<KMaxFileName> iLang;
    TBool iStartTimerFlag;
};

#endif //_INTERNALREVERSEGEOCODE_H_

// End of file
