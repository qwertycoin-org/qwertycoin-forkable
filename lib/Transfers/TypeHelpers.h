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

#include <cstring>
#include <functional>

#include <QwertyNoteCore/ITransaction.h>

namespace QwertyNote {

inline bool operator==(const FAccountPublicAddress &_v1, const FAccountPublicAddress &_v2)
{
    return memcmp(&_v1, &_v2, sizeof(FAccountPublicAddress)) == 0;
}

} // namespace QwertyNote

namespace std {

template<>
struct hash <QwertyNote::FAccountPublicAddress>
{
    size_t operator()(const QwertyNote::FAccountPublicAddress &val) const
    {
        size_t spend = *(reinterpret_cast<const size_t *>(&val.sSpendPublicKey));
        size_t view = *(reinterpret_cast<const size_t *>(&val.sViewPublicKey));
        return spend ^ view;
    }
};

} // namespace std
