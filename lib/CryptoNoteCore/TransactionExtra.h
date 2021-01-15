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
#include <vector>
#include <boost/variant.hpp>
#include <CryptoNote.h>

#define TX_EXTRA_PADDING_MAX_COUNT 255
#define TX_EXTRA_NONCE_MAX_COUNT 255

#define TX_EXTRA_TAG_PADDING 0x00
#define TX_EXTRA_TAG_PUBKEY 0x01
#define TX_EXTRA_NONCE 0x02
#define TX_EXTRA_MESSAGE_TAG 0x03
#define TX_EXTRA_TTL 0x04
#define TX_EXTRA_SENDER_TAG 0x05

#define TX_EXTRA_NONCE_PAYMENT_ID 0x00

namespace CryptoNote {

class ISerializer;

struct TransactionExtraPadding
{
    size_t size;
};

struct TransactionExtraPublicKey
{
    Crypto::PublicKey publicKey;
};

struct TransactionExtraNonce
{
    std::vector<uint8_t> nonce;
};

struct tx_extra_message
{
    bool encrypt(
        std::size_t index,
        const std::string &message,
        const AccountPublicAddress *recipient,
        const KeyPair &txkey
    );
    bool decrypt(
        std::size_t index,
        const Crypto::PublicKey &txkey,
        Crypto::SecretKey *recepient_secret_key,
        std::string &message
    ) const;

    bool serialize(ISerializer &serializer);

    std::string data;
};

struct tx_extra_sender
{
    bool encrypt(
        std::size_t index,
        const std::string &sender,
        const AccountPublicAddress *recipient,
        const KeyPair &txkey
    );
    bool decrypt(
        std::size_t index,
        const Crypto::PublicKey &txkey,
        Crypto::SecretKey *recepient_secret_key,
        std::string &sender
    ) const;

    bool serialize(ISerializer &serializer);

    std::string data;
};

struct TransactionExtraTTL
{
    uint64_t ttl;
};

/*!
    tx_extra_field format, except tx_extra_padding and tx_extra_pub_key:
    - varint tag;
    - varint size;
    - varint data[].
*/
typedef boost::variant<
    TransactionExtraPadding,
    TransactionExtraPublicKey,
    TransactionExtraNonce,
    tx_extra_message,
    TransactionExtraTTL,
    tx_extra_sender
> TransactionExtraField;

template<typename T>
bool findTransactionExtraFieldByType(
    const std::vector<TransactionExtraField> &tx_extra_fields,
    T &field)
{
    auto it = std::find_if(
        tx_extra_fields.begin(),
        tx_extra_fields.end(),
        [](const TransactionExtraField &f) {
            return typeid(T) == f.type();
    });

    if (tx_extra_fields.end() == it) {
        return false;
    }

    field = boost::get<T>(*it);

    return true;
}

bool parseTransactionExtra(
    const std::vector<uint8_t> &tx_extra,
    std::vector<TransactionExtraField> &tx_extra_fields);
bool writeTransactionExtra(
    std::vector<uint8_t> &tx_extra,
    const std::vector<TransactionExtraField> &tx_extra_fields);

Crypto::PublicKey getTransactionPublicKeyFromExtra(const std::vector<uint8_t> &tx_extra);
bool addTransactionPublicKeyToExtra(std::vector<uint8_t> &tx_extra, const Crypto::PublicKey &tx_pub_key);
bool addExtraNonceToTransactionExtra(std::vector<uint8_t> &tx_extra, const BinaryArray &extra_nonce);
void setPaymentIdToTransactionExtraNonce(BinaryArray &extra_nonce, const Crypto::Hash &payment_id);
bool getPaymentIdFromTransactionExtraNonce(const BinaryArray &extra_nonce,Crypto::Hash &payment_id);
bool appendMessageToExtra(std::vector<uint8_t> &tx_extra, const tx_extra_message &message);
bool appendSenderToExtra(std::vector<uint8_t> &tx_extra, const tx_extra_sender &sender);
void appendTTLToExtra(std::vector<uint8_t> &tx_extra, uint64_t ttl);
std::vector<std::string> getMessagesFromExtra(
    const std::vector<uint8_t> &extra,
    const Crypto::PublicKey &txkey,
    Crypto::SecretKey *recepient_secret_key);
std::vector<std::string> getSendersFromExtra(
    const std::vector<uint8_t> &extra,
    const Crypto::PublicKey &txkey,
    Crypto::SecretKey *recepient_secret_key);

bool createTxExtraWithPaymentId(const std::string &paymentIdString, std::vector<uint8_t> &extra);
// returns false if payment id is not found or parse error
bool getPaymentIdFromTxExtra(const std::vector<uint8_t> &extra, Crypto::Hash &paymentId);
bool parsePaymentId(const std::string &paymentIdString, Crypto::Hash &paymentId);

} // namespace CryptoNote
