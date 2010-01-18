/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Client harvest item implementation
*
*/


#include "clientharvestitem.h"
#include "harvesterlog.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
//
RClientHarvestItem::RClientHarvestItem()
    {
    WRITELOG( "RClientHarvestItem::RClientHarvestItem()" );
    iUri = NULL;
    }

// ---------------------------------------------------------------------------
// Copy constructor
// ---------------------------------------------------------------------------
//
RClientHarvestItem::RClientHarvestItem( const RClientHarvestItem& aItem )
    {
    iUri = aItem.iUri;
    iTimeStamp = aItem.iTimeStamp;
    const TInt count = aItem.iAlbumIds.Count();
    iAlbumIds.Reserve( count );
    for ( TInt i = 0; i < count; i++ )
        {
        iAlbumIds.Append( aItem.iAlbumIds[i] );
        }
    }

// ---------------------------------------------------------------------------
// InitL
// ---------------------------------------------------------------------------
//
void RClientHarvestItem::InitL( const TDesC& aUri, RArray<TItemId>& aAlbumIds ) 
    {
    if ( aUri.Length() <= 0 || aUri.Length() > KMaxFileName )
        {
        User::Leave( KErrArgument );
        }

    this->Reset();

    iTimeStamp.UniversalTime();
    iUri = aUri.AllocL();
    const TInt count = aAlbumIds.Count();
    
    iAlbumIds.Reserve( count );
    for ( TInt i = 0; i < count; i++ )
        {
        iAlbumIds.Append( aAlbumIds[i] );
        }
    }

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------
//
void RClientHarvestItem::Reset() 
    {
    if ( iUri )
        {
        delete iUri;
        iUri = NULL;
        }

    iAlbumIds.Reset();
    }

// ---------------------------------------------------------------------------
// Close
// ---------------------------------------------------------------------------
//
void RClientHarvestItem::Close()
    {
    Reset();
    iAlbumIds.Close();
    }




