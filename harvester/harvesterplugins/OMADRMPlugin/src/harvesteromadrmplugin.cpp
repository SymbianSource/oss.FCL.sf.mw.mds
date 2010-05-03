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


#include <e32base.h>
#include <caf/caf.h>

#include "mdsutils.h"
#include "harvesteromadrmplugin.h"
#include "harvesterlog.h"
#include "mdeobjectwrapper.h"
#include <harvesterdata.h>

#include <mdenamespacedef.h>
#include <mdeobjectdef.h>
#include <mdeobject.h>
#include <mdetextproperty.h>
#include <mdenamespacedef.h>
#include <mdeconstants.h>

_LIT(KImage, "Image");
_LIT(KVideo, "Video");
_LIT(KAudio, "Audio");
_LIT(KRmMimetype, "realmedia");

_LIT( KSvgMime, "image/svg+xml" );

CHarvesterOmaDrmPluginPropertyDefs::CHarvesterOmaDrmPluginPropertyDefs() : CBase()
	{
	}

void CHarvesterOmaDrmPluginPropertyDefs::ConstructL(CMdEObjectDef& aObjectDef)
	{
	CMdENamespaceDef& nsDef = aObjectDef.NamespaceDef();
	
	// Common property definitions
	CMdEObjectDef& objectDef = nsDef.GetObjectDefL( MdeConstants::Object::KBaseObject );
	iCreationDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KCreationDateProperty );
	iLastModifiedDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KLastModifiedDateProperty );
	iSizePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KSizeProperty );
	iItemTypePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KItemTypeProperty );
	iTitlePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KTitleProperty );

	CMdEObjectDef& mediaDef = nsDef.GetObjectDefL( MdeConstants::MediaObject::KMediaObject );
	iDrmPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KDRMProperty );
	iDescriptionPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KDescriptionProperty );
	iAuthorPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KAuthorProperty );
	iGenrePropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KGenreProperty );
	}

CHarvesterOmaDrmPluginPropertyDefs* CHarvesterOmaDrmPluginPropertyDefs::NewL(CMdEObjectDef& aObjectDef)
	{
	CHarvesterOmaDrmPluginPropertyDefs* self = 
		new (ELeave) CHarvesterOmaDrmPluginPropertyDefs();
	CleanupStack::PushL( self );
	self->ConstructL( aObjectDef );
	CleanupStack::Pop( self );
	return self;
	}

/**
* Default constructor
*/
CHarvesterOMADRMPlugin::CHarvesterOMADRMPlugin() : CHarvesterPlugin()
	{
	WRITELOG("CHarvesterOMADRMPlugin::CHarvesterOMADRMPlugin()");
	}

/**
* Construction
* @return Harvester image plugin
*/
CHarvesterOMADRMPlugin* CHarvesterOMADRMPlugin::NewL()
	{
	WRITELOG("CHarvesterOMADRMPlugin::NewL()");
	CHarvesterOMADRMPlugin* self = new(ELeave) CHarvesterOMADRMPlugin();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	
	return self;
	}

/**
* Destruction
*/
CHarvesterOMADRMPlugin::~CHarvesterOMADRMPlugin()
	{
	WRITELOG("CHarvesterOMADRMPlugin::~CHarvesterOMADRMPlugin()");
	
	delete iPropDefs;
	}

/**
* 2nd phase constructor
*/
void CHarvesterOMADRMPlugin::ConstructL()
	{
	WRITELOG( "CHarvesterOMADRMPlugin::ConstructL()" );
	}

void CHarvesterOMADRMPlugin::HarvestL( CHarvesterData* aHD )
	{
    CMdEObject& mdeObject = aHD->MdeObject();
    CDRMHarvestData* fileData = CDRMHarvestData::NewL();
    CleanupStack::PushL( fileData );

    TRAPD( error, GatherDataL( mdeObject, *fileData ) );
    if ( error == KErrNone || error == KErrCompletion )
    	{
        TBool isNewObject( mdeObject.Id() == 0 );
        
        if ( isNewObject || mdeObject.Placeholder() )
            {
            TRAP( error, HandleObjectPropertiesL( *aHD, *fileData, ETrue ) );
            mdeObject.SetPlaceholder( EFalse );
            }
        else
            {
            TRAP( error, HandleObjectPropertiesL( *aHD, *fileData, EFalse ) );
            }

        if ( error != KErrNone )
            {
            WRITELOG1( "CHarvesterOMADRMPlugin::HarvestL() - Handling object failed: ", error );
            }
    	}
    else	
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::HarvestL() - TRAP error: %d", error );
        TInt convertedError = KErrNone;
        MdsUtils::ConvertTrapError( error, convertedError );
        aHD->SetErrorCode( convertedError );
        }

    CleanupStack::PopAndDestroy( fileData );
	}

// ---------------------------------------------------------------------------
// GatherDataL
// ---------------------------------------------------------------------------
//
void CHarvesterOMADRMPlugin::GatherDataL( CMdEObject& aMetadataObject,
		CDRMHarvestData& aVHD )
    {
    WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL" );
    
    TEntry* entry = new (ELeave) TEntry();
    CleanupStack::PushL( entry );
    
    const TDesC& uri = aMetadataObject.Uri();
    User::LeaveIfError( iFs.Entry( uri, *entry ) );
    
    aVHD.iModified = entry->iModified;
    aVHD.iFileSize = (TUint)entry->iSize;
    CleanupStack::PopAndDestroy( entry );
    
    ContentAccess::CContent* content = ContentAccess::CContent::NewLC( uri );   
    ContentAccess::CData* data = content->OpenContentLC( ContentAccess::EPeek );
    
    ContentAccess::RStringAttributeSet attrSet;
    CleanupClosePushL( attrSet );
    
    attrSet.AddL( ContentAccess::EDescription );
    attrSet.AddL( ContentAccess::EMimeType );
    attrSet.AddL( ContentAccess::ETitle );
    attrSet.AddL( ContentAccess::EAuthor );
    attrSet.AddL( ContentAccess::EGenre );

    User::LeaveIfError( data->GetStringAttributeSet(attrSet) );
    
    TInt err = attrSet.GetValue( ContentAccess::EDescription, aVHD.iDescription );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting description failed %d", err );
        }
        
    if ( aVHD.iDescription.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no description" );
        }
    
    err = attrSet.GetValue( ContentAccess::EMimeType, aVHD.iMimetype );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting mimetype failed %d", err );
        }
        
    if ( aVHD.iMimetype.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no mimetype" );
        }
    
    err = attrSet.GetValue( ContentAccess::ETitle, aVHD.iTitle );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting title failed %d", err );
        }
        
    if ( aVHD.iTitle.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no title" );
        }
    
    err = attrSet.GetValue( ContentAccess::EAuthor, aVHD.iAuthor );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting author failed %d", err );
        }
        
    if ( aVHD.iAuthor.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no author" );
        }

    err = attrSet.GetValue( ContentAccess::EGenre, aVHD.iGenre );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting genre failed %d", err );
        }
        
    if ( aVHD.iGenre.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no genre" );
        }
    
    err = content->GetAttribute( ContentAccess::EIsProtected, aVHD.iDrmProtected );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting protection info failed %d", err );
        }
        
    CleanupStack::PopAndDestroy( 3 ); // content, data, attrSet
    }

// ---------------------------------------------------------------------------
// HandleNewObjectL
// ---------------------------------------------------------------------------
//
void CHarvesterOMADRMPlugin::HandleObjectPropertiesL(
		CHarvesterData& aHD,
		CDRMHarvestData& aVHD,
		TBool aIsAdd )
    {
    WRITELOG("CHarvesterOMADRMPlugin - HandleNewObject ");
    CMdEObject& mdeObject = aHD.MdeObject();

    if( !iPropDefs )
    	{
    	CMdEObjectDef& objectDef = mdeObject.Def();
    	iPropDefs = CHarvesterOmaDrmPluginPropertyDefs::NewL( objectDef );
    	// Prefetch max text lengt for validity checking
    	iMaxTextLength = iPropDefs->iGenrePropertyDef->MaxTextLengthL();
    	}
    
    TTimeIntervalSeconds timeOffset = User::UTCOffset();
    
    if( ! mdeObject.Placeholder() )
    	{
    	// Creation date
    	TTime localTime = aVHD.iModified + timeOffset;
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iCreationDatePropertyDef, &localTime, aIsAdd );
    	// Last modified date
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iLastModifiedDatePropertyDef, &aVHD.iModified, aIsAdd );
    	// File size
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iSizePropertyDef, &aVHD.iFileSize, aIsAdd );
    	}
    
    // Item Type
    if(aVHD.iMimetype.Length() > 0)
        {
        TBool isAdd( EFalse );
        CMdEProperty* prop = NULL;
        TInt index = mdeObject.Property( *iPropDefs->iItemTypePropertyDef, prop );
        if( index < 0 )
            {
            isAdd = ETrue;
            }
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iItemTypePropertyDef, &aVHD.iMimetype, isAdd );
        }
    
    // DRM protection
    CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    		*iPropDefs->iDrmPropertyDef, &aVHD.iDrmProtected, aIsAdd );
    
    // Title (is set from URI by default)
    if( aVHD.iTitle.Length() > 0 && aVHD.iTitle.Length() < iMaxTextLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iTitlePropertyDef, &aVHD.iTitle, EFalse );
    	}
    // Description
    if( aVHD.iDescription.Length() > 0 && aVHD.iDescription.Length() < iMaxTextLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iDescriptionPropertyDef, &aVHD.iDescription, aIsAdd );
    	}   
    // Author
    if( aVHD.iAuthor.Length() > 0 && aVHD.iAuthor.Length() < iMaxTextLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iAuthorPropertyDef, &aVHD.iAuthor, aIsAdd );
    	}
    // Genre
    if( aVHD.iGenre.Length() > 0 && aVHD.iGenre.Length() < iMaxTextLength )
        {
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iGenrePropertyDef, &aVHD.iGenre, aIsAdd );
        }
    }

// ---------------------------------------------------------------------------
// ChangeObjectType
// ---------------------------------------------------------------------------
//
void CHarvesterOMADRMPlugin::GetObjectType( const TDesC& aUri, TDes& aObjectType )
	{
	ContentAccess::CContent* content = NULL;
    TBuf16<KMaxDataTypeLength> mime;
    
	TRAPD( err, content = ContentAccess::CContent::NewL( aUri ) );
	if (err == KErrNone) 
		{
		err = content->GetStringAttribute( ContentAccess::EMimeType, mime );
		delete content;
		}
    
	if( mime == KSvgMime )
	    {
	    WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - ERROR: mimetype %S. Not supported", &mime );
	     aObjectType.Zero();
	     return;
	    }
	
    if( err == KErrNone )
    	{
	    TPtrC ptrImage( KImage );
		if( MdsUtils::Find( mime, ptrImage ) != KErrNotFound )
			{
			WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - mimetype %S. Object type changed to Image", &mime );
			aObjectType.Copy( KImage );
			return;
			}
		
		TPtrC ptrVideo( KVideo );
		if( MdsUtils::Find( mime, ptrVideo ) != KErrNotFound )
			{
			WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - mimetype %S. Object type changed to Video", &mime );
			aObjectType.Copy( KVideo );
			return;
			}
		
        TPtrC ptrAudio( KAudio );
        if( MdsUtils::Find( mime, ptrAudio ) != KErrNotFound )
            {
            WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - mimetype %S. Object type changed to Audio", &mime );
            aObjectType.Copy( KAudio );
            return;
            }		
		
		TPtrC ptrRm( KRmMimetype );
		if( MdsUtils::Find( mime, ptrRm ) != KErrNotFound )
			{
			WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - mimetype %S. Object type changed to Rm", &mime );
			aObjectType.Copy( KVideo );
			return;
			}
    	}
    
	WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - ERROR: mimetype %S. No object type found", &mime );
	aObjectType.Zero();
	}

// ---------------------------------------------------------------------------
// CHarvesterOMADRMPlugin::GetMimeType (from CHarvesterPlugin)
// ---------------------------------------------------------------------------
//    
void CHarvesterOMADRMPlugin::GetMimeType( const TDesC& aUri, TDes& aMimeType )
    {
    aMimeType.Zero();
    
    ContentAccess::CContent* content = NULL;
    
    TRAPD( err, content = ContentAccess::CContent::NewL( aUri ) );
    if (err == KErrNone) 
        {
        err = content->GetStringAttribute( ContentAccess::EMimeType, aMimeType );
        delete content;
        }
    }

