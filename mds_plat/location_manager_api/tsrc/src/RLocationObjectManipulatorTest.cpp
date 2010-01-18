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
#include "LocationManagerTestScripter.h"
#include "mdeobject.h"
#include "mdenamespacedef.h"
#include "mdeobjectdef.h"
#include "mdepropertydef.h"
#include "mdeconstants.h"
#include "locationdatatype.h"
#include "mdsutils.h"
#include <etel3rdparty.h>
#include <StifTestEventInterface.h>
#include <StifParser.h>
#include <StifTestEventInterface.h>
#include <StifTestInterface.h>

using namespace MdeConstants;

TInt CLocationManagerTestScripter::PrepareSessionL( CStifItemParser& /*aItem*/ )
    {
    _LIT( KMsg1, "PrepareSessionL" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
    
    User::LeaveIfError( iOM.Connect() );
    iMdeSession = CMdESession::NewL( *this );
    
    return KErrNone;
    }

TInt CLocationManagerTestScripter::SetupOML( CStifItemParser& /*aItem*/ )
	{
    _LIT( KMsg1, "Enter SetupOM" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
	
	iSourceObject = CreateMetadataObjectL();
	TLocationData locationData;
	TestLocationData( locationData );
	TInt error( KErrNone );
	error = iOM.CreateLocationObject( locationData, iSourceObject->Id() );
	if( error != KErrNone )
	    {
	    return error;
	    }
	iTargetObject = CreateMetadataObjectL();	
	
    _LIT( KMsg2, "Exit SetupOM" );
    iLog->Log( KMsg2 );  
    RDebug::Print( KMsg2 );
	
	return KErrNone;
	}

TInt CLocationManagerTestScripter::LocationSnapshotL( CStifItemParser& /*aItem*/ )
    {
    _LIT( KMsg1, "Enter LocationSnapshotL" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
    
    iSourceObject = CreateMetadataObjectL();
    TInt error( KErrNone );
    error = iOM.LocationSnapshot( iSourceObject->Id() );
    
    _LIT( KMsg2, "Exit LocationSnapshotL" );
    iLog->Log( KMsg2 );  
    RDebug::Print( KMsg2 );
    
    return error;
    }

TInt CLocationManagerTestScripter::RemoveLocationObjectL( CStifItemParser& /*aItem*/ )
    {
    _LIT( KMsg1, "Enter RemoveLocationObjectL" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );

    TInt error( KErrNone );
    error = iOM.RemoveLocationObject( iSourceObject->Id() );
    
    _LIT( KMsg2, "Exit RemoveLocationObjectL" );
    iLog->Log( KMsg2 );  
    RDebug::Print( KMsg2 );
    
    return error;
    }

TInt CLocationManagerTestScripter::TearDownOML( CStifItemParser& /*aItem*/ )
	{
    _LIT( KMsg1, "Enter TearDownOM" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
	
	iOM.Close();
	
	if( iSourceObject && iMdeSession )
	    {
	    TRAP_IGNORE( iMdeSession->RemoveObjectL( iSourceObject->Id() ) );
	    }
	delete iSourceObject;
	iSourceObject = NULL;
    if( iTargetObject && iMdeSession )
        {
        TRAP_IGNORE( iMdeSession->RemoveObjectL( iTargetObject->Id() ) );
        }
	delete iTargetObject;
	iTargetObject = NULL;
	delete iMdeSession;
	iMdeSession = NULL;
	
    _LIT( KMsg2, "Exit TearDownOM" );
    iLog->Log( KMsg2 );  
    RDebug::Print( KMsg2 );
	
	return KErrNone;
	}

TInt CLocationManagerTestScripter::CloseOML( CStifItemParser& /*aItem*/ )
    {
    _LIT( KMsg1, "CloseOML" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
    
    iOM.Close();
    
    return KErrNone;
    }

TInt CLocationManagerTestScripter::RLocationObjectManipulatorTest_CopyByIDL( CStifItemParser& /*aItem*/ )
	{
    _LIT( KMsg1, "Enter RLocationObjectManipulatorTest_CopyByIDL" );
    iLog->Log( KMsg1 );  
    RDebug::Print( KMsg1 );
	
	TRequestStatus status = KRequestPending;
	RArray<TItemId> items;
	items.Append( iTargetObject->Id() );
	iOM.CopyLocationData( iSourceObject->Id(), items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNone );
	
	status = KRequestPending;
	iOM.CopyLocationData( TItemId(12345678), items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	items.Reset();
	items.Append(TItemId(12345678));
	status = KRequestPending;
	iOM.CopyLocationData( iSourceObject->Id(), items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	status = KRequestPending;
	iOM.CopyLocationData( TItemId(12345678), items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	items.Close();
	
    _LIT( KMsg2, "Exit RLocationObjectManipulatorTest_CopyByIDL" );
    iLog->Log( KMsg2 );  
    RDebug::Print( KMsg2 );
	
	return KErrNone;
	}

TInt CLocationManagerTestScripter::RLocationObjectManipulatorTest_CopyByURIL( CStifItemParser& /*aItem*/ )
	{
	_LIT(KURIThatDoesNotExist, "qwerty");
	
	TRequestStatus status = KRequestPending;
	RPointerArray<TDesC> items;
	items.AppendL( &iTargetObject->Uri() );
	TBuf<256> source( iSourceObject->Uri() );
	iOM.CopyLocationData( source, items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNone );
	
	source.Copy( KURIThatDoesNotExist );
	status = KRequestPending;
	iOM.CopyLocationData( source, items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	items.Reset();
    HBufC* nouri = source.AllocL();
	items.Append( nouri );
	source.Copy( iSourceObject->Uri() );
	status = KRequestPending;
    iOM.CopyLocationData( source, items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	status = KRequestPending;
	source.Copy( KURIThatDoesNotExist );
	iOM.CopyLocationData( source, items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrNotFound );
	
	items.ResetAndDestroy();
	
	return KErrNone;
	}

TInt CLocationManagerTestScripter::RLocationObjectManipulatorTest_CopyByID_DisconnectedL( 
		CStifItemParser& /*aItem*/ )
	{
	TRequestStatus status = KRequestPending;
	RArray<TItemId> items;
	items.Append( 97976479 );
	iOM.CopyLocationData( iSourceObject->Id(), items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrDisconnected );
	
	return KErrNone;
	}

TInt CLocationManagerTestScripter::RLocationObjectManipulatorTest_CopyByURI_DisconnectedL( 
		CStifItemParser& /*aItem*/ )
	{
	_LIT(KURIThatDoesNotExist, "notexistinganywhere");
	
	TRequestStatus status = KRequestPending;
	RPointerArray<TDesC> items;
	items.AppendL( &KURIThatDoesNotExist );
	TBuf<256> source( iSourceObject->Uri() );
	iOM.CopyLocationData( source, items, status );
	User::WaitForRequest( status );
	TL( status.Int() == KErrDisconnected );
	
	return KErrNone;
	}	

CMdEObject* CLocationManagerTestScripter::CreateMetadataObjectL( )
	{	
	CMdENamespaceDef& namespaceDef = iMdeSession->GetDefaultNamespaceDefL();
	CMdEObjectDef& objectDef = namespaceDef.GetObjectDefL( Image::KImageObject );
	CMdEObject *obj = iMdeSession->NewObjectLC( objectDef, Object::KAutomaticUri );
	
	// required object properties
	CMdEPropertyDef& creationDef = objectDef.GetPropertyDefL( Object::KCreationDateProperty );
	CMdEPropertyDef& modifiedDef = objectDef.GetPropertyDefL( Object::KLastModifiedDateProperty );
	CMdEPropertyDef& sizeDef = objectDef.GetPropertyDefL( Object::KSizeProperty );
	CMdEPropertyDef& itemTypeDef = objectDef.GetPropertyDefL( Object::KItemTypeProperty );
	
	TTime timestamp( 0 );
	timestamp.UniversalTime();

	// required object properties
	obj->AddTimePropertyL( creationDef, timestamp );
	obj->AddTimePropertyL( modifiedDef, timestamp );
	obj->AddUint32PropertyL( sizeDef, 0 ); // always zero size for location objects
	obj->AddTextPropertyL( itemTypeDef, Location::KLocationItemType );
	
	iMdeSession->AddObjectL( *obj );
	CMdEObject* obj2 = iMdeSession->GetObjectL( obj->Id() );
	CleanupStack::Pop( obj );
	
	return obj2;
	}

void CLocationManagerTestScripter::HandleSessionOpened(CMdESession& /*aSession*/, TInt aError)
	{
    _LIT( KMsg, "CallBck HandleSessionOpened - Error code : %d" );
    TBuf <100> msg;
    msg.Format(KMsg, aError);
    iLog->Log( msg );     
    RDebug::Print(msg);
    
    // session event
    TEventIf event( TEventIf::ESetEvent, _L("Session") );
    TestModuleIf().Event( event );
	}

void CLocationManagerTestScripter::HandleSessionError(CMdESession& /*aSession*/, TInt aError)
	{
    _LIT( KMsg, "CallBck HandleSessionError - Error code : %d" );
    TBuf <100> msg;
    msg.Format(KMsg, aError);
    iLog->Log( msg );
    RDebug::Print(msg);
    
    // session event
    TEventIf event( TEventIf::ESetEvent, _L("Session") );
    TestModuleIf().Event( event );
	}

void CLocationManagerTestScripter::TestLocationData( TLocationData& aLocationData )
	{
	_LIT( temp, "XXX" );
	TBuf<3> countryTxt( temp );

	TPosition testLocality;
	testLocality.SetCoordinate( (TReal64)Math::Random()/KMaxTUint*180.0, (TReal64)Math::Random()/KMaxTUint*90.0, 10.0 );
	testLocality.SetAccuracy( 2.0, 2.0 );

	CTelephony::TNetworkInfoV1 networkInfo;
	networkInfo.iAccess = CTelephony::ENetworkAccessGsm;
	networkInfo.iAreaKnown = ETrue;
	networkInfo.iBandInfo = CTelephony::E800BandA;
	networkInfo.iCellId = 1;
	networkInfo.iLocationAreaCode = 1;
	networkInfo.iMode = CTelephony::ENetworkModeGsm;
	networkInfo.iStatus = CTelephony::ENetworkStatusCurrent;
	networkInfo.iCountryCode = countryTxt;
	networkInfo.iNetworkId = countryTxt;

	TCourse kurssi;
	kurssi.SetCourse( 1.0 );
	kurssi.SetCourseAccuracy( 1.0 );
	kurssi.SetHeading( 10.0 );
	kurssi.SetSpeed( 10.0 );

	aLocationData.iPosition = testLocality;
	aLocationData.iCountry = countryTxt;
	aLocationData.iNetworkInfo = networkInfo;
	aLocationData.iSatellites = 4;
	aLocationData.iCourse = kurssi;
	aLocationData.iQuality = 1;
	}

// End of file

