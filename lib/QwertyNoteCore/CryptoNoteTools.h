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

#include <limits>

#include <Common/MemoryInputStream.h>
#include <Common/StringTools.h>
#include <Common/VectorOutputStream.h>

#include <QwertyNoteCore/CryptoNoteSerialization.h>

#include <Global/Constants.h>

#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/BinaryOutputStreamSerializer.h>

using namespace Qwertycoin;

namespace QwertyNote {

void getBinaryArrayHash(const BinaryArray &binaryArray, Crypto::FHash &hash);
Crypto::FHash getBinaryArrayHash(const BinaryArray &binaryArray);

template<class T>
bool toBinaryArray(const T &object, BinaryArray &binaryArray)
{
    try {
        ::Common::QVectorOutputStream stream(binaryArray);
        BinaryOutputStreamSerializer serializer(stream);
        serialize(const_cast<T &>(object), serializer);
    } catch (std::exception &) {
        return false;
    }

    return true;
}

template<>
bool toBinaryArray(const BinaryArray &object, BinaryArray &binaryArray);

template<class T>
BinaryArray toBinaryArray(const T &object)
{
    BinaryArray ba;
    toBinaryArray(object, ba);

    return ba;
}

template<class T>
bool fromBinaryArray(T &object, const BinaryArray &binaryArray)
{
    bool result = false;
    try {
        Common::QMemoryInputStream stream(binaryArray.data(), binaryArray.size());
        BinaryInputStreamSerializer serializer(stream);
        serialize(object, serializer);
        result = stream.endOfStream(); // check that all uData was consumed
    } catch (std::exception &) {
        // do nothing
    }

    return result;
}

template<class T>
bool getObjectBinarySize(const T &object, size_t &size)
{
    BinaryArray ba;
    if (!toBinaryArray(object, ba)) {
        size = (std::numeric_limits<size_t>::max)();
        return false;
    }

    size = ba.size();

    return true;
}

template<class T>
size_t getObjectBinarySize(const T &object)
{
    size_t size;
    getObjectBinarySize(object, size);

    return size;
}

template<class T>
bool getObjectHash(const T &object, Crypto::FHash &hash)
{
    BinaryArray ba;
    if (!toBinaryArray(object, ba)) {
        hash = NULL_HASH;
        return false;
    }

    hash = getBinaryArrayHash(ba);

    return true;
}

template<class T>
bool getObjectHash(const T &object, Crypto::FHash &hash, size_t &size)
{
    BinaryArray ba;
    if (!toBinaryArray(object, ba)) {
        hash = NULL_HASH;
        size = (std::numeric_limits<size_t>::max)();
        return false;
    }

    size = ba.size();
    hash = getBinaryArrayHash(ba);

    return true;
}

template<class T>
Crypto::FHash getObjectHash(const T &object)
{
    Crypto::FHash hash;
    getObjectHash(object, hash);

    return hash;
}

uint64_t getInputAmount(const FTransaction &transaction);
std::vector<uint64_t> getInputsAmounts(const FTransaction &transaction);
uint64_t getOutputAmount(const FTransaction &transaction);
void decomposeAmount(
    uint64_t amount,
    uint64_t dustThreshold,
    std::vector<uint64_t> &decomposedAmounts);

} // namespace QwertyNote
