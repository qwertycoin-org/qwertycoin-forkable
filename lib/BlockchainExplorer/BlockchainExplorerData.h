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
#include "CryptoTypes.h"
#include "CryptoNote.h"

namespace CryptoNote {

enum class TransactionRemoveReason : uint8_t
{
    INCLUDED_IN_BLOCK = 0,
    TIMEOUT = 1
};

struct TransactionOutputToKeyDetails
{
    Crypto::PublicKey txOutKey;
};

struct TransactionOutputMultiSignatureDetails {
    std::vector<Crypto::PublicKey> keys;
    uint32_t requiredSignatures;
};

struct TransactionOutputReferenceDetails
{
    Crypto::Hash transactionHash;
    size_t number;
};

struct TransactionInputGenerateDetails
{
    uint32_t height;
};

struct TransactionInputToKeyDetails
{
    Crypto::KeyImage keyImage;
    uint64_t mixin;
    std::vector<TransactionOutputReferenceDetails> outputs;
    std::vector<uint32_t> outputIndexes;
};

struct TransactionInputMultiSignatureDetails {
    uint32_t signatures;
    TransactionOutputReferenceDetails output;
};

struct BaseInputDetails
{
    BaseInput input;
    uint64_t amount;
};

struct KeyInputDetails
{
    KeyInput input;
    uint64_t mixin;
    std::vector<TransactionOutputReferenceDetails> outputs;
};

struct MultiSignatureInputDetails {
    MultiSignatureInput input;
    TransactionOutputReferenceDetails output;
};

typedef boost::variant<
    BaseInputDetails,
    KeyInputDetails, MultiSignatureInputDetails>
        TransactionInputDetails;

struct TransactionOutputDetails {
    TransactionOutput output;
    uint64_t globalIndex;
};

struct TransactionExtraDetails
{
    std::vector<size_t> padding;
    std::vector<Crypto::PublicKey> publicKey;
    BinaryArray nonce;
    BinaryArray raw;
    size_t size = 0;
};

struct TransactionDetails
{
    Crypto::Hash hash;
    uint64_t size = 0;
    uint64_t fee = 0;
    uint64_t totalInputsAmount = 0;
    uint64_t totalOutputsAmount = 0;
    uint64_t mixin = 0;
    uint64_t unlockTime = 0;
    uint64_t timestamp = 0;
    uint8_t version = 0;
    Crypto::Hash paymentId;
    bool hasPaymentId = false;
    bool inBlockchain = false;
    Crypto::Hash blockHash;
    uint32_t blockHeight = 0;
    TransactionExtraDetails extra;
    std::vector<std::vector<Crypto::Signature>> signatures;
    std::vector<TransactionInputDetails> inputs;
    std::vector<TransactionOutputDetails> outputs;
};

struct BlockDetails
{
    uint8_t majorVersion = 0;
    uint8_t minorVersion = 0;
    uint64_t timestamp = 0;
    Crypto::Hash prevBlockHash;
    Crypto::Hash proofOfWork;
    uint32_t nonce = 0;
    bool isOrphaned = false;
    uint32_t height = 0;
    uint32_t depth = 0;
    Crypto::Hash hash;
    uint64_t difficulty = 0;
    uint64_t cumulativeDifficulty = 0;
    uint64_t reward = 0;
    uint64_t baseReward = 0;
    uint64_t blockSize = 0;
    uint64_t transactionsCumulativeSize = 0;
    uint64_t alreadyGeneratedCoins = 0;
    uint64_t alreadyGeneratedTransactions = 0;
    uint64_t sizeMedian = 0;
    uint64_t effectiveSizeMedian = 0;
    double penalty = 0.0;
    uint64_t totalFeeAmount = 0;
    std::vector<TransactionDetails> transactions;
};

} // namespace CryptoNote
