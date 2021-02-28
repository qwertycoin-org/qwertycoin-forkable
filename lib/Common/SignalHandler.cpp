// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// This file is part of Qwertycoin.
//
// Qwertycoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Qwertycoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Qwertycoin.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <mutex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else

#include <csignal>
#include <cstring>
#include <utility>

#endif

#include <Common/SignalHandler.h>

namespace {

    std::function <void (void)> UHandler;

    void handleSignal ()
    {
        static std::mutex m_mutex;
        std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return;
        }
        UHandler();
    }

#if defined(WIN32)
    BOOL WINAPI winHandler(DWORD type)
    {
        if (CTRL_C_EVENT == type || CTRL_BREAK_EVENT == type) {
            handleSignal();
            return TRUE;
        } else {
            std::cerr << "Got control signal " << type << ". Exiting without saving...";
            return FALSE;
        }
        return TRUE;
    }
#else

    void posixHandler (int /*sType*/)
    {
        handleSignal();
    }

#endif

} // namespace

namespace Tools {

    bool QSignalHandler::install (std::function <void (void)> t)
    {
#if defined(WIN32)
        bool r = TRUE == ::SetConsoleCtrlHandler(&winHandler, TRUE);
        if (r)  {
            UHandler = t;
        }
        return r;
#else
        struct sigaction newMask {};

        std::memset(&newMask, 0, sizeof(struct sigaction));
        newMask.sa_handler = posixHandler;

        if (sigaction(SIGINT, &newMask, nullptr) != 0) {
            return false;
        }

        if (sigaction(SIGTERM, &newMask, nullptr) != 0) {
            return false;
        }

        std::memset(&newMask, 0, sizeof(struct sigaction));
        newMask.sa_handler = SIG_IGN;

        if (sigaction(SIGPIPE, &newMask, nullptr) != 0) {
            return false;
        }

        UHandler = std::move(t);

        return true;
#endif
    }

} // namespace Tools
