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
* Description:  
*
*/


#ifndef C_CNETWORKINFO_H
#define C_CNETWORKINFO_H

#include <etel3rdparty.h>

/**
*  An observer interface, which is used for getting current network cell id.
*
*  @since S60 3.1
*/
class MNetworkInfoObserver
    {
public:    
    /**
     * This method is used for setting the network cell id to the 
     * location trail.
     */
    virtual void NetworkInfo( const CTelephony::TNetworkInfoV1 &aNetworkInfo, const TInt aError ) = 0;
    };

/**
 *  
 *  @since S60 3.1
 */
class CNetworkInfo : public CActive
    {
public:  
    /**
     * 2-phased constructor.
     * @since S60 3.1
     */
    IMPORT_C static CNetworkInfo* NewL( MNetworkInfoObserver* aTrail );

    /**
     * C++ destructor.
     * @since S60 3.1
     */    
    IMPORT_C virtual ~CNetworkInfo();

protected:
    /**
     * Run error implementation in case of RunL leaving.
     * @since S60 3.1
     */
    TInt RunError( TInt aError );
    
private:
    /**
     * C++ constructor.
     */  
    CNetworkInfo( MNetworkInfoObserver* aTrail );
    
    /**
     * 2nd phase constructor.
     */
    void ConstructL();
    
private:
    /**
    * From CActive.
    */        
    void DoCancel();
    
    /**
    * From CActive.
    */        
    void RunL(); 

private:
    /**
     * An observer interface to set current cell id to the location trail.
     * Not own.
     */
    MNetworkInfoObserver* iTrail;
    
    /**
     * Flag to indicate that we retrieve network info for the first time.
     */ 
    TBool iFirstTime;
    
    /**
     * Interface to phone's telephony system to get Cell Id.
     * Own.
     */
    CTelephony* iTelephony;
    
    CTelephony::TNetworkInfoV1 iNetworkInfoV1;
    CTelephony::TNetworkInfoV1Pckg iNetworkInfoV1Pckg;
    };

#endif // C_CNETWORKINFO_H

// End of file.
