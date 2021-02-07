// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2019, The Karbowanec developers
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

#include <sstream>
#include <unordered_set>

#include <boost/utility/value_init.hpp>
#include <boost/range/combine.hpp>

#include <Common/CommandLine.h>
#include <Common/Math.h>
#include <Common/StringTools.h>
#include <Common/Util.h>

#include <Crypto/Crypto.h>

#include <Global/QwertyNoteConfig.h>

#include <Logging/LoggerRef.h>

#include <QwertyNoteCore/Core.h>
#include <QwertyNoteCore/CoreConfig.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteStatInfo.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/IBlock.h>
#include <QwertyNoteCore/Miner.h>
#include <QwertyNoteCore/TransactionExtra.h>

#include <QwertyNoteProtocol/CryptoNoteProtocolDefinitions.h>

#include <Rpc/CoreRpcServerCommandsDefinitions.h>

#undef ERROR

using namespace Logging;
using namespace Common;
using namespace Qwertycoin;

namespace QwertyNote {

class BlockWithTransactions : public IBlock
{
public:
    const FBlock &getBlock() const override { return block; }

    size_t getTransactionCount() const override { return transactions.size(); }

    const FTransaction &getTransaction(size_t index) const override
    {
        assert(index < transactions.size());
        return transactions[index];
    }

private:
    FBlock block;
    std::vector<FTransaction> transactions;

    friend class core;
};

core::core(const Currency &currency, i_cryptonote_protocol *pprotocol, Logging::ILogger &logger,
           bool blockchainIndexesEnabled)
    : m_currency(currency),
      logger(logger, "core"),
      m_mempool(currency, m_blockchain, *this, m_timeProvider, logger, blockchainIndexesEnabled),
      m_blockchain(currency, m_mempool, logger, blockchainIndexesEnabled),
      m_miner(new miner(currency, *this, logger)),
      m_starter_message_showed(false)
{
    set_cryptonote_protocol(pprotocol);
    m_blockchain.addObserver(this);
    m_mempool.addObserver(this);
}

core::~core()
{
    m_blockchain.removeObserver(this);
}

void core::set_cryptonote_protocol(i_cryptonote_protocol *pprotocol)
{
    if (pprotocol) {
        m_pprotocol = pprotocol;
    } else {
        m_pprotocol = &m_protocol_stub;
    }
}

void core::set_checkpoints(Checkpoints &&chk_pts)
{
    m_blockchain.setCheckpoints(std::move(chk_pts));
}

void core::init_options(boost::program_options::options_description &desc) { }

bool core::handle_command_line(const boost::program_options::variables_map &vm)
{
    m_config_folder = CommandLine::getArg(vm, CommandLine::argDataDir);
    return true;
}

uint32_t core::getCurrentBlockchainHeight()
{
    return m_blockchain.getCurrentBlockchainHeight();
}

uint8_t core::getCurrentBlockMajorVersion()
{
    assert(m_blockchain.getCurrentBlockchainHeight() > 0);
    return m_blockchain.getBlockMajorVersionForHeight(m_blockchain.getCurrentBlockchainHeight());
}

uint8_t core::getBlockMajorVersionForHeight(uint32_t height)
{
    assert(m_blockchain.getCurrentBlockchainHeight() > 0);
    return m_blockchain.getBlockMajorVersionForHeight(height);
}

void core::get_blockchain_top(uint32_t &height, Crypto::FHash &top_id)
{
    assert(m_blockchain.getCurrentBlockchainHeight() > 0);
    top_id = m_blockchain.getTailId(height);
}

bool core::get_blocks(uint32_t start_offset, uint32_t count, std::list<FBlock> &blocks,
                      std::list<FTransaction> &txs)
{
    return m_blockchain.getBlocks(start_offset, count, blocks, txs);
}

bool core::get_blocks(uint32_t start_offset, uint32_t count, std::list<FBlock> &blocks)
{
    return m_blockchain.getBlocks(start_offset, count, blocks);
}

void core::getTransactions(const std::vector<Crypto::FHash> &txs_ids, std::list<FTransaction> &txs,
						   std::list<Crypto::FHash> &missed_txs, bool checkTxPool)
{
    m_blockchain.getTransactions(txs_ids, txs, missed_txs, checkTxPool);
}

bool core::getTransactionsWithOutputGlobalIndexes(
		const std::vector<Crypto::FHash> &txsIds, std::list<Crypto::FHash> &missedTxs,
		std::vector<std::pair<FTransaction, std::vector<uint32_t>>> &txs)
{
    return m_blockchain.getTransactionsWithOutputGlobalIndexes(txsIds, missedTxs, txs);
}

bool core::getAlternativeBlocks(std::list<FBlock> &blocks)
{
    return m_blockchain.getAlternativeBlocks(blocks);
}

size_t core::getAlternativeBlocksCount()
{
    return m_blockchain.getAlternativeBlocksCount();
}

bool core::getBlockEntry(uint32_t height, uint64_t &blockCumulativeSize,
                         difficulty_type &difficulty, uint64_t &alreadyGeneratedCoins,
                         uint64_t &reward, uint64_t &transactionsCount, uint64_t &timestamp)
{
    return m_blockchain.getBlockEntry(static_cast<size_t>(height), blockCumulativeSize, difficulty,
                                      alreadyGeneratedCoins, reward, transactionsCount, timestamp);
}

std::time_t core::getStartTime() const
{
    return start_time;
}

bool core::init(const CoreConfig &config, const MinerConfig &minerConfig, bool load_existing)
{
    m_config_folder = config.configFolder;
    bool r = m_mempool.init(m_config_folder);
    if (!(r)) {
        logger(ERROR, BRIGHT_RED) << "Failed to initialize memory pool";
        return false;
    }

    r = m_blockchain.init(m_config_folder, load_existing);
    if (!(r)) {
        logger(ERROR, BRIGHT_RED) << "Failed to initialize blockchain storage";
        return false;
    }

    r = m_miner->init(minerConfig);
    if (!(r)) {
        logger(ERROR, BRIGHT_RED) << "Failed to initialize miner";
        return false;
    }

    start_time = std::time(nullptr);

    return load_state_data();
}

bool core::set_genesis_block(const FBlock &b)
{
    return m_blockchain.resetAndSetGenesisBlock(b);
}

bool core::load_state_data()
{
    // TODO: may be some code later
    return true;
}

bool core::deinit()
{
    m_miner->stop();
    m_mempool.deinit();
    m_blockchain.deinit();

    return true;
}

size_t core::addChain(const std::vector<const IBlock *> &chain)
{
    size_t blocksCounter = 0;

    for (const IBlock *block : chain) {
        bool allTransactionsAdded = true;
        for (size_t txNumber = 0; txNumber < block->getTransactionCount(); ++txNumber) {
            const FTransaction &tx = block->getTransaction(txNumber);

            Crypto::FHash txHash = NULL_HASH;
            size_t blobSize = 0;
            getObjectHash(tx, txHash, blobSize);
            tx_verification_context tvc = boost::value_initialized<tx_verification_context>();

            if (!handleIncomingTransaction(tx, txHash, blobSize, tvc, true,
                                           get_block_height(block->getBlock()), true)) {
                logger(ERROR, BRIGHT_RED)
                        << "core::addChain() failed to handle transaction " << txHash
                        << " from block " << blocksCounter << "/" << chain.size();
                allTransactionsAdded = false;
                break;
            }
        }

        if (!allTransactionsAdded) {
            break;
        }

        block_verification_context bvc = boost::value_initialized<block_verification_context>();
        m_blockchain.addNewBlock(block->getBlock(), bvc);
        if (bvc.m_marked_as_orphaned || bvc.m_verification_failed) {
            logger(ERROR, BRIGHT_RED) << "core::addChain() failed to handle incoming block "
                                      << getBlockHash(block->getBlock()) << ", " << blocksCounter
                                      << "/" << chain.size();
            break;
        }

        ++blocksCounter;
        // TODO: m_dispatcher.yield()?
    }

    return blocksCounter;
}

// TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
bool core::handle_incoming_tx(const BinaryArray &tx_blob, tx_verification_context &tvc,
                              bool keeped_by_block, bool loose_check)
{
    tvc = boost::value_initialized<tx_verification_context>();
    // want to process all transactions sequentially

    if (tx_blob.size() > m_currency.maxTransactionSizeLimit()
        && getCurrentBlockMajorVersion() >= BLOCK_MAJOR_VERSION_3) {
        logger(INFO) << "WRONG TRANSACTION BLOB, too big size " << tx_blob.size() << ", rejected";
        tvc.m_verification_failed = true;
        return false;
    }

    Crypto::FHash tx_hash = NULL_HASH;
    Crypto::FHash tx_prefixt_hash = NULL_HASH;
    FTransaction tx;

    if (!parse_tx_from_blob(tx, tx_hash, tx_prefixt_hash, tx_blob)) {
        logger(INFO) << "WRONG TRANSACTION BLOB, Failed to parse, rejected";
        tvc.m_verification_failed = true;
        return false;
    }

    Crypto::FHash blockId;
    uint32_t blockHeight;
    bool ok = getBlockContainingTx(tx_hash, blockId, blockHeight);
    if (!ok) {
        blockHeight = this->getCurrentBlockchainHeight();
    }
    return handleIncomingTransaction(tx, tx_hash, tx_blob.size(), tvc, keeped_by_block, blockHeight,
                                     loose_check);
}

bool core::get_stat_info(core_stat_info &st_inf)
{
    st_inf.mining_speed = m_miner->get_speed();
    st_inf.alternative_blocks = m_blockchain.getAlternativeBlocksCount();
    st_inf.blockchain_height = m_blockchain.getCurrentBlockchainHeight();
    st_inf.tx_pool_size = m_mempool.get_transactions_count();
    st_inf.top_block_id_str = Common::podToHex(m_blockchain.getTailId());

    return true;
}

bool core::check_tx_mixin(const FTransaction &tx, uint32_t height)
{
    size_t inputIndex = 0;
    for (const auto &txin : tx.vInputs) {
        assert(inputIndex < tx.vSignatures.size());
        if (txin.type() == typeid(FKeyInput)) {
            uint64_t txMixin = boost::get<FKeyInput>(txin).vOutputIndexes.size();
            if (txMixin > QwertyNote::parameters::MAX_TX_MIXIN_SIZE) {
                logger(ERROR) << "Transaction " << getObjectHash(tx)
                              << " has too large mixIn count, rejected";
                return false;
            }
            if (txMixin < QwertyNote::parameters::MIN_TX_MIXIN_SIZE) {
                logger(ERROR) << "Transaction " << getObjectHash(tx)
                              << " has mixIn count below the required minimum, rejected";
                return false;
            }
        }
    }
    return true;
}

bool core::check_tx_fee(const FTransaction &tx, size_t blobSize, tx_verification_context &tvc,
						uint32_t height, bool loose_check)
{
    uint64_t inputs_amount = 0;
    if (!getInputsMoneyAmount(tx, inputs_amount)) {
        tvc.m_verification_failed = true;
        return false;
    }

    uint64_t outputs_amount = getOutsMoneyAmount(tx);

    if (outputs_amount > inputs_amount) {
        logger(DEBUGGING) << "transaction use more money then it has: use "
                          << m_currency.formatAmount(outputs_amount) << ", have "
                          << m_currency.formatAmount(inputs_amount);

        tvc.m_verification_failed = true;

        return false;
    }

    Crypto::FHash h = NULL_HASH;
    getObjectHash(tx, h, blobSize);
    const uint64_t fee = inputs_amount - outputs_amount;
    bool isFusionTransaction = fee == 0 && m_currency.isFusionTransaction(tx, blobSize, height);

    if (!isFusionTransaction) {
        std::vector<TransactionExtraField> txExtraFields;
        parseTransactionExtra(tx.vExtra, txExtraFields);
        TransactionExtraTTL ttl;

        if (!findTransactionExtraFieldByType(txExtraFields, ttl)) {
            ttl.ttl = 0;

            // TODO: simplify overcomplicated expression.
            if (fee < QwertyNote::parameters::MINIMUM_FEE) {
                logger(ERROR) << "[Core] Transaction fee is not enough: "
                              << m_currency.formatAmount(fee)
                              << ", minimum fee: " << QwertyNote::parameters::MINIMUM_FEE;

                tvc.m_verification_failed = true;
                tvc.m_tx_fee_too_small = true;

                return false;
            }
        } else {
            return true;
        }
    }
    return true;
}

bool core::check_tx_unmixable(const FTransaction &tx, uint32_t height)
{
    for (const auto &out : tx.vOutputs) {
        if (!is_valid_decomposed_amount(out.uAmount)
            && height > QwertyNote::parameters::UPGRADE_HEIGHT_V1) {
            logger(ERROR) << "Invalid decomposed output amount " << out.uAmount
                          << " for tx id= " << getObjectHash(tx);
            return false;
        }
    }
    return true;
}

bool core::check_tx_semantic(const FTransaction &tx, bool keeped_by_block)
{
    if (tx.vInputs.empty()) {
        logger(ERROR) << "tx with empty inputs, rejected for tx id= " << getObjectHash(tx);
        return false;
    }

    if (tx.vInputs.size() != tx.vSignatures.size()) {
        logger(ERROR) << "tx signatures size doesn't match inputs size, rejected for tx id= "
                      << getObjectHash(tx);
        return false;
    }

    for (size_t i = 0; i < tx.vInputs.size(); ++i) {
        if (tx.vInputs[i].type() == typeid(FKeyInput)) {
            if (boost::get<FKeyInput>(tx.vInputs[i]).vOutputIndexes.size()
                != tx.vSignatures[i].size()) {
                logger(ERROR) << "tx signatures count doesn't match outputIndexes count for input "
                              << i << ", rejected for tx id= " << getObjectHash(tx);
                return false;
            }
        }
    }

    if (!check_inputs_types_supported(tx)) {
        logger(ERROR) << "unsupported input types for tx id= " << getObjectHash(tx);
        return false;
    }

    std::string errmsg;
    if (!check_outs_valid(tx, &errmsg)) {
        logger(ERROR) << "tx with invalid outputs, rejected for tx id= " << getObjectHash(tx)
                      << ": " << errmsg;
        return false;
    }

    if (!check_money_overflow(tx)) {
        logger(ERROR) << "tx have money overflow, rejected for tx id= " << getObjectHash(tx);
        return false;
    }

    uint64_t amount_in = 0;
    getInputsMoneyAmount(tx, amount_in);
    uint64_t amount_out = getOutsMoneyAmount(tx);

    if (amount_in < amount_out) {
        logger(ERROR) << "tx with wrong amounts:"
                      << " ins " << amount_in << ", outs " << amount_out
                      << ", rejected for tx id= " << getObjectHash(tx);
        return false;
    }

    // check if tx use different key images
    if (!check_tx_inputs_keyimages_diff(tx)) {
        logger(ERROR) << "tx has a few inputs with identical keyimages";
        return false;
    }

    if (!checkMultisignatureInputsDiff(tx)) {
        logger(ERROR) << "tx has a few multisignature inputs with identical output indexes";
        return false;
    }

    return true;
}

bool core::check_tx_inputs_keyimages_diff(const FTransaction &tx)
{
    std::unordered_set<Crypto::FKeyImage> ki;
    std::set<std::pair<uint64_t, uint32_t>> outputsUsage;
    for (const auto &input : tx.vInputs) {
        if (input.type() == typeid(FKeyInput)) {
            const FKeyInput &in = boost::get<FKeyInput>(input);
            if (!ki.insert(in.sKeyImage).second) {
                logger(ERROR) << "Transaction has identical key images";
                return false;
            }

            if (in.vOutputIndexes.empty()) {
                logger(ERROR) << "Transaction's input uses empty output";
                return false;
            }

            // Additional key_image check.
            // Fix discovered by Monero Lab and suggested by "fluffypony" (bitcointalk.org)
            if (!(scalarMultKey(in.sKeyImage, Crypto::EllipticCurveScalar2KeyImage(Crypto::L))
                  == Crypto::EllipticCurveScalar2KeyImage(Crypto::I))) {
                logger(ERROR) << "Transaction uses key image not in the valid domain";
                return false;
            }

            // outputIndexes are packed here, first is absolute, others are offsets to previous,
            // so first can be zero, others can't
            auto i = std::find(++std::begin(in.vOutputIndexes), std::end(in.vOutputIndexes), 0);
            if (i != std::end(in.vOutputIndexes)) {
                logger(ERROR) << "Transaction has identical output indexes";
                return false;
            }
        }
    }

    return true;
}

size_t core::get_blockchain_total_transactions()
{
    return m_blockchain.getTotalTransactions();
}

bool core::add_new_tx(const FTransaction &tx, const Crypto::FHash &tx_hash, size_t blob_size,
					  tx_verification_context &tvc, bool keeped_by_block)
{
    // Locking on m_mempool and m_blockchain closes possibility to add tx
    // to memory pool which is already in blockchain
    std::lock_guard<decltype(m_mempool)> lk(m_mempool);
    LockedBlockchainStorage lbs(m_blockchain);

    if (m_blockchain.haveTransaction(tx_hash)) {
        logger(TRACE) << "tx " << tx_hash << " is already in blockchain";
        return true;
    }

    if (m_mempool.have_tx(tx_hash)) {
        logger(TRACE) << "tx " << tx_hash << " is already in transaction pool";
        return true;
    }

    return m_mempool.add_tx(tx, tx_hash, blob_size, tvc, keeped_by_block);
}

bool core::get_block_template(FBlock &b, const FAccountPublicAddress &adr, difficulty_type &diffic,
							  uint32_t &height, const BinaryArray &ex_nonce)
{
    size_t median_size;
    uint64_t already_generated_coins;

    {
        LockedBlockchainStorage blockchainLock(m_blockchain);
        height = m_blockchain.getCurrentBlockchainHeight();

        b = boost::value_initialized<FBlock>();
        b.uMajorVersion = m_blockchain.getBlockMajorVersionForHeight(height);

        if (b.uMajorVersion == BLOCK_MAJOR_VERSION_1) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_1)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        } else if (b.uMajorVersion == BLOCK_MAJOR_VERSION_2) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_2)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        } else if (b.uMajorVersion == BLOCK_MAJOR_VERSION_3) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_3)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        } else if (b.uMajorVersion == BLOCK_MAJOR_VERSION_4) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_4)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        } else if (b.uMajorVersion == BLOCK_MAJOR_VERSION_5) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_5)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        } else if (b.uMajorVersion >= BLOCK_MAJOR_VERSION_6) {
            b.uMinorVersion = m_currency.upgradeHeight(BLOCK_MAJOR_VERSION_6)
							  == UpgradeDetectorBase::UNDEF_HEIGHT
                    ? BLOCK_MINOR_VERSION_1
                    : BLOCK_MINOR_VERSION_0;
        }

        b.sPreviousBlockHash = get_tail_id();
        b.uTimestamp = time(nullptr);

        // Don't generate a block template with invalid timestamp
        // Fix by Jagerman
        // https://github.com/graft-project/GraftNetwork/pull/118/commits

        if (height >= m_currency.timestampCheckWindow()) {
            std::vector<uint64_t> timestamps;
            for (size_t offset = height - m_currency.timestampCheckWindow(); offset < height;
                 ++offset) {
                timestamps.push_back(m_blockchain.getBlockTimestamp(offset));
            }
            uint64_t median_ts = Common::medianValue(timestamps);
            if (b.uTimestamp < median_ts) {
                b.uTimestamp = median_ts;
            }
        }

        diffic = m_blockchain.getDifficultyForNextBlock(b.uTimestamp);
        if (!(diffic)) {
            logger(ERROR, BRIGHT_RED) << "difficulty overhead.";
            return false;
        }

        median_size = m_blockchain.getCurrentCumulativeBlocksizeLimit() / 2;
        already_generated_coins = m_blockchain.getCoinsInCirculation();
    }

    size_t txs_size;
    uint64_t fee;
    if (!m_mempool.fill_block_template(b, median_size, m_currency.maxBlockCumulativeSize(height),
                                       already_generated_coins, txs_size, fee)) {
        logger(ERROR, BRIGHT_RED) << "failed to fill block template from mempool.";
        return false;
    }

    uint32_t previousBlockHeight = 0;
    uint64_t blockTarget = QwertyNote::parameters::DIFFICULTY_TARGET;

    if (height > QwertyNote::parameters::UPGRADE_HEIGHT_V1) {
        getBlockHeight(b.sPreviousBlockHash, previousBlockHeight);
        uint64_t prev_timestamp = getBlockTimestamp(previousBlockHeight);
        if (prev_timestamp > b.uTimestamp) {
            logger(ERROR, BRIGHT_RED) << "incorrect timestamp, prev = " << prev_timestamp
                                      << ",  new = " << b.uTimestamp;
            return false;
        }
        blockTarget = b.uTimestamp - getBlockTimestamp(previousBlockHeight);
    }

    // two-phase miner transaction generation: we don't know exact block size until we prepare
    // block, but we don't know reward until we know block size, so first miner transaction
    // generated with fake amount of money, and with phase we know think we know expected block size

    // make blocks coin-base tx looks close to real coinbase tx to get truthful blob size
    bool r = m_currency.constructMinerTx(b.uMajorVersion, height, median_size,
										 already_generated_coins, txs_size, fee, adr,
										 b.sBaseTransaction, ex_nonce, 14, blockTarget);
    if (!r) {
        logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, first chance";
        return false;
    }

    size_t cumulative_size = txs_size + getObjectBinarySize(b.sBaseTransaction);
    for (size_t try_count = 0; try_count != 10; ++try_count) {
        r = m_currency.constructMinerTx(b.uMajorVersion, height, median_size,
										already_generated_coins, cumulative_size, fee, adr,
										b.sBaseTransaction, ex_nonce, 14, blockTarget);

        if (!(r)) {
            logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, second chance";
            return false;
        }
        size_t coinbase_blob_size = getObjectBinarySize(b.sBaseTransaction);
        if (coinbase_blob_size > cumulative_size - txs_size) {
            cumulative_size = txs_size + coinbase_blob_size;
            continue;
        }

        if (coinbase_blob_size < cumulative_size - txs_size) {
            size_t delta = cumulative_size - txs_size - coinbase_blob_size;
            b.sBaseTransaction.vExtra.insert(b.sBaseTransaction.vExtra.end(), delta, 0);
            // here  could be 1 byte difference, because of extra field counter is varint,
            // and it can become from 1-byte len to 2-bytes len.
            if (cumulative_size != txs_size + getObjectBinarySize(b.sBaseTransaction)) {
                if (cumulative_size + 1 != txs_size + getObjectBinarySize(b.sBaseTransaction)) {
                    logger(ERROR, BRIGHT_RED)
                            << "unexpected case: cumulative_size=" << cumulative_size
                            << " + 1 is not equal txs_cumulative_size=" << txs_size
                            << " + get_object_blobsize(b.baseTransaction)="
                            << getObjectBinarySize(b.sBaseTransaction);
                    return false;
                }
                b.sBaseTransaction.vExtra.resize(b.sBaseTransaction.vExtra.size() - 1);
                if (cumulative_size != txs_size + getObjectBinarySize(b.sBaseTransaction)) {
                    // fuck, not lucky, -1 makes varint-counter size smaller,
                    // in that case we continue to grow with cumulative_size
                    logger(TRACE, BRIGHT_RED)
                            << "Miner tx creation have no luck with delta_extra size = " << delta
                            << " and " << delta - 1;
                    cumulative_size += delta - 1;
                    continue;
                }
                logger(DEBUGGING, BRIGHT_GREEN)
                        << "Setting extra for block: " << b.sBaseTransaction.vExtra.size()
                        << ", try_count=" << try_count;
            }
        }
        if (cumulative_size != txs_size + getObjectBinarySize(b.sBaseTransaction)) {
            logger(ERROR, BRIGHT_RED) << "unexpected case: cumulative_size=" << cumulative_size
                                      << " is not equal txs_cumulative_size=" << txs_size
                                      << " + get_object_blobsize(b.baseTransaction)="
                                      << getObjectBinarySize(b.sBaseTransaction);
            return false;
        }

        return true;
    }

    logger(ERROR, BRIGHT_RED) << "Failed to create_block_template with " << 10 << " tries";

    return false;
}

bool core::get_difficulty_stat(uint32_t height, IMinerHandler::stat_period period,
                               uint32_t &block_num, uint64_t &avg_solve_time,
                               uint64_t &stddev_solve_time, uint32_t &outliers_num,
                               difficulty_type &avg_diff, difficulty_type &min_diff,
                               difficulty_type &max_diff)
{
    return m_blockchain.getDifficultyStat(height, period, block_num, avg_solve_time,
                                          stddev_solve_time, outliers_num, avg_diff, min_diff,
                                          max_diff);
}

std::vector<Crypto::FHash>
core::findBlockchainSupplement(const std::vector<Crypto::FHash> &remoteBlockIds, size_t maxCount,
							   uint32_t &totalBlockCount, uint32_t &startBlockIndex)
{
    assert(!remoteBlockIds.empty());
    assert(remoteBlockIds.back() == m_blockchain.getBlockIdByHeight(0));

    return m_blockchain.findBlockchainSupplement(remoteBlockIds, maxCount, totalBlockCount,
                                                 startBlockIndex);
}

void core::print_blockchain(uint32_t start_index, uint32_t end_index)
{
    m_blockchain.print_blockchain(start_index, end_index);
}

void core::print_blockchain_index()
{
    m_blockchain.print_blockchain_index();
}

void core::print_blockchain_outs(const std::string &file)
{
    m_blockchain.print_blockchain_outs(file);
}

bool core::get_random_outs_for_amounts(
        const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request &req,
        COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response &res)
{
    return m_blockchain.getRandomOutsByAmount(req, res);
}

bool core::getTxOutputsGlobalIndexes(const Crypto::FHash &tx_id, std::vector<uint32_t> &indexs)
{
    return m_blockchain.getTransactionOutputGlobalIndexes(tx_id, indexs);
}

bool core::getOutByMultiSigGlobalIndex(uint64_t amount, uint64_t gindex, FMultiSignatureOutput &out)
{
    return m_blockchain.getOutByMultiSigGlobalIndex(amount, gindex, out);
}

void core::pause_mining()
{
    m_miner->pause();
}

void core::update_block_template_and_resume_mining()
{
    update_miner_block_template();
    m_miner->resume();
}

bool core::handle_block_found(FBlock &b)
{
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    handle_incoming_block(b, bvc, true, true);

    if (bvc.m_verification_failed) {
        logger(ERROR) << "mined block failed verification";
    } else {
        if (m_blocksToFind > 0) {
            m_blocksFound++;
            if (m_blocksFound >= m_blocksToFind) {
                get_miner().send_stop_signal();
            }
        }
    }

    return bvc.m_added_to_main_chain;
}

void core::on_synchronized()
{
    m_miner->on_synchronized();
}

bool core::getPoolChanges(const Crypto::FHash &tailBlockId,
                          const std::vector<Crypto::FHash> &knownTxsIds,
                          std::vector<FTransaction> &addedTxs,
                          std::vector<Crypto::FHash> &deletedTxsIds)
{
    getPoolChanges(knownTxsIds, addedTxs, deletedTxsIds);
    return tailBlockId == m_blockchain.getTailId();
}

bool core::getPoolChangesLite(const Crypto::FHash &tailBlockId,
                              const std::vector<Crypto::FHash> &knownTxsIds,
                              std::vector<TransactionPrefixInfo> &addedTxs,
                              std::vector<Crypto::FHash> &deletedTxsIds)
{
    std::vector<FTransaction> added;
    bool returnStatus = getPoolChanges(tailBlockId, knownTxsIds, added, deletedTxsIds);

    for (const auto &tx : added) {
        TransactionPrefixInfo tpi;
        tpi.txPrefix = tx; // TODO: Discards 24 bytes of uData.
        tpi.txHash = getObjectHash(tx);

        addedTxs.push_back(std::move(tpi));
    }

    return returnStatus;
}

void core::getPoolChanges(const std::vector<Crypto::FHash> &knownTxsIds,
                          std::vector<FTransaction> &addedTxs,
                          std::vector<Crypto::FHash> &deletedTxsIds)
{
    std::vector<Crypto::FHash> addedTxsIds;
    auto guard = m_mempool.obtainGuard();
    m_mempool.get_difference(knownTxsIds, addedTxsIds, deletedTxsIds);
    std::vector<Crypto::FHash> misses;
    m_mempool.getTransactions(addedTxsIds, addedTxs, misses);
    assert(misses.empty());
}

bool core::handle_incoming_block_blob(const BinaryArray &block_blob,
                                      block_verification_context &bvc, bool control_miner,
                                      bool relay_block)
{
    if (block_blob.size() > m_currency.maxBlockBlobSize()) {
        logger(INFO) << "WRONG BLOCK BLOB, too big size " << block_blob.size() << ", rejected";
        bvc.m_verification_failed = true;
        return false;
    }

    FBlock b;
    if (!fromBinaryArray(b, block_blob)) {
        logger(INFO) << "Failed to parse and validate new block";
        bvc.m_verification_failed = true;
        return false;
    }

    return handle_incoming_block(b, bvc, control_miner, relay_block);
}

bool core::handle_incoming_block(const FBlock &b, block_verification_context &bvc,
								 bool control_miner, bool relay_block)
{
    if (control_miner) {
        pause_mining();
    }

    m_blockchain.addNewBlock(b, bvc);

    if (control_miner) {
        update_block_template_and_resume_mining();
    }

    if (relay_block && bvc.m_added_to_main_chain) {
        std::list<Crypto::FHash> missed_txs;
        std::list<FTransaction> txs;
        m_blockchain.getTransactions(b.vTransactionHashes, txs, missed_txs);
        if (!missed_txs.empty() && getBlockIdByHeight(get_block_height(b)) != getBlockHash(b)) {
            logger(INFO) << "Block added, but it seems that reorganize just happened after that, "
                         << "do not relay this block";
        } else {
            if (!(txs.size() == b.vTransactionHashes.size() && missed_txs.empty())) {
                logger(ERROR, BRIGHT_RED)
                        << "can't find some transactions in found block:" << getBlockHash(b)
                        << " txs.size()=" << txs.size()
                        << ", b.transactionHashes.size()=" << b.vTransactionHashes.size()
                        << ", missed_txs.size()" << missed_txs.size();
                return false;
            }

            NOTIFY_NEW_BLOCK::request arg;
            arg.hop = 0;
            arg.current_blockchain_height = m_blockchain.getCurrentBlockchainHeight();
            BinaryArray blockBa;
            bool r = toBinaryArray(b, blockBa);
            if (!(r)) {
                logger(ERROR, BRIGHT_RED) << "failed to serialize block";
                return false;
            }
            arg.b.block = asString(blockBa);
            for (auto &tx : txs) {
                arg.b.txs.push_back(asString(toBinaryArray(tx)));
            }

            m_pprotocol->relay_block(arg);
        }
    }

    return true;
}

Crypto::FHash core::get_tail_id()
{
    return m_blockchain.getTailId();
}

size_t core::get_pool_transactions_count()
{
    return m_mempool.get_transactions_count();
}

bool core::have_block(const Crypto::FHash &id)
{
    return m_blockchain.haveBlock(id);
}

bool core::parse_tx_from_blob(FTransaction &tx, Crypto::FHash &tx_hash, Crypto::FHash &tx_prefix_hash,
							  const BinaryArray &blob)
{
    return parseAndValidateTransactionFromBinaryArray(blob, tx, tx_hash, tx_prefix_hash);
}

bool core::check_tx_syntax(const FTransaction &tx)
{
    return true;
}

std::vector<FTransaction> core::getPoolTransactions()
{
    std::list<FTransaction> txs;
    m_mempool.get_transactions(txs);

    std::vector<FTransaction> result;
    for (auto &tx : txs) {
        result.emplace_back(std::move(tx));
    }
    return result;
}

std::list<QwertyNote::tx_memory_pool::TransactionDetails> core::getMemoryPool() const
{
    return m_mempool.getMemoryPool();
}

std::vector<Crypto::FHash> core::buildSparseChain()
{
    assert(m_blockchain.getCurrentBlockchainHeight() != 0);
    return m_blockchain.buildSparseChain();
}

std::vector<Crypto::FHash> core::buildSparseChain(const Crypto::FHash &startBlockId)
{
    LockedBlockchainStorage lbs(m_blockchain);
    assert(m_blockchain.haveBlock(startBlockId));
    return m_blockchain.buildSparseChain(startBlockId);
}

// TODO: Deprecated. Should be removed with CryptoNoteProtocolHandler.
bool core::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request &arg,
                              NOTIFY_RESPONSE_GET_OBJECTS::request &rsp)
{
    return m_blockchain.handleGetObjects(arg, rsp);
}

Crypto::FHash core::getBlockIdByHeight(uint32_t height)
{
    LockedBlockchainStorage lbs(m_blockchain);
    if (height < m_blockchain.getCurrentBlockchainHeight()) {
        return m_blockchain.getBlockIdByHeight(height);
    } else {
        return NULL_HASH;
    }
}

bool core::getBlockByHash(const Crypto::FHash &h, FBlock &blk)
{
    return m_blockchain.getBlockByHash(h, blk);
}

bool core::getBlockHeight(const Crypto::FHash &blockId, uint32_t &blockHeight)
{
    return m_blockchain.getBlockHeight(blockId, blockHeight);
}

std::string core::print_pool(bool short_format)
{
    return m_mempool.print_pool(short_format);
}

bool core::update_miner_block_template()
{
    m_miner->on_block_chain_update();
    return true;
}

bool core::on_idle()
{
    if (!m_starter_message_showed) {
        logger(INFO) << ENDL
                     << "**********************************************************************"
                     << ENDL << "The daemon will start synchronizing with the network. "
                     << "It may take up to several hours." << ENDL << ENDL
                     << "You can set the level of process detalization through \"set_log <level>\" "
                        "command, "
                     << "where <level> is between 0 (no details) and 4 (very verbose)." << ENDL
                     << ENDL << "Use \"help\" command to see the list of available commands."
                     << ENDL << ENDL
                     << "Note: in case you need to interrupt the process, use \"exit\" command. "
                     << "Otherwise, the current progress won't be saved." << ENDL
                     << "**********************************************************************";
        m_starter_message_showed = true;
    }

    m_miner->on_idle();
    m_mempool.on_idle();

    return true;
}

bool core::addObserver(ICoreObserver *observer)
{
    return m_observerManager.add(observer);
}

bool core::removeObserver(ICoreObserver *observer)
{
    return m_observerManager.remove(observer);
}

void core::blockchainUpdated()
{
    m_observerManager.notify(&ICoreObserver::blockchainUpdated);
}

void core::txDeletedFromPool()
{
    poolUpdated();
}

void core::poolUpdated()
{
    m_observerManager.notify(&ICoreObserver::poolUpdated);
}

bool core::queryBlocks(const std::vector<Crypto::FHash> &knownBlockIds, uint64_t timestamp,
					   uint32_t &resStartHeight, uint32_t &resCurrentHeight,
					   uint32_t &resFullOffset, std::vector<BlockFullInfo> &entries)
{
    LockedBlockchainStorage lbs(m_blockchain);

    uint32_t currentHeight = lbs->getCurrentBlockchainHeight();
    uint32_t startOffset = 0;
    uint32_t startFullOffset = 0;

    if (!findStartAndFullOffsets(knownBlockIds, timestamp, startOffset, startFullOffset)) {
        return false;
    }

    resFullOffset = startFullOffset;
    std::vector<Crypto::FHash> blockIds = findIdsForShortBlocks(startOffset, startFullOffset);
    entries.reserve(blockIds.size());

    for (const auto &id : blockIds) {
        entries.push_back(BlockFullInfo());
        entries.back().block_id = id;
    }

    resCurrentHeight = currentHeight;
    resStartHeight = startOffset;

    uint32_t blocksLeft =
            static_cast<uint32_t>(std::min(BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT - entries.size(),
                                           size_t(BLOCKS_SYNCHRONIZING_DEFAULT_COUNT)));

    if (blocksLeft == 0) {
        return true;
    }

    std::list<FBlock> blocks;
    lbs->getBlocks(startFullOffset, blocksLeft, blocks);

    for (auto &b : blocks) {
        BlockFullInfo item;

        item.block_id = getBlockHash(b);

        if (b.uTimestamp >= timestamp) {
            // query transactions
            std::list<FTransaction> txs;
            std::list<Crypto::FHash> missedTxs;
            lbs->getTransactions(b.vTransactionHashes, txs, missedTxs);

            // fill uData
            BlockCompleteEntry &completeEntry = item;
            completeEntry.block = asString(toBinaryArray(b));
            for (auto &tx : txs) {
                completeEntry.txs.push_back(asString(toBinaryArray(tx)));
            }
        }

        entries.push_back(std::move(item));
    }

    return true;
}

bool core::findStartAndFullOffsets(const std::vector<Crypto::FHash> &knownBlockIds,
                                   uint64_t timestamp, uint32_t &startOffset,
                                   uint32_t &startFullOffset)
{
    LockedBlockchainStorage lbs(m_blockchain);

    if (knownBlockIds.empty()) {
        logger(ERROR, BRIGHT_RED) << "knownBlockIds is empty";
        return false;
    }

    if (knownBlockIds.back() != m_blockchain.getBlockIdByHeight(0)) {
        logger(ERROR, BRIGHT_RED) << "knownBlockIds doesn't end with genesis block FHash: "
                                  << knownBlockIds.back();
        return false;
    }

    startOffset = lbs->findBlockchainSupplement(knownBlockIds);
    if (!lbs->getLowerBound(timestamp, startOffset, startFullOffset)) {
        startFullOffset = startOffset;
    }

    return true;
}

std::vector<Crypto::FHash> core::findIdsForShortBlocks(uint32_t startOffset,
													   uint32_t startFullOffset)
{
    assert(startOffset <= startFullOffset);

    LockedBlockchainStorage lbs(m_blockchain);

    std::vector<Crypto::FHash> result;
    if (startOffset < startFullOffset) {
        result = lbs->getBlockIds(
                startOffset,
                std::min(static_cast<uint32_t>(BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT),
                         startFullOffset - startOffset));
    }

    return result;
}

bool core::queryBlocksLite(const std::vector<Crypto::FHash> &knownBlockIds, uint64_t timestamp,
						   uint32_t &resStartHeight, uint32_t &resCurrentHeight,
						   uint32_t &resFullOffset, std::vector<BlockShortInfo> &entries)
{
    LockedBlockchainStorage lbs(m_blockchain);

    resCurrentHeight = lbs->getCurrentBlockchainHeight();
    resStartHeight = 0;
    resFullOffset = 0;

    if (!findStartAndFullOffsets(knownBlockIds, timestamp, resStartHeight, resFullOffset)) {
        return false;
    }

    std::vector<Crypto::FHash> blockIds = findIdsForShortBlocks(resStartHeight, resFullOffset);
    entries.reserve(blockIds.size());

    for (const auto &id : blockIds) {
        entries.push_back(BlockShortInfo());
        entries.back().blockId = id;
    }

    uint32_t blocksLeft =
            static_cast<uint32_t>(std::min(BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT - entries.size(),
                                           size_t(BLOCKS_SYNCHRONIZING_DEFAULT_COUNT)));

    if (blocksLeft == 0) {
        return true;
    }

    std::list<FBlock> blocks;
    lbs->getBlocks(resFullOffset, blocksLeft, blocks);

    for (auto &b : blocks) {
        BlockShortInfo item;

        item.blockId = getBlockHash(b);

        if (b.uTimestamp >= timestamp) {
            std::list<FTransaction> txs;
            std::list<Crypto::FHash> missedTxs;
            lbs->getTransactions(b.vTransactionHashes, txs, missedTxs);

            item.block = asString(toBinaryArray(b));

            for (const auto &tx : txs) {
                TransactionPrefixInfo info;
                info.txPrefix = tx; // TODO: Slicing object from type loses 24 bytes.
                info.txHash = getObjectHash(tx);

                item.txPrefixes.push_back(std::move(info));
            }
        }

        entries.push_back(std::move(item));
    }

    return true;
}

bool core::queryBlocksDetailed(const std::vector<Crypto::FHash> &knownBlockHashes,
                               uint64_t timestamp, uint32_t &startIndex, uint32_t &currentIndex,
                               uint32_t &fullOffset, std::vector<BlockFullInfo> &entries)
{
    LockedBlockchainStorage lbs(m_blockchain);

    uint32_t currentHeight = lbs->getCurrentBlockchainHeight();
    uint32_t startOffset = 0;
    uint32_t startFullOffset = 0;

    fullOffset = startFullOffset;
    std::vector<Crypto::FHash> blockIds = findIdsForShortBlocks(startOffset, startFullOffset);
    entries.reserve(blockIds.size());

    for (const auto &id : blockIds) {
        entries.push_back(BlockFullInfo());
        entries.back().block_id = id;
    }

    currentIndex = currentHeight;
    startIndex = startOffset;

    uint32_t blocksLeft =
            static_cast<uint32_t>(std::min(BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT - entries.size(),
                                           size_t(BLOCKS_SYNCHRONIZING_DEFAULT_COUNT)));

    if (blocksLeft == 0) {
        return true;
    }

    std::list<FBlock> blocks;
    lbs->getBlocks(startFullOffset, blocksLeft, blocks);

    for (auto &b : blocks) {
        BlockFullInfo item;

        item.block_id = getBlockHash(b);

        if (b.uTimestamp >= timestamp) {
            // query transactions
            std::list<FTransaction> txs;
            std::list<Crypto::FHash> missedTxs;
            lbs->getTransactions(b.vTransactionHashes, txs, missedTxs);

            // fill uData
            BlockCompleteEntry &completeEntry = item;
            completeEntry.block = asString(toBinaryArray(b));
            for (auto &tx : txs) {
                completeEntry.txs.push_back(asString(toBinaryArray(tx)));
            }
        }

        entries.push_back(std::move(item));
    }

    return true;
}

bool core::getBackwardBlocksSizes(uint32_t fromHeight, std::vector<size_t> &sizes, size_t count)
{
    return m_blockchain.getBackwardBlocksSize(fromHeight, sizes, count);
}

bool core::getBlockSize(const Crypto::FHash &hash, size_t &size)
{
    return m_blockchain.getBlockSize(hash, size);
}

bool core::getAlreadyGeneratedCoins(const Crypto::FHash &hash, uint64_t &generatedCoins)
{
    return m_blockchain.getAlreadyGeneratedCoins(hash, generatedCoins);
}

bool core::getBlockReward(uint8_t blockMajorVersion, size_t medianSize, size_t currentBlockSize,
                          uint64_t alreadyGeneratedCoins, uint64_t fee, uint64_t &reward,
                          int64_t &emissionChange, uint32_t height, uint64_t blockTarget)
{
    return m_currency.getBlockReward(blockMajorVersion, medianSize, currentBlockSize,
                                     alreadyGeneratedCoins, fee, reward, emissionChange, height,
                                     blockTarget);
}

bool core::scanOutputkeysForIndices(const FKeyInput &txInToKey,
                                    std::list<std::pair<Crypto::FHash, size_t>> &outputReferences)
{
    struct outputs_visitor {
        explicit outputs_visitor(std::list<std::pair<Crypto::FHash, size_t>> &resultsCollector)
            : m_resultsCollector(resultsCollector)
        {
        }

        bool handle_output(const FTransaction &tx, const FTransactionOutput &out,
						   size_t transactionOutputIndex)
        {
            m_resultsCollector.push_back(std::make_pair(getObjectHash(tx), transactionOutputIndex));
            return true;
        }

        std::list<std::pair<Crypto::FHash, size_t>> &m_resultsCollector;
    };

    outputs_visitor vi(outputReferences);

    return m_blockchain.scanOutputKeysForIndexes(txInToKey, vi);
}

bool core::getBlockDifficulty(uint32_t height, difficulty_type &difficulty)
{
    difficulty = m_blockchain.blockDifficulty(height);
    return true;
}

bool core::getBlockCumulativeDifficulty(uint32_t height, difficulty_type &difficulty)
{
    difficulty = m_blockchain.blockCumulativeDifficulty(height);
    return true;
}

bool core::getBlockContainingTx(const Crypto::FHash &txId, Crypto::FHash &blockId,
								uint32_t &blockHeight)
{
    return m_blockchain.getBlockContainingTransaction(txId, blockId, blockHeight);
}

bool core::getMultisigOutputReference(const FMultiSignatureInput &txInMultisig,
                                      std::pair<Crypto::FHash, size_t> &outputReference)
{
    return m_blockchain.getMultisigOutputReference(txInMultisig, outputReference);
}

bool core::getGeneratedTransactionsNumber(uint32_t height, uint64_t &generatedTransactions)
{
    return m_blockchain.getGeneratedTransactionsNumber(height, generatedTransactions);
}

bool core::getOrphanBlocksByHeight(uint32_t height, std::vector<FBlock> &blocks)
{
    std::vector<Crypto::FHash> blockHashes;
    if (!m_blockchain.getOrphanBlockIdsByHeight(height, blockHashes)) {
        return false;
    }
    for (const Crypto::FHash &hash : blockHashes) {
        FBlock blk;
        if (!getBlockByHash(hash, blk)) {
            return false;
        }
        blocks.push_back(std::move(blk));
    }
    return true;
}

bool core::getBlocksByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd,
                                uint32_t blocksNumberLimit, std::vector<FBlock> &blocks,
                                uint32_t &blocksNumberWithinTimestamps)
{
    std::vector<Crypto::FHash> blockHashes;
    if (!m_blockchain.getBlockIdsByTimestamp(timestampBegin, timestampEnd, blocksNumberLimit,
                                             blockHashes, blocksNumberWithinTimestamps)) {
        return false;
    }
    for (const Crypto::FHash &hash : blockHashes) {
        FBlock blk;
        if (!getBlockByHash(hash, blk)) {
            return false;
        }
        blocks.push_back(std::move(blk));
    }
    return true;
}

bool core::getPoolTransactionsByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd,
                                          uint32_t transactionsNumberLimit,
                                          std::vector<FTransaction> &transactions,
                                          uint64_t &transactionsNumberWithinTimestamps)
{
    std::vector<Crypto::FHash> poolTransactionHashes;
    if (!m_mempool.getTransactionIdsByTimestamp(timestampBegin, timestampEnd,
                                                transactionsNumberLimit, poolTransactionHashes,
                                                transactionsNumberWithinTimestamps)) {
        return false;
    }

    std::list<FTransaction> txs;
    std::list<Crypto::FHash> missed_txs;

    getTransactions(poolTransactionHashes, txs, missed_txs, true);
    if (!missed_txs.empty()) {
        return false;
    }

    transactions.insert(transactions.end(), txs.begin(), txs.end());

    return true;
}

bool core::getTransactionsByPaymentId(const Crypto::FHash &paymentId,
                                      std::vector<FTransaction> &transactions)
{
    std::vector<Crypto::FHash> blockchainTransactionHashes;
    m_blockchain.getTransactionIdsByPaymentId(paymentId, blockchainTransactionHashes);

    std::vector<Crypto::FHash> poolTransactionHashes;
    m_mempool.getTransactionIdsByPaymentId(paymentId, poolTransactionHashes);

    std::list<FTransaction> txs;
    std::list<Crypto::FHash> missed_txs;

    if (!poolTransactionHashes.empty()) {
        blockchainTransactionHashes.insert(blockchainTransactionHashes.end(),
                                           poolTransactionHashes.begin(),
                                           poolTransactionHashes.end());
    }

    if (blockchainTransactionHashes.empty()) {
        return false;
    }

    getTransactions(blockchainTransactionHashes, txs, missed_txs, true);
    if (!missed_txs.empty()) {
        return false;
    }

    transactions.insert(transactions.end(), txs.begin(), txs.end());

    return true;
}

std::vector<Crypto::FHash> core::getTransactionHashesByPaymentId(const Crypto::FHash &paymentId)
{
    logger(DEBUGGING) << "getTransactionHashesByPaymentId request with paymentId " << paymentId;

    std::vector<Crypto::FHash> blockchainTransactionHashes;
    m_blockchain.getTransactionIdsByPaymentId(paymentId, blockchainTransactionHashes);

    std::vector<Crypto::FHash> poolTransactionHashes;
    m_mempool.getTransactionIdsByPaymentId(paymentId, poolTransactionHashes);

    blockchainTransactionHashes.reserve(blockchainTransactionHashes.size()
                                        + poolTransactionHashes.size());
    std::move(poolTransactionHashes.begin(), poolTransactionHashes.end(),
              std::back_inserter(blockchainTransactionHashes));

    return blockchainTransactionHashes;
}

uint64_t core::getMinimalFee()
{
    return getMinimalFeeForHeight(getCurrentBlockchainHeight() - 1);
}

uint64_t core::getBlockTimestamp(uint32_t height)
{
    return m_blockchain.getBlockTimestamp(height);
}

uint64_t core::getMinimalFeeForHeight(uint32_t height)
{
    return m_blockchain.getMinimalFee(height);
}

std::error_code core::executeLocked(const std::function<std::error_code()> &func)
{
    std::lock_guard<decltype(m_mempool)> lk(m_mempool);
    LockedBlockchainStorage lbs(m_blockchain);

    return func();
}

uint64_t core::getNextBlockDifficulty(uint64_t nextBlockTime)
{
    return m_blockchain.getDifficultyForNextBlock(nextBlockTime);
}

uint64_t core::getTotalGeneratedAmount()
{
    return m_blockchain.getCoinsInCirculation();
}

uint8_t core::getBlockMajorVersionForHeight(uint32_t height) const
{
    return m_blockchain.getBlockMajorVersionForHeight(height);
}

bool core::getPaymentId(const FTransaction &transaction, Crypto::FHash &paymentId)
{
    std::vector<TransactionExtraField> txExtraFields;
    parseTransactionExtra(transaction.vExtra, txExtraFields);
    TransactionExtraNonce extraNonce;
    if (!findTransactionExtraFieldByType(txExtraFields, extraNonce)) {
        return false;
    }
    return getPaymentIdFromTransactionExtraNonce(extraNonce.nonce, paymentId);
}

void core::setBlocksToFind(uint64_t blocksToFind)
{
    m_blocksFound = 0;
    m_blocksToFind = blocksToFind;
}

bool core::handleIncomingTransaction(const FTransaction &tx, const Crypto::FHash &txHash,
									 size_t blobSize, tx_verification_context &tvc,
									 bool keptByBlock, uint32_t height, bool loose_check)
{
    if (!check_tx_syntax(tx)) {
        logger(INFO) << "WRONG TRANSACTION BLOB, Failed to check tx " << txHash
                     << " syntax, rejected";
        tvc.m_verification_failed = true;
        return false;
    }

    // is in checkpoint zone
    if (!m_blockchain.isInCheckpointZone(getCurrentBlockchainHeight())) {
        if (blobSize > m_currency.maxTransactionSizeLimit()
            && getCurrentBlockMajorVersion() >= BLOCK_MAJOR_VERSION_3) {
            logger(INFO) << "Transaction verification failed: too big size " << blobSize
                         << " of transaction " << txHash << ", rejected";
            tvc.m_verification_failed = true;
            return false;
        }

        if (!check_tx_fee(tx, blobSize, tvc, height, loose_check)) {
            tvc.m_verification_failed = true;
            return false;
        }

        if (!check_tx_mixin(tx, height)) {
            logger(INFO) << "Transaction verification failed: mixin count for transaction "
                         << txHash << " is too large, rejected";
            tvc.m_verification_failed = true;
            return false;
        }

        if (!check_tx_unmixable(tx, height)) {
            logger(ERROR) << "Transaction verification failed: unmixable output for transaction "
                          << txHash << ", rejected";
            tvc.m_verification_failed = true;
            return false;
        }
    }

    if (!check_tx_semantic(tx, keptByBlock)) {
        logger(INFO) << "WRONG TRANSACTION BLOB, Failed to check tx " << txHash
                     << " semantic, rejected";
        tvc.m_verification_failed = true;
        return false;
    }

    bool r = add_new_tx(tx, txHash, blobSize, tvc, keptByBlock);
    if (tvc.m_verification_failed) {
        if (!tvc.m_tx_fee_too_small) {
            logger(ERROR) << "Transaction verification failed: " << txHash;
        } else {
            logger(INFO) << "Transaction verification failed: " << txHash;
        }
    } else if (tvc.m_verifivation_impossible) {
        logger(ERROR) << "Transaction verification impossible: " << txHash;
    }

    if (tvc.m_added_to_pool) {
        logger(DEBUGGING) << "tx added: " << txHash;
        poolUpdated();
    }

    return r;
}

std::unique_ptr<IBlock> core::getBlock(const Crypto::FHash &blockId)
{
    std::lock_guard<decltype(m_mempool)> lk(m_mempool);
    LockedBlockchainStorage lbs(m_blockchain);

    std::unique_ptr<BlockWithTransactions> blockPtr(new BlockWithTransactions());
    if (!lbs->getBlockByHash(blockId, blockPtr->block)) {
        logger(DEBUGGING) << "Can't find block: " << blockId;
        return std::unique_ptr<BlockWithTransactions>(nullptr);
    }

    blockPtr->transactions.reserve(blockPtr->block.vTransactionHashes.size());
    std::vector<Crypto::FHash> missedTxs;
    lbs->getTransactions(blockPtr->block.vTransactionHashes, blockPtr->transactions, missedTxs,
						 true);

    // if can't find transaction for blockchain block -> error
    assert(missedTxs.empty() || !lbs->isBlockInMainChain(blockId));

    if (!missedTxs.empty()) {
        logger(DEBUGGING) << "Can't find transactions for block: " << blockId;
        return std::unique_ptr<BlockWithTransactions>(nullptr);
    }

    return std::move(blockPtr);
}

bool core::getMixin(const FTransaction &transaction, uint64_t &mixin)
{
    mixin = 0;
    for (const FTransactionInput &txin : transaction.vInputs) {
        if (txin.type() != typeid(FKeyInput)) {
            continue;
        }
        uint64_t currentMixin = boost::get<FKeyInput>(txin).vOutputIndexes.size();
        if (currentMixin > mixin) {
            mixin = currentMixin;
        }
    }
    return true;
}

bool core::is_key_image_spent(const Crypto::FKeyImage &key_im)
{
    return m_blockchain.have_tx_keyimg_as_spent(key_im);
}

bool core::addMessageQueue(MessageQueue<BlockchainMessage> &messageQueue)
{
    return m_blockchain.addMessageQueue(messageQueue);
}

bool core::removeMessageQueue(MessageQueue<BlockchainMessage> &messageQueue)
{
    return m_blockchain.removeMessageQueue(messageQueue);
}

void core::rollbackBlockchain(uint32_t height)
{
    logger(INFO, BRIGHT_YELLOW) << "Rewinding blockchain to height: " << height;
    m_blockchain.rollbackBlockchainTo(height);
}

} // namespace QwertyNote
