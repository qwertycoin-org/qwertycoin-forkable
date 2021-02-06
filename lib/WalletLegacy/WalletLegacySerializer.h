// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, Karbo developers
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

#include <istream>
#include <ostream>
#include <vector>
#include <Crypto/Chacha8.h>
#include <Crypto/Hash.h>

namespace QwertyNote {

class AccountBase;
class ISerializer;

} // namespace QwertyNote

namespace QwertyNote {

class WalletUserTransactionsCache;

class WalletLegacySerializer
{
public:
    WalletLegacySerializer(QwertyNote::AccountBase &account,
        WalletUserTransactionsCache &transactionsCache);

    void serialize(
        std::ostream &stream,
        const std::string &password,
        bool saveDetailed,
        const std::string &cache,
        const std::vector<Crypto::FHash> & safeTxes);
    void deserialize(
        std::istream &stream,
        const std::string &password,
        std::string &cache,
        std::vector<Crypto::FHash> & safeTxes);

private:
    void saveKeys(QwertyNote::ISerializer &serializer);
    void loadKeys(QwertyNote::ISerializer &serializer);

    Crypto::Chacha8Iv encrypt(
        const std::string &plain,
        const std::string &password,
        std::string &cipher);
    void decrypt(
        const std::string &cipher,
        std::string &plain,
        Crypto::Chacha8Iv iv,
        const std::string &password);

    QwertyNote::AccountBase &account;
    WalletUserTransactionsCache &transactionsCache;
    const uint32_t walletSerializationVersion;
};

extern uint32_t WALLET_LEGACY_SERIALIZATION_VERSION;

} // namespace QwertyNote
