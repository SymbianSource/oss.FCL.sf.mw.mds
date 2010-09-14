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
* Description: Implementation of reversegeocodea achieved using HTTP get
* request to the Maps server.
*/

#include "internalreversegeocode.h"
#include "internaladdressinfo.h"
#include "locationmanagerdebug.h"


//Maximum length for the response buffer size
const int KMaxResponseLength = 2048;

//Language option for the REST request
_LIT8( KDefaultLanguage, "eng" );


//Format of the HTTP request for the reverse geocode
_LIT8( KRequestFormat, "http://loc.mobile.maps.svc.ovi.com/geocoder/rgc/1.0?n=10&lat=%f&long=%f&lg=%S&output=xml" );


// http://www.loc.gov/marc/languages/
// These are the nokia language id converted to MARC language strings.
 static const char Marc_Table[ 104 ][ 4 ] = {
        "   ", // dummy
        "ENG", // 1 English
        "FRE", // 2 French
        "GER", // 3 German
        "SPA", // 4 Spanish
        "ITA", // 5 Italian
        "SWE", // 6 Swedish
        "DAN", // 7 Danish
        "NOR", // 8 Norwegian
        "FIN", // 9 Finnish
        "010", // 10 American
        "011", // 11 Swiss French
        "GSW", // 12 Swiss German
        "POR", // 13 Portuguese
        "TUR", // 14 Turkish
        "ICE", // 15 Icelandic
        "RUS", // 16 Russian
        "HUN", // 17 Hungarian
        "DUT", // 18 Dutch
        "019", // 19 Flemish
        "020", // 20 Australian English
        "021", // 21 Belgian French
        "022", // 22 Austrian German
        "023", // 23 New Zealand English
        "FRE", // 24 International French
        "CZE", // 25 Czech
        "SLO", // 26 Slovak
        "POL", // 27 Polish
        "SLV", // 28 Slovenian
        "029", // 29 TaiwanChinese
        "CHT", // 30 HongKongChinese
        "CHI", // 31 PeoplesRepublicOfChina Chinese
        "JPN", // 32 Japanese
        "THA", // 33 Thai
        "AFR", // 34 Afrikaans
        "ALB", // 35 Albanian
        "AMH", // 36 Amharic
        "ARA", // 37 Arabic
        "ARM", // 38 Armenian
        "TGL", // 39 Tagalog
        "BEL", // 40 Belarusian
        "BEN", // 41 Bengali
        "BUL", // 42 Bulgarian
        "BUR", // 43 Burmese
        "CAT", // 44 Catalan
        "SCR", // 45 Croatian
        "046", // 46 Canadian English
        "ENG", // 47 International English
        "048", // 48 SouthAfrican English
        "EST", // 49 Estonian
        "PER", // 50 Persian (Farsi)
        "051", // 51 Canadian French
        "GAE", // 52 Scots Gaelic
        "GEO", // 53 Georgian
        "GRE", // 54 Greek
        "055", // 55 Cyprus Greek
        "GUJ", // 56 Gujarati
        "HEB", // 57 Hebrew
        "HIN", // 58 Hindi
        "IND", // 59 Bahasa indonesia
        "GLE", // 60 Irish
        "061", // 61 Swiss Italian
        "KAN", // 62 Kannada
        "KAZ", // 63 Kazakh
        "KHM", // 64 Khmer
        "KOR", // 65 Korean
        "LAO", // 66 Lao
        "LAV", // 67 Latvian
        "LIT", // 68 Lithuanian
        "MAC", // 69 Macedonian
        "070", // 70 Bahasa Malaysia
        "MAL", // 71 Malayalam
        "MAR", // 72 Marathi
        "MOL", // 73 Moldavian
        "MON", // 74 Mongolian
        "NNO", // 75 Norwegian Nynorsk
        "076", // 76 Brazilian Portuguese
        "PAN", // 77 Punjabi
        "RUM", // 78 Romanian
        "SCC", // 79 Serbian
        "SNH", // 80 Sinhalese
        "SOM", // 81 Somali
        "082", // 82 International Spanish
        "083", // 83 LatinAmerican Spanish
        "SWA", // 84 Swahili
        "085", // 85 Finland Swedish
        "TAJ", // 86 Tajik
        "TAM", // 87 Tamil
        "TEL", // 88 Telugu
        "TIB", // 89 Tibetan
        "TIR", // 90 Tigrinya
        "091", // 91 Cyprus Turkish
        "TUK", // 92 Turkmen
        "UKR", // 93 Ukrainian
        "URD", // 94 Urdu
        "UZB", // 95 Uzbek
        "VIE", // 96 Vietnamese
        "WEL", // 97 Welsh
        "ZUL", // 98 Zulu
        "UND", // 99 Other
        "UND", // 100 Undef
        "UND", // 101 Undef
        "BAQ", // 102 Basque
        "103", // 103 Galician
    };

 // Timer interval
 const TInt KInterval = 15000000;  // 15 seconds


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::NewL()
// Factory function to create the instance
// ----------------------------------------------------------------------------
CInternalReverseGeocode* CInternalReverseGeocode::NewL( MReverseGeocodeObserver& aObserver )
    {
        LOG( "CInternalReverseGeocode::NewL,begin" );
        CInternalReverseGeocode *self = new (ELeave) CInternalReverseGeocode( aObserver );
        CleanupStack::PushL( self );
        self->ConstructL();
        
        CleanupStack::Pop( self );
        return self;
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::CInternalReverseGeocode()
// Constructor
// ---------------------------------------------------------------------------- 
CInternalReverseGeocode::CInternalReverseGeocode( MReverseGeocodeObserver& aObserver ):
                            iXmlHandler ( NULL ),
                            iClientEngine ( NULL ),
                            iXMLBuf ( NULL ),
                            iObserver( aObserver ),
                            iTimer( NULL ),
                            iStartTimerFlag(EFalse)
    {
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::~CInternalReverseGeocode()
// Destructor
// ---------------------------------------------------------------------------- 
CInternalReverseGeocode::~CInternalReverseGeocode()
    {
    LOG( "CInternalReverseGeocode::~CInternalReverseGeocode,begin" );
    if ( iTimer)
        {
        iTimer->Cancel();
        delete iTimer;
        iTimer = NULL;
        }
    delete iXMLBuf;
    iXMLBuf = NULL;
    delete iXmlHandler;
    iXmlHandler = NULL;
    delete iClientEngine;
    iClientEngine = NULL;
    delete iAddressInfo;
    iAddressInfo = NULL;
    
    LOG( "CInternalReverseGeocode::~CInternalReverseGeocode,end" );  
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::GetAddressByCoordinateL()
// Gets the address for the given geo-coordinaates.
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::GetAddressByCoordinateL( TLocality aLocality, 
                                                      const TConnectionOption aOption )
    {
    LOG("CInternalReverseGeocode::GetAddressByCoordinateL ,begin");
    TReal64 latitude = aLocality.Latitude();
    TReal64 longitude = aLocality.Longitude();

    GetLanguageForTheRequest( iLang );
        
    //Form the request URI
    iQueryString.Format( KRequestFormat, latitude, longitude, &iLang );
    TInt err = KErrNone;
    TRAP(err, iClientEngine->IssueHTTPGetL( iQueryString, aOption ));
        
    if ( iTimer && iTimer->IsActive() )
        {
        iTimer->Cancel();
        }
    if(err == KErrNone || err == KErrNotSupported)
        {
        // connection is closed because of data usage is set to manual
        iStartTimerFlag = EFalse;
        }
    LOG("CInternalReverseGeocode::GetAddressByCoordinateL ,end");
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::ConstructL()
// second phase construction.
// ---------------------------------------------------------------------------- 
void CInternalReverseGeocode::ConstructL()
    {
    LOG( "CInternalReverseGeocode::ConstructL,begin" );
    iXMLBuf = HBufC8::NewL( KMaxResponseLength );
    
    //Address Info
    iAddressInfo = CInternalAddressInfo::NewL();

    iXmlHandler = CXmlHandler::NewL( *this, iAddressInfo );
        
    iClientEngine = CClientEngine::NewL( *this );
        
    iTimer = CConnectionTimerHandler::NewL(*this);

    LOG( "CInternalReverseGeocode::ConstructL,end" );		
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::OnParseCompletedL()
// callback which notifys the completion of parsing.
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::OnParseCompletedL( TInt aError, MAddressInfo& aLocationInfo )
    {
    LOG( "CInternalReverseGeocode::OnParseCompletedL,begin" );
    ARG_USED(aLocationInfo);
    iStartTimerFlag = ETrue;
    iObserver.ReverseGeocodeComplete( aError, *iAddressInfo );
    if(iStartTimerFlag)
        {
        StartTimer();
        }
    LOG( "CInternalReverseGeocode::OnParseCompletedL,end" );
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::ClientEvent()
// callback which notifys progess of HTTP request
// ---------------------------------------------------------------------------- 
void CInternalReverseGeocode::ClientEvent( const THttpStatus& aEvent )
    {
    LOG1( "CInternalReverseGeocode::ClientEvent,begin. Err - %d", aEvent);
    TInt err = KErrNone;
    //Have a Switch here
    switch( aEvent )
        {
        case EHttpTxCompleted:
            //Reading is done
            //Parse the String and get the restults
            if( iXmlHandler )
            {
             TRAP_IGNORE( iXmlHandler->StartParsingL( iXMLBuf ) );
            }
            break;
        //All these cases will in turn lead to
        //generic failure due to connection/Tx related problems    
        case EHttpConnectionFailure:
            // May happen if Socket connection fails
            err = KErrCouldNotConnect;
            iObserver.ReverseGeocodeComplete( err , *iAddressInfo );
            break;
        case EHttpTxFailed:
        case EHttpMhfRunError:
            // May happen if Socket connection fails
            // Complete the RGC with generic error.
            err = KErrGeneral;
            iObserver.ReverseGeocodeComplete( err , *iAddressInfo );
            break;
        case EHttpTxCancelled:
            //On Cancellation of request.
            err = KErrCancel;
            iObserver.ReverseGeocodeComplete( err , *iAddressInfo );
            break;
        case EHttpAuthFailed:
            //On Cancellation of request.
            err = KErrPermissionDenied;
            iObserver.ReverseGeocodeComplete( err , *iAddressInfo );
            break;
        }
    if(err != KErrNone)
        {
        LOG("Error occur while getting data.");
        StartTimer();
        }
	LOG( "CInternalReverseGeocode::ClientEvent,end" );
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::ClientBodyReceived()
// callback through which the HTTP body data is recieved. 
// ---------------------------------------------------------------------------- 
void CInternalReverseGeocode::ClientBodyReceived( const TDesC8& aBodyData )
    {
    LOG( "CInternalReverseGeocode::ClientBodyReceived" );
    //Dump the contents here
    TPtr8 ptr = iXMLBuf->Des();
    ptr.Zero();
    ptr.Append( aBodyData );
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::GetLanguageForTheRequest()
// Gets the appropriate language based on the Phone language setting 
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::GetLanguageForTheRequest( TDes8& aLanguage )
    {
     LOG( "CInternalReverseGeocode::GetLanguageForTheRequest,begin" );    
	//get the current phone langauge  
	TInt phoneLangIndex = User::Language();

	// Get the converted language 
	if (  phoneLangIndex < sizeof( Marc_Table ) / sizeof( Marc_Table[ 0 ]  ) )
	    {
	    aLanguage = (const TUint8*) Marc_Table[ phoneLangIndex ];
	    }
	else
	    {
	     //By default language will be Eng
	    aLanguage.Copy( KDefaultLanguage );
	    }
	LOG( "CInternalReverseGeocode::GetLanguageForTheRequest,begin" );
    }


// ----------------------------------------------------------------------------
// CInternalReverseGeocode::CloseConnection()
// Closes the http connection and notifies the observer
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::CloseConnection()
    {
    LOG( "CInternalReverseGeocode::CloseConnection ,begin" );
    if(iClientEngine)
        {
        iClientEngine->CloseConnection();
        LOG( "Connection closed\n" );
        TInt err = KErrCouldNotConnect;
        iObserver.ReverseGeocodeComplete( err , *iAddressInfo );
        }
	LOG( "CInternalReverseGeocode::CloseConnection,end" );
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::StartTimer()
// starts the timer
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::StartTimer()
    {
    LOG( "CInternalReverseGeocode::StartTimer ,begin" );
    if(iTimer)
        {
        iTimer->StartTimer( KInterval);
        LOG( "Timer started" );
        }
    LOG( "CInternalReverseGeocode::StartTimer,end" );
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::HandleTimedoutEvent()
// Closes the connection once it times out
// ----------------------------------------------------------------------------
void CInternalReverseGeocode::HandleTimedoutEvent(TInt aErrorCode)
    {
    LOG( "CInternalReverseGeocode::HandleTimedoutEvent" );
    ARG_USED(aErrorCode);
    CloseConnection();
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::SilentConnectionAllowed()
// Checks if a silent connection is allowed
// ---------------------------------------------------------------------------- 
TBool CInternalReverseGeocode::SilentConnectionAllowed()
    {
    LOG( "CInternalReverseGeocode::SilentConnectionAllowed ,begin" );
    TBool retVal = EFalse;
    if(iClientEngine)
        {
        retVal = iClientEngine->SilentConnectionAllowed();
        }
    LOG1("Silent connection allowed ,end- %d", (TInt)retVal);
    return retVal;
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::GetCurrentRegisterNw()
// ---------------------------------------------------------------------------- 
RMobilePhone::TMobilePhoneNetworkInfoV2& CInternalReverseGeocode::GetCurrentRegisterNw()
    {
    LOG( "CInternalReverseGeocode::GetCurrentRegisterNw ,begin" );
    return iObserver.GetCurrentRegisterNw();
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::IsRegisteredAtHomeNetwork()
// ---------------------------------------------------------------------------- 
TBool CInternalReverseGeocode::IsRegisteredAtHomeNetwork()
    {
    LOG( "CInternalReverseGeocode::IsRegisteredAtHomeNetwork" );
    return iObserver.IsRegisteredAtHomeNetwork();
    }

// ----------------------------------------------------------------------------
// CInternalReverseGeocode::GetHomeNetworkInfo()
// ----------------------------------------------------------------------------
const RMobilePhone::TMobilePhoneNetworkInfoV1& 
        CInternalReverseGeocode::GetHomeNetworkInfo(TBool& aHomeNwInfoAvailableFlag)
    {
    LOG( "CInternalReverseGeocode::GetHomeNetworkInfo" );
    return iObserver.GetHomeNetworkInfo(aHomeNwInfoAvailableFlag);
    }


//end of file

