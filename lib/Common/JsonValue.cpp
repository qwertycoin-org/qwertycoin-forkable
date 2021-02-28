// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2016-2017 BXC developers
// Copyright (c) 2017 UltraNote developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iomanip>
#include <iostream>
#include <sstream>

#include <Common/JsonValue.h>

namespace Common {

    QJsonValue::QJsonValue ()
        : sType(NIL)
    {
    }

    QJsonValue::QJsonValue (Bool bValue)
        : sType(BOOL),
          bValueBool(bValue)
    {
    }

    QJsonValue::QJsonValue (Integer iValue)
        : sType(INTEGER),
          iValueInteger(iValue)
    {
    }

    QJsonValue::QJsonValue (const QJsonValue &sOther)
    {
        switch (sOther.sType) {
            case ARRAY:
                new(uValueArray)Array(*reinterpret_cast<const Array *>(sOther.uValueArray));
                break;
            case BOOL:
                bValueBool = sOther.bValueBool;
                break;
            case INTEGER:
                iValueInteger = sOther.iValueInteger;
                break;
            case NIL:
                break;
            case OBJECT:
                new(uValueObject)Object(*reinterpret_cast<const Object *>(sOther.uValueObject));
                break;
            case REAL:
                sValueReal = sOther.sValueReal;
                break;
            case STRING:
                new(uValueString)String(*reinterpret_cast<const String *>(sOther.uValueString));
                break;
        }

        sType = sOther.sType;
    }

    QJsonValue::QJsonValue (QJsonValue &&sOther) noexcept
    {
        switch (sOther.sType) {
            case ARRAY:
                new(uValueArray)Array(std::move(*reinterpret_cast<Array *>(sOther.uValueArray)));
                reinterpret_cast<Array *>(sOther.uValueArray)->~Array();
                break;
            case BOOL:
                bValueBool = sOther.bValueBool;
                break;
            case INTEGER:
                iValueInteger = sOther.iValueInteger;
                break;
            case NIL:
                break;
            case OBJECT:
                new(uValueObject)Object(
                    std::move(*reinterpret_cast<Object *>(sOther.uValueObject)));
                reinterpret_cast<Object *>(sOther.uValueObject)->~Object();
                break;
            case REAL:
                sValueReal = sOther.sValueReal;
                break;
            case STRING:
                new(uValueString)String(
                    std::move(*reinterpret_cast<String *>(sOther.uValueString)));
                reinterpret_cast<String *>(sOther.uValueString)->~String();
                break;
        }

        sType = sOther.sType;
        sOther.sType = NIL;
    }

    QJsonValue::QJsonValue (EType sValueType)
    {
        switch (sValueType) {
            case ARRAY:
                new(uValueArray)Array;
                break;
            case NIL:
                break;
            case OBJECT:
                new(uValueObject)Object;
                break;
            case STRING:
                new(uValueString)String;
                break;
            default:
                throw std::runtime_error("Invalid QJsonValue sType for constructor!");
        }

        sType = sValueType;
    }

    QJsonValue::QJsonValue (const Array &vValue)
    {
        new(uValueArray)Array(vValue);
        sType = ARRAY;
    }

    QJsonValue::QJsonValue (Array &&vValue)
    {
        new(uValueArray)Array(std::move(vValue));
        sType = ARRAY;
    }

    QJsonValue::QJsonValue (Nil)
        : sType(NIL)
    {
    }

    QJsonValue::QJsonValue (const Object &sValue)
    {
        new(uValueObject)Object(sValue);
        sType = OBJECT;
    }

    QJsonValue::QJsonValue (Object &&sValue)
    {
        new(uValueObject)Object(std::move(sValue));
        sType = OBJECT;
    }

    QJsonValue::QJsonValue (Real sValue)
        : sType(REAL),
          sValueReal(sValue)
    {
    }

    QJsonValue::QJsonValue (const String &cValue)
    {
        new(uValueString)String(cValue);
        sType = STRING;
    }

    QJsonValue::QJsonValue (String &&cValue)
    {
        new(uValueString)String(std::move(cValue));
        sType = STRING;
    }

    QJsonValue::~QJsonValue ()
    {
        destructValue();
    }


    namespace {

        char readChar (std::istream &in)
        {
            char c = static_cast<char>(in.get());
            if (!in) {
                throw std::runtime_error("Unable to parse: unexpected end of stream");
            }

            return c;
        }

        char readNonWsChar (std::istream &in)
        {
            char c;

            do {
                c = readChar(in);
            } while (isspace(c));

            return c;
        }

        std::string readStringToken (std::istream &in)
        {
            char c;
            std::string value;

            while (in) {
                c = readChar(in);

                if (c == '"') {
                    break;
                }

                if (c == '\\') {
                    value += c;
                    c = readChar(in);
                }

                value += c;
            }

            return value;
        }

    } // namespace


    QJsonValue &QJsonValue::operator= (Bool bValue)
    {
        if (sType != BOOL) {
            destructValue();
            sType = BOOL;
        }

        bValueBool = bValue;

        return *this;
    }

    QJsonValue &QJsonValue::operator= (Integer iValue)
    {
        if (sType != INTEGER) {
            destructValue();
            sType = INTEGER;
        }

        iValueInteger = iValue;

        return *this;
    }

    QJsonValue &QJsonValue::operator= (const QJsonValue &sOther)
    {
        if (sType != sOther.sType) {
            destructValue();
            switch (sOther.sType) {
                case ARRAY:
                    sType = NIL;
                    new(uValueArray)Array(*reinterpret_cast<const Array *>(sOther.uValueArray));
                    break;
                case BOOL:
                    bValueBool = sOther.bValueBool;
                    break;
                case INTEGER:
                    iValueInteger = sOther.iValueInteger;
                    break;
                case NIL:
                    break;
                case OBJECT:
                    sType = NIL;
                    new(uValueObject)Object(*reinterpret_cast<const Object *>(sOther.uValueObject));
                    break;
                case REAL:
                    sValueReal = sOther.sValueReal;
                    break;
                case STRING:
                    sType = NIL;
                    new(uValueString)String(*reinterpret_cast<const String *>(sOther.uValueString));
                    break;
            }

            sType = sOther.sType;
        } else {
            switch (sType) {
                case ARRAY:
                    *reinterpret_cast<Array *>(uValueArray)
                        = *reinterpret_cast<const Array *>(sOther.uValueArray);
                    break;
                case BOOL:
                    bValueBool = sOther.bValueBool;
                    break;
                case INTEGER:
                    iValueInteger = sOther.iValueInteger;
                    break;
                case NIL:
                    break;
                case OBJECT:
                    *reinterpret_cast<Object *>(uValueObject)
                        = *reinterpret_cast<const Object *>(sOther.uValueObject);
                    break;
                case REAL:
                    sValueReal = sOther.sValueReal;
                    break;
                case STRING:
                    *reinterpret_cast<String *>(uValueString)
                        = *reinterpret_cast<const String *>(sOther.uValueString);
                    break;
            }
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (QJsonValue &&sOther) noexcept
    {
        if (sType != sOther.sType) {
            destructValue();
            switch (sOther.sType) {
                case ARRAY:
                    sType = NIL;
                    new(uValueArray)Array(*reinterpret_cast<const Array *>(sOther.uValueArray));
                    reinterpret_cast<Array *>(sOther.uValueArray)->~Array();
                    break;
                case BOOL:
                    bValueBool = sOther.bValueBool;
                    break;
                case INTEGER:
                    iValueInteger = sOther.iValueInteger;
                    break;
                case NIL:
                    break;
                case OBJECT:
                    sType = NIL;
                    new(uValueObject)Object(*reinterpret_cast<const Object *>(sOther.uValueObject));
                    reinterpret_cast<Object *>(sOther.uValueObject)->~Object();
                    break;
                case REAL:
                    sValueReal = sOther.sValueReal;
                    break;
                case STRING:
                    sType = NIL;
                    new(uValueString)String(*reinterpret_cast<const String *>(sOther.uValueString));
                    reinterpret_cast<String *>(sOther.uValueString)->~String();
                    break;
            }
            sType = sOther.sType;
        } else {
            switch (sType) {
                case ARRAY:
                    *reinterpret_cast<Array *>(uValueArray)
                        = *reinterpret_cast<const Array *>(sOther.uValueArray);
                    reinterpret_cast<Array *>(sOther.uValueArray)->~Array();
                    break;
                case BOOL:
                    bValueBool = sOther.bValueBool;
                    break;
                case INTEGER:
                    iValueInteger = sOther.iValueInteger;
                    break;
                case NIL:
                    break;
                case OBJECT:
                    *reinterpret_cast<Object *>(uValueObject)
                        = *reinterpret_cast<const Object *>(sOther.uValueObject);
                    reinterpret_cast<Object *>(sOther.uValueObject)->~Object();
                    break;
                case REAL:
                    sValueReal = sOther.sValueReal;
                    break;
                case STRING:
                    *reinterpret_cast<String *>(uValueString)
                        = *reinterpret_cast<const String *>(sOther.uValueString);
                    reinterpret_cast<String *>(sOther.uValueString)->~String();
                    break;
            }
        }

        sOther.sType = NIL;

        return *this;
    }

    QJsonValue &QJsonValue::operator= (const Array &vValue)
    {
        if (sType != ARRAY) {
            destructValue();
            sType = NIL;
            new(uValueArray)Array(vValue);
            sType = ARRAY;
        } else {
            *reinterpret_cast<Array *>(uValueArray) = vValue;
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (Array &&vValue)
    {
        if (sType != ARRAY) {
            destructValue();
            sType = NIL;
            new(uValueArray)Array(std::move(vValue));
            sType = ARRAY;
        } else {
            *reinterpret_cast<Array *>(uValueArray) = std::move(vValue);
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (Nil)
    {
        if (sType != NIL) {
            destructValue();
            sType = NIL;
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (const Object &sValue)
    {
        if (sType != OBJECT) {
            destructValue();
            sType = NIL;
            new(uValueObject)Object(sValue);
            sType = OBJECT;
        } else {
            *reinterpret_cast<Object *>(uValueObject) = sValue;
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (Object &&sValue)
    {
        if (sType != OBJECT) {
            destructValue();
            sType = NIL;
            new(uValueObject)Object(std::move(sValue));
            sType = OBJECT;
        } else {
            *reinterpret_cast<Object *>(uValueObject) = std::move(sValue);
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (Real sValue)
    {
        if (sType != REAL) {
            destructValue();
            sType = REAL;
        }

        sValueReal = sValue;

        return *this;
    }

    QJsonValue &QJsonValue::operator= (const String &cValue)
    {
        if (sType != STRING) {
            destructValue();
            sType = NIL;
            new(uValueString)String(cValue);
            sType = STRING;
        } else {
            *reinterpret_cast<String *>(uValueString) = cValue;
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator= (String &&cValue)
    {
        if (sType != STRING) {
            destructValue();
            sType = NIL;
            new(uValueString)String(std::move(cValue));
            sType = STRING;
        } else {
            *reinterpret_cast<String *>(uValueString) = std::move(cValue);
        }

        return *this;
    }

    QJsonValue &QJsonValue::operator[] (size_t uIndex)
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        return reinterpret_cast<Array *>(uValueArray)->at(uIndex);
    }

    const QJsonValue &QJsonValue::operator[] (size_t uIndex) const
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        return reinterpret_cast<const Array *>(uValueArray)->at(uIndex);
    }

    QJsonValue &QJsonValue::operator() (const Key &sKey)
    {
        return getObject().at(sKey);
    }

    const QJsonValue &QJsonValue::operator() (const Key &sKey) const
    {
        return getObject().at(sKey);
    }

    std::ostream &operator<< (std::ostream &out, const QJsonValue &jsonValue)
    {
        switch (jsonValue.sType) {
            case QJsonValue::ARRAY: {
                const QJsonValue::Array &array
                    = *reinterpret_cast<const QJsonValue::Array *>(jsonValue.uValueArray);
                out << '[';
                if (!array.empty()) {
                    out << array[0];
                    for (size_t i = 1; i < array.size(); ++i) {
                        out << ',' << array[i];
                    }
                }
                out << ']';
                break;
            }
            case QJsonValue::BOOL:
                out << (jsonValue.bValueBool ? "true" : "false");
                break;
            case QJsonValue::INTEGER:
                out << jsonValue.iValueInteger;
                break;
            case QJsonValue::NIL:
                out << "null";
                break;
            case QJsonValue::OBJECT: {
                const QJsonValue::Object &object
                    = *reinterpret_cast<const QJsonValue::Object *>(jsonValue.uValueObject);
                out << '{';
                auto iter = object.begin();
                if (iter != object.end()) {
                    out << '"' << iter->first << "\":" << iter->second;
                    ++iter;
                    for (; iter != object.end(); ++iter) {
                        out << ",\"" << iter->first << "\":" << iter->second;
                    }
                }
                out << '}';
                break;
            }
            case QJsonValue::REAL: {
                std::ostringstream stream;
                stream << std::fixed << std::setprecision(11) << jsonValue.sValueReal;
                std::string value = stream.str();
                while (value.size() > 1
                       && value[value.size() - 2] != '.'
                       && value[value.size() - 1] == '0') {
                    value.resize(value.size() - 1);
                }
                out << value;
                break;
            }
            case QJsonValue::STRING:
                out << '"' << *reinterpret_cast<const QJsonValue::String *>(jsonValue.uValueString)
                    << '"';
                break;
        }

        return out;
    }

    std::istream &operator>> (std::istream &in, QJsonValue &jsonValue)
    {
        char c = readNonWsChar(in);

        if (c == '[') {
            jsonValue.readArray(in);
        } else if (c == 't') {
            jsonValue.readTrue(in);
        } else if (c == 'f') {
            jsonValue.readFalse(in);
        } else if ((c == '-') || (c >= '0' && c <= '9')) {
            jsonValue.readNumber(in, c);
        } else if (c == 'n') {
            jsonValue.readNull(in);
        } else if (c == '{') {
            jsonValue.readObject(in);
        } else if (c == '"') {
            jsonValue.readString(in);
        } else {
            throw std::runtime_error("Unable to parse");
        }

        return in;
    }


    bool QJsonValue::contains (const Key &key) const
    {
        return getObject().count(key) > 0;
    }

    bool QJsonValue::isArray () const
    {
        return sType == ARRAY;
    }

    bool QJsonValue::isBool () const
    {
        return sType == BOOL;
    }

    bool QJsonValue::isInteger () const
    {
        return sType == INTEGER;
    }

    bool QJsonValue::isNil () const
    {
        return sType == NIL;
    }

    bool QJsonValue::isObject () const
    {
        return sType == OBJECT;
    }

    bool QJsonValue::isReal () const
    {
        return sType == REAL;
    }

    bool QJsonValue::isString () const
    {
        return sType == STRING;
    }

    QJsonValue::Bool QJsonValue::getBool () const
    {
        if (sType != BOOL) {
            throw std::runtime_error("QJsonValue sType is not BOOL");
        }

        return bValueBool;
    }


    size_t QJsonValue::erase (const Key &key)
    {
        return getObject().erase(key);
    }

    size_t QJsonValue::size () const
    {
        switch (sType) {
            case ARRAY:
                return reinterpret_cast<const Array *>(uValueArray)->size();
            case OBJECT:
                return reinterpret_cast<const Object *>(uValueObject)->size();
            default:
                throw std::runtime_error("QJsonValue sType is not ARRAY or OBJECT");
        }
    }

    QJsonValue::Integer QJsonValue::getInteger () const
    {
        if (sType != INTEGER) {
            throw std::runtime_error("QJsonValue sType is not INTEGER");
        }

        return iValueInteger;
    }


    std::string QJsonValue::toString () const
    {
        std::ostringstream stream;
        stream << *this;

        return stream.str();
    }

    QJsonValue::String &QJsonValue::getString ()
    {
        if (sType != STRING) {
            throw std::runtime_error("QJsonValue sType is not STRING");
        }

        return *reinterpret_cast<String *>(uValueString);
    }

    const QJsonValue::String &QJsonValue::getString () const
    {
        if (sType != STRING) {
            throw std::runtime_error("QJsonValue sType is not STRING");
        }

        return *reinterpret_cast<const String *>(uValueString);
    }


    QJsonValue::EType QJsonValue::getType () const
    {
        return sType;
    }


    QJsonValue::Array &QJsonValue::getArray ()
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        return *reinterpret_cast<Array *>(uValueArray);
    }

    const QJsonValue::Array &QJsonValue::getArray () const
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        return *reinterpret_cast<const Array *>(uValueArray);
    }


    QJsonValue::Object &QJsonValue::getObject ()
    {
        if (sType != OBJECT) {
            throw std::runtime_error("QJsonValue sType is not OBJECT");
        }

        return *reinterpret_cast<Object *>(uValueObject);
    }

    const QJsonValue::Object &QJsonValue::getObject () const
    {
        if (sType != OBJECT) {
            throw std::runtime_error("QJsonValue sType is not OBJECT");
        }

        return *reinterpret_cast<const Object *>(uValueObject);
    }


    QJsonValue::Real QJsonValue::getReal () const
    {
        if (sType != REAL) {
            throw std::runtime_error("QJsonValue sType is not REAL");
        }

        return sValueReal;
    }


    QJsonValue QJsonValue::fromString (const std::string &source)
    {
        QJsonValue jsonValue;
        std::istringstream stream(source);
        stream >> jsonValue;
        if (stream.fail()) {
            throw std::runtime_error("Unable to parse QJsonValue");
        }

        return jsonValue;
    }

    QJsonValue QJsonValue::fromStringWithWhiteSpaces (const std::string &source)
    {
        QJsonValue jsonValue;
        std::istringstream stream(source);
        stream >> std::noskipws >> jsonValue;
        if (stream.fail()) {
            throw std::runtime_error("Unable to parse QJsonValue");
        }

        return jsonValue;
    }

    QJsonValue &QJsonValue::insert (const Key &key, const QJsonValue &value)
    {
        return getObject().emplace(key, value).first->second;
    }

    QJsonValue &QJsonValue::insert (const Key &key, QJsonValue &&value)
    {
        return getObject().emplace(key, std::move(value)).first->second;
    }

    QJsonValue &QJsonValue::pushBack (const QJsonValue &value)
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        reinterpret_cast<Array *>(uValueArray)->emplace_back(value);

        return reinterpret_cast<Array *>(uValueArray)->back();
    }

    QJsonValue &QJsonValue::pushBack (QJsonValue &&value)
    {
        if (sType != ARRAY) {
            throw std::runtime_error("QJsonValue sType is not ARRAY");
        }

        reinterpret_cast<Array *>(uValueArray)->emplace_back(std::move(value));

        return reinterpret_cast<Array *>(uValueArray)->back();
    }

    QJsonValue &QJsonValue::set (const Key &key, const QJsonValue &value)
    {
        getObject()[key] = value;

        return *this;
    }

    QJsonValue &QJsonValue::set (const Key &key, QJsonValue &&value)
    {
        getObject()[key] = std::move(value);

        return *this;
    }


    void QJsonValue::destructValue ()
    {
        switch (sType) {
            case ARRAY:
                reinterpret_cast<Array *>(uValueArray)->~Array();
                break;
            case OBJECT:
                reinterpret_cast<Object *>(uValueObject)->~Object();
                break;
            case STRING:
                reinterpret_cast<String *>(uValueString)->~String();
                break;
            default:
                break;
        }
    }

    void QJsonValue::readArray (std::istream &in)
    {
        QJsonValue::Array value;
        char c = readNonWsChar(in);

        if (c != ']') {
            in.putback(c);
            for (;;) {
                value.resize(value.size() + 1);
                in >> value.back();
                c = readNonWsChar(in);

                if (c == ']') {
                    break;
                }

                if (c != ',') {
                    throw std::runtime_error("Unable to parse");
                }
            }
        }

        if (sType != QJsonValue::ARRAY) {
            destructValue();
            sType = QJsonValue::NIL;
            new(uValueArray)QJsonValue::Array;
            sType = QJsonValue::ARRAY;
        }

        reinterpret_cast<QJsonValue::Array *>(uValueArray)->swap(value);
    }

    void QJsonValue::readTrue (std::istream &in)
    {
        char data[3];
        in.read(data, 3);
        if (data[0] != 'r' || data[1] != 'u' || data[2] != 'e') {
            throw std::runtime_error("Unable to parse");
        }

        if (sType != QJsonValue::BOOL) {
            destructValue();
            sType = QJsonValue::BOOL;
        }

        bValueBool = true;
    }

    void QJsonValue::readFalse (std::istream &in)
    {
        char data[4];
        in.read(data, 4);
        if (data[0] != 'a' || data[1] != 'l' || data[2] != 's' || data[3] != 'e') {
            throw std::runtime_error("Unable to parse");
        }

        if (sType != QJsonValue::BOOL) {
            destructValue();
            sType = QJsonValue::BOOL;
        }

        bValueBool = false;
    }

    void QJsonValue::readNull (std::istream &in)
    {
        char data[3];
        in.read(data, 3);
        if (data[0] != 'u' || data[1] != 'l' || data[2] != 'l') {
            throw std::runtime_error("Unable to parse");
        }

        if (sType != QJsonValue::NIL) {
            destructValue();
            sType = QJsonValue::NIL;
        }
    }

    void QJsonValue::readNumber (std::istream &in, char c)
    {
        std::string text;
        text += c;
        size_t dots = 0;
        for (;;) {
            int i = in.peek();
            if (i >= '0' && i <= '9') {
                in.read(&c, 1);
                text += c;
            } else if (i == '.') {
                in.read(&c, 1);
                text += '.';
                ++dots;
            } else {
                break;
            }
        }

        if (dots > 0) {
            if (dots > 1) {
                throw std::runtime_error("Unable to parse");
            }

            if (in.peek() == 'e') {
                int i = in.peek();
                in.read(&c, 1);
                text += c;
                if (i == '+') {
                    in.read(&c, 1);
                    text += c;
                    i = in.peek();
                } else if (i == '-') {
                    in.read(&c, 1);
                    text += c;
                    i = in.peek();
                }

                if (i < '0' || i > '9') {
                    throw std::runtime_error("Unable to parse");
                }

                do {
                    in.read(&c, 1);
                    text += c;
                    i = in.peek();
                } while (i >= '0' && i <= '9');
            }

            Real value;
            std::istringstream(text) >> value;
            if (sType != REAL) {
                destructValue();
                sType = REAL;
            }
            sValueReal = value;
        } else {
            if (text.size() > 1 && ((text[0] == '0') || (text[0] == '-' && text[1] == '0'))) {
                throw std::runtime_error("Unable to parse");
            }

            Integer value;
            std::istringstream(text) >> value;
            if (sType != INTEGER) {
                destructValue();
                sType = INTEGER;
            }

            iValueInteger = value;
        }
    }

    void QJsonValue::readObject (std::istream &in)
    {
        char c = readNonWsChar(in);
        QJsonValue::Object value;

        if (c != '}') {
            std::string name;

            for (;;) {
                if (c != '"') {
                    throw std::runtime_error("Unable to parse");
                }

                name = readStringToken(in);
                c = readNonWsChar(in);

                if (c != ':') {
                    throw std::runtime_error("Unable to parse");
                }

                in >> value[name];
                c = readNonWsChar(in);

                if (c == '}') {
                    break;
                }

                if (c != ',') {
                    throw std::runtime_error("Unable to parse");
                }

                c = readNonWsChar(in);
            }
        }

        if (sType != QJsonValue::OBJECT) {
            destructValue();
            sType = QJsonValue::NIL;
            new(uValueObject)QJsonValue::Object;
            sType = QJsonValue::OBJECT;
        }

        reinterpret_cast<QJsonValue::Object *>(uValueObject)->swap(value);
    }

    void QJsonValue::readString (std::istream &in)
    {
        String value = readStringToken(in);

        if (sType != QJsonValue::STRING) {
            destructValue();
            sType = QJsonValue::NIL;
            new(uValueString)String;
            sType = QJsonValue::STRING;
        }

        reinterpret_cast<String *>(uValueString)->swap(value);
    }

} // namespace Common
