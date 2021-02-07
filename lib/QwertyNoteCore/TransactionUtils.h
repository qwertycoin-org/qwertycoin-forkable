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

#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/ITransaction.h>

namespace QwertyNote {

bool checkInputsKeyimagesDiff(const QwertyNote::FTransactionPrefix &tx);

// TransactionInput helper functions
size_t getRequiredSignaturesCount(const FTransactionInput &in);
uint64_t getTransactionInputAmount(const FTransactionInput &in);
TransactionTypes::InputType getTransactionInputType(const FTransactionInput &in);
const FTransactionInput &getInputChecked(const FTransactionPrefix &transaction, size_t index);
const FTransactionInput &getInputChecked(
    const FTransactionPrefix &transaction,
    size_t index,
    TransactionTypes::InputType type);

bool isOutToKey(
    const Crypto::FPublicKey &spendPublicKey,
    const Crypto::FPublicKey &outKey,
    const Crypto::FKeyDerivation &derivation,
    size_t keyIndex);

// TransactionOutput helper functions
TransactionTypes::OutputType getTransactionOutputType(const FTransactionOutputTarget &out);
const FTransactionOutput &getOutputChecked(const FTransactionPrefix &transaction, size_t index);
const FTransactionOutput &getOutputChecked(
    const FTransactionPrefix &transaction,
    size_t index,
    TransactionTypes::OutputType type);

bool findOutputsToAccount(
    const QwertyNote::FTransactionPrefix &transaction,
    const FAccountPublicAddress &addr,
    const Crypto::FSecretKey &viewSecretKey,
    std::vector<uint32_t> &out,
    uint64_t &amount);

} // namespace QwertyNote
