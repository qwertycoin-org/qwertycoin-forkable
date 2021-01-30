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
#include <numeric>
#include <vector>
#include <cmath>
#include <Common/StringTools.h>

using namespace std;

namespace Common {

template <class T>
double meanValue(const std::vector<T>& v)
{
    if (v.empty()) {
        return T();
    }

    T sum = std::accumulate(v.begin(), v.end(), T());

    return double(sum) / double(v.size());
}

template <class T>
double stddevValue(const std::vector<T>& v)
{
    if (v.size() < 2) {
        return T();
    }

    double mean = meanValue(v);
    std::vector<T> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mean](T x) { return x - mean; });
    T sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), T());

    return std::sqrt(double(sq_sum) / double(v.size()));
}

template <class T>
double medianValue(std::vector<T> &v)
{
    if (v.empty()) {
        return T();
    }

    if (v.size() == 1) {
        return v[0];
    }

    auto n = (v.size()) / 2;
    std::sort(v.begin(), v.end());
    if (v.size() % 2) { // 1, 3, 5...
        return v[n];
    } else { // 2, 4, 6...
        return double(v[n - 1] + v[n]) / 2.0;
    }
}

template<typename Target, typename Source>
void integerCastThrow(const Source &arg)
{
    throw std::out_of_range("Cannot convert value "
                            + Common::toString(arg)
                            + " to integer in range ["
                            + Common::toString(std::numeric_limits<Target>::min()) + ".."
                            + Common::toString(std::numeric_limits<Target>::max()) + "]");
}

template<typename Target, typename Source>
inline Target integerCastImpl(const Source &arg, std::true_type, std::true_type)
{
    // both unsigned
    if (arg > std::numeric_limits<Target>::max()) {
        integerCastThrow<Target>(arg);
    }

    return static_cast<Target>(arg);
}

template<typename Target, typename Source>
inline Target integerCastImpl(const Source &arg, std::false_type, std::false_type)
{
    // both signed
    if (arg > std::numeric_limits<Target>::max()) {
        integerCastThrow<Target>(arg);
    }

    if (arg < std::numeric_limits<Target>::min()) {
        integerCastThrow<Target>(arg);
    }

    return static_cast<Target>(arg);
}

template<typename Target, typename Source>
inline Target integerCastImpl(const Source &arg, std::true_type, std::false_type)
{
    // signed to unsigned
    typedef typename std::make_unsigned<Source>::type USource;
    if (arg < 0) {
        integerCastThrow<Target>(arg);
    }
    if (static_cast<USource>(arg) > std::numeric_limits<Target>::max()) {
        integerCastThrow<Target>(arg);
    }

    return static_cast<Target>(arg);
}

template<typename Target, typename Source>
inline Target integerCastImpl(const Source &arg, std::false_type, std::true_type)
{
    // unsigned to signed
    typedef typename std::make_unsigned<Target>::type UTarget;
    if (arg > static_cast<UTarget>(std::numeric_limits<Target>::max())) {

    }

    return static_cast<Target>(arg);
}

template<typename Target, typename Source>  // source integral
inline Target integerCastIsIntegral(const Source &arg, std::true_type)
{
    return integerCastImpl<Target, Source>(arg, std::is_unsigned<Target> {},
                                           std::is_unsigned<Source> {});
}

inline bool hasSign(const std::string &arg)
{
    size_t pos = 0;
    while (pos != arg.size() && isspace(arg[pos]))
        pos += 1;
    return pos != arg.size() && arg[pos] == '-';
}

inline bool hasTail(const std::string &arg, size_t &pos)
{
    while (pos != arg.size() && isspace(arg[pos]))
        pos += 1;
    return pos != arg.size();
}

template<typename Target, typename Source>  // source not integral (string convertible)
inline Target integerCastIsIntegral(const Source &arg, std::false_type)
{
    const std::string &sarg = arg;          // creates tmp object if neccessary
    if (std::is_unsigned<Target>::value) {  // Crazy stupid C++ standard :(
        if (hasSign(sarg)) {
            throw std::out_of_range("Cannot convert string '"
                                    + sarg
                                    + "' to integer, must be >= 0");
        }

        size_t pos = 0;
        auto val = std::stoull(sarg, &pos);
        if (hasTail(sarg, pos)) {
            throw std::out_of_range("Cannot convert string '"
                                    + sarg
                                    + "' to integer, excess characters not allowed");
        }

        return integerCastIsIntegral<Target>(val, std::true_type {});
    }

    size_t pos = 0;
    auto val = std::stoll(sarg, &pos);
    if (hasTail(sarg, pos)) {
        throw std::out_of_range("Cannot convert string '"
                                + sarg
                                + "' to integer, excess characters '"
                                + sarg.substr(pos)
                                + "' not allowed");
    }

    return integerCastIsIntegral<Target>(val, std::true_type {});
}

template<typename Target, typename Source>
inline Target integerCast(const Source &arg)
{
    static_assert(
        std::is_integral<Target>::value,
        "Target type must be integral, source either integral or string convertible");

    return integerCastIsIntegral<Target, Source>(arg, std::is_integral<Source> {});
}

} // namespace Common
