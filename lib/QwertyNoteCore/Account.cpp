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

#include <Crypto/Crypto.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>

extern "C" {
#include <Crypto/Keccak.h>
} // extern "C"

namespace QwertyNote {

AccountBase::AccountBase()
{
    setNull();
}

void AccountBase::setNull()
{
    mKeys = AccountKeys();
}

void AccountBase::generate()
{
    Crypto::generateKeys(mKeys.address.spendPublicKey, mKeys.spendSecretKey);
    Crypto::generateKeys(mKeys.address.viewPublicKey, mKeys.viewSecretKey);
    mCreationTimestamp = time(nullptr);
}

void AccountBase::generateDeterministic()
{
    Crypto::FSecretKey second;
    Crypto::generateKeys(mKeys.address.spendPublicKey, mKeys.spendSecretKey);
    auto ssk = (uint8_t *)&mKeys.spendSecretKey;
    keccak(ssk, sizeof(Crypto::FSecretKey), (uint8_t *)&second, sizeof(Crypto::FSecretKey));
    Crypto::generateDeterministicKeys(mKeys.address.viewPublicKey, mKeys.viewSecretKey, second);
    mCreationTimestamp = time(nullptr);
}

Crypto::FSecretKey AccountBase::generateKey(
    const Crypto::FSecretKey &recovery_key,
    bool recover,
    bool two_random)
{
    Crypto::FSecretKey first = generateMKeys(mKeys.address.spendPublicKey, mKeys.spendSecretKey,
											 recovery_key,
											 recover
    );

    // rng for generating second set of keys is FHash of first rng.
    // means only one set of electrum-style words needed for recovery.
    Crypto::FSecretKey second;
    keccak((uint8_t*)&first, sizeof(Crypto::FSecretKey), (uint8_t*)&second, sizeof(Crypto::FSecretKey));

    generateMKeys(mKeys.address.viewPublicKey, mKeys.viewSecretKey, second, !two_random);

    struct tm timestamp;
    timestamp.tm_year = 2016 - 1900; // year 2016
    timestamp.tm_mon = 5 - 1; // month May
    timestamp.tm_mday = 30; // 30 of May
    timestamp.tm_hour = 0;
    timestamp.tm_min = 0;
    timestamp.tm_sec = 0;

    if (recover) {
        mCreationTimestamp = mktime(&timestamp);
    } else {
        mCreationTimestamp = time(nullptr);
    }

    return first;
}

void AccountBase::generateViewFromSpend(
    Crypto::FSecretKey &spend,
    Crypto::FSecretKey &viewSecret,
    Crypto::FPublicKey &viewPublic)
{
    Crypto::FSecretKey viewKeySeed;
    keccak((uint8_t *)&spend, sizeof(spend), (uint8_t *)&viewKeySeed, sizeof(viewKeySeed));
    Crypto::generateDeterministicKeys(viewPublic, viewSecret, viewKeySeed);
}

void AccountBase::generateViewFromSpend(Crypto::FSecretKey &spend, Crypto::FSecretKey &viewSecret)
{
    // If we don't need the pub key
    Crypto::FPublicKey unusedDummyVariable;
    generateViewFromSpend(spend, viewSecret, unusedDummyVariable);
}

const AccountKeys &AccountBase::getAccountKeys() const
{
    return mKeys;
}

void AccountBase::setAccountKeys(const AccountKeys &keys)
{
    mKeys = keys;
}

void AccountBase::serialize(ISerializer &s)
{
    s(mKeys, "mKeys");
    s(mCreationTimestamp, "mCreationTimestamp");
}

} // namespace QwertyNote
