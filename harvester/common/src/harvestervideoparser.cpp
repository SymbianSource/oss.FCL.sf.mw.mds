/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies). 
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

#include <hxmetadatautil.h>
#include <hxmetadatakeys.h>

#include "mdsutils.h"
#include "harvestervideoparser.h"
#include "harvesterlog.h"

_LIT( KMimeTypeRm,        "application/vnd.rn-realmedia" ); // can be audio or video

_LIT(KVideo, "Video");
_LIT(KAudio, "Audio");

_LIT(KAudioAC3, "audio/AC3");
_LIT(KAudioEAC3, "audio/EAC3");
const TUint32 KMDSFourCCCodeAC3 = 0x33434120;       //{' ', 'A', 'C', '3'}
const TUint32 KMDSFourCCCodeEAC3 = 0x33434145;      //{'E', 'A', 'C', '3'}

const TInt KKiloBytes = 1024;
const TReal32 KThousandReal = 1000.0;

CHarvesterVideoParser::CHarvesterVideoParser()
    {
	// No implementation required
    }


CHarvesterVideoParser::~CHarvesterVideoParser()
    {
    }

EXPORT_C CHarvesterVideoParser* CHarvesterVideoParser::NewL()
    {
	CHarvesterVideoParser* self = new (ELeave)CHarvesterVideoParser();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop( self );
	return self;
    }

void CHarvesterVideoParser::ConstructL()
    {

    }

EXPORT_C void CHarvesterVideoParser::ParseVideoMetadataL( RFile64& aFileHandle, CVideoHarvestData& aVHD )
    {
    // doesn't own pointers to MIME types
    RPointerArray<HBufC> mimes;
    CleanupClosePushL( mimes );
    
    CHXMetaDataUtility* helixMetadata = CHXMetaDataUtility::NewL();
    CleanupStack::PushL( helixMetadata );

    helixMetadata->OpenFileL( aFileHandle );        

    // No need for the file handle anymore so closing it
    WRITELOG( "CHarvesterVideoPlugin - Parsing done, file handle can be closed" );   
    aFileHandle.Close();

    HBufC *buf = NULL;
    HXMetaDataKeys::EHXMetaDataId metaid;           
    TUint metacount = 0;
    helixMetadata->GetMetaDataCount( metacount );
    TLex lex;
    for ( TUint i = 0; i < metacount; i++ )
        {               
        helixMetadata->GetMetaDataAt( i, metaid, buf );
        switch (metaid)
            {
            case HXMetaDataKeys::EHXTitle:
                {
                aVHD.iTitle = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXVideoBitRate:
                {
                WRITELOG( "CHarvesterVideoPlugin - found videobitrate" );
                if( aVHD.iVideoObject )
                    {
                    lex.Assign( *buf );
                    if( KErrNone == lex.Val( aVHD.iVideoBitrate ) )
                        {
                        aVHD.iVideoBitrate /= KKiloBytes;
                        }
                    }
                break;
                }
            case HXMetaDataKeys::EHXAudioBitRate:
                {
                WRITELOG( "CHarvesterVideoPlugin - found audiobitrate" );
                lex.Assign( *buf );
                if( KErrNone == lex.Val( aVHD.iAudioBitrate ) )
                    {
                    aVHD.iAudioBitrate /= KKiloBytes;
                    }
                break;
                }
            case HXMetaDataKeys::EHXClipBitRate:
                {
                WRITELOG( "CHarvesterVideoPlugin - found clipbitrate" );
                lex.Assign( *buf );
                if( KErrNone == lex.Val( aVHD.iClipBitrate ) )
                    {
                    aVHD.iClipBitrate /= KKiloBytes;
                    }
                break;
                }
            case HXMetaDataKeys::EHXDuration:
                {
                WRITELOG( "CHarvesterVideoPlugin - found duration" );
                lex.Assign(*buf);
                if( KErrNone == lex.Val( aVHD.iDuration ) )
                    {
                    aVHD.iDuration /= KThousandReal;
                    }
                break;
                }
            case HXMetaDataKeys::EHXFramesPerSecond:
                {
                WRITELOG( "CHarvesterVideoPlugin - found framerate" );
                lex.Assign( *buf );
                lex.Val( aVHD.iFrameRate );
                break;
                }
            case HXMetaDataKeys::EHXCopyright:
                {
                aVHD.iCopyright = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXAuthor:
                {
                aVHD.iAuthor = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXGenre:
                {
                aVHD.iGenre = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXPerformer:
                {
                aVHD.iPerformer = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXDescription:
                {
                aVHD.iDescription = buf->Alloc();
                break;
                }
            case HXMetaDataKeys::EHXMimeType:
                {
                mimes.AppendL( buf );
                if( aVHD.iCodec == 0 )
                    {
                    CheckForCodecSupport( buf, aVHD );
                    }
                break;
                }
            case HXMetaDataKeys::EHXFrameSize:
                {
                const TChar separator = 'x';    // as in e.g."177x144"
                const TInt separatorLocation = buf->Locate( separator );
                TLex input( buf->Left( separatorLocation ) );

                input.Val( aVHD.iFrameWidth );
                input = buf->Right(buf->Length() - separatorLocation - 1);
                input.Val( aVHD.iFrameHeight );
                break;
                }
            default:
                break;
            }
        }

    const TInt mimeCount = mimes.Count();

    TPtrC mime( NULL, 0 );

    if( mimeCount == 0 )
        {
        aVHD.iMimeBuf = NULL;
        }
    else
        {
        for( TInt i = 0; i < mimeCount; i++ )
            {
            HBufC* mimeTmp = mimes[i];
            
            if( !mimeTmp )
                {
                continue;
                }
            
            mime.Set( mimeTmp->Des().Ptr(), mimeTmp->Des().Length() );

            // MIME contains substring "application/vnd.rn-realmedia".
            // That case MIME matches also with 
            // string "application/vnd.rn-realmedia-vbr".
            if( MdsUtils::Find( mime, KMimeTypeRm() ) != KErrNotFound )
                {
                break;
                }
            // Match MIME type, for video object with "video" substring
            else if( aVHD.iVideoObject )
                {
                if( MdsUtils::Find( mime, KVideo() ) != KErrNotFound )
                    {
                    break;
                    }
                }
            // Match MIME type for audio object with "audio" substring
            else if( MdsUtils::Find( mime, KAudio() ) != KErrNotFound )
                {
                if( !aVHD.iVideoObject )
                    {
                    break;
                    }
                }
            }
        
        if( mime.Ptr() && ( mime.Length() > 0 ) )
            {
            aVHD.iMimeBuf = mime.Alloc();
            }
        }

    helixMetadata->ResetL();
    CleanupStack::PopAndDestroy( helixMetadata );

    // don't destory mime type pointers just clean array
    CleanupStack::PopAndDestroy( &mimes );
    }

void CHarvesterVideoParser::CheckForCodecSupport( HBufC* aMimeBuffer, CVideoHarvestData& aVHD )
    {
    if( !aMimeBuffer )
        {
        return;
        }
    
    TPtrC mime( NULL, 0 );
    mime.Set( aMimeBuffer->Des().Ptr(), aMimeBuffer->Des().Length() );
    
    if( MdsUtils::Find( mime, KAudioAC3() ) != KErrNotFound )
        {
        aVHD.iCodec = KMDSFourCCCodeAC3;
        return;
        }
    
    if( MdsUtils::Find( mime, KAudioEAC3() ) != KErrNotFound )
        {
        aVHD.iCodec = KMDSFourCCCodeEAC3;
        return;
        }
    return;
    }
