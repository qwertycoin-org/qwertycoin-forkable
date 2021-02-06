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

#include <Common/StdInputStream.h>
#include <Common/StdOutputStream.h>
#include <CryptoNoteCore/CryptoNoteBasicImpl.h>
#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/BinaryOutputStreamSerializer.h>
#include <Transfers/TransfersConsumer.h>
#include <Transfers/TransfersSynchronizer.h>

using namespace Common;
using namespace Crypto;

namespace CryptoNote {

const uint32_t TRANSFERS_STORAGE_ARCHIVE_VERSION = 0;

TransfersSynchronizer::TransfersSynchronizer(
    const CryptoNote::Currency &currency,
    Logging::ILogger &logger,
    IBlockchainSynchronizer &sync,
    INode &node)
    : m_currency(currency),
      m_logger(logger, "TransfersSynchronizer"),
      m_sync(sync),
      m_node(node)
{
}

TransfersSynchronizer::~TransfersSynchronizer()
{
    m_sync.stop();
    for (const auto &kv : m_consumers) {
        m_sync.removeConsumer(kv.second.get());
    }
}

void TransfersSynchronizer::initTransactionPool(
    const std::unordered_set<Crypto::FHash> &uncommitedTransactions)
{
    for (auto it = m_consumers.begin(); it != m_consumers.end(); ++it) {
        it->second->initTransactionPool(uncommitedTransactions);
    }
}

ITransfersSubscription& TransfersSynchronizer::addSubscription(const AccountSubscription& acc) {
    auto it = m_consumers.find(acc.keys.address.viewPublicKey);

    if (it == m_consumers.end()) {
        std::unique_ptr<TransfersConsumer> consumer(
            new TransfersConsumer(m_currency, m_node, m_logger.getLogger(), acc.keys.viewSecretKey)
        );

        m_sync.addConsumer(consumer.get());
        consumer->addObserver(this);
        it = m_consumers.insert(std::make_pair(
            acc.keys.address.viewPublicKey,
            std::move(consumer))
        ).first;
    }

    return it->second->addSubscription(acc);
}

bool TransfersSynchronizer::removeSubscription(const AccountPublicAddress &acc)
{
    auto it = m_consumers.find(acc.viewPublicKey);
    if (it == m_consumers.end()) {
        return false;
    }

    if (it->second->removeSubscription(acc)) {
        m_sync.removeConsumer(it->second.get());
        m_consumers.erase(it);
        m_subscribers.erase(acc.viewPublicKey);
    }

    return true;
}

void TransfersSynchronizer::getSubscriptions(std::vector<AccountPublicAddress> &subscriptions)
{
    for (const auto &kv : m_consumers) {
        kv.second->getSubscriptions(subscriptions);
    }
}

ITransfersSubscription *TransfersSynchronizer::getSubscription(const AccountPublicAddress &acc)
{
    auto it = m_consumers.find(acc.viewPublicKey);
    return (it == m_consumers.end()) ? nullptr : it->second->getSubscription(acc);
}

void TransfersSynchronizer::addPublicKeysSeen(
    const AccountPublicAddress &acc,
    const Crypto::FHash &transactionHash,
    const Crypto::FPublicKey &outputKey)
{
    auto it = m_consumers.find(acc.viewPublicKey);
    if (it != m_consumers.end()) {
        it->second->addPublicKeysSeen(transactionHash, outputKey);
    }
}

void TransfersSynchronizer::markTransactionSafe(const FHash &transactionHash)
{
    for (const auto &kv : m_consumers) {
        kv.second->markTransactionSafe(transactionHash);
    }
}

std::vector<Crypto::FHash> TransfersSynchronizer::getViewKeyKnownBlocks(
    const Crypto::FPublicKey &publicViewKey)
{
    auto it = m_consumers.find(publicViewKey);
    if (it == m_consumers.end()) {
        throw std::invalid_argument("Consumer not found");
    }

    return m_sync.getConsumerKnownBlocks(*it->second);
}

void TransfersSynchronizer::onBlocksAdded(
    IBlockchainConsumer *consumer,
    const std::vector<Crypto::FHash> &blockHashes)
{
    auto it = findSubscriberForConsumer(consumer);
    if (it != m_subscribers.end()) {
        it->second->notify(
            &ITransfersSynchronizerObserver::onBlocksAdded,
            it->first,
            blockHashes
        );
    }
}

void TransfersSynchronizer::onBlockchainDetach(
    IBlockchainConsumer *consumer,
    uint32_t blockIndex)
{
    auto it = findSubscriberForConsumer(consumer);
    if (it != m_subscribers.end()) {
        it->second->notify(
            &ITransfersSynchronizerObserver::onBlockchainDetach,
            it->first,
            blockIndex
        );
    }
}

void TransfersSynchronizer::onTransactionDeleteBegin(
    IBlockchainConsumer *consumer,
    Crypto::FHash transactionHash)
{
    auto it = findSubscriberForConsumer(consumer);
    if (it != m_subscribers.end()) {
        it->second->notify(
            &ITransfersSynchronizerObserver::onTransactionDeleteBegin,
            it->first,
            transactionHash
        );
    }
}

void TransfersSynchronizer::onTransactionDeleteEnd(
    IBlockchainConsumer *consumer,
    Crypto::FHash transactionHash)
{
    auto it = findSubscriberForConsumer(consumer);
    if (it != m_subscribers.end()) {
        it->second->notify(
            &ITransfersSynchronizerObserver::onTransactionDeleteEnd,
            it->first,
            transactionHash
        );
    }
}

void TransfersSynchronizer::onTransactionUpdated(
    IBlockchainConsumer *consumer,
    const Crypto::FHash &transactionHash,
    const std::vector<ITransfersContainer *> &containers)
{
    auto it = findSubscriberForConsumer(consumer);
    if (it != m_subscribers.end()) {
        it->second->notify(
            &ITransfersSynchronizerObserver::onTransactionUpdated,
            it->first,
            transactionHash,
            containers
        );
    }
}

void TransfersSynchronizer::subscribeConsumerNotifications(
    const Crypto::FPublicKey &viewPublicKey,
    ITransfersSynchronizerObserver *observer)
{
    auto it = m_subscribers.find(viewPublicKey);
    if (it != m_subscribers.end()) {
        it->second->add(observer);
        return;
    }

    auto insertedIt = m_subscribers.emplace(
        viewPublicKey,
        std::unique_ptr<SubscribersNotifier>(new SubscribersNotifier())
    ).first;
    insertedIt->second->add(observer);
}

void TransfersSynchronizer::unsubscribeConsumerNotifications(
    const Crypto::FPublicKey &viewPublicKey,
    ITransfersSynchronizerObserver *observer)
{
    m_subscribers.at(viewPublicKey)->remove(observer);
}

void TransfersSynchronizer::save(std::ostream &os)
{
    m_sync.save(os);

    StdOutputStream stream(os);
    CryptoNote::BinaryOutputStreamSerializer s(stream);
    s(const_cast<uint32_t&>(TRANSFERS_STORAGE_ARCHIVE_VERSION), "version");

    size_t subscriptionCount = m_consumers.size();

    s.beginArray(subscriptionCount, "consumers");

    for (const auto &consumer : m_consumers) {
        s.beginObject("");
        s(const_cast<FPublicKey &>(consumer.first), "view_key");

        std::stringstream consumerState;
        // synchronization state
        m_sync.getConsumerState(consumer.second.get())->save(consumerState);

        std::string blob = consumerState.str();
        s(blob, "state");

        std::vector<AccountPublicAddress> subscriptions;
        consumer.second->getSubscriptions(subscriptions);
        size_t subCount = subscriptions.size();

        s.beginArray(subCount, "subscriptions");

        for (auto &addr : subscriptions) {
            auto sub = consumer.second->getSubscription(addr);
            if (sub != nullptr) {
                s.beginObject("");

                std::stringstream subState;
                assert(sub);
                sub->getContainer().save(subState);
                // store data block
                std::string blobStr = subState.str();
                s(addr, "address");
                s(blobStr, "state");

                s.endObject();
            }
        }

        s.endArray();
        s.endObject();
    }
}

namespace {

std::string getObjectState(IStreamSerializable &obj)
{
    std::stringstream stream;
    obj.save(stream);

    return stream.str();
}

void setObjectState(IStreamSerializable &obj, const std::string &state)
{
    std::stringstream stream(state);
    obj.load(stream);
}

} // namespace

void TransfersSynchronizer::load(std::istream &is)
{
    m_sync.load(is);

    StdInputStream inputStream(is);
    CryptoNote::BinaryInputStreamSerializer s(inputStream);
    uint32_t version = 0;

    s(version, "version");

    if (version > TRANSFERS_STORAGE_ARCHIVE_VERSION) {
        throw std::runtime_error("TransfersSynchronizer version mismatch");
    }

    struct ConsumerState
    {
        FPublicKey viewKey;
        std::string state;
        std::vector<std::pair<AccountPublicAddress, std::string>> subscriptionStates;
    };

    std::vector<ConsumerState> updatedStates;

    try {
        size_t subscriptionCount = 0;
        s.beginArray(subscriptionCount, "consumers");

        while (subscriptionCount--) {
            s.beginObject("");
            FPublicKey viewKey;
            s(viewKey, "view_key");

            std::string blob;
            s(blob, "state");

            auto subIter = m_consumers.find(viewKey);
            if (subIter != m_consumers.end()) {
                auto consumerState = m_sync.getConsumerState(subIter->second.get());
                assert(consumerState);

                {
                    // store previous state
                    auto prevConsumerState = getObjectState(*consumerState);
                    // load consumer state
                    setObjectState(*consumerState, blob);
                    updatedStates.push_back(ConsumerState{ viewKey, std::move(prevConsumerState) });
                }

                // load subscriptions
                size_t subCount = 0;
                s.beginArray(subCount, "subscriptions");

                while (subCount--) {
                    s.beginObject("");

                    AccountPublicAddress acc;
                    std::string state;

                    s(acc, "address");
                    s(state, "state");

                    auto sub = subIter->second->getSubscription(acc);

                    if (sub != nullptr) {
                        auto prevState = getObjectState(sub->getContainer());
                        setObjectState(sub->getContainer(), state);
                        updatedStates.back().subscriptionStates.push_back(
                            std::make_pair(acc, prevState)
                        );
                    } else {
                        m_logger(Logging::DEBUGGING)
                            << "Subscription not found: "
                            << m_currency.accountAddressAsString(acc);
                    }

                    s.endObject();
                }

                s.endArray();
            } else {
                m_logger(Logging::DEBUGGING) << "Consumer not found: " << viewKey;
            }

            s.endObject();
        }

        s.endArray();
    } catch (...) {
        // rollback state
        for (const auto &consumerState : updatedStates) {
            auto consumer = m_consumers.find(consumerState.viewKey)->second.get();
            setObjectState(*m_sync.getConsumerState(consumer), consumerState.state);
            for (const auto &sub : consumerState.subscriptionStates) {
                setObjectState(consumer->getSubscription(sub.first)->getContainer(), sub.second);
            }
        }
        throw;
    }
}

bool TransfersSynchronizer::findViewKeyForConsumer(IBlockchainConsumer *consumer,
                                                  Crypto::FPublicKey &viewKey) const
{
    // since we have only couple of consumers linear complexity is fine
    auto it = std::find_if(m_consumers.begin(),
                           m_consumers.end(),
                           [consumer] (const ConsumersContainer::value_type &subscription) {
        return subscription.second.get() == consumer;
    });

    if (it == m_consumers.end()) {
        return false;
    }

    viewKey = it->first;

    return true;
}

TransfersSynchronizer::SubscribersContainer::const_iterator
TransfersSynchronizer::findSubscriberForConsumer(IBlockchainConsumer *consumer) const
{
    Crypto::FPublicKey viewKey;
    if (findViewKeyForConsumer(consumer, viewKey)) {
        auto it = m_subscribers.find(viewKey);
        if (it != m_subscribers.end()) {
            return it;
        }
    }

    return m_subscribers.end();
}

} // namespace CryptoNote
