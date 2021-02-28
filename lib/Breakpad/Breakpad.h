// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Licensed under the GNU General Public License, Version 3.
// See the file LICENSE from this package for details.

#pragma once

#include <string>

namespace google_breakpad {
    class ExceptionHandler;
} // namespace google_breakpad

namespace Qwertycoin {

    namespace Breakpad {

        class QExceptionHandler
        {
        public:
            explicit QExceptionHandler (const std::string &sDumpPath = std::string());

            virtual ~QExceptionHandler ();

            static void dummyCrash ();

        private:
            google_breakpad::ExceptionHandler *mExceptionHandler = nullptr;
        };

    } // namespace Breakpad

} // namespace Qwertycoin
