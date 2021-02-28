// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2016-2017 BXC developers
// Copyright (c) 2017 UltraNote developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <Common/StreamTools.h>
#include <Serialization/BinaryInputStreamSerializer.h>
#include <Serialization/SerializationOverloads.h>

using namespace Common;

namespace QwertyNote {

namespace {

template<typename StorageType, typename T>
void readVarintAs(IInputStream &s, T &i)
{
    i = static_cast<T>(readVarint<StorageType>(s));
}

} // namespace

ISerializer::SerializerType BinaryInputStreamSerializer::type() const
{
    return ISerializer::INPUT;
}

bool BinaryInputStreamSerializer::beginObject(Common::QStringView /*name*/)
{
    return true;
}

void BinaryInputStreamSerializer::endObject()
{
}

bool BinaryInputStreamSerializer::beginArray(size_t &size, Common::QStringView /*name*/)
{
    readVarintAs<uint64_t>(stream, size);

    return true;
}

void BinaryInputStreamSerializer::endArray()
{
}

bool BinaryInputStreamSerializer::operator()(uint8_t &value, Common::QStringView /*name*/)
{
    readVarint(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(uint16_t &value, Common::QStringView /*name*/)
{
    readVarint(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(int16_t &value, Common::QStringView /*name*/)
{
    readVarintAs<uint16_t>(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(uint32_t &value, Common::QStringView /*name*/)
{
    readVarint(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(int32_t &value, Common::QStringView /*name*/)
{
    readVarintAs<uint32_t>(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(int64_t &value, Common::QStringView /*name*/)
{
    readVarintAs<uint64_t>(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(uint64_t &value, Common::QStringView /*name*/)
{
    readVarint(stream, value);

    return true;
}

bool BinaryInputStreamSerializer::operator()(bool &value, Common::QStringView /*name*/)
{
    value = read<uint8_t>(stream) != 0;

    return true;
}

bool BinaryInputStreamSerializer::operator()(std::string &value, Common::QStringView /*name*/)
{
    uint64_t size;
    readVarint(stream, size);

    if (size > 0) {
        std::vector<char> temp;
        temp.resize(size);
        checkedRead(&temp[0], size);
        value.reserve(size);
        value.assign(&temp[0], size);
    } else {
        value.clear();
    }

    return true;
}

bool BinaryInputStreamSerializer::binary(void *value, size_t size, Common::QStringView /*name*/)
{
    checkedRead(static_cast<char *>(value), size);

    return true;
}

bool BinaryInputStreamSerializer::binary(std::string &value, Common::QStringView name)
{
    return (*this)(value, name);
}

bool BinaryInputStreamSerializer::operator()(double &value, Common::QStringView /*name*/)
{
    assert(false); // method is not supported for this type of serialization
    throw std::runtime_error("double serialization is not supported");
}

void BinaryInputStreamSerializer::checkedRead(char *buf, size_t size)
{
    read(stream, buf, size);
}

} // namespace QwertyNote
