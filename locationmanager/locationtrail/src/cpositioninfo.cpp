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
* Description:  A class for getting network cell id.
*
*/

#include <etel3rdparty.h>

#include "rlocationtrail.h"
#include "cpositioninfo.h"
#include "locationtraildefs.h"
#include "locationmanagerdebug.h"

// --------------------------------------------------------------------------
// CPositionInfo::NewL
// --------------------------------------------------------------------------
//
EXPORT_C CPositionInfo* CPositionInfo::NewL( MPositionInfoObserver* aTrail )
    {
    LOG( "CPositionInfo::NewL(), begin" );
    CPositionInfo* self = new (ELeave) CPositionInfo( aTrail );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    LOG( "CPositionInfo::NewL(), end" );
    return self;
    }
        
// --------------------------------------------------------------------------
// CPositionInfo::CPositionInfo
// --------------------------------------------------------------------------
//  
CPositionInfo::CPositionInfo( MPositionInfoObserver* aTrail ) 
    : CActive( CActive::EPriorityStandard ),
    iFirstInterval( ETrue )
    {
    LOG( "CPositionInfo::CPositionInfo()");
    CActiveScheduler::Add( this );
    iTrail = aTrail;
    iTrailCaptureSetting = RLocationTrail::ECaptureAll;
    
    // Set update interval.
    iUpdateOptions.SetUpdateInterval( TTimeIntervalMicroSeconds(KFirstInterval) );
    // Set time out level. 
    iUpdateOptions.SetUpdateTimeOut( TTimeIntervalMicroSeconds(KFirstTimeOut) );
    // Positions which have time stamp below KMaxAge can be reused
    iUpdateOptions.SetMaxUpdateAge( TTimeIntervalMicroSeconds(KMaxAge) );
    // Disables location framework to send partial position data
    iUpdateOptions.SetAcceptPartialUpdates( EFalse );    
    }

// --------------------------------------------------------------------------
// CPositionInfo::ConstructL
// --------------------------------------------------------------------------
//    
void CPositionInfo::ConstructL()
    {
    
    }
    
// --------------------------------------------------------------------------
// CPositionInfo::~CPositionInfo
// --------------------------------------------------------------------------
//    
EXPORT_C CPositionInfo::~CPositionInfo()
    {
    Cancel();
    iPositioner.Close();
    iPosServer.Close();
    }

// --------------------------------------------------------------------------
// CPositionInfo::RunError
// --------------------------------------------------------------------------
//
TInt CPositionInfo::RunError( TInt /*aError*/ )
    {
    return KErrNone;
    }

// --------------------------------------------------------------------------
// CPositionInfo::GetCellId
// --------------------------------------------------------------------------
//
void CPositionInfo::StartL( RLocationTrail::TTrailCaptureSetting aCaptureSetting, TInt aUpdateInterval )
    {
    LOG( "CPositionInfo::StartL(), begin" );

    iTrailCaptureSetting = aCaptureSetting;
    iUpdateInterval = aUpdateInterval;
    iFirstInterval = ETrue;
    iPositionInfo = TPositionSatelliteInfo();
    
    // Set update interval.
     iUpdateOptions.SetUpdateInterval( TTimeIntervalMicroSeconds(KFirstInterval) );
     // Set time out level. 
     iUpdateOptions.SetUpdateTimeOut( TTimeIntervalMicroSeconds( KFirstTimeOut) );
     // Positions which have time stamp below KMaxAge can be reused
     iUpdateOptions.SetMaxUpdateAge( TTimeIntervalMicroSeconds(KMaxAge) );
     // Disables location framework to send partial position data
     iUpdateOptions.SetAcceptPartialUpdates( EFalse );
    
    if ( aCaptureSetting == RLocationTrail::ECaptureAll ) 
    	{
	    User::LeaveIfError( iPosServer.Connect() );
	    User::LeaveIfError( iPositioner.Open( iPosServer ) );
	    User::LeaveIfError( iPositioner.SetRequestor( CRequestor::ERequestorService,
	                        CRequestor::EFormatApplication, KRequestor ) );
	    User::LeaveIfError( iPositioner.SetUpdateOptions( iUpdateOptions ) );
	    iPositioner.NotifyPositionUpdate( iPositionInfo, iStatus );
    	}
    
    SetActive();
    
    if ( aCaptureSetting == RLocationTrail::ECaptureNetworkInfo ) 
    	{
    	TRequestStatus* status = &iStatus;
        User::RequestComplete( status, KErrNone );
    	}

    LOG( "CPositionInfo::StartL(), end" );
    }

// --------------------------------------------------------------------------
// CPositionInfo::NextPosition
// --------------------------------------------------------------------------
//
void CPositionInfo::NextPosition()
    {
    iPositionInfo = TPositionSatelliteInfo(); // Clear position info.
    if ( iTrailCaptureSetting == RLocationTrail::ECaptureAll )
    	{
    	iPositioner.NotifyPositionUpdate( iPositionInfo, iStatus );
    	}
    
    SetActive();
    
    if ( iTrailCaptureSetting == RLocationTrail::ECaptureNetworkInfo ) 
    	{
    	TRequestStatus* status = &iStatus;
        User::RequestComplete( status, KErrNone );
    	}
    }
    
// --------------------------------------------------------------------------
// CPositionInfo::Stop
// --------------------------------------------------------------------------
//
void CPositionInfo::Stop()
    {
    Cancel();    

    iPositioner.Close();
    iPosServer.Close();
    }    
        
// --------------------------------------------------------------------------
// CPositionInfo::RunL
// --------------------------------------------------------------------------
//
void CPositionInfo::RunL()
    { 
    iTrail->Position( iPositionInfo, iStatus.Int() );
 
    if ( iFirstInterval && IsActive() )
    	{
    	Cancel();
    	LOG("CPositionInfo::RunL() - First Time");
    	iUpdateOptions.SetUpdateInterval( TTimeIntervalMicroSeconds (iUpdateInterval) );  
    	iUpdateOptions.SetUpdateTimeOut( TTimeIntervalMicroSeconds(KUpdateTimeOut ) );
        if ( iTrailCaptureSetting == RLocationTrail::ECaptureAll ) 
        	{
        	User::LeaveIfError( iPositioner.SetUpdateOptions( iUpdateOptions ) );        	
        	iPositioner.NotifyPositionUpdate( iPositionInfo, iStatus );
        	}
    	SetActive();
    	iFirstInterval = EFalse;
    	}
    }    

// --------------------------------------------------------------------------
// CPositionInfo::DoCancel
// --------------------------------------------------------------------------
// 
void CPositionInfo::DoCancel()
    {
    LOG( "CPositionInfo::DoCancel()" );
    if ( IsActive() )    
        {
        iPositioner.CancelRequest( EPositionerNotifyPositionUpdate );
        }
    }

// End of file
