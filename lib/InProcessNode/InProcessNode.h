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

#include <thread>

#include <boost/asio.hpp>

#include <INode.h>

#include <BlockchainExplorer/BlockchainExplorerDataBuilder.h>

#include <Common/ObserverManager.h>

#include <QwertyNoteCore/ICore.h>
#include <QwertyNoteCore/ICoreObserver.h>
#include <QwertyNoteCore/ITransaction.h>

#include <QwertyNoteProtocol/IQwertyNoteProtocolQuery.h>
#include <QwertyNoteProtocol/IQwertyNoteProtocolObserver.h>


namespace QwertyNote {

class core;

class InProcessNode : public INode,
                      public QwertyNote::IQwertyNoteProtocolObserver,
                      public QwertyNote::ICoreObserver
{
public:
    InProcessNode(QwertyNote::ICore &core, QwertyNote::IQwertyNoteProtocolQuery &protocol);
    InProcessNode(const InProcessNode &) = delete;
    InProcessNode(InProcessNode &&) = delete;
    ~InProcessNode() override;

    void init(const UCallback &callback) override;
    bool shutdown() override;

    bool addObserver(INodeObserver *observer) override;
    bool removeObserver(INodeObserver *observer) override;

    size_t getPeerCount() const override;
    uint32_t getLastLocalBlockHeight() const override;
    uint32_t getLastKnownBlockHeight() const override;
    uint32_t getLocalBlockCount() const override;
    uint32_t getKnownBlockCount() const override;
    uint32_t getNodeHeight() const override;
    uint64_t getLastLocalBlockTimestamp() const override;
    uint64_t getMinimalFee() const override;
    FBlockHeaderInfo getLastLocalBlockHeaderInfo() const override;
    uint32_t getGRBHeight() const override;

    void getNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                      std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                      uint32_t &startHeight,
                      const UCallback &callback) override;
    void getTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                         std::vector<uint32_t> &outsGlobalIndices,
                                         const UCallback &callback) override;
    void getRandomOutsByAmounts(
        std::vector<uint64_t> &&amounts,
        uint64_t outsCount,
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result,
        const UCallback &callback) override;
    void relayTransaction(const QwertyNote::FTransaction &transaction,
                          const UCallback &callback) override;
    void queryBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                     uint64_t timestamp,
                     std::vector<FBlockShortEntry> &newBlocks,
                     uint32_t &startHeight,
                     const UCallback &callback) override;
    void getPoolSymmetricDifference(std::vector<Crypto::FHash> &&knownPoolTxIds,
                                    Crypto::FHash knownBlockId,
                                    bool &isBcActual,
                                    std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
                                    std::vector<Crypto::FHash> &deletedTxIds,
                                    const UCallback &callback) override;
    void getMultisignatureOutputByGlobalIndex(uint64_t amount,
											  uint32_t gindex,
											  FMultiSignatureOutput &out,
											  const UCallback &callback) override;

    void getBlocks(const std::vector<uint32_t> &blockHeights,
                   std::vector<std::vector<FBlockDetails>> &blocks,
                   const UCallback &callback) override;
    void getBlocks(const std::vector<Crypto::FHash> &blockHashes,
                   std::vector<FBlockDetails> &blocks,
                   const UCallback &callback) override;
    void getBlocks(uint64_t timestampBegin,
                   uint64_t timestampEnd,
                   uint32_t blocksNumberLimit,
                   std::vector<FBlockDetails> &blocks,
                   uint32_t &blocksNumberWithinTimestamps,
                   const UCallback &callback) override;
    void getTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                         std::vector<FTransactionDetails> &transactions,
                         const UCallback &callback) override;
    void getTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                    std::vector<FTransactionDetails> &transactions,
                                    const UCallback &callback) override;
    void getPoolTransactions(uint64_t timestampBegin,
                             uint64_t timestampEnd,
                             uint32_t transactionsNumberLimit,
                             std::vector<FTransactionDetails> &transactions,
                             uint64_t &transactionsNumberWithinTimestamps,
                             const UCallback &callback) override;
    void isSynchronized(bool &syncStatus, const UCallback &callback) override;

    InProcessNode &operator=(const InProcessNode &) = delete;
    InProcessNode &operator=(InProcessNode &&) = delete;

private:
    void peerCountUpdated(size_t count) override;
    void lastKnownBlockHeightUpdated(uint32_t height) override;
    void blockchainSynchronized(uint32_t topHeight) override;
    void blockchainUpdated() override;
    void poolUpdated() override;

    void updateLastLocalBlockHeaderInfo();
    void resetLastLocalBlockHeaderInfo();
    void getNewBlocksAsync(std::vector<Crypto::FHash> &knownBlockIds,
                           std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                           uint32_t &startHeight,
                           const UCallback &callback);
    std::error_code doGetNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                                   std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                                   uint32_t &startHeight);

    void getTransactionOutsGlobalIndicesAsync(const Crypto::FHash &transactionHash,
                                              std::vector<uint32_t> &outsGlobalIndices,
                                              const UCallback &callback);
    std::error_code doGetTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                                      std::vector<uint32_t> &outsGlobalIndices);

    void getRandomOutsByAmountsAsync(
        std::vector<uint64_t> &amounts,
        uint64_t outsCount,
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result,
        const UCallback &callback);
    std::error_code doGetRandomOutsByAmounts(
        std::vector<uint64_t> &&amounts,
        uint64_t outsCount,
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result);

    void relayTransactionAsync(const QwertyNote::FTransaction &transaction, const UCallback &callback);
    std::error_code doRelayTransaction(const QwertyNote::FTransaction &transaction);

    void queryBlocksLiteAsync(std::vector<Crypto::FHash> &knownBlockIds,
                              uint64_t timestamp,
                              std::vector<FBlockShortEntry> &newBlocks,
                              uint32_t &startHeight,
                              const UCallback &callback);
    std::error_code doQueryBlocksLite(std::vector<Crypto::FHash> &&knownBlockIds,
                                      uint64_t timestamp,
                                      std::vector<FBlockShortEntry> &newBlocks,
                                      uint32_t &startHeight);

    void getPoolSymmetricDifferenceAsync(std::vector<Crypto::FHash> &&knownPoolTxIds,
                                         Crypto::FHash knownBlockId,
                                         bool &isBcActual,
                                         std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
                                         std::vector<Crypto::FHash> &deletedTxIds,
                                         const UCallback &callback);

    void getOutByMSigGIndexAsync(uint64_t amount,
								 uint32_t gindex, FMultiSignatureOutput &out,
								 const UCallback &callback);

    void getBlocksAsync(const std::vector<uint32_t> &blockHeights,
                        std::vector<std::vector<FBlockDetails>> &blocks,
                        const UCallback &callback);
    std::error_code doGetBlocks(const std::vector<uint32_t> &blockHeights,
                                std::vector<std::vector<FBlockDetails>> &blocks);

    void getBlocksAsync(const std::vector<Crypto::FHash> &blockHashes,
                        std::vector<FBlockDetails> &blocks,
                        const UCallback &callback);
    std::error_code doGetBlocks(const std::vector<Crypto::FHash> &blockHashes,
                                std::vector<FBlockDetails> &blocks);

    void getBlocksAsync(uint64_t timestampBegin,
                        uint64_t timestampEnd,
                        uint32_t blocksNumberLimit,
                        std::vector<FBlockDetails> &blocks,
                        uint32_t &blocksNumberWithinTimestamps,
                        const UCallback &callback);
    std::error_code doGetBlocks(uint64_t timestampBegin,
                                uint64_t timestampEnd,
                                uint32_t blocksNumberLimit,
                                std::vector<FBlockDetails> &blocks,
                                uint32_t &blocksNumberWithinTimestamps);

    void getTransactionsAsync(const std::vector<Crypto::FHash> &transactionHashes,
                              std::vector<FTransactionDetails> &transactions,
                              const UCallback &callback);
    std::error_code doGetTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                                      std::vector<FTransactionDetails> &transactions);

    void getPoolTransactionsAsync(uint64_t timestampBegin,
                                  uint64_t timestampEnd,
                                  uint32_t transactionsNumberLimit,
                                  std::vector<FTransactionDetails> &transactions,
                                  uint64_t &transactionsNumberWithinTimestamps,
                                  const UCallback &callback);
    std::error_code doGetPoolTransactions(uint64_t timestampBegin,
                                          uint64_t timestampEnd,
                                          uint32_t transactionsNumberLimit,
                                          std::vector<FTransactionDetails> &transactions,
                                          uint64_t &transactionsNumberWithinTimestamps);

    void getTransactionsByPaymentIdAsync(const Crypto::FHash &paymentId,
                                         std::vector<FTransactionDetails> &transactions,
                                         const UCallback &callback);
    std::error_code doGetTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                                 std::vector<FTransactionDetails> &transactions);

    void isSynchronizedAsync(bool& syncStatus, const UCallback& callback);

    void workerFunc();
    bool doShutdown();

    enum State
    {
        NOT_INITIALIZED,
        INITIALIZED
    };

    State state;
    QwertyNote::ICore &core;
    QwertyNote::IQwertyNoteProtocolQuery &protocol;
    Tools::ObserverManager<INodeObserver> observerManager;
    FBlockHeaderInfo lastLocalBlockHeaderInfo;

    boost::asio::io_service ioService;
    std::unique_ptr<std::thread> workerThread;
    std::unique_ptr<boost::asio::io_service::work> work;

    QBlockchainExplorerDataBuilder blockchainExplorerDataBuilder;

    mutable std::mutex mutex;
};

} // namespace QwertyNote
