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
* Description:  Connection close timer handler
*/

#include "connectiontimerhandler.h"
#include "locationmanagerdebug.h"


// ----------------------------------------------------------------------------
// CConnectionTimerHandler::CConnectionTimerHandler()
// ----------------------------------------------------------------------------


CConnectionTimerHandler::CConnectionTimerHandler(MConnectionTimeoutHandlerInterface& aConnectionTimeoutHandlerInterface):
        CTimer(EPriorityStandard ),
        iConnectionTimeoutHandlerInterface(aConnectionTimeoutHandlerInterface)
{

}

// ----------------------------------------------------------------------------
// CConnectionTimerHandler::~CConnectionTimerHandler()
// ----------------------------------------------------------------------------
CConnectionTimerHandler::~CConnectionTimerHandler()
    {
    }

// ----------------------------------------------------------------------------
// CConnectionTimerHandler::NewL()
// ----------------------------------------------------------------------------
CConnectionTimerHandler* CConnectionTimerHandler::NewL(MConnectionTimeoutHandlerInterface& aConnectionTimeoutHandlerInterface)
    {
    LOG("CConnectionTimerHandler::NewL ,begin");
    CConnectionTimerHandler* self = new( ELeave ) CConnectionTimerHandler(aConnectionTimeoutHandlerInterface);
       CleanupStack::PushL( self );
       self->ConstructL();
       CleanupStack::Pop();
       
       return self;
    }

// ----------------------------------------------------------------------------
// CConnectionTimerHandler::ConstructL()
// ----------------------------------------------------------------------------
void CConnectionTimerHandler::ConstructL()
    {
    LOG("CConnectionTimerHandler::ConstructL ,begin");
	CActiveScheduler::Add(this);
    CTimer::ConstructL();
    LOG("CConnectionTimerHandler::ConstructL ,end");
    }

// ----------------------------------------------------------------------------
// CConnectionTimerHandler::StartTimer
// starts a timer 
// ----------------------------------------------------------------------------
void CConnectionTimerHandler::StartTimer(const TInt aTimeoutVal)
    {
    LOG("CConnectionTimerHandler::StartTimer ,begin");
    if(!IsActive())
        {
        // already active.
        LOG("Timer started");
        After(aTimeoutVal);
        }
    LOG("CConnectionTimerHandler::StartTimer ,end");
    }

// ----------------------------------------------------------------------------
// CConnectionTimerHandler::RunL
// ----------------------------------------------------------------------------
void CConnectionTimerHandler::RunL( )
    {
    LOG("CConnectionTimerHandler::RunL ,begin");
    iConnectionTimeoutHandlerInterface.HandleTimedoutEvent(iStatus.Int());
	LOG("CConnectionTimerHandler::RunL ,end");
    }
    


// End of file

