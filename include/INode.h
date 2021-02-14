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
#include <functional>
#include <system_error>
#include <vector>

#include <BlockchainExplorer/BlockchainExplorerData.h>

#include <Crypto/Crypto.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/ITransaction.h>

#include <QwertyNoteProtocol/CryptoNoteProtocolDefinitions.h>

#include <Rpc/CoreRpcServerCommandsDefinitions.h>

namespace QwertyNote {

	class INodeObserver
	{
	public:
		virtual ~INodeObserver () = default;

		virtual void peerCountUpdated (size_t count)
		{
		}

		virtual void localBlockchainUpdated (uint32_t height)
		{
		}

		virtual void lastKnownBlockHeightUpdated (uint32_t height)
		{
		}

		virtual void poolChanged ()
		{
		}

		virtual void blockchainSynchronized (uint32_t topHeight)
		{
		}
	};

	struct FOutEntry
	{
		uint32_t uOutGlobalIndex;
		Crypto::FPublicKey sOutKey;
	};

	struct FOutsForAmount
	{
		uint64_t uAmount;
		std::vector <FOutEntry> vOuts;
	};

	struct FTransactionShortInfo
	{
		Crypto::FHash sTxId;
		FTransactionPrefix sTxPrefix;
	};

	struct FBlockShortEntry
	{
		bool bHasBlock;
		std::vector <FTransactionShortInfo> vTxsShortInfo;
		Crypto::FHash sBlockHash;
		QwertyNote::FBlock sBlock;
	};

	struct FBlockHeaderInfo
	{
		bool bIsAlternative;
		uint8_t uMajorVersion;
		uint8_t uMinorVersion;
		uint32_t uDepth; // last block index = current block index + depth
		uint32_t uIndex;
		uint32_t uNonce;
		uint64_t uReward;
		uint64_t uTimestamp;
		Crypto::FHash sHash;
		Crypto::FHash sPrevHash;
		difficulty_type sDifficulty;
	};

	class INode
	{
	public:
		typedef std::function <void (std::error_code)> UCallback;

		virtual ~INode () = default;

		virtual bool addObserver (INodeObserver *sObserver) = 0;

		virtual bool removeObserver (INodeObserver *sObserver) = 0;

		virtual void init (const UCallback &sCallback) = 0;

		virtual bool shutdown () = 0;

		virtual size_t getPeerCount () const = 0;

		virtual uint32_t getLastLocalBlockHeight () const = 0;

		virtual uint32_t getLastKnownBlockHeight () const = 0;

		virtual uint32_t getLocalBlockCount () const = 0;

		virtual uint32_t getKnownBlockCount () const = 0;

		virtual uint64_t getMinimalFee () const = 0;

		virtual uint64_t getLastLocalBlockTimestamp () const = 0;

		virtual uint32_t getNodeHeight () const = 0;

		virtual uint32_t getGRBHeight () const = 0;

		virtual FBlockHeaderInfo getLastLocalBlockHeaderInfo () const = 0;

		virtual void relayTransaction (const FTransaction &sTransaction,
									   const UCallback &sCallback) = 0;

		virtual void getRandomOutsByAmounts (
				std::vector <uint64_t> &&vAmounts,
				uint64_t uOutsCount,
				std::vector <QwertyNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>
				&vResult,
				const UCallback &sCallback) = 0;

		virtual void getNewBlocks (std::vector <Crypto::FHash> &&vKnownBlockIds,
								   std::vector <QwertyNote::BlockCompleteEntry> &sNewBlocks,
								   uint32_t &uStartHeight,
								   const UCallback &sCallback) = 0;

		virtual void getTransactionOutsGlobalIndices (const Crypto::FHash &sTransactionHash,
													  std::vector <uint32_t> &vOutsGlobalIndices,
													  const UCallback &sCallback) = 0;

		virtual void queryBlocks (std::vector <Crypto::FHash> &&vKnownBlockIds,
								  uint64_t uTimestamp,
								  std::vector <FBlockShortEntry> &vNewBlocks,
								  uint32_t &uStartHeight,
								  const UCallback &sCallback) = 0;

		virtual void getPoolSymmetricDifference (
				std::vector <Crypto::FHash> &&sKnownPoolTxIds,
				Crypto::FHash sKnownBlockId,
				bool &bIsBcActual,
				std::vector <std::unique_ptr <ITransactionReader>> &vNewTxs,
				std::vector <Crypto::FHash> &vDeletedTxIds,
				const UCallback &sCallback) = 0;

		virtual void getMultisignatureOutputByGlobalIndex (uint64_t uAmount,
														   uint32_t uGlobalIndex,
														   FMultiSignatureOutput &sOut,
														   const UCallback &sCallback) = 0;

		virtual void getBlocks (const std::vector <uint32_t> &vBlockHeights,
								std::vector <std::vector <FBlockDetails>> &vBlocks,
								const UCallback &vCallback) = 0;

		virtual void getBlocks (const std::vector <Crypto::FHash> &vBlockHashes,
								std::vector <FBlockDetails> &vBlocks,
								const UCallback &sCallback) = 0;

		virtual void getBlocks (uint64_t uTimestampBegin,
								uint64_t uTimestampEnd,
								uint32_t uBlocksNumberLimit,
								std::vector <FBlockDetails> &vBlocks,
								uint32_t &uBlocksNumberWithinTimestamps,
								const UCallback &sCallback) = 0;

		virtual void getTransactions (const std::vector <Crypto::FHash> &vTransactionHashes,
									  std::vector <FTransactionDetails> &vTransactions,
									  const UCallback &sCallback) = 0;

		virtual void getTransactionsByPaymentId (const Crypto::FHash &sPaymentId,
												 std::vector <FTransactionDetails> &vTransactions,
												 const UCallback &sCallback) = 0;

		virtual void getPoolTransactions (uint64_t uTimestampBegin,
										  uint64_t uTimestampEnd,
										  uint32_t uTransactionsNumberLimit,
										  std::vector <FTransactionDetails> &vTransactions,
										  uint64_t &uTransactionsNumberWithinTimestamps,
										  const UCallback &sCallback) = 0;

		virtual void isSynchronized (bool &bSyncStatus,
									 const UCallback &sCallback) = 0;
	};

} // namespace QwertyNote
