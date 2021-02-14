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

namespace QwertyNote {

    class IBlockchainObserver
    {
        typedef std::pair <Crypto::FHash, ETransactionRemoveReason> RemovedTransactionDetails;

    public:
        virtual ~IBlockchainObserver () = default;

        virtual void blockchainSynchronized (const FBlockDetails &sTopBlock)
        {
        }

        virtual void blockchainUpdated (const std::vector <FBlockDetails> &vNewBlocks,
                                        const std::vector <FBlockDetails> &vOrphanedBlocks)
        {
        }

        virtual void poolUpdated (const std::vector <FTransactionDetails> &vNewTransactions,
                                  const std::vector <RemovedTransactionDetails> &vRemovedTransactions)
        {
        }
    };

} // namespace QwertyNote
