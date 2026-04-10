/** @file crashhandler.h
 * @brief Windows structured exception handler for crash dump generation.
 */

#pragma once

/** @brief Installs a Windows SEH handler that writes a minidump on unhandled exceptions. */
void setupCrashHandler();