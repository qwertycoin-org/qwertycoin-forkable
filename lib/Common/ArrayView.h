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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Common {

/**
 * @class TArrayView
 * @namespace Common
 * @brief This is a pair of pointer to constant object of parametrized sType and uSize.
 *
 * Supports 'EMPTY' and 'NIL' representations as follows:
 * - 'uData' == 'nullptr' && 'uSize' == 0 - EMPTY NIL
 * - 'uData' != 'nullptr' && 'uSize' == 0 - EMPTY NOTNIL
 * - 'uData' == 'nullptr' && 'uSize' > 0 - Undefined
 * - 'uData' != 'nullptr' && 'uSize' > 0 - NOTEMPTY NOTNIL
 * For signed integer 'uSize', 'TArrayView' with 'uSize' < 0 is undefined.
 *
 * @note It is recommended to pass 'TArrayView' to procedures by value.
 *
 * @tparam QObjectType
 * @tparam QSizeType
 */
template<class QObjectType = uint8_t, class QSizeType = size_t>
class TArrayView
{
public:
    typedef QObjectType uObject;
    typedef QSizeType uSize;

    const static uSize INVALID;
    const static TArrayView EMPTY;
    const static TArrayView NIL;

    /**
     * Default constructor.
     * Leaves object uninitialized. Any usage before initializing it is undefined.
     */
    TArrayView()
#ifndef NDEBUG // In debug mode, fill in object with uInvalid mState (undefined).
        : gData(nullptr),
          gSize(INVALID)
#endif
    {
    }

    /**
     * Direct constructor.
     * The behavior is undefined unless 'sArrayData' != 'nullptr' || 'sArraySize' == 0
     *
     * @param sArrayData
     * @param sArraySize
     */
    TArrayView(const uObject *sArrayData, uSize sArraySize)
        : gData(sArrayData),
          gSize(sArraySize)
    {
        assert(gData != nullptr || gSize == 0);
    }

    /**
     * Constructor from C array.
     * The behavior is undefined unless 'arrayData' != 'nullptr' || 'sArraySize' == 0.
     * Input mState can be malformed using pointer conversions.
     *
     * @tparam sArraySize
     * @param arrayData
     */
    template<uSize sArraySize>
    explicit TArrayView(const uObject(&arrayData)[sArraySize])
        : gData(arrayData),
          gSize(sArraySize)
    {
        assert(gData != nullptr || gSize == 0);
    }

    /**
     * Copy constructor.
     * Performs default action - bitwise copying of source object.
     * The behavior is undefined unless 'other' 'TArrayView' is in defined mState,
     * that is 'uData' != 'nullptr' || 'uSize' == 0
     *
     * @param other
     */
    TArrayView(const TArrayView &other)
        : gData(other.gData),
          gSize(other.gSize)
    {
        assert(gData != nullptr || gSize == 0);
    }

    /**
     * Destructor.
     * Does nothing.
     */
    ~TArrayView() = default;

    const uObject *getData() const
    {
        assert(gData != nullptr || gSize == 0);
        return gData;
    }

    uSize getSize() const
    {
        assert(gData != nullptr || gSize == 0);

        return gSize;
    }

    /**
     * Return false if 'TArrayView' is not EMPTY.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @return
     */
    bool isEmpty() const
    {
        assert(gData != nullptr || gSize == 0);

        return gSize == 0;
    }

    /**
     * Return false if 'TArrayView' is not NIL.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @return
     */
    bool isNil() const
    {
        assert(gData != nullptr || gSize == 0);

        return gData == nullptr;
    }

    /**
     * Get first element.
     * The behavior is undefined unless 'TArrayView' was initialized and 'uSize' > 0
     *
     * @return
     */
    const uObject &first() const
    {
        assert(gData != nullptr || gSize == 0);

        assert(gSize > 0);

        return *gData;
    }

    /**
     * Get last element.
     * The behavior is undefined unless 'TArrayView' was initialized and 'uSize' > 0
     *
     * @return
     */
    const uObject &last() const
    {
        assert(gData != nullptr || gSize == 0);

        assert(gSize > 0);

        return *(gData + (gSize - 1));
    }

    /**
     * Return a pointer to the first element.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @return
     */
    const uObject *begin() const
    {
        assert(gData != nullptr || gSize == 0);

        return gData;
    }

    /**
     * Return a pointer after the last element.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @return
     */
    const uObject *end() const
    {
        assert(gData != nullptr || gSize == 0);

        return gData + gSize;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sObject' at the beginning.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @param sObject
     * @return
     */
    bool beginsWith(const uObject &sObject) const
    {
        assert(gData != nullptr || gSize == 0);

        if (gSize == 0) {
            return false;
        }

        return *gData == sObject;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sOther' at the beginning.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    bool beginsWith(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize >= sOther.gSize) {
            for (uSize i = 0;; ++i) {
                if (i == sOther.gSize) {
                    return true;
                }

                if (!(*(gData + i) == *(sOther.gData + i))) {
                    break;
                }
            }
        }

        return false;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sObject'.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @param sObject
     * @return
     */
    bool contains(const uObject &sObject) const
    {
        assert(gData != nullptr || gSize == 0);

        for (uSize i = 0; i < gSize; ++i) {
            if (*(gData + i) == sObject) {
                return true;
            }
        }

        return false;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sOther'.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    bool contains(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize >= sOther.gSize) {
            uSize i = gSize - sOther.gSize;
            for (uSize j = 0; !(i < j); ++j) {
                for (uSize k = 0;; ++k) {
                    if (k == sOther.gSize) {
                        return true;
                    }

                    if (!(*(gData + j + k) == *(sOther.gData + k))) {
                        break;
                    }
                }
            }
        }

        return false;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sObject' at the end.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @param sObject
     * @return
     */
    bool endsWith(const uObject &sObject) const
    {
        assert(gData != nullptr || gSize == 0);

        if (gSize == 0) {
            return false;
        }

        return *(gData + (gSize - 1)) == sObject;
    }

    /**
     * Return false if 'TArrayView' does not contain 'sOther' at the end.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    bool endsWith(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize >= sOther.gSize) {
            uSize i = gSize - sOther.gSize;
            for (uSize j = 0;; ++j) {
                if (j == sOther.gSize) {
                    return true;
                }

                if (!(*(gData + i + j) == *(sOther.gData + j))) {
                    break;
                }
            }
        }

        return false;
    }

    /**
     * Looks for the first occurrence of 'sObject' in 'TArrayView',
     * returns index or uInvalid if there are no occurrences.
     * The behavior is undefined unless 'TArrayView' was initialized.
     *
     * @param sObject
     * @return
     */
    uSize find(const uObject &sObject) const
    {
        assert(gData != nullptr || gSize == 0);

        for (uSize i = 0; i < gSize; ++i) {
            if (*(gData + i) == sObject) {
                return i;
            }
        }

        return INVALID;
    }

    /**
     * Looks for the first occurrence of 'sOther' in 'TArrayView',
     * returns index or uInvalid if there are no occurrences.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    uSize find(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize >= sOther.gSize) {
            uSize i = gSize - sOther.gSize;
            for (uSize j = 0; !(i < j); ++j) {
                for (uSize k = 0;; ++k) {
                    if (k == sOther.gSize) {
                        return j;
                    }

                    if (!(*(gData + j + k) == *(sOther.gData + k))) {
                        break;
                    }
                }
            }
        }

        return INVALID;
    }

    /*!
        Looks for the last occurence of 'object' in 'TArrayView',
        returns index or uInvalid if there are no occurences.
        The behavior is undefined unless 'TArrayView' was initialized.
    */
    uSize findLast(const uObject &object) const
    {
        assert(gData != nullptr || gSize == 0);

        for (uSize i = 0; i < gSize; ++i) {
            if (*(gData + (gSize - 1 - i)) == object) {
                return gSize - 1 - i;
            }
        }

        return INVALID;
    }

    /**
     * Looks for the first occurrence of 'sOther' in 'TArrayView',
     * returns index or uInvalid if there are no occurrences.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    uSize findLast(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize >= sOther.gSize) {
            uSize i = gSize - sOther.gSize;
            for (uSize j = 0; !(i < j); ++j) {
                for (uSize k = 0;; ++k) {
                    if (k == sOther.gSize) {
                        return i - j;
                    }

                    if (!(*(gData + (i - j + k)) == *(sOther.gData + k))) {
                        break;
                    }
                }
            }
        }

        return INVALID;
    }

    /**
     * Returns subarray of 'sHeadSize' first elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sHeadSize' <= 'uSize'.
     *
     * @param sHeadSize
     * @return
     */
    TArrayView head(uSize sHeadSize) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sHeadSize <= gSize);

        return TArrayView(gData, sHeadSize);
    }

    /**
     * Returns subarray of 'sTailSize' last elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sTailSize' <= 'uSize'.
     *
     * @param sTailSize
     * @return
     */
    TArrayView tail(uSize sTailSize) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sTailSize <= gSize);

        return TArrayView(gData + (gSize - sTailSize), sTailSize);
    }

    /**
     * Returns 'TArrayView' without 'sHeadSize' first elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sHeadSize' <= 'uSize'.
     *
     * @param sHeadSize
     * @return
     */
    TArrayView unHead(uSize sHeadSize) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sHeadSize <= gSize);

        return TArrayView(gData + sHeadSize, gSize - sHeadSize);
    }

    /**
     * Returns 'TArrayView' without 'sTailSize' last elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sTailSize' <= 'uSize'.
     *
     * @param sTailSize
     * @return
     */
    TArrayView unTail(uSize sTailSize) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sTailSize <= gSize);

        return TArrayView(gData, gSize - sTailSize);
    }

    /**
     * Returns subarray starting at 'sStartIndex' and contaning 'sEndIndex' - 'sStartIndex' elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sStartIndex' <= 'sEndIndex'
     * and 'sEndIndex' <= 'uSize'.
     *
     * @param sStartIndex
     * @param sEndIndex
     * @return
     */
    TArrayView range(uSize sStartIndex, uSize sEndIndex) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sStartIndex <= sEndIndex && sEndIndex <= gSize);

        return TArrayView(gData + sStartIndex, sEndIndex - sStartIndex);
    }

    /**
     * Returns subarray starting at 'sStartIndex' and containing 'sSliceSize' elements.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sStartIndex' <= 'uSize'
     * and 'sStartIndex' + 'sSliceSize' <= 'uSize'.
     *
     * @param sStartIndex
     * @param sSliceSize
     * @return
     */
    TArrayView slice(uSize sStartIndex, uSize sSliceSize) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sStartIndex <= gSize && sStartIndex + sSliceSize <= gSize);

        return TArrayView(gData + sStartIndex, sSliceSize);
    }

    /**
     * Copy assignment operator.
     * The behavior is undefined unless 'sOther' 'TArrayView' is in defined mState,
     * that is 'uData' != 'nullptr' || 'uSize' == 0
     *
     * @param sOther
     * @return
     */
    TArrayView &operator=(const TArrayView &sOther)
    {
        assert(sOther.gData != nullptr || sOther.gSize == 0);

        gData = sOther.gData;
        gSize = sOther.gSize;

        return *this;
    }

    /**
     * Compare elements of two arrays, return false if there is a difference.
     * EMPTY and NIL arrays are considered equal.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param sOther
     * @return
     */
    bool operator==(TArrayView sOther) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(sOther.gData != nullptr || sOther.gSize == 0);

        if (gSize == sOther.gSize) {
            for (uSize i = 0;; ++i) {
                if (i == gSize) {
                    return true;
                }

                if (!(*(gData + i) == *(sOther.gData + i))) {
                    break;
                }
            }
        }

        return false;
    }

    /**
     * Compare elements two arrays, return false if there is no difference.
     * EMPTY and NIL arrays are considered equal.
     * The behavior is undefined unless both arrays were initialized.
     *
     * @param other
     * @return
     */
    bool operator!=(TArrayView other) const
    {
        assert(gData != nullptr || gSize == 0);

        assert(other.gData != nullptr || other.gSize == 0);

        if (gSize == other.gSize) {
            for (uSize i = 0;; ++i) {
                if (i == gSize) {
                    return false;
                }

                if (*(gData + i) != *(other.gData + i)) {
                    break;
                }
            }
        }

        return true;
    }

    /**
     * Get 'TArrayView' element by sIndex.
     * The behavior is undefined unless 'TArrayView' was initialized and 'sIndex' < 'uSize'.
     *
     * @param sIndex
     * @return
     */
    const uObject &operator[](uSize sIndex) const
    {
        assert(gData != nullptr || gSize == 0);
        assert(sIndex < gSize);
        return *(gData + sIndex);
    }

protected:
    const uObject *gData;
    uSize gSize;
};

template<class QObject, class QSize>
const QSize TArrayView<QObject, QSize>::INVALID = std::numeric_limits<uSize>::max();

template<class QObject, class QSize>
const TArrayView<QObject, QSize> TArrayView<QObject, QSize>::EMPTY(reinterpret_cast<uObject *>(1), 0);

template<class QObject, class QSize>
const TArrayView<QObject, QSize> TArrayView<QObject, QSize>::NIL(nullptr, 0);

} // namespace Common
