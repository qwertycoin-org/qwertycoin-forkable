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

#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace Tools {

    template <typename OutputIt, typename T>
    typename std::enable_if <std::is_integral <T>::value &&
                             std::is_unsigned <T>::value, void>::type writeVarint (OutputIt &&sDest,
                                                                                   T i)
    {
        while (i >= 0x80) {
            *sDest++ = (static_cast<char>(i) & 0x7f) | 0x80;
            i >>= 7;
        }
        *sDest++ = static_cast<char>(i);
    }

    template <typename TType>
    std::string getVarintData (const TType &v)
    {
        std::stringstream ss;

        writeVarint(std::ostreambuf_iterator <char>(ss), v);

        return ss.str();
    }

    template <int iBits, typename InputIt, typename T>
    typename std::enable_if <std::is_integral <T>::value
                             && std::is_unsigned <T>::value
                             && 0 <= iBits && iBits <= std::numeric_limits <T>::digits,
        int>::type
    readVarint (InputIt &&sFirst, InputIt &&sLast, T &i)
    {
        int read = 0;
        i = 0;
        for (int shift = 0;; shift += 7) {
            if (sFirst == sLast) {
                return read; // End of input.
            }

            unsigned char byte = *sFirst++;
            ++read;
            if (shift + 7 >= iBits && byte >= 1 << (iBits - shift)) {
                return -1; // Overflow.
            }

            if (byte == 0 && shift != 0) {
                return -2; // Non-canonical representation.
            }
            i |= static_cast<T>(byte & 0x7f) << shift;

            if ((byte & 0x80) == 0) {
                break;
            }
        }
        return read;
    }

    template <typename InputIt, typename T>
    int readVarint (InputIt &&first, InputIt &&last, T &i)
    {
        return readVarint <std::numeric_limits <T>::digits, InputIt, T>(std::move(first),
                                                                        std::move(last),
                                                                        i);
    }

} // namespace Tools
