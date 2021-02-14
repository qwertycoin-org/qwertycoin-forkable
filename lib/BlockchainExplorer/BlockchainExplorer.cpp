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

#include <functional>
#include <future>
#include <memory>

#include <BlockchainExplorer/BlockchainExplorer.h>
#include <BlockchainExplorer/BlockchainExplorerErrors.h>

#include <Global/QwertyNoteConfig.h>

#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/ITransaction.h>

using namespace Logging;
using namespace Crypto;

namespace QwertyNote {

    class QContextCounterHolder
    {
    public:
        explicit QContextCounterHolder (WalletAsyncContextCounter &counter)
            : mCounter(counter)
        {
        }

        ~QContextCounterHolder ()
        {
            mCounter.delAsyncContext();
        }

    private:
        WalletAsyncContextCounter &mCounter;
    };

    class QNodeRequest
    {
    public:
        explicit QNodeRequest (const std::function <void (const INode::UCallback &)> &request)
            : URequestFunc(request)
        {
        }

        std::error_code performBlocking ()
        {
            std::promise <std::error_code> promise;
            std::future <std::error_code> future = promise.get_future();
            URequestFunc(
                [&] (std::error_code c)
                {
                    blockingCompletionCallback(std::move(promise), c);
                });

            return future.get();
        }

        void performAsync (WalletAsyncContextCounter &sAsyncContextCounter,
                           const INode::UCallback &sCallback)
        {
            sAsyncContextCounter.addAsyncContext();
            URequestFunc(std::bind(&QNodeRequest::asyncCompletionCallback,
                                   sCallback,
                                   std::ref(sAsyncContextCounter),
                                   std::placeholders::_1));
        }

    private:
        static void
        blockingCompletionCallback (std::promise <std::error_code> sPromise, std::error_code sError)
        {
            sPromise.set_value(sError);
        }

        static void asyncCompletionCallback (const INode::UCallback &sCallback,
                                             WalletAsyncContextCounter &asyncContextCounter,
                                             std::error_code ec)
        {
            QContextCounterHolder counterHolder(asyncContextCounter);
            try {
                sCallback(ec);
            } catch (...) {
                return;
            }
        }

        const std::function <void (const INode::UCallback &)> URequestFunc;
    };

    class QScopeExitHandler
    {
    public:
        explicit QScopeExitHandler (std::function <void ()> &&handler)
            : UHandler(std::move(handler)),
              mCancelled(false)
        {
        }

        ~QScopeExitHandler ()
        {
            if (!mCancelled) {
                UHandler();
            }
        }

        void reset ()
        {
            mCancelled = true;
        }

    private:
        std::function <void ()> UHandler;
        bool mCancelled;
    };

    QBlockchainExplorer::QPoolUpdateGuard::QPoolUpdateGuard ()
        : mState(EState::NONE)
    {
    }

    bool QBlockchainExplorer::QPoolUpdateGuard::beginUpdate ()
    {
        auto s = mState.load();
        for (;;) {
            switch (s) {
                case EState::NONE:
                    if (mState.compare_exchange_weak(s, EState::UPDATING)) {
                        return true;
                    }
                    break;
                case EState::UPDATING:
                    if (mState.compare_exchange_weak(s, EState::UPDATE_REQUIRED)) {
                        return false;
                    }
                    break;
                case EState::UPDATE_REQUIRED:
                    return false;
                default:
                    assert(false);
                    return false;
            }
        }
    }

    bool QBlockchainExplorer::QPoolUpdateGuard::endUpdate ()
    {
        auto s = mState.load();
        for (;;) {
            assert(s != EState::NONE);

            if (mState.compare_exchange_weak(s, EState::NONE)) {
                return s == EState::UPDATE_REQUIRED;
            }
        }
    }

    QBlockchainExplorer::QBlockchainExplorer (INode &sNode, Logging::ILogger &sLogger)
        : mNode(sNode),
          mLogger(sLogger, "BlockchainExplorer"),
          mState(NOT_INITIALIZED),
          mSynchronized(false),
          mObserversCounter(0),
          mKnownBlockchainTopHeight(0)
    {
    }

    void QBlockchainExplorer::init ()
    {
        if (mState.load() != NOT_INITIALIZED) {
            mLogger(ERROR) << "Init called on already initialized BlockchainExplorer.";
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::ALREADY_INITIALIZED));
        }

        if (mNode.addObserver(this)) {
            mState.store(INITIALIZED);
        } else {
            mLogger(ERROR) << "Can't add observer to node.";
            mState.store(NOT_INITIALIZED);
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::INTERNAL_ERROR));
        }

        if (getBlockchainTop(mKnownBlockchainTop)) {
            mKnownBlockchainTopHeight = mKnownBlockchainTop.uHeight;
        } else {
            mLogger(ERROR) << "Can't get blockchain top.";
            mState.store(NOT_INITIALIZED);
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::INTERNAL_ERROR));
        }
    }

    void QBlockchainExplorer::shutdown ()
    {
        if (mState.load() != INITIALIZED) {
            mLogger(ERROR) << "Shutdown called on not initialized BlockchainExplorer.";
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mNode.removeObserver(this);
        mAsyncContextCounter.waitAsyncContextsFinish();
        mState.store(NOT_INITIALIZED);
    }

    bool QBlockchainExplorer::addObserver (IBlockchainObserver *sObserver)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mObserversCounter.fetch_add(1);

        return mObserverManager.add(sObserver);
    }

    bool QBlockchainExplorer::removeObserver (IBlockchainObserver *sObserver)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        if (mObserversCounter.load() != 0) {
            mObserversCounter.fetch_sub(1);
        }

        return mObserverManager.remove(sObserver);
    }

    bool QBlockchainExplorer::getBlocks (const std::vector <uint32_t> &vBlockHeights,
                                         std::vector <std::vector <FBlockDetails>> &vBlocks)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get blocks by height request came.";
        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        const std::vector <uint32_t> &,
                        std::vector <std::vector <FBlockDetails>> &,
                        const INode::UCallback &
                    )
                    >(&INode::getBlocks),
                std::ref(mNode),
                std::cref(vBlockHeights),
                std::ref(vBlocks),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get blocks by height: " << sEc.message();
            throw std::system_error(sEc);
        }

        assert(vBlocks.size() == vBlockHeights.size());

        return true;
    }

    bool QBlockchainExplorer::getBlocks (const std::vector <FHash> &vBlockHashes,
                                         std::vector <FBlockDetails> &vBlocks)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get blocks by Hash request came.";
        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        const std::vector <FHash> &,
                        std::vector <FBlockDetails> &,
                        const INode::UCallback &
                    )
                    >(&INode::getBlocks),
                std::ref(mNode),
                std::cref(reinterpret_cast<const std::vector <FHash> &>(vBlockHashes)),
                std::ref(vBlocks),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get blocks by Hash: " << sEc.message();
            throw std::system_error(sEc);
        }

        assert(vBlocks.size() == vBlockHashes.size());

        return true;
    }

    bool QBlockchainExplorer::getBlocks (uint64_t uTimestampBegin,
                                         uint64_t uTimestampEnd,
                                         uint32_t uBlocksNumberLimit,
                                         std::vector <FBlockDetails> &vBlocks,
                                         uint32_t &uBlocksNumberWithinTimestamps)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get blocks by timestamp request came.";
        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        uint64_t,
                        uint64_t,
                        uint32_t,
                        std::vector <FBlockDetails> &,
                        uint32_t &,
                        const INode::UCallback &
                    )
                    >(&INode::getBlocks),
                std::ref(mNode),
                uTimestampBegin,
                uTimestampEnd,
                uBlocksNumberLimit,
                std::ref(vBlocks),
                std::ref(uBlocksNumberWithinTimestamps),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get blocks by timestamp: " << sEc.message();
            throw std::system_error(sEc);
        }

        return true;
    }

    bool QBlockchainExplorer::getBlockchainTop (FBlockDetails &sTopBlock)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get blockchain top request came.";
        uint32_t uLastLocalBlockHeight = mNode.getLastLocalBlockHeight();
        uint32_t uLastHeightCopy = uLastLocalBlockHeight;
        std::vector <uint32_t> vHeights;
        vHeights.push_back(uLastLocalBlockHeight);

        std::vector <std::vector <FBlockDetails>> vBlocks;
        if (!getBlocks(vHeights, vBlocks)) {
            mLogger(ERROR) << "Can't get blockchain top.";
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::INTERNAL_ERROR));
        }

        assert(vBlocks.size() == vHeights.size() && vBlocks.size() == 1);

        bool bGotMainChainBlock = false;
        for (const FBlockDetails &block : vBlocks.back()) {
            if (!block.bIsOrphaned) {
                sTopBlock = block;
                bGotMainChainBlock = true;
                break;
            }
        }

        if (!bGotMainChainBlock) {
            mLogger(ERROR)
                << "Can't get blockchain top: all vBlocks on height "
                << uLastHeightCopy
                << " are orphaned.";
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::INTERNAL_ERROR));
        }

        return true;
    }

    bool QBlockchainExplorer::getPoolState (const std::vector <FHash> &vKnownPoolTransactionHashes,
                                            FHash sKnownBlockchainTop,
                                            bool &bIsBlockchainActual,
                                            std::vector <FTransactionDetails> &vNewTransactions,
                                            std::vector <FHash> &vRemovedTransactions)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED)
            );
        }

        mLogger(DEBUGGING) << "Get pool state request came.";
        std::vector <std::unique_ptr <ITransactionReader>> vRawNewTransactions;

        QNodeRequest request(
            [&] (const INode::UCallback &sCallback)
            {
                std::vector <FHash> vHashes;
                for (FHash hash : vKnownPoolTransactionHashes) {
                    vHashes.push_back(std::move(hash));
                }

                mNode.getPoolSymmetricDifference(
                    std::move(vHashes),
                    reinterpret_cast<FHash &>(sKnownBlockchainTop),
                    bIsBlockchainActual,
                    vRawNewTransactions,
                    vRemovedTransactions,
                    sCallback
                );
            });

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get pool state: " << sEc.message();
            throw std::system_error(sEc);
        }

        std::vector <FHash> vNewTransactionsHashes;
        for (const auto &sRawTransaction : vRawNewTransactions) {
            FHash transactionHash = sRawTransaction->getTransactionHash();
            vNewTransactionsHashes.push_back(std::move(transactionHash));
        }

        return getTransactions(vNewTransactionsHashes, vNewTransactions);
    }

    bool QBlockchainExplorer::getPoolTransactions (uint64_t uTimestampBegin,
                                                   uint64_t uTimestampEnd,
                                                   uint32_t uTransactionsNumberLimit,
                                                   std::vector <FTransactionDetails> &vTransactions,
                                                   uint64_t &uTransactionsNumberWithinTimestamps)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get transactions by timestamp request came.";
        QNodeRequest request(
            std::bind(
                &INode::getPoolTransactions,
                std::ref(mNode),
                uTimestampBegin,
                uTimestampEnd,
                uTransactionsNumberLimit,
                std::ref(vTransactions),
                std::ref(uTransactionsNumberWithinTimestamps),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get transactions by timestamp: " << sEc.message();
            throw std::system_error(sEc);
        }

        return true;
    }

    bool QBlockchainExplorer::getTransactions (const std::vector <FHash> &vTransactionHashes,
                                               std::vector <FTransactionDetails> &vTransactions)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get transactions by Hash request came.";
        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        const std::vector <FHash> &,
                        std::vector <FTransactionDetails> &,
                        const INode::UCallback &
                    )
                    >(&INode::getTransactions),
                std::ref(mNode),
                std::cref(reinterpret_cast<const std::vector <FHash> &>(vTransactionHashes)),
                std::ref(vTransactions),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get transactions by Hash: " << sEc.message();
            throw std::system_error(sEc);
        }

        return true;
    }

    bool QBlockchainExplorer::getTransactionsByPaymentId (const FHash &sPaymentId,
                                                          std::vector <FTransactionDetails> &vTransactions)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(make_error_code(
                QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Get transactions by payment id request came.";
        QNodeRequest request(
            std::bind(
                &INode::getTransactionsByPaymentId,
                std::ref(mNode),
                std::cref(reinterpret_cast<const FHash &>(sPaymentId)),
                std::ref(vTransactions),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get transactions by payment id: " << sEc.message();
            throw std::system_error(sEc);
        }

        return true;
    }

    uint64_t QBlockchainExplorer::getRewardBlocksWindow ()
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        return parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW;
    }

    uint64_t QBlockchainExplorer::getFullRewardMaxBlockSize (uint8_t uMajorVersion)
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        return parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
    }

    bool QBlockchainExplorer::isSynchronized ()
    {
        if (mState.load() != INITIALIZED) {
            throw std::system_error(
                make_error_code(
                    QwertyNote::Error::EBlockchainExplorerErrorCodes::NOT_INITIALIZED));
        }

        mLogger(DEBUGGING) << "Synchronization status request came.";
        bool syncStatus = false;
        QNodeRequest request(
            std::bind(
                &INode::isSynchronized,
                std::ref(mNode),
                std::ref(syncStatus),
                std::placeholders::_1
            )
        );

        std::error_code sEc = request.performBlocking();
        if (sEc) {
            mLogger(ERROR) << "Can't get synchronization status: " << sEc.message();
            throw std::system_error(sEc);
        }

        mSynchronized.store(syncStatus);

        return syncStatus;
    }

    void QBlockchainExplorer::poolChanged ()
    {
        mLogger(DEBUGGING) << "Got poolChanged notification.";

        if (!mSynchronized.load() || mObserversCounter.load() == 0) {
            return;
        }

        if (!mPoolUpdateGuard.beginUpdate()) {
            return;
        }

        QScopeExitHandler poolUpdateEndGuard(
            std::bind(&QBlockchainExplorer::poolUpdateEndHandler, this));

        std::unique_lock <std::mutex> sLock(mMutex);

        std::shared_ptr <std::vector <std::unique_ptr <ITransactionReader>>> vRawNewTransactionsPtr =
            std::make_shared <std::vector <std::unique_ptr <ITransactionReader>>>();
        std::shared_ptr <std::vector <FHash>> vRemovedTransactionsPtr =
            std::make_shared <std::vector <FHash>>();
        std::shared_ptr <bool> bIsBlockchainActualPtr = std::make_shared <bool>(false);

        QNodeRequest request(
            [this, vRawNewTransactionsPtr, vRemovedTransactionsPtr, bIsBlockchainActualPtr]
                (const INode::UCallback &callback)
            {
                std::vector <FHash> hashes;
                hashes.resize(mKnownPoolState.size());
                for (const FHash &hash : mKnownPoolState) {
                    hashes.push_back(hash);
                }

                mNode.getPoolSymmetricDifference(
                    std::move(hashes),
                    reinterpret_cast<FHash &>(mKnownBlockchainTop.sBlockHash),
                    *bIsBlockchainActualPtr,
                    *vRawNewTransactionsPtr,
                    *vRemovedTransactionsPtr,
                    callback
                );
            });

        request.performAsync(
            mAsyncContextCounter,
            [this, vRawNewTransactionsPtr, vRemovedTransactionsPtr, bIsBlockchainActualPtr] (
                std::error_code sEc)
            {
                QScopeExitHandler poolUpdateEndGuard(
                    std::bind(
                        &QBlockchainExplorer::poolUpdateEndHandler,
                        this
                    )
                );

                if (sEc) {
                    mLogger(ERROR)
                        << "Can't send poolChanged notification because can't get symmetric difference: "
                        << sEc.message();
                    return;
                }

                std::unique_lock <std::mutex> sLock(mMutex);

                auto vNewTransactionsHashesPtr = std::make_shared <std::vector <FHash>>();
                for (const auto &sRawTransaction : *vRawNewTransactionsPtr) {
                    auto hash = sRawTransaction->getTransactionHash();
                    FHash transactionHash = reinterpret_cast<const FHash &>(hash);
                    bool inserted = mKnownPoolState.emplace(transactionHash).second;
                    if (inserted) {
                        vNewTransactionsHashesPtr->push_back(transactionHash);
                    }
                }

                auto removedTransactionsHashesPtr = std::make_shared <
                    std::vector <std::pair <FHash, ETransactionRemoveReason>>
                >();
                for (const FHash sHash : *vRemovedTransactionsPtr) {
                    auto iter = mKnownPoolState.find(sHash);
                    if (iter != mKnownPoolState.end()) {
                        removedTransactionsHashesPtr->push_back(
                            {
                                sHash,
                                ETransactionRemoveReason::IncludedInBlock // can't have real reason here
                            });
                        mKnownPoolState.erase(iter);
                    }
                }

                auto vNewTransactionsPtr = std::make_shared <std::vector <FTransactionDetails>>();
                QNodeRequest request(
                    std::bind(
                        static_cast<
                            void (INode::*) (
                                const std::vector <FHash> &,
                                std::vector <FTransactionDetails> &,
                                const INode::UCallback &
                            )
                            >(&INode::getTransactions),
                        std::ref(mNode),
                        std::cref(*vNewTransactionsHashesPtr),
                        std::ref(*vNewTransactionsPtr),
                        std::placeholders::_1
                    )
                );

                request.performAsync(
                    mAsyncContextCounter,
                    [this, vNewTransactionsHashesPtr, vNewTransactionsPtr, removedTransactionsHashesPtr]
                        (std::error_code sEc)
                    {
                        QScopeExitHandler poolUpdateEndGuard(
                            std::bind(
                                &QBlockchainExplorer::poolUpdateEndHandler,
                                this
                            )
                        );

                        if (sEc) {
                            mLogger(ERROR)
                                << "Can't send poolChanged notification because can't get transactions: "
                                << sEc.message();
                            return;
                        }

                        if (!vNewTransactionsPtr->empty() ||
                            !removedTransactionsHashesPtr->empty()) {
                            mObserverManager.notify(
                                &IBlockchainObserver::poolUpdated,
                                *vNewTransactionsPtr,
                                *removedTransactionsHashesPtr);
                            mLogger(DEBUGGING)
                                << "poolUpdated notification was successfully sent.";
                        }
                    });

                poolUpdateEndGuard.reset();
            });

        poolUpdateEndGuard.reset();
    }

    void QBlockchainExplorer::blockchainSynchronized (uint32_t uTopHeight)
    {
        mLogger(DEBUGGING) << "Got blockchainSynchronized notification.";

        mSynchronized.store(true);

        if (mObserversCounter.load() == 0) {
            return;
        }

        auto blockHeightsPtr = std::make_shared <std::vector <uint32_t>>();
        auto blocksPtr = std::make_shared <std::vector <std::vector <FBlockDetails>>>();

        blockHeightsPtr->push_back(uTopHeight);

        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        const std::vector <uint32_t> &,
                        std::vector <std::vector <FBlockDetails>> &,
                        const INode::UCallback &
                    )
                    >(&INode::getBlocks),
                std::ref(mNode),
                std::cref(*blockHeightsPtr),
                std::ref(*blocksPtr),
                std::placeholders::_1
            )
        );

        request.performAsync(
            mAsyncContextCounter,
            [this, blockHeightsPtr, blocksPtr, uTopHeight] (std::error_code sEc)
            {
                if (sEc) {
                    mLogger(ERROR)
                        << "Can't send blockchainSynchronized notification, can't get blocks by height: "
                        << sEc.message();
                    return;
                }

                assert(blocksPtr->size() == blockHeightsPtr->size() &&
                       blocksPtr->size() == 1);

                FBlockDetails topMainChainBlock = FBlockDetails();
                bool gotMainChainBlock = false;

                for (const FBlockDetails &block : blocksPtr->back()) {
                    if (!block.bIsOrphaned) {
                        topMainChainBlock = block;
                        gotMainChainBlock = true;
                        break;
                    }
                }

                if (!gotMainChainBlock) {
                    mLogger(ERROR)
                        << "Can't send blockchainSynchronized notification, can't get blockchain top:"
                        << " all blocks on height "
                        << uTopHeight
                        << " are orphaned.";
                    return;
                }

                mObserverManager
                    .notify(&IBlockchainObserver::blockchainSynchronized,
                            topMainChainBlock);
                mLogger(DEBUGGING)
                    << "blockchainSynchronized notification was successfully sent.";
            }
        );
    }

    void QBlockchainExplorer::localBlockchainUpdated (uint32_t uHeight)
    {
        mLogger(DEBUGGING) << "Got localBlockchainUpdated notification.";

        if (mObserversCounter.load() == 0) {
            mKnownBlockchainTopHeight = uHeight;
            return;
        }

        std::unique_lock <std::mutex> sLock(mMutex);

        assert(uHeight >= mKnownBlockchainTopHeight);

        auto vBlockHeightsPtr = std::make_shared <std::vector <uint32_t>>();
        auto vBlocksPtr = std::make_shared <std::vector <std::vector <FBlockDetails>>>();

        for (uint32_t i = mKnownBlockchainTopHeight; i <= uHeight; ++i) {
            vBlockHeightsPtr->push_back(i);
        }

        mKnownBlockchainTopHeight = uHeight;

        QNodeRequest request(
            std::bind(
                static_cast<
                    void (INode::*) (
                        const std::vector <uint32_t> &,
                        std::vector <std::vector <FBlockDetails>> &,
                        const INode::UCallback &
                    )
                    >(&INode::getBlocks),
                std::ref(mNode),
                std::cref(*vBlockHeightsPtr),
                std::ref(*vBlocksPtr),
                std::placeholders::_1
            )
        );

        request.performAsync(
            mAsyncContextCounter,
            [this, vBlockHeightsPtr, vBlocksPtr] (std::error_code sEc)
            {
                if (sEc) {
                    mLogger(ERROR)
                        << "Can't send blockchainUpdated notification because can't get blocks by height: "
                        << sEc.message();
                    return;
                }

                assert(vBlocksPtr->size() == vBlockHeightsPtr->size());

                std::unique_lock <std::mutex> sLock(mMutex);

                FBlockDetails sTopMainchainBlock = FBlockDetails();
                bool bGotTopMainchainBlock = false;
                uint64_t uTopHeight = 0;

                std::vector <FBlockDetails> vNewBlocks;
                std::vector <FBlockDetails> vOrphanedBlocks;
                for (const std::vector <FBlockDetails> &vSameHeightBlocks : *vBlocksPtr) {
                    for (const FBlockDetails &sBlock : vSameHeightBlocks) {
                        if (uTopHeight < sBlock.uHeight) {
                            uTopHeight = sBlock.uHeight;
                            bGotTopMainchainBlock = false;
                        }

                        if (sBlock.bIsOrphaned) {
                            vOrphanedBlocks.push_back(sBlock);
                        } else {
                            if (sBlock.uHeight > mKnownBlockchainTop.uHeight
                                ||
                                sBlock.sBlockHash !=
                                mKnownBlockchainTop.sBlockHash) {
                                vNewBlocks.push_back(sBlock);
                            }

                            if (!bGotTopMainchainBlock) {
                                sTopMainchainBlock = sBlock;
                                bGotTopMainchainBlock = true;
                            }
                        }
                    }
                }

                if (!bGotTopMainchainBlock) {
                    mLogger(ERROR)
                        << "Can't send localBlockchainUpdated notification, can't get blockchain top:"
                        << " all blocks on height "
                        << uTopHeight
                        << " are orphaned.";
                    return;
                }

                mKnownBlockchainTop = sTopMainchainBlock;

                mObserverManager
                    .notify(&IBlockchainObserver::blockchainUpdated, vNewBlocks,
                            vOrphanedBlocks);
                mLogger(DEBUGGING)
                    << "localBlockchainUpdated notification was successfully sent.";
            }
        );
    }

    void QBlockchainExplorer::poolUpdateEndHandler ()
    {
        if (mPoolUpdateGuard.endUpdate()) {
            poolChanged();
        }
    }

} // namespace QwertyNote
