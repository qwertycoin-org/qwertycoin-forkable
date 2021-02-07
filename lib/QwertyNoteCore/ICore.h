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
#include <list>
#include <system_error>
#include <utility>
#include <vector>

#include <QwertyNote.h>

#include <QwertyNoteCore/BlockchainMessages.h>
#include <QwertyNoteCore/Difficulty.h>
#include <QwertyNoteCore/MessageQueue.h>

namespace QwertyNote {

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request;
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response;
struct NOTIFY_RESPONSE_GET_OBJECTS_request;
struct NOTIFY_REQUEST_GET_OBJECTS_request;

class Currency;
class IBlock;
class ICoreObserver;
struct FBlock;
struct block_verification_context;
struct BlockFullInfo;
struct BlockShortInfo;
struct core_stat_info;
struct i_cryptonote_protocol;
struct FTransaction;
struct FMultiSignatureInput;
struct FKeyInput;
struct TransactionPrefixInfo;
struct tx_verification_context;

class ICore
{
public:
    virtual ~ICore() = default;

    virtual bool addObserver(ICoreObserver *observer) = 0;
    virtual bool removeObserver(ICoreObserver *observer) = 0;

    virtual bool have_block(const Crypto::FHash &id) = 0;
    virtual std::vector<Crypto::FHash> buildSparseChain() = 0;
    virtual std::vector<Crypto::FHash> buildSparseChain(const Crypto::FHash &startBlockId) = 0;
    virtual bool get_stat_info(QwertyNote::core_stat_info &st_inf) = 0;
    virtual bool on_idle() = 0;
    virtual void pause_mining() = 0;
    virtual void update_block_template_and_resume_mining() = 0;
    virtual bool handle_incoming_block_blob(
        const QwertyNote::BinaryArray &block_blob,
                                            QwertyNote::block_verification_context &bvc,
        bool control_miner,
        bool relay_block) = 0;
    virtual bool handle_get_objects( // TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
        NOTIFY_REQUEST_GET_OBJECTS_request &arg,
        NOTIFY_RESPONSE_GET_OBJECTS_request &rsp) = 0;
    virtual void on_synchronized() = 0;
    virtual size_t addChain(const std::vector<const IBlock*>& chain) = 0;

    virtual void get_blockchain_top(uint32_t &height, Crypto::FHash &top_id) = 0;
    virtual std::vector<Crypto::FHash> findBlockchainSupplement(
        const std::vector<Crypto::FHash> &remoteBlockIds,
        size_t maxCount,
        uint32_t &totalBlockCount,
        uint32_t &startBlockIndex) = 0;
    virtual bool get_random_outs_for_amounts(
        const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request &req,
        COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response &res) = 0;
    virtual bool getTxOutputsGlobalIndexes(
        const Crypto::FHash &tx_id,
        std::vector<uint32_t> &indexs) = 0;
    virtual bool getOutByMultiSigGlobalIndex(uint64_t amount, uint64_t gindex,
											 FMultiSignatureOutput &out) = 0;
    virtual i_cryptonote_protocol *get_protocol() = 0;
    virtual bool handle_incoming_tx( // TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
        const BinaryArray &tx_blob,
        tx_verification_context &tvc,
        bool keeped_by_block,
        bool loose_check) = 0;
    virtual std::vector<FTransaction> getPoolTransactions() = 0;

    virtual bool getPoolChanges(
        const Crypto::FHash &tailBlockId,
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<FTransaction> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) = 0;
    virtual bool getPoolChangesLite(
        const Crypto::FHash &tailBlockId,
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<TransactionPrefixInfo> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) = 0;
    virtual void getPoolChanges(
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<FTransaction> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) = 0;

    virtual bool queryBlocks(
        const std::vector<Crypto::FHash> &block_ids,
        uint64_t timestamp,
        uint32_t &start_height,
        uint32_t &current_height,
        uint32_t &full_offset,
        std::vector<BlockFullInfo> &entries) = 0;

    virtual bool queryBlocksLite(
        const std::vector<Crypto::FHash> &block_ids,
        uint64_t timestamp,
        uint32_t &start_height,
        uint32_t &current_height,
        uint32_t &full_offset,
        std::vector<BlockShortInfo> &entries) = 0;

    virtual bool queryBlocksDetailed(
        const std::vector<Crypto::FHash> &knownBlockHashes,
        uint64_t timestamp,
        uint32_t &startIndex,
        uint32_t &currentIndex,
        uint32_t &fullOffset,
        std::vector<BlockFullInfo> &entries) = 0;

    virtual Crypto::FHash getBlockIdByHeight(uint32_t height) = 0;
    virtual bool getBlockByHash(const Crypto::FHash &h, FBlock &blk) = 0;
    virtual bool getBlockHeight(const Crypto::FHash &blockId, uint32_t &blockHeight) = 0;
    virtual void getTransactions(
        const std::vector<Crypto::FHash> &txs_ids,
        std::list<FTransaction> &txs,
        std::list<Crypto::FHash> &missed_txs,
        bool checkTxPool = false) = 0;
    virtual bool getTransactionsWithOutputGlobalIndexes(const std::vector<Crypto::FHash> &txsIds,
							std::list<Crypto::FHash> &missedTxs,
							std::vector<std::pair<FTransaction,
							std::vector<uint32_t>>> &txs) = 0;
    virtual bool getBackwardBlocksSizes(
        uint32_t fromHeight,
        std::vector<size_t> &sizes,
        size_t count) = 0;
    virtual bool getBlockSize(const Crypto::FHash &hash, size_t &size) = 0;
    virtual bool getAlreadyGeneratedCoins(const Crypto::FHash &hash, uint64_t &generatedCoins) = 0;
    virtual bool getBlockReward(
        uint8_t blockMajorVersion,
        size_t medianSize,
        size_t currentBlockSize,
        uint64_t alreadyGeneratedCoins,
        uint64_t fee,
        uint64_t &reward,
        int64_t &emissionChange,
        uint32_t height,
        uint64_t blockTarget) = 0;
    virtual bool scanOutputkeysForIndices(
        const FKeyInput &txInToKey,
        std::list<std::pair<Crypto::FHash, size_t>> &outputReferences) = 0;
    virtual bool getBlockDifficulty(uint32_t height, difficulty_type& difficulty) = 0;
    virtual bool getBlockCumulativeDifficulty(uint32_t height, difficulty_type &difficulty) = 0;
    virtual bool getBlockContainingTx(
        const Crypto::FHash &txId,
        Crypto::FHash &blockId,
        uint32_t &blockHeight) = 0;
    virtual bool getMultisigOutputReference(
        const FMultiSignatureInput &txInMultisig,
        std::pair<Crypto::FHash, size_t> &outputReference) = 0;

    virtual bool getGeneratedTransactionsNumber(
        uint32_t height,
        uint64_t &generatedTransactions) = 0;
    virtual bool getOrphanBlocksByHeight(uint32_t height, std::vector<FBlock> &blocks) = 0;
    virtual bool getBlocksByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t blocksNumberLimit,
        std::vector<FBlock> &blocks,
        uint32_t &blocksNumberWithinTimestamps) = 0;
    virtual bool getPoolTransactionsByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t transactionsNumberLimit,
        std::vector<FTransaction> &transactions,
        uint64_t &transactionsNumberWithinTimestamps) = 0;
    virtual bool getTransactionsByPaymentId(
        const Crypto::FHash &paymentId,
        std::vector<FTransaction> &transactions) = 0;
    virtual std::vector<Crypto::FHash> getTransactionHashesByPaymentId(
        const Crypto::FHash &paymentId) = 0;
    virtual uint64_t getMinimalFeeForHeight(uint32_t height) = 0;
    virtual uint64_t getMinimalFee() = 0;
    virtual uint64_t getBlockTimestamp(uint32_t height) = 0;

    virtual uint32_t getCurrentBlockchainHeight() = 0;
    virtual uint8_t getBlockMajorVersionForHeight(uint32_t height) = 0;
    virtual uint8_t getCurrentBlockMajorVersion() = 0;

    virtual bool getBlockEntry(uint32_t height,
                               uint64_t &blockCumulativeSize,
                               difficulty_type &difficulty,
                               uint64_t &alreadyGeneratedCoins,
                               uint64_t &reward,
                               uint64_t &transactionsCount,
                               uint64_t &timestamp) = 0;
    virtual std::unique_ptr<IBlock> getBlock(const Crypto::FHash &blocksId) = 0;
    virtual bool handleIncomingTransaction(
        const FTransaction &tx,
        const Crypto::FHash &txHash,
        size_t blobSize,
        tx_verification_context &tvc,
        bool keptByBlock,
        uint32_t height,
        bool loose_check) = 0;
    virtual std::error_code executeLocked(const std::function<std::error_code()> &func) = 0;

    virtual bool addMessageQueue(MessageQueue<BlockchainMessage> &messageQueue) = 0;
    virtual bool removeMessageQueue(MessageQueue<BlockchainMessage> &messageQueue) = 0;

    virtual void rollbackBlockchain(const uint32_t height) = 0;
};

} // namespace QwertyNote
