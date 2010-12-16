/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  
*
*/

#ifndef HARVESTERVIDEOPARSER_H
#define HARVESTERVIDEOPARSER_H

// INCLUDES
#include <e32std.h>
#include <e32base.h>

#include "harvestervideoplugin.h"

/**
*  CMdeObjectWrapper
* 
*/
NONSHARABLE_CLASS( CHarvesterVideoParser ) : public CBase
{
public: // Constructors and destructor

	/**
		* Destructor.
		*/
	~CHarvesterVideoParser();

		/**
		* Two-phased constructor.
		*/
	IMPORT_C static CHarvesterVideoParser* NewL();

	IMPORT_C void ParseVideoMetadataL( RFile64& aFileHandle, CVideoHarvestData& aVHD );
	
private:

	/**
		* Constructor for performing 1st stage construction
		*/
	CHarvesterVideoParser();

	/**
		* default constructor for performing 2nd stage construction
		*/
	void ConstructL();

	void CheckForCodecSupport( HBufC* aMimeBuffer, CVideoHarvestData& aVHD );
};

#endif // HARVESTERVIDEOPARSER_H

