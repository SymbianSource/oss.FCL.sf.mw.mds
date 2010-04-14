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
* Description:  Harvests meta data from oma drm file.*
*/


#ifndef __CHARVESTEROMADRMPLUGIN_H__
#define __CHARVESTEROMADRMPLUGIN_H__

#include <e32base.h>
#include <apmstd.h>
#include "harvesterplugin.h"

// FORWARD DECLARATION
class CMdEObjectDef;
class CMdEObject;

/**
* A data transfer class for harvested drm metadata.
*/
class CDRMHarvestData : public CBase
    {
    public:

        /** NewL */
        static CDRMHarvestData* NewL()
            {
            CDRMHarvestData* self = new (ELeave) CDRMHarvestData;
            return self;
            }
        
        /** Destructor */
        virtual ~CDRMHarvestData()
            {
            }

    private:
        /** Constructor */
    		CDRMHarvestData() :  iFileSize( 0 ), iDrmProtected( EFalse )
            {
            // no implementation required
            }

    public:

    	TBuf<KMaxDataTypeLength> iMimetype;
        TBuf<KMaxDataTypeLength> iDescription;
        TBuf<KMaxDataTypeLength> iTitle;
        TBuf<KMaxDataTypeLength> iAuthor;
        TBuf<KMaxDataTypeLength> iGenre;
        TInt64 iFileSize;
        TTime iModified;
        TBool iDrmProtected;
    };

/**
 * Helper class to hold all property definitions 
 * (pointers are not owned) used in harvester OMA DRM plug-in.
 */
class CHarvesterOmaDrmPluginPropertyDefs : public CBase
	{
	public:
		// Common property definitions
		CMdEPropertyDef* iCreationDatePropertyDef;
		CMdEPropertyDef* iLastModifiedDatePropertyDef;
		CMdEPropertyDef* iSizePropertyDef;
		CMdEPropertyDef* iItemTypePropertyDef;
		CMdEPropertyDef* iTitlePropertyDef;
	
		// Media property definitions
		CMdEPropertyDef* iDrmPropertyDef;
		CMdEPropertyDef* iDescriptionPropertyDef;
		CMdEPropertyDef* iAuthorPropertyDef;
		CMdEPropertyDef* iGenrePropertyDef;
	
	private:
		CHarvesterOmaDrmPluginPropertyDefs();
	
		void ConstructL(CMdEObjectDef& aObjectDef);

	public:	
		static CHarvesterOmaDrmPluginPropertyDefs* NewL(CMdEObjectDef& aObjectDef);
	};

class CHarvesterOMADRMPlugin : public CHarvesterPlugin
  	{
	public:
		/**
		* Constructs a new CHarvesterOMADRMPlugin implementation.
		*
		* @return A pointer to the new CHarvesterOMADRMPlugin implementation
		*/
		static CHarvesterOMADRMPlugin* NewL();
		
		/**
		* Destructor
		*/
		virtual ~CHarvesterOMADRMPlugin();
		
		/**
		* Harvests several files. Inherited from CHarvestPlugin.
		*
		* @param aHarvesterData  CHarvesterData datatype containing needed harvest data
 		* @param aClientData  TAny* to client specific data
		*/
		void HarvestL( CHarvesterData* aHD );
		
		/** */
		void GetObjectType( const TDesC& aUri, TDes& aObjectType );
		
    protected: // from CHarvesterPlugin
        
        void GetMimeType( const TDesC& aUri, TDes& aMimeType );
	
	private:
		/**
		* C++ constructor - not exported;
		* implicitly called from NewL()
		*
		* @return an instance of CHarvesterOMADRMPlugin.
		*/
		CHarvesterOMADRMPlugin();
		
		/**
		* 2nd phase construction, called by NewLC()
		*/
		void ConstructL();
		
		/**
        * Gathers data from file to meta data object.
        *
        * @param aMetadataObject  A reference to meta data object to gather the data.
        * @param aHarvestData  An object to store harvested video file data.
        */
        void GatherDataL( CMdEObject& aMetadataObject, CDRMHarvestData& aHarvestData );
		
        /**
         * Handle addition of new mde video objects.
         *
         * @param aMetadataObject  A reference to meta data object to gather the data.
         * @param aHarvestData  An object containing harvested video file data.
         */
        void HandleObjectPropertiesL( CHarvesterData& aHD, CDRMHarvestData& aVHD, TBool aIsAdd );

	private:
		CHarvesterOmaDrmPluginPropertyDefs* iPropDefs;
		
		TInt iMaxTextLength;
	};

#endif // __CHarvesterOMADRMPlugin_H__
