/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Processor object for running harvester requests
*
*/


#ifndef HARVESTERNOTIFICATIONQUEUE_H
#define HARVESTERNOTIFICATIONQUEUE_H

#include <e32base.h>

class CHarvesterClientAO;
class MHarvestObserver;

/**
 *  Processor object for running queued tasks.
 *
 */
class CHarvesterNotificationQueue: public CBase
    {
public:

    /**
     * Two-phased constructor.
     *
     * @return Instance of CHarvesterNotificationQueue.
     */
    static CHarvesterNotificationQueue* NewL();

    /**
     * Destructor
     *
     */
    virtual ~CHarvesterNotificationQueue();
    
    /**
     * Adds new request to the queue.
     *
     * @param aRequest Request to be added to the queue.
     */
    void AddRequestL( CHarvesterClientAO* aRequest );
    
    /**
     * Marks request completed.
     */
    void Cleanup( TBool aShutdown );
    
    void SetObserver( MHarvestObserver* aObserver );
    
private:

    /**
     * C++ default constructor
     * @return Instance of CHarvesterNotificationQueue.
     */
    CHarvesterNotificationQueue();

    /**
     * Symbian 2nd phase constructor can leave.
     */
    void ConstructL();

private:

    /**
     * Array of active objects for each pending harvester request.
     */
    RPointerArray <CHarvesterClientAO> iRequests;

};

#endif // HARVESTERNOTIFICATIONQUEUE_H
