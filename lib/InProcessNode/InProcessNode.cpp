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

#include <functional>

#include <boost/utility/value_init.hpp>

#include <Common/StringTools.h>

#include <Global/Constants.h>
#include <Global/QwertyNoteConfig.h>

#include <InProcessNode/InProcessNode.h>
#include <InProcessNode/InProcessNodeErrors.h>

#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/IBlock.h>
#include <QwertyNoteCore/TransactionApi.h>
#include <QwertyNoteCore/VerificationContext.h>

#include <QwertyNoteProtocol/CryptoNoteProtocolHandlerCommon.h>

using namespace Crypto;
using namespace Common;
using namespace Qwertycoin;

namespace QwertyNote {

namespace {

    uint64_t getBlockReward(const FBlock &block)
    {
        uint64_t reward = 0;
        for (const FTransactionOutput& out : block.sBaseTransaction.vOutputs) {
            reward += out.uAmount;
        }

        return reward;
    }

} // namespace

InProcessNode::InProcessNode(QwertyNote::ICore &core,
                             QwertyNote::IQwertyNoteProtocolQuery &protocol)
    : state(NOT_INITIALIZED),
      core(core),
      protocol(protocol),
      blockchainExplorerDataBuilder(core, protocol)
{
    resetLastLocalBlockHeaderInfo();
}

InProcessNode::~InProcessNode()
{
    doShutdown();
}

bool InProcessNode::addObserver(INodeObserver *observer)
{
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return observerManager.add(observer);
}

bool InProcessNode::removeObserver(INodeObserver *observer)
{
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return observerManager.remove(observer);
}

void InProcessNode::init(const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    std::error_code ec;

    if (state != NOT_INITIALIZED) {
        ec = make_error_code(QwertyNote::Error::ALREADY_INITIALIZED);
    } else {
        protocol.addObserver(this);
        core.addObserver(this);

        work.reset(new boost::asio::io_service::work(ioService));
        workerThread.reset(new std::thread(&InProcessNode::workerFunc, this));
        updateLastLocalBlockHeaderInfo();

        state = INITIALIZED;
    }

    ioService.post(std::bind(callback, ec));
}

bool InProcessNode::shutdown()
{
    return doShutdown();
}

bool InProcessNode::doShutdown()
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        return false;
    }

    protocol.removeObserver(this);
    core.removeObserver(this);
    resetLastLocalBlockHeaderInfo();
    state = NOT_INITIALIZED;

    work.reset();
    ioService.stop();
    workerThread->join();
    ioService.reset();

    return true;
}

void InProcessNode::workerFunc()
{
    ioService.run();
}

void InProcessNode::getNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                                 std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                                 uint32_t &startHeight,
                                 const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::getNewBlocksAsync,
        this,
        std::move(knownBlockIds),
        std::ref(newBlocks),
        std::ref(startHeight),
        callback
    ));
}

void InProcessNode::getNewBlocksAsync(std::vector<Crypto::FHash> &knownBlockIds,
                                      std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                                      uint32_t &startHeight, const UCallback &callback)
{
    std::error_code ec = doGetNewBlocks(std::move(knownBlockIds), newBlocks, startHeight);
    callback(ec);
}

// it's always protected with mutex
std::error_code InProcessNode::doGetNewBlocks(
    std::vector<Crypto::FHash> &&knownBlockIds,
    std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
    uint32_t &startHeight)
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            return make_error_code(QwertyNote::Error::NOT_INITIALIZED);
        }
    }

    try {
        // TODO code duplication see RpcServer::on_get_blocks()
        if (knownBlockIds.empty()) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        if (knownBlockIds.back() != core.getBlockIdByHeight(0)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        uint32_t totalBlockCount;
        std::vector<Crypto::FHash> supplement = core.findBlockchainSupplement(
            knownBlockIds, QwertyNote::COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT,
            totalBlockCount,
            startHeight
        );

        for (const auto &blockId : supplement) {
            assert(core.have_block(blockId));

            auto completeBlock = core.getBlock(blockId);

            assert(completeBlock != nullptr);

            QwertyNote::BlockCompleteEntry be;
            be.block = asString(toBinaryArray(completeBlock->getBlock()));

            be.txs.reserve(completeBlock->getTransactionCount());
            for (size_t i = 0; i < completeBlock->getTransactionCount(); ++i) {
                be.txs.push_back(asString(toBinaryArray(completeBlock->getTransaction(i))));
            }

            newBlocks.push_back(std::move(be));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code();
}

void InProcessNode::getTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                                    std::vector<uint32_t> &outsGlobalIndices,
                                                    const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::getTransactionOutsGlobalIndicesAsync,
        this,
        std::cref(transactionHash),
        std::ref(outsGlobalIndices),
        callback
    ));
}

void InProcessNode::getTransactionOutsGlobalIndicesAsync(const Crypto::FHash &transactionHash,
                                                         std::vector<uint32_t>& outsGlobalIndices,
                                                         const UCallback &callback)
{
    std::error_code ec = doGetTransactionOutsGlobalIndices(transactionHash, outsGlobalIndices);
    callback(ec);
}

// it's always protected with mutex
std::error_code InProcessNode::doGetTransactionOutsGlobalIndices(
    const Crypto::FHash &transactionHash,
    std::vector<uint32_t> &outsGlobalIndices)
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            return make_error_code(QwertyNote::Error::NOT_INITIALIZED);
        }
    }

    try {
        bool r = core.getTxOutputsGlobalIndexes(transactionHash, outsGlobalIndices);
        if(!r) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

void InProcessNode::getRandomOutsByAmounts(
    std::vector<uint64_t> &&amounts,
    uint64_t outsCount,
    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result,
    const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::getRandomOutsByAmountsAsync,
        this,
        std::move(amounts),
        outsCount,
        std::ref(result),
        callback
    ));
}

void InProcessNode::getRandomOutsByAmountsAsync(
    std::vector<uint64_t> &amounts,
    uint64_t outsCount,
    std::vector<QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result,
    const UCallback &callback)
{
    std::error_code ec = doGetRandomOutsByAmounts(std::move(amounts), outsCount, result);
    callback(ec);
}

// it's always protected with mutex
std::error_code InProcessNode::doGetRandomOutsByAmounts(
    std::vector<uint64_t> &&amounts,
    uint64_t outsCount,
    std::vector<QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> &result)
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            return make_error_code(QwertyNote::Error::NOT_INITIALIZED);
        }
    }

    try {
        QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;
        QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req;
        req.amounts = amounts;
        req.outs_count = outsCount;

        if(!core.get_random_outs_for_amounts(req, res)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        result = std::move(res.outs);
    } catch (std::system_error& e) {
        return e.code();
    } catch (std::exception&) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code();
}


void InProcessNode::relayTransaction(const QwertyNote::FTransaction &transaction,
                                     const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::relayTransactionAsync,
        this,
        transaction,
        callback
    ));
}

void InProcessNode::relayTransactionAsync(const QwertyNote::FTransaction &transaction,
                                          const UCallback &callback)
{
    std::error_code ec = doRelayTransaction(transaction);
    callback(ec);
}

// it's always protected with mutex
std::error_code InProcessNode::doRelayTransaction(const QwertyNote::FTransaction &transaction)
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            return make_error_code(QwertyNote::Error::NOT_INITIALIZED);
        }
    }

    try {
        QwertyNote::BinaryArray transactionBinaryArray = toBinaryArray(transaction);
        QwertyNote::tx_verification_context tvc
            = boost::value_initialized<QwertyNote::tx_verification_context>();

        if (!core.handle_incoming_tx(transactionBinaryArray, tvc, false, false)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        if(tvc.m_verification_failed) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        if(!tvc.m_should_be_relayed) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }

        QwertyNote::NOTIFY_NEW_TRANSACTIONS::request r;
        r.txs.push_back(asString(transactionBinaryArray));
        core.get_protocol()->relay_transactions(r);
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

size_t InProcessNode::getPeerCount() const
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
        }
    }

    return protocol.getPeerCount();
}

uint32_t InProcessNode::getLocalBlockCount() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return lastLocalBlockHeaderInfo.uIndex + 1;
}

uint32_t InProcessNode::getNodeHeight() const
{
    return getLocalBlockCount();
}

uint32_t InProcessNode::getKnownBlockCount() const
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
        }
    }

    return protocol.getObservedHeight();
}

uint32_t InProcessNode::getLastLocalBlockHeight() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return lastLocalBlockHeaderInfo.uIndex;
}

uint32_t InProcessNode::getLastKnownBlockHeight() const
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != INITIALIZED) {
            throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
        }
    }

    return protocol.getObservedHeight() - 1;
}

uint64_t InProcessNode::getLastLocalBlockTimestamp() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return lastLocalBlockHeaderInfo.uTimestamp;
}

uint64_t InProcessNode::getMinimalFee() const
{
    std::unique_lock<std::mutex> lock(mutex);

    return core.getMinimalFee();
}

FBlockHeaderInfo InProcessNode::getLastLocalBlockHeaderInfo() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        throw std::system_error(make_error_code(QwertyNote::Error::NOT_INITIALIZED));
    }

    return lastLocalBlockHeaderInfo;
}

uint32_t InProcessNode::getGRBHeight() const
{
    // stub, not implemented yet
    return getLocalBlockCount();
}

void InProcessNode::peerCountUpdated(size_t count)
{
    observerManager.notify(&INodeObserver::peerCountUpdated, count);
}

void InProcessNode::lastKnownBlockHeightUpdated(uint32_t height)
{
    observerManager.notify(&INodeObserver::lastKnownBlockHeightUpdated, height - 1);
}

void InProcessNode::blockchainUpdated()
{
    std::unique_lock<std::mutex> lock(mutex);
    updateLastLocalBlockHeaderInfo();
    uint32_t blockIndex = lastLocalBlockHeaderInfo.uIndex;
    lock.unlock();
    observerManager.notify(&INodeObserver::localBlockchainUpdated, blockIndex);
}

void InProcessNode::poolUpdated()
{
    observerManager.notify(&INodeObserver::poolChanged);
}

void InProcessNode::updateLastLocalBlockHeaderInfo()
{
    uint32_t height;
    Crypto::FHash hash;
    FBlock block;
    uint64_t difficulty;

    try {
        core.get_blockchain_top(height, hash);
        core.getBlockByHash(hash, block);
        difficulty = core.getBlockDifficulty(height, difficulty);
    } catch (const std::exception &) {
        return;
    }

    lastLocalBlockHeaderInfo.uIndex = height;
    lastLocalBlockHeaderInfo.uMajorVersion = block.uMajorVersion;
    lastLocalBlockHeaderInfo.uMinorVersion = block.uMinorVersion;
    lastLocalBlockHeaderInfo.uTimestamp  = block.uTimestamp;
    lastLocalBlockHeaderInfo.sHash = hash;
    lastLocalBlockHeaderInfo.sPrevHash = block.sPreviousBlockHash;
    lastLocalBlockHeaderInfo.uNonce = block.uNonce;
    lastLocalBlockHeaderInfo.bIsAlternative = false;
    lastLocalBlockHeaderInfo.uDepth = 0;
    lastLocalBlockHeaderInfo.sDifficulty = difficulty;
    lastLocalBlockHeaderInfo.uReward = getBlockReward(block);
}

void InProcessNode::resetLastLocalBlockHeaderInfo()
{
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
}

void InProcessNode::blockchainSynchronized(uint32_t topHeight)
{
    observerManager.notify(&INodeObserver::blockchainSynchronized, topHeight);
}

void InProcessNode::queryBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                                uint64_t timestamp,
                                std::vector<FBlockShortEntry> &newBlocks,
                                uint32_t &startHeight,
                                const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::queryBlocksLiteAsync,
        this,
        std::move(knownBlockIds),
        timestamp,
        std::ref(newBlocks),
        std::ref(startHeight),
        callback
    ));
}

void InProcessNode::queryBlocksLiteAsync(std::vector<Crypto::FHash> &knownBlockIds,
                                         uint64_t timestamp,
                                         std::vector<FBlockShortEntry> &newBlocks,
                                         uint32_t &startHeight,
                                         const UCallback &callback)
{
    std::error_code ec = doQueryBlocksLite(std::move(knownBlockIds),
                                           timestamp,
                                           newBlocks,
                                           startHeight);
    callback(ec);
}

std::error_code InProcessNode::doQueryBlocksLite(std::vector<Crypto::FHash> &&knownBlockIds,
                                                 uint64_t timestamp,
                                                 std::vector<FBlockShortEntry> &newBlocks,
                                                 uint32_t &startHeight)
{
    uint32_t currentHeight, fullOffset;
    std::vector<QwertyNote::BlockShortInfo> entries;

    if (!core.queryBlocksLite(knownBlockIds, timestamp, startHeight, currentHeight, fullOffset, entries)) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    for (const auto &entry: entries) {
        FBlockShortEntry bse;
        bse.sBlockHash = entry.blockId;
        bse.bHasBlock = false;

        if (!entry.block.empty()) {
            bse.bHasBlock = true;
            if (!fromBinaryArray(bse.sBlock, asBinaryArray(entry.block))) {
                return std::make_error_code(std::errc::invalid_argument);
            }
        }

        for (const auto &tsi: entry.txPrefixes) {
            FTransactionShortInfo tpi;
            tpi.sTxId = tsi.txHash;
            tpi.sTxPrefix = tsi.txPrefix;

            bse.vTxsShortInfo.push_back(std::move(tpi));
        }

        newBlocks.push_back(std::move(bse));
    }

    return std::error_code{};

}

void InProcessNode::getPoolSymmetricDifference(
    std::vector<Crypto::FHash> &&knownPoolTxIds,
    Crypto::FHash knownBlockId,
    bool &isBcActual,
    std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
    std::vector<Crypto::FHash> &deletedTxIds,
    const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(
    [this, knownPoolTxIds, knownBlockId, &isBcActual, &newTxs, &deletedTxIds, callback] () mutable {
        this->getPoolSymmetricDifferenceAsync(std::move(knownPoolTxIds),
                                              knownBlockId,
                                              isBcActual,
                                              newTxs,
                                              deletedTxIds,
                                              callback);
    });
}

void InProcessNode::getPoolSymmetricDifferenceAsync(
    std::vector<Crypto::FHash> &&knownPoolTxIds,
    Crypto::FHash knownBlockId,
    bool &isBcActual,
    std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
    std::vector<Crypto::FHash> &deletedTxIds,
    const UCallback &callback)
{
    std::error_code ec = std::error_code();

    std::vector<TransactionPrefixInfo> added;
    isBcActual = core.getPoolChangesLite(knownBlockId, knownPoolTxIds, added, deletedTxIds);

    try {
        for (const auto& tx: added) {
            newTxs.push_back(createTransactionPrefix(tx.txPrefix, tx.txHash));
        }
    } catch (std::system_error& ex) {
        ec = ex.code();
    } catch (std::exception&) {
        ec = make_error_code(std::errc::invalid_argument);
    }

    callback(ec);
}

void InProcessNode::getMultisignatureOutputByGlobalIndex(uint64_t amount,
														 uint32_t gindex,
														 FMultiSignatureOutput &out,
														 const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post([this, amount, gindex, &out, callback]() mutable {
        this->getOutByMSigGIndexAsync(amount, gindex, out, callback);
    });
}

void InProcessNode::getOutByMSigGIndexAsync(uint64_t amount,
											uint32_t gindex,
											FMultiSignatureOutput &out,
											const UCallback &callback)
{
    std::error_code ec = std::error_code();
    bool result = core.getOutByMultiSigGlobalIndex(amount, gindex, out);
    if (!result) {
        ec = make_error_code(std::errc::invalid_argument);
        callback(ec);

        return;
    }

    callback(ec);
}

void InProcessNode::getBlocks(const std::vector<uint32_t> &blockHeights,
                              std::vector<std::vector<FBlockDetails>> &blocks,
                              const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        static_cast<void(InProcessNode::*)(
            const std::vector<uint32_t> &,
            std::vector<std::vector<FBlockDetails>> &,
            const UCallback &)>(&InProcessNode::getBlocksAsync),
        this,
        std::cref(blockHeights),
        std::ref(blocks),
        callback
    ));
}

void InProcessNode::getBlocksAsync(const std::vector<uint32_t> &blockHeights,
                                   std::vector<std::vector<FBlockDetails>> &blocks,
                                   const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        static_cast<
        std::error_code(InProcessNode::*)(
            const std::vector<uint32_t> &,
            std::vector<std::vector<FBlockDetails>> &)>(&InProcessNode::doGetBlocks),
        this,
        std::cref(blockHeights),
        std::ref(blocks)
    ));
    callback(ec);
}

std::error_code InProcessNode::doGetBlocks(const std::vector<uint32_t> &blockHeights,
                                           std::vector<std::vector<FBlockDetails>> &blocks)
{
    try {
        uint32_t topHeight = 0;
        Crypto::FHash topHash = boost::value_initialized<Crypto::FHash>();
        core.get_blockchain_top(topHeight, topHash);
        for (const uint32_t &height : blockHeights) {
            if (height > topHeight) {
                return make_error_code(QwertyNote::Error::REQUEST_ERROR);
            }
            Crypto::FHash hash = core.getBlockIdByHeight(height);
            FBlock block;
            if (!core.getBlockByHash(hash, block)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            FBlockDetails blockDetails;
            if (!blockchainExplorerDataBuilder.fillBlockDetails(block,
                                                                blockDetails,
                                                                false)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            std::vector<FBlockDetails> blocksOnSameHeight;
            blocksOnSameHeight.push_back(std::move(blockDetails));

            // getting orphans
            std::vector<FBlock> orphanBlocks;
            core.getOrphanBlocksByHeight(height, orphanBlocks);
            for (const FBlock &orphanBlock : orphanBlocks) {
                FBlockDetails orphanBlockDetails;
                if (!blockchainExplorerDataBuilder.fillBlockDetails(orphanBlock,
                                                                    orphanBlockDetails,
                                                                    false)) {
                    return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
                }
                blocksOnSameHeight.push_back(std::move(orphanBlockDetails));
            }
            blocks.push_back(std::move(blocksOnSameHeight));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

void InProcessNode::getBlocks(const std::vector<Crypto::FHash> &blockHashes,
                              std::vector<FBlockDetails> &blocks,
                              const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        static_cast<void(InProcessNode::*)(
            const std::vector<Crypto::FHash> &,
            std::vector<FBlockDetails> &,
            const UCallback &)>(&InProcessNode::getBlocksAsync),
        this,
        std::cref(blockHashes),
        std::ref(blocks),
        callback
    ));
}

void InProcessNode::getBlocksAsync(const std::vector<Crypto::FHash> &blockHashes,
                                   std::vector<FBlockDetails> &blocks,
                                   const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        static_cast<std::error_code(InProcessNode::*)(
            const std::vector<Crypto::FHash> &,
            std::vector<FBlockDetails> &)>(&InProcessNode::doGetBlocks),
        this,
        std::cref(blockHashes),
        std::ref(blocks)
    ));
    callback(ec);
}

std::error_code InProcessNode::doGetBlocks(const std::vector<Crypto::FHash> &blockHashes,
                                           std::vector<FBlockDetails> &blocks)
{
    try {
        for (const Crypto::FHash& hash : blockHashes) {
            FBlock block;
            if (!core.getBlockByHash(hash, block)) {
                return make_error_code(QwertyNote::Error::REQUEST_ERROR);
            }
            FBlockDetails blockDetails;
            if (!blockchainExplorerDataBuilder.fillBlockDetails(block,
                                                                blockDetails,
                                                                false)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            blocks.push_back(std::move(blockDetails));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }
    return std::error_code{};
}

void InProcessNode::getBlocks(uint64_t timestampBegin,
                              uint64_t timestampEnd,
                              uint32_t blocksNumberLimit,
                              std::vector<FBlockDetails> &blocks,
                              uint32_t &blocksNumberWithinTimestamps,
                              const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        static_cast<void(InProcessNode::*)(
            uint64_t,
            uint64_t,
            uint32_t,
            std::vector<FBlockDetails> &,
            uint32_t &,
            const UCallback&)>(&InProcessNode::getBlocksAsync),
        this,
        timestampBegin,
        timestampEnd,
        blocksNumberLimit,
        std::ref(blocks),
        std::ref(blocksNumberWithinTimestamps),
        callback
    ));
}

void InProcessNode::getBlocksAsync(uint64_t timestampBegin,
                                   uint64_t timestampEnd,
                                   uint32_t blocksNumberLimit,
                                   std::vector<FBlockDetails> &blocks,
                                   uint32_t &blocksNumberWithinTimestamps,
                                   const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        static_cast<std::error_code(InProcessNode::*)(
            uint64_t,
            uint64_t,
            uint32_t,
            std::vector<FBlockDetails> &,
            uint32_t &)>(&InProcessNode::doGetBlocks),
        this,
        timestampBegin,
        timestampEnd,
        blocksNumberLimit,
        std::ref(blocks),
        std::ref(blocksNumberWithinTimestamps)
    ));

    callback(ec);
}

std::error_code InProcessNode::doGetBlocks(uint64_t timestampBegin,
                                           uint64_t timestampEnd,
                                           uint32_t blocksNumberLimit,
                                           std::vector<FBlockDetails> &blocks,
                                           uint32_t &blocksNumberWithinTimestamps)
{
    try {
        std::vector<FBlock> rawBlocks;
        if (!core.getBlocksByTimestamp(timestampBegin,
                                       timestampEnd,
                                       blocksNumberLimit,
                                       rawBlocks,
                                       blocksNumberWithinTimestamps)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }
        for (const FBlock &rawBlock : rawBlocks) {
            FBlockDetails block;
            if (!blockchainExplorerDataBuilder.fillBlockDetails(rawBlock,
                                                                block,
                                                                false)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            blocks.push_back(std::move(block));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

void InProcessNode::getTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                                    std::vector<FTransactionDetails> &transactions,
                                    const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        static_cast<void(InProcessNode::*)(
            const std::vector<Crypto::FHash> &,
            std::vector<FTransactionDetails> &,
            const UCallback &)>(&InProcessNode::getTransactionsAsync),
        this,
        std::cref(transactionHashes),
        std::ref(transactions),
        callback
    ));
}

void InProcessNode::getTransactionsAsync(const std::vector<Crypto::FHash> &transactionHashes,
                                         std::vector<FTransactionDetails> &transactions,
                                         const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        static_cast<std::error_code(InProcessNode::*)(
            const std::vector<Crypto::FHash> &,
            std::vector<FTransactionDetails> &)>(&InProcessNode::doGetTransactions),
        this,
        std::cref(transactionHashes),
        std::ref(transactions)
    ));

    callback(ec);
}

std::error_code InProcessNode::doGetTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                                                 std::vector<FTransactionDetails> &transactions)
{
    try {
        std::list<FTransaction> txs;
        std::list<Crypto::FHash> missed_txs;
        core.getTransactions(transactionHashes, txs, missed_txs, true);
        if (!missed_txs.empty()) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }
        for (const FTransaction &tx : txs) {
            FTransactionDetails transactionDetails;
            if (!blockchainExplorerDataBuilder.fillTransactionDetails(tx, transactionDetails)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            transactions.push_back(std::move(transactionDetails));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

void InProcessNode::getPoolTransactions(uint64_t timestampBegin,
                                        uint64_t timestampEnd,
                                        uint32_t transactionsNumberLimit,
                                        std::vector<FTransactionDetails> &transactions,
                                        uint64_t &transactionsNumberWithinTimestamps,
                                        const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::getPoolTransactionsAsync,
        this,
        timestampBegin,
        timestampEnd,
        transactionsNumberLimit,
        std::ref(transactions),
        std::ref(transactionsNumberWithinTimestamps),
        callback
    ));
}

void InProcessNode::getPoolTransactionsAsync(uint64_t timestampBegin,
                                             uint64_t timestampEnd,
                                             uint32_t transactionsNumberLimit,
                                             std::vector<FTransactionDetails> &transactions,
                                             uint64_t &transactionsNumberWithinTimestamps,
                                             const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        &InProcessNode::doGetPoolTransactions,
        this,
        timestampBegin,
        timestampEnd,
        transactionsNumberLimit,
        std::ref(transactions),
        std::ref(transactionsNumberWithinTimestamps)
    ));

    callback(ec);
}

std::error_code InProcessNode::doGetPoolTransactions(
    uint64_t timestampBegin,
    uint64_t timestampEnd,
    uint32_t transactionsNumberLimit,
    std::vector<FTransactionDetails> &transactions,
    uint64_t &transactionsNumberWithinTimestamps)
{
    try {
        std::vector<FTransaction> rawTransactions;
        if (!core.getPoolTransactionsByTimestamp(timestampBegin,
                                                 timestampEnd,
                                                 transactionsNumberLimit,
                                                 rawTransactions,
                                                 transactionsNumberWithinTimestamps)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }
        for (const FTransaction& rawTransaction : rawTransactions) {
            FTransactionDetails transactionDetails;
            if (!blockchainExplorerDataBuilder.fillTransactionDetails(rawTransaction,
                                                                      transactionDetails)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            transactions.push_back(std::move(transactionDetails));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }
    return std::error_code{};
}

void InProcessNode::getTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                               std::vector<FTransactionDetails> &transactions,
                                               const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::getTransactionsByPaymentIdAsync,
        this,
        std::cref(paymentId),
        std::ref(transactions),
        callback
    ));
}

void InProcessNode::getTransactionsByPaymentIdAsync(
    const Crypto::FHash &paymentId,
    std::vector<FTransactionDetails> &transactions,
    const UCallback &callback)
{
    std::error_code ec = core.executeLocked(std::bind(
        &InProcessNode::doGetTransactionsByPaymentId,
        this,
        paymentId,
        std::ref(transactions)
    ));

    callback(ec);
}

std::error_code InProcessNode::doGetTransactionsByPaymentId(
    const Crypto::FHash &paymentId,
    std::vector<FTransactionDetails> &transactions)
{
    try {
        std::vector<FTransaction> rawTransactions;
        if (!core.getTransactionsByPaymentId(paymentId, rawTransactions)) {
            return make_error_code(QwertyNote::Error::REQUEST_ERROR);
        }
        for (const FTransaction &rawTransaction : rawTransactions) {
            FTransactionDetails transactionDetails;
            if (!blockchainExplorerDataBuilder.fillTransactionDetails(rawTransaction,
                                                                      transactionDetails)) {
                return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
            }
            transactions.push_back(std::move(transactionDetails));
        }
    } catch (std::system_error &e) {
        return e.code();
    } catch (std::exception &) {
        return make_error_code(QwertyNote::Error::INTERNAL_NODE_ERROR);
    }

    return std::error_code{};
}

void InProcessNode::isSynchronized(bool &syncStatus, const UCallback &callback)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (state != INITIALIZED) {
        lock.unlock();
        callback(make_error_code(QwertyNote::Error::NOT_INITIALIZED));

        return;
    }

    ioService.post(std::bind(
        &InProcessNode::isSynchronizedAsync,
        this,
        std::ref(syncStatus),
        callback
    ));
}

void InProcessNode::isSynchronizedAsync(bool &syncStatus, const UCallback &callback)
{
    syncStatus = protocol.isSynchronized();
    callback(std::error_code());
}

} // namespace QwertyNote
