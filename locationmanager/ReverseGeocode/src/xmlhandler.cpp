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
* Description: Implementation of the Parser 
*/




#include <utf.h>
#include "internaladdressinfo.h"
#include "xmlhandler.h"
#include "locationmanagerdebug.h"

using namespace xmlhandler;

// CONSTANTS
_LIT8( KXmlMimeType, "text/xml" );
_LIT8( KCountry, "country" );
_LIT8( KState, "state" );
_LIT8( KDistrict, "district" );
_LIT8( KCity, "city" );
_LIT8( KPostalCode, "postCode" );
_LIT8( KThoroughfare, "thoroughfare" );
_LIT8( KNameTag, "name" );
_LIT8( KNumberTag, "number" );

// METHODS DEFINITION

// --------------------------------------------------------------------------
// CXmlHandler::NewL
// --------------------------------------------------------------------------
CXmlHandler* CXmlHandler::NewL( MXmlHandlerObserver& aObserver,  CInternalAddressInfo *aAddressInfo )
    {
    LOG("CXmlHandler::NewL ,begin");
    CXmlHandler* self = CXmlHandler::NewLC( aObserver, aAddressInfo );
    CleanupStack::Pop(); //self
    return self;
    }

// --------------------------------------------------------------------------
// CXmlHandler::NewLC
// --------------------------------------------------------------------------

CXmlHandler* CXmlHandler::NewLC( MXmlHandlerObserver& aObserver,  CInternalAddressInfo *aAddressInfo  )
    {
    LOG("CXmlHandler::NewLC ,begin");
    CXmlHandler* self = new ( ELeave ) CXmlHandler( aObserver, aAddressInfo );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// --------------------------------------------------------------------------
// CXmlHandler::~CXmlHandler
// --------------------------------------------------------------------------
CXmlHandler::~CXmlHandler()
    {
    LOG("CXmlHandler::~CXmlHandler");
    delete iParser;
    delete iBuffer;
    }

// --------------------------------------------------------------------------
// CXmlHandler::CXmlHandler
// --------------------------------------------------------------------------
CXmlHandler::CXmlHandler( MXmlHandlerObserver& aObserver, CInternalAddressInfo *aAddressInfo  ):
    iObserver( aObserver ),
    iParser( NULL ),
    iBuffer( NULL ),
    iAddressInfo( aAddressInfo ),
    iThoroughfare( EFalse )
    {
    }

// --------------------------------------------------------------------------
// CXmlHandler::ConstructL
// --------------------------------------------------------------------------
void CXmlHandler::ConstructL()
    {
    LOG("CXmlHandler::ConstructL ,begin");
    iParser = CParser::NewL( KXmlMimeType, *this );
 
    }

// --------------------------------------------------------------------------
// CXmlHandler::StartParsingL()
// --------------------------------------------------------------------------   
void CXmlHandler::StartParsingL( HBufC8 *aBuf )
    {
	LOG("CXmlHandler::StartParsingL ,begin");
     //Reset the address values before starting new content parsing
     iAddressInfo->ResetAddressInfoL();
      
    if( iBuffer )
        {
        delete iBuffer;
        iBuffer = NULL;
        }

    iBuffer = HBufC8::NewL( aBuf->Size() );
    TPtr8 ptr = iBuffer->Des();
    ptr.Copy( aBuf->Ptr() ,  aBuf->Size() );
    
    // Now, we have the whole file content in iBuffer.
    // We are ready to parse the XML content.
    iParser->ParseBeginL();
    iParser->ParseL( *iBuffer );
    
    // Since we read the whole file contents within one-shot,
    // we can call ParseEndL() right after calling ParseL().
    iParser->ParseEndL();
	LOG("CXmlHandler::StartParsingL ,end");
    }

// --------------------------------------------------------------------------
// CXmlHandler::OnStartDocumentL()
// --------------------------------------------------------------------------
void CXmlHandler::OnStartDocumentL( const RDocumentParameters& /*aDocParam*/,
                                    TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnStartDocumentL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
	LOG("CXmlHandler::OnStartDocumentL ,end");
    }

// --------------------------------------------------------------------------
// CXmlHandler::OnEndDocumentL()
// --------------------------------------------------------------------------    
void CXmlHandler::OnEndDocumentL( TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnEndDocumentL ,Errcode - %d", aErrorCode);
    iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
    }

// --------------------------------------------------------------------------
// CXmlHandler::OnStartElementL()
// --------------------------------------------------------------------------
void CXmlHandler::OnStartElementL( const RTagInfo& aElement,
                                   const RAttributeArray& /*aAttributes*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnStartElementL , Errorcode - %d", aErrorCode);
    if ( KErrNone == aErrorCode )
        {
        // If we find the start of an element, we write to the screen,
        // for example: "<tag>"
        
        if( !aElement.LocalName().DesC().Compare( KCountry ) )
            {
            iCurrentElement = ECountryName;
            }
        else if( !aElement.LocalName().DesC().Compare( KState ) )
            {
            iCurrentElement = EState;
            }
        else if( !aElement.LocalName().DesC().Compare( KCity ) )
            {
            iCurrentElement = ECity;
            }
        else if( !aElement.LocalName().DesC().Compare( KDistrict ) )
            {
            iCurrentElement = EDistrict;
            }
        else if( !aElement.LocalName().DesC().Compare( KPostalCode )  )
            {
            iCurrentElement = EPostalCode;
            }
        else if( !aElement.LocalName().DesC().Compare( KThoroughfare )  )
            {
            iThoroughfare = ETrue;
            }
        else if( !aElement.LocalName().DesC().Compare( KNameTag ) && iThoroughfare )
            {
            iCurrentElement = EThoroughfareName;
            }
        else if( !aElement.LocalName().DesC().Compare( KNumberTag ) && iThoroughfare )
            {
            iCurrentElement = EThoroughfareNumber;
            }
        else
            {
            ///Do something
            }
        }
    else
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
	LOG("CXmlHandler::OnStartElementL ,end");
    }
        
// --------------------------------------------------------------------------
// CXmlHandler::OnEndElementL()
// --------------------------------------------------------------------------
void CXmlHandler::OnEndElementL( const RTagInfo& /*aElement*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnEndElementL ,Error code - %d", aErrorCode);
    if( KErrNone == aErrorCode )
        {
        // at the end of the tag </tag>
        //Set it to ENone
        iCurrentElement = ENone;
        iThoroughfare = EFalse;
        }
    else
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    }
    
// --------------------------------------------------------------------------
// CXmlHandler::OnContentL()
// --------------------------------------------------------------------------
void CXmlHandler::OnContentL( const TDesC8 &aBytes, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnContentL ,Errorcode - %d", aErrorCode);
    if( KErrNone == aErrorCode )
        {

        if( iCurrentElement == ENone )
            {
            //ignore if the current element is not there
            return;
            }

        // convert the content to UCS-2
        // from UTF-8        
        RBuf buffer;
        buffer.CreateL( aBytes.Length() );
        CleanupClosePushL(buffer);
        CnvUtfConverter::ConvertToUnicodeFromUtf8( buffer , aBytes );
        
        if( iCurrentElement == ECountryName )
            {
            iAddressInfo->SetCountryName( buffer );
            }
        else if( iCurrentElement == EState )
            {
            iAddressInfo->SetState( buffer );
            }
        else if( iCurrentElement == EDistrict )
            {
            iAddressInfo->SetDistrict( buffer );
            }
        else if( iCurrentElement == ECity )
            {
            iAddressInfo->SetCity( buffer );
            }
        else if( iCurrentElement == EPostalCode )
            {
            iAddressInfo->SetPincode( buffer );
            }
        else if( iCurrentElement == EThoroughfareName )
            {
            iAddressInfo->SetThoroughfareName( buffer );
            }
        else if( iCurrentElement == EThoroughfareNumber )
            {
            iAddressInfo->SetThoroughfareNumber( buffer );
            }
        else
            {
                 ///Do something
            }
        CleanupStack::PopAndDestroy(); // buffer
        }
    else
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
	LOG("CXmlHandler::OnContentL ,end");
    }
    
// --------------------------------------------------------------------------
// CXmlHandler::OnStartPrefixMappingL()
// --------------------------------------------------------------------------
void CXmlHandler::OnStartPrefixMappingL( const RString& /*aPrefix*/,
                                         const RString& /*aUri*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnStartPrefixMappingL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
    }
        
// --------------------------------------------------------------------------
// CXmlHandler::OnEndPrefixMappingL()
// --------------------------------------------------------------------------
void CXmlHandler::OnEndPrefixMappingL( const RString& /*aPrefix*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnEndPrefixMappingL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
    }
    
// --------------------------------------------------------------------------
// CXmlHandler::OnIgnorableWhiteSpaceL()
// --------------------------------------------------------------------------
void CXmlHandler::OnIgnorableWhiteSpaceL( const TDesC8& /*aBytes*/,TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnIgnorableWhiteSpaceL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
    }
    
// --------------------------------------------------------------------------
// CXmlHandler::OnSkippedEntityL()
// --------------------------------------------------------------------------
void CXmlHandler::OnSkippedEntityL( const RString& /*aName*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnSkippedEntityL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
    }

// --------------------------------------------------------------------------
// CXmlHandler::OnProcessingInstructionL()
// --------------------------------------------------------------------------
void CXmlHandler::OnProcessingInstructionL( const TDesC8& /*aTarget*/, const TDesC8& /*aData*/, TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnProcessingInstructionL ,Error code - %d", aErrorCode);
    if( KErrNone != aErrorCode )
        {
        iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo );
        }
    else
        {
        // Do nothing
        }
    }

// --------------------------------------------------------------------------
// CXmlHandler::OnError()
// --------------------------------------------------------------------------
void CXmlHandler::OnError( TInt aErrorCode )
    {
    LOG1("CXmlHandler::OnError ,Error code - %d", aErrorCode);
    TRAP_IGNORE( iObserver.OnParseCompletedL( aErrorCode, *iAddressInfo ) );
    }

// --------------------------------------------------------------------------
// CXmlHandler::GetExtendedInterface()
// --------------------------------------------------------------------------
TAny* CXmlHandler::GetExtendedInterface( const TInt32 /*aUid*/ )
    {
    return 0;
    }

// End of File
