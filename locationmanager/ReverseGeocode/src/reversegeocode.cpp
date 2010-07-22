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
* Description: Implementation of exported interface class of reverse geo-code 
*/

#include "reversegeocode.h"
#include "internalreversegeocode.h"
#include "locationmanagerdebug.h"

// --------------------------------------------------------------------------
// CReverseGeocode::NewL()
// factory class to create the instance
// --------------------------------------------------------------------------

EXPORT_C CReverseGeocode* CReverseGeocode::NewL( MReverseGeocodeObserver& aObserver )
    {
        LOG("CReverseGeocode::NewL ,begin");
        CInternalReverseGeocode *self = CInternalReverseGeocode::NewL( aObserver );
    
        return self;
    }

//End of file
