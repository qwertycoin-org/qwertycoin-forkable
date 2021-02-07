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

#include <boost/range/combine.hpp>
#include <boost/utility/value_init.hpp>

#include <BlockchainExplorer/BlockchainExplorerDataBuilder.h>

#include <Common/StringTools.h>

#include <Global/QwertyNoteConfig.h>

#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/TransactionExtra.h>

namespace QwertyNote {

BlockchainExplorerDataBuilder::BlockchainExplorerDataBuilder(ICore &core,
															 IQwertyNoteProtocolQuery &protocol)
    : mCore(core), mProtocol(protocol)
{
}

bool BlockchainExplorerDataBuilder::fillBlockDetails(const FBlock &block,
                                                     BlockDetails &blockDetails,
                                                     bool calculatePoW)
{
    Crypto::FHash hash = getBlockHash(block);

    blockDetails.majorVersion = block.uMajorVersion;
    blockDetails.minorVersion = block.uMinorVersion;
    blockDetails.timestamp = block.uTimestamp;
    blockDetails.prevBlockHash = block.sPreviousBlockHash;
    blockDetails.nonce = block.uNonce;
    blockDetails.hash = hash;

    blockDetails.reward = 0;
    for (const FTransactionOutput &out : block.sBaseTransaction.vOutputs) {
        blockDetails.reward += out.uAmount;
    }

    if (block.sBaseTransaction.vInputs.front().type() != typeid(FBaseInput)) {
        return false;
    }

    blockDetails.height = boost::get<FBaseInput>(block.sBaseTransaction.vInputs.front()).uBlockIndex;

    Crypto::FHash tmpHash = mCore.getBlockIdByHeight(blockDetails.height);
    blockDetails.isOrphaned = hash != tmpHash;

    blockDetails.proofOfWork = boost::value_initialized<Crypto::FHash>();
    if (calculatePoW) {
        Crypto::CnContext context;
        if (!getBlockLongHash(context, block, blockDetails.proofOfWork)) {
            return false;
        }
    }

    if (!m_core.getBlockDifficulty(blockDetails.height, blockDetails.difficulty)) {
        return false;
    }

    std::vector<size_t> blocksSizes;
    if (!m_core.getBackwardBlocksSizes(blockDetails.height, blocksSizes,
                                       parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW)) {
        return false;
    }
    blockDetails.sizeMedian = median(blocksSizes);

    size_t blockGrantedFullRewardZone =
            QwertyNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
    blockDetails.effectiveSizeMedian =
            std::max(blockDetails.sizeMedian, (uint64_t)blockGrantedFullRewardZone);

    size_t blockSize = 0;
    if (!m_core.getBlockSize(hash, blockSize)) {
        return false;
    }
    blockDetails.transactionsCumulativeSize = blockSize;

    size_t blockBlobSize = getObjectBinarySize(block);
    size_t minerTxBlobSize = getObjectBinarySize(block.sBaseTransaction);
    blockDetails.blockSize =
            blockBlobSize + blockDetails.transactionsCumulativeSize - minerTxBlobSize;

    if (!m_core.getAlreadyGeneratedCoins(hash, blockDetails.alreadyGeneratedCoins)) {
        return false;
    }

    if (!m_core.getGeneratedTransactionsNumber(blockDetails.height,
                                               blockDetails.alreadyGeneratedTransactions)) {
        return false;
    }

    uint64_t prevBlockGeneratedCoins = 0;
    uint32_t previousBlockHeight = 0;
    uint64_t blockTarget = QwertyNote::parameters::DIFFICULTY_TARGET;

    if (blockDetails.height > 0) {
        if (!m_core.getAlreadyGeneratedCoins(block.previousBlockHash, prevBlockGeneratedCoins)) {
            return false;
        }
    }

    if (blockDetails.height > QwertyNote::parameters::UPGRADE_HEIGHT_V1) {
        m_core.getBlockHeight(block.previousBlockHash, previousBlockHeight);
        blockTarget = block.timestamp - m_core.getBlockTimestamp(previousBlockHeight);
    }

    uint64_t maxReward = 0;
    uint64_t currentReward = 0;
    int64_t emissionChange = 0;
    if (!m_core.getBlockReward(block.majorVersion, blockDetails.sizeMedian, 0,
                               prevBlockGeneratedCoins, 0, maxReward, emissionChange,
                               blockDetails.height, blockTarget)) {
        return false;
    }

    if (!m_core.getBlockReward(block.majorVersion, blockDetails.sizeMedian,
                               blockDetails.transactionsCumulativeSize, prevBlockGeneratedCoins, 0,
                               currentReward, emissionChange, blockDetails.height, blockTarget)) {
        return false;
    }

    blockDetails.baseReward = maxReward;
    if (maxReward == 0 && currentReward == 0) {
        blockDetails.penalty = static_cast<double>(0);
    } else {
        if (maxReward < currentReward) {
            return false;
        }
        const uint64_t deltaReward = maxReward - currentReward;
        blockDetails.penalty = static_cast<double>(deltaReward) / static_cast<double>(maxReward);
    }

    blockDetails.transactions.reserve(block.vTransactionHashes.size() + 1);
    TransactionDetails transactionDetails;
    if (!fillTransactionDetails(block.sBaseTransaction, transactionDetails, block.uTimestamp)) {
        return false;
    }
    blockDetails.transactions.push_back(std::move(transactionDetails));

    std::list<FTransaction> found;
    std::list<Crypto::FHash> missed;
    m_core.getTransactions(block.transactionHashes, found, missed, blockDetails.isOrphaned);
    if (found.size() != block.vTransactionHashes.size()) {
        return false;
    }

    blockDetails.totalFeeAmount = 0;

    for (const FTransaction &tx : found) {
        TransactionDetails txDetails;
        if (!fillTransactionDetails(tx, txDetails, block.uTimestamp)) {
            return false;
        }
        blockDetails.totalFeeAmount += txDetails.fee;
        blockDetails.transactions.push_back(std::move(txDetails));
    }

    return true;
}

bool BlockchainExplorerDataBuilder::fillTransactionDetails(const FTransaction &transaction,
                                                           TransactionDetails &transactionDetails,
                                                           uint64_t timestamp)
{
    Crypto::FHash hash = getObjectHash(transaction);
    transactionDetails.hash = hash;
    transactionDetails.version = transaction.uVersion;
    transactionDetails.timestamp = timestamp;

    Crypto::FHash blockHash{};
    uint32_t blockHeight;

    if (!m_core.getBlockContainingTx(hash, blockHash, blockHeight)) {
        transactionDetails.inBlockchain = false;
        transactionDetails.blockHeight = boost::value_initialized<uint32_t>();
        transactionDetails.blockHash = boost::value_initialized<Crypto::FHash>();
    } else {
        transactionDetails.inBlockchain = true;
        transactionDetails.blockHeight = blockHeight;
        transactionDetails.blockHash = blockHash;
        if (timestamp == 0) {
            Block block;
            if (!m_core.getBlockByHash(blockHash, block)) {
                return false;
            }
            transactionDetails.timestamp = block.uTimestamp;
        }
    }

    transactionDetails.size = getObjectBinarySize(transaction);
    transactionDetails.unlockTime = transaction.uUnlockTime;
    transactionDetails.totalOutputsAmount = getOutsMoneyAmount(transaction);

    uint64_t inputsAmount;
    if (!getInputsMoneyAmount(transaction, inputsAmount)) {
        return false;
    }
    transactionDetails.totalInputsAmount = inputsAmount;

    if (!transaction.vInputs.empty() && transaction.vInputs.front().type() == typeid(FBaseInput)) {
        // it's gen transaction
        transactionDetails.fee = 0;
        transactionDetails.mixin = 0;
    } else {
        uint64_t fee;
        if (!getTxFee(transaction, fee)) {
            return false;
        }
        transactionDetails.fee = fee;
        uint64_t mixin;
        if (!getMixin(transaction, mixin)) {
            return false;
        }
        transactionDetails.mixin = mixin;
    }

    Crypto::FHash paymentId {};
    if (getPaymentId(transaction, paymentId)) {
        transactionDetails.paymentId = paymentId;
    } else {
        transactionDetails.paymentId = boost::value_initialized<Crypto::FHash>();
    }

    fillTxExtra(transaction.vExtra, transactionDetails.extra);

    transactionDetails.signatures.reserve(transaction.vSignatures.size());

    for (const std::vector<Crypto::FSignature> &signatures : transaction.vSignatures) {
        std::vector<Crypto::FSignature> signaturesDetails;
        signaturesDetails.reserve(signatures.size());
        for (const Crypto::FSignature &signature : signatures) {
            signaturesDetails.push_back(std::move(signature));
        }
        transactionDetails.signatures.push_back(std::move(signaturesDetails));
    }

    transactionDetails.inputs.reserve(transaction.vInputs.size());
    for (const FTransactionInput &txIn : transaction.vInputs) {
        TransactionInputDetails txInDetails;

        if (txIn.type() == typeid(FBaseInput)) {
            BaseInputDetails txInGenDetails {};
            txInGenDetails.input.uBlockIndex = boost::get<FBaseInput>(txIn).uBlockIndex;
            txInGenDetails.amount = 0;

            for (const FTransactionOutput &out : transaction.vOutputs) {
                txInGenDetails.amount += out.uAmount;
            }

            txInDetails = txInGenDetails;
        } else if (txIn.type() == typeid(FKeyInput)) {
            KeyInputDetails txInToKeyDetails;
            const FKeyInput &txInToKey = boost::get<FKeyInput>(txIn);
            txInToKeyDetails.input = txInToKey;
            std::list<std::pair<Crypto::FHash, size_t>> outputReferences;

            if (!m_core.scanOutputkeysForIndices(txInToKey, outputReferences)) {
                return false;
            }

            txInToKeyDetails.mixin = txInToKey.vOutputIndexes.size();

            for (const auto &r : outputReferences) {
                TransactionOutputReferenceDetails d {};
                d.number = r.second;
                d.transactionHash = r.first;
                txInToKeyDetails.outputs.push_back(d);
            }
            txInDetails = txInToKeyDetails;

        } else if (txIn.type() == typeid(FMultiSignatureInput)) {
            MultiSignatureInputDetails txInMultiSigDetails {};
            const FMultiSignatureInput &txInMultiSig = boost::get<FMultiSignatureInput>(txIn);
            txInMultiSigDetails.input = txInMultiSig;
            std::pair<Crypto::FHash, size_t> outputReference;

            if (!m_core.getMultisigOutputReference(txInMultiSig, outputReference)) {
                return false;
            }

            txInMultiSigDetails.output.number = outputReference.second;
            txInMultiSigDetails.output.transactionHash = outputReference.first;
            txInDetails = txInMultiSigDetails;
        } else {
            return false;
        }
        transactionDetails.inputs.push_back(std::move(txInDetails));
    }

    transactionDetails.outputs.reserve(transaction.vOutputs.size());
    std::vector<uint32_t> globalIndices;
    globalIndices.reserve(transaction.vOutputs.size());
    if (!transactionDetails.inBlockchain
        || !m_core.getTxOutputsGlobalIndexes(hash, globalIndices)) {
        for (size_t i = 0; i < transaction.vOutputs.size(); ++i) {
            globalIndices.push_back(0);
        }
    }

    typedef boost::tuple<FTransactionOutput, uint32_t> outputWithIndex;
    auto range = boost::combine(transaction.vOutputs, globalIndices);
    for (const outputWithIndex &txOutput : range) {
        TransactionOutputDetails txOutDetails;
        txOutDetails.globalIndex = txOutput.get<1>();
        txOutDetails.output.uAmount = txOutput.get<0>().uAmount;
        txOutDetails.output.sTarget = txOutput.get<0>().sTarget;
        transactionDetails.outputs.push_back(std::move(txOutDetails));
    }

    return true;
}

bool BlockchainExplorerDataBuilder::getPaymentId(const FTransaction &transaction,
                                                 Crypto::FHash &paymentId)
{
    std::vector<TransactionExtraField> txExtraFields;
    parseTransactionExtra(transaction.vExtra, txExtraFields);

    TransactionExtraNonce extraNonce;

    if (!findTransactionExtraFieldByType(txExtraFields, extraNonce)) {
        return false;
    }

    return getPaymentIdFromTransactionExtraNonce(extraNonce.nonce, paymentId);
}

bool BlockchainExplorerDataBuilder::getMixin(const FTransaction &transaction, uint64_t &mixin)
{
    mixin = 0;

    for (const FTransactionInput &txIn : transaction.vInputs) {
        if (txIn.type() != typeid(FKeyInput)) {
            continue;
        }

        uint64_t currentMixin = boost::get<FKeyInput>(txIn).vOutputIndexes.size();
        if (currentMixin > mixin) {
            mixin = currentMixin;
        }
    }

    return true;
}

bool BlockchainExplorerDataBuilder::fillTxExtra(const std::vector<uint8_t> &rawExtra,
                                                TransactionExtraDetails &extraDetails)
{
    extraDetails.raw = rawExtra;

    std::vector<TransactionExtraField> txExtraFields;
    parseTransactionExtra(rawExtra, txExtraFields);

    for (const TransactionExtraField &field : txExtraFields) {
        if (typeid(TransactionExtraPadding) == field.type()) {
            extraDetails.padding.push_back(
                    std::move(boost::get<TransactionExtraPadding>(field).size));
        } else if (typeid(TransactionExtraPublicKey) == field.type()) {
            extraDetails.publicKey.push_back(
                    std::move(boost::get<TransactionExtraPublicKey>(field).publicKey));
        } else if (typeid(TransactionExtraNonce) == field.type()) {
            extraDetails.nonce = boost::get<TransactionExtraNonce>(field).nonce;
        }
    }

    return true;
}

size_t BlockchainExplorerDataBuilder::median(std::vector<size_t> &v)
{
    if (v.empty()) {
        return boost::value_initialized<size_t>();
    }

    if (v.size() == 1) {
        return v[0];
    }

    size_t n = (v.size()) / 2;
    std::sort(v.begin(), v.end());
    // nth_element(v.begin(), v.begin()+n-1, v.end());
    if (v.size() % 2) { // 1, 3, 5...
        return v[n];
    } else { // 2, 4, 6...
        return (v[n - 1] + v[n]) / 2;
    }
}

} // namespace QwertyNote
