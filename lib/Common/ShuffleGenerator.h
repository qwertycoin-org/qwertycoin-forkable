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

#include <unordered_map>

#include <Crypto/random.h>

template<typename T>
class ShuffleGenerator
{
public:
    explicit ShuffleGenerator(T n) : count(n), N(n)
    {
    }

    bool empty() const { return count == 0; }

    void reset()
    {
        count = N;
        selected.clear();
    }

    T operator()()
    {
        if (count == 0) {
            throw std::runtime_error("shuffle sequence ended");
        }

        T value = Random::randomValue<T>(0, --count);

        auto rValIt = selected.find(count);
        auto rVal = rValIt != selected.end() ? rValIt->second : count;

        auto lValIt = selected.find(value);

        if (lValIt != selected.end()) {
            value = lValIt->second;
            lValIt->second = rVal;
        } else {
            selected[value] = rVal;
        }

        return value;
    }

private:
    std::unordered_map<T, T> selected;
    T count;
    const T N;
};
