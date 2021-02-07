// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, Karbo developers
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

#include <boost/utility/value_init.hpp>

#include <QwertyNoteCore/CryptoNoteBasic.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>

#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/BinaryOutputStreamSerializer.h>

namespace Logging {

class ILogger;

} // namespace Logging

namespace QwertyNote {

bool parseAndValidateTransactionFromBinaryArray(
		const BinaryArray &transactionBinaryArray,
		FTransaction &transaction,
		Crypto::FHash &transactionHash,
		Crypto::FHash &transactionPrefixHash);

struct TransactionSourceEntry
{
    typedef std::pair<uint32_t, Crypto::FPublicKey> OutputEntry;

    std::vector<OutputEntry> outputs; // index + key
    size_t realOutput; // index in outputs vector of real output_entry
    Crypto::FPublicKey realTransactionPublicKey; // incoming real tx public key
    size_t realOutputIndexInTransaction; // index in transaction outputs vector
    uint64_t amount; // money
};

struct TransactionDestinationEntry
{
    TransactionDestinationEntry()
        : amount(0),
          addr(boost::value_initialized<FAccountPublicAddress>())
    {
    }

    TransactionDestinationEntry(uint64_t amount, const FAccountPublicAddress &addr)
        : amount(amount),
          addr(addr)
    {
    }

    uint64_t amount; // money
    FAccountPublicAddress addr; // destination address
};

struct tx_message_entry
{
    std::string message;
    bool encrypt;
    FAccountPublicAddress addr;
};

bool constructTransaction(
		const FAccountKeys &senderAccountKeys,
		const std::vector<TransactionSourceEntry> &sources,
		const std::vector<TransactionDestinationEntry> &destinations,
		const std::vector<tx_message_entry> &messages,
		const std::string &sender,
		uint64_t ttl,
		std::vector<uint8_t> extra,
		FTransaction &transaction,
		uint64_t unlock_time,
		Crypto::FSecretKey &tx_key,
		Logging::ILogger &log);

inline bool constructTransaction(
		const FAccountKeys &sender_account_keys,
		const std::vector<TransactionSourceEntry> &sources,
		const std::vector<TransactionDestinationEntry> &destinations,
		std::vector<uint8_t> extra,
		FTransaction &tx,
		uint64_t unlock_time,
		Crypto::FSecretKey &tx_key,
		Logging::ILogger &log)
{
    return constructTransaction(
        sender_account_keys,
        sources,
        destinations,
        std::vector<tx_message_entry>(),
        "",
        0,
        extra,
        tx,
        unlock_time,
        tx_key,
        log);
}

bool is_out_to_acc(
    const FAccountKeys &acc,
    const FKeyOutput &out_key,
    const Crypto::FPublicKey &tx_pub_key,
    size_t keyIndex);
bool is_out_to_acc(
    const FAccountKeys &acc,
    const FKeyOutput &out_key,
    const Crypto::FKeyDerivation &derivation,
    size_t keyIndex);

bool lookup_acc_outs(
    const FAccountKeys &acc,
    const FTransaction &tx,
    const Crypto::FPublicKey &tx_pub_key,
    std::vector<size_t> &outs,
    uint64_t &money_transfered);
bool lookup_acc_outs(
    const FAccountKeys &acc,
    const FTransaction &tx,
    std::vector<size_t> &outs,
    uint64_t &money_transfered);

bool getTxFee(const FTransaction &tx, uint64_t &fee);
uint64_t getTxFee(const FTransaction &tx);
bool generate_key_image_helper(
		const FAccountKeys &ack,
		const Crypto::FPublicKey &tx_public_key,
		size_t real_output_index,
		FKeyPair &in_ephemeral,
		Crypto::FKeyImage &ki);
std::string short_hash_str(const Crypto::FHash &h);

bool get_block_hashing_blob(const FBlock &b, BinaryArray &blob);
bool getBlockHash(const FBlock &b, Crypto::FHash &res);
Crypto::FHash getBlockHash(const FBlock &b);
bool getBlockLongHash(Crypto::CnContext &context, const FBlock &b, Crypto::FHash &res);
bool getInputsMoneyAmount(const FTransaction &tx, uint64_t &money);
uint64_t getOutsMoneyAmount(const FTransaction &tx);
bool check_inputs_types_supported(const FTransactionPrefix &tx);
bool check_outs_valid(const FTransactionPrefix &tx, std::string *error = nullptr);
bool checkMultisignatureInputsDiff(const FTransactionPrefix &tx);

bool check_money_overflow(const FTransactionPrefix &tx);
bool check_outs_overflow(const FTransactionPrefix &tx);
bool check_inputs_overflow(const FTransactionPrefix &tx);
uint32_t get_block_height(const FBlock &b);
std::vector<uint32_t> relative_output_offsets_to_absolute(const std::vector<uint32_t> &off);
std::vector<uint32_t> absolute_output_offsets_to_relative(const std::vector<uint32_t> &off);


// 62387455827 -> 455827 + 7000000 + 80000000 + 300000000 + 2000000000 + 60000000000,
// where 455827 <= dust_threshold
template<typename chunk_handler_t, typename dust_handler_t>
void decompose_amount_into_digits(
    uint64_t amount,
    uint64_t dust_threshold,
    const chunk_handler_t &chunk_handler,
    const dust_handler_t &dust_handler)
{
    if (0 == amount) {
        return;
    }

    bool is_dust_handled = false;
    uint64_t dust = 0;
    uint64_t order = 1;
    while (0 != amount) {
        uint64_t chunk = (amount % 10) * order;
        amount /= 10;
        order *= 10;

        if (dust + chunk <= dust_threshold) {
            dust += chunk;
        } else {
            if (!is_dust_handled && 0 != dust) {
                dust_handler(dust);
                is_dust_handled = true;
            }
            if (0 != chunk) {
                chunk_handler(chunk);
            }
        }
    }

    if (!is_dust_handled && 0 != dust) {
        dust_handler(dust);
    }
}

void get_tx_tree_hash(const std::vector<Crypto::FHash> &tx_hashes, Crypto::FHash &h);
Crypto::FHash get_tx_tree_hash(const std::vector<Crypto::FHash> &tx_hashes);
Crypto::FHash get_tx_tree_hash(const FBlock &b);
bool is_valid_decomposed_amount(uint64_t amount);

} // namespace QwertyNote
