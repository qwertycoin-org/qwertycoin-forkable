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

#include <list>
#include <vector>
#include <Common/MemoryInputStream.h>
#include <Common/StringOutputStream.h>
#include <Serialization/JsonInputStreamSerializer.h>
#include <Serialization/JsonOutputStreamSerializer.h>
#include <Serialization/KVBinaryInputStreamSerializer.h>
#include <Serialization/KVBinaryOutputStreamSerializer.h>

namespace Common {

template <typename T>
T getValueAs(const QJsonValue &js)
{
    return js;
}

template <>
inline std::string getValueAs<std::string>(const QJsonValue &js)
{
    return js.getString();
}

template <>
inline uint64_t getValueAs<uint64_t>(const QJsonValue &js)
{
    return static_cast<uint64_t>(js.getInteger());
}

} // namespace Common

namespace QwertyNote {

template <typename T>
Common::QJsonValue storeToJsonValue(const T &v)
{
    JsonOutputStreamSerializer s;
    serialize(const_cast<T &>(v), s);

    return s.getValue();
}

template <typename T>
Common::QJsonValue storeContainerToJsonValue(const T &cont)
{
    Common::QJsonValue js(Common::QJsonValue::ARRAY);
    for (const auto &item : cont) {
        js.pushBack(item);
    }

    return js;
}

template <typename T>
Common::QJsonValue storeToJsonValue(const std::vector<T> &v)
{
    return storeContainerToJsonValue(v);
}

template <typename T>
Common::QJsonValue storeToJsonValue(const std::list<T> &v)
{
    return storeContainerToJsonValue(v);
}

template <>
inline Common::QJsonValue storeToJsonValue(const std::string &v)
{
    return Common::QJsonValue(v);
}

template <typename T>
void loadFromJsonValue(T& v, const Common::QJsonValue &js)
{
    JsonInputValueSerializer s(js);
    serialize(v, s);
}

template <typename T>
void loadFromJsonValue(std::vector<T> &v, const Common::QJsonValue &js)
{
    for (size_t i = 0; i < js.size(); ++i) {
        v.push_back(Common::getValueAs<T>(js[i]));
    }
}

template <typename T>
void loadFromJsonValue(std::list<T> &v, const Common::QJsonValue &js)
{
    for (size_t i = 0; i < js.size(); ++i) {
        v.push_back(Common::getValueAs<T>(js[i]));
    }
}

template <typename T>
std::string storeToJson(const T &v)
{
    return storeToJsonValue(v).toString();
}

template <typename T>
bool loadFromJson(T &v, const std::string &buf)
{
    try {
        if (buf.empty()) {
            return true;
        }
        auto js = Common::QJsonValue(buf);
        loadFromJsonValue(v, js);
    } catch (std::exception &) {
        return false;
    }

    return true;
}

template <typename T>
std::string storeToBinaryKeyValue(const T &v)
{
    KVBinaryOutputStreamSerializer s;
    serialize(const_cast<T &>(v), s);

    std::string result;
    Common::QStringOutputStream stream(result);
    s.dump(stream);

    return result;
}

template <typename T>
bool loadFromBinaryKeyValue(T &v, const std::string &buf)
{
    try {
        Common::QMemoryInputStream stream(buf.data(), buf.size());
        KVBinaryInputStreamSerializer s(stream);
        serialize(v, s);

        return true;
    } catch (std::exception &) {
        return false;
    }
}

} // namespace QwertyNote
