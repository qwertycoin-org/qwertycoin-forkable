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

#include <cstdint>
#include <future>
#include <system_error>
#include <unordered_set>
#include <Crypto/Crypto.h>
#include <CryptoNoteCore/CryptoNoteBasic.h>
#include <IObservable.h>
#include <IStreamSerializable.h>
#include <ITransfersSynchronizer.h>

namespace CryptoNote {
namespace QwertyNote {

struct CompleteBlock;

class IBlockchainSynchronizerObserver
{
public:
    virtual void synchronizationProgressUpdated(uint32_t processedBlockCount,
                                                uint32_t totalBlockCount)
    {
    }

    virtual void synchronizationCompleted(std::error_code result)
    {
    }
};

class IBlockchainConsumerObserver;

class IBlockchainConsumer : public IObservable<IBlockchainConsumerObserver>
{
public:
    virtual ~IBlockchainConsumer() = default;

    virtual SynchronizationStart getSyncStart() = 0;
    virtual const std::unordered_set<Crypto::FHash> &getKnownPoolTxIds() const = 0;
    virtual void onBlockchainDetach(uint32_t height) = 0;
    virtual bool onNewBlocks(const CompleteBlock *blocks, uint32_t startHeight, uint32_t count) = 0;
    virtual std::error_code onPoolUpdated(
        const std::vector<std::unique_ptr<ITransactionReader>> &addedTransactions,
        const std::vector<Crypto::FHash> &deletedTransactions) = 0;

    virtual std::error_code addUnconfirmedTransaction(const ITransactionReader &transaction) = 0;
    virtual void removeUnconfirmedTransaction(const Crypto::FHash &transactionHash) = 0;
};

class IBlockchainConsumerObserver
{
public:
    virtual void onBlocksAdded(
        IBlockchainConsumer *consumer,
        const std::vector<Crypto::FHash> &blockHashes)
    {
    }

    virtual void onBlockchainDetach(IBlockchainConsumer *consumer, uint32_t blockIndex)
    {
    }

    virtual void onTransactionDeleteBegin(
        IBlockchainConsumer *consumer,
        Crypto::FHash transactionHash)
    {
    }

    virtual void onTransactionDeleteEnd(IBlockchainConsumer *consumer, Crypto::FHash transactionHash)
    {
    }

    virtual void onTransactionUpdated(
        IBlockchainConsumer *consumer,
        const Crypto::FHash &transactionHash,
        const std::vector<ITransfersContainer *> &containers)
    {
    }
};

class IBlockchainSynchronizer : public IObservable<IBlockchainSynchronizerObserver>,
                                public IStreamSerializable
{
public:
    virtual void addConsumer(IBlockchainConsumer *consumer) = 0;
    virtual bool removeConsumer(IBlockchainConsumer *consumer) = 0;
    virtual IStreamSerializable* getConsumerState(IBlockchainConsumer *consumer) const = 0;
    virtual std::vector<Crypto::FHash> getConsumerKnownBlocks(IBlockchainConsumer &consumer) const=0;

    virtual std::future<std::error_code> addUnconfirmedTransaction(
        const ITransactionReader &transaction) = 0;
    virtual std::future<void> removeUnconfirmedTransaction(
        const Crypto::FHash &transactionHash) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
};

} // namespace QwertyNote
