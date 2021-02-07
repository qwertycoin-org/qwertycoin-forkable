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
// Parts of this file are copyright (c) 2016-2018, The Monero Project

#include <Common/Base58.h>
#include <Common/IntUtil.h>
#include <Crypto/Hash.h>

#include <QwertyNoteCore/CryptoNoteBasicImpl.h>
#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteTools.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>

using namespace Crypto;
using namespace Common;

namespace QwertyNote {

uint64_t getPenalizedAmount(uint64_t amount, size_t medianSize, size_t currentBlockSize)
{
    static_assert(sizeof(size_t) >= sizeof(uint32_t), "size_t is too small");

    assert(currentBlockSize <= 2 * medianSize);
    assert(medianSize <= std::numeric_limits<uint32_t>::max());
    assert(currentBlockSize <= std::numeric_limits<uint32_t>::max());

    if (amount == 0) {
        return 0;
    }

    if (currentBlockSize <= medianSize) {
        return amount;
    }

    uint64_t productHi;
    // BUGFIX by Monero Project: 32-bit saturation bug (e.g. ARM7),
    // the result was being treated as 32-bit by default
    uint64_t multiplicand = UINT64_C(2) * medianSize - currentBlockSize;
    multiplicand *= currentBlockSize;
    uint64_t productLo = mul128(amount, multiplicand, &productHi);

    uint64_t penalizedAmountHi;
    uint64_t penalizedAmountLo;
    div128_32(
        productHi,
        productLo,
        static_cast<uint32_t>(medianSize),
        &penalizedAmountHi,
        &penalizedAmountLo);
    div128_32(
        penalizedAmountHi,
        penalizedAmountLo,
        static_cast<uint32_t>(medianSize),
        &penalizedAmountHi,
        &penalizedAmountLo);

    assert(0 == penalizedAmountHi);
    assert(penalizedAmountLo < amount);

    return penalizedAmountLo;
}

std::string getAccountAddressAsStr(uint64_t prefix, const FAccountPublicAddress &adr)
{
    BinaryArray ba;
    bool r = toBinaryArray(adr, ba);
    assert(r);
    return Tools::Base58::encodeAddr(prefix, Common::asString(ba));
}

bool is_coinbase(const FTransaction &tx)
{
    if(tx.vInputs.size() != 1) {
        return false;
    }

    if(tx.vInputs[0].type() != typeid(FBaseInput)) {
        return false;
    }

    return true;
}

bool parseAccountAddressString(uint64_t& prefix, FAccountPublicAddress& adr, const std::string& str)
{
    std::string data;

    return Tools::Base58::decodeAddr(str, prefix, data)
           && fromBinaryArray(adr, asBinaryArray(data))
           && checkKey(adr.sSpendPublicKey)
           && checkKey(adr.sViewPublicKey);
}

bool operator ==(const QwertyNote::FTransaction &a, const QwertyNote::FTransaction &b)
{
    return getObjectHash(a) == getObjectHash(b);
}

bool operator ==(const QwertyNote::FBlock &a, const QwertyNote::FBlock &b)
{
    return QwertyNote::getBlockHash(a) == QwertyNote::getBlockHash(b);
}

} // namespace QwertyNote

bool parse_hash256(const std::string &str_hash, Crypto::FHash &hash)
{
    return Common::podFromHex(str_hash, hash);
}
