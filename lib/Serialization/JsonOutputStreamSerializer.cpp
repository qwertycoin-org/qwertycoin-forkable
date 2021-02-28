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

#include <cassert>
#include <stdexcept>
#include <Common/StringTools.h>
#include <Serialization/JsonOutputStreamSerializer.h>

using Common::QJsonValue;
using namespace QwertyNote;

namespace QwertyNote {

std::ostream &operator<<(std::ostream &out, const JsonOutputStreamSerializer &enumerator)
{
    out << enumerator.root;

    return out;
}

} // namespace QwertyNote

namespace {

template <typename T>
void insertOrPush(QJsonValue &js, Common::QStringView name, const T &value)
{
    if (js.isArray()) {
        js.pushBack(QJsonValue(value));
    } else {
        js.insert(std::string(name), QJsonValue(value));
    }
}

} // namespace

JsonOutputStreamSerializer::JsonOutputStreamSerializer()
    : root(QJsonValue::OBJECT)
{
    chain.push_back(&root);
}

ISerializer::SerializerType JsonOutputStreamSerializer::type() const
{
    return ISerializer::OUTPUT;
}

bool JsonOutputStreamSerializer::beginObject(Common::QStringView name)
{
    QJsonValue &parent = *chain.back();
    QJsonValue obj(QJsonValue::OBJECT);

    if (parent.isObject()) {
        chain.push_back(&parent.insert(std::string(name), obj));
    } else {
        chain.push_back(&parent.pushBack(obj));
    }

    return true;
}

void JsonOutputStreamSerializer::endObject()
{
    assert(!chain.empty());

    chain.pop_back();
}

bool JsonOutputStreamSerializer::beginArray(size_t &size, Common::QStringView name)
{
    QJsonValue val(QJsonValue::ARRAY);
    QJsonValue &res = chain.back()->insert(std::string(name), val);

    chain.push_back(&res);

    return true;
}

void JsonOutputStreamSerializer::endArray()
{
    assert(!chain.empty());

    chain.pop_back();
}

bool JsonOutputStreamSerializer::operator()(uint64_t &value, Common::QStringView name)
{
  auto v = static_cast<int64_t>(value);

  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(uint16_t &value, Common::QStringView name)
{
    auto v = static_cast<uint64_t>(value);

    return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int16_t &value, Common::QStringView name)
{
    auto v = static_cast<int64_t>(value);

    return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(uint32_t &value, Common::QStringView name)
{
    auto v = static_cast<uint64_t>(value);

    return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int32_t &value, Common::QStringView name)
{
    auto v = static_cast<int64_t>(value);

    return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int64_t &value, Common::QStringView name)
{
    insertOrPush(*chain.back(), name, value);

    return true;
}

bool JsonOutputStreamSerializer::operator()(double &value, Common::QStringView name)
{
    insertOrPush(*chain.back(), name, value);

    return true;
}

bool JsonOutputStreamSerializer::operator()(std::string &value, Common::QStringView name)
{
    insertOrPush(*chain.back(), name, value);

    return true;
}

bool JsonOutputStreamSerializer::operator()(uint8_t &value, Common::QStringView name)
{
    insertOrPush(*chain.back(), name, static_cast<int64_t>(value));

    return true;
}

bool JsonOutputStreamSerializer::operator()(bool &value, Common::QStringView name)
{
    insertOrPush(*chain.back(), name, value);

    return true;
}

bool JsonOutputStreamSerializer::binary(void *value, size_t size, Common::QStringView name)
{
    std::string hex = Common::toHex(value, size);

    return (*this)(hex, name);
}

bool JsonOutputStreamSerializer::binary(std::string &value, Common::QStringView name)
{
    return binary(const_cast<char *>(value.data()), value.size(), name);
}
