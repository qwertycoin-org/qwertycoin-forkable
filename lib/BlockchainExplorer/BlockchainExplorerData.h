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

#include <array>
#include <string>
#include <vector>

#include <boost/variant.hpp>

#include <QwertyNote.h>
#include <CryptoTypes.h>

namespace QwertyNote {

    enum class ETransactionRemoveReason : uint8_t
    {
        IncludedInBlock = 0,
        Timeout = 1
    };

    struct FTransactionOutputReferenceDetails
    {
        uint64_t uNumber;
        Crypto::FHash sTransactionHash;
    };

    struct FBaseInputDetails
    {
        uint64_t uAmount;
        FBaseInput sBaseInput;
    };

    struct FKeyInputDetails
    {
        uint64_t uMixin;
        std::vector <FTransactionOutputReferenceDetails> vKeyOutputs;
        FKeyInput sKeyInput;
    };

    struct FMultiSignatureInputDetails
    {
        FMultiSignatureInput sMultiSignatureInput;
        FTransactionOutputReferenceDetails sTransactionOutputReference;
    };

    typedef boost::variant <
        FBaseInputDetails,
        FKeyInputDetails,
        FMultiSignatureInputDetails>
        FTransactionInputDetails;

    struct FTransactionOutputDetails
    {
        uint64_t uGlobalIndex;
        FTransactionOutput sTransactionsOutput;
    };

    struct FTransactionExtraDetails
    {
        uint32_t uSize = 0;
        std::vector <size_t> vPadding;
        std::vector <Crypto::FPublicKey> vPublicKeys;
        BinaryArray vNonce;
        BinaryArray vRaw;
    };

    struct FTransactionDetails
    {
        bool bHasPaymentId = false;
        bool bInBlockchain = false;
        uint8_t uVersion = 0;
        uint32_t uBlockHeight = 0;
        uint64_t uSize = 0;
        uint64_t uFee = 0;
        uint64_t uTotalInputsAmount = 0;
        uint64_t uTotalOutputsAmount = 0;
        uint64_t uMixin = 0;
        uint64_t uUnlockTime = 0;
        uint64_t uTimestamp = 0;
        std::vector <std::vector <Crypto::FSignature>> vSignatures;
        std::vector <FTransactionInputDetails> vTxInputDetails;
        std::vector <FTransactionOutputDetails> vTxOutputDetails;
        Crypto::FHash sBlockHash;
        Crypto::FHash sTransactionHash;
        Crypto::FHash sPaymentId;
        FTransactionExtraDetails sTransactionExtra;
    };

    struct FBlockDetails
    {
        bool bIsOrphaned = false;
        double dPenalty = 0.0;
        uint8_t uMajorVersion = 0;
        uint8_t uMinorVersion = 0;
        uint64_t uTimestamp = 0;
        uint32_t uNonce = 0;
        uint32_t uHeight = 0;
        uint32_t uDepth = 0;
        uint64_t uDifficulty = 0;
        uint64_t uCumulativeDifficulty = 0;
        uint64_t uReward = 0;
        uint64_t uBaseReward = 0;
        uint64_t uBlockSize = 0;
        uint64_t uTransactionsCumulativeSize = 0;
        uint64_t uAlreadyGeneratedCoins = 0;
        uint64_t uAlreadyGeneratedTransactions = 0;
        uint64_t uSizeMedian = 0;
        uint64_t uEffectiveSizeMedian = 0;
        uint64_t uTotalFeeAmount = 0;
        std::vector <FTransactionDetails> vTransactions;
        Crypto::FHash sPrevBlockHash;
        Crypto::FHash sBlockHash;
        Crypto::FHash sProofOfWork;
    };

} // namespace QwertyNote
