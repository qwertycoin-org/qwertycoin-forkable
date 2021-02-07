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
#include <system_error>

#include <IStreamSerializable.h>
#include <ITransfersContainer.h>

#include <QwertyNoteCore/ITransaction.h>

namespace QwertyNote {

	struct FSynchronizationStart
	{
		uint64_t uHeight;
		uint64_t uTimestamp;
	};

	struct FAccountSubscription
	{
		uint64_t uTransactionSpendableAge;
		uint64_t uSafeTransactionSpendableAge;
		FAccountKeys sKeys;
		FSynchronizationStart sSyncStart;
	};

	class ITransfersSubscription;

	class ITransfersObserver
	{
	public:
		virtual void onError (ITransfersSubscription *sObject,
							  uint32_t uHeight,
							  std::error_code sEc)
		{
		}

		virtual void onTransactionUpdated (ITransfersSubscription *sObject,
										   const Crypto::FHash &sTransactionHash)
		{
		}

		// NOTE: The sender must guarantee that onTransactionDeleted() is called only
		// after onTransactionUpdated() is called for the same \a transactionHash.
		virtual void onTransactionDeleted (ITransfersSubscription *sObject,
										   const Crypto::FHash &sTransactionHash)
		{
		}
	};

	class ITransfersSubscription : public TObservable <ITransfersObserver>
	{
	public:
		virtual ~ITransfersSubscription () = default;

		virtual FAccountPublicAddress getAddress () = 0;

		virtual ITransfersContainer &getContainer () = 0;
	};

	class ITransfersSynchronizerObserver
	{
	public:
		virtual void onBlocksAdded (const Crypto::FPublicKey &sViewPublicKey,
									const std::vector <Crypto::FHash> &vBlockHashes)
		{
		}

		virtual void onBlockchainDetach (const Crypto::FPublicKey &sViewPublicKey,
										 uint32_t uBlockIndex)
		{
		}

		virtual void onTransactionDeleteBegin (const Crypto::FPublicKey &sViewPublicKey,
											   Crypto::FHash sTransactionHash)
		{
		}

		virtual void onTransactionDeleteEnd (const Crypto::FPublicKey &sViewPublicKey,
											 Crypto::FHash sTransactionHash)
		{
		}

		virtual void onTransactionUpdated (const Crypto::FPublicKey &sViewPublicKey,
										   const Crypto::FHash &sTransactionHash,
										   const std::vector <ITransfersContainer *> &vContainers)
		{
		}
	};

	class ITransfersSynchronizer : public IStreamSerializable
	{
	public:
		virtual ~ITransfersSynchronizer () = default;

		virtual ITransfersSubscription &addSubscription (const FAccountSubscription &sAcc) = 0;

		virtual bool removeSubscription (const FAccountPublicAddress &sAcc) = 0;

		virtual void getSubscriptions (std::vector <FAccountPublicAddress> &vSubscriptions) = 0;

		// returns nullptr if address is not found
		virtual ITransfersSubscription *getSubscription (const FAccountPublicAddress &sAcc) = 0;

		virtual std::vector <Crypto::FHash>
		getViewKeyKnownBlocks (const Crypto::FPublicKey &sPublicViewKey) = 0;
	};

} // namespace QwertyNote
