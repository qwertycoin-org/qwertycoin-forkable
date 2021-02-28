// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2016, The Monero Project
// Copyright (c) 2017-2018, Karbo developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <QwertyNote.h>
#include <INode.h>

#include <Common/ObserverManager.h>

#include <QwertyNoteCore/TransactionExtra.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/Currency.h>

#include <Transfers/BlockchainSynchronizer.h>
#include <Transfers/TransfersSynchronizer.h>

#include <Wallet/WalletAsyncContextCounter.h>
#include <Wallet/WalletErrors.h>
#include <WalletLegacy/IWalletLegacy.h>
#include <WalletLegacy/WalletRequest.h>
#include <WalletLegacy/WalletTransactionSender.h>
#include <WalletLegacy/WalletUserTransactionsCache.h>
#include <WalletLegacy/WalletUnconfirmedTransactions.h>

namespace QwertyNote {

class SyncStarter;

class WalletLegacy : public IWalletLegacy, IBlockchainSynchronizerObserver, ITransfersObserver
{
    enum WalletState
    {
        NOT_INITIALIZED = 0,
        INITIALIZED,
        LOADING,
        SAVING
    };

public:
    WalletLegacy(const QwertyNote::Currency &currency, INode &node, Logging::ILogger &loggerGroup);
    ~WalletLegacy() override;

    void addObserver(IWalletLegacyObserver *observer) override;
    void removeObserver(IWalletLegacyObserver *observer) override;

    void initAndGenerate(const std::string &password) override;
    void initAndGenerateDeterministic(const std::string &password) override;
    void initAndLoad(std::istream &source, const std::string &password) override;
    void initWithKeys(const FAccountKeys &accountKeys, const std::string &password) override;
    void shutdown() override;
    void rescan() override;
    void purge() override;

    Crypto::FSecretKey generateKey(
        const std::string &password,
        const Crypto::FSecretKey &recovery_param = Crypto::FSecretKey(),
        bool recover = false,
        bool two_random = false) override;

    void save(std::ostream &destination, bool saveDetailed = true, bool saveCache = true) override;

    std::error_code changePassword(
        const std::string &oldPassword,
        const std::string &newPassword) override;

    std::string getAddress() override;

    uint64_t actualBalance() override;
    uint64_t pendingBalance() override;
    uint64_t dustBalance() override;

    size_t getTransactionCount() override;
    size_t getTransferCount() override;
    size_t getUnlockedOutputsCount() override;

    std::list<FTransactionOutputInformation> selectAllOldOutputs(uint32_t height) override;

    TransactionId findTransactionByTransferId(TransferId transferId) override;

    bool getTransaction(TransactionId transactionId, WalletLegacyTransaction &transaction) override;
    bool getTransfer(TransferId transferId, WalletLegacyTransfer &transfer) override;
    std::vector<Payments> getTransactionsByPaymentIds(const std::vector<PaymentId> &paymentIds) const override;
    bool getTxProof(
			Crypto::FHash &txid, QwertyNote::FAccountPublicAddress &address,
			Crypto::FSecretKey &tx_key,
			std::string &sig_str) override;
    std::string getReserveProof(const uint64_t &reserve, const std::string &message) override;
    Crypto::FSecretKey getTxKey(Crypto::FHash &txid) override;
    bool get_tx_key(Crypto::FHash &txid, Crypto::FSecretKey &txSecretKey) override;
    void getAccountKeys(FAccountKeys &keys) override;
    bool getSeed(std::string &electrum_words) override;

    TransactionId sendTransaction(
        const WalletLegacyTransfer &transfer,
        uint64_t fee,
        const std::string &extra = "",
        uint64_t mixIn = 0,
        uint64_t unlockTimestamp = 0,
        const std::vector<TransactionMessage> &messages = std::vector<TransactionMessage>(),
        uint64_t ttl = 0,
        const std::string &sender = "") override;
    TransactionId sendTransaction(
        const std::vector<WalletLegacyTransfer> &transfers,
        uint64_t fee,
        const std::string &extra = "",
        uint64_t mixIn = 0,
        uint64_t unlockTimestamp = 0,
        const std::vector<TransactionMessage> &messages = std::vector<TransactionMessage>(),
        uint64_t ttl = 0,
        const std::string &sender = "") override;

    TransactionId sendDustTransaction(
        const std::vector<WalletLegacyTransfer> &transfers,
        uint64_t fee,
        const std::string &extra = "",
        uint64_t mixIn = 0,
        uint64_t unlockTimestamp = 0) override;
    TransactionId sendFusionTransaction(
        const std::list<FTransactionOutputInformation> &fusionInputs,
        uint64_t fee,
        const std::string &extra = "",
        uint64_t mixIn = 0,
        uint64_t unlockTimestamp = 0) override;
    std::error_code cancelTransaction(size_t transactionId) override;

    size_t estimateFusion(const uint64_t &threshold) override;
    std::list<FTransactionOutputInformation> selectFusionTransfersToSend(
        uint64_t threshold,
        size_t minInputCount,
        size_t maxInputCount) override;

    std::string sign_message(const std::string &data) override;
    bool verify_message(const std::string &data,
                        const QwertyNote::FAccountPublicAddress &address,
                        const std::string &signature) override;

    bool isTrackingWallet() override;

    void setConsolidateHeight(uint32_t height, const Crypto::FHash &consolidateTx) override;
    uint32_t getConsolidateHeight() const override;
    Crypto::FHash getConsolidateTx() const override;

    void markTransactionSafe(const Crypto::FHash &transactionHash) override;
private:
    // IBlockchainSynchronizerObserver
    void synchronizationProgressUpdated(uint32_t current, uint32_t total) override;
    void synchronizationCompleted(std::error_code result) override;

    // ITransfersObserver
    void onTransactionUpdated(
        ITransfersSubscription *object,
        const Crypto::FHash &transactionHash) override;
    void onTransactionDeleted(
        ITransfersSubscription *object,
        const Crypto::FHash &transactionHash) override;

    void initSync();
    void throwIfNotInitialised();

    void doSave(std::ostream &destination, bool saveDetailed, bool saveCache);
    void doLoad(std::istream &source);

    void synchronizationCallback(WalletRequest::Callback callback, std::error_code ec);
    void sendTransactionCallback(WalletRequest::Callback callback, std::error_code ec);
    void notifyClients(std::deque<std::shared_ptr<WalletLegacyEvent>> &events);
    void notifyIfBalanceChanged();

    std::vector<TransactionId> deleteOutdatedUnconfirmedTransactions();

private:
    WalletState m_state;
    std::mutex m_cacheMutex;
    QwertyNote::AccountBase m_account;
    std::string m_password;
    const QwertyNote::Currency &m_currency;
    INode &m_node;
    Logging::ILogger &m_loggerGroup;
    bool m_isStopping;

    std::atomic<uint64_t> m_lastNotifiedActualBalance;
    std::atomic<uint64_t> m_lastNotifiedPendingBalance;
    std::atomic<uint64_t> m_lastNotifiedUnmixableBalance;

    BlockchainSynchronizer m_blockchainSync;
    TransfersSynchronizer m_transfersSync;
    ITransfersContainer *m_transferDetails;

    WalletUserTransactionsCache m_transactionsCache;
    std::unique_ptr<WalletTransactionSender> m_sender;

    WalletAsyncContextCounter m_asyncContextCounter;
    Tools::QObserverManager<QwertyNote::IWalletLegacyObserver> m_observerManager;

    std::unique_ptr<SyncStarter> m_onInitSyncStarter;
};

} // namespace QwertyNote
