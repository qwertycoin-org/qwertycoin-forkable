// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2016-2017, The Karbowanec developers
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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <Common/ObserverManager.h>
#include <Global/QwertyNoteConfig.h>
#include <INode.h>

namespace System {

class ContextGroup;
class Dispatcher;
class Event;

} // namespace System

namespace QwertyNote {

class HttpClient;

class INodeRpcProxyObserver
{
public:
    virtual ~INodeRpcProxyObserver() = default;

    virtual void connectionStatusUpdated(bool connected) {}
};

class NodeRpcProxy : public QwertyNote::INode
{
    enum State
    {
        STATE_NOT_INITIALIZED,
        STATE_INITIALIZING,
        STATE_INITIALIZED
    };

public:
    NodeRpcProxy(const std::string &nodeHost, unsigned short nodePort);
    ~NodeRpcProxy() override;

    void init(const UCallback &callback) override;
    bool shutdown() override;

    bool addObserver(QwertyNote::INodeObserver *observer) override;
    bool removeObserver(QwertyNote::INodeObserver *observer) override;

    virtual bool addObserver(QwertyNote::INodeRpcProxyObserver *observer);
    virtual bool removeObserver(QwertyNote::INodeRpcProxyObserver *observer);

    size_t getPeerCount() const override;
    uint32_t getLastLocalBlockHeight() const override;
    uint32_t getLastKnownBlockHeight() const override;
    uint32_t getLocalBlockCount() const override;
    uint32_t getKnownBlockCount() const override;
    uint64_t getLastLocalBlockTimestamp() const override;
    uint64_t getMinimalFee() const override;
    uint32_t getNodeHeight() const override;
    FBlockHeaderInfo getLastLocalBlockHeaderInfo() const override;
    uint32_t getGRBHeight() const override;

    void relayTransaction(const QwertyNote::FTransaction &transaction,
                          const UCallback &callback) override;
    void getRandomOutsByAmounts(
        std::vector<uint64_t> &&amounts,
        uint64_t outsCount,
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result,
        const UCallback &callback) override;
    void getNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                      std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                      uint32_t &startHeight,
                      const UCallback &callback) override;
    void getTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                         std::vector<uint32_t> &outsGlobalIndices,
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

    unsigned int rpcTimeout() const { return m_rpcTimeout; }
    void rpcTimeout(unsigned int val) { m_rpcTimeout = val; }

    const std::string m_nodeHost;
    const unsigned short m_nodePort;

private:
    void resetInternalState();
    void workerThread(const UCallback &initialized_callback);

    std::vector<Crypto::FHash> getKnownTxsVector() const;

    void updateNodeStatus();
    void updateBlockchainStatus();
    bool updatePoolStatus();
    void updatePeerCount(size_t peerCount);
    void updatePoolState(const std::vector<std::unique_ptr<ITransactionReader>> &addedTxs,
                         const std::vector<Crypto::FHash> &deletedTxsIds);

    std::error_code doRelayTransaction(const QwertyNote::FTransaction &transaction);
    std::error_code doGetRandomOutsByAmounts(
        std::vector<uint64_t> &amounts,
        uint64_t outsCount,
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result);
    std::error_code doGetNewBlocks(std::vector<Crypto::FHash> &knownBlockIds,
                                   std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                                   uint32_t &startHeight);
    std::error_code doGetTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                                      std::vector<uint32_t> &outsGlobalIndices);
    std::error_code doQueryBlocksLite(const std::vector<Crypto::FHash> &knownBlockIds,
                                      uint64_t timestamp,
                                      std::vector<QwertyNote::FBlockShortEntry> &newBlocks,
                                      uint32_t &startHeight);
    std::error_code doGetPoolSymmetricDifference(std::vector<Crypto::FHash> &&knownPoolTxIds,
                                                 Crypto::FHash knownBlockId,
                                                 bool &isBcActual,
                                                 std::vector<
                                                     std::unique_ptr<ITransactionReader>
                                                     > &newTxs,
                                                 std::vector<Crypto::FHash> &deletedTxIds);

    void scheduleRequest(std::function<std::error_code()> &&procedure, const UCallback &callback);

    template <typename Request, typename Response>
    std::error_code binaryCommand(const std::string &url, const Request &req, Response &res);
    template <typename Request, typename Response>
    std::error_code jsonCommand(const std::string &url, const Request &req, Response &res);
    template <typename Request, typename Response>
    std::error_code jsonRpcCommand(const std::string &method, const Request &req, Response &res);

private:
    State m_state = STATE_NOT_INITIALIZED;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv_initialized;
    std::thread m_workerThread;
    System::Dispatcher *m_dispatcher = nullptr;
    System::ContextGroup *m_context_group = nullptr;
    Tools::QObserverManager<QwertyNote::INodeObserver> m_observerManager;
    Tools::QObserverManager<QwertyNote::INodeRpcProxyObserver> m_rpcProxyObserverManager;

    unsigned int m_rpcTimeout;
    HttpClient *m_httpClient = nullptr;
    System::Event *m_httpEvent = nullptr;

    uint64_t m_pullInterval;

    // Internal state
    bool m_stop = false;
    std::atomic<size_t> m_peerCount;
    std::atomic<uint32_t> m_networkHeight;
    std::atomic<uint64_t> m_nodeHeight;
    std::atomic<uint64_t> m_minimalFee;
    std::atomic<uint32_t> m_GRBHeight;

    FBlockHeaderInfo lastLocalBlockHeaderInfo;
    // protect it with mutex if decided to add worker threads
    std::unordered_set<Crypto::FHash> m_knownTxs;

    bool m_connected;
};

} // namespace QwertyNote
