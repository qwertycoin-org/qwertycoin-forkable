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

#include <BlockchainExplorer/BlockchainExplorerData.h>

#include <QwertyNoteCore/ICore.h>

#include <QwertyNoteProtocol/IQwertyNoteProtocolQuery.h>

namespace QwertyNote {

    class QBlockchainExplorerDataBuilder
    {
    public:
        QBlockchainExplorerDataBuilder (ICore &sCore, IQwertyNoteProtocolQuery &sProtocol);

        QBlockchainExplorerDataBuilder (const QBlockchainExplorerDataBuilder &) = delete;

        QBlockchainExplorerDataBuilder (QBlockchainExplorerDataBuilder &&) = delete;

        bool fillBlockDetails (const FBlock &sBlock, FBlockDetails &sBlockDetails,
                               bool bCalculatePoW = false);

        bool fillTransactionDetails (const FTransaction &sTransaction,
                                     FTransactionDetails &sTransactionDetails,
                                     uint64_t uTimestamp = 0);

        static bool getPaymentId (const FTransaction &sTransaction, Crypto::FHash &sPaymentId);

        QBlockchainExplorerDataBuilder &operator= (const QBlockchainExplorerDataBuilder &) = delete;

        QBlockchainExplorerDataBuilder &operator= (QBlockchainExplorerDataBuilder &&) = delete;

    private:
        static bool getMixin (const FTransaction &sTransaction, uint64_t &uMixin);

        static bool fillTxExtra (const std::vector <uint8_t> &vRawExtra,
                                 FTransactionExtraDetails &sExtraDetails);

        static size_t median (std::vector <size_t> &vSizes);

        ICore &mCore;
        IQwertyNoteProtocolQuery &mProtocol; // Not used, but why we have it here?
    };

} // namespace QwertyNote
