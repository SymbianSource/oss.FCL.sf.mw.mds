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
* Description: Implementation of Internal address info structure.
*/

#include "internaladdressinfo.h"
#include "locationmanagerdebug.h"



// ----------------------------------------------------------------------------
// CInternalAddressInfo::CInternalAddressInfo()
// Constructor
// ----------------------------------------------------------------------------
CInternalAddressInfo::CInternalAddressInfo():   iCountryName( NULL ),
                                                iState( NULL ),
                                                iCity( NULL ),
                                                iDistrict( NULL ),                                                
                                                iPin( NULL ),
                                                iTFName( NULL ),
                                                iTFNumber( NULL )

    {
    
    
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::~CInternalAddressInfo()
// Destructor
// ----------------------------------------------------------------------------
CInternalAddressInfo::~CInternalAddressInfo()
    {
    LOG("CInternalAddressInfo::~CInternalAddressInfo ,begin");
    if (iCountryName)
        {
        delete iCountryName;
        iCountryName = NULL ;
        }
    if (iState)
        {
        delete iState;
        iState= NULL ;
        }
    if (iCity)
        {
        delete iCity;
        iCity= NULL ;
        }
    if (iDistrict)
        {
        delete iDistrict;
        iDistrict = NULL ;
        }
    if (iPin)
        {
        delete iPin;
        iPin = NULL ;
        }
    if (iTFName)
        {
        delete iTFName;
        iTFName = NULL ;
        }
    if (iTFNumber)
        {
        delete iTFNumber;
        iTFNumber = NULL ;
        }

    LOG("CInternalAddressInfo::~CInternalAddressInfo ,end");
    }

// ----------------------------------------------------------------------------
// CInternalAddressInfo::NewL()
// Factory function to create the instance
// ---------------------------------------------------------------------------- 
CInternalAddressInfo* CInternalAddressInfo::NewL()
    {
    LOG("CInternalAddressInfo::NewL ,begin");
    CInternalAddressInfo *self = new (ELeave) CInternalAddressInfo();
    CleanupStack::PushL(self);
    self->ConstructL();
        
    CleanupStack::Pop( self );
    return self;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::ConstructL()
// Second phase construction.
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::ConstructL()
    {
    LOG("CInternalAddressInfo::ConstructL ,begin");
    //Copy all with the empty strings;
	ResetAddressInfoL();
	LOG("CInternalAddressInfo::ConstructL ,end");
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetCountryName()
// Gets Country name 
// ---------------------------------------------------------------------------- 
TDesC& CInternalAddressInfo::GetCountryName()
    {
    return *iCountryName;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetState()
// Gets State name 
// ----------------------------------------------------------------------------
TDesC& CInternalAddressInfo::GetState()
    {
    return *iState;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetCity()
// Gets City name. 
// ---------------------------------------------------------------------------- 
TDesC& CInternalAddressInfo::GetCity()
    {
    return *iCity;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetDistrict()
// Gets District name of the address.
// ----------------------------------------------------------------------------
TDesC& CInternalAddressInfo::GetDistrict()
    {
    return *iDistrict;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetPincode()
// Gets the postal code.
// ---------------------------------------------------------------------------- 
TDesC& CInternalAddressInfo::GetPincode()
    {
    return *iPin;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetThoroughfareName()
// Gets thoroughfare name
// ---------------------------------------------------------------------------- 
TDesC& CInternalAddressInfo::GetThoroughfareName()
    {
    return *iTFName;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::GetThoroughfareNumber()
// Gets thoroughfare number
// ---------------------------------------------------------------------------- 
TDesC& CInternalAddressInfo::GetThoroughfareNumber()
    {
    return *iTFNumber;
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetCountryName()
// Sets the country name information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetCountryName( const TDesC& aCountryName )
    {
    if( iCountryName )
        {
        delete iCountryName;
        }
    iCountryName = aCountryName.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetState()
// Sets the State name information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetState( const TDesC& aState )
    {
    if( iState )
        {
        delete iState;
        }
    iState = aState.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetCity()
// Sets the City name information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetCity( const TDesC& aCity )
    {
    if( iCity )
        {
        delete iCity;
        }
    iCity = aCity.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetDistrict()
// Sets the District name information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetDistrict( const TDesC& aDisrict )
    {
    if( iDistrict )
        {
        delete iDistrict;
        }
    iDistrict = aDisrict.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetPincode()
// Sets the Postal code information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetPincode( const TDesC& aPincode )
    {
    if( iPin )
        {
        delete iPin;
        }
    iPin = aPincode.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetThoroughfareName()
// Sets the thoroughfare name information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetThoroughfareName( const TDesC& aTFName )
    {
    if( iTFName )
        {
        delete iTFName;
        }
    iTFName = aTFName.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::SetThoroughfareNumber()
// Sets the thoroughfare number information
// ---------------------------------------------------------------------------- 
void CInternalAddressInfo::SetThoroughfareNumber( const TDesC& aTFNumber )
    {
    if( iTFNumber )
        {
            delete iTFNumber;
        }
    iTFNumber = aTFNumber.Alloc();
    }


// ----------------------------------------------------------------------------
// CInternalAddressInfo::ResetAddressInfoL()
// resets the address info
// ----------------------------------------------------------------------------
void CInternalAddressInfo::ResetAddressInfoL()
   {
    // Allocate empty buffer to avoid crash on get method
    
	// Free memory	
	if(iCountryName)
	    {
        delete iCountryName;
        iCountryName = NULL;
        }
    if(iState)
        {
        delete iState;
        iState = NULL;
        }
    if(iCity)
        {
        delete iCity;
        iCity = NULL;
        }
    if(iDistrict)
        {
        delete iDistrict;
        iDistrict = NULL;
        }
    if(iPin)
        {
        delete iPin;
        iPin = NULL;
        }
    if(iTFName)
        {
        delete iTFName;
        iTFName = NULL;
        }
    if(iTFNumber)
        {
        delete iTFNumber;
        iTFNumber = NULL;
        }
	// Allocate empty strings, 
    iCountryName = KNullDesC().AllocL();
    iCity = KNullDesC().AllocL();
    iState = KNullDesC().AllocL();
    iDistrict = KNullDesC().AllocL();
    iPin = KNullDesC().AllocL();
    iTFName = KNullDesC().AllocL();
    iTFNumber = KNullDesC().AllocL();

   }
	

//end of file
