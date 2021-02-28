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

#include <future>
#include <numeric>

#include <INode.h>

#include <Common/BlockingQueue.h>
#include <Common/StringTools.h>

#include <Global/Constants.h>

#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/TransactionApi.h>

#include <Transfers/CommonTypes.h>
#include <Transfers/TransfersConsumer.h>

#include <Wallet/IWallet.h>

using namespace Crypto;
using namespace Logging;
using namespace Common;
using namespace Qwertycoin;

std::unordered_set<Crypto::FHash> transactions_hash_seen;
std::unordered_set<Crypto::FPublicKey> public_keys_seen;
std::mutex seen_mutex;

namespace {

using namespace QwertyNote;

void checkOutputKey(
    const FKeyDerivation &derivation,
    const FPublicKey &key,
    size_t keyIndex,
    size_t outputIndex,
    const std::unordered_set<FPublicKey> &spendKeys,
    std::unordered_map<FPublicKey, std::vector<uint32_t>> &outputs)
{
    FPublicKey spendKey;
    underivePublicKey(derivation, keyIndex, key, spendKey);

    if (spendKeys.find(spendKey) != spendKeys.end()) {
        outputs[spendKey].push_back(static_cast<uint32_t>(outputIndex));
    }
}

void findMyOutputs(
    const ITransactionReader &tx,
    const FSecretKey &viewSecretKey,
    const std::unordered_set<FPublicKey> &spendKeys,
    std::unordered_map<FPublicKey, std::vector<uint32_t>> &outputs)
{
    auto txPublicKey = tx.getTransactionPublicKey();

    FKeyDerivation derivation;

    if (!generateKeyDerivation(txPublicKey, viewSecretKey, derivation)) {
        return;
    }

    size_t keyIndex = 0;
    size_t outputCount = tx.getOutputCount();

    for (size_t idx = 0; idx < outputCount; ++idx) {
        auto outType = tx.getOutputType(size_t(idx));

        if (outType == TransactionTypes::OutputType::Key) {
            uint64_t amount;
            FKeyOutput out;
            tx.getOutput(idx, out, amount);

            checkOutputKey(derivation, out.sPublicKey, keyIndex, idx, spendKeys, outputs);
            ++keyIndex;
        } else if (outType == TransactionTypes::OutputType::Multisignature) {
            uint64_t amount;
            FMultiSignatureOutput out;
            tx.getOutput(idx, out, amount);

            for (const auto &key : out.vPublicKeys) {
                checkOutputKey(derivation, key, idx, idx, spendKeys, outputs);
                ++keyIndex;
            }
        }
    }
}

std::vector<Crypto::FHash> getBlockHashes(const QwertyNote::CompleteBlock *blocks, size_t count)
{
    std::vector<Crypto::FHash> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        result.push_back(blocks[i].blockHash);
    }

    return result;
}

} // namespace

namespace QwertyNote {

TransfersConsumer::TransfersConsumer(
    const QwertyNote::Currency &currency,
    INode &node,
    Logging::ILogger &logger,
    const FSecretKey &viewSecret)
    : m_node(node),
      m_viewSecret(viewSecret),
      m_currency(currency),
      m_logger(logger, "TransfersConsumer")
{
    updateSyncStart();
}

ITransfersSubscription& TransfersConsumer::addSubscription(const FAccountSubscription &subscription)
{
    if (subscription.sKeys.sViewSecretKey != m_viewSecret) {
        throw std::runtime_error("TransfersConsumer: view secret key mismatch");
    }

    auto &res = m_subscriptions[subscription.sKeys.sAddress.sSpendPublicKey];

    if (res.get() == nullptr) {
        res.reset(new TransfersSubscription(m_currency, m_logger.getLogger(), subscription));
        m_spendKeys.insert(subscription.sKeys.sAddress.sSpendPublicKey);
        if (m_subscriptions.size() == 1) {
            m_syncStart = res->getSyncStart();
        } else {
            auto subStart = res->getSyncStart();
            m_syncStart.uHeight = std::min(m_syncStart.uHeight, subStart.uHeight);
            m_syncStart.uTimestamp = std::min(m_syncStart.uTimestamp, subStart.uTimestamp);
        }
    }

    return *res;
}

bool TransfersConsumer::removeSubscription(const FAccountPublicAddress &address)
{
    m_subscriptions.erase(address.sSpendPublicKey);
    m_spendKeys.erase(address.sSpendPublicKey);

    updateSyncStart();

    return m_subscriptions.empty();
}

ITransfersSubscription* TransfersConsumer::getSubscription(const FAccountPublicAddress &acc)
{
    auto it = m_subscriptions.find(acc.sSpendPublicKey);

    return it == m_subscriptions.end() ? nullptr : it->second.get();
}

void TransfersConsumer::getSubscriptions(std::vector<FAccountPublicAddress> &subscriptions)
{
    for (const auto &kv : m_subscriptions) {
        subscriptions.push_back(kv.second->getAddress());
    }
}

void TransfersConsumer::initTransactionPool(
    const std::unordered_set<Crypto::FHash> &uncommitedTransactions)
{
    for (auto itSubscriptions = m_subscriptions.begin();
         itSubscriptions != m_subscriptions.end();
         ++itSubscriptions) {
        std::vector<Crypto::FHash> unconfirmedTransactions;
        itSubscriptions->second->getContainer().getUnconfirmedTransactions(unconfirmedTransactions);
        for (auto itTransactions = unconfirmedTransactions.begin();
             itTransactions != unconfirmedTransactions.end();
             ++itTransactions) {
            if (uncommitedTransactions.count(*itTransactions) == 0) {
                m_poolTxs.emplace(*itTransactions);
            }
        }
    }
}

void TransfersConsumer::updateSyncStart()
{
    FSynchronizationStart start;

    start.uHeight =   std::numeric_limits<uint64_t>::max();
    start.uTimestamp = std::numeric_limits<uint64_t>::max();

    for (const auto &kv : m_subscriptions) {
        auto subStart = kv.second->getSyncStart();
        start.uHeight = std::min(start.uHeight, subStart.uHeight);
        start.uTimestamp = std::min(start.uTimestamp, subStart.uTimestamp);
    }

    m_syncStart = start;
}

FSynchronizationStart TransfersConsumer::getSyncStart()
{
    return m_syncStart;
}

void TransfersConsumer::onBlockchainDetach(uint32_t height)
{
    m_observerManager.notify(&IBlockchainConsumerObserver::onBlockchainDetach, this, height);

    for (const auto &kv : m_subscriptions) {
        kv.second->onBlockchainDetach(height);
    }
}

bool TransfersConsumer::onNewBlocks(const CompleteBlock *blocks,uint32_t startHeight,uint32_t count)
{
    assert(blocks);
    assert(count > 0);

    struct Tx
    {
        TransactionBlockInfo blockInfo;
        const ITransactionReader *tx;
    };

    struct PreprocessedTx : Tx, PreprocessInfo {};

    std::vector<PreprocessedTx> preprocessedTransactions;
    std::mutex preprocessedTransactionsMutex;

    size_t workers = std::thread::hardware_concurrency();
    if (workers == 0) {
        workers = 2;
    }

    TBlockingQueue<Tx> inputQueue(workers * 2);

    std::atomic<bool> stopProcessing(false);

    auto pushingThread = std::async(std::launch::async, [&] {
        for( uint32_t i = 0; i < count && !stopProcessing; ++i) {
            const auto &block = blocks[i].block;

            if (!block.is_initialized()) {
                continue;
            }

            // filter by syncStartTimestamp
            if (m_syncStart.uTimestamp && block->uTimestamp < m_syncStart.uTimestamp) {
                continue;
            }

            TransactionBlockInfo blockInfo;
            blockInfo.height = startHeight + i;
            blockInfo.timestamp = block->uTimestamp;
            blockInfo.transactionIndex = 0; // position in block

            for (const auto &tx : blocks[i].transactions) {
                auto pubKey = tx->getTransactionPublicKey();
                if (pubKey == NULL_PUBLIC_KEY) {
                    ++blockInfo.transactionIndex;
                    continue;
                }

                Tx item = { blockInfo, tx.get() };
                inputQueue.push(item);
                ++blockInfo.transactionIndex;
            }
        }

        inputQueue.close();
    });

    auto processingFunction = [&] {
        Tx item;
        std::error_code ec;
        while (!stopProcessing && inputQueue.pop(item)) {
            PreprocessedTx output;
            static_cast<Tx &>(output) = item;

            ec = preprocessOutputs(item.blockInfo, *item.tx, output);
            if (ec) {
                stopProcessing = true;
                break;
            }

            std::lock_guard<std::mutex> lk(preprocessedTransactionsMutex);
            preprocessedTransactions.push_back(std::move(output));
        }
        return ec;
    };

    std::vector<std::future<std::error_code>> processingThreads;
    for (size_t i = 0; i < workers; ++i) {
        processingThreads.push_back(std::async(std::launch::async, processingFunction));
    }

    std::error_code processingError;
    for (auto &f : processingThreads) {
        try {
            std::error_code ec = f.get();
            if (!processingError && ec) {
                processingError = ec;
            }
        } catch (const std::system_error &e) {
            processingError = e.code();
        } catch (const std::exception &) {
            processingError = std::make_error_code(std::errc::operation_canceled);
        }
    }

    std::vector<Crypto::FHash> blockHashes = getBlockHashes(blocks, count);
    if (!processingError) {
        m_observerManager.notify(&IBlockchainConsumerObserver::onBlocksAdded, this, blockHashes);

        // sort by block height and transaction index in block
        std::sort(preprocessedTransactions.begin(),
                  preprocessedTransactions.end(),
                  [](const PreprocessedTx &a, const PreprocessedTx &b) {
            auto tieA = std::tie(a.blockInfo.height, a.blockInfo.transactionIndex);
            auto tieB = std::tie(b.blockInfo.height, b.blockInfo.transactionIndex);
            return tieA < tieB;
        });

        for (const auto &tx : preprocessedTransactions) {
            processTransaction(tx.blockInfo, *tx.tx, tx);
        }
    } else {
        forEachSubscription([&](TransfersSubscription &sub) {
            sub.onError(processingError, startHeight);
        });

        return false;
    }

    auto newHeight = startHeight + count - 1;
    forEachSubscription([newHeight](TransfersSubscription &sub) {
        sub.advanceHeight(newHeight);
    });

    return true;
}

std::error_code TransfersConsumer::onPoolUpdated(
    const std::vector<std::unique_ptr<ITransactionReader>> &addedTransactions,
    const std::vector<FHash> &deletedTransactions)
{
    TransactionBlockInfo unconfirmedBlockInfo;
    unconfirmedBlockInfo.timestamp = 0;
    unconfirmedBlockInfo.height = WALLET_UNCONFIRMED_TRANSACTION_HEIGHT;

    std::error_code processingError;
    for (auto &cryptonoteTransaction : addedTransactions) {
        m_poolTxs.emplace(cryptonoteTransaction->getTransactionHash());
        processingError = processTransaction(unconfirmedBlockInfo, *cryptonoteTransaction.get());
        if (processingError) {
            for (auto &sub : m_subscriptions) {
                sub.second->onError(processingError, WALLET_UNCONFIRMED_TRANSACTION_HEIGHT);
            }

            return processingError;
        }
    }

    for (auto &deletedTxHash : deletedTransactions) {
        m_poolTxs.erase(deletedTxHash);
        m_observerManager.notify(
            &IBlockchainConsumerObserver::onTransactionDeleteBegin,
            this,
            deletedTxHash
        );
        for (auto &sub : m_subscriptions) {
            sub.second->deleteUnconfirmedTransaction(
                *reinterpret_cast<const FHash *>(&deletedTxHash)
            );
        }

        m_observerManager.notify(
            &IBlockchainConsumerObserver::onTransactionDeleteEnd,
            this,
            deletedTxHash
        );
    }

    return std::error_code{};
}

const std::unordered_set<Crypto::FHash> &TransfersConsumer::getKnownPoolTxIds() const
{
    return m_poolTxs;
}

std::error_code TransfersConsumer::addUnconfirmedTransaction(const ITransactionReader &transaction)
{
    TransactionBlockInfo unconfirmedBlockInfo;
    unconfirmedBlockInfo.height = WALLET_UNCONFIRMED_TRANSACTION_HEIGHT;
    unconfirmedBlockInfo.timestamp = 0;
    unconfirmedBlockInfo.transactionIndex = 0;

    return processTransaction(unconfirmedBlockInfo, transaction);
}

void TransfersConsumer::removeUnconfirmedTransaction(const Crypto::FHash& transactionHash)
{
    m_observerManager.notify(
        &IBlockchainConsumerObserver::onTransactionDeleteBegin,
        this,
        transactionHash
    );
    for (auto &subscription : m_subscriptions) {
        subscription.second->deleteUnconfirmedTransaction(transactionHash);
    }
    m_observerManager.notify(
        &IBlockchainConsumerObserver::onTransactionDeleteEnd,
        this,
        transactionHash
                );
}

void TransfersConsumer::markTransactionSafe(const FHash &transactionHash)
{
    forEachSubscription([transactionHash](TransfersSubscription &sub) {
        sub.markTransactionSafe(transactionHash);
    });
}

void TransfersConsumer::addPublicKeysSeen(
    const Crypto::FHash &transactionHash,
    const Crypto::FPublicKey &outputKey)
{
    std::lock_guard<std::mutex> lk(seen_mutex);
    transactions_hash_seen.insert(transactionHash);
    public_keys_seen.insert(outputKey);
}

std::error_code TransfersConsumer::createTransfers(
    const FAccountKeys &account,
    const TransactionBlockInfo &blockInfo,
    const ITransactionReader &tx,
    const std::vector<uint32_t> &outputs,
    const std::vector<uint32_t> &globalIdxs,
    std::vector<TransactionOutputInformationIn> &transfers)
{
    auto txPubKey = tx.getTransactionPublicKey();
    auto txHash = tx.getTransactionHash();
    std::vector<FPublicKey> temp_keys;
    std::lock_guard<std::mutex> lk(seen_mutex);

    for (auto idx : outputs) {
        if (idx >= tx.getOutputCount()) {
            return std::make_error_code(std::errc::argument_out_of_domain);
        }

        auto outType = tx.getOutputType(size_t(idx));

        if (outType != TransactionTypes::OutputType::Key
            && outType != TransactionTypes::OutputType::Multisignature) {
            continue;
        }

        TransactionOutputInformationIn info;

        info.sOutputType = outType;
        info.sTransactionPublicKey = txPubKey;
        info.uOutputInTransaction = idx;
        info.uGlobalOutputIndex = (blockInfo.height == WALLET_UNCONFIRMED_TRANSACTION_HEIGHT)
                                 ? UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX : globalIdxs[idx];

        if (outType == TransactionTypes::OutputType::Key) {
            uint64_t amount;
            FKeyOutput out;
            tx.getOutput(idx, out, amount);

            QwertyNote::FKeyPair in_ephemeral;
            QwertyNote::generate_key_image_helper(
                account,
                txPubKey,
                idx,
                in_ephemeral,
                info.keyImage
            );

            assert(out.sPublicKey == reinterpret_cast<const FPublicKey &>(in_ephemeral.sPublicKey));

            auto it = transactions_hash_seen.find(txHash);
            if (it == transactions_hash_seen.end()) {
                auto key_it = public_keys_seen.find(out.sPublicKey);
                if (key_it != public_keys_seen.end()) {
                    m_logger(ERROR, BRIGHT_RED)
                        << "Failed to process transaction "
                        << Common::podToHex(txHash)
                        << ": duplicate output key is found!";
                    return std::error_code{};
                }
                if (std::find(temp_keys.begin(), temp_keys.end(), out.sPublicKey) != temp_keys.end()) {
                    m_logger(ERROR, BRIGHT_RED)
                        << "Failed to process transaction "
                        << Common::podToHex(txHash)
                        << ": the same output key is present more than once";
                    return std::error_code{};
                }
                temp_keys.push_back(out.sPublicKey);
            }
            info.uAmount = amount;
            info.sOutputKey = out.sPublicKey;
        } else if (outType == TransactionTypes::OutputType::Multisignature) {
            uint64_t amount;
            FMultiSignatureOutput out;
            tx.getOutput(idx, out, amount);

            for (const auto &key : out.vPublicKeys) {
                auto it = transactions_hash_seen.find(txHash);
                if (it == transactions_hash_seen.end()) {
                    auto key_it = public_keys_seen.find(key);
                    if (key_it != public_keys_seen.end()) {
                        m_logger(ERROR, BRIGHT_RED)
                            << "Failed to process transaction "
                            << Common::podToHex(txHash)
                            << ": duplicate multisignature output key is found";
                        return std::error_code{};
                    }
                    if (std::find(temp_keys.begin(), temp_keys.end(), key) != temp_keys.end()) {
                        m_logger(ERROR, BRIGHT_RED)
                            << "Failed to process transaction "
                            << Common::podToHex(txHash)
                            << ": the same multisignature output key is present more than once";
                        return std::error_code();
                    }
                    temp_keys.push_back(key);
                }
            }
            info.uAmount = amount;
            info.uRequiredSignatures = out.uRequiredSignatureCount;
        }

        transfers.push_back(info);
    }

    transactions_hash_seen.emplace(txHash);
    std::copy(
        temp_keys.begin(),
        temp_keys.end(),
        std::inserter(public_keys_seen, public_keys_seen.end())
     );

    return std::error_code{};
}

std::error_code TransfersConsumer::preprocessOutputs(
    const TransactionBlockInfo &blockInfo,
    const ITransactionReader &tx,
    PreprocessInfo &info)
{
    std::unordered_map<FPublicKey, std::vector<uint32_t>> outputs;
    findMyOutputs(tx, m_viewSecret, m_spendKeys, outputs);
    if (outputs.empty()) {
        return std::error_code{};
    }

    std::error_code errorCode;
    auto txHash = tx.getTransactionHash();
    if (blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
        errorCode = getGlobalIndices(reinterpret_cast<const FHash &>(txHash), info.globalIdxs);
        if (errorCode) {
            return errorCode;
        }
    }

    for (const auto &kv : outputs) {
        auto it = m_subscriptions.find(kv.first);
        if (it != m_subscriptions.end()) {
            auto &transfers = info.outputs[kv.first];
            errorCode = createTransfers(it->second->getKeys(),
                                        blockInfo,
                                        tx,
                                        kv.second,
                                        info.globalIdxs,
                                        transfers);
            if (errorCode) {
                return errorCode;
            }
        }
    }

    return std::error_code{};
}

std::error_code TransfersConsumer::processTransaction(
    const TransactionBlockInfo &blockInfo,
    const ITransactionReader &tx)
{
    PreprocessInfo info;
    auto ec = preprocessOutputs(blockInfo, tx, info);
    if (ec) {
        return ec;
    }

    processTransaction(blockInfo, tx, info);

    return std::error_code{};
}

void TransfersConsumer::processTransaction(
    const TransactionBlockInfo &blockInfo,
    const ITransactionReader &tx,
    const PreprocessInfo &info)
{
    std::vector<TransactionOutputInformationIn> emptyOutputs;
    std::vector<ITransfersContainer*> transactionContainers;
    bool someContainerUpdated = false;
    for (auto &kv : m_subscriptions) {
        auto it = info.outputs.find(kv.first);
        auto &subscriptionOutputs = (it == info.outputs.end()) ? emptyOutputs : it->second;

        bool containerContainsTx;
        bool containerUpdated;
        processOutputs(blockInfo,
                       *kv.second,
                       tx,
                       subscriptionOutputs,
                       info.globalIdxs,
                       containerContainsTx,
                       containerUpdated);
        someContainerUpdated = someContainerUpdated || containerUpdated;
        if (containerContainsTx) {
            transactionContainers.emplace_back(&kv.second->getContainer());
        }
    }

    if (someContainerUpdated) {
        m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionUpdated,
                                 this,
                                 tx.getTransactionHash(),
                                 transactionContainers);
    }
}

void TransfersConsumer::processOutputs(
    const TransactionBlockInfo &blockInfo,
    TransfersSubscription &sub,
    const ITransactionReader &tx,
    const std::vector<TransactionOutputInformationIn> &transfers,
    const std::vector<uint32_t> &globalIdxs,
    bool &contains,
    bool &updated)
{
    FTransactionInformation subscribtionTxInfo;
    contains = sub.getContainer().getTransactionInformation(tx.getTransactionHash(),
                                                            subscribtionTxInfo);
    updated = false;

    if (contains) {
        if (subscribtionTxInfo.uBlockHeight == WALLET_UNCONFIRMED_TRANSACTION_HEIGHT
            && blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
            // pool->blockchain
            sub.markTransactionConfirmed(blockInfo, tx.getTransactionHash(), globalIdxs);
            updated = true;
        } else {
            assert(subscribtionTxInfo.uBlockHeight == blockInfo.height);
        }
    } else {
        updated = sub.addTransaction(blockInfo, tx, transfers);
        contains = updated;
    }
}

std::error_code TransfersConsumer::getGlobalIndices(
    const FHash &transactionHash,
    std::vector<uint32_t> &outsGlobalIndices)
{
    std::promise<std::error_code> prom;
    std::future<std::error_code> f = prom.get_future();

    INode::UCallback cb = [&prom](std::error_code ec) {
        std::promise<std::error_code> p(std::move(prom));
        p.set_value(ec);
    };

    outsGlobalIndices.clear();
    m_node.getTransactionOutsGlobalIndices(transactionHash, outsGlobalIndices, cb);

    return f.get();
}

} // namespace QwertyNote
