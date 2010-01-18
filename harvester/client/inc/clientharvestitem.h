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
* Description:  Client harvest item
*
*/


#ifndef __CCLIENTHARVESTITEM_H__
#define __CCLIENTHARVESTITEM_H__

#include <e32base.h>

#include "mdccommon.h"

NONSHARABLE_CLASS( RClientHarvestItem )
    {
    public:
        /**
         * Constructor.
         */
        RClientHarvestItem();

        /**
         * Copy constructor.
         */
        RClientHarvestItem( const RClientHarvestItem& aItem );

        /**
         * Initialize with data.
         */
        void InitL( const TDesC& aURI, RArray<TItemId>& aAlbumIds );

        /**
         * Close (release memory).
         */
        void Close();

        /**
         * Resets the item. Releases memory.
         */
        void Reset();

    private:

        /** @var 	HBufC16* iUri;
         *  @brief 	URI to harvest
         */
        HBufC16* iUri;

        /** @var 	TTime iTimeStamp;
         *  @brief  Timestamp for the file
         */
        TTime iTimeStamp;

        /** @var 	RArray<TInt> iAlbumIds;
         *  @brief  Album IDs
         */
        RArray<TItemId> iAlbumIds;
    };

#endif
