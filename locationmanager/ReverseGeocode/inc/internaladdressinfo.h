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
* Description: Implementation class from the CAddressInfo abstract class.
*/

#ifndef INTERNALADDRESSINFO_H_
#define INTERNALADDRESSINFO_H_

#include "reversegeocode.h"
/*
 * Internal implementation class for theCAddressInfo abstract class.
 */
NONSHARABLE_CLASS( CInternalAddressInfo ) : public MAddressInfo
    {
    public:
    //from CAddressInfo class

	/**
	  * Gets the country name
	  * @return The country name
	  */
    virtual TDesC& GetCountryName();

	/**
	  * Gets the state name
	  * @return The state name
	  */	
    virtual TDesC& GetState();

	/**
	  * Gets the city name
	  * @return The city name
	  */	
    virtual TDesC& GetCity();

	/**
	  * Gets the district name
	  * @return The district name
	  */	
    virtual TDesC& GetDistrict();

	/**
	  * Gets the pincode
	  * @return The cpincode
	  */	
    virtual TDesC& GetPincode();

	/**
	  * Gets the Thoroughfare name
	  * @return The Thoroughfare name
	  */	
    virtual TDesC& GetThoroughfareName();

	/**
	  * Gets the ThoroughfareNumber
	  * @return The ThoroughfareNumber
	  */	
    virtual TDesC& GetThoroughfareNumber();
          
    protected:

	/**
	  * Performs the secondphase construction
	  */
    void ConstructL();

	/**
	  * Constructor
	  */    
    CInternalAddressInfo();

    public:
   

	/**
	  * Factory function to create the instance
	  * @return A pointer to the CInternalAddressInfo
	  */
    static CInternalAddressInfo* NewL();
    

	/**
	  * Destructor
	  */
    ~CInternalAddressInfo();
      
    // internal setter utilities to
    // set the different attributes of the address.

	/**
	  * Sets the country name
	  * @param aCountryName The country name
	  */		
    void SetCountryName( const TDesC& aCountryName );

	/**
	  * Sets the state name
	  * @param aStateName The state name
	  */		
    void SetState( const TDesC& aStateName );

	/**
	  * Sets the City name
	  * @param aCityName The City name
	  */		
    void SetCity( const TDesC& aCityName );

	/**
	  * Sets the District name
	  * @param aDistrictName The District name
	  */		
    void SetDistrict( const TDesC& aDistrictName );

	/**
	  * Sets the Pincode 
	  * @param aPincode The Pincode\
	  */		
    void SetPincode( const TDesC& aPincode );

	/**
	  * Sets the Thoroughfare name
	  * @param aTFName The Thoroughfare name
	  */		
    void SetThoroughfareName( const TDesC& aTFName );
	/**
	  * Sets the ThoroughfareNumber
	  * @param  aTFNumber The ThoroughfareNumber
	  */		
    void SetThoroughfareNumber( const TDesC& aTFNumber );

	/**
	  * Resets the address info
	  */		
    void ResetAddressInfoL();
          
    private:
    HBufC* iCountryName;
    HBufC* iState;
    HBufC* iCity;
    HBufC* iDistrict;
    HBufC* iPin;
    HBufC* iTFName;
    HBufC* iTFNumber;
    };
#endif /* INTERNALADDRESSINFO_H_ */

// End of file
