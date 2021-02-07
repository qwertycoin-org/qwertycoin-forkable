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
#include <system_error>

#include <Global/Constants.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/ITransaction.h>
#include <QwertyNoteCore/TransactionApiExtra.h>
#include <QwertyNoteCore/TransactionUtils.h>

using namespace Crypto;
using namespace Qwertycoin;

namespace QwertyNote {

class TransactionPrefixImpl : public ITransactionReader
{
public:
    TransactionPrefixImpl();
    TransactionPrefixImpl(const FTransactionPrefix &prefix, const FHash &transactionHash);

    ~TransactionPrefixImpl() override = default;

    FHash getTransactionHash() const override;
    FHash getTransactionPrefixHash() const override;
    FPublicKey getTransactionPublicKey() const override;
    uint64_t getUnlockTime() const override;

    // extra
    bool getPaymentId(FHash &paymentId) const override;
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

    // signatures
    size_t getRequiredSignaturesCount(size_t inputIndex) const override;
    bool findOutputsToAccount(
        const FAccountPublicAddress &addr,
        const FSecretKey &viewSecretKey,
        std::vector<uint32_t> &outs,
        uint64_t &outputAmount) const override;

    // various checks
    bool validateInputs() const override;
    bool validateOutputs() const override;
    bool validateSignatures() const override;

    // serialized transaction
    BinaryArray getTransactionData() const override;

    bool getTransactionSecretKey(FSecretKey& key) const override;

private:
    FTransactionPrefix m_txPrefix;
    TransactionExtra m_extra;
    FHash m_txHash;
};

TransactionPrefixImpl::TransactionPrefixImpl()
{
}

TransactionPrefixImpl::TransactionPrefixImpl(
    const FTransactionPrefix &prefix,
    const FHash &transactionHash)
{
    m_extra.parse(prefix.vExtra);
    m_txPrefix = prefix;
    m_txHash = transactionHash;
}

FHash TransactionPrefixImpl::getTransactionHash() const
{
    return m_txHash;
}

FHash TransactionPrefixImpl::getTransactionPrefixHash() const
{
    return getObjectHash(m_txPrefix);
}

FPublicKey TransactionPrefixImpl::getTransactionPublicKey() const
{
    Crypto::FPublicKey pk(NULL_PUBLIC_KEY);
    m_extra.getPublicKey(pk);
    return pk;
}

uint64_t TransactionPrefixImpl::getUnlockTime() const
{
    return m_txPrefix.uUnlockTime;
}

bool TransactionPrefixImpl::getPaymentId(FHash &hash) const
{
    BinaryArray nonce;

    if (getExtraNonce(nonce)) {
        Crypto::FHash paymentId;
        if (getPaymentIdFromTransactionExtraNonce(nonce, paymentId)) {
            hash = reinterpret_cast<const FHash &>(paymentId);
            return true;
        }
    }

    return false;
}

bool TransactionPrefixImpl::getExtraNonce(BinaryArray &nonce) const
{
    TransactionExtraNonce extraNonce;

    if (m_extra.get(extraNonce)) {
        nonce = extraNonce.nonce;
        return true;
    }

    return false;
}

BinaryArray TransactionPrefixImpl::getExtra() const
{
    return m_txPrefix.vExtra;
}

size_t TransactionPrefixImpl::getInputCount() const
{
    return m_txPrefix.vInputs.size();
}

uint64_t TransactionPrefixImpl::getInputTotalAmount() const
{
    return std::accumulate(
        m_txPrefix.vInputs.begin(),
        m_txPrefix.vInputs.end(),
        0ULL,
        [](uint64_t val, const FTransactionInput &in) {
            return val + getTransactionInputAmount(in);
        }
    );
}

TransactionTypes::InputType TransactionPrefixImpl::getInputType(size_t index) const
{
    return getTransactionInputType(getInputChecked(m_txPrefix, index));
}

void TransactionPrefixImpl::getInput(size_t index, FKeyInput &input) const
{
    input = boost::get<FKeyInput>(
        getInputChecked(m_txPrefix, index, TransactionTypes::InputType::Key)
    );
}

void TransactionPrefixImpl::getInput(size_t index, FMultiSignatureInput &input) const
{
    input = boost::get<FMultiSignatureInput>(
        getInputChecked(m_txPrefix, index, TransactionTypes::InputType::Multisignature)
    );
}

size_t TransactionPrefixImpl::getOutputCount() const
{
    return m_txPrefix.vOutputs.size();
}

uint64_t TransactionPrefixImpl::getOutputTotalAmount() const
{
    return std::accumulate(
        m_txPrefix.vOutputs.begin(),
        m_txPrefix.vOutputs.end(),
        0ULL,
        [](uint64_t val, const FTransactionOutput &out) {
            return val + out.uAmount;
        }
    );
}

TransactionTypes::OutputType TransactionPrefixImpl::getOutputType(size_t index) const
{
    return getTransactionOutputType(getOutputChecked(m_txPrefix, index).sTarget);
}

void TransactionPrefixImpl::getOutput(size_t index, FKeyOutput &output, uint64_t &amount) const
{
    const auto &out = getOutputChecked(m_txPrefix, index, TransactionTypes::OutputType::Key);
    output = boost::get<FKeyOutput>(out.sTarget);
    amount = out.uAmount;
}

void TransactionPrefixImpl::getOutput(
		size_t index, FMultiSignatureOutput &output,
		uint64_t &amount) const
{
    const auto &out = getOutputChecked(
        m_txPrefix,
        index,
        TransactionTypes::OutputType::Multisignature
    );
    output = boost::get<FMultiSignatureOutput>(out.sTarget);
    amount = out.uAmount;
}

size_t TransactionPrefixImpl::getRequiredSignaturesCount(size_t inputIndex) const
{
    return ::QwertyNote::getRequiredSignaturesCount(getInputChecked(m_txPrefix, inputIndex));
}

bool TransactionPrefixImpl::findOutputsToAccount(
    const FAccountPublicAddress &addr,
    const FSecretKey &viewSecretKey,
    std::vector<uint32_t> &outs,
    uint64_t &outputAmount) const
{
    return ::QwertyNote::findOutputsToAccount(m_txPrefix, addr, viewSecretKey, outs, outputAmount);
}

bool TransactionPrefixImpl::validateInputs() const
{
    return
        check_inputs_types_supported(m_txPrefix)
        && check_inputs_overflow(m_txPrefix)
        && checkInputsKeyimagesDiff(m_txPrefix)
        && checkMultisignatureInputsDiff(m_txPrefix);
}

bool TransactionPrefixImpl::validateOutputs() const
{
    return check_outs_valid(m_txPrefix) && check_outs_overflow(m_txPrefix);
}

bool TransactionPrefixImpl::validateSignatures() const
{
    throw std::system_error(
        std::make_error_code(std::errc::function_not_supported),
        "Validating signatures is not supported for transaction prefix"
    );
}

BinaryArray TransactionPrefixImpl::getTransactionData() const
{
    return toBinaryArray(m_txPrefix);
}

bool TransactionPrefixImpl::getTransactionSecretKey(FSecretKey &key) const
{
    return false;
}

std::unique_ptr<ITransactionReader> createTransactionPrefix(
    const FTransactionPrefix& prefix,
    const FHash &transactionHash)
{
    return std::unique_ptr<ITransactionReader> (new TransactionPrefixImpl(prefix, transactionHash));
}

std::unique_ptr<ITransactionReader> createTransactionPrefix(const FTransaction &fullTransaction)
{
    return std::unique_ptr<ITransactionReader>(
        new TransactionPrefixImpl(fullTransaction, getObjectHash(fullTransaction))
    );
}

} // namespace QwertyNote
