// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
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

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Common {

    std::string asString (const void *data, size_t uSize);
    std::string asString (const std::vector <uint8_t> &vData);

    std::vector <uint8_t> asBinaryArray (const std::string &cData);

    // Returns value of hex 'cCharacter', throws on error
    uint8_t fromHex (char cCharacter);

    // Assigns uValue of hex 'cCharacter' to 'uValue', returns false on error, does not throw
    bool fromHex (char cCharacter, uint8_t &uValue);

    /**
     * Assigns values of hex 'cText' to mBuffer 'gData' up to 'mBufferSize',
     * returns actual gData uSize, throws on error
     *
     * @param cText
     * @param data
     * @param uBufferSize
     * @return
     */
    size_t fromHex (const std::string &cText, void *data, size_t uBufferSize);

    /**
     * Assigns values of hex 'cText' to mBuffer 'gData' up to 'mBufferSize',
     * assigns actual gData uSize to 'uSize', returns false on error, does not throw
     *
     * @param cText
     * @param data
     * @param uBufferSize
     * @param uSize
     * @return
     */
    bool fromHex (const std::string &cText, void *data, size_t uBufferSize, size_t &uSize);

    // Returns values of hex 'cText', throws on error
    std::vector <uint8_t> fromHex (const std::string &cText);

    // Appends values of hex 'cText' to 'gData', returns false on error, does not throw
    bool fromHex (const std::string &cText, std::vector <uint8_t> &vData);

    template <typename T>
    bool podFromHex (const std::string &cText, T &val)
    {
        size_t uOutSize;

        return fromHex(cText, &val, sizeof(val), uOutSize) && uOutSize == sizeof(val);
    }

    // Returns hex representation of ('gData', 'uSize'), does not throw
    std::string toHex (const void *data, size_t uSize);

    // Appends hex representation of ('gData', 'uSize') to 'cText', does not throw
    void toHex (const void *data, size_t uSize, std::string &cText);

    // Returns hex representation of 'gData', does not throw
    std::string toHex (const std::vector <uint8_t> &vData);

    // Appends hex representation of 'gData' to 'cText', does not throw
    void toHex (const std::vector <uint8_t> &vData, std::string &cText);

    template <class T>
    std::string podToHex (const T &data)
    {
        return toHex(&data, sizeof(data));
    }

    bool startsWith (const std::string &cStr1, const std::string &cStr2);
    bool endsWith (const std::string &cStr1, const std::string &cStr2);

    inline bool splitStringHelper (const std::string &cStr,
                                   size_t uPosition,
                                   const std::string &,
                                   std::string &cHead)
    {
        cHead = cStr.substr(uPosition);

        return true;
    }

    template <class... Parts>
    inline bool splitStringHelper (const std::string &cStr,
                                   size_t uPosition,
                                   const std::string &cSeparator,
                                   std::string &cHead,
                                   Parts &... sParts)
    {
        size_t pos2 = cStr.find(cSeparator, uPosition);
        if (pos2 == std::string::npos) {
            return false;
        }
        cHead = cStr.substr(uPosition, pos2 - uPosition);

        return splitStringHelper(cStr, pos2 + 1, cSeparator, sParts...);
    }

    template <class... Parts>
    inline bool splitString (const std::string &str,
                             const std::string &separator,
                             Parts &... parts)
    {
        return splitStringHelper(str, 0, separator, parts...);
    }

    std::string extract (std::string &cText, char cDelimiter);
    std::string extract (const std::string &cText, char cDelimiter, size_t &uOffset);

    template <typename T>
    T fromString (const std::string &cText)
    {
        T value;

        std::istringstream stream(cText);
        stream >> value;

        if (stream.fail()) {
            throw std::runtime_error("fromString: unable to parse value");
        }

        return value;
    }

    template <typename T>
    bool fromString (const std::string &cText, T &value)
    {
        std::istringstream stream(cText);
        stream >> value;

        return !stream.fail();
    }

    template <typename T>
    std::vector <T> fromDelimitedString (const std::string &cSource, char cDelimiter)
    {
        std::vector <T> vData;
        for (size_t offset = 0; offset != cSource.size();) {
            vData.emplace_back(fromString <T>(extract(cSource, cDelimiter, offset)));
        }

        return vData;
    }

    template <typename T>
    bool fromDelimitedString (const std::string &cSource,
                              char cDelimiter,
                              std::vector <T> &vData)
    {
        for (size_t offset = 0; offset != cSource.size();) {
            T value;
            if (!fromString <T>(extract(cSource, cDelimiter, offset), value)) {
                return false;
            }

            vData.emplace_back(value);
        }

        return true;
    }

    template <typename T>
    std::string toString (const T &value)
    {
        std::ostringstream stream;
        stream << value;

        return stream.str();
    }

    template <typename T>
    void toString (const T &value, std::string &cText)
    {
        std::ostringstream stream;
        stream << value;
        cText += stream.str();
    }

    bool loadFileToString (const std::string &cFilepath, std::string &cBuf);
    bool saveStringToFile (const std::string &cFilepath, const std::string &cBuf);

    std::string base64Encode (std::string const &cEncodableString);
    std::string base64Decode (std::string const &cEncodedString);

    std::string ipAddressToString (uint32_t uIp);
    uint32_t stringToIpAddress (const std::string &cAddress);
    bool parseIpAddressAndPort (uint32_t &uIp, uint32_t &uPort, const std::string &cAddress);

    std::string timeIntervalToString (uint64_t uIntervalInSeconds);

} // namespace Common
