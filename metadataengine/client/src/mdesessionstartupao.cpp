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
* Description:  MdE session startup active object*
*/

// INCLUDE FILES
#include "mdesessionstartupao.h"

#include "mdeenginesession.h"
#include "mdesessionimpl.h"

// ========================= MEMBER FUNCTIONS ==================================

CMdESessionStartupAO* CMdESessionStartupAO::NewL(
    CMdESessionImpl& aSessionImpl, RMdEEngineSession& aSession )
    {
    CMdESessionStartupAO* self = 
    	CMdESessionStartupAO::NewLC( aSessionImpl, aSession );
    CleanupStack::Pop( self );
    return self;
    }

CMdESessionStartupAO* CMdESessionStartupAO::NewLC(
    CMdESessionImpl& aSessionImpl, RMdEEngineSession& aSession )
    {
    CMdESessionStartupAO* self =
        new ( ELeave ) CMdESessionStartupAO( aSessionImpl, aSession );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

CMdESessionStartupAO::CMdESessionStartupAO(
    CMdESessionImpl& aSessionImpl, RMdEEngineSession& aSession )
    : CActive( CActive::EPriorityStandard )
    , iSessionImpl( aSessionImpl )
    , iSession( aSession )
    {
    CActiveScheduler::Add( this );
    
    iState = EStartupOpenServer;
    }

void CMdESessionStartupAO::ConstructL()
    {
	SetActive();
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
    }

CMdESessionStartupAO::~CMdESessionStartupAO()
    {
    Cancel(); // Causes call to DoCancel()
    }

void CMdESessionStartupAO::DoCancel()
    {
    iSession.OpenCancel( iStatus );
    }

void CMdESessionStartupAO::RunL()
    {
    const TInt status = iStatus.Int();
    
    switch ( iState )
        {
        case EStartupOpenServer:
        	{
        	iSession.OpenL( iStatus );

			iState = EStartupConnect;

        	SetActive();

        	break;
        	}
        case EStartupConnect:
        	{
        	iSession.ConnectL();

			iState = EStartupLoadSchema;

			SetActive();
			TRequestStatus* status = &iStatus;
			User::RequestComplete( status, KErrNone );

            break;
            }
        case EStartupLoadSchema:
        	{
        	iSessionImpl.DoLoadSchemaL();

			iState = EStartupNotifyClient;

			SetActive();
			TRequestStatus* status = &iStatus;
			User::RequestComplete( status, KErrNone );

        	break;
        	}
        case EStartupNotifyClient:
        	{
        	iSessionImpl.NotifySessionOpened( KErrNone );

        	break;
        	}
       	default:
       		{
       		User::Leave( KErrUnknown );
       		break;
       		}
        }
    }

TInt CMdESessionStartupAO::RunError(TInt aError)
    {
    iSessionImpl.NotifyError( aError );

    return KErrNone;
    }
