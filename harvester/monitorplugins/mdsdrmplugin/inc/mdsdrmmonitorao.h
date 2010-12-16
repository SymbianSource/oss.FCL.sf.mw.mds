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
*/

#ifndef __MDSDRMMONITORAO_H__
#define __MDSDRMMONITORAO_H__

#include <e32base.h>
#include <mdequery.h>
#include "mdeobjectdef.h"
#include "mdepropertydef.h"
#include "mdeobject.h"
#include "mdesession.h"
#include "mdsdrmplugin.h"

class CMdsDrmMonitorAO : public CActive,
                         public MMdEQueryObserver
    {
    public:
    
    enum TRequest
        {
        ERequestIdle,
        ERequestQuery
        };
    
        /**
        * Creates and constructs a new instance of CMdsDrmMonitorAO.
        *
        * @return A pointer to the new instance of CMdsDrmMonitorAO
        */
        static CMdsDrmMonitorAO* NewL();
        
        /**
        * Destructor
        */
        virtual ~CMdsDrmMonitorAO();
        
        // From MMdEQueryObserver
        
        void HandleQueryNewResults( CMdEQuery& aQuery,
                                    TInt aFirstNewItemIndex,
                                    TInt aNewItemCount);
        
        void HandleQueryCompleted(CMdEQuery& aQuery, TInt aError);
        
        // New functions
        
        /**
         * Set next request (=state) of this active object.
         * @param aRequest  State enumeration.
         */       
        void SetNextRequest( TRequest aRequest );
        
        void Setup( MMonitorPluginObserver& aObserver,
                    CMdESession* aSession );
        
        void AddToQueue( HBufC8* aContentID );
        
        void DoQueryL();

    protected:
        
        /**
         * RunL.
         * From CActive.
         */
        void RunL();

        /**
         * DoCancel.
         * From CActive.
         */
        void DoCancel();

        /**
         * RunError for handling leaves occuring in RunL.
         * From CActive.
         */
        TInt RunError( TInt aError );
        
    private:
        /**
        * C++ constructor - not exported;
        * implicitly called from NewL()
        */
        CMdsDrmMonitorAO();
        
        /**
        * 2nd phase construction, called by NewL()
        */
        void ConstructL();

    private:
        
        // Not owned
        MMonitorPluginObserver* iObserver;
        // Not owned
        CMdESession* iSession;
        
        RPointerArray<HBufC8> iEventArray;
        
        /**
         * Indicator to show which task will be next to do
         */
        TRequest iNextRequest;
        
        CMdEObjectQuery* iObjectQuery;
        
        CMdEPropertyDef* iOriginPropertyDef;
        CMdEPropertyDef* iContentIDPropertyDef;
        
        RPointerArray<CHarvesterData> iHdArray;

    };

#endif // __MDSDRMMONITORAO_H__
