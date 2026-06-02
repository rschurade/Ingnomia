/** @file crashhandler.cpp
 *  @brief Crash handler setup using BugSplat for minidump generation.
 *
 *  When built with BugSplat support (HAVE_BUGSPLAT, BUGSPLAT_DB, and
 *  BUILD_VERSION all defined), installs a MiniDmpSender that captures
 *  crash dumps including the game log file. Otherwise provides a no-op stub.
 */

#include "crashhandler.h"

#include "../version.h"
#include "io.h"

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

/** @brief Installs the BugSplat crash handler for minidump generation.
 *
 *  Creates a MiniDmpSender configured with the project's BugSplat database,
 *  name, and version. Enables guard memory, log file collection, hang
 *  detection, thread suspension, and hijack prevention. Sets a 20 MB guard
 *  buffer for out-of-memory crash capture and attaches the game log file
 *  as an additional crash report artifact.
 */
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
	QString logFile = IO::getDataFolder() + "/log.txt";
	mpSender->sendAdditionalFile( logFile.replace( '/', '\\' ).toStdWString().c_str() );
}
#else
/** @brief No-op crash handler stub when BugSplat is not available. */
void setupCrashHandler()
{
}
#endif // USE_BUGSPLAT