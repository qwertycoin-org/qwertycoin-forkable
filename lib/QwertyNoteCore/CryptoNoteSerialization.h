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

#include <Crypto/Chacha8.h>
#include <Crypto/Crypto.h>

#include <QwertyNoteCore/CryptoNoteBasic.h>

#include <Serialization/ISerializer.h>

namespace CN = QwertyNote;

namespace Crypto {

bool serialize(FPublicKey &pubKey, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FSecretKey &secKey, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FHash &h, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(Chacha8Iv &chacha, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FKeyImage& keyImage, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FSignature &sig, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FEllipticCurveScalar &ecScalar, Common::QStringView name, CN::ISerializer &serializer);
bool serialize(FEllipticCurvePoint &ecPoint, Common::QStringView name, CN::ISerializer &serializer);

} // namespace Crypto

namespace QwertyNote {

struct FAccountKeys;
struct TransactionExtraMergeMiningTag;

void serialize(FTransactionPrefix &txP, ISerializer &serializer);
void serialize(FTransaction &tx, ISerializer &serializer);
void serialize(FTransactionInput &in, ISerializer &serializer);
void serialize(FTransactionOutput &in, ISerializer &serializer);

void serialize(FBaseInput &gen, ISerializer &serializer);
void serialize(FKeyInput &key, ISerializer &serializer);
void serialize(FMultiSignatureInput &multisignature, ISerializer &serializer);

void serialize(FTransactionOutput &output, ISerializer &serializer);
void serialize(FTransactionOutputTarget &output, ISerializer &serializer);
void serialize(FKeyOutput &key, ISerializer &serializer);
void serialize(FMultiSignatureOutput &multisignature, ISerializer &serializer);

void serialize(FBlockHeader &header, ISerializer &serializer);
void serialize(FBlock &block, ISerializer &serializer);
void serialize(TransactionExtraMergeMiningTag& tag, ISerializer& serializer);

void serialize(FAccountPublicAddress &address, ISerializer &serializer);
void serialize(FAccountKeys &keys, ISerializer &s);

void serialize(FKeyPair &keyPair, ISerializer &serializer);

} // namespace QwertyNote
