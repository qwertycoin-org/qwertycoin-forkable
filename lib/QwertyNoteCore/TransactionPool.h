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

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <boost/utility.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <CryptoTypes.h>

#include <Common/IntUtil.h>
#include <Common/ObserverManager.h>
#include <Common/Util.h>

#include <Crypto/Hash.h>

#include <Logging/LoggerRef.h>

#include <QwertyNoteCore/BlockchainIndices.h>
#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/CryptoNoteBasicImpl.h>
#include <QwertyNoteCore/Currency.h>
#include <QwertyNoteCore/ICore.h>
#include <QwertyNoteCore/ITimeProvider.h>
#include <QwertyNoteCore/ITransactionValidator.h>
#include <QwertyNoteCore/ITxPoolObserver.h>
#include <QwertyNoteCore/VerificationContext.h>

namespace QwertyNote {

class ISerializer;

class OnceInTimeInterval
{
public:
    OnceInTimeInterval(unsigned interval, QwertyNote::ITimeProvider &timeProvider)
        : m_interval(interval),
          m_timeProvider(timeProvider)
    {
        m_lastWorkedTime = 0;
    }

    template<class functor_t>
    bool call(functor_t functr)
    {
        time_t now = m_timeProvider.now();

        if (now - m_lastWorkedTime > m_interval) {
            bool res = functr();
            m_lastWorkedTime = m_timeProvider.now();
            return res;
        }

        return true;
    }

private:
    time_t m_lastWorkedTime;
    unsigned m_interval;
    QwertyNote::ITimeProvider &m_timeProvider;
};

using QwertyNote::BlockInfo;
using namespace boost::multi_index;

class tx_memory_pool: boost::noncopyable
{
public:
        struct TransactionCheckInfo
    {
        BlockInfo maxUsedBlock;
        BlockInfo lastFailedBlock;
    };

    struct TransactionDetails : public TransactionCheckInfo
    {
        Crypto::FHash id;
        FTransaction tx;
        size_t blobSize;
        uint64_t fee;
        bool keptByBlock;
        time_t receiveTime;
    };

private:
    struct TransactionPriorityComparator
    {
        // lhs > hrs
        bool operator()(const TransactionDetails &lhs, const TransactionDetails &rhs) const
        {
            // price(lhs) = lhs.fee / lhs.blobSize
            // price(lhs) > price(rhs) -->
            // lhs.fee / lhs.blobSize > rhs.fee / rhs.blobSize -->
            // lhs.fee * rhs.blobSize > rhs.fee * lhs.blobSize
            uint64_t lhs_hi, lhs_lo = mul128(lhs.fee, rhs.blobSize, &lhs_hi);
            uint64_t rhs_hi, rhs_lo = mul128(rhs.fee, lhs.blobSize, &rhs_hi);

            return
                // prefer more profitable transactions
                (lhs_hi >  rhs_hi) ||
                (lhs_hi == rhs_hi && lhs_lo >  rhs_lo) ||
                // prefer smaller
                (lhs_hi == rhs_hi && lhs_lo == rhs_lo && lhs.blobSize <  rhs.blobSize) ||
                // prefer older
                (
                    lhs_hi == rhs_hi && lhs_lo == rhs_lo
                    && lhs.blobSize == rhs.blobSize
                    && lhs.receiveTime < rhs.receiveTime
                );
        }
    };

    typedef hashed_unique<BOOST_MULTI_INDEX_MEMBER(TransactionDetails, Crypto::FHash, id)> main_index_t;
    typedef ordered_non_unique<identity<TransactionDetails>, TransactionPriorityComparator> fee_index_t;
    typedef multi_index_container<TransactionDetails, indexed_by<main_index_t, fee_index_t>> tx_container_t;
    typedef std::pair<uint64_t, uint64_t> GlobalOutput;
    typedef std::set<GlobalOutput> GlobalOutputsContainer;
    typedef std::unordered_map<Crypto::FKeyImage, std::unordered_set<Crypto::FHash> > key_images_container;

public:
    tx_memory_pool(
        const QwertyNote::Currency &currency,
                   QwertyNote::ITransactionValidator &validator, QwertyNote::ICore &core,
                   QwertyNote::ITimeProvider &timeProvider,
        Logging::ILogger &log,
        bool blockchainIndexesEnabled);

    bool addObserver(ITxPoolObserver *observer);
    bool removeObserver(ITxPoolObserver *observer);

    // load/store operations
    bool init(const std::string &config_folder);
    bool deinit();

    bool have_tx(const Crypto::FHash &id) const;
    bool add_tx(
        const FTransaction &tx,
        const Crypto::FHash &id,
        size_t blobSize,
        tx_verification_context &tvc,
        bool keeped_by_block);
    bool add_tx(const FTransaction &tx, tx_verification_context& tvc, bool keeped_by_block);
    bool take_tx(const Crypto::FHash &id, FTransaction &tx, size_t &blobSize, uint64_t &fee);

    bool on_blockchain_inc(uint64_t new_block_height, const Crypto::FHash &top_block_id);
    bool on_blockchain_dec(uint64_t new_block_height, const Crypto::FHash &top_block_id);

    void lock() const;
    void unlock() const;
    std::unique_lock<std::recursive_mutex> obtainGuard() const;

    bool fill_block_template(
			FBlock &bl,
			size_t median_size,
			size_t maxCumulativeSize,
			uint64_t already_generated_coins,
			size_t &total_size,
			uint64_t &fee);

    void get_transactions(std::list<FTransaction> &txs) const;
    void get_difference(
        const std::vector<Crypto::FHash> &known_tx_ids,
        std::vector<Crypto::FHash> &new_tx_ids,
        std::vector<Crypto::FHash> &deleted_tx_ids) const;
    size_t get_transactions_count() const;
    std::string print_pool(bool short_format) const;

    void on_idle();

    bool getTransactionIdsByPaymentId(
        const Crypto::FHash &paymentId,
        std::vector<Crypto::FHash> &transactionIds);
    bool getTransactionIdsByTimestamp(
        uint64_t timestampBegin,
        uint64_t timestampEnd,
        uint32_t transactionsNumberLimit,
        std::vector<Crypto::FHash> &hashes,
        uint64_t &transactionsNumberWithinTimestamps);

    template<class T, class D, class S>
    void getTransactions(const T &txsIds, D &txs, S &missedTxs)
    {
        std::lock_guard<std::recursive_mutex> lock(m_transactions_lock);

        for (const auto &id : txsIds) {
            auto it = m_transactions.find(id);
            if (it == m_transactions.end()) {
                missedTxs.push_back(id);
            } else {
                txs.push_back(it->tx);
            }
        }
    }

    void serialize(ISerializer &s);

	void getMemoryPool(std::list<QwertyNote::tx_memory_pool::TransactionDetails> txs) const;
	std::list<QwertyNote::tx_memory_pool::TransactionDetails> getMemoryPool() const;

private:
    // double spending checking
    bool addTransactionInputs(const Crypto::FHash &id, const FTransaction &tx, bool keptByBlock);
    bool haveSpentInputs(const FTransaction &tx) const;
    bool removeTransactionInputs(const Crypto::FHash &id, const FTransaction &tx, bool keptByBlock);

    tx_container_t::iterator removeTransaction(tx_container_t::iterator i);
    bool removeExpiredTransactions();
    bool is_transaction_ready_to_go(const FTransaction &tx, TransactionCheckInfo &txd) const;

    void buildIndices();

    Tools::QObserverManager<ITxPoolObserver> m_observerManager;
    const QwertyNote::Currency &m_currency;
    QwertyNote::ICore &m_core;
    OnceInTimeInterval m_txCheckInterval;
    mutable std::recursive_mutex m_transactions_lock;
    key_images_container m_spent_key_images;
    GlobalOutputsContainer m_spentOutputs;

    std::string m_config_folder;
    QwertyNote::ITransactionValidator &m_validator;
    QwertyNote::ITimeProvider &m_timeProvider;

    tx_container_t m_transactions;
    tx_container_t::nth_index<1>::type &m_fee_index;
    std::unordered_map<Crypto::FHash, uint64_t> m_recentlyDeletedTransactions;

    Logging::LoggerRef logger;

    PaymentIdIndex m_paymentIdIndex;
    TimestampTransactionsIndex m_timestampIndex;
    std::unordered_map<Crypto::FHash, uint64_t> m_ttlIndex;
};

} // namespace QwertyNote


