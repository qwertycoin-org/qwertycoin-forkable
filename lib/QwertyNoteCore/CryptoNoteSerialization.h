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

bool serialize(FPublicKey &pubKey, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FSecretKey &secKey, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FHash &h, Common::StringView name, CN::ISerializer &serializer);
bool serialize(Chacha8Iv &chacha, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FKeyImage& keyImage, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FSignature &sig, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FEllipticCurveScalar &ecScalar, Common::StringView name, CN::ISerializer &serializer);
bool serialize(FEllipticCurvePoint &ecPoint, Common::StringView name, CN::ISerializer &serializer);

} // namespace Crypto

namespace QwertyNote {

struct AccountKeys;
struct TransactionExtraMergeMiningTag;

void serialize(TransactionPrefix &txP, ISerializer &serializer);
void serialize(Transaction &tx, ISerializer &serializer);
void serialize(TransactionInput &in, ISerializer &serializer);
void serialize(TransactionOutput &in, ISerializer &serializer);

void serialize(BaseInput &gen, ISerializer &serializer);
void serialize(KeyInput &key, ISerializer &serializer);
void serialize(MultiSignatureInput &multisignature, ISerializer &serializer);

void serialize(TransactionOutput &output, ISerializer &serializer);
void serialize(TransactionOutputTarget &output, ISerializer &serializer);
void serialize(KeyOutput &key, ISerializer &serializer);
void serialize(MultiSignatureOutput &multisignature, ISerializer &serializer);

void serialize(BlockHeader &header, ISerializer &serializer);
void serialize(Block &block, ISerializer &serializer);
void serialize(TransactionExtraMergeMiningTag& tag, ISerializer& serializer);

void serialize(AccountPublicAddress &address, ISerializer &serializer);
void serialize(AccountKeys &keys, ISerializer &s);

void serialize(KeyPair &keyPair, ISerializer &serializer);

} // namespace QwertyNote
