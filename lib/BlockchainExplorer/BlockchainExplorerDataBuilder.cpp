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

    QBlockchainExplorerDataBuilder::QBlockchainExplorerDataBuilder (ICore &sCore,
                                                                    IQwertyNoteProtocolQuery &sProtocol)
        : mCore(sCore), mProtocol(sProtocol)
    {
    }

    bool QBlockchainExplorerDataBuilder::fillBlockDetails (const FBlock &sBlock,
                                                           FBlockDetails &sBlockDetails,
                                                           bool bCalculatePoW)
    {
        Crypto::FHash sHash = getBlockHash(sBlock);

        sBlockDetails.uMajorVersion = sBlock.uMajorVersion;
        sBlockDetails.uMinorVersion = sBlock.uMinorVersion;
        sBlockDetails.uTimestamp = sBlock.uTimestamp;
        sBlockDetails.sPrevBlockHash = sBlock.sPreviousBlockHash;
        sBlockDetails.uNonce = sBlock.uNonce;
        sBlockDetails.sBlockHash = sHash;

        sBlockDetails.uReward = 0;
        for (const FTransactionOutput &sOut : sBlock.sBaseTransaction.vOutputs) {
            sBlockDetails.uReward += sOut.uAmount;
        }

        if (sBlock.sBaseTransaction.vInputs.front().type() != typeid(FBaseInput)) {
            return false;
        }

        sBlockDetails.uHeight = boost::get <FBaseInput>(sBlock.sBaseTransaction.vInputs.front())
            .uBlockIndex;

        Crypto::FHash sTmpHash = mCore.getBlockIdByHeight(sBlockDetails.uHeight);
        sBlockDetails.bIsOrphaned = sHash != sTmpHash;

        sBlockDetails.sProofOfWork = boost::value_initialized <Crypto::FHash>();
        if (bCalculatePoW) {
            Crypto::CnContext sContext;
            if (!getBlockLongHash(sContext, sBlock, sBlockDetails.sProofOfWork)) {
                return false;
            }
        }

        if (!mCore.getBlockDifficulty(sBlockDetails.uHeight, sBlockDetails.uDifficulty)) {
            return false;
        }

        std::vector <size_t> sBlocksSizes;
        if (!mCore.getBackwardBlocksSizes(sBlockDetails.uHeight, sBlocksSizes,
                                          parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW)) {
            return false;
        }
        sBlockDetails.uSizeMedian = median(sBlocksSizes);

        size_t blockGrantedFullRewardZone =
            QwertyNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
        sBlockDetails.uEffectiveSizeMedian =
            std::max(sBlockDetails.uSizeMedian, (uint64_t) blockGrantedFullRewardZone);

        size_t sBlockSize = 0;
        if (!mCore.getBlockSize(sHash, sBlockSize)) {
            return false;
        }
        sBlockDetails.uTransactionsCumulativeSize = sBlockSize;

        size_t sBlockBlobSize = getObjectBinarySize(sBlock);
        size_t sMinerTxBlobSize = getObjectBinarySize(sBlock.sBaseTransaction);
        sBlockDetails.uBlockSize = sBlockBlobSize +
                                   sBlockDetails.uTransactionsCumulativeSize -
                                   sMinerTxBlobSize;

        if (!mCore.getAlreadyGeneratedCoins(sHash, sBlockDetails.uAlreadyGeneratedCoins)) {
            return false;
        }

        if (!mCore.getGeneratedTransactionsNumber(sBlockDetails.uHeight,
                                                  sBlockDetails.uAlreadyGeneratedTransactions)) {
            return false;
        }

        uint32_t uPreviousBlockHeight = 0;
        uint64_t uPrevBlockGeneratedCoins = 0;
        uint64_t uBlockTarget = QwertyNote::parameters::DIFFICULTY_TARGET;

        if (sBlockDetails.uHeight > 0) {
            if (!mCore.getAlreadyGeneratedCoins(sBlock.sPreviousBlockHash,
                                                uPrevBlockGeneratedCoins)) {
                return false;
            }
        }

        if (sBlockDetails.uHeight > QwertyNote::parameters::UPGRADE_HEIGHT_V1) {
            mCore.getBlockHeight(sBlock.sPreviousBlockHash, uPreviousBlockHeight);
            uBlockTarget = sBlock.uTimestamp - mCore.getBlockTimestamp(uPreviousBlockHeight);
        }

        int64_t iEmissionChange = 0;
        uint64_t uCurrentReward = 0;
        uint64_t uMaxReward = 0;

        if (!mCore.getBlockReward(sBlock.uMajorVersion,
                                  sBlockDetails.uSizeMedian,
                                  0,
                                  uPrevBlockGeneratedCoins,
                                  0,
                                  uMaxReward,
                                  iEmissionChange,
                                  sBlockDetails.uHeight,
                                  uBlockTarget)) {
            return false;
        }

        if (!mCore.getBlockReward(sBlock.uMajorVersion,
                                  sBlockDetails.uSizeMedian,
                                  sBlockDetails.uTransactionsCumulativeSize,
                                  uPrevBlockGeneratedCoins,
                                  0,
                                  uCurrentReward,
                                  iEmissionChange,
                                  sBlockDetails.uHeight,
                                  uBlockTarget)) {
            return false;
        }

        sBlockDetails.uBaseReward = uMaxReward;
        if (uMaxReward == 0 && uCurrentReward == 0) {
            sBlockDetails.dPenalty = static_cast<double>(0);
        } else {
            if (uMaxReward < uCurrentReward) {
                return false;
            }
            const uint64_t deltaReward = uMaxReward - uCurrentReward;
            sBlockDetails.dPenalty = static_cast<double>(deltaReward) /
                                     static_cast<double>(uMaxReward);
        }

        sBlockDetails.vTransactions.reserve(sBlock.vTransactionHashes.size() + 1);
        FTransactionDetails sTransactionDetails;
        if (!fillTransactionDetails(sBlock.sBaseTransaction,
                                    sTransactionDetails,
                                    sBlock.uTimestamp)) {
            return false;
        }
        sBlockDetails.vTransactions.push_back(std::move(sTransactionDetails));

        std::list <FTransaction> lFoundTxs;
        std::list <Crypto::FHash> lMissedTxs;
        mCore.getTransactions(sBlock.vTransactionHashes,
                              lFoundTxs,
                              lMissedTxs,
                              sBlockDetails.bIsOrphaned);

        if (lFoundTxs.size() != sBlock.vTransactionHashes.size()) {
            return false;
        }

        sBlockDetails.uTotalFeeAmount = 0;

        for (const FTransaction &sTx : lFoundTxs) {
            FTransactionDetails txDetails;

            if (!fillTransactionDetails(sTx, txDetails, sBlock.uTimestamp)) {
                return false;
            }

            sBlockDetails.uTotalFeeAmount += txDetails.uFee;
            sBlockDetails.vTransactions.push_back(std::move(txDetails));
        }

        return true;
    }

    bool QBlockchainExplorerDataBuilder::fillTransactionDetails (const FTransaction &sTransaction,
                                                                 FTransactionDetails &sTransactionDetails,
                                                                 uint64_t uTimestamp)
    {
        Crypto::FHash sHash = getObjectHash(sTransaction);
        sTransactionDetails.sTransactionHash = sHash;
        sTransactionDetails.uVersion = sTransaction.uVersion;
        sTransactionDetails.uTimestamp = uTimestamp;

        Crypto::FHash sBlockHash {};
        uint32_t uBlockHeight;

        if (!mCore.getBlockContainingTx(sHash, sBlockHash, uBlockHeight)) {
            sTransactionDetails.bInBlockchain = false;
            sTransactionDetails.uBlockHeight = boost::value_initialized <uint32_t>();
            sTransactionDetails.sBlockHash = boost::value_initialized <Crypto::FHash>();
        } else {
            sTransactionDetails.bInBlockchain = true;
            sTransactionDetails.uBlockHeight = uBlockHeight;
            sTransactionDetails.sBlockHash = sBlockHash;

            if (uTimestamp == 0) {
                FBlock block;

                if (!mCore.getBlockByHash(sBlockHash, block)) {
                    return false;
                }

                sTransactionDetails.uTimestamp = block.uTimestamp;
            }
        }

        sTransactionDetails.uSize = getObjectBinarySize(sTransaction);
        sTransactionDetails.uUnlockTime = sTransaction.uUnlockTime;
        sTransactionDetails.uTotalOutputsAmount = getOutsMoneyAmount(sTransaction);

        uint64_t uInputsAmount;

        if (!getInputsMoneyAmount(sTransaction, uInputsAmount)) {
            return false;
        }

        sTransactionDetails.uTotalInputsAmount = uInputsAmount;

        if (!sTransaction.vInputs.empty() &&
            sTransaction.vInputs.front().type() == typeid(FBaseInput)) {
            // it's gen sTransaction
            sTransactionDetails.uFee = 0;
            sTransactionDetails.uMixin = 0;
        } else {
            uint64_t uFee;
            if (!getTxFee(sTransaction, uFee)) {
                return false;
            }
            sTransactionDetails.uFee = uFee;
            uint64_t uMixin;

            if (!getMixin(sTransaction, uMixin)) {
                return false;
            }

            sTransactionDetails.uMixin = uMixin;
        }

        Crypto::FHash sPaymentId {};
        if (getPaymentId(sTransaction, sPaymentId)) {
            sTransactionDetails.sPaymentId = sPaymentId;
        } else {
            sTransactionDetails.sPaymentId = boost::value_initialized <Crypto::FHash>();
        }

        fillTxExtra(sTransaction.vExtra, sTransactionDetails.sTransactionExtra);

        sTransactionDetails.vSignatures.reserve(sTransaction.vSignatures.size());

        for (const std::vector <Crypto::FSignature> &vSignatures : sTransaction.vSignatures) {
            std::vector <Crypto::FSignature> signaturesDetails;
            signaturesDetails.reserve(vSignatures.size());

            for (const Crypto::FSignature &signature : vSignatures) {
                signaturesDetails.push_back(std::move(signature));
            }

            sTransactionDetails.vSignatures.push_back(std::move(signaturesDetails));
        }

        sTransactionDetails.vTxInputDetails.reserve(sTransaction.vInputs.size());

        for (const FTransactionInput &sTxIn : sTransaction.vInputs) {
            FTransactionInputDetails sTxInDetails;

            if (sTxIn.type() == typeid(FBaseInput)) {
                FBaseInputDetails sTxInGenDetails {};
                sTxInGenDetails.sBaseInput.uBlockIndex = boost::get <FBaseInput>(sTxIn).uBlockIndex;
                sTxInGenDetails.uAmount = 0;

                for (const FTransactionOutput &sOut : sTransaction.vOutputs) {
                    sTxInGenDetails.uAmount += sOut.uAmount;
                }

                sTxInDetails = sTxInGenDetails;
            } else if (sTxIn.type() == typeid(FKeyInput)) {
                FKeyInputDetails sTxInToKeyDetails;
                const FKeyInput &sTxInToKey = boost::get <FKeyInput>(sTxIn);
                sTxInToKeyDetails.sKeyInput = sTxInToKey;
                std::list <std::pair <Crypto::FHash, size_t>> lOutputReferences;

                if (!mCore.scanOutputkeysForIndices(sTxInToKey, lOutputReferences)) {
                    return false;
                }

                sTxInToKeyDetails.uMixin = sTxInToKey.vOutputIndexes.size();

                for (const auto &sOutputReference : lOutputReferences) {
                    FTransactionOutputReferenceDetails sTxOutputDetail {};
                    sTxOutputDetail.uNumber = sOutputReference.second;
                    sTxOutputDetail.sTransactionHash = sOutputReference.first;
                    sTxInToKeyDetails.vKeyOutputs.push_back(sTxOutputDetail);
                }
                sTxInDetails = sTxInToKeyDetails;

            } else if (sTxIn.type() == typeid(FMultiSignatureInput)) {
                FMultiSignatureInputDetails sTxInMultiSigDetails {};
                const FMultiSignatureInput &sTxInMultiSig = boost::get <FMultiSignatureInput>(
                    sTxIn);
                sTxInMultiSigDetails.sMultiSignatureInput = sTxInMultiSig;
                std::pair <Crypto::FHash, size_t> sOutputReference;

                if (!mCore.getMultisigOutputReference(sTxInMultiSig, sOutputReference)) {
                    return false;
                }

                sTxInMultiSigDetails.sTransactionOutputReference.uNumber = sOutputReference.second;
                sTxInMultiSigDetails.sTransactionOutputReference.sTransactionHash = sOutputReference
                    .first;
                sTxInDetails = sTxInMultiSigDetails;
            } else {
                return false;
            }

            sTransactionDetails.vTxInputDetails.push_back(std::move(sTxInDetails));
        }

        sTransactionDetails.vTxOutputDetails.reserve(sTransaction.vOutputs.size());
        std::vector <uint32_t> vGlobalIndices;
        vGlobalIndices.reserve(sTransaction.vOutputs.size());
        if (!sTransactionDetails.bInBlockchain
            || !mCore.getTxOutputsGlobalIndexes(sHash, vGlobalIndices)) {
            for (size_t i = 0; i < sTransaction.vOutputs.size(); ++i) {
                vGlobalIndices.push_back(0);
            }
        }

        typedef boost::tuple <FTransactionOutput, uint32_t> sOutputWithIndex;
        auto sRange = boost::combine(sTransaction.vOutputs, vGlobalIndices);
        for (const sOutputWithIndex &sTxOutput : sRange) {
            FTransactionOutputDetails sTxOutDetails;
            sTxOutDetails.uGlobalIndex = sTxOutput.get <1>();
            sTxOutDetails.sTransactionsOutput.uAmount = sTxOutput.get <0>().uAmount;
            sTxOutDetails.sTransactionsOutput.sTarget = sTxOutput.get <0>().sTarget;
            sTransactionDetails.vTxOutputDetails.push_back(std::move(sTxOutDetails));
        }

        return true;
    }

    bool QBlockchainExplorerDataBuilder::getPaymentId (const FTransaction &sTransaction,
                                                       Crypto::FHash &sPaymentId)
    {
        std::vector <TransactionExtraField> vTxExtraFields;
        parseTransactionExtra(sTransaction.vExtra, vTxExtraFields);

        TransactionExtraNonce sExtraNonce;

        if (!findTransactionExtraFieldByType(vTxExtraFields, sExtraNonce)) {
            return false;
        }

        return getPaymentIdFromTransactionExtraNonce(sExtraNonce.nonce, sPaymentId);
    }

    bool QBlockchainExplorerDataBuilder::getMixin (const FTransaction &sTransaction,
                                                   uint64_t &uMixin)
    {
        uMixin = 0;

        for (const FTransactionInput &sTxIn : sTransaction.vInputs) {
            if (sTxIn.type() != typeid(FKeyInput)) {
                continue;
            }

            uint64_t uCurrentMixin = boost::get <FKeyInput>(sTxIn).vOutputIndexes.size();
            if (uCurrentMixin > uMixin) {
                uMixin = uCurrentMixin;
            }
        }

        return true;
    }

    bool QBlockchainExplorerDataBuilder::fillTxExtra (const std::vector <uint8_t> &vRawExtra,
                                                      FTransactionExtraDetails &sExtraDetails)
    {
        sExtraDetails.vRaw = vRawExtra;

        std::vector <TransactionExtraField> vTxExtraFields;
        parseTransactionExtra(vRawExtra, vTxExtraFields);

        for (const TransactionExtraField &sField : vTxExtraFields) {
            if (typeid(TransactionExtraPadding) == sField.type()) {
                sExtraDetails.vPadding.push_back(
                    std::move(boost::get <TransactionExtraPadding>(sField).size));
            } else if (typeid(TransactionExtraPublicKey) == sField.type()) {
                sExtraDetails.vPublicKeys.push_back(
                    std::move(boost::get <TransactionExtraPublicKey>(sField).publicKey));
            } else if (typeid(TransactionExtraNonce) == sField.type()) {
                sExtraDetails.vNonce = boost::get <TransactionExtraNonce>(sField).nonce;
            }
        }

        return true;
    }

    size_t QBlockchainExplorerDataBuilder::median (std::vector <size_t> &vSizes)
    {
        if (vSizes.empty()) {
            return boost::value_initialized <size_t>();
        }

        if (vSizes.size() == 1) {
            return vSizes[0];
        }

        size_t n = (vSizes.size()) / 2;
        std::sort(vSizes.begin(), vSizes.end());
        // nth_element(vSizes.begin(), vSizes.begin()+n-1, vSizes.end());
        if (vSizes.size() % 2) { // 1, 3, 5...
            return vSizes[n];
        } else { // 2, 4, 6...
            return (vSizes[n - 1] + vSizes[n]) / 2;
        }
    }

} // namespace QwertyNote
