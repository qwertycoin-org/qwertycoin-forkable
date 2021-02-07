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

#include <IStreamSerializable.h>
#include <TObservable.h>

#include <Crypto/Hash.h>

#include <QwertyNoteCore/ITransaction.h>

namespace QwertyNote {

const uint32_t UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX = std::numeric_limits<uint32_t>::max();

struct FTransactionInformation {
	uint32_t uBlockHeight;
	uint64_t uTimestamp;
	uint64_t uUnlockTime;
	uint64_t uTotalAmountIn;
	uint64_t uTotalAmountOut;
    std::vector<uint8_t> vExtra;
	Crypto::FHash sPaymentId;
	Crypto::FHash sTransactionHash;
	Crypto::FPublicKey sPublicKey;
};

struct FTransactionOutputInformation {
    // output info
	uint32_t uOutputInTransaction;
	uint32_t uGlobalOutputIndex;
	uint64_t uAmount;
    TransactionTypes::OutputType sOutputType;

    // transaction info
    Crypto::FHash sTransactionHash;
    Crypto::FPublicKey sTransactionPublicKey;

    union
    {
		uint32_t uRequiredSignatures; // Type: Multisignature
    	Crypto::FPublicKey sOutputKey; // Type: Key
    };
};

struct FTransactionSpentOutputInformation: public FTransactionOutputInformation {
    uint32_t uSpendingBlockHeight;
	uint32_t uInputInTransaction;
    uint64_t uTimestamp;
    Crypto::FHash sSpendingTransactionHash;
    Crypto::FKeyImage sKeyImage; // WARNING: Used only for TransactionTypes::OutputType::Key
};

class ITransfersContainer : public IStreamSerializable
{
public:
    enum Flags : uint32_t {
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

        IncludeKeyUnlocked = IncludeTypeKey
        					 | IncludeStateUnlocked,
        IncludeKeyNotUnlocked = IncludeTypeKey
        						| IncludeStateLocked
        						| IncludeStateSoftLocked,

        IncludeAllLocked = IncludeTypeAll
        				   | IncludeStateLocked
        				   | IncludeStateSoftLocked,
        IncludeAllUnlocked = IncludeTypeAll
        					 | IncludeStateUnlocked,
        IncludeAll = IncludeTypeAll
        			 | IncludeStateAll,

        IncludeDefault = IncludeKeyUnlocked
    };

    virtual size_t transfersCount() const = 0;
    virtual size_t transactionsCount() const = 0;
    virtual uint64_t balance(uint32_t uFlags = IncludeDefault) const = 0;
    virtual void getOutputs(std::vector<FTransactionOutputInformation> &vTransfers,
        					uint32_t uFlags = IncludeDefault) const = 0;
    virtual bool getTransactionInformation(const Crypto::FHash &sTransactionHash,
										   FTransactionInformation &sInfo,
										   uint64_t *uAmountIn = nullptr,
										   uint64_t *uAmountOut = nullptr) const = 0;
    virtual std::vector<FTransactionOutputInformation> getTransactionOutputs(
        const Crypto::FHash &sTransactionHash,
        uint32_t uFlags = IncludeDefault) const = 0;

    //only type flags are feasible for this function
    virtual std::vector<FTransactionOutputInformation> getTransactionInputs(
        const Crypto::FHash &sTransactionHash,
        uint32_t uFlags) const = 0;

    virtual void getUnconfirmedTransactions(std::vector<Crypto::FHash> &vTransactions) const = 0;
    virtual std::vector<FTransactionSpentOutputInformation> getSpentOutputs() const = 0;
    virtual void markTransactionSafe(const Crypto::FHash &sTransactionHash) = 0;
    virtual void getSafeTransactions(std::vector<Crypto::FHash> &vTransactions) const = 0;
};

} // namespace QwertyNote
