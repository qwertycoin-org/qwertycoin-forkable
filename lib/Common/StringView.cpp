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

#include <limits>

#include <Common/StringView.h>

namespace Common {

const QStringView::Size QStringView::INVALID = std::numeric_limits<QStringView::Size>::max();
const QStringView QStringView::EMPTY(reinterpret_cast<Object *>(1), 0);
const QStringView QStringView::NIL(nullptr, 0);

QStringView::QStringView()
#ifndef NDEBUG // In debug mode, fill in object with invalid state (undefined).
    : gData(nullptr),
      gSize(INVALID)
#endif
{
}

QStringView::QStringView(const Object *stringData, Size stringSize)
    : gData(stringData),
      gSize(stringSize)
{
    assert(gData != nullptr || gSize == 0);
}

QStringView::QStringView(const std::string &string)
    : gData(string.data()),
      gSize(string.size())
{
}

QStringView::QStringView(const QStringView &other)
    : gData(other.gData),
      gSize(other.gSize)
{
    assert(gData != nullptr || gSize == 0);
}

QStringView &QStringView::operator=(const QStringView &other)
{
    assert(other.gData != nullptr || other.gSize == 0);

    gData = other.gData;
    gSize = other.gSize;

    return *this;
}

QStringView::operator std::string() const
{
    return std::string(gData, gSize);
}

const QStringView::Object *QStringView::getData() const
{
    assert(gData != nullptr || gSize == 0);

    return gData;
}

QStringView::Size QStringView::getSize() const
{
    assert(gData != nullptr || gSize == 0);

    return gSize;
}

bool QStringView::isEmpty() const
{
    assert(gData != nullptr || gSize == 0);

    return gSize == 0;
}

bool QStringView::isNil() const
{
    assert(gData != nullptr || gSize == 0);

    return gData == nullptr;
}

const QStringView::Object &QStringView::operator[](Size index) const
{
    assert(gData != nullptr || gSize == 0);

    assert(index < gSize);

    return *(gData + index);
}

const QStringView::Object &QStringView::first() const
{
    assert(gData != nullptr || gSize == 0);

    assert(gSize > 0);

    return *gData;
}

const QStringView::Object &QStringView::last() const
{
    assert(gData != nullptr || gSize == 0);

    assert(gSize > 0);

    return *(gData + (gSize - 1));
}

const QStringView::Object *QStringView::begin() const
{
    assert(gData != nullptr || gSize == 0);

    return gData;
}

const QStringView::Object *QStringView::end() const
{
    assert(gData != nullptr || gSize == 0);

    return gData + gSize;
}

bool QStringView::operator==(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize == other.gSize) {
        for (Size i = 0;; ++i) {
            if (i == gSize) {
                return true;
            }

            if (*(gData + i) != *(other.gData + i)) {
                break;
            }
        }
    }

    return false;
}

bool QStringView::operator!=(QStringView other) const
{
    return !(*this == other);
}

bool QStringView::operator<(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    Size count = other.gSize < gSize ? other.gSize : gSize;
    for (Size i = 0; i < count; ++i) {
        Object char1 = *(gData + i);
        Object char2 = *(other.gData + i);
        if (char1 < char2) {
            return true;
        }

        if (char2 < char1) {
            return false;
        }
    }

    return gSize < other.gSize;
}

bool QStringView::operator<=(QStringView other) const
{
    return !(other < *this);
}

bool QStringView::operator>(QStringView other) const
{
    return other < *this;
}

bool QStringView::operator>=(QStringView other) const
{
    return !(*this < other);
}

bool QStringView::beginsWith(const Object &object) const
{
    assert(gData != nullptr || gSize == 0);

    if (gSize == 0) {
        return false;
    }

    return *gData == object;
}

bool QStringView::beginsWith(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize >= other.gSize) {
        for (Size i = 0;; ++i) {
            if (i == other.gSize) {
                return true;
            }

            if (*(gData + i) != *(other.gData + i)) {
                break;
            }
        }
    }

    return false;
}

bool QStringView::contains(const Object &object) const
{
    assert(gData != nullptr || gSize == 0);

    for (Size i = 0; i < gSize; ++i) {
        if (*(gData + i) == object) {
            return true;
        }
    }

    return false;
}

bool QStringView::contains(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize >= other.gSize) {
    Size i = gSize - other.gSize;
        for (Size j = 0; i >= j; ++j) {
            for (Size k = 0;; ++k) {
                if (k == other.gSize) {
                    return true;
                }

                if (*(gData + j + k) != *(other.gData + k)) {
                    break;
                }
            }
        }
    }

    return false;
}

bool QStringView::endsWith(const Object &object) const
{
    assert(gData != nullptr || gSize == 0);

    if (gSize == 0) {
        return false;
    }

    return *(gData + (gSize - 1)) == object;
}

bool QStringView::endsWith(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize >= other.gSize) {
        Size i = gSize - other.gSize;
        for (Size j = 0;; ++j) {
            if (j == other.gSize) {
                return true;
            }

            if (*(gData + i + j) != *(other.gData + j)) {
                break;
            }
        }
    }

    return false;
}

QStringView::Size QStringView::find(const Object &object) const
{
    assert(gData != nullptr || gSize == 0);

    for (Size i = 0; i < gSize; ++i) {
        if (*(gData + i) == object) {
            return i;
        }
    }

    return INVALID;
}

QStringView::Size QStringView::find(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize >= other.gSize) {
        Size i = gSize - other.gSize;
        for (Size j = 0; i >= j; ++j) {
            for (Size k = 0;; ++k) {
                if (k == other.gSize) {
                    return j;
                }

                if (*(gData + j + k) != *(other.gData + k)) {
                    break;
                }
            }
        }
    }

    return INVALID;
}

QStringView::Size QStringView::findLast(const Object &object) const
{
    assert(gData != nullptr || gSize == 0);

    for (Size i = 0; i < gSize; ++i) {
        if (*(gData + (gSize - 1 - i)) == object) {
            return gSize - 1 - i;
        }
    }

    return INVALID;
}

QStringView::Size QStringView::findLast(QStringView other) const
{
    assert(gData != nullptr || gSize == 0);

    assert(other.gData != nullptr || other.gSize == 0);

    if (gSize >= other.gSize) {
        Size i = gSize - other.gSize;
        for (Size j = 0; i >= j; ++j) {
            for (Size k = 0;; ++k) {
                if (k == other.gSize) {
                    return i - j;
                }

                if (*(gData + (i - j + k)) != *(other.gData + k)) {
                    break;
                }
            }
        }
    }

    return INVALID;
}

QStringView QStringView::head(Size headSize) const
{
    assert(gData != nullptr || gSize == 0);

    assert(headSize <= gSize);

    return QStringView(gData, headSize);
}

QStringView QStringView::tail(Size tailSize) const
{
    assert(gData != nullptr || gSize == 0);

    assert(tailSize <= gSize);

    return QStringView(gData + (gSize - tailSize), tailSize);
}

QStringView QStringView::unhead(Size headSize) const
{
    assert(gData != nullptr || gSize == 0);

    assert(headSize <= gSize);

    return QStringView(gData + headSize, gSize - headSize);
}

QStringView QStringView::untail(Size tailSize) const
{
    assert(gData != nullptr || gSize == 0);

    assert(tailSize <= gSize);

    return QStringView(gData, gSize - tailSize);
}

QStringView QStringView::range(Size startIndex, Size endIndex) const
{
    assert(gData != nullptr || gSize == 0);

    assert(startIndex <= endIndex && endIndex <= gSize);

    return QStringView(gData + startIndex, endIndex - startIndex);
}

QStringView QStringView::slice(Size startIndex, Size sliceSize) const
{
    assert(gData != nullptr || gSize == 0);

    assert(startIndex <= gSize && startIndex + sliceSize <= gSize);

    return QStringView(gData + startIndex, sliceSize);
}

} // namespace Common
