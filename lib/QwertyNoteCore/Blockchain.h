// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2016, The Karbowanec developers
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

#include <google/sparse_hash_set>
#include <google/sparse_hash_map>

#include <Common/ObserverManager.h>
#include <Common/Util.h>

#include <Logging/LoggerRef.h>

#include <QwertyNoteCore/BlockchainIndices.h>
#include <QwertyNoteCore/BlockchainMessages.h>
#include <QwertyNoteCore/BlockIndex.h>
#include <QwertyNoteCore/Checkpoints.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/Currency.h>
#include <QwertyNoteCore/IBlockchainStorageObserver.h>
#include <QwertyNoteCore/IMinerHandler.h>
#include <QwertyNoteCore/IntrusiveLinkedList.h>
#include <QwertyNoteCore/ITransactionValidator.h>
#include <QwertyNoteCore/MessageQueue.h>
#include <QwertyNoteCore/SwappedVector.h>
#include <QwertyNoteCore/TransactionPool.h>
#include <QwertyNoteCore/UpgradeDetector.h>

#undef ERROR

namespace QwertyNote {

using QwertyNote::BlockInfo;

struct NOTIFY_REQUEST_GET_OBJECTS_request;
struct NOTIFY_RESPONSE_GET_OBJECTS_request;
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request;
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response;
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount;

class Blockchain : public QwertyNote::ITransactionValidator
{
public:
    Blockchain(
        const Currency &currency,
        tx_memory_pool &tx_pool,
        Logging::ILogger &logger,
        bool blockchainIndexesEnabled
    );

    bool addObserver(IBlockchainStorageObserver *observer);
    bool removeObserver(IBlockchainStorageObserver *observer);

    // ITransactionValidator
    bool checkTransactionInputs(const FTransaction &tx, BlockInfo &maxUsedBlock) override;
    bool checkTransactionInputs(const FTransaction &tx, BlockInfo &maxUsedBlock, BlockInfo &lastFailed) override;
    bool haveSpentKeyImages(const FTransaction &tx) override;
    bool checkTransactionSize(size_t blobSize) override;

    bool init() { return init(Tools::getDefaultDataDirectory(), true); }
    bool init(const std::string &config_folder, bool load_existing);
    bool deinit();

    bool getLowerBound(uint64_t timestamp, uint64_t startOffset, uint32_t &height);
    std::vector<Crypto::FHash> getBlockIds(uint32_t startHeight, uint32_t maxCount);

    void setCheckpoints(Checkpoints &&chk_pts) { m_checkpoints = chk_pts; }
    bool getBlocks(uint32_t start_offset, uint32_t count, std::list<FBlock> &blocks, std::list<FTransaction> &txs);
    bool getBlocks(uint32_t start_offset, uint32_t count, std::list<FBlock> &blocks);
    bool getTransactionsWithOutputGlobalIndexes(const std::vector<Crypto::FHash> &txsIds,
							  					std::list<Crypto::FHash> &missedTxs,
							  					std::vector<std::pair<FTransaction,
							  										  std::vector<uint32_t>>> &txs);
    bool getAlternativeBlocks(std::list<FBlock> &blocks);
    uint32_t getAlternativeBlocksCount();
    Crypto::FHash getBlockIdByHeight(uint32_t height);
    bool getBlockByHash(const Crypto::FHash &h, FBlock &blk);
    bool getBlockHeight(const Crypto::FHash &blockId, uint32_t &blockHeight);

    bool haveTransaction(const Crypto::FHash &id);
    bool haveTransactionKeyImagesAsSpent(const FTransaction &tx);

    uint32_t getCurrentBlockchainHeight(); // TODO: rename to getCurrentBlockchainSize
    Crypto::FHash getTailId();
    Crypto::FHash getTailId(uint32_t &height);
    difficulty_type getDifficultyForNextBlock(uint64_t nextBlockTime);
    bool getDifficultyStat(uint32_t height, IMinerHandler::stat_period period,
                           uint32_t& block_num, uint64_t& avg_solve_time,
                           uint64_t& stddev_solve_time, uint32_t& outliers_num,
                           difficulty_type &avg_diff, difficulty_type &min_diff,
                           difficulty_type &max_diff);
    uint64_t getBlockTimestamp(uint32_t height);
    uint64_t getMinimalFee(uint32_t height);
    uint64_t getCoinsInCirculation();
    uint8_t getBlockMajorVersionForHeight(uint32_t height) const;
    bool addNewBlock(const FBlock &bl, block_verification_context &bvc);
    bool resetAndSetGenesisBlock(const FBlock &b);
    bool haveBlock(const Crypto::FHash &id);
    size_t getTotalTransactions();
    std::vector<Crypto::FHash> buildSparseChain();
    std::vector<Crypto::FHash> buildSparseChain(const Crypto::FHash &startBlockId);
    uint32_t findBlockchainSupplement(const std::vector<Crypto::FHash> &qblock_ids); // !!!
    std::vector<Crypto::FHash> findBlockchainSupplement(
        const std::vector<Crypto::FHash> &remoteBlockIds,
        size_t maxCount,
        uint32_t &totalBlockCount,
        uint32_t &startBlockIndex);
    bool handleGetObjects(
        NOTIFY_REQUEST_GET_OBJECTS_request &arg,
        NOTIFY_RESPONSE_GET_OBJECTS_request &rsp); // TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
    bool getRandomOutsByAmount(
        const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request &req,
        COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response &res);
    bool getBackwardBlocksSize(size_t from_height, std::vector<size_t> &sz, size_t count);
    bool getTransactionOutputGlobalIndexes(const Crypto::FHash &tx_id, std::vector<uint32_t> &indexs);
    bool getOutByMultiSigGlobalIndex(uint64_t amount, uint64_t gindex, FMultiSignatureOutput &out);
    bool checkTransactionInputs(
        const FTransaction &tx,
        uint32_t &pmax_used_block_height,
        Crypto::FHash &max_used_block_id,
        BlockInfo *tail = 0);
    uint64_t getCurrentCumulativeBlocksizeLimit();
    uint64_t blockDifficulty(size_t i);
    uint64_t blockCumulativeDifficulty(size_t i);
    bool getBlockEntry(size_t i,
                       uint64_t &blockCumulativeSize,
                       difficulty_type &difficulty,
                       uint64_t &alreadyGeneratedCoins,
                       uint64_t &reward,
                       uint64_t &transactionsCount,
                       uint64_t &timestamp);
    bool getBlockContainingTransaction(
        const Crypto::FHash &txId,
        Crypto::FHash &blockId,
        uint32_t &blockHeight);
    bool getAlreadyGeneratedCoins(const Crypto::FHash &hash, uint64_t &generatedCoins);
    bool getBlockSize(const Crypto::FHash &hash, size_t &size);
    bool getMultisigOutputReference(
        const FMultiSignatureInput &txInMultisig,
        std::pair<Crypto::FHash, size_t> &outputReference);
    bool getGeneratedTransactionsNumber(uint32_t height, uint64_t &generatedTransactions);
    bool getOrphanBlockIdsByHeight(uint32_t height, std::vector<Crypto::FHash> &blockHashes);
    bool getBlockIdsByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t blocksNumberLimit,
        std::vector<Crypto::FHash> &hashes,
        uint32_t &blocksNumberWithinTimestamps);
    bool getTransactionIdsByPaymentId(
        const Crypto::FHash &paymentId,
        std::vector<Crypto::FHash> &transactionHashes);
    bool isBlockInMainChain(const Crypto::FHash &blockId);
    bool isInCheckpointZone(const uint32_t height);
    uint64_t getAvgDifficultyForHeight(uint32_t height, size_t window);

    template<class visitor_t>
    bool scanOutputKeysForIndexes(
        const FKeyInput &tx_in_to_key,
        visitor_t &vis,
        uint32_t *pmax_related_block_height = nullptr);

    bool addMessageQueue(MessageQueue<BlockchainMessage> &messageQueue);
    bool removeMessageQueue(MessageQueue<BlockchainMessage> &messageQueue);

    template<class T, class D, class S>
    bool getBlocks(const T &block_ids, D &blocks, S &missed_bs)
    {
        std::lock_guard<std::recursive_mutex> lk(m_blockchain_lock);

        for (const auto &bl_id : block_ids) {
            try {
                uint32_t height = 0;
                if (!m_blockIndex.getBlockHeight(bl_id, height)) {
                    missed_bs.push_back(bl_id);
                } else {
                    if (height >= m_blocks.size()) {
                        logger(Logging::ERROR, Logging::BRIGHT_RED)
                            << "Internal error: bl_id=" << Common::podToHex(bl_id)
                            << " have index record with offset=" << height
                            << ", bigger then m_blocks.size()=" << m_blocks.size();
                        return false;
                    }
                    blocks.push_back(m_blocks[height].bl);
                }
            } catch (const std::exception &e) {
                return false;
            }
        }

        return true;
    }

    template<class T, class D, class S>
    void getBlockchainTransactions(const T &txs_ids, D &txs, S &missed_txs)
    {
        std::lock_guard<decltype(m_blockchain_lock)> bcLock(m_blockchain_lock);

        for (const auto &tx_id : txs_ids) {
            auto it = m_transactionMap.find(tx_id);
            if (it == m_transactionMap.end()) {
                missed_txs.push_back(tx_id);
            } else {
                txs.push_back(transactionByIndex(it->second).tx);
            }
        }
    }

    template<class T, class D, class S>
    void getTransactions(const T &txs_ids, D &txs, S &missed_txs, bool checkTxPool = false)
    {
        if (checkTxPool) {
            std::lock_guard<decltype(m_tx_pool)> txLock(m_tx_pool);

            getBlockchainTransactions(txs_ids, txs, missed_txs);

            auto poolTxIds = std::move(missed_txs);
            missed_txs.clear();
            m_tx_pool.getTransactions(poolTxIds, txs, missed_txs);
        } else {
            getBlockchainTransactions(txs_ids, txs, missed_txs);
        }
    }

    // debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string &file);

    struct TransactionIndex
    {
        void serialize(ISerializer &s)
        {
            s(block, "block");
            s(transaction, "tx");
        }

        uint32_t block;
        uint16_t transaction;
    };

    void rollbackBlockchainTo(uint32_t height);
    bool have_tx_keyimg_as_spent(const Crypto::FKeyImage &key_im);

private:
    struct MultisignatureOutputUsage
    {
        void serialize(ISerializer &s)
        {
            s(transactionIndex, "txindex");
            s(outputIndex, "outindex");
            s(isUsed, "used");
        }

        TransactionIndex transactionIndex;
        uint16_t outputIndex;
        bool isUsed;
    };

    struct TransactionEntry
    {
        void serialize(ISerializer &s)
        {
            s(tx, "tx");
            s(m_global_output_indexes, "indexes");
        }

        FTransaction tx;
        std::vector<uint32_t> m_global_output_indexes;
    };

    struct BlockEntry
    {
        void serialize(ISerializer &s)
        {
            s(bl, "block");
            s(height, "height");
            s(block_cumulative_size, "block_cumulative_size");
            s(cumulative_difficulty, "cumulative_difficulty");
            s(already_generated_coins, "already_generated_coins");
            s(transactions, "transactions");
        }

        FBlock bl;
        uint32_t height;
        uint64_t block_cumulative_size;
        difficulty_type cumulative_difficulty;
        uint64_t already_generated_coins;
        std::vector<TransactionEntry> transactions;
    };

    typedef google::sparse_hash_set<Crypto::FKeyImage> key_images_container;
    typedef std::unordered_map<Crypto::FHash, BlockEntry> blocks_ext_by_hash;
    // Crypto::FHash - tx FHash, size_t - index of out in transaction
    typedef google::sparse_hash_map<uint64_t, std::vector<std::pair<TransactionIndex, uint16_t>>> outputs_container;
    typedef google::sparse_hash_map<uint64_t, std::vector<MultisignatureOutputUsage>> MultisignatureOutputsContainer;

    const Currency &m_currency;
    tx_memory_pool &m_tx_pool;
    std::recursive_mutex m_blockchain_lock; // TODO: add here reader/writer lock
    Crypto::CnContext m_cn_context;
    Tools::ObserverManager<IBlockchainStorageObserver> m_observerManager;

    key_images_container m_spent_keys;
    size_t m_current_block_cumul_sz_limit;
    blocks_ext_by_hash m_alternative_chains; // Crypto::FHash -> block_extended_info
    outputs_container m_outputs;

    std::string m_config_folder;
    Checkpoints m_checkpoints;

    typedef SwappedVector<BlockEntry> Blocks;
    typedef std::unordered_map<Crypto::FHash, uint32_t> BlockMap;
    typedef std::unordered_map<Crypto::FHash, TransactionIndex> TransactionMap;
    typedef BasicUpgradeDetector<Blocks> UpgradeDetector;

    friend class BlockCacheSerializer;
    friend class BlockchainIndicesSerializer;

    Blocks m_blocks;
    QwertyNote::BlockIndex m_blockIndex;
    TransactionMap m_transactionMap;
    MultisignatureOutputsContainer m_multisignatureOutputs;
    UpgradeDetector m_upgradeDetectorV2;
    UpgradeDetector m_upgradeDetectorV3;
    UpgradeDetector m_upgradeDetectorV4;
    UpgradeDetector m_upgradeDetectorV5;
    UpgradeDetector m_upgradeDetectorV6;

    PaymentIdIndex m_paymentIdIndex;
    TimestampBlocksIndex m_timestampIndex;
    GeneratedTransactionsIndex m_generatedTransactionsIndex;
    OrphanBlocksIndex m_orphanBlocksIndex;
    bool m_blockchainIndexesEnabled;

    IntrusiveLinkedList<MessageQueue<BlockchainMessage>> m_messageQueueList;

    Logging::LoggerRef logger;

    void rebuildCache();
    bool storeCache();
    bool switch_to_alternative_blockchain(
        std::list<blocks_ext_by_hash::iterator> &alt_chain,
        bool discard_disconnected_chain);
    bool handle_alternative_block(
        const FBlock &b,
        const Crypto::FHash &id,
        block_verification_context &bvc,
        bool sendNewAlternativeBlockMessage = true);
    difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator> &alt_chain,
        BlockEntry &bei, uint64_t nextBlockTime);
    bool prevalidate_miner_transaction(const FBlock &b, uint32_t height);
    bool validate_miner_transaction(
        const FBlock &b,
        uint32_t height,
        size_t cumulativeBlockSize,
        uint64_t alreadyGeneratedCoins,
        uint64_t fee,
        uint64_t &reward,
        int64_t &emissionChange);
    bool rollback_blockchain_switching(std::list<FBlock> &original_chain, size_t rollback_height);
    bool get_last_n_blocks_sizes(std::vector<size_t> &sz, size_t count);
    bool add_out_to_get_random_outs(
        std::vector<std::pair<TransactionIndex, uint16_t>> &amount_outs,
        COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount &result_outs,
        uint64_t amount,
        size_t i);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time);
    size_t find_end_of_allowed_index(const std::vector<std::pair<TransactionIndex, uint16_t>> &amount_outs);
    bool check_block_timestamp_main(const FBlock &b);
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const FBlock &b);
    uint64_t get_adjusted_time();
    bool complete_timestamps_vector(
        uint8_t blockMajorVersion,
        uint64_t start_height,
        std::vector<uint64_t> &timestamps);
    bool checkBlockVersion(const FBlock &b, const Crypto::FHash &blockHash);
    bool checkCumulativeBlockSize(
        const Crypto::FHash &blockId,
        size_t cumulativeBlockSize,
        uint64_t height);
    std::vector<Crypto::FHash> doBuildSparseChain(const Crypto::FHash &startBlockId) const;
    bool getBlockCumulativeSize(const FBlock &block, size_t &cumulativeSize);
    bool update_next_cumulative_size_limit();
    bool check_tx_input(
        const FKeyInput &txin,
        const Crypto::FHash &tx_prefix_hash,
        const std::vector<Crypto::FSignature> &sig,
        uint32_t *pmax_related_block_height = nullptr);
    bool checkTransactionInputs(
        const FTransaction &tx,
        const Crypto::FHash &tx_prefix_hash,
        uint32_t *pmax_used_block_height = nullptr);
    bool checkTransactionInputs(const FTransaction &tx, uint32_t *pmax_used_block_height = nullptr);
    const TransactionEntry &transactionByIndex(TransactionIndex index);
    bool pushBlock(const FBlock &blockData, block_verification_context &bvc);
    bool pushBlock(
        const FBlock &blockData,
        const std::vector<FTransaction> &transactions,
        block_verification_context &bvc);
    bool pushBlock(BlockEntry &block);
    void popBlock();
    bool pushTransaction(
        BlockEntry &block,
        const Crypto::FHash &transactionHash,
        TransactionIndex transactionIndex);
    void popTransaction(const FTransaction &transaction, const Crypto::FHash &transactionHash);
    void popTransactions(const BlockEntry &block, const Crypto::FHash &minerTransactionHash);
    bool validateInput(
        const FMultiSignatureInput &input,
        const Crypto::FHash &transactionHash,
        const Crypto::FHash &transactionPrefixHash,
        const std::vector<Crypto::FSignature> &transactionSignatures);
    bool checkCheckpoints(uint32_t &lastValidCheckpointHeight);
    void removeLastBlock();
    bool checkUpgradeHeight(const UpgradeDetector &upgradeDetector);

    bool storeBlockchainIndices();
    bool loadBlockchainIndices();

    bool loadTransactions(const FBlock &block, std::vector<FTransaction> &transactions);
    void saveTransactions(const std::vector<FTransaction> &transactions);

    void sendMessage(const BlockchainMessage &message);

    friend class LockedBlockchainStorage;
};

class LockedBlockchainStorage: boost::noncopyable
{
public:
    explicit LockedBlockchainStorage(Blockchain &bc)
        : m_bc(bc),
          m_lock(bc.m_blockchain_lock)
    {
    }

    Blockchain *operator->()
    {
        return &m_bc;
    }

private:
    Blockchain &m_bc;
    std::lock_guard<std::recursive_mutex> m_lock;
};

template<class visitor_t>
bool Blockchain::scanOutputKeysForIndexes(
    const FKeyInput &tx_in_to_key,
    visitor_t &vis,
    uint32_t *pmax_related_block_height)
{
    std::lock_guard<std::recursive_mutex> lk(m_blockchain_lock);
    auto it = m_outputs.find(tx_in_to_key.uAmount);
    if (it == m_outputs.end() || tx_in_to_key.vOutputIndexes.empty()) {
        return false;
    }

    auto absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.vOutputIndexes);
    std::vector<std::pair<TransactionIndex, uint16_t>> &amount_outs_vec = it->second;
    size_t count = 0;
    for (uint64_t i : absolute_offsets) {
        if(i >= amount_outs_vec.size() ) {
            logger(Logging::INFO)
                << "Wrong index in transaction inputs: " << i
                << ", expected maximum " << amount_outs_vec.size() - 1;
            return false;
        }

        const TransactionEntry &tx = transactionByIndex(amount_outs_vec[i].first);

        if (amount_outs_vec[i].second >= tx.tx.vOutputs.size()) {
            logger(Logging::ERROR, Logging::BRIGHT_RED)
                << "Wrong index in transaction outputs: "
                << amount_outs_vec[i].second << ", expected less then "
                << tx.tx.vOutputs.size();
            return false;
        }

        const auto veci = amount_outs_vec[i];
        if (!vis.handle_output(tx.tx, tx.tx.vOutputs[veci.second], veci.second)) {
            logger(Logging::INFO)
                << "Failed to handle_output for output no = " << count
                << ", with absolute offset " << i;
            return false;
        }

        if(count++ == absolute_offsets.size()-1 && pmax_related_block_height) {
            if (*pmax_related_block_height < amount_outs_vec[i].first.block) {
                *pmax_related_block_height = amount_outs_vec[i].first.block;
            }
        }
    }

    return true;
}

} // namespace QwertyNote
