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

#include <atomic>
#include <system_error>
#include <thread>

#include <Common/StringTools.h>

#include <Global/Constants.h>

#include <Http/HttpRequest.h>
#include <Http/HttpResponse.h>

#include <NodeRpcProxy/NodeErrors.h>
#include <NodeRpcProxy/NodeRpcProxy.h>

#include <QwertyNoteCore/CryptoNoteBasicImpl.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/TransactionApi.h>

#include <Rpc/CoreRpcServerCommandsDefinitions.h>
#include <Rpc/HttpClient.h>
#include <Rpc/JsonRpc.h>

#include <System/ContextGroup.h>
#include <System/Dispatcher.h>
#include <System/Event.h>
#include <System/Timer.h>
#include <System/EventLock.h>

#ifndef AUTO_VAL_INIT
#define AUTO_VAL_INIT(n) boost::value_initialized<decltype(n)>()
#endif

using namespace Crypto;
using namespace Common;
using namespace System;
using namespace Qwertycoin;

namespace QwertyNote {

namespace {

std::error_code interpretResponseStatus(const std::string &status)
{
    if (CORE_RPC_STATUS_BUSY == status) {
        return make_error_code(Error::NODE_BUSY);
    } else if (CORE_RPC_STATUS_OK != status) {
        return make_error_code(Error::INTERNAL_NODE_ERROR);
    } else {
        return std::error_code{};
    }
}

} // namespace

NodeRpcProxy::NodeRpcProxy(const std::string &nodeHost, unsigned short nodePort)
    : m_rpcTimeout(10000),
      m_pullInterval(5000),
      m_nodeHost(nodeHost),
      m_nodePort(nodePort),
      m_connected(true),
      m_peerCount(0),
      m_networkHeight(0),
	  m_nodeHeight(0),
	  m_minimalFee(QwertyNote::parameters::MINIMUM_FEE)
{
    resetInternalState();
}

NodeRpcProxy::~NodeRpcProxy()
{
    try {
        shutdown();
    } catch (std::exception &) {
        // do nothing
    }
}

void NodeRpcProxy::init(const INode::UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_state != STATE_NOT_INITIALIZED) {
        callback(make_error_code(Error::ALREADY_INITIALIZED));
        return;
    }

    m_state = STATE_INITIALIZING;
    resetInternalState();
    m_workerThread = std::thread([this, callback] {
        workerThread(callback);
    });
}

bool NodeRpcProxy::shutdown()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_state == STATE_NOT_INITIALIZED) {
        return true;
    } else if (m_state == STATE_INITIALIZING) {
        m_cv_initialized.wait(lock, [this] { return m_state != STATE_INITIALIZING; });
        if (m_state == STATE_NOT_INITIALIZED) {
            return true;
        }
    }

    assert(m_state == STATE_INITIALIZED);

    assert(m_dispatcher != nullptr);

    m_dispatcher->remoteSpawn([this]() {
        m_stop = true;
        // Run all spawned contexts
        m_dispatcher->yield();
    });

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_state = STATE_NOT_INITIALIZED;
    m_cv_initialized.notify_all();

    return true;
}

void NodeRpcProxy::resetInternalState()
{
    m_stop = false;
    m_peerCount.store(0, std::memory_order_relaxed);
    m_networkHeight.store(0, std::memory_order_relaxed);
    m_GRBHeight.store(0, std::memory_order_relaxed);
    lastLocalBlockHeaderInfo.uIndex = 0;
    lastLocalBlockHeaderInfo.uMajorVersion = 0;
    lastLocalBlockHeaderInfo.uMinorVersion = 0;
    lastLocalBlockHeaderInfo.uTimestamp = 0;
    lastLocalBlockHeaderInfo.sHash = NULL_HASH;
    lastLocalBlockHeaderInfo.sPrevHash = NULL_HASH;
    lastLocalBlockHeaderInfo.uNonce = 0;
    lastLocalBlockHeaderInfo.bIsAlternative = false;
    lastLocalBlockHeaderInfo.uDepth = 0;
    lastLocalBlockHeaderInfo.sDifficulty = 0;
    lastLocalBlockHeaderInfo.uReward = 0;
    m_knownTxs.clear();
}

void NodeRpcProxy::workerThread(const INode::UCallback &initialized_callback)
{
    try {
        Dispatcher dispatcher;
        m_dispatcher = &dispatcher;
        ContextGroup contextGroup(dispatcher);
        m_context_group = &contextGroup;
        HttpClient httpClient(dispatcher, m_nodeHost, m_nodePort);
        m_httpClient = &httpClient;
        Event httpEvent(dispatcher);
        m_httpEvent = &httpEvent;
        m_httpEvent->set();

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            assert(m_state == STATE_INITIALIZING);

            m_state = STATE_INITIALIZED;
            m_cv_initialized.notify_all();
        }

        initialized_callback(std::error_code());

        contextGroup.spawn([this]() {
            Timer pullTimer(*m_dispatcher);
            while (!m_stop) {
                updateNodeStatus();
                if (!m_stop) {
                    pullTimer.sleep(std::chrono::milliseconds(m_pullInterval));
                }
            }
        });

        contextGroup.wait();
        // Make sure all remote spawns are executed
        m_dispatcher->yield();
    } catch (std::exception &) {
        // do nothing
    }

    m_dispatcher = nullptr;
    m_context_group = nullptr;
    m_httpClient = nullptr;
    m_httpEvent = nullptr;
    m_connected = false;
    m_rpcProxyObserverManager.notify(&INodeRpcProxyObserver::connectionStatusUpdated, m_connected);
}

void NodeRpcProxy::updateNodeStatus()
{
    bool updateBlockchain = true;
    while (updateBlockchain) {
        updateBlockchainStatus();
        updateBlockchain = !updatePoolStatus();
    }
}

bool NodeRpcProxy::updatePoolStatus()
{
    std::vector<Crypto::FHash> knownTxs = getKnownTxsVector();
    Crypto::FHash tailBlock = lastLocalBlockHeaderInfo.sHash;

    bool isBcActual = false;
    std::vector<std::unique_ptr<ITransactionReader>> addedTxs;
    std::vector<Crypto::FHash> deletedTxsIds;

    std::error_code ec = doGetPoolSymmetricDifference(std::move(knownTxs),
                                                      tailBlock,
                                                      isBcActual,
                                                      addedTxs,
                                                      deletedTxsIds);
    if (ec) {
        return true;
    }

    if (!isBcActual) {
        return false;
    }

    if (!addedTxs.empty() || !deletedTxsIds.empty()) {
        updatePoolState(addedTxs, deletedTxsIds);
        m_observerManager.notify(&INodeObserver::poolChanged);
    }

    return true;
}

void NodeRpcProxy::updateBlockchainStatus()
{
    QwertyNote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::request req = AUTO_VAL_INIT(req);
    QwertyNote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::response rsp = AUTO_VAL_INIT(rsp);

    std::error_code ec = jsonRpcCommand("getlastblockheader", req, rsp);

    if (!ec) {
        Crypto::FHash blockHash;
        Crypto::FHash prevBlockHash;
        if (!parse_hash256(rsp.block_header.hash, blockHash)
            || !parse_hash256(rsp.block_header.prev_hash, prevBlockHash)) {
            return;
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        uint32_t blockIndex = rsp.block_header.height;
        if (blockHash != lastLocalBlockHeaderInfo.sHash) {
            lastLocalBlockHeaderInfo.uIndex = blockIndex;
            lastLocalBlockHeaderInfo.uMajorVersion = rsp.block_header.major_version;
            lastLocalBlockHeaderInfo.uMinorVersion = rsp.block_header.minor_version;
            lastLocalBlockHeaderInfo.uTimestamp = rsp.block_header.timestamp;
            lastLocalBlockHeaderInfo.sHash = blockHash;
            lastLocalBlockHeaderInfo.sPrevHash = prevBlockHash;
            lastLocalBlockHeaderInfo.uNonce = rsp.block_header.nonce;
            lastLocalBlockHeaderInfo.bIsAlternative = rsp.block_header.orphan_status;
            lastLocalBlockHeaderInfo.uDepth = rsp.block_header.depth;
            lastLocalBlockHeaderInfo.sDifficulty = rsp.block_header.difficulty;
            lastLocalBlockHeaderInfo.uReward = rsp.block_header.reward;

            lock.unlock();

            m_observerManager.notify(&INodeObserver::localBlockchainUpdated, blockIndex);
        }
    }

    QwertyNote::COMMAND_RPC_GET_INFO::request getInfoReq = AUTO_VAL_INIT(getInfoReq);
    QwertyNote::COMMAND_RPC_GET_INFO::response getInfoResp = AUTO_VAL_INIT(getInfoResp);

    ec = jsonCommand("/getinfo", getInfoReq, getInfoResp);
    if (!ec) {
        // A quirk to let wallets work with previous versions daemons. Previous daemons didn't
        // have the 'last_known_block_index' parameter in RPC so it may have zero value.
        std::unique_lock<std::mutex> lock(m_mutex);
        auto lastKnownBlockIndex = std::max(getInfoResp.last_known_block_index,
                                            lastLocalBlockHeaderInfo.uIndex);
        lock.unlock();
        if (m_networkHeight.load(std::memory_order_relaxed) != lastKnownBlockIndex) {
            m_networkHeight.store(lastKnownBlockIndex, std::memory_order_relaxed);
            m_observerManager.notify(&INodeObserver::lastKnownBlockHeightUpdated,
                                     m_networkHeight.load(std::memory_order_relaxed));
        }

        updatePeerCount(getInfoResp.incoming_connections_count
                        + getInfoResp.outgoing_connections_count);

        m_minimalFee.store(getInfoResp.min_tx_fee, std::memory_order_relaxed);
        m_nodeHeight.store(getInfoResp.height, std::memory_order_relaxed);
        m_GRBHeight.store(getInfoResp.height, std::memory_order_relaxed);
    }

    if (m_connected != m_httpClient->isConnected()) {
        m_connected = m_httpClient->isConnected();
        m_rpcProxyObserverManager.notify(&INodeRpcProxyObserver::connectionStatusUpdated,
                                         m_connected);
    }
}

void NodeRpcProxy::updatePeerCount(size_t peerCount)
{
    if (peerCount != m_peerCount) {
        m_peerCount = peerCount;
        m_observerManager.notify(&INodeObserver::peerCountUpdated,
                                 m_peerCount.load(std::memory_order_relaxed));
    }
}

void NodeRpcProxy::updatePoolState(const std::vector<std::unique_ptr<ITransactionReader>> &addedTxs,
                                   const std::vector<Crypto::FHash> &deletedTxsIds)
{
    for (const auto& hash : deletedTxsIds) {
        m_knownTxs.erase(hash);
    }

    for (const auto& tx : addedTxs) {
        FHash hash = tx->getTransactionHash();
        m_knownTxs.emplace(std::move(hash));
    }
}

std::vector<Crypto::FHash> NodeRpcProxy::getKnownTxsVector() const
{
    return std::vector<Crypto::FHash>(m_knownTxs.begin(), m_knownTxs.end());
}

bool NodeRpcProxy::addObserver(INodeObserver *observer)
{
    return m_observerManager.add(observer);
}

bool NodeRpcProxy::removeObserver(INodeObserver *observer)
{
    return m_observerManager.remove(observer);
}

bool NodeRpcProxy::addObserver(QwertyNote::INodeRpcProxyObserver *observer)
{
    return m_rpcProxyObserverManager.add(observer);
}

bool NodeRpcProxy::removeObserver(QwertyNote::INodeRpcProxyObserver *observer)
{
    return m_rpcProxyObserverManager.remove(observer);
}

size_t NodeRpcProxy::getPeerCount() const
{
    return m_peerCount.load(std::memory_order_relaxed);
}

uint32_t NodeRpcProxy::getLastLocalBlockHeight() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return lastLocalBlockHeaderInfo.uIndex;
}

uint32_t NodeRpcProxy::getLastKnownBlockHeight() const
{
    return m_networkHeight.load(std::memory_order_relaxed);
}

uint32_t NodeRpcProxy::getLocalBlockCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return lastLocalBlockHeaderInfo.uIndex + 1;
}

uint32_t NodeRpcProxy::getKnownBlockCount() const
{
    return m_networkHeight.load(std::memory_order_relaxed) + 1;
}

uint64_t NodeRpcProxy::getLastLocalBlockTimestamp() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return lastLocalBlockHeaderInfo.uTimestamp;
}

uint64_t NodeRpcProxy::getMinimalFee() const
{
    return m_minimalFee.load(std::memory_order_relaxed);
}

FBlockHeaderInfo NodeRpcProxy::getLastLocalBlockHeaderInfo() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return lastLocalBlockHeaderInfo;
}

uint32_t NodeRpcProxy::getGRBHeight() const
{
    return m_GRBHeight;
}

uint32_t NodeRpcProxy::getNodeHeight() const
{
    return m_nodeHeight.load(std::memory_order_relaxed);
}

void NodeRpcProxy::relayTransaction(const QwertyNote::FTransaction &transaction,
                                    const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest(std::bind(&NodeRpcProxy::doRelayTransaction, this, transaction), callback);
}

void NodeRpcProxy::getRandomOutsByAmounts(
    std::vector<uint64_t> &&amounts,
    uint64_t outsCount,
    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &outs,
    const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest(
        std::bind(
            &NodeRpcProxy::doGetRandomOutsByAmounts,
            this,
            std::move(amounts),
            outsCount,
            std::ref(outs)
        ),
        callback);
}

void NodeRpcProxy::getNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                                std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                                uint32_t &startHeight,
                                const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest(
        std::bind(
            &NodeRpcProxy::doGetNewBlocks,
            this,
            std::move(knownBlockIds),
            std::ref(newBlocks),
            std::ref(startHeight)
        ),
        callback);
}

void NodeRpcProxy::getTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                                   std::vector<uint32_t> &outsGlobalIndices,
                                                   const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest(
        std::bind(
            &NodeRpcProxy::doGetTransactionOutsGlobalIndices,
            this,
            transactionHash,
            std::ref(outsGlobalIndices)
        ),
        callback);
}

void NodeRpcProxy::queryBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                               uint64_t timestamp,
                               std::vector<FBlockShortEntry> &newBlocks,
                               uint32_t &startHeight,
                               const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest(
        std::bind(
            &NodeRpcProxy::doQueryBlocksLite,
            this,
            std::move(knownBlockIds),
            timestamp,
            std::ref(newBlocks),
            std::ref(startHeight)
        ),
        callback);
}

void NodeRpcProxy::getPoolSymmetricDifference(
    std::vector<Crypto::FHash> &&knownPoolTxIds,
    Crypto::FHash knownBlockId,
    bool &isBcActual,
    std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
    std::vector<Crypto::FHash> &deletedTxIds,
    const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    scheduleRequest([
        this,
        knownPoolTxIds,
        knownBlockId,
        &isBcActual,
        &newTxs,
        &deletedTxIds] () mutable -> std::error_code {
        return this->doGetPoolSymmetricDifference(std::move(knownPoolTxIds),
                                                  knownBlockId,
                                                  isBcActual,
                                                  newTxs,
                                                  deletedTxIds);
    }, callback);
}

void NodeRpcProxy::getMultisignatureOutputByGlobalIndex(uint64_t amount,
														uint32_t gindex,
														FMultiSignatureOutput &out,
														const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getBlocks(const std::vector<uint32_t> &blockHeights,
                             std::vector<std::vector<FBlockDetails>> &blocks,
                             const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getBlocks(uint64_t timestampBegin,
                             uint64_t timestampEnd,
                             uint32_t blocksNumberLimit,
                             std::vector<FBlockDetails> &blocks,
                             uint32_t &blocksNumberWithinTimestamps,
                             const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getBlocks(const std::vector<Crypto::FHash> &blockHashes,
                             std::vector<FBlockDetails> &blocks,
                             const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                                   std::vector<FTransactionDetails> &transactions,
                                   const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getPoolTransactions(uint64_t timestampBegin,
                                       uint64_t timestampEnd,
                                       uint32_t transactionsNumberLimit,
                                       std::vector<FTransactionDetails> &transactions,
                                       uint64_t &transactionsNumberWithinTimestamps,
                                       const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::getTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                              std::vector<FTransactionDetails> &transactions,
                                              const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    // TODO: NOT IMPLEMENTED!
    callback(std::error_code{});
}

void NodeRpcProxy::isSynchronized(bool &syncStatus, const UCallback &callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != STATE_INITIALIZED) {
        syncStatus = false;
        callback(make_error_code(Error::NOT_INITIALIZED));
        return;
    }

    syncStatus = (lastLocalBlockHeaderInfo.uIndex == m_networkHeight);
    callback(std::error_code{});
}

std::error_code NodeRpcProxy::doRelayTransaction(const QwertyNote::FTransaction &transaction)
{
    COMMAND_RPC_SEND_RAW_TX::request req;
    COMMAND_RPC_SEND_RAW_TX::response rsp;

    req.tx_as_hex = toHex(toBinaryArray(transaction));

    return jsonCommand("/sendrawtransaction", req, rsp);
}

std::error_code NodeRpcProxy::doGetRandomOutsByAmounts(
    std::vector<uint64_t> &amounts,
    uint64_t outsCount,
    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &outs)
{
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response rsp = AUTO_VAL_INIT(rsp);

    req.amounts = std::move(amounts);
    req.outs_count = outsCount;

    std::error_code ec = binaryCommand("/getrandom_outs.bin", req, rsp);
    if (!ec) {
        outs = std::move(rsp.outs);
    }

    return ec;
}

std::error_code NodeRpcProxy::doGetNewBlocks(
    std::vector<Crypto::FHash> &knownBlockIds,
    std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
    uint32_t &startHeight)
{
    QwertyNote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
    QwertyNote::COMMAND_RPC_GET_BLOCKS_FAST::response rsp = AUTO_VAL_INIT(rsp);

    req.block_ids = std::move(knownBlockIds);

    std::error_code ec = binaryCommand("/getblocks.bin", req, rsp);
    if (!ec) {
        newBlocks = std::move(rsp.blocks);
        startHeight = static_cast<uint32_t>(rsp.start_height);
    }

    return ec;
}

std::error_code NodeRpcProxy::doGetTransactionOutsGlobalIndices(
    const Crypto::FHash &transactionHash,
    std::vector<uint32_t> &outsGlobalIndices)
{
    QwertyNote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req = AUTO_VAL_INIT(req);
    QwertyNote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response rsp = AUTO_VAL_INIT(rsp);

    req.txid = transactionHash;

    std::error_code ec = binaryCommand("/get_o_indexes.bin", req, rsp);
    if (!ec) {
        outsGlobalIndices.clear();
        for (auto idx : rsp.o_indexes) {
            outsGlobalIndices.push_back(static_cast<uint32_t>(idx));
        }
    }

    return ec;
}

std::error_code NodeRpcProxy::doQueryBlocksLite(const std::vector<Crypto::FHash> &knownBlockIds,
                                                uint64_t timestamp,
                                                std::vector<QwertyNote::FBlockShortEntry> &newBlocks,
                                                uint32_t &startHeight)
{
    QwertyNote::COMMAND_RPC_QUERY_BLOCKS_LITE::request req = AUTO_VAL_INIT(req);
    QwertyNote::COMMAND_RPC_QUERY_BLOCKS_LITE::response rsp = AUTO_VAL_INIT(rsp);

    req.blockIds = knownBlockIds;
    req.timestamp = timestamp;

    std::error_code ec = binaryCommand("/queryblockslite.bin", req, rsp);
    if (ec) {
        return ec;
    }

    startHeight = static_cast<uint32_t>(rsp.startHeight);

    for (auto &item: rsp.items) {
        FBlockShortEntry bse;
        bse.bHasBlock = false;

        bse.sBlockHash = std::move(item.blockId);
        if (!item.block.empty()) {
            if (!fromBinaryArray(bse.sBlock, asBinaryArray(item.block))) {
                return std::make_error_code(std::errc::invalid_argument);
            }

            bse.bHasBlock = true;
        }

        for (const auto &txp: item.txPrefixes) {
            FTransactionShortInfo tsi;
            tsi.sTxId = txp.txHash;
            tsi.sTxPrefix = txp.txPrefix;
            bse.vTxsShortInfo.push_back(std::move(tsi));
        }

        newBlocks.push_back(std::move(bse));
    }

    return std::error_code{};
}

std::error_code NodeRpcProxy::doGetPoolSymmetricDifference(
    std::vector<Crypto::FHash> &&knownPoolTxIds,
    Crypto::FHash knownBlockId,
    bool &isBcActual,
    std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
    std::vector<Crypto::FHash> &deletedTxIds)
{
    QwertyNote::COMMAND_RPC_GET_POOL_CHANGES_LITE::request req = AUTO_VAL_INIT(req);
    QwertyNote::COMMAND_RPC_GET_POOL_CHANGES_LITE::response rsp = AUTO_VAL_INIT(rsp);

    req.tailBlockId = knownBlockId;
    req.knownTxsIds = knownPoolTxIds;

    std::error_code ec = binaryCommand("/get_pool_changes_lite.bin", req, rsp);

    if (ec) {
        return ec;
    }

    isBcActual = rsp.isTailBlockActual;

    deletedTxIds = std::move(rsp.deletedTxsIds);

    for (const auto &tpi : rsp.addedTxs) {
        newTxs.push_back(createTransactionPrefix(tpi.txPrefix, tpi.txHash));
    }

    return ec;
}

void NodeRpcProxy::scheduleRequest(std::function<std::error_code()> &&procedure,
                                   const UCallback &callback)
{
    // callback is located on stack, so copy it inside binder
    class Wrapper
    {
    public:
        Wrapper(std::function<void(std::function<std::error_code()> &, UCallback &)> &&_func,
                std::function<std::error_code()> &&_procedure,
                const UCallback &_callback)
            : func(std::move(_func)),
              procedure(std::move(_procedure)),
              callback(std::move(_callback))
        {
        }

        Wrapper(const Wrapper &other)
            : func(other.func),
              procedure(other.procedure),
              callback(other.callback)
        {
        }

        Wrapper(Wrapper &&other) noexcept
            : func(std::move(other.func)),
              procedure(std::move(other.procedure)),
              callback(std::move(other.callback))
        {
        }

        void operator()()
        {
            func(procedure, callback);
        }
    private:
        std::function<void(std::function<std::error_code()>&, UCallback&)> func;
        std::function<std::error_code()> procedure;
        UCallback callback;
    };

    assert(m_dispatcher != nullptr && m_context_group != nullptr);

    m_dispatcher->remoteSpawn(Wrapper([this](std::function<std::error_code()> &procedure,
											 UCallback &callback) {
        m_context_group->spawn(Wrapper([this](std::function<std::error_code()> &procedure,
                                              const UCallback &callback) {
            if (m_stop) {
                callback(std::make_error_code(std::errc::operation_canceled));
            } else {
                std::error_code ec = procedure();
                if (m_connected != m_httpClient->isConnected()) {
                    m_connected = m_httpClient->isConnected();
                    m_rpcProxyObserverManager.notify(
                        &INodeRpcProxyObserver::connectionStatusUpdated,
                        m_connected);
                }
                callback(m_stop ? std::make_error_code(std::errc::operation_canceled) : ec);
            }
        }, std::move(procedure), std::move(callback)));
    }, std::move(procedure), callback));
}

template <typename Request, typename Response>
std::error_code NodeRpcProxy::binaryCommand(const std::string &url,
                                            const Request &req,
                                            Response &res)
{
    std::error_code ec;

    try {
        EventLock eventLock(*m_httpEvent);
        invokeBinaryCommand(*m_httpClient, url, req, res);
        ec = interpretResponseStatus(res.status);
    } catch (const ConnectException &) {
        ec = make_error_code(Error::CONNECT_ERROR);
    } catch (const std::exception &) {
        ec = make_error_code(Error::NETWORK_ERROR);
    }

    return ec;
}

template <typename Request, typename Response>
std::error_code NodeRpcProxy::jsonCommand(const std::string &url, const Request &req, Response &res)
{
    std::error_code ec;

    try {
        EventLock eventLock(*m_httpEvent);
        invokeJsonCommand(*m_httpClient, url, req, res);
        ec = interpretResponseStatus(res.status);
    } catch (const ConnectException &) {
        ec = make_error_code(Error::CONNECT_ERROR);
    } catch (const std::exception &) {
        ec = make_error_code(Error::NETWORK_ERROR);
    }

    return ec;
}

template <typename Request, typename Response>
std::error_code NodeRpcProxy::jsonRpcCommand(const std::string &method,
                                             const Request &req,
                                             Response &res)
{
    std::error_code ec = make_error_code(Error::INTERNAL_NODE_ERROR);

    try {
        EventLock eventLock(*m_httpEvent);

        JsonRpc::JsonRpcRequest jsReq;

        jsReq.setMethod(method);
        jsReq.setParams(req);

        HttpRequest httpReq;
        HttpResponse httpRes;

        httpReq.addHeader("Content-Type", "application/json");
        httpReq.setUrl("/json_rpc");
        httpReq.setBody(jsReq.getBody());

        m_httpClient->request(httpReq, httpRes);

        JsonRpc::JsonRpcResponse jsRes;

        if (httpRes.getStatus() == HttpResponse::STATUS_200) {
            jsRes.parse(httpRes.getBody());
            if (jsRes.getResult(res)) {
                ec = interpretResponseStatus(res.status);
            }
        }
    } catch (const ConnectException &) {
        ec = make_error_code(Error::CONNECT_ERROR);
    } catch (const std::exception &) {
        ec = make_error_code(Error::NETWORK_ERROR);
    }

    return ec;
}

} // namespace QwertyNote
