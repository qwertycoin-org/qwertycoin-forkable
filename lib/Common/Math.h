// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2020, The Karbowanec developers
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

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <Common/StringTools.h>

using namespace std;

namespace Common {

    template <class T>
    double meanValue (const std::vector <T> &vValue)
    {
        if (vValue.empty()) {
            return T();
        }

        T sum = std::accumulate(vValue.begin(), vValue.end(), T());

        return double(sum) / double(vValue.size());
    }

    template <class T>
    double stddevValue (const std::vector <T> &vValue)
    {
        if (vValue.size() < 2) {
            return T();
        }

        double mean = meanValue(vValue);
        std::vector <T> diff(vValue.size());
        std::transform(vValue.begin(),
                       vValue.end(),
                       diff.begin(),
                       [mean] (T x) {
            return x - mean;
        });
        T sqSum = std::inner_product(diff.begin(), diff.end(), diff.begin(), T());

        return std::sqrt(double(sqSum) / double(vValue.size()));
    }

    template <class T>
    double medianValue (std::vector <T> &vValue)
    {
        if (vValue.empty()) {
            return T();
        }

        if (vValue.size() == 1) {
            return vValue[0];
        }

        auto n = (vValue.size()) / 2;
        std::sort(vValue.begin(), vValue.end());

        if (vValue.size() % 2) {
            // 1, 3, 5...
            return vValue[n];
        } else {
            // 2, 4, 6...
            return double(vValue[n - 1] + vValue[n]) / 2.0;
        }
    }

    template <typename Target, typename Source>
    void integerCastThrow (const Source &sArg)
    {
        throw std::out_of_range("Cannot convert value "
                                + Common::toString(sArg)
                                + " to integer in range ["
                                + Common::toString(std::numeric_limits <Target>::min()) + ".."
                                + Common::toString(std::numeric_limits <Target>::max()) + "]");
    }

    template <typename Target, typename Source>
    inline Target integerCastImpl (const Source &sArg, std::true_type, std::true_type)
    {
        // Both unsigned
        if (sArg > std::numeric_limits <Target>::max()) {
            integerCastThrow <Target>(sArg);
        }

        return static_cast<Target>(sArg);
    }

    template <typename Target, typename Source>
    inline Target integerCastImpl (const Source &sArg, std::false_type, std::false_type)
    {
        // Both signed
        if (sArg > std::numeric_limits <Target>::max()) {
            integerCastThrow <Target>(sArg);
        }

        if (sArg < std::numeric_limits <Target>::min()) {
            integerCastThrow <Target>(sArg);
        }

        return static_cast<Target>(sArg);
    }

    template <typename Target, typename Source>
    inline Target integerCastImpl (const Source &sArg, std::true_type, std::false_type)
    {
        // Signed to unsigned
        typedef typename std::make_unsigned <Source>::type USource;
        if (sArg < 0) {
            integerCastThrow <Target>(sArg);
        }
        if (static_cast<USource>(sArg) > std::numeric_limits <Target>::max()) {
            integerCastThrow <Target>(sArg);
        }

        return static_cast<Target>(sArg);
    }

    template <typename Target, typename Source>
    inline Target integerCastImpl (const Source &sArg, std::false_type, std::true_type)
    {
        // Unsigned to signed
        typedef typename std::make_unsigned <Target>::type UTarget;
        if (sArg > static_cast<UTarget>(std::numeric_limits <Target>::max())) {

        }

        return static_cast<Target>(sArg);
    }

    template <typename Target, typename Source>
    // Source integral
    inline Target integerCastIsIntegral (const Source &sArg, std::true_type)
    {
        return integerCastImpl <Target, Source>(sArg, std::is_unsigned <Target> {},
                                                std::is_unsigned <Source> {});
    }

    inline bool hasSign (const std::string &cArg)
    {
        size_t uPosition = 0;
        while (uPosition != cArg.size() && isspace(cArg[uPosition])) {
            uPosition += 1;
        }

        return uPosition != cArg.size() && cArg[uPosition] == '-';
    }

    inline bool hasTail (const std::string &sArg, size_t &uPosition)
    {
        while (uPosition != sArg.size() && isspace(sArg[uPosition])) {
            uPosition += 1;
        }

        return uPosition != sArg.size();
    }

    template <typename Target, typename Source>
    // Source not integral (string convertible)
    inline Target integerCastIsIntegral (const Source &sArg, std::false_type)
    {
        // Creates tmp object if neccessary
        const std::string &cArg = sArg;
        // Crazy stupid C++ standard :(
        if (std::is_unsigned <Target>::value) {
            if (hasSign(cArg)) {
                throw std::out_of_range("Cannot convert string '"
                                        + cArg
                                        + "' to integer, must be >= 0");
            }

            size_t pos = 0;
            auto val = std::stoull(cArg, &pos);
            if (hasTail(cArg, pos)) {
                throw std::out_of_range("Cannot convert string '"
                                        + cArg
                                        + "' to integer, excess characters not allowed");
            }

            return integerCastIsIntegral <Target>(val, std::true_type {});
        }

        size_t pos = 0;
        auto val = std::stoll(cArg, &pos);
        if (hasTail(cArg, pos)) {
            throw std::out_of_range("Cannot convert string '"
                                    + cArg
                                    + "' to integer, excess characters '"
                                    + cArg.substr(pos)
                                    + "' not allowed");
        }

        return integerCastIsIntegral <Target>(val, std::true_type {});
    }

    template <typename Target, typename Source>
    inline Target integerCast (const Source &sArg)
    {
        static_assert(
            std::is_integral <Target>::value,
            "Target sType must be integral, source either integral or string convertible");

        return integerCastIsIntegral <Target, Source>(sArg, std::is_integral <Source> {});
    }

} // namespace Common
