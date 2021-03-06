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
* Description:  Interface class.
*
*/


#ifndef MPROPERTYWATCHER_H
#define MPROPERTYWATCHER_H

// SYSTEM INCLUDE
#include <e32base.h>

// CLASS DECLARATION 
class MKeyObserver
	{
public:


	/**
	 * Factory method NotifyKeyAndStatusL
	 * @param aKeyValue - observed key's value.
	 * @param aPropertyCategory observed key's category.
	 * @param aKey The P&S key to be watched.
	 */
	virtual void NotifyKeyL( 
					const TInt aKeyValue,
					const TUid aPropertyCategory,
					const TUint aKey ) = 0;
		};
	
// CLASS DECLARATION
class MKeyAndStatusObserver
	{
public:

    /**
     * Factory method NotifyKeyAndStatusL
     * @param aKeyValue - observed key's value.
     * @param aStatus - reference to iStatus.
     * @param aPropertyCategory observed key's category.
     * @param aKey he P&S key to be watched.
     */
	virtual void NotifyKeyAndStatusL( 
				const TInt aKeyValue,
				TRequestStatus& aStatus,
				const TUid aPropertyCategory,
				const TUint aKey ) = 0;
	};
	
#endif // MPROPERTYWATCHER_H
