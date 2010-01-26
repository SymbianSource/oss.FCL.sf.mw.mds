/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Implementation of CLF API test 
*
*/


// INCLUDES
#include "T_CLFApiModuleTests.h"
#include <s32mem.h> 
#include <eunitmacros.h>

// the header for the tested class
#include <ContentListingFactory.h>
#include <MCLFChangedItemObserver.h>
#include <MCLFContentListingEngine.h>
#include <MCLFCustomGrouper.h>
#include <MCLFCustomSorter.h>
#include <MCLFItem.h>
#include <MCLFItemListModel.h>
#include <MCLFModifiableItem.h>
#include <MCLFOperationObserver.h>
#include <MCLFPostFilter.h>
#include <MCLFSortingStyle.h>
#include <T_ContentListingFramework.rsg>
#include <collate.h>
#include <pathInfo.h>
#include <MCLFProcessObserver.h>
#include <CLFContentListingExtended.hrh>

//CONSTS
#ifdef __WINSCW__
_LIT( KTestResourceFile, "z:\\resource\\T_ContentListingFramework.rsc" );
#else
_LIT( KTestResourceFile, "c:\\sys\\bin\\T_ContentListingFramework.rsc" );
#endif
_LIT( KTestFileNameBase, "CLFTestFiles\\TestFile" );
_LIT( KTestFileExt, ".txt" );

const TCLFFieldId KMultibleSortingTestField1 = 0x80000001;
const TCLFFieldId KMultibleSortingTestField2 = 0x80000002;
const TCLFFieldId KMultibleSortingTestField3 = 0x80000003;
const TCLFFieldId KMultibleSortingTestField4 = 0x80000004;
const TCLFFieldId KMultibleSortingTestField5 = 0x80000005;
const TCLFFieldId KMultibleSortingTestField6 = 0x80000006;

const TInt KCLFUpdateFoldersSemanticId = 0x1000;

// ---------------------------------------------------------------------------
// class CMGXAsyncCallback
// ---------------------------------------------------------------------------
//
class CCLFAsyncCallback : public CBase
    {
    public:
        ~CCLFAsyncCallback()
            {
            if ( iActiveWait.IsStarted() )
                {
                iActiveWait.AsyncStop();
                }
            }
    public:
        static void AfterL( TTimeIntervalMicroSeconds32 aInterval )
            {
            CCLFAsyncCallback* self = new( ELeave ) CCLFAsyncCallback();
            CleanupStack::PushL( self );
            CPeriodic* periodic = CPeriodic::NewL( CActive::EPriorityIdle );
            CleanupStack::PushL( periodic );
            TCallBack callBack( CallBackL, self );
            TTimeIntervalMicroSeconds32 interval( 1000000 );
            periodic->Start( aInterval, aInterval, callBack );
            self->iActiveWait.Start();
            periodic->Cancel();
            CleanupStack::PopAndDestroy( 2 ); // periodic, self
            }

        static TInt CallBackL( TAny* aObject )
            {
            CCLFAsyncCallback* self = reinterpret_cast< CCLFAsyncCallback* >( aObject );
            if ( self->iActiveWait.IsStarted() )
                {
                self->iActiveWait.AsyncStop();
                }
            return EFalse;
            }
            
    private:
        CActiveSchedulerWait iActiveWait;
    };

// ---------------------------------------------------------------------------
// class TTestOperationObserver
// ---------------------------------------------------------------------------
//
class TTestOperationObserver : public MCLFOperationObserver
    {
    public:
        TTestOperationObserver()
            // set invalid values
            : iOperationEvent( TCLFOperationEvent( -1 ) ), iError( 1 ), iWait( NULL )
            {}
        void HandleOperationEventL( TCLFOperationEvent aOperationEvent,
                                    TInt aError )
            {
            iError = aError;
            iOperationEvent = aOperationEvent;
            if( iWait &&
                iWait->IsStarted() )
                {
                iWait->AsyncStop();
                }
            }
        TCLFOperationEvent iOperationEvent;
        TInt iError;
        CActiveSchedulerWait* iWait;

    };

// ---------------------------------------------------------------------------
// class TTestCustomSorter
// ---------------------------------------------------------------------------
//
class TTestCustomSorter : public MCLFCustomSorter
    {
    public:
        TTestCustomSorter() : iSortItems( EFalse )
            {
            }
        void SortItemsL( RPointerArray<MCLFItem>& /*aItemArray*/ )
            {
            //aItemArray;
            iSortItems = ETrue;
            }
        TBool iSortItems;

    };

// ---------------------------------------------------------------------------
// class TTestCustomGrouper
// ---------------------------------------------------------------------------
//
class TTestCustomGrouper : public MCLFCustomGrouper
    {
    public:
        TTestCustomGrouper() : iGroupCount( 2 ), iCopyItems( EFalse ), iModifiableItems( NULL )
            {
            }
        void GroupItemsL( const TArray<MCLFItem*>& /*aSourceList*/,
                          RPointerArray<MCLFItem>& aGroupedList )
            {
            //aSourceList;
            if( iCopyItems )
                {
                TInt count( iModifiableItems->Count() );
                for( TInt i = 0 ; i < count ; ++i )
                    {
                    aGroupedList.AppendL( (*iModifiableItems)[i] );
                    }
                }
            else if( iModifiableItems )
                {
                iModifiableItems->ResetAndDestroy();
                for( TInt i = 0 ; i < iGroupCount ; ++i )
                    {
                    MCLFModifiableItem* item = ContentListingFactory::NewModifiableItemLC();
                    iModifiableItems->AppendL( item );
                    CleanupStack::Pop(); // item
                    aGroupedList.AppendL( item );
                    }
                }
            }
        TInt iGroupCount;
        TBool iCopyItems;
        RPointerArray<MCLFModifiableItem>* iModifiableItems;
    };

// ---------------------------------------------------------------------------
// class TTestPostFilter
// ---------------------------------------------------------------------------
//
class TTestPostFilter : public MCLFPostFilter
    {
    public:
        TTestPostFilter() : iShouldFilterCount( 5 ), iAllFilter( EFalse )
            {
            }

        void FilterItemsL( const TArray<MCLFItem*>& aItemList,
                           RPointerArray<MCLFItem>& aFilteredItemList )
            {
            iFilteredCount = 0;
            if( iAllFilter )
                {
                iFilteredCount = aItemList.Count();
                return;
                }
            for( TInt i = 0 ; i < aItemList.Count() ; ++i )
                {
                if( i < iShouldFilterCount  )
                    {
                    iFilteredCount++;
                    }
                else
                    {
                    aFilteredItemList.AppendL( aItemList[i] );
                    }
                }
            }
        TInt iShouldFilterCount;
        TBool iAllFilter;
        TInt iFilteredCount;

    };

// ---------------------------------------------------------------------------
// class TTestChangedItemObserver
// ---------------------------------------------------------------------------
//
class TTestChangedItemObserver : public MCLFChangedItemObserver
    {
    public:
        TTestChangedItemObserver()
            : iHandleItemChange( EFalse ),
              iLastError( KErrNone ),
              iChangedArray( NULL ),
              iWait( NULL )      
            {
            }
        void HandleItemChangeL( const TArray<TCLFItemId>& aItemIDArray )
            {
            iHandleItemChange = ETrue;
            if( iChangedArray )
                {
                iChangedArray->Reset();
                for( TInt i = 0 ; i < aItemIDArray.Count() ; ++i )
                    {
                    iChangedArray->AppendL( aItemIDArray[i] );
                    }
                }
            if( iWait && iWait->IsStarted() )
                {
                iWait->AsyncStop();
                }

            }
        void HandleError( TInt aError )
            {
            iLastError = aError;
            if( iWait && iWait->IsStarted() )
                {
                iWait->AsyncStop();
                }
            }
        TInt iHandleItemChange;
        TInt iLastError;
        RArray<TCLFItemId>* iChangedArray;
        CActiveSchedulerWait* iWait;
    };

// ---------------------------------------------------------------------------
// class TTestCLFProcessObserver
// ---------------------------------------------------------------------------
//
class TTestCLFProcessObserver : public MCLFProcessObserver
    {
    public:
        TTestCLFProcessObserver()
            : iStartEvent( EFalse ), iEndEvent( EFalse )
            {}
        void HandleCLFProcessEventL( TCLFProcessEvent aProcessEvent )
            {
            switch ( aProcessEvent )
                {
                case ECLFUpdateStart:
                    {
                    iStartEvent = ETrue;
                    break;
                    }
                case ECLFUpdateStop:
                    {
                    iEndEvent = ETrue;
                    break;
                    }
                default:
                    {
                    User::Panic( _L("CLF module test"), 1 );
                    }
                }
            }
        void Reset()
            {
            iStartEvent = EFalse;
            iEndEvent = EFalse;
            }
        TBool iStartEvent;
        TBool iEndEvent;
    };

void SerializeL( const MDesCArray& aDataArray, CBufBase& aBuffer )
    {
    const TInt count( aDataArray.MdcaCount() );
    RBufWriteStream writeStream( aBuffer );
    CleanupClosePushL( writeStream );
    writeStream.WriteInt32L( count );
    for( TInt i = 0 ; i < count ; ++i )
        {
        const TDesC& des = aDataArray.MdcaPoint( i );
        TInt length( des.Length() );
        writeStream.WriteInt32L( length );
        writeStream.WriteL( des, length );
        }
    CleanupStack::PopAndDestroy( &writeStream );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::NewLC()
// Create the testing class
// ---------------------------------------------------------------------------
//
T_CLFApiModuleTests* T_CLFApiModuleTests::NewLC()
    {
    T_CLFApiModuleTests* self = new(ELeave) T_CLFApiModuleTests;

    CleanupStack::PushL( self );
    // need to generate the table, so call base classes
    // second phase constructor
    self->ConstructL();
    return self;
    }


// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ConstructL()
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::ConstructL()
    {
    CEUnitTestSuiteClass::ConstructL();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::~T_CLFApiModuleTests()
// ---------------------------------------------------------------------------
//
T_CLFApiModuleTests::~T_CLFApiModuleTests()
    {
    Teardown();
    }

/**
 * Assistance methods
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ResourceL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::ResourceL( TInt aResourceId )
    {
    delete iDataBuffer;
    iDataBuffer = NULL;
    iDataBuffer = iResourceFile.AllocReadL( aResourceId );
    iResourceReader.SetBuffer( iDataBuffer );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleResourceL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SortingStyleResourceL()
    {
    ResourceL( R_SORTING_STYLE );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ListModelResourceL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::ListModelResourceL()
    {
    ResourceL( R_LIST_MODEL );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateNewFileL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::CreateNewFileL( TInt aNumber, TDes& aFileName )
    {
    aFileName.Copy( PathInfo::PhoneMemoryRootPath() );
    aFileName.Append( KTestFileNameBase );
    TBuf<125> buf;
    buf.Num( aNumber );
    aFileName.Append( buf );
    aFileName.Append( KTestFileExt );

    RFile file;
    BaflUtils::EnsurePathExistsL( iFs, aFileName );
    TInt error( file.Replace( iFs, aFileName, EFileShareAny | EFileWrite ) );
    if( error == KErrNone )
        {
        error = file.Write( _L8("Test data") );
        }
    file.Close();
    User::LeaveIfError( error );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::FindTestFileIdL
// ---------------------------------------------------------------------------
//
TCLFItemId T_CLFApiModuleTests::FindTestFileIdL( TInt aNumber )
    {
    TCLFItemId id( 0 );
    MCLFItemListModel* model = iEngine->CreateListModelLC( *iTestObserver );
    iMimeTypeArray->AppendL( _L("*") );

    iTestObserver->iWait = &iWait;
    model->SetWantedMimeTypesL( *iMimeTypeArray );
    model->RefreshL();
    iWait.Start();

    TFileName testFileName( PathInfo::PhoneMemoryRootPath() );
    testFileName.Append( KTestFileNameBase );
    TBuf<125> buf;
    buf.Num( aNumber );
    testFileName.Append( buf );
    testFileName.Append( KTestFileExt );
    for( TInt i = 0 ; i < model->ItemCount() ; ++i )
        {
        const MCLFItem& item = model->Item( i );
        TPtrC fileName;
        if( item.GetField( ECLFFieldIdFileNameAndPath, fileName ) != KErrNone )
            {
            continue;
            }
        if( fileName.CompareF( testFileName ) == 0 )
            {
            id = item.ItemId();
            break;
            }
        }
    CleanupStack::PopAndDestroy(); // model
    return id;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckFileNameShortingL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckFileNameShortingL()
    {
    TCollationMethod m = *Mem::CollationMethodByIndex( 0 );
    m.iFlags |= TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase;

    for( TInt i = 0 ; i < iListModel->ItemCount() -1 ; ++i )
        {
        const MCLFItem& item = iListModel->Item( i );
        const MCLFItem& item1 = iListModel->Item( i + 1 );
        TPtrC name;
        TPtrC name1;

        if( item.GetField( ECLFFieldIdFileName, name ) != KErrNone ||
            item1.GetField( ECLFFieldIdFileName, name1 ) != KErrNone )
            {
            return EFalse;
            }
        if( name.CompareC( name1, 3, &m ) > 0 )
            {
            return EFalse;
            }
        }
    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckFileSizeShortingL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckFileSizeShortingL()
    {
    for( TInt i = 0 ; i < iListModel->ItemCount() -1 ; ++i )
        {
        const MCLFItem& item = iListModel->Item( i );
        const MCLFItem& item1 = iListModel->Item( i + 1 );
        TInt32 size;
        TInt32 size1;
        if( item.GetField( ECLFFieldIdFileSize, size ) != KErrNone ||
            item1.GetField( ECLFFieldIdFileSize, size1 ) != KErrNone )
            {
            return EFalse;
            }
        if( size < size1 )
            {
            return EFalse;
            }
        }
    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckMultibleSortingShortingL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckMultibleSortingShortingL()
    {
    const MCLFItem* item = &( iListModel->Item( 0 ) );
    TInt32 data( 0 );
    if( item->GetField( KMultibleSortingTestField2, data ) != KErrNone ||
        data != 5 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 1 ) );
    if( item->GetField( KMultibleSortingTestField2, data ) != KErrNone ||
        data != 4 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 2 ) );
    if( item->GetField( KMultibleSortingTestField2, data ) != KErrNone ||
        data != 3 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 3 ) );
    if( item->GetField( KMultibleSortingTestField3, data ) != KErrNone ||
        data != 6 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 4 ) );
    if( item->GetField( KMultibleSortingTestField3, data ) != KErrNone ||
        data != 7 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 5 ) );
    if( item->GetField( KMultibleSortingTestField3, data ) != KErrNone ||
        data != 8 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 6 ) );
    if( item->GetField( KMultibleSortingTestField4, data ) != KErrNone ||
        data != 9 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 7 ) );
    if( item->GetField( KMultibleSortingTestField4, data ) != KErrNone ||
        data != 10 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 8 ) );
    if( item->GetField( KMultibleSortingTestField4, data ) != KErrNone ||
        data != 11 )
        {
        return EFalse;
        }
// unsorted start
    item = &( iListModel->Item( 9 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        ( data < 15 || data > 17 ) )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 10 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        ( data < 15 || data > 17 ) )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 11 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        ( data < 15 || data > 17 ) )
        {
        return EFalse;
        }
// unsorted end
    item = &( iListModel->Item( 12 ) );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 12 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 13 ) );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 13 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 14 ) );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 14 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 15 ) );
    if( item->GetField( KMultibleSortingTestField1, data ) != KErrNone ||
        data != 0 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 16 ) );
    if( item->GetField( KMultibleSortingTestField1, data ) != KErrNone ||
        data != 1 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 17 ) );
    if( item->GetField( KMultibleSortingTestField1, data ) != KErrNone ||
        data != 2 )
        {
        return EFalse;
        }

    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckMultibleSortingShorting2L
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckMultibleSortingShorting2L()
    {
// check sorted
    const MCLFItem* item = &( iListModel->Item( 0 ) );
    TInt32 data( 0 );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 12 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 1 ) );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 13 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 2 ) );
    if( item->GetField( KMultibleSortingTestField5, data ) != KErrNone ||
        data != 14 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 3 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        data != 15 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 4 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        data != 16 )
        {
        return EFalse;
        }
    item = &( iListModel->Item( 5 ) );
    if( item->GetField( KMultibleSortingTestField6, data ) != KErrNone ||
        data != 17 )
        {
        return EFalse;
        }
// check unsorted
    for( TInt i = 6 ; i < 18 ; ++i )
        {
        item = &( iListModel->Item( i ) );
        if( item->GetField( KMultibleSortingTestField1, data ) != KErrNone )
            {
            if( item->GetField( KMultibleSortingTestField2, data ) != KErrNone )
                {
                if( item->GetField( KMultibleSortingTestField3, data ) != KErrNone )
                    {
                    if( item->GetField( KMultibleSortingTestField4, data ) != KErrNone )
                        {
                        return EFalse;
                        }
                    }
                }
            }
        if( data < 0 || data > 11 )
            {
            return EFalse;
            }
        }

    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckFileDateShortingL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckFileDateShortingL()
    {
    for( TInt i = 0 ; i < iListModel->ItemCount() -1 ; ++i )
        {
        const MCLFItem& item = iListModel->Item( i );
        const MCLFItem& item1 = iListModel->Item( i + 1 );
        TTime date;
        TTime date1;
        if( item.GetField( ECLFFieldIdFileDate, date ) != KErrNone ||
            item1.GetField( ECLFFieldIdFileDate, date1 ) != KErrNone )
            {
            return EFalse;
            }
        if( date > date1 )
            {
            return EFalse;
            }
        }
    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckFileTypesL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckFileTypesL( const MDesCArray& aMimeTypeArray,
                                            const TArray<TInt>& aMediaTypes )
    {
    for( TInt i = 0 ; i < iListModel->ItemCount() ; ++i )
        {
        TPtrC mimeType;
        TInt32 mediaType;
        TInt error = iListModel->Item( i ).GetField( ECLFFieldIdMimeType, mimeType );
        if( iListModel->Item( i ).GetField( ECLFFieldIdMediaType, mediaType ) != KErrNone )
            {
            return EFalse;
            }
        TBool mimeTypeVal( EFalse );
        TBool mediaTypeVal( EFalse );
        if( error == KErrNone )
            {
            mimeTypeVal = CheckMimeTypesL( aMimeTypeArray, mimeType );
            }
        mediaTypeVal = CheckMediaTypesL( aMediaTypes, TCLFMediaType( mediaType ) );
        if( !( mimeTypeVal || mediaTypeVal ) )
            {
            return EFalse;
            }
        }
    return ETrue;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckMimeTypesL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckMimeTypesL( const MDesCArray& aMimeTypeArray,
                                            const TDesC& aMimeType )
    {
    for( TInt j = 0 ; j < aMimeTypeArray.MdcaCount() ; ++j )
        {
        if( aMimeTypeArray.MdcaPoint( j ).Match( aMimeType ) == KErrNotFound )
            {
            return ETrue;
            }
        }
    return EFalse;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CheckMediaTypesL
// ---------------------------------------------------------------------------
//
TBool T_CLFApiModuleTests::CheckMediaTypesL( const TArray<TInt>& aMediaTypes,
                                             TCLFMediaType aMediaType )
    {
    for( TInt j = 0 ; j < aMediaTypes.Count() ; ++j )
        {
        if( aMediaTypes[j] == aMediaType )
            {
            return ETrue;
            }
        }
    return EFalse;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MakeOpaqueDataL
// ---------------------------------------------------------------------------
//
HBufC8* T_CLFApiModuleTests::MakeOpaqueDataL( const MDesCArray& aFiles )
    {
    CDesCArray* folderArray = new ( ELeave ) CDesCArraySeg( 8 );
    CleanupStack::PushL( folderArray );
    TInt count( aFiles.MdcaCount() );
    for( TInt i = 0 ; i < count ; ++i )
        {
        TPtrC folderPath( TParsePtrC(
                            aFiles.MdcaPoint( i ) ).DriveAndPath() );
        TInt tmp( 0 );
        if( folderArray->Find( folderPath, tmp, ECmpFolded ) != 0 )
            {
            folderArray->AppendL( folderPath );
            }
        }

    CBufBase* dynBuffer = CBufFlat::NewL( 64 );
    CleanupStack::PushL( dynBuffer );
    SerializeL( *folderArray, *dynBuffer );
    HBufC8* ret = dynBuffer->Ptr( 0 ).AllocL();
    CleanupStack::PopAndDestroy( 2, folderArray );
    return ret;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MakeMultibleSortingItemsL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::MakeMultibleSortingItemsL(
                        RPointerArray<MCLFModifiableItem>& aItemArray )
    {
    for( TInt i = 0 ; i < 18 ; ++i )
        {
        MCLFModifiableItem* item = ContentListingFactory::NewModifiableItemLC();
        aItemArray.AppendL( item );
        CleanupStack::Pop();

        if( i < 3 )
            {
            item->AddFieldL( KMultibleSortingTestField1, i );
            }
        else if( i < 6 )
            {
            item->AddFieldL( KMultibleSortingTestField2, i );
            }
        else if( i < 9 )
            {
            item->AddFieldL( KMultibleSortingTestField3, i );
            }
        else if( i < 12 )
            {
            item->AddFieldL( KMultibleSortingTestField4, i );
            }
        else if( i < 15 )
            {
            item->AddFieldL( KMultibleSortingTestField5, i );
            }
        else
            {
            item->AddFieldL( KMultibleSortingTestField6, i );
            }
        }
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::FindItem
// ---------------------------------------------------------------------------
//
const MCLFItem* T_CLFApiModuleTests::FindItem( MCLFItemListModel& aModel, TCLFItemId aItemId )
    {
    for( TInt i = 0 ; i < aModel.ItemCount() ; ++i )
        {
        const MCLFItem& item = aModel.Item( i );
        if( item.ItemId() == aItemId )
            {
            return &item;
            }
        }
    return NULL;
    }

/**
 * Setup
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::BaseSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::BaseSetupL()
    {
    iFs.Close();
    iResourceFile.Close();
    
    User::LeaveIfError( iFs.Connect() );
    TFileName fileName( KTestResourceFile );
    BaflUtils::NearestLanguageFile( iFs, fileName );
    iResourceFile.OpenL( iFs, KTestResourceFile );
    iResourceFile.ConfirmSignatureL( 0 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleResourceSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleResourceSetupL()
    {
    BaseSetupL();
    SortingStyleResourceL();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateModelSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateModelSetupL()
    {
    iEngine = ContentListingFactory::NewContentListingEngineLC();
    CleanupStack::Pop();
    iTestObserver  = new (ELeave) TTestOperationObserver;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateModelFromResourceSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateModelFromResourceSetupL()
    {
    BaseSetupL();
    CreateModelSetupL();
    ListModelResourceL();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ListModelSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ListModelSetupL()
    {
    BaseSetupL();
    CreateModelSetupL();
    iListModel = iEngine->CreateListModelLC( *iTestObserver );
    CleanupStack::Pop();

    iSortingStyle = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();
    iSortingStyle1 = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();
    iSortingStyle2 = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();
    iSortingStyle3 = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();

    iTestSorter = new (ELeave) TTestCustomSorter;
    iTestSorter1 = new (ELeave) TTestCustomSorter;
    iTestGrouper = new (ELeave) TTestCustomGrouper;
    iTestGrouper1 = new (ELeave) TTestCustomGrouper;
    iTestFilter = new (ELeave) TTestPostFilter;
    iTestFilter1 = new (ELeave) TTestPostFilter;
    iMimeTypeArray = new (ELeave) CDesCArrayFlat( 8 );
    iMimeTypeArray1 = new (ELeave) CDesCArrayFlat( 8 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MultibleSortingSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::MultibleSortingSetupL()
    {
    ListModelSetupL();
    MakeMultibleSortingItemsL( iModifiableItems );

// use custom grouper to make own items
    iTestGrouper->iModifiableItems = &iModifiableItems;
    iTestGrouper->iCopyItems = ETrue;
    iListModel->SetCustomGrouper( iTestGrouper );

// set sorters
    iSortingStyle->ResetL();
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle->AddFieldL( KMultibleSortingTestField1 );
    iSortingStyle->SetUndefinedItemPosition( ECLFSortingStyleUndefinedFirst );

    iSortingStyle1->ResetL();
    iSortingStyle1->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle1->AddFieldL( KMultibleSortingTestField2 );
    iSortingStyle1->SetUndefinedItemPosition( ECLFSortingStyleUndefinedEnd );
    iSortingStyle1->SetOrdering( ECLFOrderingDescending );

    iSortingStyle2->ResetL();
    iSortingStyle2->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle2->AddFieldL( KMultibleSortingTestField3 );
    iSortingStyle2->AddFieldL( KMultibleSortingTestField4 );
    iSortingStyle2->SetUndefinedItemPosition( ECLFSortingStyleUndefinedEnd );

    iSortingStyle3->ResetL();
    iSortingStyle3->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle3->AddFieldL( KMultibleSortingTestField5 );
    iSortingStyle3->SetUndefinedItemPosition( ECLFSortingStyleUndefinedFirst );

    iListModel->SetSortingStyle( iSortingStyle );
    iListModel->AppendSecondarySortingStyleL( *iSortingStyle1 );
    iListModel->AppendSecondarySortingStyleL( *iSortingStyle2 );
    iListModel->AppendSecondarySortingStyleL( *iSortingStyle3 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MultibleSortingResourceSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::MultibleSortingResourceSetupL()
    {
    ListModelSetupL();
    delete iListModel;
    iListModel = NULL;
    ResourceL( R_LIST_MODEL_MULTIBLE );
    iListModel = iEngine->CreateListModelLC( *iTestObserver, iResourceReader );
    CleanupStack::Pop();

// use custom grouper to make own items
    MakeMultibleSortingItemsL( iModifiableItems );
    iTestGrouper->iModifiableItems = &iModifiableItems;
    iTestGrouper->iCopyItems = ETrue;
    iListModel->SetCustomGrouper( iTestGrouper );

    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ListModelAllFileItemsSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ListModelAllFileItemsSetupL()
    {
    ListModelSetupL();
    iMediaTypeArray.AppendL( ECLFMediaTypeVideo );
    iMediaTypeArray.AppendL( ECLFMediaTypeImage );
    iMediaTypeArray.AppendL( ECLFMediaTypeSound );
    iMediaTypeArray.AppendL( ECLFMediaTypeMusic );
    iMediaTypeArray.AppendL( ECLFMediaTypeStreamingURL );
    iMediaTypeArray.AppendL( ECLFMediaTypePlaylist );
    iListModel->SetWantedMediaTypesL( iMediaTypeArray.Array() );
    iTestObserver->iWait = &iWait;
    iListModel->RefreshL();
    iWait.Start();
    iItemCount = iListModel->ItemCount();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ListModelSetupFromResourceL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ListModelSetupFromResourceL()
    {
    BaseSetupL();
    CreateModelSetupL();
    ListModelResourceL();
    iListModel = iEngine->CreateListModelLC( *iTestObserver, iResourceReader );
    CleanupStack::Pop();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::EngineTestSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::EngineTestSetupL()
    {
    User::LeaveIfError( iFs.Connect() );
    iTestObserver  = new (ELeave) TTestOperationObserver;
    iMimeTypeArray = new (ELeave) CDesCArrayFlat( 8 );
    iEngine = ContentListingFactory::NewContentListingEngineLC();
    CleanupStack::Pop();
    iChangedItemObserver = new (ELeave) TTestChangedItemObserver;
    iChangedItemObserver1 = new (ELeave) TTestChangedItemObserver;
    iTestCLFProcessObserver = new (ELeave) TTestCLFProcessObserver;
    iTestCLFProcessObserver1 = new (ELeave) TTestCLFProcessObserver;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleTestSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleTestSetupL()
    {
    iSortingStyle = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleResourceTestSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleResourceTestSetupL()
    {
    SortingStyleResourceSetupL();
    iSortingStyle1 = ContentListingFactory::NewSortingStyleLC( iResourceReader );
    CleanupStack::Pop();

    ResourceL( R_SORTING_STYLE_EMPTY );
    iSortingStyle = ContentListingFactory::NewSortingStyleLC( iResourceReader );
    CleanupStack::Pop();

    ResourceL( R_SORTING_STYLE_UNDEFINEDITEM );
    iSortingStyle2 = ContentListingFactory::NewSortingStyleLC( iResourceReader );
    CleanupStack::Pop();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ModifiableItemTestSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ModifiableItemTestSetupL()
    {
    iModifiableItem = ContentListingFactory::NewModifiableItemLC();
    CleanupStack::Pop();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ItemTestSetupL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ItemTestSetupL()
    {
    ListModelSetupL();
    CreateNewFileL( 12, iFileName );
    iEngine->UpdateItemsL();
    iMimeTypeArray->Reset();
    iMimeTypeArray->AppendL( _L("*") );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );

    iTestObserver->iWait = &iWait;
    iListModel->RefreshL();
    iWait.Start();

    for( TInt i = 0 ; i < iListModel->ItemCount() ; ++i )
        {
        const MCLFItem& item = iListModel->Item( i );
        TPtrC fn;
        item.GetField( ECLFFieldIdFileNameAndPath, fn );
        if( iFileName.CompareF( fn ) == 0 )
            {
            iItem = &item;
            }
        }

    EUNIT_ASSERT( iItem ); // Item should be in model

    }

/**
 * Teardown
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::Teardown
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::Teardown()
    {
    iResourceFile.Close();
    iFs.Close();
    iMediaTypeArray.Reset();
    iMediaTypeArray.Close();
    iMediaTypeArray1.Reset();
    iMediaTypeArray1.Close();
    iUpdateItemIdArray.Reset();
    iUpdateItemIdArray.Close();
    iChangedArray.Reset();
    iChangedArray.Close();
    iModifiableItems.ResetAndDestroy();
    iModifiableItems.Close();
    
    delete iListModel;
    iListModel = NULL;
    delete iEngine;
    iEngine = NULL;
    delete iSortingStyle;
    iSortingStyle = NULL;
    delete iSortingStyle1;
    iSortingStyle1 = NULL;
    delete iSortingStyle2;
    iSortingStyle2 = NULL;
    delete iSortingStyle3;
    iSortingStyle3 = NULL;
    delete iDataBuffer;
    iDataBuffer = NULL;    
    delete iTestObserver;
    iTestObserver = NULL;
    delete iTestSorter;
    iTestSorter = NULL;
    delete iTestSorter1;
    iTestSorter1 = NULL;
    delete iTestGrouper;
    iTestGrouper = NULL;
    delete iTestGrouper1;
    iTestGrouper1 = NULL;
    delete iTestFilter;
    iTestFilter = NULL;
    delete iTestFilter1;
    iTestFilter1 = NULL;
    delete iMimeTypeArray;
    iMimeTypeArray = NULL;
    delete iMimeTypeArray1;
    iMimeTypeArray1 = NULL;
    delete iChangedItemObserver;
    iChangedItemObserver = NULL;
    delete iChangedItemObserver1;
    iChangedItemObserver1 = NULL;
    delete iOpaqueData;
    iOpaqueData = NULL;
    delete iModifiableItem;
    iModifiableItem = NULL;
    delete iTestCLFProcessObserver1;
    iTestCLFProcessObserver1 = NULL;
    delete iTestCLFProcessObserver;
    iTestCLFProcessObserver = NULL;
    
    TTimeIntervalMicroSeconds32 time = 1000000;
    TRAP_IGNORE( CCLFAsyncCallback::AfterL( time ) );
    }

/**
 * Tests, construction
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateEngineTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateEngineTestL()
    {
    MCLFContentListingEngine* engine = NULL;
    engine = ContentListingFactory::NewContentListingEngineLC();
    EUNIT_ASSERT( engine );
    CleanupStack::PopAndDestroy();
    engine = NULL;
    engine = ContentListingFactory::NewContentListingEngineLC();
    CleanupStack::Pop();
    EUNIT_ASSERT( engine );
    delete engine;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateModifiableItemTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateModifiableItemTestL()
    {
    MCLFModifiableItem* item = NULL;
    item = ContentListingFactory::NewModifiableItemLC();
    EUNIT_ASSERT( item );
    CleanupStack::PopAndDestroy();
    item = NULL;
    item = ContentListingFactory::NewModifiableItemLC();
    CleanupStack::Pop();
    EUNIT_ASSERT( item );
    delete item;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateSortignStyleTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateSortignStyleTestL()
    {
    MCLFSortingStyle* sortingStyle = NULL;
    sortingStyle = ContentListingFactory::NewSortingStyleLC();
    EUNIT_ASSERT( sortingStyle );
    CleanupStack::PopAndDestroy();
    sortingStyle = NULL;
    sortingStyle = ContentListingFactory::NewSortingStyleLC();
    CleanupStack::Pop();
    EUNIT_ASSERT( sortingStyle );
    delete sortingStyle;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateSortignStyleFromResourceTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateSortignStyleFromResourceTestL()
    {
    MCLFSortingStyle* sortingStyle = NULL;
    sortingStyle = ContentListingFactory::NewSortingStyleLC( iResourceReader );
    EUNIT_ASSERT( sortingStyle );
    CleanupStack::PopAndDestroy();
    sortingStyle = NULL;

    SortingStyleResourceL(); // refresh resource reader
    sortingStyle = ContentListingFactory::NewSortingStyleLC( iResourceReader );
    CleanupStack::Pop();
    EUNIT_ASSERT( sortingStyle );
    delete sortingStyle;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateListModelTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateListModelTestL()
    {
    MCLFItemListModel* model = NULL;
    model = iEngine->CreateListModelLC( *iTestObserver );
    EUNIT_ASSERT( model );
    CleanupStack::PopAndDestroy();
    model = NULL;

    model = iEngine->CreateListModelLC( *iTestObserver );
    CleanupStack::Pop();
    EUNIT_ASSERT( model );
    delete model;
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::CreateListModelFromResourceTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::CreateListModelFromResourceTestL()
    {
    MCLFItemListModel* model = NULL;
    model = iEngine->CreateListModelLC( *iTestObserver, iResourceReader );
    EUNIT_ASSERT( model );
    CleanupStack::PopAndDestroy();
    model = NULL;

    ListModelResourceL();
    model = iEngine->CreateListModelLC( *iTestObserver, iResourceReader );
    CleanupStack::Pop();
    EUNIT_ASSERT( model );
    delete model;


    ResourceL( R_LIST_MODEL_INCORRECT_VERSION );
    EUNIT_ASSERT_SPECIFIC_LEAVE( iEngine->CreateListModelLC( *iTestObserver, iResourceReader ), KErrNotSupported );
   }

/**
 * Tests, engine
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::UpdateItemsTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::UpdateItemsTestL()
    {
    // MDS will do the updating automatically in the background,
    // thus only checking that the methods return correctly
    iChangedItemObserver->iWait = &iWait;
    iChangedItemObserver->iChangedArray = &iChangedArray;
    iEngine->AddChangedItemObserverL( *iChangedItemObserver );
    iEngine->AddCLFProcessObserverL( *iTestCLFProcessObserver );
    iEngine->AddCLFProcessObserverL( *iTestCLFProcessObserver1 );

    CreateNewFileL( 0, iFileName );
    CreateNewFileL( 1, iFileName );
    CreateNewFileL( 2, iFileName );
    CreateNewFileL( 3, iFileName );
    CreateNewFileL( 4, iFileName );
    CreateNewFileL( 5, iFileName );

// update server
// to avoid incorrect test result
    CreateNewFileL( 0, iFileName );
    iEngine->UpdateItemsL();

    EUNIT_ASSERT( iChangedItemObserver->iLastError == KErrNone );

    iChangedArray.Reset();
    iEngine->RemoveCLFProcessObserver( *iTestCLFProcessObserver1 );
    iChangedItemObserver->iHandleItemChange = EFalse;
    iChangedItemObserver1->iHandleItemChange = EFalse;
    iTestCLFProcessObserver->Reset();
    iTestCLFProcessObserver1->Reset();
    iEngine->UpdateItemsL();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::UpdateItemsWithIdTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::UpdateItemsWithIdTestL()
    {
    // MDS will do the updating automatically in the background,
    // thus only checking that the methods return correctly
    iChangedItemObserver->iWait = &iWait;
    iChangedItemObserver->iChangedArray = &iChangedArray;
    iEngine->AddChangedItemObserverL( *iChangedItemObserver );
    iEngine->AddChangedItemObserverL( *iChangedItemObserver1 );
    iEngine->AddCLFProcessObserverL( *iTestCLFProcessObserver );

    CreateNewFileL( 0, iFileName );
    CreateNewFileL( 1, iFileName );
    CreateNewFileL( 2, iFileName );

// update server
// to avoid incorrect test result
    CreateNewFileL( 0, iFileName );
    iEngine->UpdateItemsL();
    
    EUNIT_ASSERT( iChangedItemObserver->iLastError == KErrNone );

// start testing
// update by id
    TUint id1 = FindTestFileIdL( 1 );
    TUint id0 = FindTestFileIdL( 0 );
    TUint id2 = FindTestFileIdL( 2 );
    iUpdateItemIdArray.AppendL( id1 );
    iChangedItemObserver->iHandleItemChange = EFalse;
    iChangedItemObserver1->iHandleItemChange = EFalse;
    iTestCLFProcessObserver->Reset();
    iTestCLFProcessObserver1->Reset();
    CreateNewFileL( 0, iFileName );
    CreateNewFileL( 1, iFileName );
    iEngine->UpdateItemsL( iUpdateItemIdArray.Array() );
    
    EUNIT_ASSERT( iChangedItemObserver->iLastError == KErrNone );

    iEngine->RemoveChangedItemObserver( *iChangedItemObserver1 );
    iEngine->AddCLFProcessObserverL( *iTestCLFProcessObserver1 );

    iChangedItemObserver->iHandleItemChange = EFalse;
    iChangedItemObserver1->iHandleItemChange = EFalse;
    iTestCLFProcessObserver->Reset();
    iTestCLFProcessObserver1->Reset();
    iUpdateItemIdArray.AppendL( id0 );
    iUpdateItemIdArray.AppendL( id2 );
    
// update server
// to avoid incorrect test result
    iEngine->UpdateItemsL();
    
    EUNIT_ASSERT( iChangedItemObserver->iLastError == KErrNone );
    
    CreateNewFileL( 0, iFileName );
    CreateNewFileL( 1, iFileName );
    CreateNewFileL( 2, iFileName );
    iChangedArray.Reset();
    
    iEngine->UpdateItemsL( iUpdateItemIdArray.Array() );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::UpdateItemsWithOpaqueDataFolderTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::UpdateItemsWithOpaqueDataFolderTestL()
    {
    // update server
    // to avoid incorrect test result
    iEngine->UpdateItemsL();
    iWait.Start();

    // start testing
    // update by opaque data
    // folders data
    iSemanticId = KCLFUpdateFoldersSemanticId;
    delete iOpaqueData;
    iOpaqueData = NULL;

    CDesCArray* fileArray = new (ELeave) CDesCArraySeg( 8 );
    iOpaqueData = MakeOpaqueDataL( *fileArray );
    // Calls internally same MDS method as when updating all data
    // thus only interested if this call leaves
    iEngine->UpdateItemsL( iSemanticId, *iOpaqueData );
    }

/**
 * Tests, list model
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::RefreshTestL
// ---------------------------------------------------------------------------
//  
void T_CLFApiModuleTests::RefreshTestL()
    {
    iMimeTypeArray->Reset();
    iMimeTypeArray->AppendL( _L("*") );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );

    iTestObserver->iWait = &iWait;
    iTestObserver->iError = 100;
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFRefreshComplete );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() > 0 );

// cancel refresh
    iListModel->RefreshL();
    iListModel->CancelRefresh();
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetSortingStyleTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SetSortingStyleTestL()
    {
    iListModel->SetSortingStyle( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !CheckFileNameShortingL() );

// file name sorting
    iSortingStyle->ResetL();
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeDesC );
    iSortingStyle->AddFieldL( ECLFFieldIdFileName );
    iListModel->SetSortingStyle( iSortingStyle );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( CheckFileNameShortingL() );

// file size sorting
    iSortingStyle1->ResetL();
    iSortingStyle1->SetOrdering( ECLFOrderingDescending );
    iSortingStyle1->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle1->AddFieldL( ECLFFieldIdFileSize );
    iListModel->SetSortingStyle( iSortingStyle1 );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( CheckFileSizeShortingL() );

    iListModel->SetSortingStyle( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( !CheckFileDateShortingL() );

// time sorting
    iSortingStyle->ResetL();
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTTime );
    iSortingStyle->AddFieldL( ECLFFieldIdFileDate );
    iListModel->SetSortingStyle( iSortingStyle );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( CheckFileDateShortingL() );

// parameter test (time)
    iListModel->SetSortingStyle( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( !CheckFileDateShortingL() );

    iListModel->SetSortingStyle( iSortingStyle );
    iListModel->RefreshL( ECLFRefreshPostFilter );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( !CheckFileDateShortingL() );

    iListModel->RefreshL( ECLFRefreshGrouping );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( !CheckFileDateShortingL() );

    iListModel->RefreshL( ECLFRefreshSorting );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( CheckFileDateShortingL() );

// custom sorter (overwrite sorting style)
    iTestSorter->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iTestSorter->iSortItems );
    EUNIT_ASSERT( !CheckFileNameShortingL() );
    EUNIT_ASSERT( !CheckFileSizeShortingL() );
    EUNIT_ASSERT( !CheckFileDateShortingL() );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetCustomSorterTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SetCustomSorterTestL()
    {
    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter1 );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( iTestSorter1->iSortItems );

    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter1 );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( iTestSorter1->iSortItems );

// parameter test
    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );


    iTestSorter->iSortItems = EFalse;
    iTestSorter1->iSortItems = EFalse;

    iListModel->SetCustomSorter( iTestSorter );
    iListModel->RefreshL( ECLFRefreshPostFilter );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iListModel->RefreshL( ECLFRefreshGrouping );
    EUNIT_ASSERT( !iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    iListModel->RefreshL( ECLFRefreshSorting );
    EUNIT_ASSERT( iTestSorter->iSortItems );
    EUNIT_ASSERT( !iTestSorter1->iSortItems );

    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::GroupingTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::GroupingTestL()
    {
    iTestGrouper->iModifiableItems = &iModifiableItems;
    iTestGrouper1->iModifiableItems = &iModifiableItems;

// No grouping
    iListModel->SetCustomGrouper( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );

// couple groups
    iListModel->SetCustomGrouper( iTestGrouper );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iModifiableItems.Count() );

// 0 groups
    iTestGrouper1->iGroupCount = 0;
    iListModel->SetCustomGrouper( iTestGrouper1 );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iModifiableItems.Count() );

// No grouping
    iListModel->SetCustomGrouper( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
// Music album grouping
    iListModel->SetGroupingStyle( ECLFMusicAlbumGrouping );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() != iItemCount );
// No grouping
    iListModel->SetGroupingStyle( ECLFNoGrouping );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );

// test parameters
    iTestGrouper->iGroupCount = 1000;
    iListModel->SetCustomGrouper( iTestGrouper );
    iListModel->RefreshL( ECLFRefreshPostFilter );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
    iListModel->RefreshL( ECLFRefreshSorting );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
    iListModel->RefreshL( ECLFRefreshGrouping );
    EUNIT_ASSERT( iListModel->ItemCount() == iModifiableItems.Count() );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetPostFilterTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SetPostFilterTestL()
    {
// no filter
    iListModel->SetPostFilter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );

// filter couple items
    iListModel->SetPostFilter( iTestFilter );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( ( iListModel->ItemCount() + iTestFilter->iFilteredCount ) == iItemCount );

// filter all items
    iListModel->SetPostFilter( iTestFilter1 );
    iTestFilter1->iAllFilter = ETrue;
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( ( iListModel->ItemCount() + iTestFilter1->iFilteredCount ) == iItemCount );

// no filter
    iListModel->SetPostFilter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );

// filter one item
    iListModel->SetPostFilter( iTestFilter );
    iTestFilter->iShouldFilterCount = 1;
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( ( iListModel->ItemCount() + iTestFilter->iFilteredCount ) == iItemCount );

// filter couple items
    iListModel->SetPostFilter( iTestFilter1 );
    iTestFilter1->iAllFilter = EFalse;
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( ( iListModel->ItemCount() + iTestFilter1->iFilteredCount ) == iItemCount );

// test parameters
    iListModel->SetPostFilter( NULL );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
    iListModel->SetPostFilter( iTestFilter1 );
    iTestFilter1->iAllFilter = ETrue;
    iListModel->RefreshL( ECLFRefreshSorting );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
    iListModel->RefreshL( ECLFRefreshGrouping );
    EUNIT_ASSERT( iListModel->ItemCount() == iItemCount );
    iListModel->RefreshL( ECLFRefreshPostFilter );
    EUNIT_ASSERT( ( iListModel->ItemCount() + iTestFilter1->iFilteredCount ) == iItemCount );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetWantedMimeTypesTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SetWantedMimeTypesTestL()
    {
    iTestObserver->iWait = &iWait;

// list not defined (mimetype)
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// couple mime types
    iMimeTypeArray->Reset();
    iMimeTypeArray->AppendL( _L( "audio/mpeg" ) );
    iMimeTypeArray->AppendL( _L( "audio/aac" ) );
    iMimeTypeArray->AppendL( _L( "audio/mp3" ) );
    iMimeTypeArray->AppendL( _L( "audio/x-mp3" ) );
    iMimeTypeArray->AppendL( _L( "audio/mp4" ) );
    iMimeTypeArray->AppendL( _L( "audio/3gpp" ) );
    iMimeTypeArray->AppendL( _L( "audio/m4a" ) );
    iMimeTypeArray->AppendL( _L( "audio/3gpp2" ) );
    iMimeTypeArray->AppendL( _L( "audio/mpeg4") );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

// unsupported mimetype
    iMimeTypeArray1->Reset();
    iMimeTypeArray1->AppendL( _L("ei tämmöstä ees pitäis olla")  );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray1 );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// empty mimetype list
    iMimeTypeArray1->Reset();
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray1 );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// from resource (mimetype)
    iMimeTypeArray->Reset();
    iMimeTypeArray->AppendL( _L("image/*")  );
    iMimeTypeArray->AppendL( _L("audio/*")  );

    ResourceL( R_MIME_TYPE_ARRAY );
    iListModel->SetWantedMimeTypesL( iResourceReader );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

    ResourceL( R_MIME_TYPE_ARRAY_EMPTY );
    iListModel->SetWantedMimeTypesL( iResourceReader );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// incorrect resource version
    ResourceL( R_MIME_TYPE_ARRAY_INCORRECT_VERSION );
    EUNIT_ASSERT_SPECIFIC_LEAVE( iListModel->SetWantedMimeTypesL( iResourceReader ), KErrNotSupported );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetWantedMediaTypesTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SetWantedMediaTypesTestL()
    {
    iTestObserver->iWait = &iWait;

// list not defined (mediatype)
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// couple media types
    iMediaTypeArray.AppendL( ECLFMediaTypeVideo );
    iMediaTypeArray.AppendL( ECLFMediaTypeImage );
    iMediaTypeArray.AppendL( ECLFMediaTypeSound );
    iMediaTypeArray.AppendL( ECLFMediaTypeMusic );
    iMediaTypeArray.AppendL( ECLFMediaTypeStreamingURL );
    iMediaTypeArray.AppendL( ECLFMediaTypePlaylist );
    iMediaTypeArray.AppendL( TCLFMediaType( ECLFMediaTypeCollection ) );

    iListModel->SetWantedMediaTypesL( iMediaTypeArray.Array() );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

// empty media type list list
    iListModel->SetWantedMediaTypesL( iMediaTypeArray1.Array() );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// media type list from resource
    iMediaTypeArray.Reset();
    iMediaTypeArray.AppendL( ECLFMediaTypeImage );
    iMediaTypeArray.AppendL( TCLFMediaType( ECLFMediaTypeCollection ) );
    ResourceL( R_MEDIA_TYPE_ARRAY );
    iListModel->SetWantedMediaTypesL( iResourceReader );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

// empty media type list from resource
    ResourceL( R_MEDIA_TYPE_ARRAY_EMPTY );
    iListModel->SetWantedMediaTypesL( iResourceReader );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SetWantedMediaAndMimeTypesTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::SetWantedMediaAndMimeTypesTestL()
    {
    iTestObserver->iWait = &iWait;

    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );

// couple types
    iMimeTypeArray->AppendL( _L( "audio/mpeg" ) );
    iMimeTypeArray->AppendL( _L( "audio/aac" ) );
    iMimeTypeArray->AppendL( _L( "audio/mp3" ) );
    iMimeTypeArray->AppendL( _L( "audio/x-mp3" ) );
    iMimeTypeArray->AppendL( _L( "audio/mp4" ) );
    iMimeTypeArray->AppendL( _L( "audio/3gpp" ) );
    iMimeTypeArray->AppendL( _L( "audio/m4a" ) );
    iMimeTypeArray->AppendL( _L( "audio/3gpp2" ) );
    iMimeTypeArray->AppendL( _L( "audio/mpeg4") );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );

    iMediaTypeArray.AppendL( ECLFMediaTypeVideo );
    iListModel->SetWantedMediaTypesL( iMediaTypeArray.Array() );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

// refresh again
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( CheckFileTypesL( *iMimeTypeArray, iMediaTypeArray.Array() ) );

// empty lists
    iMediaTypeArray.Reset();
    iMimeTypeArray->Reset();
    iListModel->SetWantedMediaTypesL( iMediaTypeArray.Array() );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );
    iListModel->RefreshL();
    iWait.Start();
    EUNIT_ASSERT( iListModel->ItemCount() == 0 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MultibleSortingTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::MultibleSortingTestL()
    {

    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( CheckMultibleSortingShortingL() );

// resort

    iSortingStyle->ResetL();
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTInt32 );
    iSortingStyle->AddFieldL( KMultibleSortingTestField6 );
    iSortingStyle->AddFieldL( KMultibleSortingTestField5 );
    iSortingStyle->SetUndefinedItemPosition( ECLFSortingStyleUndefinedEnd );
    iListModel->SetSortingStyle( iSortingStyle );
    iListModel->RefreshL( ECLFRefreshAll );
    EUNIT_ASSERT( CheckMultibleSortingShorting2L() );

    }

/*
* Test model item(s) obsolate functionality
*/

// ---------------------------------------------------------------------------
// ModelItemsChangedTestL
// ---------------------------------------------------------------------------
//
void T_CLFApiModuleTests::ModelItemsChangedTestL()
    {
    const TInt newFileNumber( 10 );

// create test files
    CreateNewFileL( 0, iFileName );
    CreateNewFileL( 1, iFileName );
    CreateNewFileL( 2, iFileName );
    CreateNewFileL( newFileNumber, iFileName );
    User::LeaveIfError( iFs.Delete( iFileName ) );

// update server
// to avoid incorrect test result
    CreateNewFileL( 0, iFileName );
    iEngine->UpdateItemsL();

// create list model with all files
    iMimeTypeArray->Reset();
    iMimeTypeArray->AppendL( _L("*") );
    iListModel->SetWantedMimeTypesL( *iMimeTypeArray );

    iTestObserver->iWait = &iWait;
    iTestObserver->iError = 100;
    iListModel->RefreshL();
    iWait.Start();  // wait until model is refreshed
    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFRefreshComplete );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() > 0 );

    const TInt listModelItemCount( iListModel->ItemCount() );
    const TCLFItemId testId( FindTestFileIdL( 0 ) );

// test with modified item
    CreateNewFileL( 0, iFileName );
    iEngine->UpdateItemsL();
    iWait.Start(); // wait until model outdated event is received

    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFModelOutdated );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount );
    EUNIT_ASSERT( NULL != FindItem( *iListModel, testId ) );

    iTestObserver->iError = 100;
    iListModel->RefreshL();
    iWait.Start();  // wait until model is refreshed
    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFRefreshComplete );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount );
    EUNIT_ASSERT( NULL != FindItem( *iListModel, testId ) );

// test with new item
    CreateNewFileL( newFileNumber, iFileName );
    iEngine->UpdateItemsL();
    iWait.Start(); // wait until model outdated event is received

    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFModelOutdated );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount );

    iTestObserver->iError = 100;
    iListModel->RefreshL();
    iWait.Start();  // wait until model is refreshed
    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFRefreshComplete );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount + 1 );

// delete file
    User::LeaveIfError( iFs.Delete( iFileName ) );
    iEngine->UpdateItemsL();
    iWait.Start(); // wait until model outdated event is received

    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFModelOutdated );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount + 1 );

    iTestObserver->iError = 100;
    iListModel->RefreshL();
    iWait.Start();  // wait until model is refreshed
    EUNIT_ASSERT( iTestObserver->iOperationEvent == ECLFRefreshComplete );
    EUNIT_ASSERT( iTestObserver->iError == KErrNone );
    EUNIT_ASSERT( iListModel->ItemCount() == listModelItemCount );
    }

/**
 * Tests, Modifiable item
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::MIFieldTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::MIFieldTestL()
    {
    const TUint KTestField1 = 1;
    const TUint KTestField2 = 112312312;
    const TUint KTestField3 = 13333;
    const TUint KTestFieldIncorrect = 2;

    TTime time( 100 );
    TInt32 integer( 12 );
    TBuf<30> data( _L("data") );
    iModifiableItem->AddFieldL( KTestField1, time );
    iModifiableItem->AddFieldL( KTestField2, integer );
    iModifiableItem->AddFieldL( KTestField3, data );

// data type test
    EUNIT_ASSERT( iModifiableItem->DataType(
                                KTestField1 ) == ECLFItemDataTypeTTime );
    EUNIT_ASSERT( iModifiableItem->DataType(
                                KTestField2 ) == ECLFItemDataTypeTInt32 );
    EUNIT_ASSERT( iModifiableItem->DataType(
                                KTestField3 ) == ECLFItemDataTypeDesC );
    EUNIT_ASSERT( iModifiableItem->DataType(
                                KTestFieldIncorrect ) == ECLFItemDataTypeNull );
    EUNIT_ASSERT( iModifiableItem->DataType(
                                ECLFFieldIdNull ) == ECLFItemDataTypeNull );

// get field
    TTime time1( 0 );
    TInt32 integer1( 0 );
    TPtrC ptr;
    EUNIT_ASSERT( KErrNone == iModifiableItem->GetField(
                                                KTestField1, time1 ) );
    EUNIT_ASSERT( time == time1 );
    EUNIT_ASSERT( KErrNone == iModifiableItem->GetField(
                                                KTestField2, integer1 ) );
    EUNIT_ASSERT( integer == integer1 );
    EUNIT_ASSERT( KErrNone == iModifiableItem->GetField(
                                                KTestField3, ptr ) );
    EUNIT_ASSERT( data == ptr );

// incorrect field id
    EUNIT_ASSERT( KErrNotFound == iModifiableItem->GetField(
                                                KTestFieldIncorrect, ptr ) );
    EUNIT_ASSERT( KErrNotFound == iModifiableItem->GetField(
                                                KTestFieldIncorrect, integer1 ) );
    EUNIT_ASSERT( KErrNotFound == iModifiableItem->GetField(
                                                KTestFieldIncorrect, time1 ) );

// incorrect field type
    EUNIT_ASSERT( KErrNotSupported == iModifiableItem->GetField(
                                                KTestField1, ptr ) );
    EUNIT_ASSERT( KErrNotSupported == iModifiableItem->GetField(
                                                KTestField3, integer1 ) );
    EUNIT_ASSERT( KErrNotSupported == iModifiableItem->GetField(
                                                KTestField2, time1 ) );


    EUNIT_ASSERT( iModifiableItem->ItemId() == 0 );
    }

/**
 * Tests, item
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::ItemFieldTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::ItemFieldTestL()
    {
// data type test
    EUNIT_ASSERT( iItem->DataType(
                                ECLFFieldIdFileDate ) == ECLFItemDataTypeTTime );
    EUNIT_ASSERT( iItem->DataType(
                                ECLFFieldIdFileSize ) == ECLFItemDataTypeTInt32 );
    EUNIT_ASSERT( iItem->DataType(
                                ECLFFieldIdFileNameAndPath ) == ECLFItemDataTypeDesC );
    EUNIT_ASSERT( iItem->DataType(
                                ECLFFieldIdNull ) == ECLFItemDataTypeNull );

// get field
    TTime time1( 0 );
    TInt32 integer1( 0 );
    TPtrC ptr;
    TEntry entry;
    User::LeaveIfError( iFs.Entry( iFileName, entry ) );

    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdFileDate, time1 ) );
    EUNIT_ASSERT( entry.iModified == time1 );
    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdFileSize, integer1 ) );
    EUNIT_ASSERT( entry.iSize == integer1 );
    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdFileNameAndPath, ptr ) );
    EUNIT_ASSERT( iFileName == ptr );

    TParsePtrC parse( iFileName );

    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdFileExtension, ptr ) );
    EUNIT_ASSERT( parse.Ext() == ptr );

    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdFileName, ptr ) );
    EUNIT_ASSERT( parse.Name() == ptr );

    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdPath, ptr ) );
    EUNIT_ASSERT( parse.Path() == ptr );

    EUNIT_ASSERT( KErrNone == iItem->GetField( ECLFFieldIdDrive, ptr ) );
    EUNIT_ASSERT( parse.Drive() == ptr );


// incorrect field id
    EUNIT_ASSERT( KErrNotFound == iItem->GetField(
                                                ECLFFieldIdNull, ptr ) );
    EUNIT_ASSERT( KErrNotFound == iItem->GetField(
                                                ECLFFieldIdNull, integer1 ) );
    EUNIT_ASSERT( KErrNotFound == iItem->GetField(
                                                ECLFFieldIdNull, time1 ) );

// incorrect field type
    EUNIT_ASSERT( KErrNotSupported == iItem->GetField(
                                                ECLFFieldIdFileSize, ptr ) );
    EUNIT_ASSERT( KErrNotSupported == iItem->GetField(
                                                ECLFFieldIdFileDate, integer1 ) );
    EUNIT_ASSERT( KErrNotSupported == iItem->GetField(
                                                ECLFFieldIdFileNameAndPath, time1 ) );


    EUNIT_ASSERT( iItem->ItemId() != 0 );
    }

/**
 * Tests, Sorting style
 */

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleResourceTestL
// ---------------------------------------------------------------------------
//  
void T_CLFApiModuleTests::SortingStyleResourceTestL()
    {
    const TInt KArray1IdCount( 4 );
    const TInt KArray2IdCount( 1 );


    RArray<TCLFItemId> itemIdArray;
    CleanupClosePushL( itemIdArray );

    EUNIT_ASSERT( iSortingStyle->Ordering() == ECLFOrderingAscending );
    EUNIT_ASSERT( iSortingStyle1->Ordering() == ECLFOrderingDescending );
    EUNIT_ASSERT( iSortingStyle2->Ordering() == ECLFOrderingDescending );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeTInt32 );
    EUNIT_ASSERT( iSortingStyle1->SortingDataType() == ECLFItemDataTypeDesC );
    EUNIT_ASSERT( iSortingStyle2->SortingDataType() == ECLFItemDataTypeDesC );

    iSortingStyle->GetFieldsL( itemIdArray );

    EUNIT_ASSERT( itemIdArray.Count() == 0 );

    itemIdArray.Reset();
    iSortingStyle1->GetFieldsL( itemIdArray );

    EUNIT_ASSERT( itemIdArray.Count() == KArray1IdCount );

    itemIdArray.Reset();
    iSortingStyle2->GetFieldsL( itemIdArray );

    EUNIT_ASSERT( itemIdArray.Count() == KArray2IdCount );

    CleanupStack::PopAndDestroy( &itemIdArray ); // itemIdArray.Close
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleOrderingTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleOrderingTestL()
    {
    iSortingStyle->SetOrdering( ECLFOrderingAscending );
    EUNIT_ASSERT( iSortingStyle->Ordering() == ECLFOrderingAscending );
    iSortingStyle->SetOrdering( ECLFOrderingDescending );
    EUNIT_ASSERT( iSortingStyle->Ordering() == ECLFOrderingDescending );
    iSortingStyle->SetOrdering( ECLFOrderingAscending );
    EUNIT_ASSERT( iSortingStyle->Ordering() == ECLFOrderingAscending );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleDataTypeTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleDataTypeTestL()
    {
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTInt32 );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeTInt32 );
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeDesC );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeDesC );
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTTime );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeTTime );
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeNull );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeNull );
    iSortingStyle->SetSortingDataType( ECLFItemDataTypeTInt32 );
    EUNIT_ASSERT( iSortingStyle->SortingDataType() == ECLFItemDataTypeTInt32 );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleUndefinedItemPositionTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleUndefinedItemPositionTestL()
    {
    iSortingStyle->SetUndefinedItemPosition( ECLFSortingStyleUndefinedEnd );
    EUNIT_ASSERT( iSortingStyle->UndefinedItemPosition() == ECLFSortingStyleUndefinedEnd );
    iSortingStyle->SetUndefinedItemPosition( ECLFSortingStyleUndefinedFirst );
    EUNIT_ASSERT( iSortingStyle->UndefinedItemPosition() == ECLFSortingStyleUndefinedFirst );
    iSortingStyle->SetUndefinedItemPosition( ECLFSortingStyleUndefinedEnd );
    EUNIT_ASSERT( iSortingStyle->UndefinedItemPosition() == ECLFSortingStyleUndefinedEnd );
    }

// ---------------------------------------------------------------------------
// T_CLFApiModuleTests::SortingStyleFieldTestL
// ---------------------------------------------------------------------------
// 
void T_CLFApiModuleTests::SortingStyleFieldTestL()
    {
    RArray<TCLFItemId> itemIdArray;
    CleanupClosePushL( itemIdArray );

    iSortingStyle->GetFieldsL( itemIdArray );
    EUNIT_ASSERT( itemIdArray.Count() == 0 );

    iSortingStyle->AddFieldL( ECLFFieldIdFileName );
    iSortingStyle->AddFieldL( ECLFFieldIdCollectionId );
    iSortingStyle->AddFieldL( ECLFFieldIdCollectionName );
    iSortingStyle->AddFieldL( ECLFFieldIdArtist );

    iSortingStyle->GetFieldsL( itemIdArray );

    EUNIT_ASSERT( itemIdArray.Count() == 4 );

    itemIdArray.Reset();
    iSortingStyle->ResetL();
    iSortingStyle->GetFieldsL( itemIdArray );
    EUNIT_ASSERT( itemIdArray.Count() == 0 );

    CleanupStack::PopAndDestroy( &itemIdArray ); // itemIdArray.Close
    }

// ---------------------------------------------------------------------------
// Test case table for this test suite class
// ---------------------------------------------------------------------------
// 

EUNIT_BEGIN_TEST_TABLE( T_CLFApiModuleTests, "T_CLFApiModuleTests", "MODULE" )

// Constructor tests
    EUNIT_TEST( "Create engine",
                "",
                "",
                "FUNCTIONALITY",
                BaseSetupL,
                CreateEngineTestL,
                Teardown )

    EUNIT_TEST( "Create modifiable item",
                "",
                "",
                "FUNCTIONALITY",
                BaseSetupL,
                CreateModifiableItemTestL,
                Teardown )

    EUNIT_TEST( "Create sorting style",
                "",
                "",
                "FUNCTIONALITY",
                BaseSetupL,
                CreateSortignStyleTestL,
                Teardown )

    EUNIT_TEST( "Create sorting style from resource",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleResourceSetupL,
                CreateSortignStyleFromResourceTestL,
                Teardown )

    EUNIT_TEST( "Create list model",
                "",
                "",
                "FUNCTIONALITY",
                CreateModelSetupL,
                CreateListModelTestL,
                Teardown )

    EUNIT_TEST( "Create list model from resource",
                "",
                "",
                "FUNCTIONALITY",
                CreateModelFromResourceSetupL,
                CreateListModelFromResourceTestL,
                Teardown )

// Engine tests

    EUNIT_TEST( "Engine update test",
                "",
                "",
                "FUNCTIONALITY",
                EngineTestSetupL,
                UpdateItemsTestL,
                Teardown )

    EUNIT_TEST( "Engine update test",
                "",
                "",
                "FUNCTIONALITY",
                EngineTestSetupL,
                UpdateItemsWithIdTestL,
                Teardown )

    EUNIT_TEST( "Engine update test",
                "",
                "",
                "FUNCTIONALITY",
                EngineTestSetupL,
                UpdateItemsWithOpaqueDataFolderTestL,
                Teardown )

// Sorting Style tests
    EUNIT_TEST( "Sorting style from resource",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleResourceTestSetupL,
                SortingStyleResourceTestL,
                Teardown )

    EUNIT_TEST( "Sorting style ordering test",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleTestSetupL,
                SortingStyleOrderingTestL,
                Teardown )

    EUNIT_TEST( "Sorting style data type test",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleTestSetupL,
                SortingStyleDataTypeTestL,
                Teardown )

    EUNIT_TEST( "Sorting style undefined item position test",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleTestSetupL,
                SortingStyleUndefinedItemPositionTestL,
                Teardown )

    EUNIT_TEST( "Sorting style field test",
                "",
                "",
                "FUNCTIONALITY",
                SortingStyleTestSetupL,
                SortingStyleFieldTestL,
                Teardown )

// List model tests
    EUNIT_TEST( "List model refresh test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelSetupL,
                RefreshTestL,
                Teardown )

    EUNIT_TEST( "List model sorting style test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelAllFileItemsSetupL,
                SetSortingStyleTestL,
                Teardown )

    EUNIT_TEST( "List model custom sorter test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelAllFileItemsSetupL,
                SetCustomSorterTestL,
                Teardown )

    EUNIT_TEST( "List model grouping test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelAllFileItemsSetupL,
                GroupingTestL,
                Teardown )

    EUNIT_TEST( "List model post filter test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelAllFileItemsSetupL,
                SetPostFilterTestL,
                Teardown )

    EUNIT_TEST( "List model wanted mime types test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelSetupL,
                SetWantedMimeTypesTestL,
                Teardown )

    EUNIT_TEST( "List model wanted media types test",
                "",
                "",
                "FUNCTIONALITY",
                ListModelSetupL,
                SetWantedMediaTypesTestL,
                Teardown )

    EUNIT_TEST( "List model wanted media and mime types",
                "",
                "",
                "FUNCTIONALITY",
                ListModelSetupL,
                SetWantedMediaAndMimeTypesTestL,
                Teardown )

    EUNIT_TEST( "List model multible sorters",
                "",
                "",
                "FUNCTIONALITY",
                MultibleSortingSetupL,
                MultibleSortingTestL,
                Teardown )

    EUNIT_TEST( "List model multible sorters",
                "",
                "",
                "FUNCTIONALITY",
                MultibleSortingResourceSetupL,
                MultibleSortingTestL,
                Teardown )

    EUNIT_TEST( "List model changed items",
                "",
                "",
                "FUNCTIONALITY",
                ListModelSetupL,
                ModelItemsChangedTestL,
                Teardown )


// Modifiable item tests
    EUNIT_TEST( "Modifiable item test",
                "",
                "",
                "FUNCTIONALITY",
                ModifiableItemTestSetupL,
                MIFieldTestL,
                Teardown )

// Item tests
    EUNIT_TEST( "Item test",
                "",
                "",
                "FUNCTIONALITY",
                ItemTestSetupL,
                ItemFieldTestL,
                Teardown )


EUNIT_END_TEST_TABLE

//  End of File
