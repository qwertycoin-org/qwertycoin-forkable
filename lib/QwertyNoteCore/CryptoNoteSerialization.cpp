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

#include <Crypto/Crypto.h>

#include <Global/QwertyNoteConfig.h>

#include <QwertyNoteCore/Account.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/TransactionExtra.h>

#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/BinaryOutputStreamSerializer.h>
#include <Serialization/ISerializer.h>
#include <Serialization/SerializationOverloads.h>

namespace {

using namespace QwertyNote;
using namespace Common;

size_t getSignaturesCount(const FTransactionInput &input)
{
    struct txin_signature_size_visitor : public boost::static_visitor < size_t >
    {
        size_t operator()(const FBaseInput &txin) const { return 0; }
        size_t operator()(const FKeyInput &txin) const { return txin.vOutputIndexes.size(); }
        size_t operator()(const FMultiSignatureInput &txin) const { return txin.uSignatureCount; }
    };

    return boost::apply_visitor(txin_signature_size_visitor(), input);
}

struct BinaryVariantTagGetter : boost::static_visitor<uint8_t>
{
    uint8_t operator()(const QwertyNote::FBaseInput) { return  0xff; }
    uint8_t operator()(const QwertyNote::FKeyInput) { return  0x2; }
    uint8_t operator()(const QwertyNote::FMultiSignatureInput) { return  0x3; }
    uint8_t operator()(const QwertyNote::FKeyOutput) { return  0x2; }
    uint8_t operator()(const QwertyNote::FMultiSignatureOutput) { return  0x3; }
    uint8_t operator()(const QwertyNote::FTransaction) { return  0xcc; }
    uint8_t operator()(const QwertyNote::FBlock) { return  0xbb; }
};

struct VariantSerializer : boost::static_visitor<>
{
    VariantSerializer(QwertyNote::ISerializer &serializer, const std::string &name)
        : s(serializer),
          name(name)
    {
    }

    template <typename T>
    void operator() (T &param) { s(param, name); }

    QwertyNote::ISerializer &s;
    std::string name;
};

void getVariantValue(QwertyNote::ISerializer&serializer,uint8_t tag,
                     QwertyNote::FTransactionInput&in)
{
    switch(tag) {
    case 0xff: {
        QwertyNote::FBaseInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    case 0x2: {
        QwertyNote::FKeyInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    case 0x3: {
        QwertyNote::FMultiSignatureInput v;
        serializer(v, "value");
        in = v;
        break;
    }
    default:
        throw std::runtime_error("Unknown variant tag");
    }
}

void getVariantValue(QwertyNote::ISerializer &serializer,
    uint8_t tag,
                     QwertyNote::FTransactionOutputTarget &out)
{
    switch(tag) {
    case 0x2: {
        QwertyNote::FKeyOutput v;
        serializer(v, "uData");
        out = v;
        break;
    }
    case 0x3: {
        QwertyNote::FMultiSignatureOutput v;
        serializer(v, "uData");
        out = v;
        break;
    }
    default:
        throw std::runtime_error("Unknown variant tag");
    }
}

template <typename T>
bool serializePod(T &v, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializer.binary(&v, sizeof(v), name);
}

bool serializeVarintVector(
    std::vector<uint32_t> &vector, QwertyNote::ISerializer &serializer,
    Common::QStringView name)
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

bool serialize(FPublicKey &pubKey, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(pubKey, name, serializer);
}

bool serialize(FSecretKey &secKey, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(secKey, name, serializer);
}

bool serialize(FHash &h, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(h, name, serializer);
}

bool serialize(FKeyImage &keyImage, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(keyImage, name, serializer);
}

bool serialize(Chacha8Iv &chacha, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(chacha, name, serializer);
}

bool serialize(FSignature &sig, Common::QStringView name, QwertyNote::ISerializer &serializer)
{
    return serializePod(sig, name, serializer);
}

bool serialize(
		FEllipticCurveScalar &ecScalar,
		Common::QStringView name,
		QwertyNote::ISerializer &serializer)
{
    return serializePod(ecScalar, name, serializer);
}

bool serialize(
		FEllipticCurvePoint &ecPoint,
		Common::QStringView name,
		QwertyNote::ISerializer &serializer)
{
    return serializePod(ecPoint, name, serializer);
}

} // namespace Crypto

namespace QwertyNote {

void serialize(FTransactionPrefix &txP, ISerializer &serializer)
{
    serializer(txP.uVersion, "version");

    if (CURRENT_TRANSACTION_VERSION < txP.uVersion) {
        throw std::runtime_error("Wrong transaction version");
    }

    serializer(txP.uUnlockTime, "unlock_time");
    serializer(txP.vInputs, "vin");
    serializer(txP.vOutputs, "vout");
    serializeAsBinary(txP.vExtra, "extra", serializer);
}

void serialize(FTransaction &tx, ISerializer &serializer)
{
    serialize(static_cast<FTransactionPrefix&>(tx), serializer);

    size_t sigSize = tx.vInputs.size();
    // TODO: make arrays without sizes
    //serializer.beginArray(sigSize, "signatures");

    // ignore base transaction
    if (serializer.type() == ISerializer::INPUT
        && !(sigSize == 1 && tx.vInputs[0].type() == typeid(FBaseInput))) {
        tx.vSignatures.resize(sigSize);
    }

    bool signaturesNotExpected = tx.vSignatures.empty();
    if (!signaturesNotExpected && tx.vInputs.size() != tx.vSignatures.size()) {
        throw std::runtime_error("Serialization error: unexpected signatures size");
    }

    for (size_t i = 0; i < tx.vInputs.size(); ++i) {
        size_t signatureSize = getSignaturesCount(tx.vInputs[i]);
        if (signaturesNotExpected) {
            if (signatureSize == 0) {
                continue;
            } else {
                throw std::runtime_error("Serialization error: signatures are not expected");
            }
        }

        if (serializer.type() == ISerializer::OUTPUT) {
            if (signatureSize != tx.vSignatures[i].size()) {
                throw std::runtime_error("Serialization error: unexpected signatures size");
            }

            for (Crypto::FSignature& sig : tx.vSignatures[i]) {
                serializePod(sig, "", serializer);
            }
        } else {
            std::vector<Crypto::FSignature> signatures(signatureSize);
            for (Crypto::FSignature &sig : signatures) {
                serializePod(sig, "", serializer);
            }

            tx.vSignatures[i] = std::move(signatures);
        }
    }
    //serializer.endArray();
}

void serialize(FTransactionInput &in, ISerializer &serializer)
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

void serialize(FBaseInput &gen, ISerializer &serializer)
{
    serializer(gen.uBlockIndex, "height");
}

void serialize(FKeyInput &key, ISerializer &serializer)
{
    serializer(key.uAmount, "amount");
    serializeVarintVector(key.vOutputIndexes, serializer, "key_offsets");
    serializer(key.sKeyImage, "k_image");
}

void serialize(FMultiSignatureInput &multisignature, ISerializer &serializer)
{
    serializer(multisignature.uAmount, "amount");
    serializer(multisignature.uSignatureCount, "signatures");
    serializer(multisignature.uOutputIndex, "outputIndex");
}

void serialize(FTransactionOutput &output, ISerializer &serializer)
{
    serializer(output.uAmount, "amount");
    serializer(output.sTarget, "target");
}

void serialize(FTransactionOutputTarget &output, ISerializer &serializer)
{
    if (serializer.type() == ISerializer::OUTPUT) {
        BinaryVariantTagGetter tagGetter;
        uint8_t tag = boost::apply_visitor(tagGetter, output);
        serializer.binary(&tag, sizeof(tag), "type");

        VariantSerializer visitor(serializer, "uData");
        boost::apply_visitor(visitor, output);
    } else {
        uint8_t tag;
        serializer.binary(&tag, sizeof(tag), "type");

        getVariantValue(serializer, tag, output);
    }
}

void serialize(FKeyOutput &key, ISerializer &serializer)
{
    serializer(key.sPublicKey, "key");
}

void serialize(FMultiSignatureOutput &multisignature, ISerializer &serializer)
{
    serializer(multisignature.vPublicKeys, "keys");
    serializer(multisignature.uRequiredSignatureCount, "required_signatures");
}

void serializeBlockHeader(FBlockHeader &header, ISerializer &serializer)
{
    serializer(header.uMajorVersion, "major_version");
    if (header.uMajorVersion > BLOCK_MAJOR_VERSION_6) {
        throw std::runtime_error("Wrong major version");
    }

    serializer(header.uMinorVersion, "minor_version");

    if (header.uMajorVersion >= BLOCK_MAJOR_VERSION_1) {
        serializer(header.uTimestamp, "timestamp");
        serializer(header.sPreviousBlockHash, "prev_id");
        serializer.binary(&header.uNonce, sizeof(header.uNonce), "nonce");
    } else {
        throw std::runtime_error("Wrong major version");
    }
}

void serialize(FBlockHeader &header, ISerializer &serializer)
{
    serializeBlockHeader(header, serializer);
}

void serialize(FBlock &block, ISerializer &serializer)
{
    serializeBlockHeader(block, serializer);
    serializer(block.sBaseTransaction, "miner_tx");
    serializer(block.vTransactionHashes, "tx_hashes");
}

void serialize(FAccountPublicAddress &address, ISerializer &serializer)
{
    serializer(address.sSpendPublicKey, "m_spend_public_key");
    serializer(address.sViewPublicKey, "m_view_public_key");
}

void serialize(FAccountKeys &keys, ISerializer &s)
{
    s(keys.sAddress, "m_account_address");
    s(keys.sSpendSecretKey, "m_spend_secret_key");
    s(keys.sViewSecretKey, "m_view_secret_key");
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
        QStringOutputStream os(field);
        BinaryOutputStreamSerializer output(os);
        doSerialize(tag, output);
        serializer(field, "");
    } else {
        std::string field;
        serializer(field, "");
        QMemoryInputStream stream(field.data(), field.size());
        BinaryInputStreamSerializer input(stream);
        doSerialize(tag, input);
    }
}

void serialize(FKeyPair &keyPair, ISerializer &serializer)
{
    serializer(keyPair.sSecretKey, "secret_key");
    serializer(keyPair.sPublicKey, "public_key");
}

} // namespace QwertyNote
