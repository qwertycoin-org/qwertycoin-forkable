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

#include <cstdio>
#include <fstream>
#include <iomanip>

#include <Common/StringTools.h>

namespace Common {

    namespace {

        const uint8_t uCharacterValues[256] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff
        };

    } // namespace

    std::string asString (const void *data, size_t uSize)
    {
        return std::string(static_cast<const char *>(data), uSize);
    }

    std::string asString (const std::vector <uint8_t> &vData)
    {
        return std::string(reinterpret_cast<const char *>(vData.data()), vData.size());
    }

    std::vector <uint8_t> asBinaryArray (const std::string &cData)
    {
        auto dataPtr = reinterpret_cast<const uint8_t *>(cData.data());

        return std::vector <uint8_t>(dataPtr, dataPtr + cData.size());
    }

    uint8_t fromHex (char cCharacter)
    {
        uint8_t value = uCharacterValues[static_cast<unsigned char>(cCharacter)];
        if (value > 0x0f) {
            throw std::runtime_error("fromHex: invalid character");
        }

        return value;
    }

    bool fromHex (char cCharacter, uint8_t &uValue)
    {
        if (uCharacterValues[static_cast<unsigned char>(cCharacter)] > 0x0f) {
            return false;
        }

        uValue = uCharacterValues[static_cast<unsigned char>(cCharacter)];

        return true;
    }

    size_t fromHex (const std::string &cText, void *data, size_t uBufferSize)
    {
        if ((cText.size() & 1) != 0) {
            throw std::runtime_error("fromHex: invalid string size");
        }

        if (cText.size() >> 1 > uBufferSize) {
            throw std::runtime_error("fromHex: invalid buffer size");
        }

        for (size_t i = 0; i < cText.size() >> 1; ++i) {
            static_cast<uint8_t *>(data)[i] =
                fromHex(cText[i << 1]) << 4 | fromHex(cText[(i << 1) + 1]);
        }

        return cText.size() >> 1;
    }

    bool fromHex (const std::string &cText,
                  void *data, size_t uBufferSize,
                  size_t &uSize)
    {
        if ((cText.size() & 1) != 0) {
            return false;
        }

        if (cText.size() >> 1 > uBufferSize) {
            return false;
        }

        for (size_t i = 0; i < cText.size() >> 1; ++i) {
            uint8_t value1;
            if (!fromHex(cText[i << 1], value1)) {
                return false;
            }

            uint8_t value2;
            if (!fromHex(cText[(i << 1) + 1], value2)) {
                return false;
            }

            static_cast<uint8_t *>(data)[i] = value1 << 4 | value2;
        }

        uSize = cText.size() >> 1;

        return true;
    }

    std::vector <uint8_t> fromHex (const std::string &cText)
    {
        if ((cText.size() & 1) != 0) {
            throw std::runtime_error("fromHex: invalid string size");
        }

        std::vector <uint8_t> data(cText.size() >> 1);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = fromHex(cText[i << 1]) << 4 | fromHex(cText[(i << 1) + 1]);
        }

        return data;
    }

    bool fromHex (const std::string &cText, std::vector <uint8_t> &vData)
    {
        if ((cText.size() & 1) != 0) {
            return false;
        }

        for (size_t i = 0; i < cText.size() >> 1; ++i) {
            uint8_t value1;
            if (!fromHex(cText[i << 1], value1)) {
                return false;
            }

            uint8_t value2;
            if (!fromHex(cText[(i << 1) + 1], value2)) {
                return false;
            }

            vData.push_back(value1 << 4 | value2);
        }

        return true;
    }

    std::string toHex (const void *data, size_t uSize)
    {
        std::string cText;
        for (size_t i = 0; i < uSize; ++i) {
            cText += "0123456789abcdef"[static_cast<const uint8_t *>(data)[i] >> 4];
            cText += "0123456789abcdef"[static_cast<const uint8_t *>(data)[i] & 15];
        }

        return cText;
    }

    void toHex (const void *data, size_t uSize, std::string &cText)
    {
        for (size_t i = 0; i < uSize; ++i) {
            cText += "0123456789abcdef"[static_cast<const uint8_t *>(data)[i] >> 4];
            cText += "0123456789abcdef"[static_cast<const uint8_t *>(data)[i] & 15];
        }
    }

    std::string toHex (const std::vector <uint8_t> &vData)
    {
        std::string text;

        for (unsigned char i : vData) {
            text += "0123456789abcdef"[i >> 4];
            text += "0123456789abcdef"[i & 15];
        }

        return text;
    }

    void toHex (const std::vector <uint8_t> &vData, std::string &cText)
    {
        for (unsigned char i : vData) {
            cText += "0123456789abcdef"[i >> 4];
            cText += "0123456789abcdef"[i & 15];
        }
    }

    std::string extract (std::string &cText, char cDelimiter)
    {
        size_t uDelimiterPosition = cText.find(cDelimiter);
        std::string subText;

        if (uDelimiterPosition != std::string::npos) {
            subText = cText.substr(0, uDelimiterPosition);
            cText = cText.substr(uDelimiterPosition + 1);
        } else {
            subText.swap(cText);
        }

        return subText;
    }

    std::string extract (const std::string &cText,
                         char cDelimiter,
                         size_t &uOffset)
    {
        size_t uDelimiterPosition = cText.find(cDelimiter, uOffset);
        if (uDelimiterPosition != std::string::npos) {
            uOffset = uDelimiterPosition + 1;
            return cText.substr(uOffset, uDelimiterPosition);
        } else {
            uOffset = cText.size();
            return cText.substr(uOffset);
        }
    }

    namespace {

        static const std::string cBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz"
                                                "0123456789+/";

        bool isBase64 (unsigned char cChar)
        {
            return (isalnum(cChar) || (cChar == '+') || (cChar == '/'));
        }

    } // namespace

    std::string base64Encode (std::string const &cEncodableString)
    {
        static const char *cEncodingTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "0123456789+/";

        const size_t uResultSize = 4 * ((cEncodableString.size() + 2) / 3);
        std::string result;
        result.reserve(uResultSize);

        for (size_t i = 0; i < cEncodableString.size(); i += 3) {
            auto a = static_cast<size_t>(static_cast<unsigned char>(cEncodableString[i]));
            size_t b =
                i + 1 < cEncodableString.size() ? static_cast<size_t>(cEncodableString[i + 1]) : 0;
            size_t c =
                i + 2 < cEncodableString.size() ? static_cast<size_t>(cEncodableString[i + 2]) : 0;

            result.push_back(cEncodingTable[a >> 2]);
            result.push_back(cEncodingTable[((a & 0x3) << 4) | (b >> 4)]);
            if (i + 1 < cEncodableString.size()) {
                result.push_back(cEncodingTable[((b & 0xF) << 2) | (c >> 6)]);
                if (i + 2 < cEncodableString.size()) {
                    result.push_back(cEncodingTable[c & 0x3F]);
                }
            }
        }

        while (result.size() != uResultSize) {
            result.push_back('=');
        }

        return result;
    }

    std::string base64Decode (const std::string &cEncodedString)
    {
        size_t inLen = cEncodedString.size();
        size_t i = 0;
        size_t j = 0;
        size_t in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string result;

        while (inLen-- && (cEncodedString[in_] != '=') && isBase64(cEncodedString[in_])) {
            char_array_4[i++] = cEncodedString[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    char_array_4[i] = (unsigned char) cBase64Chars.find(char_array_4[i]);
                }

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++) {
                    result += char_array_3[i];
                }

                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++) {
                char_array_4[j] = 0;
            }

            for (j = 0; j < 4; j++) {
                char_array_4[j] = (unsigned char) cBase64Chars.find(char_array_4[j]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++) {
                result += char_array_3[j];
            }
        }

        return result;
    }

    bool loadFileToString (const std::string &cFilepath, std::string &cBuf)
    {
        try {
            std::ifstream fStream;
            fStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fStream.open(cFilepath, std::ios_base::binary | std::ios_base::in | std::ios::ate);

            size_t fileSize = static_cast<size_t>(fStream.tellg());
            cBuf.resize(fileSize);

            if (fileSize > 0) {
                fStream.seekg(0, std::ios::beg);
                fStream.read(&cBuf[0], cBuf.size());
            }
        } catch (const std::exception &) {
            return false;
        }

        return true;
    }

    bool saveStringToFile (const std::string &cFilepath, const std::string &cBuf)
    {
        try {
            std::ofstream fStream;
            fStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fStream
                .open(cFilepath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
            fStream << cBuf;
        } catch (const std::exception &) {
            return false;
        }

        return true;
    }

    std::string ipAddressToString (uint32_t uIp)
    {
        uint8_t uBytes[4];
        uBytes[0] = uIp & 0xFF;
        uBytes[1] = (uIp >> 8) & 0xFF;
        uBytes[2] = (uIp >> 16) & 0xFF;
        uBytes[3] = (uIp >> 24) & 0xFF;

        char cBuf[16];
        sprintf(cBuf, "%d.%d.%d.%d", uBytes[0], uBytes[1], uBytes[2], uBytes[3]);

        return std::string(cBuf);
    }

    uint32_t stringToIpAddress (const std::string &cAddress)
    {
        uint32_t v[4];

        if (sscanf(cAddress.c_str(), "%d.%d.%d.%d", &v[0], &v[1], &v[2], &v[3]) != 4) {
            return false;
        }

        for (unsigned int i : v) {
            if (i > 0xff) {
                return false;
            }
        }

        return ((v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0]);
    }

    bool parseIpAddressAndPort (uint32_t &uIp, uint32_t &uPort, const std::string &cAddress)
    {
        uint32_t v[4];
        uint32_t localPort;

        if (sscanf(cAddress.c_str(), "%d.%d.%d.%d:%d", &v[0], &v[1], &v[2], &v[3], &localPort) !=
            5) {
            return false;
        }

        for (unsigned int i : v) {
            if (i > 0xff) {
                return false;
            }
        }

        uIp = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];

        uPort = localPort;

        return true;
    }

    std::string timeIntervalToString (uint64_t uIntervalInSeconds)
    {
        auto tail = uIntervalInSeconds;

        auto days = tail / (60 * 60 * 24);
        tail = tail % (60 * 60 * 24);
        auto hours = tail / (60 * 60);
        tail = tail % (60 * 60);
        auto minutes = tail / (60);
        tail = tail % (60);
        auto seconds = tail;

        std::stringstream ss;
        ss << "d" << days << std::setfill('0')
           << ".h" << std::setw(2) << hours
           << ".m" << std::setw(2) << minutes
           << ".s" << std::setw(2) << seconds;

        return ss.str();
    }

    bool startsWith (const std::string &cStr1, const std::string &cStr2)
    {
        if (cStr1.length() < cStr2.length()) {
            return false;
        }

        return cStr1.compare(0, cStr2.length(), cStr2) == 0;
    }

    bool endsWith (const std::string &cStr1, const std::string &cStr2)
    {
        if (cStr1.length() < cStr2.length()) {
            return false;
        }

        return cStr1.compare(cStr1.length() - cStr2.length(), cStr2.length(), cStr2) == 0;
    }

} // namespace Common
