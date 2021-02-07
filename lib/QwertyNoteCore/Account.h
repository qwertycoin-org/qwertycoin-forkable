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

#include <Crypto/Crypto.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>

namespace QwertyNote {

class ISerializer;

class AccountBase
{
public:
    AccountBase();

    void generate();
    void generateDeterministic();
    Crypto::FSecretKey generateKey(
        const Crypto::FSecretKey &recovery_key = Crypto::FSecretKey(),
        bool recover = false,
        bool two_random = false);

    static void generateViewFromSpend(Crypto::FSecretKey &, Crypto::FSecretKey &, Crypto::FPublicKey &);
    static void generateViewFromSpend(Crypto::FSecretKey &, Crypto::FSecretKey &);

    const FAccountKeys &getAccountKeys() const;
    void setAccountKeys(const FAccountKeys& keys);

    uint64_t getCreateTime() const { return mCreationTimestamp; }
    void setCreateTime(uint64_t val) { mCreationTimestamp = val; }

    void serialize(ISerializer &s);

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int)
    {
        a &mKeys;
        a &mCreationTimestamp;
    }

private:
    void setNull();

    FAccountKeys mKeys;
    uint64_t mCreationTimestamp;
};

} // namespace QwertyNote
