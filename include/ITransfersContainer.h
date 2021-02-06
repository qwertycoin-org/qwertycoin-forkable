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

#include <cstdint>
#include <limits>
#include <vector>

#include <IObservable.h>
#include <IStreamSerializable.h>

#include <Crypto/Hash.h>

#include <QwertyNoteCore/ITransaction.h>

namespace QwertyNote {

const uint32_t UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX = std::numeric_limits<uint32_t>::max();

struct TransactionInformation
{
    Crypto::FHash transactionHash;
    Crypto::FPublicKey publicKey;
    uint32_t blockHeight;
    uint64_t timestamp;
    uint64_t unlockTime;
    uint64_t totalAmountIn;
    uint64_t totalAmountOut;
    std::vector<uint8_t> extra;
    Crypto::FHash paymentId;
};

struct TransactionOutputInformation
{
    // output info
    TransactionTypes::OutputType type;
    uint64_t amount;
    uint32_t globalOutputIndex;
    uint32_t outputInTransaction;

    // transaction info
    Crypto::FHash transactionHash;
    Crypto::FPublicKey transactionPublicKey;

    union
    {
        Crypto::FPublicKey outputKey; // Type: Key
        uint32_t requiredSignatures; // Type: Multisignature
    };
};

struct TransactionSpentOutputInformation: public TransactionOutputInformation
{
    uint32_t spendingBlockHeight;
    uint64_t timestamp;
    Crypto::FHash spendingTransactionHash;
    Crypto::FKeyImage keyImage; // WARNING: Used only for TransactionTypes::OutputType::Key
    uint32_t inputInTransaction;
};

class ITransfersContainer : public IStreamSerializable
{
public:
    enum Flags : uint32_t
    {
        // state
        IncludeStateUnlocked = 0x01,
        IncludeStateLocked = 0x02,
        IncludeStateSoftLocked = 0x04,
        IncludeStateSpent = 0x08,

        // output type
        IncludeTypeKey = 0x100,
        IncludeTypeMultisignature = 0x200,

        // combinations
        IncludeStateAll = 0xff,
        IncludeTypeAll = 0xff00,

        IncludeKeyUnlocked = IncludeTypeKey | IncludeStateUnlocked,
        IncludeKeyNotUnlocked = IncludeTypeKey | IncludeStateLocked | IncludeStateSoftLocked,

        IncludeAllLocked = IncludeTypeAll | IncludeStateLocked | IncludeStateSoftLocked,
        IncludeAllUnlocked = IncludeTypeAll | IncludeStateUnlocked,
        IncludeAll = IncludeTypeAll | IncludeStateAll,

        IncludeDefault = IncludeKeyUnlocked
    };

    virtual size_t transfersCount() const = 0;
    virtual size_t transactionsCount() const = 0;
    virtual uint64_t balance(uint32_t flags = IncludeDefault) const = 0;
    virtual void getOutputs(
        std::vector<TransactionOutputInformation> &transfers,
        uint32_t flags = IncludeDefault) const = 0;
    virtual bool getTransactionInformation(
        const Crypto::FHash &transactionHash,
        TransactionInformation &info,
        uint64_t *amountIn = nullptr,
        uint64_t *amountOut = nullptr) const = 0;
    virtual std::vector<TransactionOutputInformation> getTransactionOutputs(
        const Crypto::FHash &transactionHash,
        uint32_t flags = IncludeDefault) const = 0;

    //only type flags are feasible for this function
    virtual std::vector<TransactionOutputInformation> getTransactionInputs(
        const Crypto::FHash &transactionHash,
        uint32_t flags) const = 0;

    virtual void getUnconfirmedTransactions(std::vector<Crypto::FHash> &transactions) const = 0;
    virtual std::vector<TransactionSpentOutputInformation> getSpentOutputs() const = 0;
    virtual void markTransactionSafe(const Crypto::FHash &transactionHash) = 0;
    virtual void getSafeTransactions(std::vector<Crypto::FHash> &transactions) const = 0;
};

} // namespace QwertyNote
