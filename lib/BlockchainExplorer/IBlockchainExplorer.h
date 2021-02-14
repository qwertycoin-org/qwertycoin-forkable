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

#include <BlockchainExplorer/IBlockchainObserver.h>

namespace QwertyNote {

    class IBlockchainExplorer
    {
    public:
        virtual ~IBlockchainExplorer () = default;

        virtual void init () = 0;

        virtual void shutdown () = 0;

        virtual bool addObserver (IBlockchainObserver *sObserver) = 0;

        virtual bool removeObserver (IBlockchainObserver *sObserver) = 0;

        virtual bool getBlocks (const std::vector <uint32_t> &vBlockHeights,
                                std::vector <std::vector <FBlockDetails>> &vBlocks) = 0;

        virtual bool getBlocks (const std::vector <Crypto::FHash> &vBlockHashes,
                                std::vector <FBlockDetails> &vBlocks) = 0;

        virtual bool getBlocks (uint64_t uTimestampBegin,
                                uint64_t uTimestampEnd,
                                uint32_t uBlocksNumberLimit,
                                std::vector <FBlockDetails> &vBlocks,
                                uint32_t &uBlocksNumberWithinTimestamps) = 0;

        virtual bool getBlockchainTop (FBlockDetails &sTopBlock) = 0;

        virtual bool getPoolState (const std::vector <Crypto::FHash> &vKnownPoolTransactionHashes,
                                   Crypto::FHash sKnownBlockchainTop,
                                   bool &bIsBlockchainActual,
                                   std::vector <FTransactionDetails> &vNewTransactions,
                                   std::vector <Crypto::FHash> &vRemovedTransactions) = 0;

        virtual bool getPoolTransactions (uint64_t uTimestampBegin,
                                          uint64_t uTimestampEnd,
                                          uint32_t uTransactionsNumberLimit,
                                          std::vector <FTransactionDetails> &vTransactions,
                                          uint64_t &uTransactionsNumberWithinTimestamps) = 0;

        virtual bool getTransactions (const std::vector <Crypto::FHash> &vTransactionHashes,
                                      std::vector <FTransactionDetails> &vTransactions) = 0;

        virtual bool getTransactionsByPaymentId (const Crypto::FHash &sPaymentId,
                                                 std::vector <FTransactionDetails> &vTransactions) = 0;

        virtual uint64_t getRewardBlocksWindow () = 0;

        virtual uint64_t getFullRewardMaxBlockSize (uint8_t uMajorVersion) = 0;

        virtual bool isSynchronized () = 0;
    };

} // namespace QwertyNote
