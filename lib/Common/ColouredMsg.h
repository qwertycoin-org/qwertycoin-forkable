// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, The TurtleCoin Developers
// Copyright (c) 2018-2019, The Karbo Developers
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

#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

#include <Common/ConsoleTools.h>

class QColouredMsg
{
public:
    QColouredMsg (std::string cMessage,
                  Common::Console::EColor sColour)
        : gMessage(std::move(cMessage)),
          gColour(sColour)
    {
    }

    QColouredMsg (std::string cMessage,
                  int iPadding,
                  Common::Console::EColor sColour)
        : gMessage(std::move(cMessage)),
          gColour(sColour),
          gPadding(iPadding),
          gPad(true)
    {
    }

    /**
     * Set the text gColour, write the message, then reset. We use a class
     * as it seems the only way to have a valid << operator. We need this
     * so we can nicely do something like:
     *
     * std::cout << "Hello " << GreenMsg("user") << std::endl;
     *
     * Without having to write:
     *
     * std::cout << "Hello ";
     * GreenMsg("user");
     * std::cout << std::endl;
     *
     * @param oStream
     * @param sColouredMsg
     * @return
     */
    friend std::ostream &operator<< (std::ostream &oStream, const QColouredMsg &sColouredMsg)
    {
        Common::Console::setTextColor(sColouredMsg.gColour);

        if (sColouredMsg.gPad) {
            oStream << std::left << std::setw(sColouredMsg.gPadding) << sColouredMsg.gMessage;
        } else {
            oStream << sColouredMsg.gMessage;
        }

        Common::Console::setTextColor(Common::Console::EColor::Default);

        return oStream;
    }

protected:
    std::string gMessage;
    const Common::Console::EColor gColour;
    const int gPadding = 0;
    const bool gPad = false;
};

class QSuccessMsg : public QColouredMsg
{
public:
    explicit QSuccessMsg (std::string cMessage)
        : QColouredMsg(cMessage,
                       Common::Console::EColor::Green)
    {
    }

    explicit QSuccessMsg (std::string cMessage, int iPadding)
        : QColouredMsg(cMessage,
                       iPadding,
                       Common::Console::EColor::Green)
    {
    }
};

class QInformationMsg : public QColouredMsg
{
public:
    explicit QInformationMsg (std::string cMessage)
        : QColouredMsg(cMessage,
                       Common::Console::EColor::BrightYellow)
    {
    }

    explicit QInformationMsg (std::string cMessage, int iPadding)
        : QColouredMsg(cMessage,
                       iPadding,
                       Common::Console::EColor::BrightYellow)
    {
    }
};

class QSuggestionMsg : public QColouredMsg
{
public:
    explicit QSuggestionMsg (std::string cMessage)
        : QColouredMsg(cMessage,
                       Common::Console::EColor::BrightBlue)
    {
    }

    explicit QSuggestionMsg (std::string cMessage, int iPadding)
        : QColouredMsg(cMessage,
                       iPadding,
                       Common::Console::EColor::BrightBlue)
    {
    }
};

class QWarningMsg : public QColouredMsg
{
public:
    explicit QWarningMsg (std::string cMessage)
        : QColouredMsg(cMessage,
                       Common::Console::EColor::BrightRed)
    {
    }

    explicit QWarningMsg (std::string cMessage, int iPadding)
        : QColouredMsg(cMessage,
                       iPadding,
                       Common::Console::EColor::BrightRed)
    {
    }
};
