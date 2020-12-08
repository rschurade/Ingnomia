#include "crashhandler.h"

#include "../version.h"

#if defined( PROJECT_VERSION) && defined( BUILD_ID )
#define BUILD_VERSION PROJECT_VERSION "-" BUILD_ID
#endif

#if defined( HAVE_BUGSPLAT ) && defined( BUGSPLAT_DB ) && defined( BUILD_VERSION )
#define USE_BUGSPLAT 1
#endif

#ifdef USE_BUGSPLAT
#include <BugSplat.h>

#define WIDEN2( x ) L##x
#define WIDEN( x )  WIDEN2( x )

void setupCrashHandler()
{
	static MiniDmpSender* mpSender = new MiniDmpSender( WIDEN( BUGSPLAT_DB ), WIDEN( PROJECT_NAME ), WIDEN( BUILD_VERSION ), NULL, MDSF_USEGUARDMEMORY | MDSF_LOGFILE | MDSF_DETECTHANGS | MDSF_SUSPENDALLTHREADS | MDSF_PREVENTHIJACKING );
	// The following calls add support for collecting crashes for abort(), vectored exceptions, out of memory,
	// pure virtual function calls, and for invalid parameters for OS functions.
	// These calls should be used for each module that links with a separate copy of the CRT.
	SetGlobalCRTExceptionBehavior();
	SetPerThreadCRTExceptionBehavior(); // This call needed in each thread of your app

	// A guard buffer of 20mb is needed to catch OutOfMemory crashes
	mpSender->setGuardByteBufferSize( 20 * 1024 * 1024 );

	// Include main log file
	QString logFile = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/log.txt";
	mpSender->sendAdditionalFile( logFile.replace( '/', '\\' ).toStdWString().c_str() );
}
#else
void setupCrashHandler()
{
}
#endif // USE_BUGSPLAT