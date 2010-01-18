/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies). 
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

#include "mdeobjectdef.h"

#include "mdcdef.h"
#include "mdenamespacedef.h"
#include "mdepropertydef.h"
#include "mdcserializationbuffer.h"
#include "mdscommoninternal.h"


CMdEObjectDef::CMdEObjectDef( const TMdCObjectDef& aObjectDef, CMdENamespaceDef& aNamespaceDef )
    : iObjectDef( aObjectDef ), iNamespaceDef(aNamespaceDef)
    {
    }

CMdEObjectDef* CMdEObjectDef::NewL( const TMdCObjectDef& aObjectDef,
		CMdCSerializationBuffer& aSchemaBuffer, CMdENamespaceDef& aNamespaceDef )
    {
    CMdEObjectDef* def = CMdEObjectDef::NewLC( aObjectDef, aSchemaBuffer, aNamespaceDef );
    CleanupStack::Pop( def );
    return def;
    }

CMdEObjectDef* CMdEObjectDef::NewLC( const TMdCObjectDef& aObjectDef,
		CMdCSerializationBuffer& aSchemaBuffer, CMdENamespaceDef& aNamespaceDef )
    {
    CMdEObjectDef* def = new( ELeave ) CMdEObjectDef( aObjectDef, aNamespaceDef );
    CleanupStack::PushL( def );
    def->ConstructL(aSchemaBuffer);
    return def;
    }

void CMdEObjectDef::ConstructL(CMdCSerializationBuffer& aSchemaBuffer)
    {
    const TUint32 propertyCount = iObjectDef.iPropertyDefs.iPtr.iCount;
    const TUint32 propertyOffset = iObjectDef.iPropertyDefs.iPtr.iOffset;

	aSchemaBuffer.PositionL( iObjectDef.iName.iPtr.iOffset );
	iName.Set( aSchemaBuffer.ReceivePtr16L() );

    iPropertyDefs.ReserveL( propertyCount );
    for ( TUint32 i = 0; i < propertyCount; ++i )
        {
		aSchemaBuffer.PositionL( propertyOffset + i * sizeof(TMdCPropertyDef) );
		const TMdCPropertyDef& propertyDef = TMdCPropertyDef::GetFromBufferL(aSchemaBuffer);
		iPropertyDefs.AppendL( CMdEPropertyDef::NewL( propertyDef, aSchemaBuffer, *this ) );
        }
    }

CMdEObjectDef::~CMdEObjectDef()
    {
    iPropertyDefs.ResetAndDestroy();
    iPropertyDefs.Close();
    }

TDefId CMdEObjectDef::Id() const
    {
    return iObjectDef.iDefId;
    }

EXPORT_C CMdENamespaceDef& CMdEObjectDef::NamespaceDef() const
    {
    return iNamespaceDef;
    }

EXPORT_C CMdEObjectDef* CMdEObjectDef::ParentL()
    {
    if ( iObjectDef.iParentId == KNoDefId )
        {
        return NULL;
        }
    return iNamespaceDef.GetObjectDefL( iObjectDef.iParentId );
    }

EXPORT_C const TDesC& CMdEObjectDef::Name() const
    {
    return iName;
    }

EXPORT_C TInt CMdEObjectDef::Compare( const CMdEObjectDef& aObjectDef ) const
    {
    TInt result = iNamespaceDef.Compare( aObjectDef.iNamespaceDef );
    if ( result == 0 )
        {
        result = iObjectDef.iDefId - aObjectDef.Id();
        }
    return result;
    }

EXPORT_C TInt CMdEObjectDef::PropertyDefCount() const
    {
    return iPropertyDefs.Count();
    }

EXPORT_C CMdEPropertyDef& CMdEObjectDef::PropertyDefL(TInt aIndex)
	{
	CMdEPropertyDef* propertyDef = PropertyDefL(aIndex, NULL);
	if (!propertyDef)
		{
		User::Leave( KErrNotFound );
		}
	return *propertyDef;
	}

EXPORT_C CMdEPropertyDef& CMdEObjectDef::GetPropertyDefL( const TDesC& aName )
	{
	CMdEPropertyDef* propertyDef = GetPropertyDefL(aName, NULL);
	if (!propertyDef)
		{
		User::Leave( KErrNotFound );
		}
	return *propertyDef;
	}

CMdEPropertyDef* CMdEObjectDef::GetPropertyDefL( const TDesC& aName, CMdEObjectDef* aChild )
    {
    const TInt propertyDefCount = iPropertyDefs.Count();
    for ( TInt i = 0; i < propertyDefCount; ++i )
        {
        if( aName.Compare( iPropertyDefs[i]->Name() ) == 0 )
        	{
        	return &PropertyDefL(i);
        	}
        }

    CMdEObjectDef* parent = ParentL();
    if ( parent ) 
        {
        if( !aChild )
            {
            return parent->GetPropertyDefL( aName, this );
            }
        else
            {
            return parent->GetPropertyDefL( aName, aChild );
            }
        }
    return NULL;
    }

CMdEPropertyDef* CMdEObjectDef::GetPropertyDefL(TDefId aId, CMdEObjectDef* aChild)
    {
    const TInt count = iPropertyDefs.Count();
    
    for ( TInt i = 0; i < count; ++i )
        {
        CMdEPropertyDef* propDef = iPropertyDefs[i];
        
        if( propDef && propDef->Id() == aId )
        	{
        	return propDef;
        	}
        }

    CMdEObjectDef* parent = ParentL();
    if ( parent ) 
        {
        if ( !aChild )
            {                
            return parent->GetPropertyDefL( aId, this );
            }
        else
            {
            return parent->GetPropertyDefL( aId, aChild );
            }
        }
    return NULL;
    }

CMdEPropertyDef* CMdEObjectDef::PropertyDefL(TInt aIndex, CMdEObjectDef* /*aChild*/)
	{
	if ( aIndex < 0 || aIndex > iPropertyDefs.Count())
	    {
	    User::Leave( KErrArgument );
	    }

	return iPropertyDefs[aIndex];
	}
