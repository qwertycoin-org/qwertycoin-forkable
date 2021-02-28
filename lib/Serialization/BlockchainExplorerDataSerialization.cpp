// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, The Karbo developers
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

#include <stdexcept>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

#include <QwertyNoteCore/CryptoNoteSerialization.h>

#include <Serialization/BlockchainExplorerDataSerialization.h>
#include <Serialization/SerializationOverloads.h>

namespace QwertyNote {

enum class SerializationTag : uint8_t
{
    Base = 0xff,
    Key = 0x2,
    Multisignature = 0x3,
    Transaction = 0xcc,
    Block = 0xbb
};

namespace {

struct BinaryVariantTagGetter : boost::static_visitor<uint8_t>
{
    uint8_t operator()(const QwertyNote::FBaseInputDetails)
    {
        return static_cast<uint8_t>(SerializationTag::Base);
    }

    uint8_t operator()(const QwertyNote::FKeyInputDetails)
    {
        return static_cast<uint8_t>(SerializationTag::Key);
    }

    uint8_t operator()(const QwertyNote::FMultiSignatureInputDetails)
    {
        return static_cast<uint8_t>(SerializationTag::Multisignature);
    }
};

struct VariantSerializer : boost::static_visitor<>
{
    VariantSerializer(QwertyNote::ISerializer &serializer, const std::string &name)
        : s(serializer),
          name(name)
    {
    }

    template <typename T>
    void operator() (T &param)
    {
        s(param, name);
    }

    QwertyNote::ISerializer &s;
    const std::string name;
};

void getVariantValue(
        QwertyNote::ISerializer &serializer,
    uint8_t tag,
    boost::variant<FBaseInputDetails, FKeyInputDetails, FMultiSignatureInputDetails> in)
{
    switch (static_cast<SerializationTag>(tag)) {
    case SerializationTag::Base: {
        FBaseInputDetails v;
        serializer(v, "uData");
        in = v;
        break;
    }
    case SerializationTag::Key: {
        FKeyInputDetails v;
        serializer(v, "uData");
        in = v;
        break;
    }
    case SerializationTag::Multisignature: {
        FMultiSignatureInputDetails v;
        serializer(v, "uData");
        in = v;
        break;
    }
    default:
        throw std::runtime_error("Unknown variant tag");
    }
}

template <typename T>
bool serializePod(T &v, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializer.binary(&v, sizeof(v), name);
}

} // namespace

void serialize(FTransactionOutputDetails &output, ISerializer &serializer)
{
    serializer(output.sTransactionsOutput, "output");
    serializer(output.uGlobalIndex, "globalIndex");
}

void serialize(FTransactionOutputReferenceDetails &outputReference, ISerializer &serializer)
{
    serializePod(outputReference.sTransactionHash, "transactionHash", serializer);
    serializer(outputReference.uNumber, "number");
}

void serialize(FBaseInputDetails &inputBase, ISerializer &serializer)
{
    serializer(inputBase.sBaseInput, "input");
    serializer(inputBase.uAmount, "amount");
}

void serialize(FKeyInputDetails &inputToKey, ISerializer &serializer)
{
    serializer(inputToKey.sKeyInput, "input");
    serializer(inputToKey.uMixin, "mixin");
    serializer(inputToKey.vKeyOutputs, "outputs");
}

void serialize(FMultiSignatureInputDetails &inputMultisig, ISerializer &serializer)
{
    serializer(inputMultisig.sMultiSignatureInput, "input");
    serializer(inputMultisig.sTransactionOutputReference, "output");
}

void serialize(FTransactionInputDetails &input, ISerializer &serializer)
{
    if (serializer.type() == ISerializer::OUTPUT) {
        BinaryVariantTagGetter tagGetter;
        uint8_t tag = boost::apply_visitor(tagGetter, input);
        serializer.binary(&tag, sizeof(tag), "type");

        VariantSerializer visitor(serializer, "uData");
        boost::apply_visitor(visitor, input);
    } else {
        uint8_t tag;
        serializer.binary(&tag, sizeof(tag), "type");

        getVariantValue(serializer, tag, input);
    }
}

void serialize(FTransactionExtraDetails &extra, ISerializer &serializer)
{
    serializePod(extra.vPublicKeys, "publicKey", serializer);
    serializer(extra.vNonce, "nonce");
    serializeAsBinary(extra.vRaw, "raw", serializer);
    serializer(extra.uSize, "size");
}

void serialize(FTransactionDetails &transaction, ISerializer &serializer)
{
    serializePod(transaction.sTransactionHash, "hash", serializer);
    serializer(transaction.uSize, "size");
    serializer(transaction.uFee, "fee");
    serializer(transaction.uTotalInputsAmount, "totalInputsAmount");
    serializer(transaction.uTotalOutputsAmount, "totalOutputsAmount");
    serializer(transaction.uMixin, "mixin");
    serializer(transaction.uUnlockTime, "unlockTime");
    serializer(transaction.uTimestamp, "timestamp");
    serializer(transaction.uVersion, "version");
    serializePod(transaction.sPaymentId, "paymentId", serializer);
    serializer(transaction.bInBlockchain, "inBlockchain");
    serializePod(transaction.sBlockHash, "blockHash", serializer);
    serializer(transaction.uBlockHeight, "blockIndex");
    serializer(transaction.sTransactionExtra, "extra");
    serializer(transaction.vTxInputDetails, "inputs");
    serializer(transaction.vTxOutputDetails, "outputs");

    if (serializer.type() == ISerializer::OUTPUT) {
        std::vector<std::pair<size_t, Crypto::FSignature>> signaturesForSerialization;
        signaturesForSerialization.reserve(transaction.vSignatures.size());
        size_t ctr = 0;
        for (const auto &signaturesV : transaction.vSignatures) {
            for (auto signature : signaturesV) {
                signaturesForSerialization.emplace_back(ctr, std::move(signature));
            }
            ++ctr;
        }
        size_t size = transaction.vSignatures.size();
        serializer(size, "signaturesSize");
        serializer(signaturesForSerialization, "signatures");
    } else {
        size_t size = 0;
        serializer(size, "signaturesSize");
        transaction.vSignatures.resize(size);

        std::vector<std::pair<size_t, Crypto::FSignature>> signaturesForSerialization;
        serializer(signaturesForSerialization, "signatures");

        for (const auto &signatureWithIndex : signaturesForSerialization) {
            transaction.vSignatures[signatureWithIndex.first].push_back(signatureWithIndex.second);
        }
    }
}

void serialize(FBlockDetails &block, ISerializer &serializer)
{
    serializer(block.uMajorVersion, "majorVersion");
    serializer(block.uMinorVersion, "minorVersion");
    serializer(block.uTimestamp, "timestamp");
    serializePod(block.sPrevBlockHash, "prevBlockHash", serializer);
    serializePod(block.sProofOfWork, "proofOfWork", serializer);
    serializer(block.uNonce, "nonce");
    serializer(block.bIsOrphaned, "isOrphaned");
    serializer(block.uHeight, "index");
    serializer(block.uDepth, "depth");
    serializePod(block.sBlockHash, "hash", serializer);
    serializer(block.uDifficulty, "difficulty");
    serializer(block.uCumulativeDifficulty, "cumulativeDifficulty");
    serializer(block.uReward, "reward");
    serializer(block.uBaseReward, "baseReward");
    serializer(block.uBlockSize, "blockSize");
    serializer(block.uTransactionsCumulativeSize, "transactionsCumulativeSize");
    serializer(block.uAlreadyGeneratedCoins, "alreadyGeneratedCoins");
    serializer(block.uAlreadyGeneratedTransactions, "alreadyGeneratedTransactions");
    serializer(block.uSizeMedian, "sizeMedian");
    serializer(block.uEffectiveSizeMedian, "effectiveSizeMedian");
    serializer(block.dPenalty, "penalty");
    serializer(block.uTotalFeeAmount, "totalFeeAmount");
    serializer(block.vTransactions, "transactions");
}

} // namespace QwertyNote
