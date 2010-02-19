/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Harvests metadata from wmv video file 
*
*/

#include <e32std.h>
#include <caf/content.h>

#include "mdsutils.h"
#include "harvesterdata.h"
#include "harvesterlog.h"
#include "harvesterwmvplugin.h"
#include <mdenamespacedef.h>
#include <mdeobjectdef.h>
#include "mdeobject.h"
#include "mdetextproperty.h"
#include "mdeobjectwrapper.h"

CHarvesterWmvPluginPropertyDefs::CHarvesterWmvPluginPropertyDefs() : CBase()
	{
	}

void CHarvesterWmvPluginPropertyDefs::ConstructL(CMdEObjectDef& aObjectDef)
	{
	CMdENamespaceDef& nsDef = aObjectDef.NamespaceDef();
	
	// Common property definitions
	CMdEObjectDef& objectDef = nsDef.GetObjectDefL( MdeConstants::Object::KBaseObject );
	iCreationDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KCreationDateProperty );
	iLastModifiedDatePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KLastModifiedDateProperty );
	iSizePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KSizeProperty );
	iItemTypePropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KItemTypeProperty );
	}

CHarvesterWmvPluginPropertyDefs* CHarvesterWmvPluginPropertyDefs::NewL(CMdEObjectDef& aObjectDef)
	{
	CHarvesterWmvPluginPropertyDefs* self = 
		new (ELeave) CHarvesterWmvPluginPropertyDefs();
	CleanupStack::PushL( self );
	self->ConstructL( aObjectDef );
	CleanupStack::Pop( self );
	return self;
	}

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
//
CHarvesterWMVPlugin* CHarvesterWMVPlugin::NewL()
    {
    CHarvesterWMVPlugin* self = new(ELeave) CHarvesterWMVPlugin();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);

    return self;
    }

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
//
CHarvesterWMVPlugin::~CHarvesterWMVPlugin()
    {
    WRITELOG( "CHarvesterWMVPlugin::~CHarvesterWMVPlugin()" );
    }

// ---------------------------------------------------------------------------
// Harvest file
// ---------------------------------------------------------------------------
//
void CHarvesterWMVPlugin::HarvestL( CHarvesterData* aHD )    
    {
    WRITELOG( "CHarvesterWMVPlugin::Harvest()" );
    CMdEObject& mdeObject = aHD->MdeObject();
    
    CHarvesterWmvClipDetails* clipDetails = CHarvesterWmvClipDetails::NewL();
    CleanupStack::PushL( clipDetails );
    
    TRAPD( error, GatherDataL( mdeObject, *clipDetails ) );
    if ( error == KErrNone || error == KErrCompletion )
    	{
    	TBool isNewObject( mdeObject.Id() == 0 );
        
        if ( isNewObject || mdeObject.Placeholder() )
            {
            TRAP( error, HandleObjectPropertiesL( *aHD, *clipDetails, ETrue ) );
            mdeObject.SetPlaceholder( EFalse );
            }
        else
            {
            TRAP( error, HandleObjectPropertiesL( *aHD, *clipDetails, EFalse ) );
            }

        if ( error != KErrNone )
            {
            WRITELOG1( "CHarvesterWMVPlugin::HarvestL() - Handling object failed: ", error );
            }
    	}
    else
        {
        TInt convertedError = KErrNone;
        MdsUtils::ConvertTrapError( error, convertedError );
        aHD->SetErrorCode( convertedError );
        }
    
    CleanupStack::PopAndDestroy( clipDetails );
    }

// ---------------------------------------------------------------------------
// CHarvesterWMVPlugin::GetMimeType (from CHarvesterPlugin)
// ---------------------------------------------------------------------------
//    
void CHarvesterWMVPlugin::GetMimeType( const TDesC& aUri, TDes& aMimeType )
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

// ---------------------------------------------------------------------------
// Default constructor
// ---------------------------------------------------------------------------
//
CHarvesterWMVPlugin::CHarvesterWMVPlugin() : CHarvesterPlugin(), iPropDefs( NULL )
    {
    }

// ---------------------------------------------------------------------------
// 2nd phase constructor
// ---------------------------------------------------------------------------
//
void CHarvesterWMVPlugin::ConstructL()
    {
    WRITELOG( "CHarvesterWMVPlugin::ConstructL()" );
    }

// ---------------------------------------------------------------------------
// GatherDataL
// ---------------------------------------------------------------------------
//
void CHarvesterWMVPlugin::GatherDataL( CMdEObject& aMetadataObject, CHarvesterWmvClipDetails& aClipDetails )
    {
    WRITELOG( "CHarvesterWMVPlugin::GatherDataL()" );
       
    const TDesC& uri = aMetadataObject.Uri();

    TInt error ( KErrNone );
    TEntry* entry = new (ELeave) TEntry();
    CleanupStack::PushL( entry );

    User::LeaveIfError( iFs.Entry( uri, *entry ) );

    aClipDetails.iModifiedDate = entry->iModified;
    aClipDetails.iFileSize = (TUint)entry->iSize;
    
    CleanupStack::PopAndDestroy( entry );
    
    ContentAccess::CContent* content = ContentAccess::CContent::NewLC( uri );

    //Mime type check
    error = content->GetStringAttribute( ContentAccess::EMimeType, aClipDetails.iMimeType );
    if (  error != KErrNone )
        {
        WRITELOG( "CHarvesterWMVPlugin - Could not resolve mime type, leave!" );
        User::Leave( KErrNotSupported );
        }

    CleanupStack::PopAndDestroy( content );  
    }

// ---------------------------------------------------------------------------
// Handle object properties
// ---------------------------------------------------------------------------
//
void CHarvesterWMVPlugin::HandleObjectPropertiesL( 
    CHarvesterData& aHD,
    CHarvesterWmvClipDetails& aClipDetails,
    TBool aIsAdd )
    {
    WRITELOG( "CHarvesterWMVPlugin::HandleObjectPropertiesL()" );
    
    CMdEObject& mdeObject = aHD.MdeObject();
    
    if( !iPropDefs )
		{
		CMdEObjectDef& objectDef = mdeObject.Def();
		iPropDefs = CHarvesterWmvPluginPropertyDefs::NewL( objectDef );
		}
    
    if( ! mdeObject.Placeholder() )
    	{
        // Creation date
    	TTimeIntervalSeconds timeOffset = User::UTCOffset();
    	TTime localModifiedDate = aClipDetails.iModifiedDate + timeOffset;
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iCreationDatePropertyDef, &localModifiedDate, aIsAdd );
    	// Last modified date
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iLastModifiedDatePropertyDef, &aClipDetails.iModifiedDate, aIsAdd );
    	// File size
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iSizePropertyDef, &aClipDetails.iFileSize, aIsAdd );
        // Mime Type
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iItemTypePropertyDef, &aClipDetails.iMimeType, aIsAdd );
    	}
    }

