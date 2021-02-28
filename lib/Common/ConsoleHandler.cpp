// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2014-2016 XDN developers
// Copyright (c) 2006-2013 Andrey N.Sabelnikov, www.sabelnikov.net
// Copyright (c) 2016-2017 The Karbowanec developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include <Common/ConsoleHandler.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#else
#include <cstdio>
#include <unistd.h>
#endif

using Common::Console::EColor;

namespace Common {

    QAsyncConsoleReader::QAsyncConsoleReader ()
        : mStop(true)
    {
    }

    QAsyncConsoleReader::~QAsyncConsoleReader ()
    {
        stop();
    }

    void QAsyncConsoleReader::start ()
    {
        mStop = false;
        mThread = std::thread([this] { consoleThread(); });
    }

    bool QAsyncConsoleReader::getLine (std::string &cLine)
    {
        return mQueue.pop(cLine);
    }

    void QAsyncConsoleReader::pause ()
    {
        if (mStop) {
            return;
        }

        mStop = true;

        if (mThread.joinable()) {
            mThread.join();
        }

        mThread = std::thread();
    }

    void QAsyncConsoleReader::unpause ()
    {
        start();
    }

    void QAsyncConsoleReader::stop ()
    {
        if (mStop) {
            return; // already stopping/stopped
        }

        mStop = true;
        mQueue.close();

#ifdef _WIN32
        ::CloseHandle(::GetStdHandle(STD_INPUT_HANDLE));
#endif

        if (mThread.joinable()) {
            mThread.join();
        }

        mThread = std::thread();
    }

    bool QAsyncConsoleReader::stopped () const
    {
        return mStop;
    }

    void QAsyncConsoleReader::consoleThread ()
    {
        while (waitInput()) {
            std::string line;

            if (!std::getline(std::cin, line)) {
                break;
            }

            if (!mQueue.push(line)) {
                break;
            }
        }
    }

    bool QAsyncConsoleReader::waitInput ()
    {
#ifndef _WIN32
#if defined(__OpenBSD__) || defined(__ANDROID__)
        int stdinFileno = fileno(stdin);
#else
        int stdinFileno = ::fileno(stdin);
#endif

    while (!mStop) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(stdinFileno, &readSet);

        struct timeval tv = timeval();
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        int retVal = ::select(stdinFileno + 1, &readSet, nullptr, nullptr, &tv);

        if (retVal == -1 && errno == EINTR) {
            continue;
        }

        if (retVal < 0) {
            return false;
        }

        if (retVal > 0) {
            return true;
        }
    }
#else
        while (!mStop.load(std::memory_order_relaxed)) {
            int retval = ::WaitForSingleObject(::GetStdHandle(STD_INPUT_HANDLE), 100);
            switch (retval) {
                case WAIT_FAILED:
                    return false;
                case WAIT_OBJECT_0:
                    return true;
                default:
                    break;
            }
        }
#endif

        return !mStop;
    }

    QConsoleHandler::~QConsoleHandler ()
    {
        stop();
    }

    void QConsoleHandler::start (bool bStartThread,
                                 const std::string &cPrompt,
                                 Console::EColor sPromptColor)
    {
        mPrompt = cPrompt;
        mPromptColor = sPromptColor;
        mConsoleReader.start();

        if (bStartThread) {
            mThread = std::thread([this] { handlerThread(); });
        } else {
            handlerThread();
        }
    }

    void QConsoleHandler::stop ()
    {
        requestStop();
        wait();
    }

    void QConsoleHandler::pause ()
    {
        mConsoleReader.pause();
    }

    void QConsoleHandler::unpause ()
    {
        mConsoleReader.unpause();
    }

    void QConsoleHandler::wait ()
    {
        try {
            if (mThread.joinable()) {
                mThread.join();
            }
        } catch (std::exception &e) {
            std::cerr << "Exception in QConsoleHandler::wait - " << e.what() << std::endl;
        }
    }

    void QConsoleHandler::requestStop ()
    {
        mConsoleReader.stop();
    }

    std::string QConsoleHandler::getUsage () const
    {
        if (mHandlers.empty()) {
            return std::string();
        }

        std::stringstream ss;

        size_t uMaxLength = std::max_element(
            mHandlers.begin(),
            mHandlers.end(),
            [] (CommandHandlersMap::const_reference &a,
                CommandHandlersMap::const_reference &b)
            {
                return a.first.size() < b.first.size();
            })->first.size();

        for (auto &x : mHandlers) {
            ss << std::left << std::setw(uMaxLength + 3) << x.first << x.second.second << std::endl;
        }

        return ss.str();
    }

    void QConsoleHandler::setHandler (const std::string &cCommand,
                                      const ConsoleCommandHandler &sHandler,
                                      const std::string &cUsage)
    {
        mHandlers[cCommand] = std::make_pair(sHandler, cUsage);
    }

    bool QConsoleHandler::runCommand (const std::vector <std::string> &vCmdAndArgs)
    {
        if (vCmdAndArgs.empty()) {
            return false;
        }

        const auto &cmd = vCmdAndArgs.front();
        auto hIter = mHandlers.find(cmd);

        if (hIter == mHandlers.end()) {
            std::cout << "Unknown command: " << cmd << std::endl;
            return false;
        }

        std::vector <std::string> vArgs(vCmdAndArgs.begin() + 1, vCmdAndArgs.end());
        hIter->second.first(vArgs);

        return true;
    }

    void QConsoleHandler::handleCommand (const std::string &cCmd)
    {
        bool bParseString = false;
        std::string cArg;
        std::vector <std::string> vArgList;

        for (auto ch : cCmd) {
            switch (ch) {
                case ' ':
                    if (bParseString) {
                        cArg += ch;
                    } else if (!cArg.empty()) {
                        vArgList.emplace_back(std::move(cArg));
                        cArg.clear();
                    }
                    break;
                case '"':
                    if (!cArg.empty()) {
                        vArgList.emplace_back(std::move(cArg));
                        cArg.clear();
                    }
                    bParseString = !bParseString;
                    break;
                default:
                    cArg += ch;
            }
        }

        if (!cArg.empty()) {
            vArgList.emplace_back(std::move(cArg));
        }

        runCommand(vArgList);
    }

    void QConsoleHandler::handlerThread ()
    {
        std::string cLine;

        while (!mConsoleReader.stopped()) {
            try {
                if (!mPrompt.empty()) {
                    if (mPromptColor != EColor::Default) {
                        Console::setTextColor(mPromptColor);
                    }

                    std::cout << mPrompt;
                    std::cout.flush();

                    if (mPromptColor != EColor::Default) {
                        Console::setTextColor(EColor::Default);
                    }
                }

                if (!mConsoleReader.getLine(cLine)) {
                    break;
                }

                boost::algorithm::trim(cLine);
                if (!cLine.empty()) {
                    handleCommand(cLine);
                }
            } catch (std::exception &) {
                // ignore errors, best comment EVER SEEN!
                // i'll add Error logs later
            }
        }
    }

} // namespace Common
