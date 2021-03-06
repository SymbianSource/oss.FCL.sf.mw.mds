
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
* Description:  Shutdown observer
*
*/

#ifndef WDSHUTDOWNOBSERVER_H
#define WDSHUTDOWNOBSERVER_H

#include <e32base.h>
#include <w32std.h>
#include <e32property.h> 


/**
 * Observer interface for signaling the need for shutdown
 */
class MWDShutdownObserver
    {
public:

    virtual void ShutdownNotification() = 0;
    
    virtual void RestartNotification() = 0;
    };

/**
 *  Active object for observing P&S keys
 *
 *  @since S60 v5.0
 */
class CWDShutdownObserver  : public CActive
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CTMShutdownObserver.
     */
    IMPORT_C static CWDShutdownObserver* NewL( MWDShutdownObserver& aObserver/*, const TUid& aKeyCategory*/ );

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CWDShutdownObserver();
    
protected:

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();
    
    TInt RunError( TInt aError );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return Instance of CShutdownObserver.
     */
    CWDShutdownObserver( MWDShutdownObserver& aObserver/*, const TUid& aKeyCategory*/ );

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    
    // not own
    MWDShutdownObserver& iObserver;
    
    /*const TUid& iKeyCategory;*/
    RProperty iProperty;
};

#endif // SHUTDOWNOBSERVER_H
