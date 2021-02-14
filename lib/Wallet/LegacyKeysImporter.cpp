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

#include <system_error>
#include <vector>

#include <Common/StringTools.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/Currency.h>
#include <QwertyNoteCore/CryptoNoteTools.h>

#include <Serialization/SerializationTools.h>

#include <Wallet/LegacyKeysImporter.h>
#include <Wallet/WalletUtils.h>
#include <Wallet/WalletErrors.h>

#include <WalletLegacy/WalletLegacySerializer.h>
#include <WalletLegacy/WalletUserTransactionsCache.h>

using namespace Crypto;

namespace {

struct keys_file_data
{
    void serialize(QwertyNote::ISerializer &s)
    {
        s(iv, "iv");
        s(account_data, "account_data");
    }

    Chacha8Iv iv;
    std::string account_data;
};

void loadKeysFromFile(const std::string &filename,
                      const std::string &password,
                      QwertyNote::AccountBase &account)
{
    keys_file_data keys_file_data;
    std::string buf;

    if (!Common::loadFileToString(filename, buf)) {
        throw std::system_error(
            make_error_code(QwertyNote::Error::INTERNAL_WALLET_ERROR),
            "failed to load \"" + filename + '\"'
        );
    }

    if (!QwertyNote::fromBinaryArray(keys_file_data, Common::asBinaryArray(buf))) {
        throw std::system_error(
            make_error_code(QwertyNote::Error::INTERNAL_WALLET_ERROR),
            "failed to deserialize \"" + filename + '\"'
        );
    }

    Chacha8Key key;
    CnContext cn_context;
    generateChacha8Key(cn_context, password, key);
    std::string account_data;
    account_data.resize(keys_file_data.account_data.size());
    chacha8(keys_file_data.account_data.data(),
            keys_file_data.account_data.size(),
            key,
            keys_file_data.iv,
            &account_data[0]);

    if (!QwertyNote::loadFromBinaryKeyValue(account, account_data)) {
        throw std::system_error(make_error_code(QwertyNote::Error::WRONG_PASSWORD));
    }

    const QwertyNote::FAccountKeys& keys = account.getAccountKeys();
    QwertyNote::throwIfKeysMissmatch(keys.sViewSecretKey, keys.sAddress.sViewPublicKey);
    QwertyNote::throwIfKeysMissmatch(keys.sSpendSecretKey, keys.sAddress.sSpendPublicKey);
}

} // namespace

namespace QwertyNote {

void importLegacyKeys(const std::string &legacyKeysFilename,
                      const std::string &password,
                      std::ostream &destination)
{
    QwertyNote::AccountBase account;

    loadKeysFromFile(legacyKeysFilename, password, account);

    QwertyNote::WalletUserTransactionsCache transactionsCache;
    std::string cache;

    QwertyNote::WalletLegacySerializer importer(account, transactionsCache);
    std::vector<Crypto::FHash> safeTxes;
    importer.serialize(destination, password, false, cache, safeTxes);
}

} // namespace QwertyNote
