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

#include <regex>
#include <sstream>
#include <unordered_map>

#include <Common/StringUtils.h>
#include <Common/VectorUtils.h>

namespace Common {

    std::string StringUtils::join (const std::vector <std::string> &vTokens,
                                   const std::string &cDelimiter)
    {
        std::stringstream stream;

        stream << vTokens.front();

        std::for_each(begin(vTokens) + 1, end(vTokens), [&] (const std::string &elem)
        {
            stream << cDelimiter << elem;
        });

        return stream.str();
    }

    std::vector <std::string> StringUtils::split (const std::string &cStr,
                                                  const std::vector <std::string> &vDelimiters)
    {
        std::regex rgx(join(escapeStrings(vDelimiters), "|"));

        std::sregex_token_iterator first {begin(cStr), end(cStr), rgx, -1};
        std::sregex_token_iterator last;

        return {first, last};
    }

    std::vector <std::string> StringUtils::split (const std::string &cStr,
                                                  const std::string &cDelimiter)
    {
        std::vector <std::string> delimiters = {cDelimiter};

        return split(cStr, delimiters);
    }

    std::string StringUtils::escapeChar (char cCharacter)
    {
        const std::unordered_map <char, std::string> ScapedSpecialCharacters = {
            {'.',  "\\."},
            {'|',  "\\|"},
            {'*',  "\\*"},
            {'?',  "\\?"},
            {'+',  "\\+"},
            {'(',  "\\("},
            {')',  "\\)"},
            {'{',  "\\{"},
            {'}',  "\\}"},
            {'[',  "\\["},
            {']',  "\\]"},
            {'^',  "\\^"},
            {'$',  "\\$"},
            {'\\', "\\\\"}
        };

        auto it = ScapedSpecialCharacters.find(cCharacter);

        if (it == ScapedSpecialCharacters.end()) {
            return std::string(1, cCharacter);
        }

        return it->second;
    }

    std::string StringUtils::escapeString (const std::string &cStr)
    {
        std::stringstream stream;

        std::for_each(begin(cStr), end(cStr), [&stream] (const char character)
        {
            stream << escapeChar(character);
        });

        return stream.str();
    }

    std::vector <std::string> StringUtils::escapeStrings (const std::vector <std::string> &cStrings)
    {
        return VectorUtils::map <std::string>(cStrings, escapeString);
    }

    bool StringUtils::isAnInteger (const std::string &cToken)
    {
        const std::regex e("\\s*[+-]?([1-9][0-9]*|0[0-7]*|0[xX][0-9a-fA-F]+)");

        return std::regex_match(cToken, e);
    }

    std::string StringUtils::extractRegion (const std::string &cStr, int iFrom, int iTo)
    {
        std::string region;
        int regionSize = iTo - iFrom;

        return cStr.substr(iFrom, regionSize);
    }

    int StringUtils::convertToInt (const std::string &cStr)
    {
        std::string::size_type sz;

        return std::stoi(cStr, &sz);
    }

} // namespace Common
