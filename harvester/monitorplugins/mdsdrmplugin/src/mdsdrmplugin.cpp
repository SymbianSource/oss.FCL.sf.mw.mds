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
* Description: 
*
*/

#include "mdsdrmplugin.h"
#include "mdsdrmmonitorao.h"
#include "harvesterlog.h"
#include "harvestercommon.h"

#include <DRMNotifier.h>
#include <DRMEventAddRemove.h>

CMdSDrmPlugin::CMdSDrmPlugin() : iMonitorAo( NULL ), iDrmNotifier( NULL ), iMonitoringPaused( ETrue )
	{
	WRITELOG("CMdSDrmPlugin::CMdSDrmPlugin()");
	}


CMdSDrmPlugin::~CMdSDrmPlugin()
	{ 
    delete iMonitorAo;
    iMonitorAo = NULL;
    
    iEventArray.ResetAndDestroy();
    iEventArray.Close();
    
    if(iDrmNotifier)
        {
        TRAP_IGNORE( iDrmNotifier->UnRegisterEventObserverL( *this, KEventAddRemove ) );
        delete iDrmNotifier;
        iDrmNotifier = NULL;
        }
	}

CMdSDrmPlugin* CMdSDrmPlugin::NewL()
	{
	CMdSDrmPlugin* self = new (ELeave) CMdSDrmPlugin();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CMdSDrmPlugin::ConstructL()
	{
    iMonitorAo = CMdsDrmMonitorAO::NewL();
    iDrmNotifier = CDRMNotifier::NewL();
	}

// ---------------------------------------------------------------------------
// CMdSDrmPlugin::StartMonitoring()
// ---------------------------------------------------------------------------
//
TBool CMdSDrmPlugin::StartMonitoring( MMonitorPluginObserver& aObserver,
        CMdESession* aMdEClient, CContextEngine* /*aCtxEngine*/,
        CHarvesterPluginFactory* /*aHarvesterPluginFactory*/ )
    {
    WRITELOG("CMdSDrmPlugin::StartMonitoring()");
    iMonitorAo->Setup( aObserver, aMdEClient );
    iMonitoringPaused = EFalse;
    TRAPD( err, iDrmNotifier->RegisterEventObserverL( *this, KEventAddRemove ) );
    if( err )
        {
        return EFalse;
        }
    
    WRITELOG("CMdSDrmPlugin::StartMonitoring() - succesful");
    return ETrue;
    }

// ---------------------------------------------------------------------------
// CMdSDrmPlugin::StopMonitoring()
// ---------------------------------------------------------------------------
//
TBool CMdSDrmPlugin::StopMonitoring()
    {
    iMonitoringPaused = ETrue;
    TRAPD( err, iDrmNotifier->UnRegisterEventObserverL( *this, KEventAddRemove ) );
    if( err )
        {
        return EFalse;
        }
    return ETrue;
    }

// ---------------------------------------------------------------------------
// CMdSDrmPlugin::ResumeMonitoring()
// ---------------------------------------------------------------------------
//
TBool CMdSDrmPlugin::ResumeMonitoring( MMonitorPluginObserver& /*aObserver*/,
        CMdESession* /*aMdEClient*/, CContextEngine* /*aCtxEngine*/,
        CHarvesterPluginFactory* /*aHarvesterPluginFactory*/ )
    {
    iMonitoringPaused = EFalse;
    
    for( TInt i = iEventArray.Count() - 1; i >=0; i-- )
        {
        iMonitorAo->AddToQueue( iEventArray[i] );
        }
    iEventArray.Reset();
    iEventArray.Compress();

    return ETrue;
    }

// ---------------------------------------------------------------------------
// CMdSDrmPlugin::PauseMonitoring()
// ---------------------------------------------------------------------------
//
TBool CMdSDrmPlugin::PauseMonitoring()
    {
    iMonitoringPaused = ETrue;
    return ETrue;
    }

void CMdSDrmPlugin::HandleEventL( MDRMEvent* aEvent )
    {
    WRITELOG("CMdSDrmPlugin::HandleEventL()");
    CDRMEventAddRemove *event  = reinterpret_cast<CDRMEventAddRemove*>(aEvent);
    
    if( event->Status() == ERightsObjectRecieved )
        {
        HBufC8 *url = event->GetContentIDL();
        
        if( iMonitoringPaused )
            {
            iEventArray.AppendL( url );
            }
        else
            {
            iMonitorAo->AddToQueue( url );
            }
        }
    }

