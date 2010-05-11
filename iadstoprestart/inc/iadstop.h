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
* Description:  Helper application to stop servers for IAD upgrade
*
*/

#ifndef IADSTOP_H
#define IADSTOP_H

#include <e32base.h>
#include <w32std.h>

_LIT( KHarvesterServerName, "HarvesterServer" );
_LIT( KHarvesterServerProcess, "*HarvesterServer*");
_LIT( KHarvesterServerExe, "harvesterserver.exe" );

_LIT( KMdsServerName, "!MdSServer" );
_LIT( KMdSServerProcess, "*!mdsserver*");

_LIT( KTAGDaemonName, "ThumbAGDaemon" );
_LIT( KTAGDaemonProcess, "*ThumbAGDaemon*" );
_LIT( KTAGDaemonExe, "thumbagdaemon.exe" );

_LIT( KListenerProcess, "*iadlistener*");

_LIT( KWatchdogProcess, "*mdswatchdog*");

GLDEF_C TInt E32Main();

#endif // IADRESTART_H
