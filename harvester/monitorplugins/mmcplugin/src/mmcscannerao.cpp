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
* Description:  Scans MMC after phone reboot for file changes*
*/

#include "mmcscannerao.h"
#include "harvesterlog.h"
#include "fsutil.h"
#include <harvesterdata.h>
#include <placeholderdata.h>
#include <centralrepository.h>

_LIT( KColon, ":" );

const TInt KEntryBufferSize = 100;
const TInt KDefaultDelay = 4;
const TInt KMillion = 1000000;

const TUid KRepositoryUid = { 0x20007183 };
const TUint32 KScanDelayKey = 0x00000001;

CMmcScannerAO::CMmcScannerAO( TUint32 aMediaId, 
		CMdEHarvesterSession* aMdEClient, MMonitorPluginObserver* aObserver, 
		CHarvesterPluginFactory* aHarvesterPluginFactory, CActive::TPriority aPriority ) : 
		CTimer( aPriority ), iState( EUninitialized ), iMmcFileList( NULL )   
	{
	iMediaId = aMediaId;
	iMdEClient = aMdEClient;
	iObserver = aObserver;
	iHarvesterPluginFactory = aHarvesterPluginFactory;
	}

CMmcScannerAO* CMmcScannerAO::NewL( TUint32 aMediaId, CMdEHarvesterSession* aMdEClient,
		MMonitorPluginObserver* aObserver, CHarvesterPluginFactory* aHarvesterPluginFactory, 
		CActive::TPriority aPriority, TBool aAlreadyWaited )
	{
	CMmcScannerAO* self = new ( ELeave ) CMmcScannerAO( aMediaId, aMdEClient, aObserver, 
			aHarvesterPluginFactory, aPriority );
	
	CleanupStack::PushL( self );
	self->ConstructL( aAlreadyWaited );
	CleanupStack::Pop( self );
	return self;
	}

void CMmcScannerAO::ConstructL( TBool aAlreadyWaited )
	{
	CTimer::ConstructL();
	CActiveScheduler::Add( this ); // Add to scheduler
	iState = EUninitialized;
	User::LeaveIfError( iFs.Connect() );
	iMmcFileList = CMmcFileList::NewL();
	
	if( !aAlreadyWaited )
	    {
        TInt tmpDelay( KDefaultDelay );
        TTimeIntervalMicroSeconds32 delay( tmpDelay * KMillion ); 
        CRepository* repo = CRepository::NewLC( KRepositoryUid );
        TInt err = repo->Get( KScanDelayKey, tmpDelay );
        if ( err == KErrNone )
            {
            delay = tmpDelay * KMillion;
            }
        CleanupStack::PopAndDestroy( repo );
        After( delay );
	    }
	else
	    {
	    TTimeIntervalMicroSeconds32 delay( 5 ); 
	    After( delay );
	    }
	}

CMmcScannerAO::~CMmcScannerAO()
	{
	Cancel(); // Cancel any request, if outstanding
	// Delete instance variables if any
	
	delete iMmcFileList;
	
	iEntryArray.ResetAndDestroy();
	iEntryArray.Close();

	iHarvestEntryArray.ResetAndDestroy();
	iHarvestEntryArray.Close();
	
	iFs.Close();
	}


void CMmcScannerAO::RunL()
	{
	switch( iState )
		{
		case( EUninitialized ):
			{
			WRITELOG("CMmcScannerAO::RunL - Setting files to not present");
			iMdEClient->SetFilesToNotPresent( iMediaId, ETrue );
			SetState( EReadFiles );
			break;
			}
		
		case( EReadFiles ):
			{
			for ( TInt i=0; i < KMaxDrives; i++ )
				{
				const TUint32 mediaId = FSUtil::MediaID(iFs, i);
				if( mediaId == iMediaId )
					{
					TChar chr;
					iFs.DriveToChar( i, chr );
					i = KMaxDrives;
					iDrive.Zero();
					iDrive.Append( chr );
					iDrive.Append( KColon );
					}
				}
			// drive not found (unmount before scanning delay)
			if ( iDrive.Length() == 0 )
				{
				SetState( EDone );
				break;
				}
			
			WRITELOG("CMmcScannerAO::RunL - build file list");
			iMmcFileList->BuildFileListL( iFs, iDrive, iEntryArray );
			SetState( EProcessFiles );
			break;
			}
		
		case( EProcessFiles ):
			{
			if( iEntryArray.Count() > 0 )
				{
				WRITELOG("CMmcScannerAO::RunL - handling file list");
				iMmcFileList->HandleFileEntryL( *iMdEClient, iEntryArray, 
						iHarvestEntryArray, iMediaId, iHarvesterPluginFactory );
				SetState( EHarvestFiles );
				}
			else 
				{
				SetState( ERemoveNPFiles );
				}
			break;
			}
		
		case( EHarvestFiles ):
			{
			if ( iHarvestEntryArray.Count() > 0 )
				{
				WRITELOG("CMmcScannerAO::RunL - adding new files to harvester queue");
				HandleReharvestL();
				SetState( EHarvestFiles );
				}
			else
				{
				SetState( EProcessFiles );
				}
			break;
			}
		
		case( ERemoveNPFiles ):
			{
			WRITELOG("CMmcScannerAO::RunL - Removing not present files");
			iMdEClient->RemoveFilesNotPresent( iMediaId, ETrue );
			SetState( EDone );
			break;
			}
		
		case( EDone ):
			{
			iFs.Close();
			break;
			}
		
		default: 
			break;
		
		}
	}

void CMmcScannerAO::HandleReharvestL()
	{
	WRITELOG("CMMCMountTaskAO::HandleReharvestL");
		
	TInt batchSize( 0 );
	RPointerArray<CHarvesterData> hdArray;
	CleanupClosePushL( hdArray );
	
    if ( iHarvestEntryArray.Count() >= KEntryBufferSize )
        {
        batchSize = KEntryBufferSize;
        }
    else
        {
        batchSize = iHarvestEntryArray.Count();
        }
        
    for ( TInt i = 0; i < batchSize; i++ )
        {
        CPlaceholderData* ei = iHarvestEntryArray[0];

        HBufC* fileName = ei->Uri().AllocLC();
        CHarvesterData* hd = CHarvesterData::NewL( fileName );
		hd->SetOrigin( MdeConstants::Object::EOther );
		CleanupStack::Pop( fileName );

		if ( ei->PresentState() == EMdsPlaceholder || 
			 ei->PresentState() == EMdsModified )
			{
			hd->SetEventType( EHarvesterEdit );
			hd->SetObjectType( ENormal );
			delete ei;
			}
		else
			{
			hd->SetEventType( EHarvesterAdd );
			hd->SetObjectType( EPlaceholder );
			hd->SetClientData( ei );
			}
		hdArray.Append( hd );
        iHarvestEntryArray.Remove( 0 );
        }

    if( iHarvestEntryArray.Count() == 0 )
    	{
    	iHarvestEntryArray.Compress();
    	}
    
	if ( iObserver )
		{
		if( hdArray.Count() > 0)
			{
			iObserver->MonitorEvent( hdArray );
			}
		else
			{
			iObserver->MonitorEvent( hdArray[0] );
			}
		}
	
	CleanupStack::PopAndDestroy( &hdArray ); 
	}
	

TInt CMmcScannerAO::RunError( TInt /*aError*/ )
	{
	return KErrNone;
	}

void CMmcScannerAO::SetState( TCMmcScannerAOState aState )
	{
	WRITELOG("CMmcScannerAO::SetNextRequest" );
	if ( !IsActive() )
		{
		iState = aState;
		TRequestStatus* ptrStatus = &iStatus;
		User::RequestComplete( ptrStatus, KErrNone );
		SetActive();
		}
	}
