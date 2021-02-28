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
#include <unordered_map>
#include <Common/ConsoleTools.h>
#include <Logging/ConsoleLogger.h>

namespace Logging {

using Common::Console::EColor;

ConsoleLogger::ConsoleLogger(Level level)
    : CommonLogger(level)
{
}

void ConsoleLogger::doLogString(const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool readingText = true;
    bool changedColor = false;
    std::string color;

    static std::unordered_map<std::string, EColor> colorMapping = {
        { BLUE,           EColor::Blue },
        { GREEN,          EColor::Green },
        { RED,            EColor::Red },
        { YELLOW,         EColor::Yellow },
        { WHITE,          EColor::White },
        { CYAN,           EColor::Cyan },
        { MAGENTA,        EColor::Magenta },

        { BRIGHT_BLUE,    EColor::BrightBlue },
        { BRIGHT_GREEN,   EColor::BrightGreen },
        { BRIGHT_RED,     EColor::BrightRed },
        { BRIGHT_YELLOW,  EColor::BrightYellow },
        { BRIGHT_WHITE,   EColor::BrightWhite },
        { BRIGHT_CYAN,    EColor::BrightCyan },
        { BRIGHT_MAGENTA, EColor::BrightMagenta },

        { DEFAULT,        EColor::Default }
    };

    for(size_t charPos = 0; charPos < message.size(); ++charPos) {
        if (message[charPos] == ILogger::COLOR_DELIMETER) {
            readingText = !readingText;
            color += message[charPos];
            if (readingText) {
                auto it = colorMapping.find(color);
                Common::Console::setTextColor(it == colorMapping.end() ? EColor::Default : it->second);
                changedColor = true;
                color.clear();
            }
        } else if (readingText) {
            std::cout << message[charPos];
        } else {
            color += message[charPos];
        }
    }

    if (changedColor) {
        Common::Console::setTextColor(EColor::Default);
    }
}

} // namespace Logging
