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

#include "cnetworkinfo.h"
#include "locationmanagerdebug.h"

// --------------------------------------------------------------------------
// CNetworkInfo::NewL
// --------------------------------------------------------------------------
//
EXPORT_C CNetworkInfo* CNetworkInfo::NewL( MNetworkInfoObserver* aTrail )
    {
    LOG( "CNetworkInfo::NewL(), begin" );
    CNetworkInfo* self = new (ELeave) CNetworkInfo( aTrail );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    LOG( "CNetworkInfo::NewL(), end" );
    return self;
    }
        
// --------------------------------------------------------------------------
// CNetworkInfo::CNetworkInfo
// --------------------------------------------------------------------------
//  
CNetworkInfo::CNetworkInfo( MNetworkInfoObserver* aTrail ) 
    : CActive( CActive::EPriorityStandard ),
    iFirstTime( EFalse ),
    iTelephony( NULL ),
    iNetworkInfoV1Pckg( iNetworkInfoV1 ) 
    {
    CActiveScheduler::Add( this );
    iTrail = aTrail;
    }

// --------------------------------------------------------------------------
// CNetworkInfo::ConstructL
// --------------------------------------------------------------------------
//    
void CNetworkInfo::ConstructL()
    {
	LOG( "CNetworkInfo::ConstructL(), begin" );
	iFirstTime = ETrue;
    iTelephony = CTelephony::NewL();
    iTelephony->GetCurrentNetworkInfo(iStatus, iNetworkInfoV1Pckg);
    LOG( "CNetworkInfo::ConstructL(), iTelephony->GetCurrentNetworkInfo called" );
    
   	if ( IsActive() )
    	{
    	Cancel();
        }    
    SetActive();
    
    LOG( "CNetworkInfo::ConstructL(), end" );
    }
    
// --------------------------------------------------------------------------
// CNetworkInfo::~CNetworkInfo
// --------------------------------------------------------------------------
//    
EXPORT_C CNetworkInfo::~CNetworkInfo()
    {
    Cancel();
    delete iTelephony;
    }

// --------------------------------------------------------------------------
// CNetworkInfo::RunError
// --------------------------------------------------------------------------
//
TInt CNetworkInfo::RunError( TInt /*aError*/ )
    {
    return KErrNone;
    }    

// --------------------------------------------------------------------------
// CNetworkInfo::RunL
// --------------------------------------------------------------------------
//    
void CNetworkInfo::RunL()
    { 
	LOG( "CNetworkInfo::RunL(), begin" );   
	iFirstTime = EFalse; 
    iTrail->NetworkInfo( iNetworkInfoV1, iStatus.Int() );
    LOG( "CNetworkInfo::RunL(), iTrail->NetworkInfo called" );   
    
    iTelephony->NotifyChange(iStatus, CTelephony::ECurrentNetworkInfoChange, iNetworkInfoV1Pckg);
    LOG( "CNetworkInfo::RunL(), iTelephony->NotifyChange called" );
      
   	if ( IsActive() )
    	{
    	Cancel();
        }    
    SetActive();
    
    LOG( "CNetworkInfo::RunL(), end" ); 
    }    

// --------------------------------------------------------------------------
// CNetworkInfo::DoCancel
// --------------------------------------------------------------------------
// 
void CNetworkInfo::DoCancel()
    {
	LOG( "CNetworkInfo::DoCancel(), begin" );
	if ( IsActive() )
		{
    	if ( iFirstTime )    
        	{
	    	LOG( "CNetworkInfo::DoCancel(), cancelling CTelephony::EGetCurrentNetworkInfoCancel" );
        	iTelephony->CancelAsync( CTelephony::EGetCurrentNetworkInfoCancel );
        	}
    	else
    		{
	    	LOG( "CNetworkInfo::DoCancel(), cancelling CTelephony::ECurrentNetworkInfoChangeCancel" );
	    	iTelephony->CancelAsync( CTelephony::ECurrentNetworkInfoChangeCancel );	
    		}
		}	
   	LOG( "CNetworkInfo::DoCancel(), end" );
    }

// End of file
