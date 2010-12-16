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
#include <pathinfo.h>

#include "mdsutils.h"
#include "harvesterexifutil.h"
#include "harvesteromadrmplugin.h"
#include "harvesterlog.h"
#include "harvestercommon.h"
#include "mdeobjectwrapper.h"
#include "mdscommoninternal.h"
#include "harvestervideoparser.h"
#include <harvesterdata.h>

#include <mdenamespacedef.h>
#include <mdeobjectdef.h>
#include <mdeobject.h>
#include <mdetextproperty.h>
#include <mdenamespacedef.h>
#include <mdeconstants.h>
#include <imageconversion.h>

using namespace MdeConstants;

_LIT(KImage, "Image");
_LIT(KVideo, "Video");
_LIT(KAudio, "Audio");
_LIT(KRmMimetype, "realmedia");

_LIT( KSvgMime, "image/svg+xml" );
_LIT( KRingingToneMime, "application/vnd.nokia.ringing-tone" );

_LIT(KInUse, "InUse");

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
	iDefaultFolderPropertyDef = &objectDef.GetPropertyDefL( MdeConstants::Object::KInDefaultFolder );

	CMdEObjectDef& mediaDef = nsDef.GetObjectDefL( MdeConstants::MediaObject::KMediaObject );
	iDrmPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KDRMProperty );
	iDescriptionPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KDescriptionProperty );
	iAuthorPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KAuthorProperty );
	iContentIDPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KContentID );
	iRightsStatusPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KRightsStatus );

	// Media property definitions
	iWidthPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KWidthProperty );
	iHeightPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KHeightProperty );

	// Image property definitions
	CMdEObjectDef& imageDef = nsDef.GetObjectDefL( MdeConstants::Image::KImageObject );
	iBitsPerSamplePropertyDef = &imageDef.GetPropertyDefL( MdeConstants::Image::KBitsPerSampleProperty );
	iFrameCountPropertyDef = &imageDef.GetPropertyDefL( MdeConstants::Image::KFrameCountProperty );
	iGenrePropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KGenreProperty );
	
	// Video specific property definitons
    CMdEObjectDef& videoDef = nsDef.GetObjectDefL( MdeConstants::Video::KVideoObject );
	iReleaseDatePropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KReleaseDateProperty );
	iCaptureDatePropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KCaptureDateProperty );
	iDurationPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KDurationProperty );
    iFrameratePropertyDef = &videoDef.GetPropertyDefL( MdeConstants::Video::KFramerateProperty );
    iBitratePropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KBitrateProperty );
    iCopyrightPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KCopyrightProperty );
    iArtistPropertyDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KArtistProperty );
    iAudioFourCCDef = &mediaDef.GetPropertyDefL( MdeConstants::MediaObject::KAudioFourCCProperty );
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
	
	iFs.Close();

	delete iPropDefs;
    iPropDefs = NULL;

    delete iVideoParser;
    iVideoParser = NULL;
    
    delete iPhoneImagesPath;
    iPhoneImagesPath = NULL;
    delete iMmcImagesPath;
    iMmcImagesPath = NULL;
    
    delete iPhoneVideosPath;
    iPhoneVideosPath = NULL;
    delete iMmcVideosPath;
    iMmcVideosPath = NULL;
    
    delete iPhoneSoundsPath;
    iPhoneSoundsPath = NULL;
    delete iMmcSoundsPath;
    iMmcSoundsPath = NULL;
	}

/**
* 2nd phase constructor
*/
void CHarvesterOMADRMPlugin::ConstructL()
	{
	WRITELOG( "CHarvesterOMADRMPlugin::ConstructL()" );
    SetPriority( KHarvesterPriorityHarvestingPlugin - 1 );	
    
    User::LeaveIfError( iFs.Connect() );
	
    iVideoParser = CHarvesterVideoParser::NewL();
    
	TFileName phoneRoot = PathInfo::PhoneMemoryRootPath();
	TFileName mmcRoot = PathInfo::MemoryCardRootPath();
	
	TFileName images = PathInfo::ImagesPath();
	
    TFileName phoneImagePath( phoneRoot );
    phoneImagePath.Append( images );
    iPhoneImagesPath = phoneImagePath.AllocL();

    TFileName mmcImagePath( mmcRoot );
    mmcImagePath.Append( images );
    iMmcImagesPath = mmcImagePath.Right( mmcImagePath.Length() - 1 ).AllocL();
    
    TFileName videos = PathInfo::VideosPath();
    
    TFileName phoneVideoPath( phoneRoot );
    phoneVideoPath.Append( videos );
    iPhoneVideosPath = phoneVideoPath.AllocL();

    TFileName mmcVideoPath( mmcRoot );
    mmcVideoPath.Append( videos );
    iMmcVideosPath = mmcVideoPath.Right( mmcVideoPath.Length() - 1 ).AllocL();
    
    TFileName sounds = PathInfo::SoundsPath();
    
    TFileName phoneSoundPath( phoneRoot );
    phoneSoundPath.Append( sounds );
    iPhoneSoundsPath = phoneSoundPath.AllocL();

    TFileName mmcSoundPath( mmcRoot );
    mmcSoundPath.Append( sounds );
    iMmcSoundsPath = mmcSoundPath.Right( mmcSoundPath.Length() - 1 ).AllocL();
	}

void CHarvesterOMADRMPlugin::HarvestL( CHarvesterData* aHarvesterData )
	{
	WRITELOG( "CHarvesterOMADRMPlugin::HarvestL()" );
    CMdEObject& mdeObject = aHarvesterData->MdeObject();
	CDRMHarvestData* drmHarvestData = CDRMHarvestData::NewL();
	CleanupStack::PushL( drmHarvestData );
	
    CFileData* fileData = CFileData::NewL();
    CleanupStack::PushL( fileData );

    CHarvestData* harvestData = CHarvestData::NewL();
    CleanupStack::PushL( harvestData );
    
    CVideoHarvestData* videoHarvestData = new (ELeave) CVideoHarvestData;
    CleanupStack::PushL( videoHarvestData );
        
    TInt errorCode( KErrNone );
    
    TRAPD( error, errorCode = GatherDataL( mdeObject, *drmHarvestData, *fileData, *harvestData, *videoHarvestData ) );
    
    if ( error == KErrNone && (errorCode == KErrNone || errorCode == KErrCompletion ) ) // ok, something got harvested
        {
        if ( mdeObject.Id() == 0 || mdeObject.Placeholder() ) // is a new object or placeholder
            {
            TRAP_IGNORE( HandleObjectPropertiesL( *harvestData,  *drmHarvestData, *fileData, 
                                                  *aHarvesterData, *videoHarvestData, ETrue ) );
            mdeObject.SetPlaceholder( EFalse );
            }
        else   // not a new object
            {
            TRAP_IGNORE( HandleObjectPropertiesL( *harvestData, *drmHarvestData, *fileData, 
                                                   *aHarvesterData, *videoHarvestData, EFalse ) );
            }

        if ( error != KErrNone )
            {
            WRITELOG1( "CHarvesterOMADRMPlugin::HarvestL() - Handling object failed: ", error );
            }
    	}
    else	
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::HarvestL() - TRAP error: %d, errorCode %d", error );
        TInt convertedError = KErrNone;
        MdsUtils::ConvertTrapError( error, convertedError );
        aHarvesterData->SetErrorCode( convertedError );
        }

    CleanupStack::PopAndDestroy( 4, drmHarvestData );
	}

// ---------------------------------------------------------------------------
// GatherDataL
// ---------------------------------------------------------------------------
//
TInt CHarvesterOMADRMPlugin::GatherDataL( CMdEObject& aMetadataObject, CDRMHarvestData& aDRMharvestData, 
        CFileData& aFileData, CHarvestData& /*aHarvestData*/, CVideoHarvestData& aVHD )
    {
    WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL" );
    
    TEntry* entry = new (ELeave) TEntry();
    CleanupStack::PushL( entry );
    
    const TDesC& uri = aMetadataObject.Uri();
    User::LeaveIfError( iFs.Entry( uri, *entry ) );
    
    aDRMharvestData.iModified = entry->iModified;
    aDRMharvestData.iFileSize = (TUint)entry->iSize;
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
    attrSet.AddL( ContentAccess::EContentID );

    User::LeaveIfError( data->GetStringAttributeSet(attrSet) );
    
    TInt err = attrSet.GetValue( ContentAccess::EDescription, aDRMharvestData.iDescription );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting description failed %d", err );
        }
        
    if ( aDRMharvestData.iDescription.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no description" );
        }
    
    err = attrSet.GetValue( ContentAccess::EMimeType, aDRMharvestData.iMimetype );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting mimetype failed %d", err );
        }
        
    if ( aDRMharvestData.iMimetype.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no mimetype" );
        }
    
    err = attrSet.GetValue( ContentAccess::ETitle, aDRMharvestData.iTitle );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting title failed %d", err );
        }
        
    if ( aDRMharvestData.iTitle.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no title" );
        }
    
    err = attrSet.GetValue( ContentAccess::EAuthor, aDRMharvestData.iAuthor );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting author failed %d", err );
        }
        
    if ( aDRMharvestData.iAuthor.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no author" );
        }

    err = attrSet.GetValue( ContentAccess::EGenre, aDRMharvestData.iGenre );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting genre failed %d", err );
        }
        
    if ( aDRMharvestData.iGenre.Length() <= 0 )
        {
        WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no genre" );
        }
    
    err = content->GetAttribute( ContentAccess::EIsProtected, aDRMharvestData.iDrmProtected );
    if ( err != KErrNone)
        {
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting protection info failed %d", err );
        }

    if( aDRMharvestData.iDrmProtected )
        {
        TPtr cid( NULL, 0 );
        HBufC* uniqueId( HBufC::NewL( ContentAccess::KMaxCafUniqueId ) );
        cid.Set( uniqueId->Des() );   
        err = attrSet.GetValue( ContentAccess::EContentID, cid );
        if ( err != KErrNone)
            {
            WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting content ID failed %d", err );
            delete uniqueId;
            uniqueId = NULL;
            }
        else
            {
            aDRMharvestData.iContentID = cid.AllocL();
            delete uniqueId;
            uniqueId = NULL;
            }
        
        TInt noRights( 0 );
        err = content->GetAttribute( ContentAccess::ERightsNone, noRights );
        if ( err != KErrNone)
            {
            noRights = 0;
            WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting rights info failed %d", err );
            }
        else if( noRights < 0 )
            {
            noRights = 0;
            }
        
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ContentAccess::ERightsNone returns %d", noRights );

        TInt rightsPending( 0 );
        err = content->GetAttribute( ContentAccess::ERightsPending, rightsPending );
        if ( err != KErrNone)
            {
            rightsPending = 0;
            WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ERROR: getting rights info failed %d", err );
            }
        else if( rightsPending < 0 )
            {
            rightsPending = 0;
            }
        
        WRITELOG1( "CHarvesterOMADRMPlugin::GatherDataL - ContentAccess::ERightsPending returns %d", rightsPending );

        if( (noRights == 1) || (rightsPending == 1) )
            {
            WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - no rights available" );
            aDRMharvestData.iRightsStatus = MdeConstants::MediaObject::ENoRights;
            }
        else
            {
            WRITELOG( "CHarvesterOMADRMPlugin::GatherDataL - rights available" );
            aDRMharvestData.iRightsStatus = MdeConstants::MediaObject::ERightsReceived;
            }
        }
    
    if( aMetadataObject.Def().Name() == MdeConstants::Image::KImageObject )
        {
        CImageDecoder* decoder = NULL;

        TRAP( err, decoder = CImageDecoder::FileNewL( iFs, uri, ContentAccess::EPeek, 
                ( CImageDecoder::TOptions )( CImageDecoder::EPreferFastDecode )));

        CleanupStack::PushL( decoder );
    
        if(decoder && !err)
            {
            WRITELOG( "CHarvesterImagePlugin::GatherData() - Image decoder has opened the file." );        
            // Get image width, frame count, height and bits per pixel from image decoder.
            const TFrameInfo info = decoder->FrameInfo( 0 );
            const TSize imageSize = info.iOverallSizeInPixels;
            const TInt framecount = decoder->FrameCount();
            aFileData.iFrameCount = framecount;
            aFileData.iImageWidth = imageSize.iWidth;
            aFileData.iImageHeight = imageSize.iHeight;
            aFileData.iBitsPerPixel = info.iBitsPerPixel;
            }
        else
            {
            WRITELOG1( "CHarvesterOMADRMPlugin::GatherData() - ERROR: decoder %d", err );
            }    
        CleanupStack::PopAndDestroy( decoder );
        }
    else if( aMetadataObject.Def().Name() == MdeConstants::Video::KVideoObject )
        {
        RFile64 file;
        TInt error = file.Open( iFs, uri, EFileRead | EFileShareReadersOnly );
        CleanupClosePushL( file );
        if ( error == KErrInUse ||
             error == KErrLocked )
            {
            WRITELOG( "CHarvesterOMADRMPlugin - File is open!" );
#ifdef _DEBUG
            TPtrC fileName( uri.Mid(2) );
            WRITELOG1( "CHarvesterOMADRMPlugin :: Checking open file handles to %S", &fileName );

            CFileList* fileList = 0;
            TOpenFileScan fileScan( iFs );

            fileScan.NextL( fileList );   
  
            while ( fileList )   
                {
                const TInt count( fileList->Count() ); 
                for (TInt i = 0; i < count; i++ )   
                    {   
                    if ( (*fileList)[i].iName == uri.Mid(2) )
                        {
                        TFullName processName;
                        TFindThread find(_L("*"));
                        while( find.Next( processName ) == KErrNone )
                            {
                            RThread thread;
                            TInt err = thread.Open( processName );
     
                            if ( err == KErrNone )
                                {
                                if ( thread.Id().Id() ==  fileScan.ThreadId() )
                                    {
                                    processName = thread.Name();
                                    thread.Close();
                                    WRITELOG1( "CHarvesterOMADRMPlugin:: %S has a file handle open", &processName );
                                    break;
                                    }
                                thread.Close();
                                }
                            }
                        }
                    }
                fileScan.NextL( fileList );   
                } 
#endif
            CleanupStack::PopAndDestroy( &file );
            User::Leave( KErrInUse );
            }
        
        TRAP_IGNORE( iVideoParser->ParseVideoMetadataL( file, aVHD ));
        CleanupStack::PopAndDestroy( &file );
        }

    CleanupStack::PopAndDestroy( 3 ); // content, data, attrSet
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// HandleObjectPropertiesL
// ---------------------------------------------------------------------------
//
void CHarvesterOMADRMPlugin::HandleObjectPropertiesL( CHarvestData& /*aHarvestData*/, CDRMHarvestData& aDRMharvestData, CFileData& aFileData, 
        CHarvesterData& aHarvesterData, CVideoHarvestData& aVHD, TBool aIsAdd )
    {
    WRITELOG("CHarvesterOMADRMPlugin - HandleNewObject ");
    CMdEObject& mdeObject = aHarvesterData.MdeObject();

    if( !iPropDefs )
    	{
    	CMdEObjectDef& objectDef = mdeObject.Def();
    	iPropDefs = CHarvesterOmaDrmPluginPropertyDefs::NewL( objectDef );
    	// Prefetch max text lengt for validity checking
    	iMaxTextLength = iPropDefs->iGenrePropertyDef->MaxTextLengthL();
    	}
    
    TTimeIntervalSeconds timeOffset = User::UTCOffset();
    
    TPtrC objectDefName( mdeObject.Def().Name());
    
    if( ! mdeObject.Placeholder() )
    	{
    	// Creation date
    	TTime localTime = aDRMharvestData.iModified + timeOffset;
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iCreationDatePropertyDef, &localTime, aIsAdd );
    	// Last modified date
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iLastModifiedDatePropertyDef, &aDRMharvestData.iModified, aIsAdd );
    	// File size
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iSizePropertyDef, &aDRMharvestData.iFileSize, aIsAdd );

    	TPtrC objectDefName( mdeObject.Def().Name());
    	
        if( objectDefName == MdeConstants::Image::KImageObject )
            {
            const TDesC& uri = mdeObject.Uri();
            if( uri.FindF( iMmcImagesPath->Des()) != KErrNotFound ||
                uri.FindF( iPhoneImagesPath->Des()) != KErrNotFound ||
                uri.FindF( KDCIMFolder ) != KErrNotFound )
                {
                TBool inDefaultFolder( ETrue );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );
                }
            else
                {
                TBool inDefaultFolder( EFalse );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );    
                }    
            }
        else if( objectDefName == MdeConstants::Video::KVideoObject )
            {
            const TDesC& uri = mdeObject.Uri();
            if( uri.FindF( iMmcVideosPath->Des()) != KErrNotFound ||
                uri.FindF( iPhoneVideosPath->Des()) != KErrNotFound ||
                uri.FindF( KDCIMFolder ) != KErrNotFound )
                {
                TBool inDefaultFolder( ETrue );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );
                }
            else
                {
                TBool inDefaultFolder( EFalse );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );    
                }    
            }
        else if( objectDefName == MdeConstants::Audio::KAudioObject )
            {
            const TDesC& uri = mdeObject.Uri();
            if( uri.FindF( iMmcSoundsPath->Des()) != KErrNotFound ||
                uri.FindF( iPhoneSoundsPath->Des()) != KErrNotFound )
                {
                TBool inDefaultFolder( ETrue );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );
                }
            else
                {
                TBool inDefaultFolder( EFalse );
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDefaultFolderPropertyDef, &inDefaultFolder, aIsAdd );    
                }     
            }
    	}
        
    // Item Type
    if(aDRMharvestData.iMimetype.Length() > 0)
        {
        TBool isAdd( EFalse );
        CMdEProperty* prop = NULL;
        TInt index = mdeObject.Property( *iPropDefs->iItemTypePropertyDef, prop );
        if( index < 0 )
            {
            isAdd = ETrue;
            }
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iItemTypePropertyDef, &aDRMharvestData.iMimetype, isAdd );
        }
    
    // DRM protection
    CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    		*iPropDefs->iDrmPropertyDef, &aDRMharvestData.iDrmProtected, aIsAdd );
    
    if( aDRMharvestData.iDrmProtected )
        {
        // DRM Content ID
        const TInt maxContentIDLength = iPropDefs->iContentIDPropertyDef->MaxTextLengthL();
        if( aDRMharvestData.iContentID && aDRMharvestData.iContentID->Length() <= maxContentIDLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                    *iPropDefs->iContentIDPropertyDef, aDRMharvestData.iContentID, aIsAdd );
            }
        
        // Rights status
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iRightsStatusPropertyDef, &aDRMharvestData.iRightsStatus, aIsAdd );        
        }

    // Title (is set from URI by default)
    if( aDRMharvestData.iTitle.Length() > 0 && aDRMharvestData.iTitle.Length() < KMaxTitleFieldLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iTitlePropertyDef, &aDRMharvestData.iTitle, EFalse );
    	}
    // Description
    if( aDRMharvestData.iDescription.Length() > 0 && aDRMharvestData.iDescription.Length() < iMaxTextLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iDescriptionPropertyDef, &aDRMharvestData.iDescription, aIsAdd );
    	}   
    // Author
    if( aDRMharvestData.iAuthor.Length() > 0 && aDRMharvestData.iAuthor.Length() < iMaxTextLength )
    	{
    	CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
    			*iPropDefs->iAuthorPropertyDef, &aDRMharvestData.iAuthor, aIsAdd );
    	}
    // Genre
    if( aDRMharvestData.iGenre.Length() > 0 && aDRMharvestData.iGenre.Length() < iMaxTextLength )
        {
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, 
                *iPropDefs->iGenrePropertyDef, &aDRMharvestData.iGenre, aIsAdd );
        }
    
    if( objectDefName == MdeConstants::Image::KImageObject )
        {
      // Image - Bits per Sample
        if (aFileData.iBitsPerPixel != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iBitsPerSamplePropertyDef, &aFileData.iBitsPerPixel, aIsAdd );
            }
    
        // Image - Framecount
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iFrameCountPropertyDef, &aFileData.iFrameCount, aIsAdd );
        
        // MediaObject - Width
        if (aFileData.iImageWidth != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iWidthPropertyDef, &aFileData.iImageWidth, aIsAdd );
            }
        
        // MediaObject - Height
        if (aFileData.iImageHeight != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iHeightPropertyDef, &aFileData.iImageHeight, aIsAdd );
            } 
        }
    else if( objectDefName == MdeConstants::Video::KVideoObject )
        {
        TTimeIntervalSeconds timeOffsetSeconds = User::UTCOffset();
        TTime localModifiedDate = aVHD.iModified + timeOffsetSeconds;
        
        if( aVHD.iMimeBuf )
            {
            TBool isAdd( EFalse );
            CMdEProperty* prop = NULL;
            TInt index = mdeObject.Property( *iPropDefs->iItemTypePropertyDef, prop );
            if( index < 0 )
                {
                isAdd = ETrue;
                }
                CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iItemTypePropertyDef, aVHD.iMimeBuf, isAdd );
            }
        
        // Release date
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iReleaseDatePropertyDef, &localModifiedDate, aIsAdd );

        // Capture date
        CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iCaptureDatePropertyDef, &localModifiedDate, aIsAdd );

        // Duration
        if( aVHD.iDuration != 0.0f )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDurationPropertyDef, &aVHD.iDuration, aIsAdd );
            }

        // Width
        if (aVHD.iFrameWidth != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iWidthPropertyDef, &aVHD.iFrameWidth, aIsAdd );
            }

        // Height
        if (aVHD.iFrameHeight != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iHeightPropertyDef, &aVHD.iFrameHeight, aIsAdd );
            }
    
        // Framerate
        if (aVHD.iFrameRate != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iFrameratePropertyDef, &aVHD.iFrameRate, aIsAdd );
            }

        // Bitrate
        if (aVHD.iClipBitrate != 0)
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iBitratePropertyDef, &aVHD.iClipBitrate, aIsAdd );
            }
        else if( aVHD.iVideoBitrate != 0 )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iBitratePropertyDef, &aVHD.iVideoBitrate, aIsAdd );
            }

        // Copyright
        if( aVHD.iCopyright && aVHD.iCopyright->Length() < iMaxTextLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iCopyrightPropertyDef, aVHD.iCopyright, aIsAdd );
            }

        // Author
        if( aVHD.iAuthor && aVHD.iAuthor->Length() < iMaxTextLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iAuthorPropertyDef, aVHD.iAuthor, aIsAdd );
            }

        // Genre
        if( aVHD.iGenre && aVHD.iGenre->Length() < iMaxTextLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iGenrePropertyDef, aVHD.iGenre, aIsAdd );
            }

        // Artist
        if( aVHD.iPerformer && aVHD.iPerformer->Length() < iMaxTextLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iArtistPropertyDef, aVHD.iPerformer, aIsAdd );
            }

        // Description
        if( aVHD.iDescription && aVHD.iDescription->Length() < iMaxTextLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iDescriptionPropertyDef, aVHD.iDescription, aIsAdd );
            }
        
        // Codec
        if( aVHD.iCodec != 0 )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iAudioFourCCDef, &aVHD.iCodec, aIsAdd );
            }
        
        // Title
        if( aVHD.iTitle && aVHD.iTitle->Length() < KMaxTitleFieldLength )
            {
            CMdeObjectWrapper::HandleObjectPropertyL(mdeObject, *iPropDefs->iTitlePropertyDef, aVHD.iTitle, EFalse );
            }          
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
		content = NULL;
		}

#ifdef _DEBUG
    if( err == KErrInUse || err == KErrLocked )
        {
        TPtrC fileName( aUri.Mid(2) );
        WRITELOG1( "CHarvesterOMADRMPlugin :: Checking open file handles to %S", &fileName );

        CFileList* fileList = 0;
        TOpenFileScan fileScan( iFs );

        TRAP_IGNORE( fileScan.NextL( fileList ) );   
          
        while ( fileList )   
            {
            const TInt count( fileList->Count() ); 
            for (TInt i = 0; i < count; i++ )   
                {   
                if ( (*fileList)[i].iName == aUri.Mid(2) )
                    {
                    TFullName processName;
                    TFindThread find(_L("*"));
                    while( find.Next( processName ) == KErrNone )
                        {
                        RThread thread;
                        TInt error = thread.Open( processName );
             
                        if ( error == KErrNone )
                            {
                            if ( thread.Id().Id() ==  fileScan.ThreadId() )
                                {
                                processName = thread.Name();
                                thread.Close();
                                WRITELOG1( "CHarvesterOMADRMPlugin :: %S has a file handle open", &processName );
                                break;
                                }
                            thread.Close();
                            }
                        }
                    }
                }
            TRAP_IGNORE( fileScan.NextL( fileList ) );   
            } 
        }
#endif
	
	if( err == KErrInUse || err == KErrLocked )
	    {
	    aObjectType.Copy( KInUse() );
	    return;
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
		
	    if( mime == KRingingToneMime )
	        {
	        WRITELOG1( "CHarvesterOMADRMPlugin::GetObjectType - mimetype %S. Object type changed to Audio", &mime );
	        aObjectType.Copy( KAudio );
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
        content = NULL;
        }
    }

