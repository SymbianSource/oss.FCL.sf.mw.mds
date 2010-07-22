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
* Description:  Reverse Geocode utility which converts the geo coordinates in to
* the actual address information.
*/

#ifndef REVERSEGEOCODE_H_
#define REVERSEGEOCODE_H_

#include <e32base.h> 
#include <lbsposition.h> 
#include <etel.h>
#include <etelmm.h>
#include "geotagger.h"

/*
 * Data class to get the address details. An handle to this type will be given to the
 * user through MReverseGeocodeObserver::ReverseGeocodeComplete callback, through which
 * user can retrieve the address details using the following interfaces.
 *
 **/
class MAddressInfo
    {
    public:
    /*
     * Gets the reference to the country name. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the Country Name. 
     */
    virtual TDesC& GetCountryName()= 0;
    
    /*
     * Gets the reference to the State. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the State name. 
     */
    virtual TDesC& GetState()= 0;
    
    /*
     * Gets the reference to the City. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the City Name. 
     */
    virtual TDesC& GetCity()= 0;
    
    /*
     * Gets the reference to the District name. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the District Name. 
     */
    virtual TDesC& GetDistrict()= 0;
    
    /*
     * Gets the reference to the postal code. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the postal code. 
     */
    virtual TDesC& GetPincode()= 0;
    
    /*
     * Gets the reference to the thoroughfare name. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the thoroughfare name. 
     */
    virtual TDesC& GetThoroughfareName()= 0;
    
    /*
     * Gets the reference to the thoroughfare number. Scope of this returned reference is limited to
     * this perticular call. User has to store it for their further processing.
     * 
     *  @return reference to the String which holds the thoroughfare number. 
     */
    virtual TDesC& GetThoroughfareNumber() = 0;
    };

/*
 * Observer class which exposes callbacks to notify the completion of reversegeocoding event.
 *
 **/
class MReverseGeocodeObserver
    {
    public:
    /*
     * Callback function which notifys the completion of reverse geocode event. This signals the completion
     * of the asynchronous function CReverseGeoCode::GetAddressByCoordinate.
     * 
     *  @param aErrorcode Error status KErrNone in case of success or other system specific errorcodes
     *                    in case of failures.
     *                       
     *  @param aAddressInfo refrence to the address stucture, through which user can access the
     *                      address information. 
     */

    virtual void ReverseGeocodeComplete( TInt& aErrorcode, MAddressInfo& aAddressInfo ) =0;

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
 * CReverseGeocode
 * Concrete class which exposes interfaces to convert the geo-coordinates information
 * in to the address information.
 *
 **/
class CReverseGeocode : public CBase
    {
    public:
    /*
     * Factory function to create the instance of CReverseGeocode Class. This also registers
     * observer for getting the reverse geocode completion notifications.
     * 
     * @param aObserver refrence to the instance MReverseGeocodeObserver's 
     *                  implementation class.
     * @return pointer to the instance of CReverseGeocode.                 
     */
    IMPORT_C static CReverseGeocode* NewL( MReverseGeocodeObserver& aObserver );
        
    /*
     * Gets the address information for the given geo coordinates. This is an asynchronous function
     * Whose completion will be notified by the MReverseGeocodeObserver::ReverseGeocodeComplete callback.
     * 
     * @param aObserver refrence to the instance MReverseGeocodeObserver's 
     *                  implementation class.
     */
    virtual void GetAddressByCoordinateL( TLocality aLocality, 
                                            const TConnectionOption aOption = ESilent ) = 0;

	/*
	* checks if silent connection is allowed
	* @return ETrue if silent connection is allowed
	*/
    virtual TBool SilentConnectionAllowed() = 0;
    };

#endif /* REVERSEGEOCODE_H_ */
