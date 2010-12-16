/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
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

#include "mdsdatabaseupdater.h"
#include "mdssqliteconnection.h"
#include "mdssqldbmaintenance.h"
#include "mdccommon.h"
#include "mdspreferences.h"
#include "mdeconstants.h"
#include "mdcserializationbuffer.h"

CMdSDatabaseUpdater* CMdSDatabaseUpdater::NewL()
    {
    CMdSDatabaseUpdater* self = new ( ELeave ) CMdSDatabaseUpdater();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;  
    }

void CMdSDatabaseUpdater::ConstructL()
    { 
    }

CMdSDatabaseUpdater::~CMdSDatabaseUpdater()
    {
    }

TBool CMdSDatabaseUpdater::UpdateDatabaseL( TInt64 aMinorVersion )
    {
    if( KMdSServMinorVersionNumber == 6 &&
        aMinorVersion == 5 )
        {
        // Add TagType column to Tag table
        _LIT( KAddTagTypeColumn, "ALTER TABLE Tag%u ADD COLUMN TagType LARGEINT" );
        
        RRowData data;
        CleanupClosePushL( data );
        
        RBuf commonClauseOne;
        User::LeaveIfError( commonClauseOne.Create( KAddTagTypeColumn.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseOne ); 
        commonClauseOne.Format( KAddTagTypeColumn, KDefaultNamespaceDefId );  
        
        CMdSSqLiteConnection& connection = MMdSDbConnectionPool::GetDefaultDBL();
        
        TRAPD( err, connection.ExecuteL( commonClauseOne, data ) );

        CleanupStack::PopAndDestroy( &commonClauseOne );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }
        
        // Add TagType property to PropertyDef table
        _LIT( KMdsSqlClauseAddPropertyDef, "INSERT INTO PropertyDef(ObjectDefId,Flags,Type,MinValue,MaxValue,Name) Values(?,?,?,?,?,?);" );

        TDefId objectDefId( 7 );
        TUint32 flags( 0 );
        TPropertyType type = EPropertyMask;
        TUint32 minValue = 0;
        TUint32 maxValue = KMaxTUint32;
        HBufC16* name = MdeConstants::Tag::KTagType().AllocL();
        
        RRowData tagRowData;
        CleanupClosePushL( tagRowData );
        tagRowData.AppendL( TColumn( objectDefId ) );
        tagRowData.AppendL( TColumn( flags ) );
        tagRowData.AppendL( TColumn( type ) );
        tagRowData.AppendL( TColumn( minValue ) );
        tagRowData.AppendL( TColumn( maxValue ) );
        tagRowData.AppendL( TColumn( name ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddPropertyDef, tagRowData ) );
        
        CleanupStack::PopAndDestroy( &tagRowData );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Update Col2Prop table with new property
        _LIT( KMdsSqlClauseAddCol2Prop, "INSERT INTO Col2Prop(ObjectDefId,PropertyDefId,ColumnId) Values(?,?,?);" );
        objectDefId = 7;
        TDefId propertyDefId( 120 );
        TInt32 columnId( 17 );
        
        RRowData tagCol2propRow;
        CleanupClosePushL( tagCol2propRow );
        tagCol2propRow.AppendL( TColumn( objectDefId ) );
        tagCol2propRow.AppendL( TColumn( propertyDefId ) );
        tagCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, tagCol2propRow ) );
        
        CleanupStack::PopAndDestroy( &tagCol2propRow );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Add RightsStatus column to MediaObject table
        _LIT( KAddDrmRightsStatusColumn, "ALTER TABLE MediaObject%u ADD COLUMN RightsStatus INTEGER" );
        
        RBuf commonClauseTwo;
        User::LeaveIfError( commonClauseTwo.Create( KAddDrmRightsStatusColumn.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseTwo ); 
        commonClauseTwo.Format( KAddDrmRightsStatusColumn, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseTwo, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseTwo );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Add RightsStatus column to Videos table
        _LIT( KAddDrmRightsStatusColumnVideos, "ALTER TABLE Video%u ADD COLUMN RightsStatus INTEGER" );
        
        RBuf commonClauseThree;
        User::LeaveIfError( commonClauseThree.Create( KAddDrmRightsStatusColumnVideos.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseThree ); 
        commonClauseThree.Format( KAddDrmRightsStatusColumnVideos, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseThree, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseThree );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }
        
        // Add RightsStatus column to Images table
        _LIT( KAddDrmRightsStatusColumnImages, "ALTER TABLE Image%u ADD COLUMN RightsStatus INTEGER" );
        
        RBuf commonClauseFour;
        User::LeaveIfError( commonClauseFour.Create( KAddDrmRightsStatusColumnImages.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseFour ); 
        commonClauseFour.Format( KAddDrmRightsStatusColumnImages, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseFour, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseFour );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }
        
        // Add RightsStatus column to Audios table
        _LIT( KAddDrmRightsStatusColumnAudio, "ALTER TABLE Audio%u ADD COLUMN RightsStatus INTEGER" );
        
        RBuf commonClauseFive;
        User::LeaveIfError( commonClauseFive.Create( KAddDrmRightsStatusColumnAudio.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseFive ); 
        commonClauseFive.Format( KAddDrmRightsStatusColumnAudio, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseFive, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseFive );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Add RightsStatus property to PropertyDef table
        objectDefId = 8;
        type = EPropertyUint16;
        TInt32 minValue2 = 0;
        TInt32 maxValue2 = KMaxTUint16;
        HBufC16* name2 = MdeConstants::MediaObject::KRightsStatus().AllocL();
        
        RRowData drmRowData;
        CleanupClosePushL( drmRowData );
        drmRowData.AppendL( TColumn( objectDefId ) );
        drmRowData.AppendL( TColumn( flags ) );
        drmRowData.AppendL( TColumn( type ) );
        drmRowData.AppendL( TColumn( minValue2 ) );
        drmRowData.AppendL( TColumn( maxValue2 ) );
        drmRowData.AppendL( TColumn( name2 ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddPropertyDef, drmRowData ) );
        
        CleanupStack::PopAndDestroy( &drmRowData );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Update Col2Prop table with new property       
        objectDefId = 8;
        propertyDefId = 121;
        columnId = 43;
        
        RRowData rightsCol2propRow;
        CleanupClosePushL( rightsCol2propRow );
        rightsCol2propRow.AppendL( TColumn( objectDefId ) );
        rightsCol2propRow.AppendL( TColumn( propertyDefId ) );
        rightsCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, rightsCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &rightsCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }  

        rightsCol2propRow.Reset();
        
        objectDefId = 9;
        propertyDefId = 121;
        columnId = 48;

        rightsCol2propRow.AppendL( TColumn( objectDefId ) );
        rightsCol2propRow.AppendL( TColumn( propertyDefId ) );
        rightsCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, rightsCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &rightsCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            } 
        
        rightsCol2propRow.Reset();
        
        objectDefId = 10;
        propertyDefId = 121;
        columnId = 84;

        rightsCol2propRow.AppendL( TColumn( objectDefId ) );
        rightsCol2propRow.AppendL( TColumn( propertyDefId ) );
        rightsCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, rightsCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &rightsCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }         
        
        rightsCol2propRow.Reset();
        
        objectDefId = 11;
        propertyDefId = 121;
        columnId = 48;

        rightsCol2propRow.AppendL( TColumn( objectDefId ) );
        rightsCol2propRow.AppendL( TColumn( propertyDefId ) );
        rightsCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, rightsCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &rightsCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }          
        
        CleanupStack::PopAndDestroy( &rightsCol2propRow );

        // Add ContentID column to MediaObject table
        _LIT( KAddContentIDColumn, "ALTER TABLE MediaObject%u ADD COLUMN ContentID TEXT" );
        
        RBuf commonClauseSix;
        User::LeaveIfError( commonClauseSix.Create( KAddContentIDColumn.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseSix ); 
        commonClauseSix.Format( KAddContentIDColumn, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseSix, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseSix );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }

        // Add ContentID column to Videos table
        _LIT( KAddContentIDColumnVideos, "ALTER TABLE Video%u ADD COLUMN ContentID TEXT" );
        
        RBuf commonClauseSeven;
        User::LeaveIfError( commonClauseSeven.Create( KAddContentIDColumnVideos.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseSeven ); 
        commonClauseSeven.Format( KAddContentIDColumnVideos, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseSeven, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseSeven );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }        
        
        // Add ContentID column to Images table
        _LIT( KAddContentIDColumnImages, "ALTER TABLE Image%u ADD COLUMN ContentID TEXT" );
        
        RBuf commonClauseEight;
        User::LeaveIfError( commonClauseEight.Create( KAddContentIDColumnImages.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseEight ); 
        commonClauseEight.Format( KAddContentIDColumnImages, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseEight, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseEight );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }          
        
        // Add ContentID column to Audios table
        _LIT( KAddContentIDColumnAudios, "ALTER TABLE Audio%u ADD COLUMN ContentID TEXT" );
        
        RBuf commonClauseNine;
        User::LeaveIfError( commonClauseNine.Create( KAddContentIDColumnAudios.iTypeLength + KMaxUintValueLength ) );
        CleanupClosePushL( commonClauseNine ); 
        commonClauseNine.Format( KAddContentIDColumnAudios, KDefaultNamespaceDefId ); 
        
        TRAP( err, connection.ExecuteL( commonClauseNine, data ) );
        
        CleanupStack::PopAndDestroy( &commonClauseNine );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }           

        // Add ContentId property to PropertyDef table
        objectDefId = 8;
        type = EPropertyText;
        TInt32 minValue3 = 1;
        TInt32 maxValue3 = KSerializedDesMaxLength;
        HBufC16* name3 = MdeConstants::MediaObject::KContentID().AllocL();
        
        RRowData contentIDRowData;
        CleanupClosePushL( contentIDRowData );
        contentIDRowData.AppendL( TColumn( objectDefId ) );
        contentIDRowData.AppendL( TColumn( flags ) );
        contentIDRowData.AppendL( TColumn( type ) );
        contentIDRowData.AppendL( TColumn( minValue3 ) );
        contentIDRowData.AppendL( TColumn( maxValue3 ) );
        contentIDRowData.AppendL( TColumn( name3 ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddPropertyDef, contentIDRowData ) );
        
        CleanupStack::PopAndDestroy( &contentIDRowData );
 
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }        

        // Update Col2Prop table with new property       
        objectDefId = 8;
        propertyDefId = 122;
        columnId = 44;
        
        RRowData contentIdCol2propRow;
        CleanupClosePushL( contentIdCol2propRow );
        contentIdCol2propRow.AppendL( TColumn( objectDefId ) );
        contentIdCol2propRow.AppendL( TColumn( propertyDefId ) );
        contentIdCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, contentIdCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &contentIdCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }  
        
        contentIdCol2propRow.Reset();
        
        objectDefId = 9;
        propertyDefId = 122;
        columnId = 49;

        contentIdCol2propRow.AppendL( TColumn( objectDefId ) );
        contentIdCol2propRow.AppendL( TColumn( propertyDefId ) );
        contentIdCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, contentIdCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &contentIdCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            } 
        
        contentIdCol2propRow.Reset();
        
        objectDefId = 10;
        propertyDefId = 122;
        columnId = 85;

        contentIdCol2propRow.AppendL( TColumn( objectDefId ) );
        contentIdCol2propRow.AppendL( TColumn( propertyDefId ) );
        contentIdCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, contentIdCol2propRow ) );
        
        if( err != KErrNone )
            {
            CleanupStack::PopAndDestroy( &contentIdCol2propRow );
            
            CleanupStack::PopAndDestroy( &data );
            return EFalse;
            }         
        
        contentIdCol2propRow.Reset();
        
        objectDefId = 11;
        propertyDefId = 122;
        columnId = 49;

        contentIdCol2propRow.AppendL( TColumn( objectDefId ) );
        contentIdCol2propRow.AppendL( TColumn( propertyDefId ) );
        contentIdCol2propRow.AppendL( TColumn( columnId ) );
        
        TRAP( err, connection.ExecuteL( KMdsSqlClauseAddCol2Prop, contentIdCol2propRow ) );        
        
        CleanupStack::PopAndDestroy( &contentIdCol2propRow );        
        CleanupStack::PopAndDestroy( &data );
        
        if( err != KErrNone )
            {
            return EFalse;
            }

        TInt dbMajorVersion = KMdSServMajorVersionNumber;
        TInt64 dbMinorVersion = KMdSServMinorVersionNumber;
        MMdsPreferences::UpdateL( KMdsDBVersionName, MMdsPreferences::EPreferenceBothSet,
                dbMajorVersion, dbMinorVersion );
        
        TInt schemaMajorVersion = KSchemaFileMajorVersion;
        TInt64 schemaMinorVersion = 1;
        MMdsPreferences::UpdateL( KMdsSchemaVersionName, MMdsPreferences::EPreferenceBothSet,
                schemaMajorVersion, schemaMinorVersion );
        }
    else
        {
        return EFalse;
        }
    
    return ETrue;
    }

