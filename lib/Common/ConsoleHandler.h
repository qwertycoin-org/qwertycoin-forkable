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

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <Common/BlockingQueue.h>
#include <Common/ConsoleTools.h>

#ifndef _WIN32
#include <sys/select.h>
#endif

namespace Common {

class QAsyncConsoleReader
{
public:
    QAsyncConsoleReader();
    ~QAsyncConsoleReader();

    void start();
    bool getLine(std::string& cLine);
    void stop();
    bool stopped() const;
    void pause();
    void unpause();

private:
    void consoleThread();

    bool waitInput();

private:
    std::atomic<bool> mStop;
    std::thread mThread;
    TBlockingQueue<std::string> mQueue;
};

class QConsoleHandler
{
    typedef std::function<bool(const std::vector<std::string> &)> ConsoleCommandHandler;
    typedef std::map<std::string,
                     std::pair<ConsoleCommandHandler,
                               std::string>> CommandHandlersMap;
public:
    ~QConsoleHandler();

    std::string getUsage() const;
    void setHandler(const std::string &cCommand,
                    const ConsoleCommandHandler &sHandler,
                    const std::string &cUsage = "");
    void requestStop();
    bool runCommand(const std::vector<std::string> &vCmdAndArgs);

    void start(bool bStartThread = true,
               const std::string &cPrompt = "",
               Console::EColor sPromptColor = Console::EColor::Default);
    void stop();
    void wait();
    void pause();
    void unpause();

private:
    virtual void handleCommand(const std::string &cCmd);

    void handlerThread();

private:
    std::thread mThread;
    std::string mPrompt;
    Console::EColor mPromptColor = Console::EColor::Default;
    CommandHandlersMap mHandlers;
    QAsyncConsoleReader mConsoleReader;
};

} // namespace Common
