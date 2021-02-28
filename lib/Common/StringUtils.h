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

#include <string>
#include <vector>

namespace Common {

    namespace StringUtils {

        std::vector <std::string> split (const std::string &cStr,
                                         const std::vector <std::string> &vDelimiters);

        std::vector <std::string> split (const std::string &cStr,
                                         const std::string &cDelimiter);

        std::string join (const std::vector <std::string> &vTokens,
                          const std::string &cDelimiter);

        std::string escapeChar (char cCharacter);

        std::string escapeString (const std::string &cStr);

        std::vector <std::string> escapeStrings (const std::vector <std::string> &cStrings);

        bool isAnInteger (const std::string &cToken);

        std::string extractRegion (const std::string &cStr,
                                   int iFrom,
                                   int iTo);

        int convertToInt (const std::string &cStr);

    } // namespace StringUtils

} // namespace Common
