// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
// Copyright (c) 2018, Karbo developers
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

#include <set>

#include <Common/Varint.h>

#include <Crypto/CnSlowHash.h>

#include <Global/Constants.h>
#include <Global/QwertyNoteConfig.h>

#include <Logging/LoggerRef.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/CryptoNoteBasicImpl.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>
#include <QwertyNoteCore/TransactionExtra.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/Currency.h>

#include <Serialization/BinaryOutputStreamSerializer.h>
#include <Serialization/BinaryInputStreamSerializer.h>

using namespace Logging;
using namespace Crypto;
using namespace Common;
using namespace Qwertycoin;

namespace QwertyNote {

bool parseAndValidateTransactionFromBinaryArray(
		const BinaryArray &tx_blob,
		FTransaction &tx,
		FHash &tx_hash,
		FHash &tx_prefix_hash)
{
    if (!fromBinaryArray(tx, tx_blob)) {
        return false;
    }

    //TODO: validate tx
    cnFastHash(tx_blob.data(), tx_blob.size(), tx_hash);
    getObjectHash(*static_cast<FTransactionPrefix *>(&tx), tx_prefix_hash);

    return true;
}

bool generate_key_image_helper(
		const FAccountKeys &ack,
		const FPublicKey &tx_public_key,
		size_t real_output_index,
		FKeyPair &in_ephemeral,
		FKeyImage &ki)
{
    FKeyDerivation recv_derivation;
    bool r = generateKeyDerivation(tx_public_key, ack.sViewSecretKey, recv_derivation);

    assert(r && "key image helper: failed to generateKeyDerivation");

    if (!r) {
        return false;
    }

    r = derivePublicKey(recv_derivation, real_output_index, ack.sAddress.sSpendPublicKey,
                        in_ephemeral.sPublicKey);

    assert(r && "key image helper: failed to derivePublicKey");

    if (!r) {
        return false;
    }

    deriveSecretKey(recv_derivation, real_output_index, ack.sSpendSecretKey, in_ephemeral.sSecretKey);
    generateKeyImage(in_ephemeral.sPublicKey, in_ephemeral.sSecretKey, ki);

    return true;
}

uint64_t power_integral(uint64_t a, uint64_t b)
{
    if (b == 0) {
        return 1;
    }
    uint64_t total = a;
    for (uint64_t i = 1; i != b; i++) {
        total *= a;
    }
    return total;
}

bool getTxFee(const FTransaction &tx, uint64_t &fee)
{
    uint64_t amountIn = 0;
    uint64_t amountOut = 0;

    for (const auto &in : tx.vInputs) {
        if (in.type() == typeid(FKeyInput)) {
            amountIn += boost::get<FKeyInput>(in).uAmount;
        } else if (in.type() == typeid(FMultiSignatureInput)) {
            amountIn += boost::get<FMultiSignatureInput>(in).uAmount;
        }
    }

    for (const auto &o : tx.vOutputs) {
        amountOut += o.uAmount;
    }

    if (!(amountIn >= amountOut)) {
        return false;
    }

    fee = amountIn - amountOut;

    return true;
}

uint64_t getTxFee(const FTransaction &tx)
{
    uint64_t r = 0;
    if (!getTxFee(tx, r)) {
        return 0;
    }
    return r;
}

bool constructTransaction(
		const FAccountKeys &sender_account_keys,
		const std::vector<TransactionSourceEntry> &sources,
		const std::vector<TransactionDestinationEntry> &destinations,
		const std::vector<tx_message_entry> &messages,
		const std::string &sender,
		uint64_t ttl,
		std::vector<uint8_t> extra,
		FTransaction &tx,
		uint64_t unlock_time,
		Crypto::FSecretKey &tx_key,
		Logging::ILogger &log)
{
    LoggerRef logger(log, "construct_tx");

    tx.vInputs.clear();
    tx.vOutputs.clear();
    tx.vSignatures.clear();

    tx.uVersion = CURRENT_TRANSACTION_VERSION;
    tx.uUnlockTime = unlock_time;

    tx.vExtra = extra;
    FKeyPair txkey = generateKeyPair();
    addTransactionPublicKeyToExtra(tx.vExtra, txkey.sPublicKey);

    tx_key = txkey.sSecretKey;

    struct input_generation_context_data
    {
        FKeyPair in_ephemeral;
    };

    std::vector<input_generation_context_data> in_contexts;
    uint64_t summary_inputs_money = 0;
    // fill inputs
    for (const TransactionSourceEntry &src_entr : sources) {
        if (src_entr.realOutput >= src_entr.outputs.size()) {
            logger(ERROR)
                << "real_output index ("
                << src_entr.realOutput
                << ")bigger than output_keys.size()="
                << src_entr.outputs.size();
            return false;
        }
        summary_inputs_money += src_entr.amount;

        in_contexts.push_back(input_generation_context_data());
        FKeyPair &in_ephemeral = in_contexts.back().in_ephemeral;
        FKeyImage img;
        if (!generate_key_image_helper(
                sender_account_keys,
                src_entr.realTransactionPublicKey,
                src_entr.realOutputIndexInTransaction,
                in_ephemeral,
                img)
            ) {
            return false;
        }

        // check that derived key is equal with real output key
        if (!(in_ephemeral.sPublicKey == src_entr.outputs[src_entr.realOutput].second)) {
            logger(ERROR)
					<< "derived public key mismatch with output public key! " << ENDL
					<< "derived_key:" << Common::podToHex(in_ephemeral.sPublicKey) << ENDL
					<< "real output_public_key:"
					<< Common::podToHex(src_entr.outputs[src_entr.realOutput].second);
            return false;
        }

        // put key image into tx input
        FKeyInput input_to_key;
        input_to_key.uAmount = src_entr.amount;
        input_to_key.sKeyImage = img;

        // fill outputs array and use relative offsets
        for (const TransactionSourceEntry::OutputEntry &out_entry : src_entr.outputs) {
            input_to_key.vOutputIndexes.push_back(out_entry.first);
        }

        input_to_key.vOutputIndexes=absolute_output_offsets_to_relative(input_to_key.vOutputIndexes);
        tx.vInputs.push_back(input_to_key);
    }

    // "shuffle" outs
    std::vector<TransactionDestinationEntry> shuffled_dsts(destinations);
    std::sort(
        shuffled_dsts.begin(),
        shuffled_dsts.end(),
        [](const TransactionDestinationEntry &de1, const TransactionDestinationEntry &de2) {
            return de1.amount < de2.amount;
        }
    );

    uint64_t summary_outs_money = 0;
    // fill outputs
    size_t output_index = 0;
    for (const TransactionDestinationEntry &dst_entr : shuffled_dsts) {
        if (!(dst_entr.amount > 0)) {
            logger(ERROR, BRIGHT_RED) << "Destination with wrong amount: " << dst_entr.amount;
            return false;
        }
        FKeyDerivation derivation;
        FPublicKey out_eph_public_key;
        bool r = generateKeyDerivation(dst_entr.addr.sViewPublicKey, txkey.sSecretKey, derivation);

        if (!(r)) {
            logger(ERROR, BRIGHT_RED)
                << "at creation outs: failed to generateKeyDerivation("
                << dst_entr.addr.sViewPublicKey
                << ", "
                << txkey.sSecretKey
                << ")";
            return false;
        }

        r = derivePublicKey(derivation, output_index, dst_entr.addr.sSpendPublicKey,
                            out_eph_public_key);
        if (!(r)) {
            logger(ERROR, BRIGHT_RED)
                << "at creation outs: failed to derivePublicKey("
                << derivation
                << ", "
                << output_index
                << ", "
                << dst_entr.addr.sSpendPublicKey
                << ")";
            return false;
        }

        FTransactionOutput out;
        out.uAmount = dst_entr.amount;
        FKeyOutput tk;
        tk.sPublicKey = out_eph_public_key;
        out.sTarget = tk;
        tx.vOutputs.push_back(out);
        output_index++;
        summary_outs_money += dst_entr.amount;
    }

    // check money
    if (summary_outs_money > summary_inputs_money) {
        logger(ERROR)
            << "Transaction inputs money ("
            << summary_inputs_money
            << ") less than outputs money ("
            << summary_outs_money
            << ")";
        return false;
    }

    if (P2P_MESSAGES) {
        for (size_t i = 0; i < messages.size(); i++) {
            const tx_message_entry &msg = messages[i];

            tx_extra_message tag;
            tx_extra_sender sTag;
            if (!tag.encrypt(i, msg.message, msg.encrypt ? &msg.addr : nullptr, txkey)) {
                return false;
            }

            if (!sTag.encrypt(i, sender, msg.encrypt ? &msg.addr : nullptr, txkey)) {
                return false;
            }

            if (!appendMessageToExtra(tx.vExtra, tag)) {
                return false;
            }

            if (!appendSenderToExtra(tx.vExtra, sTag)) {
                return false;
            }
        }

        if (ttl != 0) {
            appendTTLToExtra(tx.vExtra, ttl);
        }
    }

    //generate ring signatures
    FHash tx_prefix_hash;
    getObjectHash(*static_cast<FTransactionPrefix *>(&tx), tx_prefix_hash);

    size_t i = 0;
    for (const TransactionSourceEntry &src_entr : sources) {
        std::vector<const FPublicKey *> keys_ptrs;
        for (const TransactionSourceEntry::OutputEntry &o : src_entr.outputs) {
            keys_ptrs.push_back(&o.second);
        }

        tx.vSignatures.push_back(std::vector<FSignature>());
        std::vector<FSignature> &sigs = tx.vSignatures.back();
        sigs.resize(src_entr.outputs.size());
        generateRingSignature(tx_prefix_hash, boost::get<FKeyInput>(tx.vInputs[i]).sKeyImage,
							  keys_ptrs, in_contexts[i].in_ephemeral.sSecretKey, src_entr.realOutput,
							  sigs.data());
        i++;
    }

    return true;
}

bool getInputsMoneyAmount(const FTransaction &tx, uint64_t &money)
{
    money = 0;

    for (const auto &in : tx.vInputs) {
        uint64_t amount = 0;

        if (in.type() == typeid(FKeyInput)) {
            amount = boost::get<FKeyInput>(in).uAmount;
        } else if (in.type() == typeid(FMultiSignatureInput)) {
            amount = boost::get<FMultiSignatureInput>(in).uAmount;
        }

        money += amount;
    }
    return true;
}

uint32_t get_block_height(const FBlock &b)
{
    if (b.sBaseTransaction.vInputs.size() != 1) {
        return 0;
    }
    const auto &in = b.sBaseTransaction.vInputs[0];
    if (in.type() != typeid(FBaseInput)) {
        return 0;
    }
    return boost::get<FBaseInput>(in).uBlockIndex;
}

bool check_inputs_types_supported(const FTransactionPrefix &tx)
{
    for (const auto &in : tx.vInputs) {
        if (in.type() != typeid(FKeyInput) && in.type() != typeid(FMultiSignatureInput)) {
            return false;
        }
    }

    return true;
}

bool check_outs_valid(const FTransactionPrefix &tx, std::string *error)
{
    std::unordered_set<FPublicKey> keys_seen;
    for (const FTransactionOutput& out : tx.vOutputs) {
        if (out.sTarget.type() == typeid(FKeyOutput)) {
            if (out.uAmount == 0) {
                if (error) {
                    *error = "Zero amount ouput";
                }
                return false;
            }

            if (!checkKey(boost::get<FKeyOutput>(out.sTarget).sPublicKey)) {
                if (error) {
                    *error = "Output with invalid key";
                }
                return false;
            }

            if (keys_seen.find(boost::get<FKeyOutput>(out.sTarget).sPublicKey) != keys_seen.end()) {
                if (error) {
                    *error = "The same output target is present more than once";
                }
                return false;
            }
            keys_seen.insert(boost::get<FKeyOutput>(out.sTarget).sPublicKey);
        } else if (out.sTarget.type() == typeid(FMultiSignatureOutput)) {
            const FMultiSignatureOutput &multisignatureOutput =
                ::boost::get<FMultiSignatureOutput>(out.sTarget);
            if (multisignatureOutput.uRequiredSignatureCount > multisignatureOutput.vPublicKeys.size()) {
                if (error) {
                    *error = "Multisignature output with invalid required signature count";
                }
                return false;
            }
            for (const FPublicKey &key : multisignatureOutput.vPublicKeys) {
                if (!checkKey(key)) {
                    if (error) {
                        *error = "Multisignature output with invalid public key";
                    }
                    return false;
                }

                if (keys_seen.find(key) != keys_seen.end()) {
                    if (error) {
                        *error = "The same multisignature output target is present more than once";
                    }
                    return false;
                }
                keys_seen.insert(key);
            }
        } else {
            if (error) {
                *error = "Output with invalid type";
            }
            return false;
        }
    }

    return true;
}

bool checkMultisignatureInputsDiff(const FTransactionPrefix &tx)
{
    std::set<std::pair<uint64_t, uint32_t>> inputsUsage;
    for (const auto &inv : tx.vInputs) {
        if (inv.type() == typeid(FMultiSignatureInput)) {
            const FMultiSignatureInput & in = ::boost::get<FMultiSignatureInput>(inv);
            if (!inputsUsage.insert(std::make_pair(in.uAmount, in.uOutputIndex)).second) {
                return false;
            }
        }
    }
    return true;
}

bool check_money_overflow(const FTransactionPrefix &tx)
{
    return check_inputs_overflow(tx) && check_outs_overflow(tx);
}

bool check_inputs_overflow(const FTransactionPrefix &tx)
{
    uint64_t money = 0;

    for (const auto &in : tx.vInputs) {
        uint64_t amount = 0;

        if (in.type() == typeid(FKeyInput)) {
            amount = boost::get<FKeyInput>(in).uAmount;
        } else if (in.type() == typeid(FMultiSignatureInput)) {
            amount = boost::get<FMultiSignatureInput>(in).uAmount;
        }

        if (money > amount + money) {
            return false;
        }

        money += amount;
    }
    return true;
}

bool check_outs_overflow(const FTransactionPrefix &tx)
{
    uint64_t money = 0;
    for (const auto& o : tx.vOutputs) {
        if (money > o.uAmount + money) {
            return false;
        }
        money += o.uAmount;
    }
    return true;
}

uint64_t getOutsMoneyAmount(const FTransaction &tx)
{
    uint64_t outputs_amount = 0;
    for (const auto &o : tx.vOutputs) {
        outputs_amount += o.uAmount;
    }
    return outputs_amount;
}

std::string short_hash_str(const FHash &h)
{
    std::string res = Common::podToHex(h);

    if (res.size() == 64) {
        auto erased_pos = res.erase(8, 48);
        res.insert(8, "....");
    }

    return res;
}

bool is_out_to_acc(
    const FAccountKeys &acc,
    const FKeyOutput &out_key,
    const FKeyDerivation &derivation,
    size_t keyIndex)
{
    FPublicKey pk;
    derivePublicKey(derivation, keyIndex, acc.sAddress.sSpendPublicKey, pk);
    return pk == out_key.sPublicKey;
}

bool is_out_to_acc(
    const FAccountKeys &acc,
    const FKeyOutput &out_key,
    const FPublicKey &tx_pub_key,
    size_t keyIndex)
{
    FKeyDerivation derivation;
    generateKeyDerivation(tx_pub_key, acc.sViewSecretKey, derivation);
    return is_out_to_acc(acc, out_key, derivation, keyIndex);
}

bool lookup_acc_outs(
    const FAccountKeys &acc,
    const FTransaction &tx,
    std::vector<size_t> &outs,
    uint64_t &money_transfered)
{
    FPublicKey transactionPublicKey = getTransactionPublicKeyFromExtra(tx.vExtra);
    if (transactionPublicKey == NULL_PUBLIC_KEY) {
        return false;
    }
    return lookup_acc_outs(acc, tx, transactionPublicKey, outs, money_transfered);
}

bool lookup_acc_outs(
    const FAccountKeys &acc,
    const FTransaction &tx,
    const FPublicKey &tx_pub_key,
    std::vector<size_t> &outs,
    uint64_t &money_transfered)
{
    money_transfered = 0;
    size_t keyIndex = 0;
    size_t outputIndex = 0;

    FKeyDerivation derivation;
    generateKeyDerivation(tx_pub_key, acc.sViewSecretKey, derivation);

    for (const FTransactionOutput &o : tx.vOutputs) {
        assert(o.sTarget.type() == typeid(FKeyOutput)
               || o.sTarget.type() == typeid(FMultiSignatureOutput));

        if (o.sTarget.type() == typeid(FKeyOutput)) {
            if (is_out_to_acc(acc, boost::get<FKeyOutput>(o.sTarget), derivation, keyIndex)) {
                outs.push_back(outputIndex);
                money_transfered += o.uAmount;
            }

            ++keyIndex;
        } else if (o.sTarget.type() == typeid(FMultiSignatureOutput)) {
            keyIndex += boost::get<FMultiSignatureOutput>(o.sTarget).vPublicKeys.size();
        }

        ++outputIndex;
    }
    return true;
}

bool get_block_hashing_blob(const FBlock &b, BinaryArray &ba)
{
    if (!toBinaryArray(static_cast<const FBlockHeader &>(b), ba)) {
        return false;
    }

    FHash treeRootHash = get_tx_tree_hash(b);
    ba.insert(ba.end(), treeRootHash.uData, treeRootHash.uData + 32);
    auto transactionCount = asBinaryArray(Tools::getVarintData(b.vTransactionHashes.size() + 1));
    ba.insert(ba.end(), transactionCount.begin(), transactionCount.end());

    return true;
}

bool getBlockHash(const FBlock &b, FHash &res)
{
    BinaryArray ba;
    if (!get_block_hashing_blob(b, ba)) {
        return false;
    }

    return getObjectHash(ba, res);
}

FHash getBlockHash(const FBlock &b)
{
    FHash p = NULL_HASH;
    getBlockHash(b, p);
    return p;
}

bool getBlockLongHash(CnContext &context, const FBlock &b, FHash &res)
{
    BinaryArray bd;
    if (!get_block_hashing_blob(b, bd)) {
        return false;
    }

    cnPowHashV1 cnh;
    cnh.hash(bd.data(), bd.size(), res.uData);

    return true;
}

std::vector<uint32_t> relative_output_offsets_to_absolute(const std::vector<uint32_t> &off)
{
    std::vector<uint32_t> res = off;
    for (size_t i = 1; i < res.size(); i++) {
        res[i] += res[i - 1];
    }
    return res;
}

std::vector<uint32_t> absolute_output_offsets_to_relative(const std::vector<uint32_t> &off)
{
    std::vector<uint32_t> res = off;
    if (!off.size()) {
        return res;
    }
    std::sort(res.begin(), res.end()); // just to be sure, actually it is already should be sorted
    for (size_t i = res.size() - 1; i != 0; i--) {
        res[i] -= res[i - 1];
    }

    return res;
}

void get_tx_tree_hash(const std::vector<FHash> &tx_hashes, FHash &h)
{
    treeHash(tx_hashes.data(), tx_hashes.size(), h);
}

FHash get_tx_tree_hash(const std::vector<FHash> &tx_hashes)
{
    FHash h = NULL_HASH;
    get_tx_tree_hash(tx_hashes, h);
    return h;
}

FHash get_tx_tree_hash(const FBlock &b)
{
    std::vector<FHash> txs_ids;
    FHash h = NULL_HASH;
    getObjectHash(b.sBaseTransaction, h);
    txs_ids.push_back(h);
    for (auto &th : b.vTransactionHashes) {
        txs_ids.push_back(th);
    }
    return get_tx_tree_hash(txs_ids);
}

bool is_valid_decomposed_amount(uint64_t amount) {
    auto it = std::lower_bound(
        Constants::PRETTY_AMOUNTS.begin(),
        Constants::PRETTY_AMOUNTS.end(), amount
    );
    if (it == Constants::PRETTY_AMOUNTS.end() || amount != *it) {
        return false;
    }
    return true;
}

} // namespace QwertyNote
