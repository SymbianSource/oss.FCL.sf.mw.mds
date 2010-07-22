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
* Description:	The source file for ReverseGeoCoderPlugin that creates the ReverseGeocoder
*
*/

// INCLUDE FILES
#include <w32std.h>
#include <lbsposition.h>


#include "reversegeocoderplugin.h"
#include "reversegeocode.h"
#include "geotagger.h"
#include "locationmanagerdebug.h"


// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::NewL
// Create instance of concrete ECOM interface implementation
// ----------------------------------------------------------------------------
//
CReverseGeoCoderPlugin* CReverseGeoCoderPlugin::NewL()
    {
    LOG( "CReverseGeoCoderPlugin::NewL" );
    CReverseGeoCoderPlugin* self = CReverseGeoCoderPlugin::NewLC();
    CleanupStack::Pop(self);
    return self;
    }


// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::NewLC
// Create instance of concrete ECOM interface implementation
// ----------------------------------------------------------------------------
//
CReverseGeoCoderPlugin* CReverseGeoCoderPlugin::NewLC()
    {
    LOG( "CReverseGeoCoderPlugin::NewLC" );
    CReverseGeoCoderPlugin* self = new (ELeave) CReverseGeoCoderPlugin;
    CleanupStack::PushL (self);
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::CReverseGeoCoderPlugin()
// The default constructor
// ----------------------------------------------------------------------------
//
CReverseGeoCoderPlugin::CReverseGeoCoderPlugin() 
                      : iRevGeocoder(NULL),
                        iObserver(NULL)
    {
    }

// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::ConstructL
// Second phase construction.
// ----------------------------------------------------------------------------
//
void CReverseGeoCoderPlugin::ConstructL()
    {
    LOG( "CReverseGeoCoderPlugin::ConstructL" );
    }

// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::~CReverseGeoCoderPlugin
// Destructor
// ----------------------------------------------------------------------------
//
CReverseGeoCoderPlugin::~CReverseGeoCoderPlugin()
    {
    LOG( "CReverseGeoCoderPlugin::~CReverseGeoCoderPlugin,begin" );
    delete iRevGeocoder;
    iObserver = NULL;
    LOG( "CReverseGeoCoderPlugin::~CReverseGeoCoderPlugin,end" );
    }

// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::CreateReverseGeoCoderL
// Creates an instance of the ReverseGeoCoder
// ----------------------------------------------------------------------------
//
void CReverseGeoCoderPlugin::CreateReverseGeoCoderL()
    {
    LOG( "CReverseGeoCoderPlugin::CreateReverseGeoCoderL,begin" );
    if(iObserver)
        {
        iRevGeocoder = CReverseGeocode::NewL(*iObserver);
        }
    LOG( "CReverseGeoCoderPlugin::CreateReverseGeoCoderL,end" );
    }

// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::AddObserverL
// Initializes the ReverseGeoCodeObserver
// ----------------------------------------------------------------------------
//
void CReverseGeoCoderPlugin::AddObserverL(MReverseGeocodeObserver& aObserver)
    {
    LOG( "CReverseGeoCoderPlugin::AddObserverL,begin" );
    iObserver = &aObserver;
    
    if(!iRevGeocoder)
        {
        CreateReverseGeoCoderL();
        }
    LOG( "CReverseGeoCoderPlugin::AddObserverL,end" );
    }


// ----------------------------------------------------------------------------
// CReverseGeoCoderPlugin::GetAddressByCoordinateL
// Gets the address for the given geo-coordinaates.
// ----------------------------------------------------------------------------
//
void CReverseGeoCoderPlugin::GetAddressByCoordinateL( TLocality aLocality, 
                                                      const TConnectionOption aOption )
    {
    LOG( "CReverseGeoCoderPlugin::GetAddressByCoordinateL,begin" );
    if(iRevGeocoder)
    	{
        iRevGeocoder->GetAddressByCoordinateL(aLocality, aOption);
    	}
    LOG( "CReverseGeoCoderPlugin::GetAddressByCoordinateL,end" );
    }


 // ----------------------------------------------------------------------------
 // CReverseGeoCoderPlugin::SilentConnectionAllowed
 // Wrapper API to check if the ReverseGeoCoder allows a silent connection
 // ----------------------------------------------------------------------------
 //
TBool CReverseGeoCoderPlugin::SilentConnectionAllowed()
    {
    LOG( "CReverseGeoCoderPlugin::SilentConnectionAllowed,begin" );
    TBool ret = EFalse;
	if(iRevGeocoder)
		{
   		ret = iRevGeocoder->SilentConnectionAllowed();
		}
    LOG1( "CReverseGeoCoderPlugin::SilentConnectionAllowed,end ret - %d", ret );
	return ret;
    }

 

 //End of File
 
