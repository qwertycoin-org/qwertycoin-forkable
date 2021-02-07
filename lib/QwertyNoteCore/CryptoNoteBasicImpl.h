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

#include <Common/StringTools.h>

#include <Crypto/Crypto.h>
#include <Crypto/Hash.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>

namespace QwertyNote {

template<class t_array>
struct array_hasher : std::unary_function<t_array &, size_t>
{
    size_t operator()(const t_array &val) const
    {
        return boost::hash_range(&val.uData[0], &val.uData[sizeof(val.uData)]);
    }
};

uint64_t getPenalizedAmount(uint64_t amount, size_t medianSize, size_t currentBlockSize);
std::string getAccountAddressAsStr(uint64_t prefix, const FAccountPublicAddress &adr);
bool parseAccountAddressString(uint64_t &prefix, FAccountPublicAddress &adr, const std::string &str);
bool is_coinbase(const FTransaction &tx);

bool operator==(const QwertyNote::FTransaction &a, const QwertyNote::FTransaction &b);
bool operator==(const QwertyNote::FBlock &a, const QwertyNote::FBlock &b);

} // namespace QwertyNote

template <class T>
std::ostream &print256(std::ostream &o, const T &v)
{
    return o << Common::podToHex(v);
}

bool parse_hash256(const std::string &str_hash, Crypto::FHash &hash);

namespace Crypto {

inline std::ostream &operator<<(std::ostream &o, const Crypto::FPublicKey &v)
{
    return print256(o, v);
}

inline std::ostream &operator<<(std::ostream &o, const Crypto::FSecretKey &v)
{
    return print256(o, v);
}

inline std::ostream &operator<<(std::ostream &o, const Crypto::FKeyDerivation &v)
{
    return print256(o, v);
}

inline std::ostream &operator<<(std::ostream &o, const Crypto::FKeyImage &v)
{
    return print256(o, v);
}

inline std::ostream &operator<<(std::ostream &o, const Crypto::FSignature &v)
{
    return print256(o, v);
}

inline std::ostream &operator<<(std::ostream &o, const Crypto::FHash &v)
{
    return print256(o, v);
}

} // namespace QwertyNote
