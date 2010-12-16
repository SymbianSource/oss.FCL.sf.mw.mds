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


#ifndef MDSDRMPLUGIN_H
#define MDSDRMPLUGIN_H

// INCLUDES
#include <e32std.h>
#include <e32base.h>
#include <DRMEventObserver.h>
#include <monitorplugin.h>

class CDRMNotifier;
class CMdsDrmMonitorAO;

// CLASS DECLARATION

class CMdSDrmPlugin : public CMonitorPlugin,
                      public MDRMEventObserver
	{
public:
	// Constructors and destructor
	static CMdSDrmPlugin* NewL();
	
	/**
	 * Destructor.
	 */
	virtual ~CMdSDrmPlugin();	
	
    // From CMonitorPlugin
	
    /**
     * Starts monitoring for drm right status changes
     *
     * @param aObserver  All events are notified via the aObserver.
     * @param aMdEClient  A pointer to MdE client.
     * @param aCtxEngine  A pointer to context engine.
     * @param aPluginFactory  A pointer to harvester plugin factory.
     * @return ETrue if success, EFalse if not.
     */
     TBool StartMonitoring( MMonitorPluginObserver& aObserver, CMdESession* aMdEClient, 
             CContextEngine* aCtxEngine, CHarvesterPluginFactory* aHarvesterPluginFactory );
     
     /**
     * Stops monitoring.
     *
     * @return ETrue if success, EFalse if not.
     */
     TBool StopMonitoring();
     
     /**
     * Resumes paused monitoring.
     *
     * @param aObserver  All events are notified via the aObserver.
     * @param aMdEClient  A pointer to MdE client.
     * @param aCtxEngine  A pointer to context engine.
     * @param aPluginFactory  A pointer to harvester plugin factory.
     * @return ETrue if success, EFalse if not.
     */
     TBool ResumeMonitoring( MMonitorPluginObserver& aObserver, CMdESession* aMdEClient,
             CContextEngine* aCtxEngine, CHarvesterPluginFactory* aHarvesterPluginFactory );
     
     /**
     * Pauses monitoring.
     *
     * @return ETrue if success, EFalse if not.
     */
     TBool PauseMonitoring();
	
	/**
	 * From MDRMEventObserver
	 * Notification about change in drm rights status
	 */
	void HandleEventL( MDRMEvent* aEvent );

private:

	/**
	 * Constructor for performing 1st stage construction
	 */
	CMdSDrmPlugin();
	
	/**
	 * default constructor for performing 2nd stage construction
	 */
	void ConstructL();
    
private:
	
	CMdsDrmMonitorAO* iMonitorAo;
	
	// DRM
	CDRMNotifier* iDrmNotifier;
	
	TBool iMonitoringPaused;
	
	RPointerArray<HBufC8> iEventArray;

	};

#endif // MDSDRMPLUGIN_H

