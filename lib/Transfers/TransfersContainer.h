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
#include <mutex>
#include <unordered_map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <ITransfersContainer.h>

#include <Crypto/Crypto.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>
#include <QwertyNoteCore/Currency.h>
#include <QwertyNoteCore/ITransaction.h>

#include <Logging/LoggerRef.h>

#include <Serialization/ISerializer.h>
#include <Serialization/SerializationOverloads.h>


namespace QwertyNote {

struct TransactionOutputInformationIn;

class SpentOutputDescriptor
{
public:
    SpentOutputDescriptor();
    explicit SpentOutputDescriptor(const TransactionOutputInformationIn &transactionInfo);
    explicit SpentOutputDescriptor(const Crypto::FKeyImage *keyImage);
    SpentOutputDescriptor(uint64_t amount, uint32_t globalOutputIndex);

    void assign(const Crypto::FKeyImage *keyImage);
    void assign(uint64_t amount, uint32_t globalOutputIndex);

    bool isValid() const;

    size_t hash() const;

    bool operator==(const SpentOutputDescriptor &other) const;

private:
    TransactionTypes::OutputType m_type;
    union {
        const Crypto::FKeyImage *m_keyImage;
        struct
        {
            uint64_t m_amount;
            uint32_t m_globalOutputIndex;
        };
    };
};

struct SpentOutputDescriptorHasher
{
    size_t operator()(const SpentOutputDescriptor &descriptor) const
    {
        return descriptor.hash();
    }
};

struct TransactionOutputInformationIn : public FTransactionOutputInformation
{
    Crypto::FKeyImage keyImage; // WARNING: Used only for TransactionTypes::OutputType::Key!
};

struct TransactionOutputInformationEx : public TransactionOutputInformationIn
{
    SpentOutputDescriptor getSpentOutputDescriptor() const { return SpentOutputDescriptor(*this); }
    const Crypto::FHash& getTransactionHash() const { return sTransactionHash; }

    void serialize(QwertyNote::ISerializer &s)
    {
        s(reinterpret_cast<uint8_t &>(sOutputType), "sOutputType");
        s(uAmount, "");
        serializeGlobalOutputIndex(s, uGlobalOutputIndex, "");
        s(uOutputInTransaction, "");
        s(sTransactionPublicKey, "");
        s(keyImage, "");
        s(unlockTime, "");
        serializeBlockHeight(s, blockHeight, "");
        s(transactionIndex, "");
        s(sTransactionHash, "");
        s(visible, "");

        if (sOutputType == TransactionTypes::OutputType::Key) {
            s(sOutputKey, "");
        } else if (sOutputType == TransactionTypes::OutputType::Multisignature) {
            s(uRequiredSignatures, "");
        }
    }

    uint64_t unlockTime;
    uint32_t blockHeight;
    uint32_t transactionIndex;
    bool visible;

};

struct TransactionBlockInfo
{
    void serialize(ISerializer &s)
    {
        serializeBlockHeight(s, height, "height");
        s(timestamp, "timestamp");
        s(transactionIndex, "transactionIndex");
    }

    uint32_t height;
    uint64_t timestamp;
    uint32_t transactionIndex;
};

struct SpentTransactionOutput : TransactionOutputInformationEx
{
    const Crypto::FHash &getSpendingTransactionHash() const
    {
        return spendingTransactionHash;
    }

    void serialize(ISerializer &s)
    {
        TransactionOutputInformationEx::serialize(s);
        s(spendingBlock, "spendingBlock");
        s(spendingTransactionHash, "spendingTransactionHash");
        s(inputInTransaction, "inputInTransaction");
    }

    TransactionBlockInfo spendingBlock;
    Crypto::FHash spendingTransactionHash;
    uint32_t inputInTransaction;
};

class TransfersContainer : public ITransfersContainer
{
public:
    TransfersContainer(const QwertyNote::Currency &currency,
                       Logging::ILogger &logger,
                       size_t transactionSpendableAge,
                       size_t safeTransactionSpendableAge);

    bool addTransaction(const TransactionBlockInfo &block,
                        const ITransactionReader &tx,
                        const std::vector<TransactionOutputInformationIn> &transfers);
    bool deleteUnconfirmedTransaction(const Crypto::FHash& transactionHash);
    bool markTransactionConfirmed(const TransactionBlockInfo &block,
                                  const Crypto::FHash &transactionHash,
                                  const std::vector<uint32_t> &globalIndices);

    std::vector<Crypto::FHash> detach(uint32_t height);
    bool advanceHeight(uint32_t height);

    // ITransfersContainer
    size_t transfersCount() const override;
    size_t transactionsCount() const override;
    uint64_t balance(uint32_t flags) const override;
    void getOutputs(
        std::vector<FTransactionOutputInformation> &transfers,
        uint32_t flags) const override;
    bool getTransactionInformation(
			const Crypto::FHash &transactionHash,
			FTransactionInformation &info,
			uint64_t *amountIn = nullptr,
			uint64_t *amountOut = nullptr) const override;
    std::vector<FTransactionOutputInformation> getTransactionOutputs(
        const Crypto::FHash &transactionHash,
        uint32_t flags) const override;
    // only type flags are feasible for this function
    std::vector<FTransactionOutputInformation> getTransactionInputs(
        const Crypto::FHash &transactionHash,
        uint32_t flags) const override;
    void getUnconfirmedTransactions(std::vector<Crypto::FHash> &transactions) const override;
    std::vector<FTransactionSpentOutputInformation> getSpentOutputs() const override;
    void markTransactionSafe(const Crypto::FHash &transactionHash) override;
    void getSafeTransactions(std::vector<Crypto::FHash> &transactions) const override;

    // IStreamSerializable
    void save(std::ostream &os) override;
    void load(std::istream &in) override;

private:
    struct ContainingTransactionIndex { };
    struct SpendingTransactionIndex { };
    struct SpentOutputDescriptorIndex { };

    typedef boost::multi_index_container<
        FTransactionInformation,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                BOOST_MULTI_INDEX_MEMBER(FTransactionInformation, Crypto::FHash, sTransactionHash)
            >,
            boost::multi_index::ordered_non_unique<
                BOOST_MULTI_INDEX_MEMBER(FTransactionInformation, uint32_t, uBlockHeight)
            >
        >
    > TransactionMultiIndex;

    typedef boost::multi_index_container<
        TransactionOutputInformationEx,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<SpentOutputDescriptorIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    SpentOutputDescriptor,
                    &TransactionOutputInformationEx::getSpentOutputDescriptor
                >,
                SpentOutputDescriptorHasher
            >,
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<ContainingTransactionIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    const Crypto::FHash &,
                    &TransactionOutputInformationEx::getTransactionHash
                >
            >
        >
    > UnconfirmedTransfersMultiIndex;

    typedef boost::multi_index_container<
        TransactionOutputInformationEx,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<SpentOutputDescriptorIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    SpentOutputDescriptor,
                    &TransactionOutputInformationEx::getSpentOutputDescriptor
                >,
                SpentOutputDescriptorHasher
            >,
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<ContainingTransactionIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    const Crypto::FHash &,
                    &TransactionOutputInformationEx::getTransactionHash
                >
            >
        >
    > AvailableTransfersMultiIndex;

    typedef boost::multi_index_container<
        SpentTransactionOutput,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<SpentOutputDescriptorIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    SpentOutputDescriptor,
                    &TransactionOutputInformationEx::getSpentOutputDescriptor
                >,
                SpentOutputDescriptorHasher
            >,
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<ContainingTransactionIndex>,
                boost::multi_index::const_mem_fun<
                    TransactionOutputInformationEx,
                    const Crypto::FHash &,
                    &SpentTransactionOutput::getTransactionHash
                >
            >,
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<SpendingTransactionIndex>,
                boost::multi_index::const_mem_fun <
                    SpentTransactionOutput,
                    const Crypto::FHash &,
                    &SpentTransactionOutput::getSpendingTransactionHash
                >
            >
        >
    > SpentTransfersMultiIndex;

private:
    void addTransaction(const TransactionBlockInfo &block, const ITransactionReader &tx);
    bool addTransactionOutputs(const TransactionBlockInfo &block,
                               const ITransactionReader &tx,
                               const std::vector<TransactionOutputInformationIn> &transfers);
    bool addTransactionInputs(const TransactionBlockInfo &block, const ITransactionReader &tx);
    void deleteTransactionTransfers(const Crypto::FHash &transactionHash);
    bool isSpendTimeUnlocked(uint64_t unlockTime) const;
    bool isIncluded(const TransactionOutputInformationEx &info, uint32_t flags) const;
    static bool isIncluded(TransactionTypes::OutputType type, uint32_t state, uint32_t flags);
    void updateTransfersVisibility(const Crypto::FKeyImage &keyImage);

    void copyToSpent(const TransactionBlockInfo &block,
                     const ITransactionReader &tx,
                     size_t inputIndex,
                     const TransactionOutputInformationEx &output);
    void repair();

private:
    TransactionMultiIndex m_transactions;
    UnconfirmedTransfersMultiIndex m_unconfirmedTransfers;
    AvailableTransfersMultiIndex m_availableTransfers;
    SpentTransfersMultiIndex m_spentTransfers;

    mutable std::set<Crypto::FHash, Crypto::FHashCompare> m_safeTxes;

    uint32_t m_currentHeight; // current height is needed to check if a transfer is unlocked
    size_t m_transactionSpendableAge;
    size_t m_safeTransactionSpendableAge;
    const QwertyNote::Currency &m_currency;
    mutable std::mutex m_mutex;
    Logging::LoggerRef m_logger;
};

} // namespace QwertyNote
