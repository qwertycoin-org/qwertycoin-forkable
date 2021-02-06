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

#include <vector>

#include <boost/variant.hpp>

#include <Android.h>
#include <CryptoTypes.h>

namespace QwertyNote {

struct BaseInput
{
  uint32_t blockIndex;
};

struct KeyInput
{
    uint64_t amount;
    std::vector<uint32_t> outputIndexes;
    Crypto::FKeyImage keyImage;
};

struct MultiSignatureInput
{
    uint64_t amount;
    uint8_t signatureCount;
    uint32_t outputIndex;
};

struct KeyOutput
{
    Crypto::FPublicKey key;
};

struct MultiSignatureOutput
{
    std::vector<Crypto::FPublicKey> keys;
    uint8_t requiredSignatureCount;
};

typedef boost::variant<BaseInput, KeyInput, MultiSignatureInput> TransactionInput;

typedef boost::variant<KeyOutput, MultiSignatureOutput> TransactionOutputTarget;

struct TransactionOutput
{
    uint64_t amount;
    TransactionOutputTarget target;
};

struct TransactionPrefix
{
    uint8_t version;
    uint64_t unlockTime;
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
    std::vector<uint8_t> extra;
};

struct Transaction : public TransactionPrefix
{
    std::vector<std::vector<Crypto::FSignature>> signatures;
};

struct ParentBlock
{
    uint8_t majorVersion;
    uint8_t minorVersion;
    Crypto::FHash previousBlockHash;
    uint16_t transactionCount;
    std::vector<Crypto::FHash> baseTransactionBranch;
    Transaction baseTransaction;
    std::vector<Crypto::FHash> blockchainBranch;
};

struct BlockHeader
{
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint32_t nonce;
    uint64_t timestamp;
    Crypto::FHash previousBlockHash;
};

struct Block : public BlockHeader
{
    ParentBlock parentBlock;
    Transaction baseTransaction;
    std::vector<Crypto::FHash> transactionHashes;
};

struct AccountPublicAddress
{
    Crypto::FPublicKey spendPublicKey;
    Crypto::FPublicKey viewPublicKey;
};

struct AccountKeys
{
    AccountPublicAddress address;
    Crypto::FSecretKey spendSecretKey;
    Crypto::FSecretKey viewSecretKey;
};

struct KeyPair
{
    Crypto::FPublicKey publicKey;
    Crypto::FSecretKey secretKey;
};

using BinaryArray = std::vector<uint8_t>;

} // namespace QwertyNote
