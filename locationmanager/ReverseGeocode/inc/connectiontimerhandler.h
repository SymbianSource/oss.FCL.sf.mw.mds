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
* Description:	Handle connection close
*
*/

#ifndef __CONNECTIONTIMERHANDLER_H__
#define __CONNECTIONTIMERHANDLER_H__


// INCLUDES
#include <e32std.h>
#include <e32base.h>

class MConnectionTimeoutHandlerInterface
    {
    public:
		
	/**
	  * Handles the timedout event
	  * @param  aErrorCode  the errcode for time out
	  */
    virtual void HandleTimedoutEvent(TInt aErrorCode) = 0;
    };

class CConnectionTimerHandler : public CTimer
{
    
public:

	/**
	  * @param aConnectionTimeoutHandlerInterface the interace class that handles the timeout events
	  */	
     static CConnectionTimerHandler* NewL(MConnectionTimeoutHandlerInterface& aConnectionTimeoutHandlerInterface); 

	/**
	  * Destructor
	  */
     ~CConnectionTimerHandler();

	/**
	  * starts a timer
	  * @param aTimeoutVal the time after which the timer will expire
	  */	
     void StartTimer(const TInt aTimeoutVal);

     
protected:

	/**
	  * RunL
	  * from CActive
	  */	
      void RunL();
      
 private:

	/**
	  * Second phase construction
	  */	
      void ConstructL();

	/**
	  * @param aConnectionTimeoutHandlerInterface the interace class that handles the timeout events
	  */	
      CConnectionTimerHandler(MConnectionTimeoutHandlerInterface& aConnectionTimeoutHandlerInterface);
      


private:      

	/**
	  * An instance of the interace class that handles the timeout events
	  */	
      MConnectionTimeoutHandlerInterface& iConnectionTimeoutHandlerInterface;
};


#endif /*__CONNECTIONTIMERHANDLER_H__*/

// End of file
