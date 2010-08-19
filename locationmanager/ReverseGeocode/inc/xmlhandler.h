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
* Description: XMLHandler class to parse the resultant reverse geocoded
*              information.
*/

#ifndef __XMLHANDLER_H__
#define __XMLHANDLER_H__

// INCLUDE FILES
#include <e32base.h>
#include <xml/contenthandler.h> // for mcontenthandler
#include <xml/parser.h> // for cparser
using namespace Xml;

//Forward declarations
class CAddressInfo;
class CInternalAddressInfo;

/*
 * Category of the address information. This is used for
 * internal parsing procedure.
 */
namespace xmlhandler
    {
    enum TLocationInfoType
        {
        ENone = 0,
        ECountryName,
        EState,
        ECity,
        EDistrict,
        EPostalCode,
        EThoroughfareName,
        EThoroughfareNumber
        };
    }

/**
 * MXmlHandlerObserver, an observer to CXmlHandler class.
 */
class MXmlHandlerObserver
    {
    public:
    /*
     * Signifies the completion of parsing of the output data.
     * 
     * @param aError error status of parsing part.
     * @param aLocationInfo reference to the resultant address information structure.
     */    
    virtual void OnParseCompletedL( TInt aError, MAddressInfo& aLocationInfo ) = 0;
    };

/**
 * CXmlHandler, a class to parse XML file and then output log information
 * to a buffer.
 */
NONSHARABLE_CLASS( CXmlHandler ) : public MContentHandler
    {
    public: // Constructors and destructor

	/**
	* 1st phase constructor
	*
	* @param aObserver The observer class to be notified after xml parsing is done
	* 		  aAddressInfo   The address info 
	*/
    static CXmlHandler* NewL( MXmlHandlerObserver& aObserver, CInternalAddressInfo *aAddressInfo );

	/**
	* 1st phase constructor pushes the object into cleanup stack
	* @param aObserver The observer class to be notified after xml parsing is done
	* 		  aAddressInfo   The address info 
	*/	
    static CXmlHandler* NewLC( MXmlHandlerObserver& aObserver, CInternalAddressInfo *aAddressInfo  );

	/**
	* Destructor
	*/	
    virtual ~CXmlHandler();
    
    public: // Public methods

	/**
	* Starts parsing the xml content
	* @param aBuf The xml data 
	*/

    void StartParsingL(  HBufC8 *aBuf  );
    

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 

	/**
	* @param aObserver The observer class to be notified after xml parsing is done
	* @param aAddressInfo   The address info 
	*/

    CXmlHandler( MXmlHandlerObserver& aObserver, CInternalAddressInfo *aAddressInfo );

	/**
	* Second phase construction
	*/	
    void ConstructL();
    

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private:
#endif 
    // from MContentHandler

	/**
	* This method is a callback to indicate the start of the document
	* @param aDocParam 	Specifies the various parameters of the document. 
	* @param aErrorCode   The error code. If this is not KErrNone then special action may be required. 
	*
	*/	
    void OnStartDocumentL( const RDocumentParameters &aDocParam, TInt aErrorCode );

	/**
	* This method is a callback to indicate the end of the document
	* @param  aErrorCode   The error code. If this is not KErrNone then special action may be required. 
	*/	
    void OnEndDocumentL( TInt aErrorCode );

	/**
	* This method is a callback to indicate an element has been parsed.
	* @param aElement    Is a handle to the element's details. 
	* @param aAttributes   Contains the attributes for the element	
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required
	*/	
    void OnStartElementL( const RTagInfo &aElement, const RAttributeArray &aAttributes, TInt aErrorCode );

	/**
	* This method is a callback to indicate the end of the element has been reached. 
	* @param aElement    Is a handle to the element's details. 
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required
	*/
	
    void OnEndElementL( const RTagInfo &aElement, TInt aErrorCode );

	/**
	* This method is a callback that sends the content of the element
	* @param aBytes  the raw content data for the element
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnContentL( const TDesC8 &aBytes, TInt aErrorCode );

	/**
	* This method is a notification of the beginning of the scope of a prefix-URI Namespace mapping
	* @param aPrefix     the Namespace prefix being declared
	* @param aUri         the Namespace URI the prefix is mapped to
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnStartPrefixMappingL( const RString &aPrefix, const RString &aUri, TInt aErrorCode );

	/**
	* This method is a notification of the end of the scope of a prefix-URI mapping
	* @param aPrefix  the Namespace prefix being declared
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnEndPrefixMappingL( const RString &aPrefix, TInt aErrorCode );

	/**
	* This method is a notification of ignorable whitespace in element content
	* @param aBytes    the ignored bytes from the document being parsed
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnIgnorableWhiteSpaceL( const TDesC8 &aBytes, TInt aErrorCode );

	/**
	* This method is a notification of a skipped entity
	* @param aName  the name of the skipped entity. 
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnSkippedEntityL( const RString &aName, TInt aErrorCode );

	/**
	* This method is a receive notification of a processing instruction.
	* @param aTarget the processing instruction target
	* @param aData   the processing instruction data
	* @param aErrorCode  The error code. If this is not KErrNone then special action may be required	
	*/	
    void OnProcessingInstructionL( const TDesC8 &aTarget, const TDesC8 &aData, TInt aErrorCode);

	/**
	* This method indicates an error has occurred
	* @param  aErrorCode  The error code. 
	*/	
    void OnError( TInt aErrorCode );

	/**
	* This method obtains the interface matching the specified uid. 
	* @param aUid  the uid identifying the required interface
	*/    
    TAny *GetExtendedInterface( const TInt32 aUid );

#ifdef REVERSEGEOCODE_UNIT_TESTCASE
    public:
#else    
    private: // Private data
#endif     

    MXmlHandlerObserver& iObserver;
    CParser*             iParser;
    HBufC8*              iBuffer;
    CInternalAddressInfo *iAddressInfo;
    xmlhandler::TLocationInfoType     iCurrentElement;
    TBool iThoroughfare ;
    };

#endif /* __XMLHANDLER_H__ */

// End of File
