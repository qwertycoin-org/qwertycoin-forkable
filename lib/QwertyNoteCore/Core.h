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

#include <ctime>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <BlockchainExplorer/BlockchainExplorerData.h>
#include <Common/ObserverManager.h>
#include <QwertyNoteCore/Blockchain.h>
#include <QwertyNoteCore/BlockchainMessages.h>
#include <QwertyNoteCore/Currency.h>
#include <QwertyNoteCore/ICore.h>
#include <QwertyNoteCore/ICoreObserver.h>
#include <QwertyNoteCore/IMinerHandler.h>
#include <QwertyNoteCore/MessageQueue.h>
#include <QwertyNoteCore/MinerConfig.h>
#include <QwertyNoteCore/TransactionPool.h>
#include <QwertyNoteProtocol/CryptoNoteProtocolHandlerCommon.h>
#include <Logging/LoggerMessage.h>
#include <P2p/NetNodeCommon.h>
#include <System/Dispatcher.h>

namespace QwertyNote {

  struct core_stat_info;
  class miner;
  class CoreConfig;

class core : public ICore,
             public IMinerHandler,
             public IBlockchainStorageObserver,
             public ITxPoolObserver
{
public:
    core(
        const Currency &currency,
        i_cryptonote_protocol *pprotocol,
        Logging::ILogger &logger,
        bool blockchainIndexesEnabled);
    ~core() override;

    bool on_idle() override;
    bool handle_incoming_tx( // TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
        const BinaryArray &tx_blob,
        tx_verification_context &tvc,
        bool keeped_by_block,
        bool loose_check) override;
    bool handle_incoming_block_blob(
        const BinaryArray &block_blob,
        block_verification_context &bvc,
        bool control_miner,
        bool relay_block) override;

    i_cryptonote_protocol *get_protocol() override { return m_pprotocol; }
    const Currency &currency() const { return m_currency; }

    // IMinerHandler
    bool handle_block_found(FBlock &b) override;
    bool get_block_template(
			FBlock &b,
			const FAccountPublicAddress &adr,
			difficulty_type &diffic,
			uint32_t &height,
			const BinaryArray &ex_nonce) override;
    bool get_difficulty_stat(
        uint32_t height,
        stat_period period,
        uint32_t& block_num,
        uint64_t& avg_solve_time,
        uint64_t& stddev_solve_time,
        uint32_t& outliers_num,
        difficulty_type &avg_diff,
        difficulty_type &min_diff,
        difficulty_type &max_diff) override;

    bool addObserver(ICoreObserver *observer) override;
    bool removeObserver(ICoreObserver *observer) override;

    miner &get_miner() { return *m_miner; }

    static void init_options(boost::program_options::options_description &desc);
    bool init(const CoreConfig &config, const MinerConfig &minerConfig, bool load_existing);
    bool set_genesis_block(const FBlock &b);
    bool deinit();

    // ICore
    size_t addChain(const std::vector<const IBlock *> &chain) override;
    bool handle_get_objects( // TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
        NOTIFY_REQUEST_GET_OBJECTS_request &arg,
        NOTIFY_RESPONSE_GET_OBJECTS_request &rsp) override;
    bool getBackwardBlocksSizes(
        uint32_t fromHeight,
        std::vector<size_t> &sizes,
        size_t count) override;
    bool getBlockSize(const Crypto::FHash &hash, size_t &size) override;
    bool getAlreadyGeneratedCoins(const Crypto::FHash &hash, uint64_t &generatedCoins) override;
    bool getBlockReward(
        uint8_t blockMajorVersion,
        size_t medianSize,
        size_t currentBlockSize,
        uint64_t alreadyGeneratedCoins,
        uint64_t fee,
        uint64_t &reward,
        int64_t &emissionChange,
        uint32_t height,
        uint64_t blockTarget = QwertyNote::parameters::DIFFICULTY_TARGET) override;
    bool scanOutputkeysForIndices(
        const FKeyInput &txInToKey,
        std::list<std::pair<Crypto::FHash, size_t>> &outputReferences) override;
    bool getBlockDifficulty(uint32_t height, difficulty_type &difficulty) override;
    bool getBlockCumulativeDifficulty(uint32_t height, difficulty_type &difficulty) override;
    bool getBlockContainingTx(
        const Crypto::FHash &txId,
        Crypto::FHash &blockId,
        uint32_t &blockHeight) override;
    bool getMultisigOutputReference(
        const FMultiSignatureInput &txInMultisig,
        std::pair<Crypto::FHash, size_t> &output_reference) override;
    bool getGeneratedTransactionsNumber(uint32_t height, uint64_t &generatedTransactions) override;
    bool getOrphanBlocksByHeight(uint32_t height, std::vector<FBlock> &blocks) override;
    bool getBlocksByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t blocksNumberLimit,
        std::vector<FBlock> &blocks,
        uint32_t &blocksNumberWithinTimestamps) override;
    bool getPoolTransactionsByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t transactionsNumberLimit,
        std::vector<FTransaction> &transactions,
        uint64_t &transactionsNumberWithinTimestamps) override;
    bool getTransactionsByPaymentId(
        const Crypto::FHash &paymentId,
        std::vector<FTransaction> &transactions) override;
    std::vector<Crypto::FHash> getTransactionHashesByPaymentId(
        const Crypto::FHash &paymentId) override;
    bool getOutByMultiSigGlobalIndex(uint64_t amount, uint64_t gindex, FMultiSignatureOutput &out) override;
    std::unique_ptr<IBlock> getBlock(const Crypto::FHash &blocksId) override;
    bool handleIncomingTransaction(
        const FTransaction &tx,
        const Crypto::FHash &txHash,
        size_t blobSize,
        tx_verification_context &tvc,
        bool keptByBlock,
        uint32_t height,
        bool loose_check) override;
    std::error_code executeLocked(const std::function<std::error_code()> &func) override;
    uint64_t getMinimalFeeForHeight(uint32_t height) override;
    uint64_t getMinimalFee() override;
    uint64_t getBlockTimestamp(uint32_t height) override;

    bool addMessageQueue(MessageQueue<BlockchainMessage> &messageQueue) override;
    bool removeMessageQueue(MessageQueue<BlockchainMessage> &messageQueue) override;

    virtual std::time_t getStartTime() const;

    uint32_t getCurrentBlockchainHeight() override;
    uint8_t getCurrentBlockMajorVersion() override;
    uint8_t getBlockMajorVersionForHeight(uint32_t height) override;

    static bool getPaymentId(const FTransaction &transaction, Crypto::FHash &paymentId);

    bool have_block(const Crypto::FHash &id) override;
    std::vector<Crypto::FHash> buildSparseChain() override;
    std::vector<Crypto::FHash> buildSparseChain(const Crypto::FHash &startBlockId) override;
    void on_synchronized() override;

    void get_blockchain_top(uint32_t &height, Crypto::FHash &top_id) override;
    bool get_blocks(
        uint32_t start_offset,
        uint32_t count,
        std::list<FBlock> &blocks,
        std::list<FTransaction> &txs);
    bool get_blocks(uint32_t start_offset, uint32_t count, std::list<FBlock> &blocks);

    template<class T, class D, class S>
    bool get_blocks(const T &block_ids, D &blocks, S &missed_bs)
    {
        return m_blockchain.getBlocks(block_ids, blocks, missed_bs);
    }

    bool queryBlocks(
        const std::vector<Crypto::FHash> &block_ids,
        uint64_t timestamp,
        uint32_t &start_height,
        uint32_t &current_height,
        uint32_t &full_offset,
        std::vector<BlockFullInfo> &entries) override;

    bool queryBlocksLite(
        const std::vector<Crypto::FHash> &knownBlockIds,
        uint64_t timestamp,
        uint32_t &resStartHeight,
        uint32_t &resCurrentHeight,
        uint32_t &resFullOffset,
        std::vector<BlockShortInfo> &entries) override;

    bool queryBlocksDetailed(
        const std::vector<Crypto::FHash> &knownBlockHashes,
        uint64_t timestamp,
        uint32_t &startIndex,
        uint32_t &currentIndex,
        uint32_t &fullOffset,
        std::vector<BlockFullInfo> &entries) override;

    Crypto::FHash getBlockIdByHeight(uint32_t height) override;
    void getTransactions(
        const std::vector<Crypto::FHash> &txs_ids,
        std::list<FTransaction> &txs,
        std::list<Crypto::FHash> &missed_txs,
        bool checkTxPool = false) override;
    bool getTransactionsWithOutputGlobalIndexes(const std::vector<Crypto::FHash> &txsIds,
												std::list<Crypto::FHash> &missedTxs,
												std::vector<std::pair<FTransaction,
																	  std::vector<uint32_t>>> &txs) override;
    bool getBlockByHash(const Crypto::FHash &h, FBlock &blk) override;
    bool getBlockHeight(const Crypto::FHash &blockId, uint32_t &blockHeight) override;

    bool getAlternativeBlocks(std::list<FBlock> &blocks);
    size_t getAlternativeBlocksCount();

    virtual bool getBlockEntry(uint32_t height,
                               uint64_t &blockCumulativeSize,
                               difficulty_type &difficulty,
                               uint64_t &alreadyGeneratedCoins,
                               uint64_t &reward,
                               uint64_t &transactionsCount,
                               uint64_t &timestamp) override;

    void set_cryptonote_protocol(i_cryptonote_protocol *pprotocol);
    void set_checkpoints(Checkpoints &&chk_pts);

    std::vector<FTransaction> getPoolTransactions() override;
    size_t get_pool_transactions_count();
    size_t get_blockchain_total_transactions();

    std::vector<Crypto::FHash> findBlockchainSupplement(
        const std::vector<Crypto::FHash> &remoteBlockIds,
        size_t maxCount,
        uint32_t &totalBlockCount,
        uint32_t &startBlockIndex) override;

    bool get_stat_info(core_stat_info &st_inf) override;

    bool getTxOutputsGlobalIndexes(const Crypto::FHash &tx_id, std::vector<uint32_t> &indexs) override;
    Crypto::FHash get_tail_id();
    bool get_random_outs_for_amounts(
        const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request &req,
        COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response &res) override;
    void pause_mining() override;
    void update_block_template_and_resume_mining() override;

    Blockchain &get_blockchain_storage() { return m_blockchain; }

    std::string getConfigFolder() { return m_config_folder; }

    // debug functions
    void print_blockchain(uint32_t start_index, uint32_t end_index);
    void print_blockchain_index();
    std::string print_pool(bool short_format);
    std::list<QwertyNote::tx_memory_pool::TransactionDetails> getMemoryPool() const;
    void print_blockchain_outs(const std::string &file);
    bool getPoolChanges(
        const Crypto::FHash &tailBlockId,
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<FTransaction> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) override;
    bool getPoolChangesLite(
        const Crypto::FHash &tailBlockId,
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<TransactionPrefixInfo> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) override;
    void getPoolChanges(
        const std::vector<Crypto::FHash> &knownTxsIds,
        std::vector<FTransaction> &addedTxs,
        std::vector<Crypto::FHash> &deletedTxsIds) override;

    void rollbackBlockchain(uint32_t height) override;

    uint64_t getNextBlockDifficulty(uint64_t nextBlockTime);
    uint64_t getTotalGeneratedAmount();
    uint8_t getBlockMajorVersionForHeight(uint32_t height) const;
    bool getMixin(const FTransaction &transaction, uint64_t &mixin);

    bool is_key_image_spent(const Crypto::FKeyImage &key_im);

    void setBlocksToFind(uint64_t blocksToFind);

private:
    bool add_new_tx(
        const FTransaction &tx,
        const Crypto::FHash &tx_hash,
        size_t blob_size,
        tx_verification_context &tvc,
        bool keeped_by_block);
    bool load_state_data();
    bool parse_tx_from_blob(
			FTransaction &tx,
			Crypto::FHash &tx_hash,
			Crypto::FHash &tx_prefix_hash,
			const BinaryArray &blob);
    bool handle_incoming_block(
        const FBlock &b,
        block_verification_context &bvc,
        bool control_miner,
        bool relay_block);

    bool check_tx_syntax(const FTransaction &tx);
    // check correct values, amounts and all lightweight checks not related with database
    bool check_tx_semantic(const FTransaction &tx, bool keeped_by_block);
    // check if tx already in memory pool or in main blockchain
    bool check_tx_mixin(const FTransaction &tx, uint32_t height);
    // check if the mixin is not too large
    bool check_tx_fee(
        const FTransaction &tx,
        size_t blobSize,
        tx_verification_context &tvc,
        uint32_t height,
        bool loose_check);
    // check if tx is not sending unmixable outputs
    bool check_tx_unmixable(const FTransaction &tx, uint32_t height);

    bool update_miner_block_template();
    bool handle_command_line(const boost::program_options::variables_map &vm);
    bool check_tx_inputs_keyimages_diff(const FTransaction &tx);
    void blockchainUpdated() override;
    void txDeletedFromPool() override;
    void poolUpdated();

    bool findStartAndFullOffsets(
        const std::vector<Crypto::FHash> &knownBlockIds,
        uint64_t timestamp,
        uint32_t &startOffset,
        uint32_t &startFullOffset);

    std::vector<Crypto::FHash> findIdsForShortBlocks(uint32_t startOffset, uint32_t startFullOffset);

    const Currency &m_currency;
    Logging::LoggerRef logger;
    QwertyNote::RealTimeProvider m_timeProvider;
    tx_memory_pool m_mempool;
    Blockchain m_blockchain;
    i_cryptonote_protocol *m_pprotocol;
    std::unique_ptr<miner> m_miner;
    std::string m_config_folder;
    cryptonote_protocol_stub m_protocol_stub;
    std::atomic<bool> m_starter_message_showed;
    Tools::QObserverManager<ICoreObserver> m_observerManager;
    time_t start_time;

    std::atomic<uint64_t> m_blocksFound;
    std::atomic<uint64_t> m_blocksToFind;

    friend class tx_validate_inputs;
};

} // namespace QwertyNote
