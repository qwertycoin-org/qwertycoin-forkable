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
#include <cstring>
#include <stdexcept>
#include <Common/StreamTools.h>
#include <Serialization/KVBinaryCommon.h>
#include <Serialization/KVBinaryInputStreamSerializer.h>

using namespace Common;
using namespace QwertyNote;

namespace {

template <typename T>
T readPod(Common::IInputStream &s)
{
    T v;
    read(s, &v, sizeof(T));

    return v;
}

template <typename T, typename JsonT = T>
QJsonValue readPodJson(Common::IInputStream &s)
{
    QJsonValue jv;
    jv = static_cast<JsonT>(readPod<T>(s));

    return jv;
}

template <typename T>
QJsonValue readIntegerJson(Common::IInputStream &s)
{
    return readPodJson<T, int64_t>(s);
}

size_t readVarint(Common::IInputStream &s)
{
    auto b = read<uint8_t>(s);
    uint8_t size_mask = b & PORTABLE_RAW_SIZE_MARK_MASK;
    size_t bytesLeft = 0;

    switch (size_mask) {
    case PORTABLE_RAW_SIZE_MARK_BYTE:
        bytesLeft = 0;
        break;
    case PORTABLE_RAW_SIZE_MARK_WORD:
        bytesLeft = 1;
        break;
    case PORTABLE_RAW_SIZE_MARK_DWORD:
        bytesLeft = 3;
        break;
    case PORTABLE_RAW_SIZE_MARK_INT64:
        bytesLeft = 7;
        break;
    default:
        // do nothing
        break;
    }

    size_t value = b;

    for (size_t i = 1; i <= bytesLeft; ++i) {
        size_t n = read<uint8_t>(s);
        value |= n << (i * 8);
    }

    value >>= 2;

    return value;
}

std::string readString(Common::IInputStream &s)
{
    auto size = readVarint(s);

    std::string str;
    str.resize(size);
    if (size) {
        read(s, &str[0], size);
    }

    return str;
}

QJsonValue readStringJson(Common::IInputStream &s)
{
    return QJsonValue(readString(s));
}

void readName(Common::IInputStream &s, std::string &name)
{
    auto len = readPod<uint8_t>(s);
    if (len) {
        name.resize(len);
        read(s, &name[0], len);
    }
}

QJsonValue loadValue(Common::IInputStream &stream, uint8_t type);
QJsonValue loadSection(Common::IInputStream &stream);
QJsonValue loadEntry(Common::IInputStream &stream);
QJsonValue loadArray(Common::IInputStream &stream, uint8_t itemType);

QJsonValue loadSection(Common::IInputStream &stream)
{
    QJsonValue sec(QJsonValue::OBJECT);
    size_t count = readVarint(stream);
    std::string name;

    while (count--) {
        readName(stream, name);
        sec.insert(name, loadEntry(stream));
    }

    return sec;
}

QJsonValue loadValue(Common::IInputStream &stream, uint8_t type)
{
    switch (type) {
    case BIN_KV_SERIALIZE_TYPE_INT64:
        return readIntegerJson<int64_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_INT32:
        return readIntegerJson<int32_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_INT16:
        return readIntegerJson<int16_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_INT8:
        return readIntegerJson<int8_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_UINT64:
        return readIntegerJson<uint64_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_UINT32:
        return readIntegerJson<uint32_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_UINT16:
        return readIntegerJson<uint16_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_UINT8:
        return readIntegerJson<uint8_t>(stream);
    case BIN_KV_SERIALIZE_TYPE_DOUBLE:
        return readPodJson<double>(stream);
    case BIN_KV_SERIALIZE_TYPE_BOOL:
        return QJsonValue(read<uint8_t>(stream) != 0);
    case BIN_KV_SERIALIZE_TYPE_STRING:
        return readStringJson(stream);
    case BIN_KV_SERIALIZE_TYPE_OBJECT:
        return loadSection(stream);
    case BIN_KV_SERIALIZE_TYPE_ARRAY:
        return loadArray(stream, type);
    default:
        throw std::runtime_error("Unknown data type");
    }
}

QJsonValue loadEntry(Common::IInputStream &stream)
{
    auto type = readPod<uint8_t>(stream);

    if (type & BIN_KV_SERIALIZE_FLAG_ARRAY) {
        type &= ~BIN_KV_SERIALIZE_FLAG_ARRAY;
        return loadArray(stream, type);
    }

    return loadValue(stream, type);
}

QJsonValue loadArray(Common::IInputStream &stream, uint8_t itemType)
{
    QJsonValue arr(QJsonValue::ARRAY);
    size_t count = readVarint(stream);

    while (count--) {
        arr.pushBack(loadValue(stream, itemType));
    }

    return arr;
}

QJsonValue parseBinary(Common::IInputStream &stream)
{
    auto hdr = readPod<KVBinaryStorageBlockHeader>(stream);

    if (hdr.m_signature_a != PORTABLE_STORAGE_SIGNATUREA
        || hdr.m_signature_b != PORTABLE_STORAGE_SIGNATUREB) {
        throw std::runtime_error("Invalid binary storage signature");
    }

    if (hdr.m_ver != PORTABLE_STORAGE_FORMAT_VER) {
        throw std::runtime_error("Unknown binary storage format version");
    }

    return loadSection(stream);
}

} // namespace

KVBinaryInputStreamSerializer::KVBinaryInputStreamSerializer(Common::IInputStream &strm)
    : JsonInputValueSerializer(parseBinary(strm))
{
}

bool KVBinaryInputStreamSerializer::binary(void *value, size_t size, Common::QStringView name)
{
    std::string str;

    if (!(*this)(str, name)) {
        return false;
    }

    if (str.size() != size) {
        throw std::runtime_error("Binary block size mismatch");
    }

    memcpy(value, str.data(), size);

    return true;
}

bool KVBinaryInputStreamSerializer::binary(std::string &value, Common::QStringView name)
{
    return (*this)(value, name); // load as string
}
