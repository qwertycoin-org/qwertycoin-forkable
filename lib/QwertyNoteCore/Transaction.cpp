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

#include <numeric>
#include <unordered_set>

#include <boost/optional.hpp>

#include <Global/Constants.h>
#include <Global/QwertyNoteConfig.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/ITransaction.h>
#include <QwertyNoteCore/TransactionApiExtra.h>
#include <QwertyNoteCore/TransactionUtils.h>

using namespace Crypto;
using namespace Qwertycoin;

namespace {

using namespace QwertyNote;

void derivePublicKey(
		const FAccountPublicAddress &to,
		const FSecretKey &txKey,
		size_t outputIndex,
		FPublicKey &ephemeralKey)
{
    FKeyDerivation derivation;
    generateKeyDerivation(to.sViewPublicKey, txKey, derivation);
    derivePublicKey(derivation, outputIndex, to.sSpendPublicKey, ephemeralKey);
}

} // namespace

namespace QwertyNote {

using namespace Crypto;

class TransactionImpl : public ITransaction
{
public:
    TransactionImpl();
    explicit TransactionImpl(const BinaryArray &txblob);
    explicit TransactionImpl(const QwertyNote::FTransaction &tx);

    // ITransactionReader
    FHash getTransactionHash() const override;
    FHash getTransactionPrefixHash() const override;
    FPublicKey getTransactionPublicKey() const override;
    uint64_t getUnlockTime() const override;
    bool getPaymentId(FHash &hash) const override;
    bool getExtraNonce(BinaryArray &nonce) const override;
    BinaryArray getExtra() const override;

    // inputs
    size_t getInputCount() const override;
    uint64_t getInputTotalAmount() const override;
    TransactionTypes::InputType getInputType(size_t index) const override;
    void getInput(size_t index, FKeyInput &input) const override;
    void getInput(size_t index, FMultiSignatureInput &input) const override;

    // outputs
    size_t getOutputCount() const override;
    uint64_t getOutputTotalAmount() const override;
    TransactionTypes::OutputType getOutputType(size_t index) const override;
    void getOutput(size_t index, FKeyOutput &output, uint64_t &amount) const override;
    void getOutput(size_t index, FMultiSignatureOutput &output, uint64_t &amount) const override;

    size_t getRequiredSignaturesCount(size_t index) const override;
    bool findOutputsToAccount(
        const FAccountPublicAddress &addr,
        const FSecretKey &viewSecretKey,
        std::vector<uint32_t> &outs,
        uint64_t &outputAmount) const override;

    // various checks
    bool validateInputs() const override;
    bool validateOutputs() const override;
    bool validateSignatures() const override;

    // get serialized transaction
    BinaryArray getTransactionData() const override;

    // ITransactionWriter
    void setUnlockTime(uint64_t unlockTime) override;
    void setPaymentId(const FHash& hash) override;
    void setExtraNonce(const BinaryArray &nonce) override;
    void appendExtra(const BinaryArray &extraData) override;

    // Inputs/Outputs
    size_t addInput(const FKeyInput &input) override;
    size_t addInput(const FMultiSignatureInput &input) override;
    size_t addInput(
			const FAccountKeys &senderKeys,
			const TransactionTypes::InputKeyInfo &info,
			FKeyPair &ephKeys) override;

    size_t addOutput(uint64_t amount, const FAccountPublicAddress &to) override;
    size_t addOutput(
        uint64_t amount,
        const std::vector<FAccountPublicAddress> &to,
        uint32_t requiredSignatures) override;
    size_t addOutput(uint64_t amount, const FKeyOutput &out) override;
    size_t addOutput(uint64_t amount, const FMultiSignatureOutput &out) override;

    void signInputKey(
        size_t input,
        const TransactionTypes::InputKeyInfo &info,
        const FKeyPair &ephKeys) override;
    void signInputMultisignature(
        size_t input,
        const FPublicKey &sourceTransactionKey,
        size_t outputIndex,
        const FAccountKeys &accountKeys) override;
    void signInputMultisignature(size_t input, const FKeyPair &ephemeralKeys) override;

    // secret key
    bool getTransactionSecretKey(FSecretKey &key) const override;
    void setTransactionSecretKey(const FSecretKey &key) override;

private:
    void invalidateHash();

    std::vector<FSignature> &getSignatures(size_t input);

    const FSecretKey &txSecretKey() const
    {
        if (!secretKey) {
            throw std::runtime_error("Operation requires transaction secret key");
        }
        return *secretKey;
    }

    void checkIfSigning() const
    {
        if (!transaction.vSignatures.empty()) {
            throw std::runtime_error(
                "Cannot perform requested operation, "
                "since it will invalidate transaction signatures"
            );
        }
    }

private:
    QwertyNote::FTransaction transaction;
    boost::optional<FSecretKey> secretKey;
    mutable boost::optional<FHash> transactionHash;
    TransactionExtra extra;
};

std::unique_ptr<ITransaction> createTransaction()
{
    return std::unique_ptr<ITransaction>(new TransactionImpl());
}

std::unique_ptr<ITransaction> createTransaction(const BinaryArray &transactionBlob)
{
    return std::unique_ptr<ITransaction>(new TransactionImpl(transactionBlob));
}

std::unique_ptr<ITransaction> createTransaction(const QwertyNote::FTransaction &tx)
{
    return std::unique_ptr<ITransaction>(new TransactionImpl(tx));
}

TransactionImpl::TransactionImpl()
{
    QwertyNote::FKeyPair txKeys(QwertyNote::generateKeyPair());

    TransactionExtraPublicKey pk = { txKeys.sPublicKey };
    extra.set(pk);

    transaction.uVersion = CURRENT_TRANSACTION_VERSION;
    transaction.uUnlockTime = 0;
    transaction.vExtra = extra.serialize();

    secretKey = txKeys.sSecretKey;
}

TransactionImpl::TransactionImpl(const BinaryArray &ba)
{
    if (!fromBinaryArray(transaction, ba)) {
        throw std::runtime_error("Invalid transaction uData");
    }

    extra.parse(transaction.vExtra);
    transactionHash = getBinaryArrayHash(ba); // avoid serialization if we already have blob
}

TransactionImpl::TransactionImpl(const QwertyNote::FTransaction &tx)
    : transaction(tx)
{
    extra.parse(transaction.vExtra);
}

void TransactionImpl::invalidateHash()
{
    if (transactionHash.is_initialized()) {
        transactionHash = decltype(transactionHash)();
    }
}

FHash TransactionImpl::getTransactionHash() const
{
    if (!transactionHash.is_initialized()) {
        transactionHash = getObjectHash(transaction);
    }

    return transactionHash.get();
}

FHash TransactionImpl::getTransactionPrefixHash() const
{
    return getObjectHash(*static_cast<const FTransactionPrefix *>(&transaction));
}

FPublicKey TransactionImpl::getTransactionPublicKey() const
{
    FPublicKey pk(NULL_PUBLIC_KEY);
    extra.getPublicKey(pk);
    return pk;
}

uint64_t TransactionImpl::getUnlockTime() const
{
    return transaction.uUnlockTime;
}

void TransactionImpl::setUnlockTime(uint64_t unlockTime)
{
    checkIfSigning();
    transaction.uUnlockTime = unlockTime;
    invalidateHash();
}

bool TransactionImpl::getTransactionSecretKey(FSecretKey &key) const
{
    if (!secretKey) {
        return false;
    }

    key = reinterpret_cast<const FSecretKey &>(secretKey.get());

    return true;
}

void TransactionImpl::setTransactionSecretKey(const FSecretKey &key)
{
    const auto &sk = reinterpret_cast<const FSecretKey &>(key);
    FPublicKey pk;
    FPublicKey txPubKey;

    secretKeyToPublicKey(sk, pk);
    extra.getPublicKey(txPubKey);

    if (txPubKey != pk) {
        throw std::runtime_error("Secret transaction key does not match public key");
    }

    secretKey = key;
}

size_t TransactionImpl::addInput(const FKeyInput &input)
{
    checkIfSigning();
    transaction.vInputs.emplace_back(input);
    invalidateHash();
    return transaction.vInputs.size() - 1;
}

size_t TransactionImpl::addInput(
		const FAccountKeys &senderKeys,
		const TransactionTypes::InputKeyInfo &info,
		FKeyPair &ephKeys)
{
    checkIfSigning();
    FKeyInput input;
    input.uAmount = info.amount;

    generate_key_image_helper(
        senderKeys,
        info.realOutput.transactionPublicKey,
        info.realOutput.outputInTransaction,
        ephKeys,
        input.sKeyImage
    );

    // fill outputs array and use relative offsets
    for (const auto &out : info.outputs) {
        input.vOutputIndexes.push_back(out.outputIndex);
    }

    input.vOutputIndexes = absolute_output_offsets_to_relative(input.vOutputIndexes);

    return addInput(input);
}

size_t TransactionImpl::addInput(const FMultiSignatureInput &input)
{
    checkIfSigning();
    transaction.vInputs.push_back(input);
    invalidateHash();
    return transaction.vInputs.size() - 1;
}

size_t TransactionImpl::addOutput(uint64_t amount, const FAccountPublicAddress &to)
{
    checkIfSigning();

    FKeyOutput outKey;
    derivePublicKey(to, txSecretKey(), transaction.vOutputs.size(), outKey.sPublicKey);
    FTransactionOutput out = {amount, outKey };
    transaction.vOutputs.emplace_back(out);
    invalidateHash();

    return transaction.vOutputs.size() - 1;
}

size_t TransactionImpl::addOutput(
    uint64_t amount,
    const std::vector<FAccountPublicAddress> &to,
    uint32_t requiredSignatures)
{
    checkIfSigning();

    const auto &txKey = txSecretKey();
    size_t outputIndex = transaction.vOutputs.size();
    FMultiSignatureOutput outMsig;
    outMsig.uRequiredSignatureCount = requiredSignatures;
    outMsig.vPublicKeys.resize(to.size());

    for (size_t i = 0; i < to.size(); ++i) {
        derivePublicKey(to[i], txKey, outputIndex, outMsig.vPublicKeys[i]);
    }

    FTransactionOutput out = {amount, outMsig };
    transaction.vOutputs.emplace_back(out);
    invalidateHash();

    return outputIndex;
}

size_t TransactionImpl::addOutput(uint64_t amount, const FKeyOutput &out)
{
    checkIfSigning();
    size_t outputIndex = transaction.vOutputs.size();
    FTransactionOutput realOut = {amount, out };
    transaction.vOutputs.emplace_back(realOut);
    invalidateHash();
    return outputIndex;
}

size_t TransactionImpl::addOutput(uint64_t amount, const FMultiSignatureOutput &out)
{
    checkIfSigning();
    size_t outputIndex = transaction.vOutputs.size();
    FTransactionOutput realOut = {amount, out };
    transaction.vOutputs.emplace_back(realOut);
    invalidateHash();
    return outputIndex;
}

void TransactionImpl::signInputKey(
    size_t index,
    const TransactionTypes::InputKeyInfo &info,
    const FKeyPair &ephKeys)
{
    const auto &input = boost::get<FKeyInput>(
        getInputChecked(transaction, index, TransactionTypes::InputType::Key)
    );
    FHash prefixHash = getTransactionPrefixHash();

    std::vector<FSignature> signatures;
    std::vector<const FPublicKey *> keysPtrs;

    for (const auto &o : info.outputs) {
        keysPtrs.push_back(reinterpret_cast<const FPublicKey *>(&o.targetKey));
    }

    signatures.resize(keysPtrs.size());

    generateRingSignature(reinterpret_cast<const FHash &>(prefixHash),
						  reinterpret_cast<const FKeyImage &>(input.sKeyImage), keysPtrs,
						  reinterpret_cast<const FSecretKey &>(ephKeys.sSecretKey),
						  info.realOutput.transactionIndex, signatures.data());

    getSignatures(index) = signatures;
    invalidateHash();
}

void TransactionImpl::signInputMultisignature(
    size_t index,
    const FPublicKey &sourceTransactionKey,
    size_t outputIndex,
    const FAccountKeys &accountKeys)
{
    FKeyDerivation derivation;
    FPublicKey ephemeralPublicKey;
    FSecretKey ephemeralSecretKey;

    generateKeyDerivation(reinterpret_cast<const FPublicKey &>(sourceTransactionKey),
                          reinterpret_cast<const FSecretKey &>(accountKeys.sViewSecretKey),
                          derivation);

    derivePublicKey(derivation, outputIndex,
                    reinterpret_cast<const FPublicKey &>(accountKeys.sAddress.sSpendPublicKey),
                    ephemeralPublicKey);
    deriveSecretKey(derivation, outputIndex,
                    reinterpret_cast<const FSecretKey &>(accountKeys.sSpendSecretKey),
                    ephemeralSecretKey);

    FSignature signature;
    auto txPrefixHash = getTransactionPrefixHash();

    generateSignature(reinterpret_cast<const FHash &>(txPrefixHash), ephemeralPublicKey,
					  ephemeralSecretKey, signature);

    getSignatures(index).push_back(signature);
    invalidateHash();
}

void TransactionImpl::signInputMultisignature(size_t index, const FKeyPair &ephemeralKeys)
{
    FSignature signature;
    auto txPrefixHash = getTransactionPrefixHash();

    generateSignature(txPrefixHash, ephemeralKeys.sPublicKey, ephemeralKeys.sSecretKey, signature);

    getSignatures(index).push_back(signature);
    invalidateHash();
}

std::vector<FSignature> &TransactionImpl::getSignatures(size_t input)
{
    // update signatures container size if needed
    if (transaction.vSignatures.size() < transaction.vInputs.size()) {
        transaction.vSignatures.resize(transaction.vInputs.size());
    }
    // check range
    if (input >= transaction.vSignatures.size()) {
        throw std::runtime_error("Invalid input index");
    }

    return transaction.vSignatures[input];
}

BinaryArray TransactionImpl::getTransactionData() const
{
    return toBinaryArray(transaction);
}

void TransactionImpl::setPaymentId(const FHash &hash)
{
    checkIfSigning();
    BinaryArray paymentIdBlob;
    setPaymentIdToTransactionExtraNonce(paymentIdBlob, reinterpret_cast<const FHash &>(hash));
    setExtraNonce(paymentIdBlob);
}

bool TransactionImpl::getPaymentId(FHash &hash) const
{
    BinaryArray nonce;
    if (getExtraNonce(nonce)) {
        FHash paymentId;
        if (getPaymentIdFromTransactionExtraNonce(nonce, paymentId)) {
            hash = reinterpret_cast<const FHash &>(paymentId);
            return true;
        }
    }
    return false;
}

void TransactionImpl::setExtraNonce(const BinaryArray &nonce)
{
    checkIfSigning();
    TransactionExtraNonce extraNonce = { nonce };
    extra.set(extraNonce);
    transaction.vExtra = extra.serialize();
    invalidateHash();
}

void TransactionImpl::appendExtra(const BinaryArray &extraData)
{
    checkIfSigning();
    transaction.vExtra.insert(transaction.vExtra.end(), extraData.begin(), extraData.end());
}

bool TransactionImpl::getExtraNonce(BinaryArray &nonce) const
{
    TransactionExtraNonce extraNonce;
    if (extra.get(extraNonce)) {
        nonce = extraNonce.nonce;
        return true;
    }

    return false;
}

BinaryArray TransactionImpl::getExtra() const
{
    return transaction.vExtra;
}

size_t TransactionImpl::getInputCount() const
{
    return transaction.vInputs.size();
}

uint64_t TransactionImpl::getInputTotalAmount() const
{
    return std::accumulate(
        transaction.vInputs.begin(),
        transaction.vInputs.end(),
        0ULL,
        [](uint64_t val, const FTransactionInput& in) {
            return val + getTransactionInputAmount(in);
        }
    );
}

TransactionTypes::InputType TransactionImpl::getInputType(size_t index) const
{
    return getTransactionInputType(getInputChecked(transaction, index));
}

void TransactionImpl::getInput(size_t index, FKeyInput &input) const
{
    input = boost::get<FKeyInput>(
        getInputChecked(transaction, index, TransactionTypes::InputType::Key)
    );
}

void TransactionImpl::getInput(size_t index, FMultiSignatureInput &input) const
{
    input = boost::get<FMultiSignatureInput>(
        getInputChecked(transaction, index, TransactionTypes::InputType::Multisignature)
    );
}

size_t TransactionImpl::getOutputCount() const
{
    return transaction.vOutputs.size();
}

uint64_t TransactionImpl::getOutputTotalAmount() const
{
    return std::accumulate(
        transaction.vOutputs.begin(),
        transaction.vOutputs.end(),
        0ULL,
        [](uint64_t val, const FTransactionOutput &out) {
            return val + out.uAmount;
        }
    );
}

TransactionTypes::OutputType TransactionImpl::getOutputType(size_t index) const
{
    return getTransactionOutputType(getOutputChecked(transaction, index).sTarget);
}

void TransactionImpl::getOutput(size_t index, FKeyOutput &output, uint64_t &amount) const
{
    const auto &out = getOutputChecked(transaction, index, TransactionTypes::OutputType::Key);
    output = boost::get<FKeyOutput>(out.sTarget);
    amount = out.uAmount;
}

void TransactionImpl::getOutput(size_t index, FMultiSignatureOutput & output, uint64_t& amount) const
{
    const auto &out = getOutputChecked(
        transaction,
        index,
        TransactionTypes::OutputType::Multisignature
    );
    output = boost::get<FMultiSignatureOutput>(out.sTarget);
    amount = out.uAmount;
}

bool TransactionImpl::findOutputsToAccount(
    const FAccountPublicAddress &addr,
    const FSecretKey &viewSecretKey,
    std::vector<uint32_t> &out,
    uint64_t &amount) const
{
    return ::QwertyNote::findOutputsToAccount(transaction, addr, viewSecretKey, out, amount);
}

size_t TransactionImpl::getRequiredSignaturesCount(size_t index) const
{
    return ::getRequiredSignaturesCount(getInputChecked(transaction, index));
}

bool TransactionImpl::validateInputs() const
{
    return
        check_inputs_types_supported(transaction)
        && check_inputs_overflow(transaction)
        && checkInputsKeyimagesDiff(transaction)
        && checkMultisignatureInputsDiff(transaction);
}

bool TransactionImpl::validateOutputs() const
{
    return check_outs_valid(transaction) && check_outs_overflow(transaction);
}

bool TransactionImpl::validateSignatures() const
{
    if (transaction.vSignatures.size() < transaction.vInputs.size()) {
        return false;
    }

    for (size_t i = 0; i < transaction.vInputs.size(); ++i) {
        if (getRequiredSignaturesCount(i) > transaction.vSignatures[i].size()) {
            return false;
        }
    }

    return true;
}

} // namespace QwertyNote
