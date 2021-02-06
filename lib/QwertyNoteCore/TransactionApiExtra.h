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

#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/TransactionExtra.h>

namespace QwertyNote {

class TransactionExtra
{
public:
    TransactionExtra() = default;
    TransactionExtra(const std::vector<uint8_t> &extra)
    {
        parse(extra);
    }

    bool parse(const std::vector<uint8_t> &extra)
    {
        fields.clear();
        return QwertyNote::parseTransactionExtra(extra, fields);
    }

    template <typename T>
    bool get(T &value) const
    {
        auto it = find(typeid(T));
        if (it == fields.end()) {
            return false;
        }

        value = boost::get<T>(*it);

        return true;
    }

    template <typename T>
    void set(const T &value)
    {
        auto it = find(typeid(T));
        if (it != fields.end()) {
            *it = value;
        } else {
            fields.push_back(value);
        }
    }

    template <typename T>
    void append(const T &value)
    {
        fields.push_back(value);
    }

    bool getPublicKey(Crypto::FPublicKey &pk) const
    {
        QwertyNote::TransactionExtraPublicKey extraPk;
        if (!get(extraPk)) {
            return false;
        }

        pk = extraPk.publicKey;

        return true;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> extra;
        writeTransactionExtra(extra, fields);

        return extra;
    }

private:
    std::vector<QwertyNote::TransactionExtraField>::const_iterator find(const std::type_info &t) const
    {
        return std::find_if(fields.begin(), fields.end(), [&t](const TransactionExtraField& f) {
            return t == f.type();
        });
    }

    std::vector<QwertyNote::TransactionExtraField>::iterator find(const std::type_info &t)
    {
        return std::find_if(fields.begin(), fields.end(), [&t](const TransactionExtraField& f) {
            return t == f.type();
        });
    }

private:
    std::vector<QwertyNote::TransactionExtraField> fields;
};

} // namespace QwertyNote
