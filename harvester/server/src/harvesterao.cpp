/*
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Thread which actually performs the harvesting of the files*
*/


#include <e32svr.h>
#include <caf/caf.h>

#include <rlocationobjectmanipulator.h>
#include <placeholderdata.h>
#include <harvesterclientdata.h>

using namespace ContentAccess;

#include "harvesterao.h"
#include "harvesterlog.h"
#include "harvesterblacklist.h"
#include "mdeobject.h"
#include "mdsutils.h"
#include "mdeconstants.h"
#include "harvesterdata.h"
#include "ondemandao.h"
#include "harvestercommon.h"
#include "processoriginmapperinterface.h"
#include "mdeobjecthandler.h"
#include "harvestereventmanager.h"
#include "harvestercenreputil.h"
#include "restorewatcher.h"
#include "backupsubscriber.h"

// constants
const TInt32 KFileMonitorPluginUid = 0x20007186;  // file monitor plugin implementation uid

const TInt KPlaceholderQueueSize = 99;
const TInt KContainerPlaceholderQueueSize = 10;
const TInt KObjectDefStrSize = 20;

const TInt KCacheItemCountForEventCaching = 1;

_LIT( KTAGDaemonName, "ThumbAGDaemon" );
_LIT( KTAGDaemonExe, "thumbagdaemon.exe" );

_LIT(KInUse, "InUse");

CHarvesterAoPropertyDefs::CHarvesterAoPropertyDefs() : CBase()
	{
	}

void CHarvesterAoPropertyDefs::ConstructL(CMdEObjectDef& aObjectDef)
	{
	CMdENamespaceDef& nsDef = aObjectDef.NamespaceDef();

	// Common property definitions
	CMdEObjectDef& objectDef = nsDef.GetObjectDefL( MdeConstants::Object::KBaseObject );
	iCreationDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KCreationDateProperty );
	iLastModifiedDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KLastModifiedDateProperty );
	iSizePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KSizeProperty );
	iOriginPropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
	
	CMdEObjectDef& mediaDef = nsDef.GetObjectDefL( MdeConstants::MediaObject::KMediaObject );
	iPreinstalledPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KPreinstalledProperty );
	}

CHarvesterAoPropertyDefs* CHarvesterAoPropertyDefs::NewL(CMdEObjectDef& aObjectDef)
	{
	CHarvesterAoPropertyDefs* self = 
		new (ELeave) CHarvesterAoPropertyDefs();
	CleanupStack::PushL( self );
	self->ConstructL( aObjectDef );
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CHarvesterAO* CHarvesterAO::NewLC()
    {
    WRITELOG( "CHarvesterAO::NewLC() - begin" );
    
    CHarvesterAO* self = new (ELeave) CHarvesterAO();
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }
    
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CHarvesterAO* CHarvesterAO::NewL()
    {
    WRITELOG( "CHarvesterAO::NewL() - begin" );
    
    CHarvesterAO* self = CHarvesterAO::NewLC();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CHarvesterAO
// ---------------------------------------------------------------------------
//
CHarvesterAO::CHarvesterAO() : CActive( CActive::EPriorityStandard )
    {
    WRITELOG( "CHarvesterAO::CHarvesterAO() - begin" );
    
    iServerPaused = ETrue;
    iNextRequest = ERequestIdle;
    
    iContextEngineInitialized = EFalse;
    iMdeSessionInitialized = EFalse;
    iHarvesting = EFalse;
    }
     
// ---------------------------------------------------------------------------
// ~CHarvesterAO
// ---------------------------------------------------------------------------
//
CHarvesterAO::~CHarvesterAO()
    {
    WRITELOG( "CHarvesterAO::~CHarvesterAO()" );
    
    Cancel();

	iFs.Close();
	
	if (iCtxEngine)
		{
		iCtxEngine->ReleaseInstance();
		}
			
   	if (iHarvesterEventManager)
		{
		iHarvesterEventManager->ReleaseInstance();
		}

    StopMonitoring();
    DeleteMonitorPlugins();        
    
    StopComposers();
    DeleteComposers();

	delete iBackupSubscriber;
    
    if (iBlacklist)
		{
		iBlacklist->CloseDatabase();
		delete iBlacklist;
		}
	delete iReHarvester;

    if ( iHarvestFileMessages.Count() > 0 )
        {
        for ( TInt i = iHarvestFileMessages.Count()-1; i >= 0; --i )
            {
            RMessage2& msg = iHarvestFileMessages[i].iMessage;
            if (!msg.IsNull())
            	{
            	msg.Complete( KErrCancel );
            	}
            iHarvestFileMessages.Remove( i );
            }
        }
    iHarvestFileMessages.Close();
    
    iMonitorPluginArray.ResetAndDestroy();
    iMonitorPluginArray.Close();
  	
    iPHArray.ResetAndDestroy();
    iPHArray.Close();
	
   	iReadyPHArray.ResetAndDestroy();
    iReadyPHArray.Close();
    
    iContainerPHArray.ResetAndDestroy();
    iContainerPHArray.Close();
	
    delete iRestoreWatcher;
	delete iOnDemandAO;
	delete iMdEHarvesterSession;
	delete iMdESession;
	delete iQueue;
	delete iHarvesterPluginFactory;
	delete iMdeObjectHandler;
	delete iUnmountHandlerAO;
	
	delete iPropDefs;
	
	RMediaIdUtil::ReleaseInstance();
    
    REComSession::FinalClose();
    }

 // ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ConstructL()
    {
    WRITELOG( "CHarvesterAO::ConstructL() - begin" );
    
    CActiveScheduler::Add( this );

	User::LeaveIfError( iFs.Connect() );

    // Setting up MdE Session
	iMdESession = CMdESession::NewL( *this );
	
    RProcess process;
 	process.SetPriority( EPriorityBackground );
 	process.Close();
 	
    // Setting up context Engine (initialization is ready when ContextInitializationStatus -callback is called)
    iCtxEngine = CContextEngine::GetInstanceL( this ); // Create the context engine

    iBackupSubscriber = CBackupSubscriber::NewL( *this );
	
	iUnmountHandlerAO = CUnmountHandlerAO::NewL( *this );
	iUnmountHandlerAO->WaitForUnmountL();
	
	iHarvesterEventManager = CHarvesterEventManager::GetInstanceL();
	
	iRestoreWatcher = CRestoreWatcher::NewL();
	
	iHarvesterOomAO = CHarvesterOomAO::NewL( *this );
	
	iMediaIdUtil = &RMediaIdUtil::GetInstanceL();
	
    iBlacklist = CHarvesterBlacklist::NewL();
    iBlacklist->OpenDatabase();
    
    // Setting up Harvester queue
    iQueue = CHarvesterQueue::NewL( this, iBlacklist );
    
    // Setting up reharvester
    iReHarvester = CReHarvesterAO::NewL();
    iReHarvester->SetHarvesterQueue( iQueue );
    
    iHarvesterPluginFactory = CHarvesterPluginFactory::NewL();
    iHarvesterPluginFactory->SetBlacklist( *iBlacklist );
	
    WRITELOG( "CHarvesterAO::ConstructL() - end" );
    }

// ---------------------------------------------------------------------------
// LoadMonitorPluginsL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::LoadMonitorPluginsL()
    {
    WRITELOG( "CHarvesterAO::LoadMonitorPluginsL()" );
    
    RImplInfoPtrArray infoArray;
    
    TCleanupItem cleanupItem( MdsUtils::CleanupEComArray, &infoArray );
    CleanupStack::PushL( cleanupItem );
    
    CMonitorPlugin::ListImplementationsL( infoArray );
    TInt count( 0 );
    count = infoArray.Count();
    CMonitorPlugin* plugin = NULL;
    
    for ( TInt i = 0; i < count; i++ )
        {
        TUid uid = infoArray[i]->ImplementationUid();    // Create the plug-ins
        plugin = NULL;
        TRAPD( err, plugin = CMonitorPlugin::NewL( uid ) );
        if ( err == KErrNone && plugin )
            {
            CleanupStack::PushL( plugin );
            iMonitorPluginArray.AppendL( plugin ); // and add them to array
            CleanupStack::Pop( plugin );
            if ( uid.iUid == KFileMonitorPluginUid )
                {
                void* ptr = plugin;
                iProcessOriginMapper = STATIC_CAST( MProcessOriginMapperInterface*, ptr );
                }
            }
        else
            {
            WRITELOG( "CHarvesterAO::LoadMonitorPlugins() - Failed to load a monitor plugin!" );
            delete plugin;
            plugin = NULL;
            }
        }
    
    CleanupStack::PopAndDestroy( &infoArray ); // infoArray, results in a call to CleanupEComArray
    }

// ---------------------------------------------------------------------------
// DeleteMonitorPlugins
// ---------------------------------------------------------------------------
//
void CHarvesterAO::DeleteMonitorPlugins()
    {
    WRITELOG( "CHarvesterAO::DeleteMonitorPlugins()" );
    
    iMonitorPluginArray.ResetAndDestroy();
    }

// ---------------------------------------------------------------------------
// StartMonitoring
// ---------------------------------------------------------------------------
//
void CHarvesterAO::StartMonitoring()
    {
    WRITELOG( "CHarvesterAO::StartMonitoring()" );

    TInt count( iMonitorPluginArray.Count() );  
    
    for ( TInt i = 0; i < count; i++ )
        {
        iMonitorPluginArray[i]->StartMonitoring( *iQueue, iMdESession, NULL, 
        		iHarvesterPluginFactory );
        }
    }

// ---------------------------------------------------------------------------
// StopMonitoring
// ---------------------------------------------------------------------------
//
void CHarvesterAO::StopMonitoring()
    {
    WRITELOG( "CHarvesterAO::StopMonitoring()" );

    TInt count( iMonitorPluginArray.Count() );
    
    for ( TInt i = 0; i < count; i++ )
        {
        iMonitorPluginArray[i]->StopMonitoring();
        }
    }

// ---------------------------------------------------------------------------
// PauseMonitoring
// ---------------------------------------------------------------------------
//
void CHarvesterAO::PauseMonitoring()
    {
    WRITELOG( "CHarvesterAO::PauseMonitoring()" );
    TInt count( iMonitorPluginArray.Count() );
    
    for ( TInt i = 0; i<count; i++ )
        {
        iMonitorPluginArray[i]->PauseMonitoring();
        }
    WRITELOG( "CHarvesterAO::PauseMonitoring() - end" );
    }

// ---------------------------------------------------------------------------
// ResumeMonitoring
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ResumeMonitoring()
    {
    WRITELOG( "CHarvesterAO::ResumeMonitoring()" );
    
    TInt count( iMonitorPluginArray.Count() );
    
    for ( TInt i=0; i < count; i++ )
        {
        iMonitorPluginArray[i]->ResumeMonitoring( *iQueue, iMdESession, NULL,
        		iHarvesterPluginFactory );
        }
    WRITELOG( "CHarvesterAO::ResumeMonitoring() - end" );
    }

// ---------------------------------------------------------------------------
// HandleUnmount
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HandleUnmount( TUint32 aMediaId )
	{
	WRITELOG1( "CHarvesterAO::HandleUnmount(%d)", aMediaId );
	
    TUint32 mediaId(0);
    TUint removed(0);
    TInt err(0);
    CHarvesterData* hd = NULL;
    
#ifdef _DEBUG
    WRITELOG1( "CHarvesterAO::HandleUnmount() iReadyPHArray.Count() = %d", iReadyPHArray.Count() );
#endif
	if( iReadyPHArray.Count() > 0 )
		{
		TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder, iReadyPHArray.Count() ) );
		TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, iReadyPHArray.Count()) );
		iReadyPHArray.ResetAndDestroy();
		}

#ifdef _DEBUG
	WRITELOG1( "CHarvesterAO::HandleUnmount() iPHArray.Count() = %d", iPHArray.Count() );
#endif
   if( iPHArray.Count() > 0 )
        {
        for( TInt i=iPHArray.Count()-1; i>= 0; i--)
            {
            hd = iPHArray[i];
            err = iMediaIdUtil->GetMediaId( hd->Uri(), mediaId );
            
            if( err == KErrNone && mediaId == aMediaId )
                {
                WRITELOG1( "CHarvesterAO::HandleUnmount() remove iPHArray %d", i);
                delete hd;
				hd = NULL;
                iPHArray.Remove( i );
                removed++;
                }
            }
        WRITELOG1( "CHarvesterAO::HandleUnmount() DecreaseItemCountL iPHArray %d", removed);
        TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder, removed) );
        }
   
   removed = 0;
   
#ifdef _DEBUG
   WRITELOG1( "CHarvesterAO::HandleUnmount() iContainerPHArray.Count() = %d", iContainerPHArray.Count() );
#endif
   if( iContainerPHArray.Count() > 0 )
        {
        for( TInt i=iContainerPHArray.Count()-1; i>= 0; i--)
            {
            hd = iContainerPHArray[i];
            err = iMediaIdUtil->GetMediaId( hd->Uri(), mediaId );
            
            if( err == KErrNone && mediaId == aMediaId )
                {
                WRITELOG1( "CHarvesterAO::HandleUnmount() remove iContainerPHArray %d", i);
                delete hd;
				hd = NULL;
                iContainerPHArray.Remove( i );
                removed++;
                }
            }
        WRITELOG1( "CHarvesterAO::HandleUnmount() DecreaseItemCountL iContainerPHArray %d", removed);
        TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder, removed) );
        }
	
	TUint count = iQueue->ItemsInQueue();
	WRITELOG1( "CHarvesterAO::HandleUnmount() iQueue.Count() = %d", count );
	if( count > 0 )
	    {
	    WRITELOG( "CHarvesterAO::HandleUnmount() remove iQueue" );
	    TUint removed = iQueue->RemoveItems( aMediaId );
	    WRITELOG1( "CHarvesterAO::HandleUnmount() removed iQueue = %d", removed );
	    TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder, removed ) );
        TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, removed ) );
	    }
	
	iMediaIdUtil->RemoveMediaId( aMediaId );
	
	removed = 0;
	
	RPointerArray<CHarvesterPluginInfo>& hpiArray = iHarvesterPluginFactory->GetPluginInfos();
	if( hpiArray.Count() > 0 )
		{
		RArray<TItemId> placeholders;
		
		TUint32 mediaId(0);
		TInt err(0);
		
		for( TInt i = hpiArray.Count(); --i >= 0; )
			{
			CHarvesterPluginInfo* hpi = hpiArray[i];
			for( TInt j = hpi->iQueue.Count(); --j >= 0; )
				{
				CHarvesterData* hd = hpi->iQueue[j];
				CMdEObject& mdeobj = hd->MdeObject();
				
				err = iMediaIdUtil->GetMediaId( mdeobj.Uri(), mediaId );
	
				if( mdeobj.Placeholder() && ( aMediaId == mediaId || err != KErrNone ))
					{
					removed++;
					                    
					TItemId id = mdeobj.Id();
					placeholders.Append( id );
					TRAP_IGNORE( iMdESession->CancelObjectL( mdeobj ) );
					
					delete hd;
					hd = NULL;
					hpi->iQueue.Remove(j);
					
					if( hpi->iQueue.Count() == 0 )
						{
						hpi->iQueue.Compress();
						}
					}
				}
			}
		
		if( removed )
		    {
            WRITELOG1( "CHarvesterAO::HandleUnmount() remove from plugins = %d", removed);
            TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder, removed ) );
            TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, removed ) );
		    }
		
		RArray<TItemId> results;
		TRAP_IGNORE( iMdESession->RemoveObjectsL( placeholders, results, NULL ) );
		results.Close();
		placeholders.Close();
		}
	}

// ---------------------------------------------------------------------------
// StartComposersL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::StartComposersL()
    {
    WRITELOG( "CHarvesterAO::StartComposersL()" );
    
    RImplInfoPtrArray infoArray;
    TCleanupItem cleanupItem( MdsUtils::CleanupEComArray, &infoArray );
    CleanupStack::PushL( cleanupItem );
    CComposerPlugin::ListImplementationsL( infoArray );
    TInt count( 0 );
    count = infoArray.Count();

    for ( TInt i=0; i < count; i++ )
        {
        TUid uid = infoArray[i]->ImplementationUid();
        CComposerPlugin* plugin = NULL;
        TRAPD(trapError, plugin = CComposerPlugin::NewL( uid ) );
        if ( trapError != KErrNone )
            {
            WRITELOG( "CHarvesterAO::StartComposersL() - failed to create new composer" );
            }
        else
            {
            plugin->SetSession( iMdEHarvesterSession->SessionRef() );
            iComposerPluginArray.AppendL( plugin );            
            }
        }
            
    CleanupStack::PopAndDestroy( &infoArray );    
    WRITELOG( "CHarvesterAO::StartComposersL() - end" );
    }

// ---------------------------------------------------------------------------
// StopComposers
// ---------------------------------------------------------------------------
//
void CHarvesterAO::StopComposers()
    {
    WRITELOG( "CHarvesterAO::StopComposers()" );
    
    for ( TInt i = iComposerPluginArray.Count(); --i >= 0; )
        {
        iComposerPluginArray[i]->RemoveSession();
        }

    WRITELOG( "CHarvesterAO::StopComposers() - end" );
    }

// ---------------------------------------------------------------------------
// DeleteComposers
// ---------------------------------------------------------------------------
//
void CHarvesterAO::DeleteComposers()
    {
    WRITELOG( "CHarvesterAO::DeleteComposers()" );
    
    iComposerPluginArray.ResetAndDestroy();

    WRITELOG( "CHarvesterAO::DeleteComposers() - end" );
    }

// ---------------------------------------------------------------------------
// IsComposingReady
// ---------------------------------------------------------------------------
//
TBool CHarvesterAO::IsComposingReady()
    {
    WRITELOG( "CHarvesterAO::IsComposingReady()" );
    
    for ( TInt i = iComposerPluginArray.Count(); --i >= 0; )
        {
        if ( iComposerPluginArray[i]->IsComposingComplete() == EFalse )
            {
            return EFalse;
            }
        }

    WRITELOG( "CHarvesterAO::IsComposingReady() - end" );
    return ETrue;
    }

// ---------------------------------------------------------------------------
// ReadItemFromQueueL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ReadItemFromQueueL()
    {
    WRITELOG( "CHarvesterAO::ReadItemFromQueueL()" );
    
    CHarvesterData* hd = iQueue->GetNextItem();
    
    if ( hd->ObjectType() == EPlaceholder )
    	{
    	while( hd != NULL &&
				iPHArray.Count() < KPlaceholderQueueSize &&
				hd->ObjectType() == EPlaceholder )
    		{
        	iPHArray.Append( hd );
        	hd = iQueue->GetNextItem();
    		}
    	
		if( hd )
    		{
	    	if( hd->ObjectType() == EPlaceholder )
	    		{
	        	iPHArray.Append( hd );
	    		}
	    	else
	    		{
	    		CheckFileExtensionAndHarvestL( hd );
	    		}
    		}
			
    	if( iPHArray.Count() > 0 )
    		{
	    	TRAPD( err, HandlePlaceholdersL( ETrue ) );

	    	// make sure that when HandlePlaceholdersL leaves, iPHArray is cleared
	    	if ( err != KErrNone )
	    		{
	    		iPHArray.ResetAndDestroy();
	    		User::Leave( err );
	    		}
    		}
    	}
    else
    	{
        CheckFileExtensionAndHarvestL( hd );
    	}
    }

// ---------------------------------------------------------------------------
// HandlePlaceholdersL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HandlePlaceholdersL( TBool aCheck )
	{
	WRITELOG( "CHarvesterAO::HandlePlaceholdersL()" );

	RPointerArray<CMdEObject> mdeObjectArray;
	CleanupClosePushL( mdeObjectArray );

	TTimeIntervalSeconds timeOffsetSeconds = User::UTCOffset();
	
	TInt fastHarvestPlaceholderCount = 0;
	
	for (TInt i = iPHArray.Count() ; --i >= 0;)
		{
		CHarvesterData* hd = iPHArray[i];
		
		if( aCheck && iHarvesterPluginFactory->IsContainerFileL( hd->Uri() ) )
			{
			iContainerPHArray.Append( hd );
			iPHArray.Remove( i );
			continue;
			}
		TBuf<KObjectDefStrSize> objDefStr;
		iHarvesterPluginFactory->GetObjectDefL( *hd, objDefStr );
	    
		if( objDefStr.Length() == 0 ||
		    ( objDefStr == KInUse ) )
			{
			const TInt error( KErrUnknown );
            // notify observer, notification is needed even if file is not supported
            HarvestCompleted( hd->ClientId(), hd->Uri(), error );
			delete hd;
			hd = NULL;
			iPHArray.Remove( i );
			iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 );
			continue;
			}

		CMdENamespaceDef& defNS = iMdESession->GetDefaultNamespaceDefL();
		CMdEObjectDef& mdeObjectDef = defNS.GetObjectDefL( objDefStr );

		CMdEObject* mdeObject = iMdESession->NewObjectL( mdeObjectDef, hd->Uri() );
		
		CPlaceholderData* phData = NULL;

		if( hd->TakeSnapshot() )
			{
		    phData = CPlaceholderData::NewL();
		    CleanupStack::PushL( phData );
		    TEntry* entry = new (ELeave) TEntry();
		    CleanupStack::PushL( entry );
		    const TDesC& uri = hd->Uri();
		    TInt err = iFs.Entry( uri, *entry );
		    if ( err != KErrNone )
		    	{
		    	WRITELOG( "CHarvesterAO::HandlePlaceholdersL() - cannot create placeholder data object for camera. file does not exists" );
	    		// notify observer
	    	    HarvestCompleted( hd->ClientId(), hd->Uri(), err );
				delete hd;
				hd = NULL;
				iPHArray.Remove( i );
				iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 );
				CleanupStack::PopAndDestroy( entry );
				CleanupStack::PopAndDestroy( phData );
				continue;
		    	}
		    phData->SetUri( uri );
			phData->SetModified( entry->iModified );
			phData->SetFileSize( entry->iSize );
			CleanupStack::PopAndDestroy( entry );

			TUint32 mediaId( 0 );
			User::LeaveIfError( iMediaIdUtil->GetMediaId( uri, mediaId ) );
			phData->SetMediaId( mediaId );
			}
		else
			{
			phData = static_cast<CPlaceholderData*> ( hd->ClientData() );
		    if( !phData )
		    	{
		    	WRITELOG( "CHarvesterAO::HandlePlaceholdersL() - Placeholder data object NULL - abort" );
		    	const TInt error( KErrUnknown );
	    		// notify observer
	    	    HarvestCompleted( hd->ClientId(), hd->Uri(), error );
				delete hd;
				hd = NULL;
				iPHArray.Remove( i );
				iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 );
				continue;
		    	}	
			CleanupStack::PushL( phData );
			}	
		
		// set media id
		mdeObject->SetMediaId( phData->MediaId() );
	    
	    // set placeholder
	    mdeObject->SetPlaceholder( ETrue );
	    
	    if( !iPropDefs )
	    	{
	    	iPropDefs = CHarvesterAoPropertyDefs::NewL( mdeObjectDef );
	    	}

	    // set file size
    	mdeObject->AddUint32PropertyL( *iPropDefs->iSizePropertyDef, phData->FileSize() );

	    // set creation date
    	TTime localModifiedDate = phData->Modified() + timeOffsetSeconds;
    	mdeObject->AddTimePropertyL( *iPropDefs->iCreationDatePropertyDef, localModifiedDate );

	    // set modification date
    	mdeObject->AddTimePropertyL( *iPropDefs->iLastModifiedDatePropertyDef, phData->Modified() );
	    
    	// set origin
		mdeObject->AddUint8PropertyL( *iPropDefs->iOriginPropertyDef, hd->Origin() );
    	
    	CPlaceholderData* ph = static_cast<CPlaceholderData*>( hd->ClientData() );
	    TInt isPreinstalled = ph->Preinstalled();
	    if( isPreinstalled == MdeConstants::MediaObject::EPreinstalled )
	    	{
	    	WRITELOG("CHarvesterAO::HandlePlaceholdersL() - preinstalled");
	    	mdeObject->AddInt32PropertyL( *iPropDefs->iPreinstalledPropertyDef, isPreinstalled );
	    	}	    
		
	    hd->SetEventType( EHarvesterEdit );
	    
		// skip 
		if( hd->TakeSnapshot() )
			{
			fastHarvestPlaceholderCount++;
			hd->SetObjectType( EFastHarvest );
			}
		else
			{
			hd->SetClientData( NULL );
			hd->SetObjectType( ENormal );
			}
		
		hd->SetMdeObject( mdeObject );
		
		mdeObjectArray.Append( mdeObject );
		
	    CleanupStack::PopAndDestroy( phData );
		
		iReadyPHArray.Append( hd );
		iPHArray.Remove( i );
		}
	
	TInt objectCount = mdeObjectArray.Count();  
	
    if( objectCount > 0 )
		{
		// add object to mde
		iMdEHarvesterSession->AutoLockL( mdeObjectArray );
		const TInt addError( iMdESession->AddObjectsL( mdeObjectArray ) );
		if( addError != KErrNone )
		    {
		    // If some error occures, retry
		    iMdESession->AddObjectsL( mdeObjectArray );
		    }

		const TInt eventObjectCount = objectCount - fastHarvestPlaceholderCount;

		if( eventObjectCount > 0 )
			{
			iHarvesterEventManager->IncreaseItemCount( EHEObserverTypePlaceholder, 
					eventObjectCount );
			iHarvesterEventManager->SendEventL( EHEObserverTypePlaceholder, EHEStateStarted, 
					iHarvesterEventManager->ItemCount( EHEObserverTypePlaceholder ) );
			}
		
#ifdef _DEBUG
		for (TInt i = 0; i < mdeObjectArray.Count(); ++i)
			{
			CMdEObject* mdeObject = mdeObjectArray[i];
			if(mdeObject->Id() == 0)
				{
				WRITELOG1( "CHarvesterAO::HandlePlaceholdersL() - failed to add: %S", &mdeObject->Uri() );
				}
			}
#endif
		}

	iPHArray.ResetAndDestroy();
	
	CleanupStack::PopAndDestroy( &mdeObjectArray );
	}

// ---------------------------------------------------------------------------
// CheckFileExtensionAndHarvestL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::CheckFileExtensionAndHarvestL( CHarvesterData* aHD )
    {
    TBool isError = EFalse;
    CMdEObject* mdeObject = &aHD->MdeObject();
    const TDesC& uri = aHD->Uri();
    TBool objectExisted = ETrue;
    
    if( ! mdeObject )
    	{
    	objectExisted = EFalse;
    	WRITELOG1( "CHarvesterAO::CheckFileExtensionAndHarvestL() - no mdeobject. URI: %S", &uri );
	    TBuf<KObjectDefStrSize> objDefStr;
		iHarvesterPluginFactory->GetObjectDefL( *aHD, objDefStr );
		
		if( objDefStr.Length() == 0 )
			{
			WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - cannot get object definition" );
			isError = ETrue;
			}
		else if( objDefStr == KInUse )
		    {
            aHD->SetErrorCode( KMdEErrHarvestingFailed );
            HarvestingCompleted( aHD );
            return;
		    }
		else
			{
			TInt mdeError( KErrNone );
			
			// Check if non-binary object (messages) already exists in db 
			if ( !aHD->IsBinary() )
				{
				CMdENamespaceDef& defNS = iMdESession->GetDefaultNamespaceDefL();
				CMdEObjectDef& mdeObjectDef = defNS.GetObjectDefL( objDefStr );
				TRAP( mdeError, mdeObject = iMdESession->OpenObjectL( aHD->Uri(), mdeObjectDef ));
				}
			
			if ( mdeObject )
				{
				aHD->SetTakeSnapshot( EFalse );
				aHD->SetEventType( EHarvesterEdit );
				}
			else
				{
				WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - getting mdeobject" );
				TRAP( mdeError, mdeObject = iMdeObjectHandler->GetMetadataObjectL( *aHD, objDefStr ) );
				}
			TInt harvesterError = KErrNone;
			if( mdeError != KErrNone)
				{
				WRITELOG1( "CHarvesterAO::CheckFileExtensionAndHarvestL() - cannot get mde object. error: %d", mdeError );
				MdsUtils::ConvertTrapError( mdeError, harvesterError );
				if( harvesterError == KMdEErrHarvestingFailedPermanent )
					{
					WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - permanent fail" );
					isError = ETrue;
					}
				else if ( harvesterError == KMdEErrHarvestingFailed )
					{
	                WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - KMdEErrHarvestingFailed");
	                aHD->SetErrorCode( KMdEErrHarvestingFailed );
	                HarvestingCompleted( aHD );
	                return;
					}
				}

			if( !mdeObject )
				{
				WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - mde object is null. stop harvesting" );
				isError = ETrue;
				}
			}
		if( isError )
			{
            aHD->SetErrorCode( KMdEErrHarvestingFailedPermanent );
            HarvestingCompleted( aHD );
            return;
			}
		
		CleanupStack::PushL( aHD );
		
		TUint32 mediaId( 0 );
		if ( aHD->IsBinary() )
			{
			User::LeaveIfError( iMediaIdUtil->GetMediaId( uri, mediaId ) );
			}
		mdeObject->SetMediaId( mediaId );
		
		aHD->SetMdeObject( mdeObject );
		CleanupStack::Pop( aHD );
    	}
    
#ifdef _DEBUG
    WRITELOG1("CHarvesterAO::CheckFileExtensionAndHarvestL() - mdeobject URI: %S", &mdeObject->Uri() );
#endif
	
	aHD->SetPluginObserver( *this );
	
    if( objectExisted && aHD->EventType() == EHarvesterAdd )
    	{
    	iMdESession->RemoveObjectL( aHD->Uri() );
    	}
	
	TInt pluginErr = KErrNone;
    TRAPD( err, pluginErr = iHarvesterPluginFactory->HarvestL( aHD ));
    if ( err != KErrNone )
    	{
    	WRITELOG1( "CHarvesterAO::CheckFileExtensionAndHarvestL() - plugin error: %d", err );
    	if ( err == KErrInUse )
    		{
            WRITELOG( "CHarvesterAO::CheckFileExtensionAndHarvestL() - item in use" );
            aHD->SetErrorCode( KMdEErrHarvestingFailed );
            HarvestingCompleted( aHD );
            return;
    		}
    	
    	aHD->SetErrorCode( KMdEErrHarvestingFailedUnknown );
    	HarvestingCompleted( aHD );
    	return;
    	}
    
    if( pluginErr != KErrNone )
    	{
    	aHD->SetErrorCode( KMdEErrHarvestingFailedUnknown );
    	HarvestingCompleted( aHD );
    	return;
    	}
    
    WRITELOG1("CHarvesterAO::CheckFileExtensionAndHarvestL() - ends with error %d", pluginErr );
    SetNextRequest( ERequestHarvest );
    }

// ---------------------------------------------------------------------------
// HarvestingCompleted
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HarvestingCompleted( CHarvesterData* aHD )
    {
    WRITELOG( "CHarvesterAO::HarvestingCompleted()" );
    
    if ( aHD->ErrorCode() == KErrNone )
        {
        iReHarvester->CheckItem( *aHD );

        TInt mdeError = KErrNone;
        if( !aHD->TakeSnapshot() )
        	{
        	WRITELOG( "CHarvesterAO::HarvestingCompleted() origin is not camera or clf" );
            aHD->MdeObject().SetPlaceholder( EFalse );
            TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypePlaceholder ) );
        	TRAP( mdeError, iMdeObjectHandler->SetMetadataObjectL( *aHD ) );
        	}

        if(mdeError != KErrNone)
        	{
        	WRITELOG( "==============================ERROR===============================" );
            WRITELOG( "CHarvesterAO::HarvestingCompleted() - cannot set metadata object" );
            WRITELOG( "==============================ERROR done =========================" );
            delete aHD;
            aHD = NULL;

            TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 ) );
        	}
        else
        	{
        	WRITELOG( "CHarvesterAO::HarvestingCompleted() mdeError == KErrNone" );
	        if ( aHD->TakeSnapshot() && iCtxEngine )
	            {
	            WRITELOG( "CHarvesterAO::HarvestingCompleted() - Taking a context snapshot." );
	            iCtxEngine->ContextSnapshot( *this, *aHD );
	            }
	        else
	        	{
	        	TLocationData* locData = aHD->LocationData();
	        	if( locData )
	        		{
	        		WRITELOG( "CHarvesterAO::HarvestingCompleted() - Creating location object. " );
	        		RLocationObjectManipulator lo;
	        		
	        		TInt loError = KErrNone;     		
	        		loError = lo.Connect();
	        		
	        		if (loError == KErrNone)
	        			{
	        			TInt err = lo.CreateLocationObject( *locData, aHD->MdeObject().Id() );
	        			if( err != KErrNone )
	        				{
	        				WRITELOG( "CHarvesterAO::HarvestingCompleted() - Location object creation failed!!!" );
	        				}
	        			}
	        		else
	        			{
	        			WRITELOG( "CHarvesterAO::HarvestingCompleted() - LocationObjectManipulator connect failed!!!" );
	        			}
	        		
	        		lo.Close();
	        		}
	        	
	        	TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 ) );
	        	
				delete aHD;
				aHD = NULL;
	        	}
        	}        
        }
    else
        {
#ifdef _DEBUG
        WRITELOG( "==============================ERROR===============================" );
        WRITELOG1( "CHarvesterAO::HarvestingCompleted() - not OK! Error: %d", aHD->ErrorCode() );
#endif
				 
		const TInt errorCode( aHD->ErrorCode() );
        if ( errorCode== KMdEErrHarvestingFailed )
            {
#ifdef _DEBUG
            WRITELOG1("CHarvesterAO::HarvestingCompleted() - KMdEErrHarvestingFailed - %S - reharvesting", &aHD->Uri() );
#endif
            iReHarvester->AddItem( aHD );
            }
        else if ( errorCode == KMdEErrHarvestingFailedPermanent ||
                errorCode == KMdEErrHarvestingFailedUnknown )
            {
            WRITELOG( "CHarvesterAO::HarvestingCompleted() - KMdEErrHarvestingFailedPermanent - no need to re-harvest!" );
            
            TRAP_IGNORE( iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeMMC, 1 ) );
            
            delete aHD;
			aHD = NULL;
			return;
            }
        else
            {
            WRITELOG1( "CHarvesterAO::HarvestingCompleted() - unknown error: %d", errorCode );
            }
        
        WRITELOG( "==============================ERROR done =========================" );
        }
           
    SetNextRequest( ERequestHarvest );
    }

// ---------------------------------------------------------------------------
// HandleSessionOpened
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HandleSessionOpened( CMdESession& aSession, TInt aError )
    {
    WRITELOG( "HarvesterThread::HandleSessionOpened()" );
    if ( KErrNone == aError )
        {
        TBool isTNMDaemonEnabled( EFalse );
        TRAP_IGNORE( CHarvesterCenRepUtil::IsThumbnailDaemonEnabledL( isTNMDaemonEnabled ) );

        if( isTNMDaemonEnabled )
            {
            StartThumbAGDaemon();
            }
        
        TRAPD( errorTrap, iMdEHarvesterSession = CMdEHarvesterSession::NewL( *iMdESession ) );
        if ( errorTrap == KErrNone )
            {
        	iMdeSessionInitialized = ETrue;
            }
        else
        	{
            WRITELOG( "CHarvesterAO::HandleSessionOpened() - error creating mde harvester session" );
        	}
#ifdef _DEBUG
        TRAP( errorTrap, PreallocateNamespaceL( aSession.GetDefaultNamespaceDefL() ) );
        if ( errorTrap != KErrNone )
            {
            WRITELOG( "CHarvesterAO::HandleSessionOpened() - error loading default schema" );
            }
        
        // Setting up monitor plugins
        TRAP( errorTrap, LoadMonitorPluginsL() );
        if ( errorTrap != KErrNone )
            {
            WRITELOG( "CHarvesterAO::HandleSessionOpened() - error loading monitor plugins" );
            }
            
        TRAP( errorTrap, StartComposersL() );
        if ( errorTrap != KErrNone )
            {
            WRITELOG( "CHarvesterAO::HandleSessionOpened() - couldn't start composer plugins" );
            }
#else
        // The idea here is that all of these three methods needs to be called,
        // even if some leave, thus the three TRAPs
        TRAP_IGNORE( PreallocateNamespaceL( aSession.GetDefaultNamespaceDefL() ) );
        TRAP_IGNORE( LoadMonitorPluginsL() );
        TRAP_IGNORE( StartComposersL() );
        
#endif

        if ( iContextEngineInitialized )
            {
            iCtxEngine->SetMdeSession( iMdESession );
            }

            // Starting monitor plugins
        StartMonitoring();
        
        TRAP( errorTrap, iOnDemandAO = COnDemandAO::NewL( *iMdESession, *iQueue, 
        		*iHarvesterPluginFactory, &iReadyPHArray ) );
        if ( errorTrap == KErrNone )
        	{
        	TRAP( errorTrap, iOnDemandAO->StartL() );
        	if ( errorTrap != KErrNone )
        		{
        		WRITELOG( "CHarvesterAO::HandleSessionOpened() - couldn't start on demand observer" );
        		}
        	}
        else
        	{
        	WRITELOG( "CHarvesterAO::HandleSessionOpened() - couldn't create on demand observer" );
        	}

    	TRAPD( ohTrap, iMdeObjectHandler = CMdeObjectHandler::NewL( *iMdESession ) );
    	if ( ohTrap != KErrNone )
				{
				WRITELOG( "CHarvesterAO::HandleSessionOpened() - ObjectHandler creation failed" );
				}
    	
        // Initializing pause indicator
        iServerPaused = EFalse;
#ifdef _DEBUG
        WRITELOG( "HarvesterThread::HandleSessionOpened() - Succeeded!" );
        
        TBool isRomScanEnabled( EFalse );
        TRAP_IGNORE( CHarvesterCenRepUtil::IsRomScanEnabledL( isRomScanEnabled ) );
        
        if( isRomScanEnabled )
            {
            TRAP( errorTrap, BootRomScanL() );
            if( errorTrap != KErrNone )
                {
                WRITELOG1( "CHarvesterAO::HandleSessionOpened() - BootRomScanL() returned error: %d", errorTrap );
                }
            }

        TRAP( errorTrap, BootPartialRestoreScanL() );
        if( errorTrap != KErrNone )
        	{
        	WRITELOG1( "CHarvesterAO::HandleSessionOpened() - BootPartialRestoreScanL() returned error: %d", errorTrap );
        	}
#else
        // The idea here is that all of these three methods needs to be called,
        // even if some leave, thus the two TRAPs
        TBool isRomScanEnabled( EFalse );
        TRAP_IGNORE( CHarvesterCenRepUtil::IsRomScanEnabledL( isRomScanEnabled ) );
        
        if( isRomScanEnabled )
            {
            TRAP_IGNORE( BootRomScanL() );
            }
        TRAP_IGNORE( BootPartialRestoreScanL() );
#endif
        }
    else
        {
        iServerPaused = ETrue;    
        WRITELOG1( "HarvesterThread::HandleSessionOpened() - Failed: %d!", aError );
        }
    }

// ---------------------------------------------------------------------------
// HandleSessionError
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {    
    if ( KErrNone != aError )
        {
        WRITELOG1( "HarvesterThread::HandleSessionError() - Error: %d!", aError );        
        }
    }

// ---------------------------------------------------------------------------
// ContextInitializationStatus
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ContextInitializationStatus( TInt aErrorCode )
    {
    WRITELOG( "CHarvesterAO::ContextInitializationStatus()" );
    
    if ( KErrNone == aErrorCode )
        {
        WRITELOG( "HarvesterThread::ContextInitializationStatus() - Succeeded!" );
        iContextEngineInitialized = ETrue;
        if ( iMdeSessionInitialized )
            {
            iCtxEngine->SetMdeSession( iMdESession );
            }
        }
    else
        {
        WRITELOG1( "HarvesterThread::ContextInitializationStatus() - Failed: %d!", aErrorCode );
        }
    }

// ---------------------------------------------------------------------------
// PauseHarvester
// ---------------------------------------------------------------------------
//
TInt CHarvesterAO::PauseHarvester()
    {
    WRITELOG( "CHarvesterAO::PauseHarvester()" );

    iServerPaused = ETrue;
    
    // Everything is paused
    WRITELOG( "CHarvesterAO::PauseHarvester() - Moving paused state paused" );
    
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// ResumeHarvester
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ResumeHarvesterL()
    {
    WRITELOG( "CHarvesterAO::ResumeHarvesterL()" );
    
    iServerPaused = EFalse;
    
    SetNextRequest( ERequestHarvest );
    }

// ---------------------------------------------------------------------------
// RunL
// ---------------------------------------------------------------------------
//
void CHarvesterAO::RunL()
    {
    WRITELOG( "CHarvesterAO::RunL" );
    // check if pause is requested
    if ( this->iServerPaused && iNextRequest != ERequestPause && iNextRequest != ERequestResume)
    	{
    	iNextRequest = ERequestIdle;
    	}
    User::LeaveIfError( iStatus.Int() );
    switch( iNextRequest )
        {
        // no more items in queue
        case ERequestIdle:
            {
            WRITELOG( "CHarvesterAO::RunL - ERequestIdle" );
            }
        break;

        // data added to harvester queue
        case ERequestHarvest:
            {
            WRITELOG( "CHarvesterAO::RunL - ERequestHarvest" );
            
            // harvest new items first...
            if ( iQueue->ItemsInQueue() > 0 )
                {
                if ( !iHarvesting )
                	{
                	iHarvesting = ETrue;
                	iHarvesterEventManager->SendEventL( EHEObserverTypeOverall, EHEStateStarted );
                	// This next line is for caching the harvester started event for observers registering
                	// after harvesting has already started
                	iHarvesterEventManager->IncreaseItemCount( EHEObserverTypeOverall, KCacheItemCountForEventCaching );
                	}
                
                ReadItemFromQueueL();
				SetNextRequest( ERequestHarvest );
                }

            // no more items to harvest
            else
                {
                
                // if container files to harvest, handle those
                if( iContainerPHArray.Count() > 0 )
                	{
                	SetNextRequest( ERequestContainerPlaceholder );
                	break;
                	}
                
				if(iReadyPHArray.Count() > 0)
            		{
#ifdef _DEBUG
            		WRITELOG1("CHarvesterAO::RunL - items in ready pharray: %d", iReadyPHArray.Count() );
#endif   		
            		for ( TInt i = iReadyPHArray.Count(); --i >= 0; )
            			{
                		CheckFileExtensionAndHarvestL( iReadyPHArray[i] );
                		iReadyPHArray.Remove( i );
            			}
            		iReadyPHArray.Reset();
            		}
				
				if ( iHarvesting && !UnharvestedItemsLeftInPlugins() && 
					 !iReHarvester->ItemsInQueue() )
					{
					iHarvesting = EFalse;						
					iHarvesterEventManager->SendEventL( EHEObserverTypeOverall, EHEStateFinished );
					iHarvesterEventManager->DecreaseItemCountL( EHEObserverTypeOverall, KCacheItemCountForEventCaching );
				    iReadyPHArray.Compress();
				    iContainerPHArray.Compress();
				    iPHArray.Compress();
					}

                SetNextRequest( ERequestIdle );
                }
            }
        break;
        
        case ERequestContainerPlaceholder:
        	{
#ifdef _DEBUG
        	WRITELOG( "CHarvesterAO::RunL - ERequestContainerPlaceholder" );
        	WRITELOG1( "CHarvesterAO::RunL - Items in container pharray: %d", iContainerPHArray.Count() );
#endif
        	TInt count = iContainerPHArray.Count() > KContainerPlaceholderQueueSize ? KContainerPlaceholderQueueSize : iContainerPHArray.Count();
        	TInt i = 0;
        	while( i < count )
        		{
        		CHarvesterData* hd = iContainerPHArray[0];
        		iPHArray.Append( hd );
        		iContainerPHArray.Remove( 0 );
        		i++;
        		}
        	TRAPD( err, HandlePlaceholdersL( EFalse ) );

	    	// make sure that when HandlePlaceholdersL leaves, iPHArray is cleared
	    	if ( err != KErrNone )
	    		{
	    		iPHArray.ResetAndDestroy();
	    		User::Leave( err );
	    		}
	    	SetNextRequest( ERequestHarvest );
        	}
        break;
        
        // pause request
        case ERequestPause:
            {
            WRITELOG( "CHarvesterAO::RunL - ERequestPause" );
            User::LeaveIfError( PauseHarvester() );
            iHarvesterEventManager->SendEventL( EHEObserverTypeOverall, EHEStatePaused );
            if( iHarvesterStatusObserver )
            	{
            	iHarvesterStatusObserver->PauseReady( KErrNone );
            	}
            }
        break;

        // resume request
        case ERequestResume:
            {
            WRITELOG( "CHarvesterAO::RunL - ERequestResume" );
            ResumeHarvesterL();
            iHarvesterEventManager->SendEventL( EHEObserverTypeOverall, EHEStateResumed );
            if( iHarvesterStatusObserver )
            	{
                iHarvesterStatusObserver->ResumeReady( KErrNone );
            	}
            SetNextRequest( ERequestHarvest );
            }
        break;
        
        default:
            {
            WRITELOG( "CHarvesterAO::RunL - Not supported request" );
            User::Leave( KErrNotSupported );
            }
        break;
        }
    }
    
// ---------------------------------------------------------------------------
// DoCancel
// ---------------------------------------------------------------------------
//
void CHarvesterAO::DoCancel()
    {
    WRITELOG( "CHarvesterAO::DoCancel()" );
    }
    
// ---------------------------------------------------------------------------
// RunError
// ---------------------------------------------------------------------------
//
TInt CHarvesterAO::RunError( TInt aError )
    {
    WRITELOG( "CHarvesterAO::RunError" );
    switch( iNextRequest )
        {
        case ERequestHarvest:
            {
            WRITELOG( "CHarvesterAO::RunError - state ERequestHarvest" );
            }
        break;
        
        case ERequestPause:
            {
            WRITELOG( "CHarvesterAO::RunError - state ERequestPause" );
            if ( aError == KErrNotReady )
                {
                SetNextRequest( ERequestPause );
                }
            else if( iHarvesterStatusObserver )
            	{
                iHarvesterStatusObserver->PauseReady( aError );
            	}
            }
        break;
        
        case ERequestResume:
            {
            WRITELOG( "CHarvesterAO::RunError - state ERequestResume" );
            if( iHarvesterStatusObserver )
            	{
                iHarvesterStatusObserver->ResumeReady( aError );
            	}
            }
        break;
        
        default:
            {
            WRITELOG( "CHarvesterAO::RunError - unknown state" );
            }
        break;
        }
        
    return KErrNone;
    }
    
// ---------------------------------------------------------------------------
// SetNextRequest
// ---------------------------------------------------------------------------
//
void CHarvesterAO::SetNextRequest( TRequest aRequest )
    {
    WRITELOG( "CHarvesterAO::SetNextRequest" );
    iNextRequest = aRequest;
            
    if ( !IsActive() )
        {
        iStatus = KRequestPending;
        SetActive();
        TRequestStatus* ptrStatus = &iStatus;
        User::RequestComplete( ptrStatus, KErrNone );
        }
    }

// ---------------------------------------------------------------------------
// IsServerPaused
// ---------------------------------------------------------------------------
//
TBool CHarvesterAO::IsServerPaused()
    {
    WRITELOG( "CHarvesterAO::IsServerPaused" );
    return iServerPaused;
    }

// ---------------------------------------------------------------------------
// From MBackupRestoreObserver.
// Called by CBlacklistBackupSubscriberAO when
// Backup&Restore is backing up or restoring.
// ---------------------------------------------------------------------------
//
void CHarvesterAO::BackupRestoreStart()
    {
    // close blacklist database connection
    WRITELOG( "CHarvesterAO::BackupRestoreStart" );
    iBlacklist->CloseDatabase();
    }

// ---------------------------------------------------------------------------
// From MBackupRestoreObserver.
// Called by CBlacklistBackupSubscriberAO when
// Backup&Restore has finished backup or restore.
// ---------------------------------------------------------------------------
//
void CHarvesterAO::BackupRestoreReady()
    {
    // restart blacklist database connection
    WRITELOG( "CHarvesterAO::BackupRestoreReady" );
    iBlacklist->OpenDatabase();
    }

// ---------------------------------------------------------------------------
// HarvestFile
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HarvestFile( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::HarvestFile" );
    const TInt KParamUri = 0;
    const TInt KParamAlbumIds = 1;
    const TInt KParamAddLocation = 2;
    
    // read uri
    HBufC* uri = HBufC::New( KMaxFileName );
    
    if ( ! uri )
        {
        WRITELOG( "CHarvesterAO::HarvestFile - out of memory creating uri container" );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrNoMemory );
            }
        return;
        }
    
    TPtr uriPtr( uri->Des() );
    TInt err = aMessage.Read( KParamUri, uriPtr );
    if ( err != KErrNone )
        {
        WRITELOG1( "CHarvesterAO::HarvestFile - error in reading aMessage (uri): %d", err );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        delete uri;
        uri = NULL;
        return;
        }
    WRITELOG1( "CHarvesterAO::HarvestFile - uri: %S", uri );
    
    // read album ids
    RArray<TItemId> albumIds;
    const TInt KAlbumIdsLength = aMessage.GetDesLength( KParamAlbumIds );
    if ( KAlbumIdsLength > 0 )
        {
        HBufC8* albumIdBuf = HBufC8::New( KAlbumIdsLength );
        if ( !albumIdBuf )
            {
            WRITELOG( "CHarvesterAO::HarvestFile - error creating album id buffer." );
            if (!aMessage.IsNull())
                {
                aMessage.Complete( KErrNoMemory );
                }
            delete uri;
            uri = NULL;
            return;
            }
        TPtr8 ptr( albumIdBuf->Des() );
        err = aMessage.Read( KParamAlbumIds, ptr );
        if ( err != KErrNone )
            {
            WRITELOG1( "CHarvesterAO::HarvestFile - error in reading aMessage (albumIds): %d", err );
            delete albumIdBuf;
            albumIdBuf = NULL;
            delete uri;
            uri = NULL;
            if (!aMessage.IsNull())
                {
                aMessage.Complete( err );
                }
            return;
            }

        TRAPD( err, DeserializeArrayL( ptr, albumIds ) );
        if ( err != KErrNone )
            {
            WRITELOG1( "CHarvesterAO::HarvestFile - error in reading album id array: %d", err );
            delete albumIdBuf;
            albumIdBuf = NULL;
            delete uri;
            uri = NULL;
            if (!aMessage.IsNull())
                {
                aMessage.Complete( err );
                }
            return;
            }

#ifdef _DEBUG
        const TInt count = albumIds.Count();
        for (TInt i = 0; i < count; ++i)
        	{
        	WRITELOG2( "RHarvesterClient::HarvestFile - album id[%d]: %d", i, albumIds[i] );
        	}
#endif
        
        delete albumIdBuf;
        albumIdBuf = NULL;
        
        WRITELOG1( "CHarvesterAO::HarvestFile - album id count: %d", albumIds.Count() );
        }
    
    TBool addLocation;    
    TPckg<TBool> location( addLocation );	
		err = aMessage.Read(KParamAddLocation, location);
		if ( err != KErrNone )
			{
			WRITELOG1( "CHarvesterAO::HarvestFile - error in reading aMessage (addLocation): %d", err );
			delete uri;
			uri = NULL;
			if (!aMessage.IsNull())
				{
				aMessage.Complete( err );
				}
			return;
			}	
	
		WRITELOG1( "RHarvesterClient::HarvestFile - add location: %d", addLocation );
    
    CHarvesterData* hd = NULL;
    TRAP( err, hd = CHarvesterData::NewL( uri ) );
    if ( err != KErrNone )
        {
        WRITELOG( "CHarvesterAO::HarvestFile - creating harvUri failed" );
        albumIds.Close();
        delete uri;
        uri = NULL;
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        return;
        }

    hd->SetEventType( EHarvesterAdd );
    hd->SetOrigin( MdeConstants::Object::ECamera );
    hd->SetObjectType( EPlaceholder );
    hd->SetTakeSnapshot( ETrue );
    hd->SetClientId( aMessage.Identity() );
    hd->SetAddLocation( addLocation );

    CHarvestClientData* clientData = CHarvestClientData::New();
    if ( clientData )
        {
        clientData->SetAlbumIds( albumIds );
        hd->SetClientData( clientData ); // ownership is transferred
        }
    else
        {
        WRITELOG( "CHarvesterAO::HarvestFile - creating clientData failed" );
        }

    if( iQueue && hd )
        {
        iQueue->Append( hd );
        
        // signal to start harvest if harvester idles
        if ( !IsServerPaused() )
            {
            SetNextRequest( CHarvesterAO::ERequestHarvest );
            }
        }
    else
        {
        err = KErrUnknown;
        }
    
    if (!aMessage.IsNull())
        {
        aMessage.Complete( err );
        }
    
    albumIds.Close();
    }
  
// ---------------------------------------------------------------------------
// HarvestFileWithUID
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HarvestFileWithUID( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::HarvestFileWithUID" );
    const TInt KParamUri = 0;
    const TInt KParamAlbumIds = 1;
    const TInt KParamAddLocation = 2;
    
    // read uri
    HBufC* uri = HBufC::New( KMaxFileName );
    
    if ( ! uri )
        {
        WRITELOG( "CHarvesterAO::HarvestFileWithUID - out of memory creating uri container" );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrNoMemory );
            }
        return;
        }
    
    TPtr uriPtr( uri->Des() );
    TInt err = aMessage.Read( KParamUri, uriPtr );
    if ( err != KErrNone )
        {
        WRITELOG1( "CHarvesterAO::HarvestFileWithUID - error in reading aMessage (uri): %d", err );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        delete uri;
        uri = NULL;
        return;
        }
    WRITELOG1( "CHarvesterAO::HarvestFileWithUID - uri: %S", uri );
    
    // read album ids
    RArray<TItemId> albumIds;
    const TInt KAlbumIdsLength = aMessage.GetDesLength( KParamAlbumIds );
    if ( KAlbumIdsLength > 0 )
        {
        HBufC8* albumIdBuf = HBufC8::New( KAlbumIdsLength );
        if ( !albumIdBuf )
            {
            WRITELOG( "CHarvesterAO::HarvestFileWithUID - error creating album id buffer." );
            if (!aMessage.IsNull())
                {
                aMessage.Complete( KErrNoMemory );
                }
            delete uri;
            uri = NULL;
            return;
            }
        TPtr8 ptr( albumIdBuf->Des() );
        err = aMessage.Read( KParamAlbumIds, ptr );
        if ( err != KErrNone )
            {
            WRITELOG1( "CHarvesterAO::HarvestFileWithUID - error in reading aMessage (albumIds): %d", err );
            delete albumIdBuf;
            albumIdBuf = NULL;
            delete uri;
            uri = NULL;
            if (!aMessage.IsNull())
                {
                aMessage.Complete( err );
                }
            return;
            }

        TRAPD( err, DeserializeArrayL( ptr, albumIds ) );
        if ( err != KErrNone )
            {
            WRITELOG1( "CHarvesterAO::HarvestFileWithUID - error in reading album id array: %d", err );
            delete albumIdBuf;
            albumIdBuf = NULL;
            delete uri;
            uri = NULL;
            if (!aMessage.IsNull())
                {
                aMessage.Complete( err );
                }
            return;
            }

        #ifdef _DEBUG
        const TInt count = albumIds.Count();
        for (TInt i = 0; i < count; ++i)
            {
            WRITELOG2( "RHarvesterClient::HarvestFileWithUID - album id[%d]: %d", i, albumIds[i] );
            }
        #endif
        
        delete albumIdBuf;
        albumIdBuf = NULL;
		
#ifdef _DEBUG        
        WRITELOG1( "CHarvesterAO::HarvestFileWithUID - album id count: %d", albumIds.Count() );
#endif
        }
    
    TBool addLocation;    
    TPckg<TBool> location( addLocation );   
        err = aMessage.Read(KParamAddLocation, location);
        if ( err != KErrNone )
            {
            WRITELOG1( "CHarvesterAO::HarvestFileWithUID - error in reading aMessage (addLocation): %d", err );
            delete uri;
            uri = NULL;
            if (!aMessage.IsNull())
                {
                aMessage.Complete( err );
                }
            return;
            }   
    
        WRITELOG1( "RHarvesterClient::HarvestFileWithUID - add location: %d", addLocation );
    
    CHarvesterData* hd = NULL;
    TRAP( err, hd = CHarvesterData::NewL( uri ) );
    if ( err != KErrNone )
        {
        WRITELOG( "CHarvesterAO::HarvestFileWithUID - creating harvUri failed" );
        albumIds.Close();
        delete uri;
        uri = NULL;
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        return;
        }

    hd->SetEventType( EHarvesterAdd );
    hd->SetOrigin( MdeConstants::Object::EOther );
    hd->SetObjectType( EPlaceholder );
    hd->SetTakeSnapshot( ETrue );
    hd->SetClientId( aMessage.Identity() );
    hd->SetAddLocation( addLocation );

    CHarvestClientData* clientData = CHarvestClientData::New();
    if ( clientData )
        {
        clientData->SetAlbumIds( albumIds );
        hd->SetClientData( clientData ); // ownership is transferred
        }
    else
        {
        WRITELOG( "CHarvesterAO::HarvestFileWithUID - creating clientData failed" );
        }

    if( iQueue && hd )
    	{
    	iQueue->Append( hd );

    	// signal to start harvest if harvester idles
    	if ( !IsServerPaused() )
    		{
    		SetNextRequest( CHarvesterAO::ERequestHarvest );
    		}
    	}
    else
        {
        err = KErrUnknown;
        }

    if (!aMessage.IsNull())
        {
        aMessage.Complete( err );
        }
    
    albumIds.Close();
    }

// ---------------------------------------------------------------------------
// RegisterProcessOrigin()
// ---------------------------------------------------------------------------
//
void CHarvesterAO::RegisterProcessOrigin( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::RegisterProcessOrigin" );
    
    if ( !iProcessOriginMapper )
        {
        WRITELOG( "CHarvesterAO::RegisterProcessOrigin - mapper not available." );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrNotSupported );
            }
        return;
        }

    TUid processId = { 0 };
    processId.iUid = aMessage.Int0();
    if ( MdsUtils::IsValidProcessId( processId ) )
        {
        WRITELOG1( "CHarvesterAO::RegisterProcessOrigin - error reading processId. Read: %d", processId.iUid );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrCorrupt );
            }
        return;
        }
    
    // read origin

    TOrigin origin = STATIC_CAST( TOrigin, aMessage.Int1() );
    WRITELOG1( "CHarvesterAO::RegisterProcessOrigin - origin: %d", origin );
    if ( origin < 0 )
        {
        WRITELOG( "CHarvesterAO::RegisterProcessOrigin - error reading origin from aMessage (negative)." );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrCorrupt );
            }
        return;
        }

    TRAPD( err, iProcessOriginMapper->RegisterProcessL( processId, origin ) );
    if ( err != KErrNone )
        {
        WRITELOG1( "CHarvesterAO::RegisterProcessOrigin - error registering mapping: %d", err );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        return;
        }
    if (!aMessage.IsNull())
        {
        aMessage.Complete( KErrNone );
        }
    }

// ---------------------------------------------------------------------------
// UnregisterProcessOrigin()
// ---------------------------------------------------------------------------
//
void CHarvesterAO::UnregisterProcessOrigin( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::UnregisterProcessOrigin" );
    
    if ( !iProcessOriginMapper )
        {
        WRITELOG( "CHarvesterAO::RegisterProcessOrigin - mapper not available." );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrNotSupported );
            }
        return;
        }

    TUid processId = { 0 };
    processId.iUid = aMessage.Int0();
    if ( MdsUtils::IsValidProcessId( processId ) )
        {
        WRITELOG1( "CHarvesterAO::UnregisterProcessOrigin - error reading processId. Read: %d", processId.iUid );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( KErrCorrupt );
            }
        return;
        }
    
    TRAPD( err, iProcessOriginMapper->UnregisterProcessL( processId ) );
    if ( err != KErrNone )
        {
        WRITELOG1( "CHarvesterAO::UnregisterProcessOrigin - error unregistering mapping: %d", err );
        if (!aMessage.IsNull())
            {
            aMessage.Complete( err );
            }
        return;
        }
    if (!aMessage.IsNull())
        {
        aMessage.Complete( KErrNone );
        }
    }

// ---------------------------------------------------------------------------
// RegisterHarvestComplete()
// ---------------------------------------------------------------------------
//
TInt CHarvesterAO::RegisterHarvestComplete( const CHarvesterServerSession& aSession, const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::RegisterHarvestComplete" );
    
    return iHarvestFileMessages.Append( 
    		THarvestFileRequest( aSession, aMessage ) );
    }

// ---------------------------------------------------------------------------
// UnregisterHarvestComplete()
// ---------------------------------------------------------------------------
//
TInt CHarvesterAO::UnregisterHarvestComplete( const CHarvesterServerSession& aSession )
    {    
    WRITELOG( "CHarvesterAO::UnregisterHarvestComplete" );
    
    TInt err( KErrNotFound );
    if ( iHarvestFileMessages.Count() > 0 )
        {
        for ( TInt i = iHarvestFileMessages.Count()-1; i >= 0; --i )
            {
            THarvestFileRequest& req = iHarvestFileMessages[i];
            
            if ( req.iMessage.IsNull() )
            	{
            	iHarvestFileMessages.Remove( i );
            	continue;
            	}
            
            //if (aMessage.Identity() == msg.Identity())
            if( &req.iSession == &aSession )
            	{
            	err = KErrNone;
	            if (!req.iMessage.IsNull())
	            	{
	            	// cancels found request
	            	req.iMessage.Complete( KErrCancel );
	            	}
                iHarvestFileMessages.Remove( i );
                
                if( iHarvestFileMessages.Count() == 0 )
            		{
            		iHarvestFileMessages.Compress();
            		}
            	}
            }
        }

    return err;
    }

// ---------------------------------------------------------------------------
// RegisterHarvesterEvent()
// ---------------------------------------------------------------------------
//
void CHarvesterAO::RegisterHarvesterEvent( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::RegisterHarvesterEvent" );

    TRAPD( err, iHarvesterEventManager->RegisterEventObserverL( aMessage ) );
    aMessage.Complete( err );
    }

// ---------------------------------------------------------------------------
// UnregisterHarvesterEvent()
// ---------------------------------------------------------------------------
//
void CHarvesterAO::UnregisterHarvesterEvent( const RMessage2& aMessage )
    {
    WRITELOG( "CHarvesterAO::UnregisterHarvesterEvent" );
    
    const TInt err = iHarvesterEventManager->UnregisterEventObserver( aMessage );
    aMessage.Complete( err );
    }

void CHarvesterAO::GetLastObserverId( const RMessage2& aMessage )
	{
	WRITELOG( "CHarvesterAO::GetLastObserverId" );
	
	TUint observerId = iHarvesterEventManager->GetLastClientId();
	
	TPckg<TUint> pckgId( observerId );
	aMessage.Write( 0, pckgId );
	aMessage.Complete( KErrNone );
	}

// ---------------------------------------------------------------------------
// ContextSnapshotStatus
// ---------------------------------------------------------------------------
//
void CHarvesterAO::ContextSnapshotStatus( CHarvesterData* aHD )
    {
    WRITELOG( "CHarvesterAO::ContextSnapshotStatus()" );
    
    HarvestCompleted( aHD->ClientId(), aHD->Uri(), aHD->ErrorCode() );

    const TInt errorCode = aHD->ErrorCode();
    if( errorCode != KErrNone )
    	{
        WRITELOG1( "CHarvesterAO::ContextSnapshotStatus() - error occurred: %d", errorCode );    	
    	}
    else
    	{
        WRITELOG( "CHarvesterAO::ContextSnapshotStatus() - successfully completed" );
        if( aHD->Origin() == MdeConstants::Object::ECamera )
        	{
            aHD->MdeObject().SetPlaceholder( EFalse );
        	TRAPD(mdeError, iMdeObjectHandler->SetMetadataObjectL( *aHD ) );
        	if(mdeError != KErrNone)
            	{
            	WRITELOG( "==============================ERROR===============================" );
                WRITELOG( "CHarvesterAO::HarvestingCompleted() - cannot set metadata object" );
                WRITELOG( "==============================ERROR done =========================" );
            	}
        	}
    	}

    delete aHD;
    }

// ---------------------------------------------------------------------------
// IsConnectedToMde
// ---------------------------------------------------------------------------
//
TBool CHarvesterAO::IsConnectedToMde()
    {
    return iMdESession != NULL;
    }

// ---------------------------------------------------------------------------
// HarvestCompleted
// ---------------------------------------------------------------------------
//
void CHarvesterAO::HarvestCompleted( TUid aClientId, const TDesC& aUri, TInt aErr )
	{
	const TInt KParamUri = 0;
    // check if fast harvested file
    if ( iHarvestFileMessages.Count() > 0 )
        {
        for ( TInt i = iHarvestFileMessages.Count()-1; i >= 0; --i )
            {
            RMessage2& msg = iHarvestFileMessages[i].iMessage;
            if ( aClientId == msg.Identity() )
                {
                WRITELOG1( "CHarvesterAO::HarvestingCompleted() - Completing Fast Harvest request! Error code: %d", aErr );
                if (!msg.IsNull())
                	{
                	msg.Write( KParamUri, aUri );
                	msg.Complete( aErr );
                	}
                else
                	{
                	WRITELOG("CHarvesterAO::HarvestingCompleted() NOT COMPLETING AS msg->iMessage->IsNull returns ETrue");
                	} 
                iHarvestFileMessages.Remove( i );
                }
            }
        }
	}

void CHarvesterAO::BootRomScanL()
	{
	WRITELOG("CHarvesterAO::BootRomScanL()");

    if( !iMdeSessionInitialized )
        {
        return;
        }
	
	RPointerArray<TScanItem> scanItems;
	TCleanupItem cleanupItem( MdsUtils::CleanupPtrArray<TScanItem>, &scanItems );
    CleanupStack::PushL( cleanupItem );

	CHarvesterCenRepUtil::GetScanItemsL( scanItems );

	RPointerArray<HBufC> ignorePaths;
	TCleanupItem cleanupItem2( MdsUtils::CleanupPtrArray<HBufC>, &ignorePaths );
    CleanupStack::PushL( cleanupItem2 );

	CHarvesterCenRepUtil::GetIgnoredScanPathsL( ignorePaths );

	BootScanL( scanItems, ignorePaths, ETrue );

	CleanupStack::PopAndDestroy( &ignorePaths );
	CleanupStack::PopAndDestroy( &scanItems );
	}

void CHarvesterAO::BootPartialRestoreScanL()
	{
	// check if partial restore was done before last boot
	TBool partialRestore = iRestoreWatcher->Register();
		
	if ( !partialRestore )
		{
		return;
		}
	
	if( !iMdeSessionInitialized )
	    {
	    return;
	    }
	
	iMdEHarvesterSession->ChangeCDriveMediaId();

	WRITELOG("CHarvesterAO::BootPartialRestoreScanL() - partial restore");
		
	RPointerArray<TScanItem> scanItems;
	TCleanupItem cleanupItem( MdsUtils::CleanupPtrArray<TScanItem>, &scanItems );
    CleanupStack::PushL( cleanupItem );

	CHarvesterCenRepUtil::GetPartialRestorePathsL( scanItems );

	RPointerArray<HBufC> ignorePaths;
	TCleanupItem cleanupItem2( MdsUtils::CleanupPtrArray<HBufC>, &ignorePaths );
    CleanupStack::PushL( cleanupItem2 );

	CHarvesterCenRepUtil::GetIgnoredPartialRestorePathsL( ignorePaths );

	BootScanL( scanItems, ignorePaths, EFalse );
	
	WRITELOG("CHarvesterAO::BootPartialRestoreScanL() - iRestoreWatcher->UnregisterL()");
	iRestoreWatcher->UnregisterL();

	CleanupStack::PopAndDestroy( &ignorePaths );
	CleanupStack::PopAndDestroy( &scanItems );
	}

TBool CHarvesterAO::IsDescInArray(const TPtrC& aSearch, const RPointerArray<HBufC>& aArray)
	{
	const TInt count = aArray.Count();
	
	for( TInt i = 0; i < count; i++ )
		{
		const TDesC& ignorePath = aArray[i]->Des();
		
		TInt result = MdsUtils::Compare( aSearch, ignorePath );
		
		if( result == 0 )
			{
			return ETrue;
			}
		}

	return EFalse;
	}

void CHarvesterAO::BootScanL( RPointerArray<TScanItem>& aScanItems, 
		const RPointerArray<HBufC>& aIgnorePaths,
        TBool aCheckDrive )
	{
	WRITELOG("CHarvesterAO::BootScanL() - begin");

	TVolumeInfo volumeInfo;
	iFs.Volume( volumeInfo, EDriveC );

	iMdEHarvesterSession->SetFilesToNotPresent( volumeInfo.iUniqueID, ETrue );
	
	_LIT( KDirectorySeparator, "\\" );

#ifdef _DEBUG
	WRITELOG1("CHarvesterAO::BootScanL() - item count: %d", aScanItems.Count() );
#endif
	
	RPointerArray<CHarvesterData> hdArray;
	CleanupClosePushL( hdArray );
	
	while( aScanItems.Count() > 0 )
		{
		HBufC* folder = aScanItems[0]->iPath;
		TUint32 preinstalled = aScanItems[0]->iPreinstalled;

		CDir* directory = NULL;
		TInt error = iFs.GetDir( folder->Des(), KEntryAttDir, KHarvesterGetDirFlags, directory );
		
		if ( error == KErrNone )
			{
			CleanupStack::PushL( directory );

			TInt count = directory->Count();
			
			TUint32 mediaId( 0 );
			
			if( count > 0 )
				{
				TInt drive = 0;
				if( iFs.CharToDrive( (folder->Des())[0], drive ) == KErrNone )
					{
					TVolumeInfo volInfo;
					if( iFs.Volume( volInfo, drive ) == KErrNone )
						{
						mediaId = volInfo.iUniqueID;
						}
					}
				}			
			
			for ( TInt i = 0; i < count; i++ )
				{
				TEntry entry = (*directory)[i];
				
				TInt length = folder->Length() + entry.iName.Length() + KDirectorySeparator().Length();
				HBufC* name = HBufC::NewLC( length );
				name->Des().Append( *folder );
				TPtrC ptr = *folder;
				if( ptr[ ptr.Length() - 1 ] == TChar('\\') )
					{
					name->Des().Append( entry.iName );
					}
			
				if ( entry.IsDir() )
					{					
					name->Des().Append( KDirectorySeparator );
					TPtrC path = *name;
					if( !aCheckDrive )
						{
						path.Set( (*name).Mid( 2 ) );
						}
					if( !IsDescInArray( path, aIgnorePaths ) )
						{
						WRITELOG("CHarvesterAO::BootScanL() - scanFolders.AppendL");
						TScanItem* item = new (ELeave) TScanItem();
						item->iPath = name->AllocL();
						item->iPreinstalled = MdeConstants::MediaObject::ENotPreinstalled;
						aScanItems.AppendL( item );
						}
					}
				else
					{
					TPtrC filename = *name;
					if( !aCheckDrive )
						{
						filename.Set( (*name).Mid( 2 ) );
						}
					if( !IsDescInArray( filename, aIgnorePaths ) )
						{
						WRITELOG("CHarvesterAO::BootScanL() - check files");
						
					    RArray<TPtrC> uris;
					    RArray<TMdSFileInfo> fileInfos;
					    RArray<TFilePresentStates> results;
					    CleanupClosePushL( uris );
					    CleanupClosePushL( fileInfos );
					    CleanupClosePushL( results );
						
					    TMdSFileInfo fileInfo;
					    fileInfo.iModifiedTime = entry.iModified.Int64();
					    fileInfo.iSize = entry.iSize;
					    fileInfos.Append( fileInfo );
					    uris.Append( name->Des() );
						
					    TFilePresentStates found;
					    
					    if( mediaId == volumeInfo.iUniqueID )
					        {
					        iMdEHarvesterSession->SetFilesToPresentL( volumeInfo.iUniqueID, uris, fileInfos, results );
					        found = results[ 0 ];
					        }
					    else
					        {
					        found = EMdsNotFound;
					        }
						
						// scan file if it was not found from DB, or if it has been modified
						if( found == EMdsNotFound ||
						    found == EMdsPlaceholder ||
						    found == EMdsModified )
						    {
	                        CPlaceholderData* phData = CPlaceholderData::NewL();
	                        CleanupStack::PushL( phData );
	                        phData->SetUri( *name );
	                        phData->SetModified( entry.iModified );
	                        phData->SetFileSize( entry.iSize );
	                        phData->SetMediaId( mediaId );
	                        phData->SetPreinstalled( preinstalled );

	                        CHarvesterData* hd = CHarvesterData::NewL( name->AllocL() );
	                        hd->SetEventType( EHarvesterAdd );
	                        hd->SetObjectType( EPlaceholder );
	                        hd->SetOrigin( MdeConstants::Object::EOther );
	                        hd->SetClientData( phData );

	                        CleanupStack::Pop( phData );
	                        hdArray.Append( hd );
						    }
						CleanupStack::PopAndDestroy( &results );
						CleanupStack::PopAndDestroy( &fileInfos );
						CleanupStack::PopAndDestroy( &uris );
						}
					}
				CleanupStack::PopAndDestroy( name );
				}
			
			CleanupStack::PopAndDestroy( directory );
			}
		folder = NULL;
		delete aScanItems[0];
		aScanItems.Remove( 0 );
		}
	
	WRITELOG("CHarvesterAO::BootScanL() - iQueue->Append");
	iQueue->MonitorEvent( hdArray );
	CleanupStack::PopAndDestroy( &hdArray ); 

	iMdEHarvesterSession->RemoveFilesNotPresent( volumeInfo.iUniqueID, ETrue );
	
	WRITELOG("CHarvesterAO::BootScanL() - end");
	}

void CHarvesterAO::SetHarvesterStatusObserver( MHarvesterStatusObserver* aObserver )
	{
	iHarvesterStatusObserver = aObserver;
	}

TBool CHarvesterAO::UnharvestedItemsLeftInPlugins()
	{
	RPointerArray<CHarvesterPluginInfo>& infos = iHarvesterPluginFactory->GetPluginInfos();
	TInt count = infos.Count();
	for ( TInt i = count; --i >= 0; )
		{
		if ( infos[i]->iQueue.Count() > 0 )
			{
			return ETrue;
			}
		}
	
	return EFalse;
	}

void CHarvesterAO::PreallocateNamespaceL( CMdENamespaceDef& aNamespaceDef )
	{
	const TInt objectDefCount = aNamespaceDef.ObjectDefCount();

	for( TInt i = 0; i < objectDefCount; i++ )
		{
		CMdEObjectDef& objectDef = aNamespaceDef.ObjectDefL( i );

		const TInt propertyDefCount = objectDef.PropertyDefCount();

		for( TInt j = 0; j < propertyDefCount; j++ )
			{
			CMdEPropertyDef& propertyDef = objectDef.PropertyDefL( j );
			}
		}
	}

void CHarvesterAO::StartThumbAGDaemon()
    {
    TInt res( KErrNone );
    
    // create server - if one does not already exist
    TFindServer findServer( KTAGDaemonName );
    TFullName name;
    if ( findServer.Next( name ) != KErrNone )
        {
        RProcess server;
        // Create the server process
        // KNullDesC param causes server's E32Main() to be run
        res = server.Create( KTAGDaemonExe, KNullDesC );
        if ( res != KErrNone )
            {
            return;
            }

        // Process created successfully
        TRequestStatus status;
        server.Rendezvous( status );
        
        if ( status != KRequestPending )
            {
            server.Kill( 0 );     // abort startup
            }
        else
            {
            server.Resume();    // logon OK - start the server
            }       

        // Wait until the completion of the server creation
        User::WaitForRequest( status );

        server.Close(); // we're no longer interested in the other process
        }    
    }

void CHarvesterAO::MemoryLow()
	{
	WRITELOG("CHarvesterAO::MemoryLow()");
	// cache monitored events
	PauseMonitoring();
	
	PauseHarvester();
	}

void CHarvesterAO::MemoryGood()
	{
	WRITELOG("CHarvesterAO::MemoryGood()");
	// resume monitoring
	ResumeMonitoring();
	
	TRAP_IGNORE( ResumeHarvesterL() );
	}

