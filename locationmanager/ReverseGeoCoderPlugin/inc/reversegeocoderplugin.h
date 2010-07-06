/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:	The header file for ReverseGeoCoderPlugin that creates the ReverseGeocoder
*
*/


#ifndef __REVERSEGEOCODERPLUGIN_H__
#define __REVERSEGEOCODERPLUGIN_H__

// INCLUDES
#include <e32base.h>

#include "reversegeocode.h"

/**
 * CReverseGeoCoderPlugin
 *
 * An implementation of the CReverseGeoCoderPlugin definition. 
 *              Encapsulates the reverse goecoding functionality
 *              This is concrete class, instance of which
 *              ECOM framework gives to ECOM clients.
 */
class CReverseGeoCoderPlugin : public CBase
	{
public:
	
	/**
	* Create instance of concrete implementation. 
	* @return: Instance of this class.
	*/
	static CReverseGeoCoderPlugin* NewL();

	/**
	* Static constructor.
	* @return: Instance of this class.
	*/
	static CReverseGeoCoderPlugin* NewLC();

	/**
	* Destructor.
	*/
	~CReverseGeoCoderPlugin();


protected:
	
	/**
	* The default constructor
	* Perform the first phase of two phase construction
	*/
	CReverseGeoCoderPlugin();

	/**
	*  Perform the second phase construction of a
	*             CImplementationClassPlus object.
	*/
	void ConstructL();
	
public:
	
	/**
	 * Creates the instance of Reverse Geocoder
	 * 
	 */
	void CreateReverseGeoCoderL();

	/**
	 * Initializes the ReverseGeoCodeObserver
	 * @param: aObserver The observer class instance that is to be notified when reverse geocoding completes
	 * 
	 */
    virtual void AddObserverL(MReverseGeocodeObserver& aObserver);

	/**
	 * A wrapper API to fetch the address from geocoordinates
	 * Internally calls the ReverseGeoCoder
	 * @param aLocality A TLocality object that contains the geocoordinate information
	 * @param aOption Indicates if the connection is silent connection or not
	 * 
	 */
    virtual void GetAddressByCoordinateL( TLocality aLocality,const TConnectionOption aOption );

	/**
	 * Wrapper API to check if the ReverseGeoCoder allows a silent connection
	 * @return:TBool Indicates if a silent connection is allowed
	 * 
	 */
	virtual TBool SilentConnectionAllowed();
	
private:
	
    /** 
    * iRevGeocoder
    * An instance of the CReverseGeocode class to fetch the place name from geocoordinates
    */
	CReverseGeocode *iRevGeocoder;

	/*
	 * iObserver
	 * An instance of the class that is to be notified once Reverse Geocoding is completed
	 */
	MReverseGeocodeObserver* iObserver;
	/**
	  * iDtorKey
	  * Identification of the plugin on cleanup
	  */
	TUid iDtorKey;

	};


#endif //__REVERSEGEOCODERPLUGIN_H__

//End of file

