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

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <Common/StringOutputStream.h>
#include <crypto/Crypto.h>
#include <CryptoNoteCore/Account.h>
#include <CryptoNoteCore/CryptoNoteFormatUtils.h>
#include <CryptoNoteCore/CryptoNoteSerialization.h>
#include <CryptoNoteCore/CryptoNoteTools.h>
#include <CryptoNoteCore/TransactionExtra.h>
#include <Global/CryptoNoteConfig.h>
#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/BinaryOutputStreamSerializer.h>
#include <Serialization/ISerializer.h>
#include <Serialization/SerializationOverloads.h>

namespace {

using namespace CryptoNote;
using namespace Common;

size_t getSignaturesCount(const TransactionInput &input)
{
    struct txin_signature_size_visitor : public boost::static_visitor < size_t >
    {
        size_t operator()(const BaseInput &txin) const { return 0; }
        size_t operator()(const KeyInput &txin) const { return txin.outputIndexes.size(); }
        size_t operator()(const MultisignatureInput &txin) const { return txin.signatureCount; }
    };

    return boost::apply_visitor(txin_signature_size_visitor(), input);
}

struct BinaryVariantTagGetter : boost::static_visitor<uint8_t>
{
    uint8_t operator()(const CryptoNote::BaseInput) { return  0xff; }
    uint8_t operator()(const CryptoNote::KeyInput) { return  0x2; }
    uint8_t operator()(const CryptoNote::MultisignatureInput) { return  0x3; }
    uint8_t operator()(const CryptoNote::KeyOutput) { return  0x2; }
    uint8_t operator()(const CryptoNote::MultisignatureOutput) { return  0x3; }
    uint8_t operator()(const CryptoNote::Transaction) { return  0xcc; }
    uint8_t operator()(const CryptoNote::Block) { return  0xbb; }
};

struct VariantSerializer : boost::static_visitor<>
{
    VariantSerializer(CryptoNote::ISerializer &serializer, const std::string &name)
        : s(serializer),
          name(name)
    {
    }

    template <typename T>
    void operator() (T &param) { s(param, name); }

    CryptoNote::ISerializer &s;
    std::string name;
};

void getVariantValue(CryptoNote::ISerializer&serializer,uint8_t tag,CryptoNote::TransactionInput&in)
{
    switch(tag) {
    case 0xff: {
        CryptoNote::BaseInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    case 0x2: {
        CryptoNote::KeyInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    case 0x3: {
        CryptoNote::MultisignatureInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    default:
        throw std::runtime_error("Unknown variant tag");
    }
}

void getVariantValue(
    CryptoNote::ISerializer &serializer,
    uint8_t tag,
    CryptoNote::TransactionOutputTarget &out)
{
    switch(tag) {
    case 0x2: {
        CryptoNote::KeyOutput v;
        serializer(v, "data");
        out = v;
        break;
    }
    case 0x3: {
        CryptoNote::MultisignatureOutput v;
        serializer(v, "data");
        out = v;
        break;
    }
    default:
        throw std::runtime_error("Unknown variant tag");
    }
}

template <typename T>
bool serializePod(T &v, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializer.binary(&v, sizeof(v), name);
}

bool serializeVarintVector(
    std::vector<uint32_t> &vector,
    CryptoNote::ISerializer &serializer,
    Common::StringView name)
{
    size_t size = vector.size();

    if (!serializer.beginArray(size, name)) {
        vector.clear();
        return false;
    }

    vector.resize(size);

    for (size_t i = 0; i < size; ++i) {
        serializer(vector[i], "");
    }

    serializer.endArray();

    return true;
}

} // namespace

namespace Crypto {

bool serialize(PublicKey &pubKey, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(pubKey, name, serializer);
}

bool serialize(SecretKey &secKey, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(secKey, name, serializer);
}

bool serialize(Hash &h, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(h, name, serializer);
}

bool serialize(KeyImage &keyImage, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(keyImage, name, serializer);
}

bool serialize(chacha8_iv &chacha, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(chacha, name, serializer);
}

bool serialize(Signature &sig, Common::StringView name, CryptoNote::ISerializer &serializer)
{
    return serializePod(sig, name, serializer);
}

bool serialize(
    EllipticCurveScalar &ecScalar,
    Common::StringView name,
    CryptoNote::ISerializer &serializer)
{
    return serializePod(ecScalar, name, serializer);
}

bool serialize(
    EllipticCurvePoint &ecPoint,
    Common::StringView name,
    CryptoNote::ISerializer &serializer)
{
    return serializePod(ecPoint, name, serializer);
}

} // namespace crypto

namespace CryptoNote {

void serialize(TransactionPrefix &txP, ISerializer &serializer)
{
    serializer(txP.version, "version");

    if (CURRENT_TRANSACTION_VERSION < txP.version) {
        throw std::runtime_error("Wrong transaction version");
    }

    serializer(txP.unlockTime, "unlock_time");
    serializer(txP.inputs, "vin");
    serializer(txP.outputs, "vout");
    serializeAsBinary(txP.extra, "extra", serializer);
}

void serialize(Transaction &tx, ISerializer &serializer)
{
    serialize(static_cast<TransactionPrefix&>(tx), serializer);

    size_t sigSize = tx.inputs.size();
    // TODO: make arrays without sizes
    //serializer.beginArray(sigSize, "signatures");

    // ignore base transaction
    if (serializer.type() == ISerializer::INPUT
        && !(sigSize == 1 && tx.inputs[0].type() == typeid(BaseInput))) {
        tx.signatures.resize(sigSize);
    }

    bool signaturesNotExpected = tx.signatures.empty();
    if (!signaturesNotExpected && tx.inputs.size() != tx.signatures.size()) {
        throw std::runtime_error("Serialization error: unexpected signatures size");
    }

    for (size_t i = 0; i < tx.inputs.size(); ++i) {
        size_t signatureSize = getSignaturesCount(tx.inputs[i]);
        if (signaturesNotExpected) {
            if (signatureSize == 0) {
                continue;
            } else {
                throw std::runtime_error("Serialization error: signatures are not expected");
            }
        }

        if (serializer.type() == ISerializer::OUTPUT) {
            if (signatureSize != tx.signatures[i].size()) {
                throw std::runtime_error("Serialization error: unexpected signatures size");
            }

            for (Crypto::Signature& sig : tx.signatures[i]) {
                serializePod(sig, "", serializer);
            }
        } else {
            std::vector<Crypto::Signature> signatures(signatureSize);
            for (Crypto::Signature &sig : signatures) {
                serializePod(sig, "", serializer);
            }

            tx.signatures[i] = std::move(signatures);
        }
    }
    //serializer.endArray();
}

void serialize(TransactionInput &in, ISerializer &serializer)
{
    if (serializer.type() == ISerializer::OUTPUT) {
        BinaryVariantTagGetter tagGetter;
        uint8_t tag = boost::apply_visitor(tagGetter, in);
        serializer.binary(&tag, sizeof(tag), "type");

        VariantSerializer visitor(serializer, "value");
        boost::apply_visitor(visitor, in);
    } else {
        uint8_t tag;
        serializer.binary(&tag, sizeof(tag), "type");

        getVariantValue(serializer, tag, in);
    }
}

void serialize(BaseInput &gen, ISerializer &serializer)
{
    serializer(gen.blockIndex, "height");
}

void serialize(KeyInput &key, ISerializer &serializer)
{
    serializer(key.amount, "amount");
    serializeVarintVector(key.outputIndexes, serializer, "key_offsets");
    serializer(key.keyImage, "k_image");
}

void serialize(MultisignatureInput &multisignature, ISerializer &serializer)
{
    serializer(multisignature.amount, "amount");
    serializer(multisignature.signatureCount, "signatures");
    serializer(multisignature.outputIndex, "outputIndex");
}

void serialize(TransactionOutput &output, ISerializer &serializer)
{
    serializer(output.amount, "amount");
    serializer(output.target, "target");
}

void serialize(TransactionOutputTarget &output, ISerializer &serializer)
{
    if (serializer.type() == ISerializer::OUTPUT) {
        BinaryVariantTagGetter tagGetter;
        uint8_t tag = boost::apply_visitor(tagGetter, output);
        serializer.binary(&tag, sizeof(tag), "type");

        VariantSerializer visitor(serializer, "data");
        boost::apply_visitor(visitor, output);
    } else {
        uint8_t tag;
        serializer.binary(&tag, sizeof(tag), "type");

        getVariantValue(serializer, tag, output);
    }
}

void serialize(KeyOutput &key, ISerializer &serializer)
{
    serializer(key.key, "key");
}

void serialize(MultisignatureOutput &multisignature, ISerializer &serializer)
{
    serializer(multisignature.keys, "keys");
    serializer(multisignature.requiredSignatureCount, "required_signatures");
}

void serializeBlockHeader(BlockHeader &header, ISerializer &serializer)
{
    serializer(header.majorVersion, "major_version");
    if (header.majorVersion > BLOCK_MAJOR_VERSION_6) {
        throw std::runtime_error("Wrong major version");
    }

    serializer(header.minorVersion, "minor_version");

    if (header.majorVersion >= BLOCK_MAJOR_VERSION_1) {
        serializer(header.timestamp, "timestamp");
        serializer(header.previousBlockHash, "prev_id");
        serializer.binary(&header.nonce, sizeof(header.nonce), "nonce");
    } else {
        throw std::runtime_error("Wrong major version");
    }
}

void serialize(BlockHeader &header, ISerializer &serializer)
{
    serializeBlockHeader(header, serializer);
}

void serialize(Block &block, ISerializer &serializer)
{
    serializeBlockHeader(block, serializer);
    serializer(block.baseTransaction, "miner_tx");
    serializer(block.transactionHashes, "tx_hashes");
}

void serialize(AccountPublicAddress &address, ISerializer &serializer)
{
    serializer(address.spendPublicKey, "m_spend_public_key");
    serializer(address.viewPublicKey, "m_view_public_key");
}

void serialize(AccountKeys &keys, ISerializer &s)
{
    s(keys.address, "m_account_address");
    s(keys.spendSecretKey, "m_spend_secret_key");
    s(keys.viewSecretKey, "m_view_secret_key");
}

void doSerialize(TransactionExtraMergeMiningTag &tag, ISerializer &serializer)
{
    auto depth = static_cast<uint64_t>(tag.depth);
    serializer(depth, "depth");
    tag.depth = static_cast<size_t>(depth);
    serializer(tag.merkleRoot, "merkle_root");
}

void serialize(TransactionExtraMergeMiningTag &tag, ISerializer &serializer)
{
    if (serializer.type() == ISerializer::OUTPUT) {
        std::string field;
        StringOutputStream os(field);
        BinaryOutputStreamSerializer output(os);
        doSerialize(tag, output);
        serializer(field, "");
    } else {
        std::string field;
        serializer(field, "");
        MemoryInputStream stream(field.data(), field.size());
        BinaryInputStreamSerializer input(stream);
        doSerialize(tag, input);
    }
}

void serialize(KeyPair &keyPair, ISerializer &serializer)
{
    serializer(keyPair.secretKey, "secret_key");
    serializer(keyPair.publicKey, "public_key");
}

} // namespace CryptoNote
