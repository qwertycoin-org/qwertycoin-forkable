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

#include <Crypto/Crypto.h>
#include <Wallet/WalletErrors.h>
#include <Wallet/WalletUtils.h>
#include <QwertyNote.h>

namespace QwertyNote {

void throwIfKeysMissmatch(const Crypto::FSecretKey &secretKey,
                          const Crypto::FPublicKey &expectedPublicKey,
                          const std::string &message)
{
    Crypto::FPublicKey pub;
    bool r = Crypto::secretKeyToPublicKey(secretKey, pub);
    if (!r || expectedPublicKey != pub) {
        throw std::system_error(make_error_code(QwertyNote::error::WRONG_PASSWORD), message);
    }
}

bool validateAddress(const std::string &address, const QwertyNote::Currency &currency)
{
    QwertyNote::AccountPublicAddress ignore;
    return currency.parseAccountAddressString(address, ignore);
}

std::ostream &operator<<(std::ostream &os, QwertyNote::WalletTransactionState state)
{
    switch (state) {
    case QwertyNote::WalletTransactionState::SUCCEEDED:
        os << "SUCCEEDED";
        break;
    case QwertyNote::WalletTransactionState::FAILED:
        os << "FAILED";
        break;
    case QwertyNote::WalletTransactionState::CANCELLED:
        os << "CANCELLED";
        break;
    case QwertyNote::WalletTransactionState::CREATED:
        os << "CREATED";
        break;
    case QwertyNote::WalletTransactionState::DELETED:
        os << "DELETED";
        break;
    default:
        os << "<UNKNOWN>";
    }

    return os << " (" << static_cast<int>(state) << ')';
}

std::ostream &operator<<(std::ostream &os, QwertyNote::WalletTransferType type)
{
    switch (type) {
    case QwertyNote::WalletTransferType::USUAL:
        os << "USUAL";
        break;
    case QwertyNote::WalletTransferType::DONATION:
        os << "DONATION";
        break;
    case QwertyNote::WalletTransferType::CHANGE:
        os << "CHANGE";
        break;
    default:
        os << "<UNKNOWN>";
    }

    return os << " (" << static_cast<int>(type) << ')';
}

std::ostream &operator<<(std::ostream &os, QwertyNote::WalletGreen::WalletState state)
{
    switch (state) {
    case QwertyNote::WalletGreen::WalletState::INITIALIZED:
        os << "INITIALIZED";
        break;
    case QwertyNote::WalletGreen::WalletState::NOT_INITIALIZED:
        os << "NOT_INITIALIZED";
        break;
    default:
        os << "<UNKNOWN>";
    }

    return os << " (" << static_cast<int>(state) << ')';
}

std::ostream &operator<<(std::ostream &os, QwertyNote::WalletGreen::WalletTrackingMode mode)
{
    switch (mode) {
    case QwertyNote::WalletGreen::WalletTrackingMode::TRACKING:
        os << "TRACKING";
        break;
    case QwertyNote::WalletGreen::WalletTrackingMode::NOT_TRACKING:
        os << "NOT_TRACKING";
        break;
    case QwertyNote::WalletGreen::WalletTrackingMode::NO_ADDRESSES:
        os << "NO_ADDRESSES";
        break;
    default:
        os << "<UNKNOWN>";
    }

    return os << " (" << static_cast<int>(mode) << ')';
}

TransferListFormatter::TransferListFormatter(const QwertyNote::Currency &currency,
                                             const WalletGreen::TransfersRange &range)
    : m_currency(currency),
      m_range(range)
{
}

void TransferListFormatter::print(std::ostream &os) const
{
    for (auto it = m_range.first; it != m_range.second; ++it) {
        os << '\n' << std::setw(21) << m_currency.formatAmount(it->second.amount)
            << ' ' << (it->second.address.empty() ? "<UNKNOWN>" : it->second.address)
            << ' ' << it->second.type;
    }
}

std::ostream& operator<<(std::ostream &os, const TransferListFormatter &formatter)
{
    formatter.print(os);
    return os;
}

WalletOrderListFormatter::WalletOrderListFormatter(
    const QwertyNote::Currency &currency,
    const std::vector<QwertyNote::WalletOrder> &walletOrderList)
    : m_currency(currency),
      m_walletOrderList(walletOrderList)
{
}

void WalletOrderListFormatter::print(std::ostream &os) const
{
    os << '{';

    if (!m_walletOrderList.empty()) {
        os << '<'
            << m_currency.formatAmount(m_walletOrderList.front().amount)
            << ", "
            << m_walletOrderList.front().address
            << '>';

        for (auto it = std::next(m_walletOrderList.begin()); it != m_walletOrderList.end(); ++it) {
            os << '<' << m_currency.formatAmount(it->amount) << ", " << it->address << '>';
        }
    }

    os << '}';
}

std::ostream &operator<<(std::ostream &os, const WalletOrderListFormatter &formatter)
{
    formatter.print(os);
    return os;
}

} // namespace QwertyNote
