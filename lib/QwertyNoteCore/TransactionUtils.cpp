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

#include <unordered_set>

#include <Crypto/Crypto.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/TransactionExtra.h>
#include <QwertyNoteCore/TransactionUtils.h>

using namespace Crypto;

namespace QwertyNote {

bool checkInputsKeyimagesDiff(const QwertyNote::FTransactionPrefix &tx)
{
    std::unordered_set<Crypto::FKeyImage> ki;
    for (const auto &in : tx.vInputs) {
    if (in.type() == typeid(FKeyInput)) {
        if (!ki.insert(boost::get<FKeyInput>(in).sKeyImage).second)
            return false;
        }
    }
    return true;
}

size_t getRequiredSignaturesCount(const FTransactionInput &in)
{
    if (in.type() == typeid(FKeyInput)) {
        return boost::get<FKeyInput>(in).vOutputIndexes.size();
    }
    if (in.type() == typeid(FMultiSignatureInput)) {
        return boost::get<FMultiSignatureInput>(in).uSignatureCount;
    }
    return 0;
}

uint64_t getTransactionInputAmount(const FTransactionInput &in)
{
    if (in.type() == typeid(FKeyInput)) {
        return boost::get<FKeyInput>(in).uAmount;
    }
    if (in.type() == typeid(FMultiSignatureInput)) {
        return boost::get<FMultiSignatureInput>(in).uAmount;
    }
    return 0;
}

TransactionTypes::InputType getTransactionInputType(const FTransactionInput &in)
{
    if (in.type() == typeid(FKeyInput)) {
        return TransactionTypes::InputType::Key;
    }
    if (in.type() == typeid(FMultiSignatureInput)) {
        return TransactionTypes::InputType::Multisignature;
    }
    if (in.type() == typeid(FBaseInput)) {
        return TransactionTypes::InputType::Generating;
    }
    return TransactionTypes::InputType::Invalid;
}

const FTransactionInput &getInputChecked(
    const QwertyNote::FTransactionPrefix &transaction,
    size_t index)
{
    if (transaction.vInputs.size() <= index) {
        throw std::runtime_error("Transaction input index out of range");
    }
    return transaction.vInputs[index];
}

const FTransactionInput &getInputChecked(
    const QwertyNote::FTransactionPrefix &transaction,
    size_t index,
    TransactionTypes::InputType type)
{
    const auto &input = getInputChecked(transaction, index);
    if (getTransactionInputType(input) != type) {
        throw std::runtime_error("Unexpected transaction input type");
    }
    return input;
}

TransactionTypes::OutputType getTransactionOutputType(const FTransactionOutputTarget &out)
{
    if (out.type() == typeid(FKeyOutput)) {
        return TransactionTypes::OutputType::Key;
    }
    if (out.type() == typeid(FMultiSignatureOutput)) {
        return TransactionTypes::OutputType::Multisignature;
    }
    return TransactionTypes::OutputType::Invalid;
}

const FTransactionOutput &getOutputChecked(const FTransactionPrefix &transaction, size_t index)
{
    if (transaction.vOutputs.size() <= index) {
        throw std::runtime_error("Transaction output index out of range");
    }
    return transaction.vOutputs[index];
}

const FTransactionOutput &getOutputChecked(
    const QwertyNote::FTransactionPrefix &transaction,
    size_t index,
    TransactionTypes::OutputType type)
{
    const auto &output = getOutputChecked(transaction, index);
    if (getTransactionOutputType(output.sTarget) != type) {
        throw std::runtime_error("Unexpected transaction output target type");
    }
    return output;
}

bool isOutToKey(
    const Crypto::FPublicKey &spendPublicKey,
    const Crypto::FPublicKey &outKey,
    const Crypto::FKeyDerivation &derivation,
    size_t keyIndex)
{
    Crypto::FPublicKey pk;
    derivePublicKey(derivation, keyIndex, spendPublicKey, pk);

    return pk == outKey;
}

bool findOutputsToAccount(
    const QwertyNote::FTransactionPrefix &transaction,
    const FAccountPublicAddress &addr,
    const FSecretKey &viewSecretKey,
    std::vector<uint32_t> &out,
    uint64_t &amount)
{
    FAccountKeys keys;
    keys.sAddress = addr;
    // only view secret key is used, spend key is not needed
    keys.sViewSecretKey = viewSecretKey;

    Crypto::FPublicKey txPubKey = getTransactionPublicKeyFromExtra(transaction.vExtra);

    amount = 0;
    size_t keyIndex = 0;
    uint32_t outputIndex = 0;

    Crypto::FKeyDerivation derivation;
    generateKeyDerivation(txPubKey, keys.sViewSecretKey, derivation);

    for (const FTransactionOutput &o : transaction.vOutputs) {
        assert(o.sTarget.type() == typeid(FKeyOutput)
               || o.sTarget.type() == typeid(FMultiSignatureOutput));
        if (o.sTarget.type() == typeid(FKeyOutput)) {
            if (is_out_to_acc(keys, boost::get<FKeyOutput>(o.sTarget), derivation, keyIndex)) {
                out.push_back(outputIndex);
                amount += o.uAmount;
            }
            ++keyIndex;
        } else if (o.sTarget.type() == typeid(FMultiSignatureOutput)) {
        const auto &target = boost::get<FMultiSignatureOutput>(o.sTarget);
            for (const auto &key : target.vPublicKeys) {
                if (isOutToKey(
                        keys.sAddress.sSpendPublicKey,
                        key,
                        derivation,
                        static_cast<size_t>(outputIndex))
                    ) {
                    out.push_back(outputIndex);
                }
                ++keyIndex;
            }
        }
        ++outputIndex;
    }

    return true;
}

} // namespace QwertyNote
