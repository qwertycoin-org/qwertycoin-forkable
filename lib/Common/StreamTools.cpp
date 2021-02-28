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

#include <stdexcept>

#include <Common/IInputStream.h>
#include <Common/IOutputStream.h>
#include <Common/StreamTools.h>

namespace Common {

    void read (IInputStream &in, void *data, size_t uSize)
    {
        while (uSize > 0) {
            size_t readSize = in.readSome(data, uSize);
            if (readSize == 0) {
                throw std::runtime_error("Failed to read from IInputStream");
            }

            data = static_cast<uint8_t *>(data) + readSize;
            uSize -= readSize;
        }
    }

    void read (IInputStream &in, int8_t &iValue)
    {
        read(in, &iValue, sizeof(iValue));
    }

    void read (IInputStream &in, int16_t &iValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &iValue, sizeof(iValue));
    }

    void read (IInputStream &in, int32_t &iValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &iValue, sizeof(iValue));
    }

    void read (IInputStream &in, int64_t &iValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &iValue, sizeof(iValue));
    }

    void read (IInputStream &in, uint8_t &uValue)
    {
        read(in, &uValue, sizeof(uValue));
    }

    void read (IInputStream &in, uint16_t &uValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &uValue, sizeof(uValue));
    }

    void read (IInputStream &in, uint32_t &uValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &uValue, sizeof(uValue));
    }

    void read (IInputStream &in, uint64_t &uValue)
    {
        // TODO: Convert from little endian on big endian platforms
        read(in, &uValue, sizeof(uValue));
    }

    void read (IInputStream &in, std::vector <uint8_t> &vData, size_t uSize)
    {
        vData.resize(uSize);
        read(in, vData.data(), uSize);
    }

    void read (IInputStream &in, std::string &cData, size_t uSize)
    {
        std::vector <char> temp(uSize);
        read(in, temp.data(), uSize);
        cData.assign(temp.data(), uSize);
    }

    void readVarint (IInputStream &in, uint8_t &uValue)
    {
        uint8_t temp = 0;
        for (uint8_t shift = 0;; shift += 7) {
            uint8_t piece;
            read(in, piece);
            if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
                throw std::runtime_error("readVarint, uValue overflow");
            }

            temp |= static_cast<size_t>(piece & 0x7f) << shift;
            if ((piece & 0x80) == 0) {
                if (piece == 0 && shift != 0) {
                    throw std::runtime_error("readVarint, uInvalid uValue representation");
                }

                break;
            }
        }

        uValue = temp;
    }

    void readVarint (IInputStream &in, uint16_t &uValue)
    {
        uint16_t temp = 0;
        for (uint8_t shift = 0;; shift += 7) {
            uint8_t piece;
            read(in, piece);
            if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
                throw std::runtime_error("readVarint, uValue overflow");
            }

            temp |= static_cast<size_t>(piece & 0x7f) << shift;
            if ((piece & 0x80) == 0) {
                if (piece == 0 && shift != 0) {
                    throw std::runtime_error("readVarint, uInvalid uValue representation");
                }

                break;
            }
        }

        uValue = temp;
    }

    void readVarint (IInputStream &in, uint32_t &uValue)
    {
        uint32_t temp = 0;
        for (uint8_t shift = 0;; shift += 7) {
            uint8_t piece;
            read(in, piece);
            if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
                throw std::runtime_error("readVarint, uValue overflow");
            }

            temp |= static_cast<size_t>(piece & 0x7f) << shift;
            if ((piece & 0x80) == 0) {
                if (piece == 0 && shift != 0) {
                    throw std::runtime_error("readVarint, uInvalid uValue representation");
                }

                break;
            }
        }

        uValue = temp;
    }

    void readVarint (IInputStream &in, uint64_t &uValue)
    {
        uint64_t temp = 0;
        for (uint8_t shift = 0;; shift += 7) {
            uint8_t piece;
            read(in, piece);
            if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
                throw std::runtime_error("readVarint, uValue overflow");
            }

            temp |= static_cast<uint64_t>(piece & 0x7f) << shift;
            if ((piece & 0x80) == 0) {
                if (piece == 0 && shift != 0) {
                    throw std::runtime_error("readVarint, uInvalid uValue representation");
                }

                break;
            }
        }

        uValue = temp;
    }

    void write (IOutputStream &out, const void *data, size_t uSize)
    {
        while (uSize > 0) {
            size_t writtenSize = out.writeSome(data, uSize);
            if (writtenSize == 0) {
                throw std::runtime_error("Failed to write to IOutputStream");
            }

            data = static_cast<const uint8_t *>(data) + writtenSize;
            uSize -= writtenSize;
        }
    }

    void write (IOutputStream &out, int8_t iValue)
    {
        write(out, &iValue, sizeof(iValue));
    }

    void write (IOutputStream &out, int16_t iValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &iValue, sizeof(iValue));
    }

    void write (IOutputStream &out, int32_t iValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &iValue, sizeof(iValue));
    }

    void write (IOutputStream &out, int64_t iValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &iValue, sizeof(iValue));
    }

    void write (IOutputStream &out, uint8_t uValue)
    {
        write(out, &uValue, sizeof(uValue));
    }

    void write (IOutputStream &out, uint16_t uValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &uValue, sizeof(uValue));
    }

    void write (IOutputStream &out, uint32_t uValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &uValue, sizeof(uValue));
    }

    void write (IOutputStream &out, uint64_t uValue)
    {
        // TODO: Convert to little endian on big endian platforms
        write(out, &uValue, sizeof(uValue));
    }

    void write (IOutputStream &out, const std::vector <uint8_t> &vData)
    {
        write(out, vData.data(), vData.size());
    }

    void write (IOutputStream &out, const std::string &cData)
    {
        write(out, cData.data(), cData.size());
    }

    void writeVarint (IOutputStream &out, uint32_t uValue)
    {
        while (uValue >= 0x80) {
            write(out, static_cast<uint8_t>(uValue | 0x80));
            uValue >>= 7;
        }

        write(out, static_cast<uint8_t>(uValue));
    }

    void writeVarint (IOutputStream &out, uint64_t uValue)
    {
        while (uValue >= 0x80) {
            write(out, static_cast<uint8_t>(uValue | 0x80));
            uValue >>= 7;
        }

        write(out, static_cast<uint8_t>(uValue));
    }

} // namespace Common
