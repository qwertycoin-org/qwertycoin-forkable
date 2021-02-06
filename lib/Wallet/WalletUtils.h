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

#include <string>

#include <QwertyNoteCore/Currency.h>

#include <Wallet/IWallet.h>
#include <Wallet/WalletGreen.h>

namespace QwertyNote {

void throwIfKeysMissmatch(
    const Crypto::FSecretKey &secretKey,
    const Crypto::FPublicKey &expectedPublicKey,
    const std::string &message = "");

bool validateAddress(const std::string &address, const QwertyNote::Currency &currency);

std::ostream &operator<<(std::ostream &os, QwertyNote::WalletTransactionState state);
std::ostream &operator<<(std::ostream &os, QwertyNote::WalletTransferType type);
std::ostream &operator<<(std::ostream &os, QwertyNote::WalletGreen::WalletState state);
std::ostream &operator<<(std::ostream &os, QwertyNote::WalletGreen::WalletTrackingMode mode);

class TransferListFormatter
{
public:
  explicit TransferListFormatter(const QwertyNote::Currency &currency,
                                 const WalletGreen::TransfersRange &range);

  void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const TransferListFormatter &formatter);

private:
    const QwertyNote::Currency &m_currency;
    const WalletGreen::TransfersRange &m_range;
};

class WalletOrderListFormatter
{
public:
  explicit WalletOrderListFormatter(
      const QwertyNote::Currency &currency,
      const std::vector<QwertyNote::WalletOrder> &walletOrderList);

  void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const WalletOrderListFormatter &formatter);

private:
    const QwertyNote::Currency &m_currency;
    const std::vector<QwertyNote::WalletOrder> &m_walletOrderList;
};

} // namespace QwertyNote
