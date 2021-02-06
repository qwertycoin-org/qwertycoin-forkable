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
#include <functional>
#include <system_error>
#include <vector>
#include <BlockchainExplorer/BlockchainExplorerData.h>
#include <Crypto/Crypto.h>
#include <CryptoNoteCore/CryptoNoteBasic.h>
#include <CryptoNoteCore/ITransaction.h>
#include <CryptoNoteProtocol/CryptoNoteProtocolDefinitions.h>
#include <Rpc/CoreRpcServerCommandsDefinitions.h>

namespace QwertyNote {

class INodeObserver
{
public:
    virtual ~INodeObserver() = default;

    virtual void peerCountUpdated(size_t count) { }
    virtual void localBlockchainUpdated(uint32_t height) { }
    virtual void lastKnownBlockHeightUpdated(uint32_t height) { }
    virtual void poolChanged() { }
    virtual void blockchainSynchronized(uint32_t topHeight) { }
};

struct OutEntry {
    uint32_t outGlobalIndex;
    Crypto::FPublicKey outKey;
};

struct OutsForAmount {
    uint64_t amount;
    std::vector<OutEntry> outs;
};

struct TransactionShortInfo {
    Crypto::FHash txId;
    TransactionPrefix txPrefix;
};

struct BlockShortEntry {
    Crypto::FHash blockHash;
    bool hasBlock;
    QwertyNote::Block block;
    std::vector<TransactionShortInfo> txsShortInfo;
};

struct BlockHeaderInfo {
    uint32_t index;
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint64_t timestamp;
    Crypto::FHash hash;
    Crypto::FHash prevHash;
    uint32_t nonce;
    bool isAlternative;
    uint32_t depth; // last block index = current block index + depth
    difficulty_type difficulty;
    uint64_t reward;
};

class INode
{
public:
    typedef std::function<void(std::error_code)> Callback;

    virtual ~INode() = default;

    virtual bool addObserver(INodeObserver *observer) = 0;
    virtual bool removeObserver(INodeObserver *observer) = 0;

    virtual void init(const Callback &callback) = 0;
    virtual bool shutdown() = 0;

    virtual size_t getPeerCount() const = 0;
    virtual uint32_t getLastLocalBlockHeight() const = 0;
    virtual uint32_t getLastKnownBlockHeight() const = 0;
    virtual uint32_t getLocalBlockCount() const = 0;
    virtual uint32_t getKnownBlockCount() const = 0;
    virtual uint64_t getMinimalFee() const = 0;
    virtual uint64_t getLastLocalBlockTimestamp() const = 0;
    virtual uint32_t getNodeHeight() const = 0;
    virtual BlockHeaderInfo getLastLocalBlockHeaderInfo() const = 0;
    virtual uint32_t getGRBHeight() const = 0;

    virtual void relayTransaction(const Transaction &transaction, const Callback &callback) = 0;
    virtual void getRandomOutsByAmounts(
            std::vector<uint64_t> &&amounts, uint64_t outsCount,
            std::vector<QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>
                    &result,
            const Callback &callback) = 0;
    virtual void getNewBlocks(std::vector<Crypto::FHash> &&knownBlockIds,
                              std::vector<QwertyNote::BlockCompleteEntry> &newBlocks,
                              uint32_t &startHeight, const Callback &callback) = 0;
    virtual void getTransactionOutsGlobalIndices(const Crypto::FHash &transactionHash,
                                                 std::vector<uint32_t> &outsGlobalIndices,
                                                 const Callback &callback) = 0;
    virtual void queryBlocks(std::vector<Crypto::FHash> &&knownBlockIds, uint64_t timestamp,
                             std::vector<BlockShortEntry> &newBlocks, uint32_t &startHeight,
                             const Callback &callback) = 0;
    virtual void getPoolSymmetricDifference(
			std::vector<Crypto::FHash> &&knownPoolTxIds, Crypto::FHash knownBlockId, bool &isBcActual,
            std::vector<std::unique_ptr<ITransactionReader>> &newTxs,
			std::vector<Crypto::FHash> &deletedTxIds, const Callback &callback) = 0;
    virtual void getMultisignatureOutputByGlobalIndex(uint64_t amount, uint32_t gindex,
                                                      MultiSignatureOutput &out,
                                                      const Callback &callback) = 0;

    virtual void getBlocks(const std::vector<uint32_t> &blockHeights,
                           std::vector<std::vector<BlockDetails>> &blocks,
                           const Callback &callback) = 0;
    virtual void getBlocks(const std::vector<Crypto::FHash> &blockHashes,
                           std::vector<BlockDetails> &blocks, const Callback &callback) = 0;
    virtual void getBlocks(uint64_t timestampBegin, uint64_t timestampEnd,
                           uint32_t blocksNumberLimit, std::vector<BlockDetails> &blocks,
                           uint32_t &blocksNumberWithinTimestamps, const Callback &callback) = 0;
    virtual void getTransactions(const std::vector<Crypto::FHash> &transactionHashes,
                                 std::vector<TransactionDetails> &transactions,
                                 const Callback &callback) = 0;
    virtual void getTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                            std::vector<TransactionDetails> &transactions,
                                            const Callback &callback) = 0;
    virtual void getPoolTransactions(uint64_t timestampBegin, uint64_t timestampEnd,
                                     uint32_t transactionsNumberLimit,
                                     std::vector<TransactionDetails> &transactions,
                                     uint64_t &transactionsNumberWithinTimestamps,
                                     const Callback &callback) = 0;
    virtual void isSynchronized(bool &syncStatus, const Callback &callback) = 0;
};

} // namespace QwertyNote
