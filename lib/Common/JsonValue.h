// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2016-2017 BXC developers
// Copyright (c) 2017 UltraNote developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Common {

    class QJsonValue
    {
    public:
        // Standard JS/JSON DataTypes
        typedef bool Bool;
        typedef int64_t Integer;
        typedef std::string Key;
        typedef std::vector <QJsonValue> Array;
        typedef std::nullptr_t Nil;
        typedef std::map <Key, QJsonValue> Object;
        typedef double Real;
        typedef std::string String;

        enum EType
        {
            ARRAY,
            BOOL,
            INTEGER,
            NIL,
            OBJECT,
            REAL,
            STRING
        };

        QJsonValue ();
        QJsonValue (Bool bValue);
        QJsonValue (Integer iValue);
        QJsonValue (const QJsonValue &sOther);
        QJsonValue (QJsonValue &&sOther) noexcept;
        QJsonValue (EType sValueType);
        QJsonValue (const Array &vValue);
        QJsonValue (Array &&vValue);
        QJsonValue (Nil sValue);
        QJsonValue (const Object &sValue);
        QJsonValue (Object &&sValue);
        QJsonValue (Real sValue);
        QJsonValue (const String &cValue);
        QJsonValue (String &&cValue);

        template <size_t uSize>
        QJsonValue (const char(&cValue)[uSize])
        {
            new(uValueString)String(cValue, uSize - 1);
            sType = STRING;
        }

        ~QJsonValue ();

        QJsonValue &operator= (Bool bValue);
        QJsonValue &operator= (Integer iValue);
        QJsonValue &operator= (const QJsonValue &sOther);
        QJsonValue &operator= (QJsonValue &&sOther) noexcept;
        QJsonValue &operator= (const Array &vValue);
        QJsonValue &operator= (Array &&vValue);
        QJsonValue &operator= (Nil sValue);
        QJsonValue &operator= (const Object &sValue);
        QJsonValue &operator= (Object &&sValue);
        QJsonValue &operator= (Real sValue);
        QJsonValue &operator= (const String &cValue);
        QJsonValue &operator= (String &&cValue);
        QJsonValue &operator[] (size_t uIndex);
        const QJsonValue &operator[] (size_t uIndex) const;
        QJsonValue &operator() (const Key &sKey);
        const QJsonValue &operator() (const Key &sKey) const;

        template <size_t uSize>
        QJsonValue &operator= (const char(&cValue)[uSize])
        {
            if (sType != STRING) {
                destructValue();
                sType = NIL;
                new(uValueString)String(cValue, uSize - 1);
                sType = STRING;
            } else {
                reinterpret_cast<String *>(uValueString)->assign(cValue, uSize - 1);
            }

            return *this;
        }

        friend std::ostream &operator<< (std::ostream &out, const QJsonValue &jsonValue);
        friend std::istream &operator>> (std::istream &in, QJsonValue &jsonValue);

        bool contains (const Key &key) const;
        bool isArray () const;
        bool isBool () const;
        bool isInteger () const;
        bool isNil () const;
        bool isObject () const;
        bool isReal () const;
        bool isString () const;
        Bool getBool () const;

        size_t erase (const Key &key);
        size_t size () const;
        Integer getInteger () const;

        std::string toString () const;
        String &getString ();
        const String &getString () const;

        EType getType () const;

        Array &getArray ();
        const Array &getArray () const;

        Object &getObject ();
        const Object &getObject () const;

        Real getReal () const;

        static QJsonValue fromString (const std::string &source);
        static QJsonValue fromStringWithWhiteSpaces (const std::string &source);
        QJsonValue &insert (const Key &key, const QJsonValue &value);
        QJsonValue &insert (const Key &key, QJsonValue &&value);
        QJsonValue &pushBack (const QJsonValue &value);
        QJsonValue &pushBack (QJsonValue &&value);
        // sets or creates value, returns reference to self
        QJsonValue &set (const Key &key, const QJsonValue &value);
        QJsonValue &set (const Key &key, QJsonValue &&value);

    private:
        void destructValue ();
        void readArray (std::istream &in);
        void readTrue (std::istream &in);
        void readFalse (std::istream &in);
        void readNull (std::istream &in);
        void readNumber (std::istream &in, char c);
        void readObject (std::istream &in);
        void readString (std::istream &in);

        EType sType;
        union
        {
            uint8_t uValueArray[sizeof(Array)];
            Bool bValueBool;
            Integer iValueInteger;
            uint8_t uValueObject[sizeof(Object)];
            Real sValueReal;
            uint8_t uValueString[sizeof(std::string)];
        };
    };
} // namespace Common
