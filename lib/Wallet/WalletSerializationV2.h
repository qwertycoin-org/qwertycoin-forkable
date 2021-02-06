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

#include <Common/IInputStream.h>
#include <Common/IOutputStream.h>
#include <Serialization/ISerializer.h>
#include <Transfers/TransfersSynchronizer.h>
#include <Wallet/WalletIndices.h>

namespace QwertyNote {

class WalletSerializerV2
{
public:
    WalletSerializerV2(
        ITransfersObserver &transfersObserver,
        Crypto::FPublicKey &viewPublicKey,
        Crypto::FSecretKey &viewSecretKey,
        uint64_t &actualBalance,
        uint64_t &pendingBalance,
        WalletsContainer &walletsContainer,
        TransfersSynchronizer &synchronizer,
        UnlockTransactionJobs &unlockTransactions,
        WalletTransactions &transactions,
        WalletTransfers &transfers,
        UncommitedTransactions &uncommitedTransactions,
        std::string &extra,
        uint32_t transactionSoftLockTime
    );

    void load(Common::IInputStream &source, uint8_t version);
    void save(Common::IOutputStream &destination, WalletSaveLevel saveLevel);

    std::unordered_set<Crypto::FPublicKey> &addedKeys();
    std::unordered_set<Crypto::FPublicKey> &deletedKeys();

    static const uint8_t MIN_VERSION = 6;
    static const uint8_t SERIALIZATION_VERSION = 6;

private:
    void loadKeyListAndBalances(QwertyNote::ISerializer &serializer, bool saveCache);
    void saveKeyListAndBalances(QwertyNote::ISerializer &serializer, bool saveCache);

    void loadTransactions(QwertyNote::ISerializer &serializer);
    void saveTransactions(QwertyNote::ISerializer &serializer);

    void loadTransfers(QwertyNote::ISerializer &serializer);
    void saveTransfers(QwertyNote::ISerializer &serializer);

    void loadTransfersSynchronizer(QwertyNote::ISerializer &serializer);
    void saveTransfersSynchronizer(QwertyNote::ISerializer &serializer);

    void loadUnlockTransactionsJobs(QwertyNote::ISerializer &serializer);
    void saveUnlockTransactionsJobs(QwertyNote::ISerializer &serializer);

private:
    ITransfersObserver &m_transfersObserver;
    uint64_t &m_actualBalance;
    uint64_t &m_pendingBalance;
    WalletsContainer &m_walletsContainer;
    TransfersSynchronizer &m_synchronizer;
    UnlockTransactionJobs &m_unlockTransactions;
    WalletTransactions &m_transactions;
    WalletTransfers &m_transfers;
    UncommitedTransactions &m_uncommitedTransactions;
    std::string &m_extra;
    uint32_t m_transactionSoftLockTime;

    std::unordered_set<Crypto::FPublicKey> m_addedKeys;
    std::unordered_set<Crypto::FPublicKey> m_deletedKeys;
};

} // namespace QwertyNote
