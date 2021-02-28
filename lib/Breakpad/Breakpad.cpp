// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Licensed under the GNU General Public License, Version 3.
// See the file LICENSE from this package for details.

#include <iostream>
#include <string>

#include <Breakpad/Breakpad.h>

#if defined(__linux__) && !defined(__ANDROID__) // Linux
#include <client/linux/handler/exception_handler.h>
#elif defined(__APPLE__) // macOS
#include <client/mac/handler/exception_handler.h>
#elif defined(_WIN32) || defined(WIN32) // Windows

#include <client/windows/handler/exception_handler.h>

#endif

#if defined(__linux__) && !defined(__ANDROID__) // Linux
static bool exceptionHandlerCallback(
    const google_breakpad::MinidumpDescriptor &descriptor,
    void *context,
    bool succeeded)
{
    std::cout << "ERROR: Process crashed! Minidump created." << std::endl;
    std::cout << "Minidump dir: " << descriptor.path() << std::endl;

    return true;
}
#elif defined(__APPLE__) // macOS
static bool exceptionHandlerCallback(
    const char *minidumpDir,
    const char *minidumpId,
    void *context,
    bool succeeded)
{
    std::cout << "ERROR: Process crashed! Minidump created." << std::endl;
    std::cout << "Minidump dir: " << minidumpDir << ", id: " << minidumpId << std::endl;

    return true;
}
#elif defined(_WIN32) || defined(WIN32) // Windows

static bool exceptionHandlerCallback (
    const wchar_t *cMinidumpPath,
    const wchar_t *cMinidumpId,
    void *sContext,
    EXCEPTION_POINTERS *sExInfo,
    MDRawAssertionInfo *sAssertion,
    bool bSucceeded)
{
    std::cout << "ERROR: Process crashed! Minidump created." << std::endl;
    std::wcout << L"Minidump path: " << cMinidumpPath << L", id: " << cMinidumpId << std::endl;

    return true;
}

#endif

Qwertycoin::Breakpad::QExceptionHandler::QExceptionHandler (const std::string &sDumpPath)
{
#if defined(_WIN32) || defined(WIN32) // Windows
    const std::string sDefaultDumpPath = std::string("C:\\Windows\\Temp");
#else // Linux, macOS, etc.
    const std::string sDefaultDumpPath = std::string("/tmp");
#endif

    const std::string validDumpPath = !sDumpPath.empty() ? sDumpPath : sDefaultDumpPath;

#if defined(__linux__) && !defined(__ANDROID__) // Linux
    mExceptionHandler = new google_breakpad::ExceptionHandler(
        google_breakpad::MinidumpDescriptor(validDumpPath),
        nullptr,
        exceptionHandlerCallback,
        nullptr,
        true,
        -1
    );
#elif defined(__APPLE__) // macOS
    mExceptionHandler = new google_breakpad::ExceptionHandler(
        validDumpPath,
        nullptr,
        exceptionHandlerCallback,
        nullptr,
        true,
        nullptr
    );
#elif defined(_WIN32) || defined(WIN32) // Windows
    std::wstring validDumpPathAsWString;
    validDumpPathAsWString.assign(validDumpPath.begin(), validDumpPath.end());
    mExceptionHandler = new google_breakpad::ExceptionHandler(
        validDumpPathAsWString,
        nullptr,
        exceptionHandlerCallback,
        nullptr,
        google_breakpad::ExceptionHandler::HANDLER_ALL
    );
#else
    mExceptionHandler = nullptr;
#endif
}

Qwertycoin::Breakpad::ExceptionHandler::~ExceptionHandler()
{
    if (mExceptionHandler) {
        delete mExceptionHandler;
        mExceptionHandler = nullptr;
    }
}

/*!
    WARNING: This function will crash running process! Use only for testing purposes.
*/
void Qwertycoin::Breakpad::ExceptionHandler::dummyCrash()
{
    int *a = (int *) 0x42;
    fprintf(stdout, "Going to crash...\n");
    fprintf(stdout, "A = %d", *a);
}
