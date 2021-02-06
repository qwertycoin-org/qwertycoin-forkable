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
#include <cstdint>
#include <string>
#include <vector>
#include <QwertyNote.h>

namespace QwertyNote {

namespace TransactionTypes {

enum class InputType : uint8_t
{
    Invalid,
    Key,
    Multisignature,
    Generating
};

enum class OutputType : uint8_t
{
    Invalid,
    Key,
    Multisignature
};

struct GlobalOutput
{
    Crypto::FPublicKey targetKey;
    uint32_t outputIndex;
};

typedef std::vector<GlobalOutput> GlobalOutputsContainer;

struct OutputKeyInfo
{
    Crypto::FPublicKey transactionPublicKey;
    size_t transactionIndex;
    size_t outputInTransaction;
};

struct InputKeyInfo
{
    uint64_t amount;
    GlobalOutputsContainer outputs;
    OutputKeyInfo realOutput;
};

} // namespace TransactionTypes

class ITransactionReader
{
public:
    virtual ~ITransactionReader() = default;

    virtual Crypto::FHash getTransactionHash() const = 0;
    virtual Crypto::FHash getTransactionPrefixHash() const = 0;
    virtual Crypto::FPublicKey getTransactionPublicKey() const = 0;
    virtual bool getTransactionSecretKey(Crypto::FSecretKey &key) const = 0;
    virtual uint64_t getUnlockTime() const = 0;

    // extra
    virtual bool getPaymentId(Crypto::FHash &paymentId) const = 0;
    virtual bool getExtraNonce(BinaryArray &nonce) const = 0;
    virtual BinaryArray getExtra() const = 0;

    // inputs
    virtual size_t getInputCount() const = 0;
    virtual uint64_t getInputTotalAmount() const = 0;
    virtual TransactionTypes::InputType getInputType(size_t index) const = 0;
    virtual void getInput(size_t index, KeyInput &input) const = 0;
    virtual void getInput(size_t index, MultiSignatureInput &input) const = 0;

    // outputs
    virtual size_t getOutputCount() const = 0;
    virtual uint64_t getOutputTotalAmount() const = 0;
    virtual TransactionTypes::OutputType getOutputType(size_t index) const = 0;
    virtual void getOutput(size_t index, KeyOutput &output, uint64_t &amount) const = 0;
    virtual void getOutput(size_t index, MultiSignatureOutput &output, uint64_t &amount) const = 0;

    // signatures
    virtual size_t getRequiredSignaturesCount(size_t inputIndex) const = 0;
    virtual bool findOutputsToAccount(
        const AccountPublicAddress &addr,
        const Crypto::FSecretKey &viewSecretKey,
        std::vector<uint32_t> &outs,
        uint64_t &outputAmount) const = 0;

    // various checks
    virtual bool validateInputs() const = 0;
    virtual bool validateOutputs() const = 0;
    virtual bool validateSignatures() const = 0;

    // serialized transaction
    virtual BinaryArray getTransactionData() const = 0;
};

class ITransactionWriter
{
public:
    virtual ~ITransactionWriter() = default;

    // transaction parameters
    virtual void setUnlockTime(uint64_t unlockTime) = 0;

    // extra
    virtual void setPaymentId(const Crypto::FHash &paymentId) = 0;
    virtual void setExtraNonce(const BinaryArray &nonce) = 0;
    virtual void appendExtra(const BinaryArray &extraData) = 0;

    // Inputs/Outputs
    virtual size_t addInput(const KeyInput &input) = 0;
    virtual size_t addInput(const MultiSignatureInput &input) = 0;
    virtual size_t addInput(
        const AccountKeys &senderKeys,
        const TransactionTypes::InputKeyInfo &info,
        KeyPair &ephKeys) = 0;

    virtual size_t addOutput(uint64_t amount, const AccountPublicAddress &to) = 0;
    virtual size_t addOutput(
        uint64_t amount,
        const std::vector<AccountPublicAddress> &to,
        uint32_t requiredSignatures) = 0;
    virtual size_t addOutput(uint64_t amount, const KeyOutput &out) = 0;
    virtual size_t addOutput(uint64_t amount, const MultiSignatureOutput &out) = 0;

    // transaction info
    virtual void setTransactionSecretKey(const Crypto::FSecretKey &key) = 0;

    // signing
    virtual void signInputKey(
        size_t input,
        const TransactionTypes::InputKeyInfo &info,
        const KeyPair &ephKeys) = 0;
    virtual void signInputMultisignature(
        size_t input,
        const Crypto::FPublicKey &sourceTransactionKey,
        size_t outputIndex,
        const AccountKeys &accountKeys) = 0;
    virtual void signInputMultisignature(size_t input, const KeyPair &ephemeralKeys) = 0;
};

class ITransaction : public ITransactionReader, public ITransactionWriter
{
public:
    ~ITransaction() override = default;
};

} // namespace QwertyNote
