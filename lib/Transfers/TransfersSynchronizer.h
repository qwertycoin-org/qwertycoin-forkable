// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, The BBSCoin Developers
// Copyright (c) 2018, The Karbo Developers
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
#include <memory>
#include <unordered_map>
#include <Common/ObserverManager.h>
#include <Logging/LoggerRef.h>
#include <Transfers/IBlockchainSynchronizer.h>
#include <Transfers/TypeHelpers.h>
#include <ITransfersSynchronizer.h>

namespace QwertyNote {

class Currency;
class TransfersConsumer;
class INode;

class TransfersSynchronizer : public ITransfersSynchronizer, public IBlockchainConsumerObserver
{
    typedef std::unordered_map<
        Crypto::FPublicKey,
        std::unique_ptr<TransfersConsumer>
    > ConsumersContainer;

    typedef Tools::QObserverManager<ITransfersSynchronizerObserver> SubscribersNotifier;

    typedef std::unordered_map<
        Crypto::FPublicKey,
        std::unique_ptr<SubscribersNotifier>
    > SubscribersContainer;

public:
    TransfersSynchronizer(const QwertyNote::Currency &currency,
                         Logging::ILogger &logger,
                         IBlockchainSynchronizer &sync,
                         INode &node);
    ~TransfersSynchronizer() override;

    void initTransactionPool(const std::unordered_set<Crypto::FHash> &uncommitedTransactions);

    // ITransfersSynchronizer
    ITransfersSubscription &addSubscription(const FAccountSubscription &acc) override;
    bool removeSubscription(const FAccountPublicAddress &acc) override;
    void getSubscriptions(std::vector<FAccountPublicAddress> &subscriptions) override;
    ITransfersSubscription *getSubscription(const FAccountPublicAddress &acc) override;
    std::vector<Crypto::FHash> getViewKeyKnownBlocks(const Crypto::FPublicKey &publicViewKey)override;

    void subscribeConsumerNotifications(const Crypto::FPublicKey &viewPublicKey,
                                        ITransfersSynchronizerObserver *observer);
    void unsubscribeConsumerNotifications(const Crypto::FPublicKey &viewPublicKey,
                                          ITransfersSynchronizerObserver *observer);

    void addPublicKeysSeen(const FAccountPublicAddress &acc,
                           const Crypto::FHash &transactionHash,
                           const Crypto::FPublicKey &outputKey);
    void markTransactionSafe(const Crypto::FHash &transactionHash);


    // IStreamSerializable
    void save(std::ostream &os) override;
    void load(std::istream &in) override;

private:
    void onBlocksAdded(
        IBlockchainConsumer *consumer,
        const std::vector<Crypto::FHash> &blockHashes) override;
    void onBlockchainDetach(
        IBlockchainConsumer *consumer,
        uint32_t blockIndex) override;
    void onTransactionDeleteBegin(
        IBlockchainConsumer *consumer,
        Crypto::FHash transactionHash) override;
    void onTransactionDeleteEnd(
        IBlockchainConsumer *consumer,
        Crypto::FHash transactionHash) override;
    void onTransactionUpdated(
        IBlockchainConsumer *consumer,
        const Crypto::FHash &transactionHash,
        const std::vector<ITransfersContainer *> &containers) override;

    bool findViewKeyForConsumer(IBlockchainConsumer *consumer, Crypto::FPublicKey &viewKey) const;
    SubscribersContainer::const_iterator findSubscriberForConsumer(IBlockchainConsumer *c) const;

private:
    Logging::LoggerRef m_logger;
    ConsumersContainer m_consumers; // map { view public key -> consumer }
    SubscribersContainer m_subscribers;
    IBlockchainSynchronizer &m_sync;
    INode &m_node;
    const QwertyNote::Currency &m_currency;
};

} // namespace QwertyNote
