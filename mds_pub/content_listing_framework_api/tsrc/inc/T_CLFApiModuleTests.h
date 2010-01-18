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
* Description:  Part of CLF API tests
*
*/


// INCLUDES
#include <ceunittestsuiteclass.h>
#include <bautils.h>
#include <barsc.h>
#include <CLFContentListing.hrh>
#include <CLFContentListing.h>

// FORWARD DECLARATION
class MCLFContentListingEngine;
class MCLFItemListModel;
class MCLFSortingStyle;
class TTestOperationObserver;
class TTestCustomSorter;
class TTestCustomGrouper;
class TTestPostFilter;
class TTestChangedItemObserver;
class MCLFModifiableItem;
class TTestCLFProcessObserver;
class MCLFItem;

/**
 * Test suite for Content Listing Framework API
 */
class T_CLFApiModuleTests : public CEUnitTestSuiteClass
    {
    public:     // Construct / destruct
        static T_CLFApiModuleTests* NewLC();
        ~T_CLFApiModuleTests();

    private:
        void ConstructL();

    private:    // Assistance methods
        void ResourceL( TInt aResourceId );
        void SortingStyleResourceL();
        void ListModelResourceL();
        void CreateNewFileL( TInt aNumber, TDes& aFileName );
        TCLFItemId FindTestFileIdL( TInt aNumber );
        TBool CheckFileNameShortingL();
        TBool CheckFileSizeShortingL();
        TBool CheckFileDateShortingL();
        TBool CheckFileTypesL( const MDesCArray& aMimeTypeArray, const TArray<TInt>& aMediaTypes );
        TBool CheckMimeTypesL( const MDesCArray& aMimeTypeArray, const TDesC& aMimeType );
        TBool CheckMediaTypesL( const TArray<TInt>& aMediaTypes, TCLFMediaType aMediaType );
        HBufC8* MakeOpaqueDataL( const MDesCArray& aFiles );
        void MakeMultibleSortingItemsL( RPointerArray<MCLFModifiableItem>& aItemArray );
        TBool CheckMultibleSortingShortingL();
        TBool CheckMultibleSortingShorting2L();
        const MCLFItem* FindItem( MCLFItemListModel& aModel, TCLFItemId aItemId );


    private:    // test methods
        // setups
        void BaseSetupL();
        void SortingStyleResourceSetupL();
        void CreateModelSetupL();
        void CreateModelFromResourceSetupL();
        void ListModelSetupL();
        void ListModelSetupFromResourceL();
        void ListModelAllFileItemsSetupL();
        void EngineTestSetupL();
        void SortingStyleTestSetupL();
        void SortingStyleResourceTestSetupL();
        void ModifiableItemTestSetupL();
        void ItemTestSetupL();
        void MultibleSortingSetupL();
        void MultibleSortingResourceSetupL();

        // teardowns
        void Teardown();

        // tests
        // Constructor test
        void CreateEngineTestL();
        void CreateModifiableItemTestL();
        void CreateSortignStyleTestL();
        void CreateSortignStyleFromResourceTestL();
        void CreateListModelTestL();
        void CreateListModelFromResourceTestL();

        // Engine test
        void UpdateItemsTestL();
        void UpdateItemsWithIdTestL();
        void UpdateItemsWithOpaqueDataFolderTestL();

        // Sorting Style test;
        void SortingStyleResourceTestL();
        void SortingStyleOrderingTestL();
        void SortingStyleDataTypeTestL();
        void SortingStyleUndefinedItemPositionTestL();
        void SortingStyleFieldTestL();

        // List model test
        void RefreshTestL();
        void SetSortingStyleTestL();
        void SetCustomSorterTestL();
        void GroupingTestL();
        void SetPostFilterTestL();
        void SetWantedMimeTypesTestL();
        void SetWantedMediaTypesTestL();
        void SetWantedMediaAndMimeTypesTestL();
        void MultibleSortingTestL();
        void ModelItemsChangedTestL();


        // item test
        void ItemFieldTestL();

        // Modifiable item test
        void MIFieldTestL();

    private:    // Implementation

        EUNIT_DECLARE_TEST_TABLE;

    private:    // Data
        MCLFContentListingEngine* iEngine;
        MCLFItemListModel* iListModel;
        MCLFSortingStyle* iSortingStyle;
        MCLFSortingStyle* iSortingStyle1;
        MCLFSortingStyle* iSortingStyle2;
        MCLFSortingStyle* iSortingStyle3;
        MCLFModifiableItem* iModifiableItem;
        const MCLFItem* iItem; // ref. not owned

        TTestOperationObserver* iTestObserver;
        TTestCustomSorter* iTestSorter;
        TTestCustomSorter* iTestSorter1;
        TTestCustomGrouper* iTestGrouper;
        TTestCustomGrouper* iTestGrouper1;
        TTestPostFilter* iTestFilter;
        TTestPostFilter* iTestFilter1;
        TTestCLFProcessObserver* iTestCLFProcessObserver;
        TTestCLFProcessObserver* iTestCLFProcessObserver1;

        CDesCArray* iMimeTypeArray;
        CDesCArray* iMimeTypeArray1;
        RArray<TInt> iMediaTypeArray;
        RArray<TInt> iMediaTypeArray1;

        TTestChangedItemObserver* iChangedItemObserver;
        TTestChangedItemObserver* iChangedItemObserver1;
        RArray<TCLFItemId> iUpdateItemIdArray;
        HBufC8* iOpaqueData;
        TInt iSemanticId;

        RFs iFs;
        RResourceFile iResourceFile;
        HBufC8* iDataBuffer;
        TResourceReader iResourceReader;
        CActiveSchedulerWait iWait;
        RArray<TCLFItemId> iChangedArray;
        TInt iItemCount;
        RPointerArray<MCLFModifiableItem> iModifiableItems;
        TFileName iFileName;

    };

// End of file
