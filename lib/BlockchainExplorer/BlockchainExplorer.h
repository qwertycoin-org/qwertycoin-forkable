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

#include <atomic>
#include <mutex>
#include <unordered_set>

#include <INode.h>

#include <BlockchainExplorer/BlockchainExplorerErrors.h>
#include <BlockchainExplorer/IBlockchainExplorer.h>

#include <Common/ObserverManager.h>

#include <Logging/LoggerRef.h>

#include <Wallet/WalletAsyncContextCounter.h>

namespace QwertyNote {

    class QBlockchainExplorer : public IBlockchainExplorer, public INodeObserver
    {
        class QPoolUpdateGuard
        {
        public:
            QPoolUpdateGuard ();

            bool beginUpdate ();

            bool endUpdate ();

        private:
            enum class EState
            {
                NONE,
                UPDATING,
                UPDATE_REQUIRED
            };

            std::atomic <EState> mState;
        };

        enum EState
        {
            NOT_INITIALIZED,
            INITIALIZED
        };
    public:
        QBlockchainExplorer (INode &sNode, Logging::ILogger &sLogger);

        QBlockchainExplorer (const QBlockchainExplorer &) = delete;

        QBlockchainExplorer (QBlockchainExplorer &&) = delete;

        ~QBlockchainExplorer () override = default;

        void init () override;

        void shutdown () override;

        bool addObserver (IBlockchainObserver *sObserver) override;

        bool removeObserver (IBlockchainObserver *sObserver) override;

        bool getBlocks (const std::vector <uint32_t> &vBlockHeights,
                        std::vector <std::vector <FBlockDetails>> &vBlocks) override;

        bool getBlocks (const std::vector <Crypto::FHash> &vBlockHashes,
                        std::vector <FBlockDetails> &vBlocks) override;

        bool getBlocks (uint64_t uTimestampBegin,
                        uint64_t uTimestampEnd,
                        uint32_t uBlocksNumberLimit,
                        std::vector <FBlockDetails> &vBlocks,
                        uint32_t &uBlocksNumberWithinTimestamps) override;

        bool getBlockchainTop (FBlockDetails &sTopBlock) override;

        bool getPoolState (const std::vector <Crypto::FHash> &sCallback,
                           Crypto::FHash sKnownBlockchainTop,
                           bool &bIsBlockchainActual,
                           std::vector <FTransactionDetails> &vNewTransactions,
                           std::vector <Crypto::FHash> &vRemovedTransactions) override;

        bool getPoolTransactions (uint64_t uTimestampBegin,
                                  uint64_t uTimestampEnd,
                                  uint32_t uTransactionsNumberLimit,
                                  std::vector <FTransactionDetails> &vTransactions,
                                  uint64_t &uTransactionsNumberWithinTimestamps) override;

        bool getTransactions (const std::vector <Crypto::FHash> &vTransactionHashes,
                              std::vector <FTransactionDetails> &vTransactions) override;

        bool getTransactionsByPaymentId (const Crypto::FHash &sPaymentId,
                                         std::vector <FTransactionDetails> &vTransactions) override;

        uint64_t getRewardBlocksWindow () override;

        uint64_t getFullRewardMaxBlockSize (uint8_t uMajorVersion) override;

        bool isSynchronized () override;

        void poolChanged () override;

        void blockchainSynchronized (uint32_t sEc) override;

        void localBlockchainUpdated (uint32_t sEc) override;

        QBlockchainExplorer &operator= (const QBlockchainExplorer &) = delete;

        QBlockchainExplorer &operator= (QBlockchainExplorer &&) = delete;

    private:
        void poolUpdateEndHandler ();

        std::atomic <EState> mState;
        std::atomic <bool> mSynchronized;
        std::atomic <uint32_t> mObserversCounter;
        std::mutex mMutex;
        std::unordered_set <Crypto::FHash> mKnownPoolState;

        uint32_t mKnownBlockchainTopHeight;

        FBlockDetails mKnownBlockchainTop;
        INode &mNode;
        Logging::LoggerRef mLogger;
        QPoolUpdateGuard mPoolUpdateGuard;
        Tools::QObserverManager <IBlockchainObserver> mObserverManager;
        WalletAsyncContextCounter mAsyncContextCounter;
    };

} // namespace QwertyNote
