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
* Description: Implementation of HTTP Client Engine.
*/

//Symbian headers
#include <http.h>
#include <commdbconnpref.h>
#include <connpref.h>
#include <extendedconnpref.h>
#include <commdb.h>
#include <etel3rdparty.h>                // voice call notification
#include <mmtsy_names.h>                 // kmmtsymodulename
#include "locationmanagerdebug.h"
#include "clientengine.h"

// Used user agent for requests
_LIT8(KUserAgent, "SimpleClient 1.0");

// This client accepts all content types.
_LIT8(KAccept, "*/*");

// ----------------------------------------------------------------------------
// CClientEngine::NewL()
// ----------------------------------------------------------------------------
CClientEngine* CClientEngine::NewL( MClientObserver& aObserver)
    {
    LOG("CClientEngine::NewL ,begin");
    CClientEngine* self = CClientEngine::NewLC( aObserver);
    CleanupStack::Pop( self );
    return self;
    }

// ----------------------------------------------------------------------------
// CClientEngine::NewLC()
// ----------------------------------------------------------------------------
CClientEngine* CClientEngine::NewLC(MClientObserver& aObserver)
    {
    LOG("CClientEngine::NewLC ,begin");
    CClientEngine* self = new ( ELeave ) CClientEngine( aObserver);
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CClientEngine::CClientEngine()
// ----------------------------------------------------------------------------
CClientEngine::CClientEngine( MClientObserver& aObserver): 
                                CActive( CActive::EPriorityStandard ),
                                iObserver( aObserver ),
                                iConnectionSetupDone( EFalse ),
                                iPrevProfileId( -1 ),
                                iMobility(NULL),
                                iTransactionOpen( EFalse ),
                                iUri(NULL)
    {
    }

// ----------------------------------------------------------------------------
// CClientEngine::~CClientEngine()
// ----------------------------------------------------------------------------
CClientEngine::~CClientEngine()
    {
    LOG("CClientEngine::~CClientEngine ,begin");
    Cancel();

    if ( iTransactionOpen )
        {
        iTransaction.Close();
        iTransactionOpen = EFalse;
        }
    
    if ( iMobility )
        {
        iMobility->Cancel();
        }
    delete iMobility;
    if(iConnectionSetupDone)
        {
        iSession.Close();
        iConnection.Close();
        iSocketServ.Close();
        }
    delete iUri;
   
	iCmManager.Close();

    
    // DON'T cose RMobilePhone object

	LOG("CClientEngine::~CClientEngine ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::ConstructL()
// ----------------------------------------------------------------------------
void CClientEngine::ConstructL()
  {
  LOG("CClientEngine::ConstructL ,begin");
  CActiveScheduler::Add(this);
  iCmManager.OpenL();
  LOG("CClientEngine::ConstructL ,end");
  }

// ----------------------------------------------------------------------------
// CClientEngine::CloseConnection()
// ----------------------------------------------------------------------------
void CClientEngine::CloseConnection()
    {
    LOG("CClientEngine::CloseConnection ,begin" );
    if ( iTransactionOpen )
        {
        iTransaction.Close();
        iTransactionOpen = EFalse;
        }
    
    if ( iMobility )
        {
		iMobility->Cancel();
		delete iMobility;
		iMobility = NULL;
        }

    if(iConnectionSetupDone)
        { 
        iSession.Close();
        iConnection.Close();
        iSocketServ.Close();  
           
        iConnectionSetupDone = EFalse;
        }
	LOG("CClientEngine::CloseConnection ,end");
    }


// ----------------------------------------------------------------------------
// CClientEngine::IsVisitorNetwork()
// ----------------------------------------------------------------------------
TBool CClientEngine::IsVisitorNetwork(const TMobileRoamingStatus& aRegNetworkStatus) const
    {
    LOG1("CClientEngine::IsVisitorNetwork. reg network status - %d",
			(TInt) aRegNetworkStatus);
        
    return (aRegNetworkStatus == EMobileNationalRoaming ||
        aRegNetworkStatus== EMobileInternationalRoaming);
    
    }

// ----------------------------------------------------------------------------
// CClientEngine::IsWlanOnly()
// ----------------------------------------------------------------------------
TBool CClientEngine::IsWlanOnly(const TMobileRoamingStatus& aRoamingStatus,
                                const TCmGenConnSettings& aGenConnSettings) const
    {
    LOG1("CClientEngine::IsVisitorNetwork. reg network status - %d",(TInt) aRoamingStatus);
    TBool wlanOnlyFlag = EFalse;
    switch(aRoamingStatus)
        {
        case EMobileRegHomeNetwork:
            LOG("Home network");
            wlanOnlyFlag = (aGenConnSettings.iCellularDataUsageHome == ECmCellularDataUsageDisabled);
            break;
        case EMobileNationalRoaming:
        case EMobileInternationalRoaming:
            LOG("Visitor network");
            wlanOnlyFlag = (aGenConnSettings.iCellularDataUsageVisitor == ECmCellularDataUsageDisabled);
            break;
        default:
            break;
        } // end of switch
    LOG1("Wlan only flag - %d", wlanOnlyFlag ? 1 : 0);
    return wlanOnlyFlag;
    }


// ----------------------------------------------------------------------------
// CClientEngine::UeRegNetworkStatus()
// ----------------------------------------------------------------------------
TMobileRoamingStatus CClientEngine::UeRegNetworkStatus()
    {
    LOG("CClientEngine::UeRegNetworkStatus() ,begin");
    TMobileRoamingStatus roamingStatus = EMobileNotRegistered;
    if(iObserver.IsRegisteredAtHomeNetwork())
        {
        // home network.
        roamingStatus = EMobileRegHomeNetwork;
        LOG("UE registered in home network");
        }
    else
        {
        // roaming network
        TBool homeNwInfoAvailableFlag = EFalse;
        const RMobilePhone::TMobilePhoneNetworkInfoV1& homeNwInfo = 
            iObserver.GetHomeNetworkInfo(homeNwInfoAvailableFlag);
        if(homeNwInfoAvailableFlag)
            {
            RMobilePhone::TMobilePhoneNetworkCountryCode countryCode = 
                        iObserver.GetCurrentRegisterNw().iCountryCode;
            if(countryCode.Compare(homeNwInfo.iCountryCode) == 0)
                {
                // national roaming..
                LOG("UE is in nation roaming");
                roamingStatus = EMobileNationalRoaming;
                }
            else
                {
                // international roaming.
                LOG("UE is in international roaming");
                roamingStatus = EMobileInternationalRoaming;
                }
            }                
        }
    if(roamingStatus == EMobileNotRegistered)
        {
        LOG("UE is not registered with the network. Offline mode.");
        }
	LOG("CClientEngine::UeRegNetworkStatus ,end");
    return roamingStatus;
    }

// ----------------------------------------------------------------------------
// CClientEngine::IsDataConnectionAskAlwaysL()
// ----------------------------------------------------------------------------
TBool CClientEngine::IsDataConnectionAskAlwaysL() 
    {
    LOG("CClientEngine::IsDataConnectionAskAlwaysL ,begin");
    TMobileRoamingStatus roamingStatus = UeRegNetworkStatus();
        
	TCmGenConnSettings genConnSettings;
    TBool retVal = EFalse;
	iCmManager.ReadGenConnSettingsL(genConnSettings);

    LOG1("wlan usage - %d", genConnSettings.iUsageOfWlan);
    LOG1("Home usage - %d", genConnSettings.iCellularDataUsageHome);
    LOG1("Visitor usage - %d", genConnSettings.iCellularDataUsageVisitor);
    
	if((IsWlanOnly(roamingStatus, genConnSettings) && genConnSettings.iUsageOfWlan == ECmUsageOfWlanManual) // wlan
		|| (roamingStatus == EMobileRegHomeNetwork &&  // home
        		genConnSettings.iCellularDataUsageHome == ECmCellularDataUsageConfirm)
		|| (IsVisitorNetwork(roamingStatus) &&  // roaming
    		 genConnSettings.iCellularDataUsageVisitor == ECmCellularDataUsageConfirm)
		)
		{
		retVal = ETrue;
		}
	LOG("CClientEngine::IsDataConnectionAskAlwaysL ,end");
	return retVal;
	}

// ----------------------------------------------------------------------------
// CClientEngine::SetupConnectionL()
// ----------------------------------------------------------------------------
void CClientEngine::SetupConnectionL( const TConnectionOption aOption )
    {
	LOG("CClientEngine::SetupConnectionL ,begin");
	if ( aOption == ESilent && IsDataConnectionAskAlwaysL())
		{
		LOG("Silent mode. connection setup is asked always.");
        if ( iConnectionSetupDone )
            {
            LOG("Already connected. Close the connection\n");
            CloseConnection();
            }
		User::Leave(KErrNotSupported);
		}
    if ( iConnectionSetupDone )
        {
        // Connection setup is done
        LOG("Already connected.\n");
        User::Leave(KErrAlreadyExists);
        }
 
	
    LOG1("SetupConnectionL: connection option: %d\n", aOption );
       
    // Open HTTP Session
    iSession.OpenL();
    User::LeaveIfError(iSocketServ.Connect());
    User::LeaveIfError(iConnection.Open(iSocketServ));
    
    if ( aOption == ESilent )
        {
        // Create overrides
        TConnPrefList prefList;
        TExtendedConnPref prefs;
        prefs.SetSnapPurpose( CMManager::ESnapPurposeInternet );
        prefs.SetNoteBehaviour( TExtendedConnPref::ENoteBehaviourConnSilent );
        prefList.AppendL( &prefs );
        
        iConnection.Start(prefList, iStatus);
        }
    else
        {
        iConnection.Start( iStatus );
        }
   
    
    SetActive();
	LOG("CClientEngine::SetupConnectionL ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::SetHeaderL()
// ----------------------------------------------------------------------------
void CClientEngine::SetHeaderL( RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue )
    {
    LOG("CClientEngine::SetHeaderL ,begin");
    RStringF valStr = iSession.StringPool().OpenFStringL( aHdrValue );
    CleanupClosePushL( valStr );
    THTTPHdrVal val(valStr);
    aHeaders.SetFieldL( iSession.StringPool().StringF( aHdrField, RHTTPSession::GetTable()), val);
    CleanupStack::PopAndDestroy();  // valStr
    LOG("CClientEngine::SetHeaderL ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::IssueHTTPGetL()
// ----------------------------------------------------------------------------
void CClientEngine::IssueHTTPGetL( const TDesC8& aUri, const TConnectionOption aOption )
    {
    LOG("CClientEngine::IssueHTTPGetL ,begin");
    if ( IsActive() )
        {
        // If there is some request in pending state
        // return with out further processing
        // Should we leave here !?
        LOG("Client engine is already active");
        return;
        }
    
    delete iUri;
    iUri = NULL;

    iUri = aUri.AllocL();

    // Create HTTP connection
    TRAPD( err, SetupConnectionL( aOption ) );
    //If the Err is KErrNone, It will lead to RunL and
    //hence jump to the DoHTTPGetL() from there.
    
    if( err == KErrAlreadyExists )
        {
        DoHTTPGetL();
        }
    else if( err != KErrNone )
        {
        LOG("Connection failure. Leaving.");
        iObserver.ClientEvent( EHttpConnectionFailure );
        User::Leave(err);
        }
    iEngineState = EGet;
 	LOG("CClientEngine::IssueHTTPGetL ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::DoHTTPGetL()
// ----------------------------------------------------------------------------
void CClientEngine::DoHTTPGetL()
    {
    LOG("CClientEngine::DoHTTPGetL ,begin");
    // Parse string to URI (as defined in RFC2396)
    TUriParser8 uri;
    uri.Parse( *iUri );
    
    // Get request method string for HTTP GET
    RStringF method = iSession.StringPool().StringF( HTTP::EGET,RHTTPSession::GetTable() );
    
    // Open transaction with previous method and parsed uri. This class will
    // receive transaction events in MHFRunL and MHFRunError.
    iTransaction = iSession.OpenTransactionL( uri, *this, method );
    iTransactionOpen = ETrue;
    
    // Set headers for request; user agent and accepted content type
    RHTTPHeaders hdr = iTransaction.Request().GetHeaderCollection();
    SetHeaderL( hdr, HTTP::EUserAgent, KUserAgent );
    SetHeaderL( hdr, HTTP::EAccept, KAccept );
    
    // Submit the transaction. After this the framework will give transaction
    // events via MHFRunL and MHFRunError.
    iTransaction.SubmitL();
    
    iObserver.ClientEvent( EHttpConnecting );
	LOG("CClientEngine::DoHTTPGetL ,end");
}

// ----------------------------------------------------------------------------
// CClientEngine::CancelTransaction()
// ----------------------------------------------------------------------------
void CClientEngine::CancelTransaction()
    {
    LOG("CClientEngine::CancelTransaction ,begin");
    iEngineState = EIdle;
    delete iUri; 
    iUri = NULL;
    
    // Close() also cancels transaction (Cancel() can also be used but
    // resources allocated by transaction must be still freed with Close())
    if( iTransactionOpen )
        {
        iTransaction.Close();
        iTransactionOpen = EFalse;

        iObserver.ClientEvent( EHttpTxCancelled );
        }
	LOG("CClientEngine::CancelTransaction ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::MHFRunL()
// ----------------------------------------------------------------------------
void CClientEngine::MHFRunL( RHTTPTransaction aTransaction, const THTTPEvent& aEvent )
    {
    LOG("CClientEngine::MHFRunL ,begin");
    switch ( aEvent.iStatus )
        {
        case THTTPEvent::EGotResponseHeaders:
            {
            // HTTP response headers have been received. Use
            // aTransaction.Response() to get the response. However, it's not
            // necessary to do anything with the response when this event occurs.
            iObserver.ClientEvent( EHttpHdrReceived );
            break;
            }
        case THTTPEvent::EGotResponseBodyData:
            {
            // Part (or all) of response's body data received. Use
            // aTransaction.Response().Body()->GetNextDataPart() to get the actual
            // body data.
        
            // Get the body data supplier
            MHTTPDataSupplier* body = aTransaction.Response().Body();
            TPtrC8 dataChunk;
        
            // GetNextDataPart() returns ETrue, if the received part is the last
            // one.
            TBool isLast = body->GetNextDataPart(dataChunk);
            iObserver.ClientBodyReceived(dataChunk);
        
            iObserver.ClientEvent( EHttpBytesReceieved );

            // NOTE: isLast may not be ETrue even if last data part received.
            // (e.g. multipart response without content length field)
            // Use EResponseComplete to reliably determine when body is completely
            // received.
            if( isLast )
                {
                iObserver.ClientEvent( EHttpBodyReceieved );
                }
            // Always remember to release the body data.
            body->ReleaseData();
            break;
            }
        case THTTPEvent::EResponseComplete:
            {
            // Indicates that header & body of response is completely received.
            // No further action here needed.

            iObserver.ClientEvent( EHttpTxCompleted );
            break;
            }
        case THTTPEvent::ESucceeded:
            {
            // Indicates that transaction succeeded.
            iObserver.ClientEvent( EHttpTxSuccess );
            // Transaction can be closed now. It's not needed anymore.
            aTransaction.Close();
            iTransactionOpen = EFalse;
            break;
            }
        case THTTPEvent::EFailed:
            {
            // Transaction completed with failure.
            iObserver.ClientEvent( EHttpTxFailed );
            aTransaction.Close();
            iTransactionOpen = EFalse;
            break;
            }
        default:
            // There are more events in THTTPEvent, but they are not usually
            // needed. However, event status smaller than zero should be handled
            // correctly since it's error.
            {
            if ( aEvent.iStatus < 0 )
                {
                iObserver.ClientEvent( EHttpConnectionFailure );
                // Close the transaction on errors
                aTransaction.Close();
                iTransactionOpen = EFalse;
                }
                break;
            }
        }
	LOG("CClientEngine::MHFRunL ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::MHFRunError()
// ----------------------------------------------------------------------------
TInt CClientEngine::MHFRunError( TInt /*aError*/, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/ )
    {
    LOG("CClientEngine::MHFRunError ,begin");
    // Just notify about the error and return KErrNone.
    CloseConnection();
    iObserver.ClientEvent(EHttpMhfRunError);
    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CClientEngine::PreferredCarrierAvailable()
// ----------------------------------------------------------------------------
void CClientEngine::PreferredCarrierAvailable( TAccessPointInfo /*aOldAPInfo*/,
                                               TAccessPointInfo /*aNewAPInfo*/,
                                               TBool /*aIsUpgrade*/,
                                               TBool aIsSeamless )
    {
    LOG("CClientEngine::PreferredCarrierAvailable ,begin");
    if( !aIsSeamless && iMobility)
        {
        iMobility->MigrateToPreferredCarrier();
        }
	LOG("CClientEngine::PreferredCarrierAvailable ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::NewCarrierActive()
// ----------------------------------------------------------------------------
void CClientEngine::NewCarrierActive( TAccessPointInfo /*aNewAPInfo*/,
                                      TBool aIsSeamless )
    {
    LOG("CClientEngine::NewCarrierActive ,begin");
    if( !aIsSeamless && iMobility)
        {
        iMobility->NewCarrierAccepted();
        }
	LOG("CClientEngine::NewCarrierActive ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::Error()
// ----------------------------------------------------------------------------
void CClientEngine::Error(TInt /*aError*/)
    {
    LOG("CClientEngine::Error");
    }

// ----------------------------------------------------------------------------
// CClientEngine::DoCancel()
// ----------------------------------------------------------------------------
void CClientEngine::DoCancel()
    {
    LOG("CClientEngine::DoCancel");
    iConnection.Stop();
    }

// ----------------------------------------------------------------------------
// CClientEngine::DoCancel()
// ----------------------------------------------------------------------------
TInt  CClientEngine::RunError(TInt /*aError*/)
    {
    LOG("CClientEngine::RunError");
    // Just notify about the error and return KErrNone.
    CloseConnection();
    iObserver.ClientEvent( EHttpTxFailed );
    return KErrNone;
    }
// ----------------------------------------------------------------------------
// CClientEngine::RunL()
// ----------------------------------------------------------------------------
void CClientEngine::RunL()
    {
    LOG1("CClientEngine::RunL: error is: %d\n", iStatus.Int() );
    TInt statusCode = iStatus.Int();
    if ( statusCode == KErrNone )
        {
        // Connection done ok
        iConnectionSetupDone = ETrue;
        
        RStringPool strPool = iSession.StringPool();

        // Remove first session properties just in case.
        RHTTPConnectionInfo connInfo = iSession.ConnectionInfo();
        
        // Clear RConnection and Socket Server instances
        connInfo.RemoveProperty(strPool.StringF(HTTP::EHttpSocketServ,RHTTPSession::GetTable()));
        connInfo.RemoveProperty(strPool.StringF(HTTP::EHttpSocketConnection,RHTTPSession::GetTable()));
        
        // Clear the proxy settings
        connInfo.RemoveProperty(strPool.StringF(HTTP::EProxyUsage,RHTTPSession::GetTable()));
        connInfo.RemoveProperty(strPool.StringF(HTTP::EProxyAddress,RHTTPSession::GetTable()));
        
        // RConnection and Socket Server
        connInfo.SetPropertyL ( strPool.StringF(HTTP::EHttpSocketServ, 
                                        RHTTPSession::GetTable()), 
                                THTTPHdrVal (iSocketServ.Handle()) );
        
        TInt connPtr1 = REINTERPRET_CAST(TInt, &iConnection);
        connInfo.SetPropertyL ( strPool.StringF(HTTP::EHttpSocketConnection, 
                                RHTTPSession::GetTable() ), THTTPHdrVal (connPtr1) );    

        // Register for mobility API
		if(iMobility)
			{
	        delete iMobility;
			iMobility = NULL ;
			}
        iMobility = CActiveCommsMobilityApiExt::NewL( iConnection, *this );
        // Start selected HTTP action
        switch( iEngineState )
            {
            case EIdle:
                {
                //
                CancelTransaction();
                break;
                }
            case EGet:
               {
               DoHTTPGetL();
               break;
               }
            };
        }
    else
        {
        //handle error
        if ( statusCode == KErrPermissionDenied )
            {
            iObserver.ClientEvent( EHttpAuthFailed );
            }
        else
            {
            //Throw some general Transaction falure error!
            iObserver.ClientEvent( EHttpTxFailed );
            }
        CloseConnection();
        }
	LOG("CClientEngine::RunL ,end");
    }

// ----------------------------------------------------------------------------
// CClientEngine::SilentConnectionAllowed()
// ----------------------------------------------------------------------------
TBool CClientEngine::SilentConnectionAllowed()
    {
    LOG("CClientEngine::SilentConnectionAllowed ,begin");
    TBool retVal = EFalse;
	TRAPD(err, retVal = IsDataConnectionAskAlwaysL());
	if(err == KErrNone)
		{
		// data connection is always ask... Silent connection is not allowed
		retVal = !retVal;
		}
	LOG1("CClientEngine::SilentConnectionAllowed ,end. Ret - %d", retVal);
    return retVal;
    }

// End of file

