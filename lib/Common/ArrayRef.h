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

#include <Common/ArrayView.h>

namespace Common {

    /**
     * @class TArrayRef
     * @namespace Common
     * @brief This is a pair of pointer to object of parametrized sType and uSize.
     *
     * Supports 'EMPTY' and 'NIL' representations as follows:
     * - 'uData' == 'nullptr' && 'uSize' == 0 - EMPTY NIL
     * - 'uData' != 'nullptr' && 'uSize' == 0 - EMPTY NOTNIL
     * - 'uData' == 'nullptr' && 'uSize' > 0 - Undefined
     * - 'uData' != 'nullptr' && 'uSize' > 0 - NOTEMPTY NOTNIL
     * For signed integer 'uSize', 'TArrayRef' with 'uSize' < 0 is undefined.
     *
     * @note It is recommended to pass 'TArrayRef' to procedures by value.
     *
     * @tparam QObjectType
     * @tparam QSizeType
     */
    template <class QObjectType = uint8_t, class QSizeType = size_t>
    class TArrayRef
    {
    public:
        typedef QObjectType uObject;
        typedef QSizeType uSize;

        const static uSize INVALID;
        const static TArrayRef EMPTY;
        const static TArrayRef NIL;

        /**
         * Default constructor.
         * Leaves object uninitialized. Any usage before initializing it is undefined.
         */
        TArrayRef ()
#ifndef NDEBUG // In debug mode, fill in object with INVALID mState (undefined).
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
        TArrayRef (uObject *sArrayData, uSize sArraySize)
            : gData(sArrayData),
              gSize(sArraySize)
        {
            assert(gData != nullptr || gSize == 0);
        }

        /**
         * Constructor from C array.
         * The behavior is undefined unless 'sArrayData' != 'nullptr' || 'sArraySize' == 0.
         * Input mState can be malformed using pointer conversions.
         *
         * @tparam sArraySize
         * @param sArrayData
         */
        template <uSize sArraySize>
        explicit TArrayRef (uObject(&sArrayData)[sArraySize])
            : gData(sArrayData),
              gSize(sArraySize)
        {
            assert(gData != nullptr || gSize == 0);
        }

        /**
         * Copy constructor.
         * Performs default action - bitwise copying of source object.
         * The behavior is undefined unless 'sOther' 'TArrayRef' is in defined mState,
         * that is 'uData' != 'nullptr' || 'uSize' == 0
         *
         * @param sOther
         */
        TArrayRef (const TArrayRef &sOther)
            : gData(sOther.gData),
              gSize(sOther.gSize)
        {
            assert(gData != nullptr || gSize == 0);
        }

        /**
         * Destructor.
         * Does nothing.
         */
        ~TArrayRef () = default;

        uObject *getData () const
        {
            assert(gData != nullptr || gSize == 0);

            return gData;
        }

        uSize getSize () const
        {
            assert(gData != nullptr || gSize == 0);

            return gSize;
        }

        /**
         * Return false if 'TArrayRef' is not EMPTY.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @return
         */
        bool isEmpty () const
        {
            assert(gData != nullptr || gSize == 0);

            return gSize == 0;
        }

        /**
         * Return false if 'TArrayRef' is not NIL.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @return
         */
        bool isNil () const
        {
            assert(gData != nullptr || gSize == 0);

            return gData == nullptr;
        }

        /**
         * Get first element.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'uSize' > 0
         *
         * @return
         */
        uObject &first () const
        {
            assert(gData != nullptr || gSize == 0);

            assert(gSize > 0);

            return *gData;
        }

        /**
         * Get last element.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'uSize' > 0
         *
         * @return
         */
        uObject &last () const
        {
            assert(gData != nullptr || gSize == 0);

            assert(gSize > 0);

            return *(gData + (gSize - 1));
        }

        /**
         * Return a pointer to the first element.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @return
         */
        uObject *begin () const
        {
            assert(gData != nullptr || gSize == 0);

            return gData;
        }

        /**
         * Return a pointer after the last element.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @return
         */
        uObject *end () const
        {
            assert(gData != nullptr || gSize == 0);

            return gData + gSize;
        }

        /**
         * Return false if 'TArrayRef' does not contain 'sObject' at the beginning.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        bool beginsWith (const uObject &sObject) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize == 0) {
                return false;
            }

            return *gData == sObject;
        }

        /**
         * Return false if 'TArrayRef' does not contain 'sOther' at the beginning.
         * The behavior is undefined unless both arrays were initialized.
         *
         * @param sOther
         * @return
         */
        bool beginsWith (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize >= sOther.getSize()) {
                for (uSize i = 0;; ++i) {
                    if (i == sOther.getSize()) {
                        return true;
                    }

                    if (!(*(gData + i) == *(sOther.getData() + i))) {
                        break;
                    }
                }
            }

            return false;
        }

        /**
         * Return false if 'TArrayRef' does not contain 'sObject'.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        bool contains (const uObject &sObject) const
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
         * Return false if 'TArrayRef' does not contain 'sOther'.
         * The behavior is undefined unless both arrays were initialized.
         *
         * @param sOther
         * @return
         */
        bool contains (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; !(i < j); ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return true;
                        }

                        if (!(*(gData + j + k) == *(sOther.getData() + k))) {
                            break;
                        }
                    }
                }
            }

            return false;
        }

        /**
         * Return false if 'TArrayRef' does not contain 'sObject' at the end.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        bool endsWith (const uObject &sObject) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize == 0) {
                return false;
            }

            return *(gData + (gSize - 1)) == sObject;
        }

        /**
         * Return false if 'TArrayRef' does not contain 'sOther' at the end.
         * The behavior is undefined unless both arrays were initialized.
         *
         * @param sOther
         * @return
         */
        bool endsWith (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0;; ++j) {
                    if (j == sOther.getSize()) {
                        return true;
                    }

                    if (!(*(gData + i + j) == *(sOther.getData() + j))) {
                        break;
                    }
                }
            }

            return false;
        }

        /**
         * Looks for the first occurrence of 'sObject' in 'TArrayRef',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        uSize find (const uObject &sObject) const
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
         * Looks for the first occurrence of 'sOther' in 'TArrayRef',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless both arrays were initialized.
         *
         * @param sOther
         * @return
         */
        uSize find (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; !(i < j); ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return j;
                        }

                        if (!(*(gData + j + k) == *(sOther.getData() + k))) {
                            break;
                        }
                    }
                }
            }

            return INVALID;
        }

        /**
         * Looks for the last occurrence of 'sObject' in 'TArrayRef',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        uSize findLast (const uObject &sObject) const
        {
            assert(gData != nullptr || gSize == 0);

            for (uSize i = 0; i < gSize; ++i) {
                if (*(gData + (gSize - 1 - i)) == sObject) {
                    return gSize - 1 - i;
                }
            }

            return INVALID;
        }

        /**
         * Looks for the first occurrence of 'sOther' in 'TArrayRef',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless both arrays were initialized.
         *
         * @param sOther
         * @return
         */
        uSize findLast (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; !(i < j); ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return i - j;
                        }

                        if (!(*(gData + (i - j + k)) == *(sOther.getData() + k))) {
                            break;
                        }
                    }
                }
            }

            return INVALID;
        }

        /**
         * Returns subarray of 'sHeadSize' first elements.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sHeadSize' <= 'uSize'.
         *
         * @param sHeadSize
         * @return
         */
        TArrayRef head (uSize sHeadSize) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sHeadSize <= gSize);

            return TArrayRef(gData, sHeadSize);
        }

        /**
         * Returns subarray of 'sTailSize' last elements.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sTailSize' <= 'uSize'.
         *
         * @param sTailSize
         * @return
         */
        TArrayRef tail (uSize sTailSize) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sTailSize <= gSize);

            return TArrayRef(gData + (gSize - sTailSize), sTailSize);
        }

        /**
         * Returns 'TArrayRef' without 'sHeadSize' first elements.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sHeadSize' <= 'uSize'.
         *
         * @param sHeadSize
         * @return
         */
        TArrayRef unHead (uSize sHeadSize) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sHeadSize <= gSize);

            return TArrayRef(gData + sHeadSize, gSize - sHeadSize);
        }

        /**
         * Returns 'TArrayRef' without 'sTailSize' last elements.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sTailSize' <= 'uSize'.
         *
         * @param sTailSize
         * @return
         */
        TArrayRef unTail (uSize sTailSize) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sTailSize <= gSize);

            return TArrayRef(gData, gSize - sTailSize);
        }

        /**
         * Returns subarray starting at 'sStartIndex' and containing 'sEndIndex' - 'sStartIndex'
         * elements. The behavior is undefined unless 'TArrayRef' was initialized and
         * 'sStartIndex' <= 'sEndIndex' and 'sEndIndex' <= 'uSize'.
         *
         * @param sStartIndex
         * @param sEndIndex
         * @return
         */
        TArrayRef range (uSize sStartIndex, uSize sEndIndex) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sStartIndex <= sEndIndex && sEndIndex <= gSize);

            return TArrayRef(gData + sStartIndex, sEndIndex - sStartIndex);
        }

        /**
         * Returns subarray starting at 'sStartIndex' and containing 'sSliceSize' elements.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sStartIndex' <= 'uSize'
         * and 'sStartIndex' + 'sSliceSize' <= 'uSize'.
         *
         * @param sStartIndex
         * @param sSliceSize
         * @return
         */
        TArrayRef slice (uSize sStartIndex, uSize sSliceSize) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sStartIndex <= gSize && sStartIndex + sSliceSize <= gSize);

            return TArrayRef(gData + sStartIndex, sSliceSize);
        }

        /**
         * Copy 'sObject' to each element of 'TArrayRef'.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @param sObject
         * @return
         */
        const TArrayRef &fill (const uObject &sObject) const
        {
            assert(gData != nullptr || gSize == 0);

            for (uSize i = 0; i < gSize; ++i) {
                *(gData + i) = sObject;
            }

            return *this;
        }

        /**
         * Reverse 'TArrayRef' elements.
         * The behavior is undefined unless 'TArrayRef' was initialized.
         *
         * @return
         */
        const TArrayRef &reverse () const
        {
            assert(gData != nullptr || gSize == 0);

            for (uSize i = 0; i < gSize / 2; ++i) {
                uObject object = *(gData + i);
                *(gData + i) = *(gData + (gSize - 1 - i));
                *(gData + (gSize - 1 - i)) = object;
            }

            return *this;
        }

        /**
         * Copy assignment operator.
         * The behavior is undefined unless 'sOther' 'TArrayRef' is in defined mState,
         * that is 'uData' != 'nullptr' || 'uSize' == 0
         *
         * @param sOther
         * @return
         */
        TArrayRef &operator= (TArrayRef sOther)
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
        bool operator== (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize == sOther.getSize()) {
                for (uSize i = 0;; ++i) {
                    if (i == gSize) {
                        return true;
                    }

                    if (!(*(gData + i) == *(sOther.getData() + i))) {
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
         * @param sOther
         * @return
         */
        bool operator!= (TArrayView <uObject, uSize> sOther) const
        {
            assert(gData != nullptr || gSize == 0);

            if (gSize == sOther.getSize()) {
                for (uSize i = 0;; ++i) {
                    if (i == gSize) {
                        return false;
                    }

                    if (*(gData + i) != *(sOther.getData() + i)) {
                        break;
                    }
                }
            }

            return true;
        }

        /**
         * Get 'TArrayRef' element by sIndex.
         * The behavior is undefined unless 'TArrayRef' was initialized and 'sIndex' < 'uSize'.
         *
         * @param sIndex
         * @return
         */
        uObject &operator[] (uSize sIndex) const
        {
            assert(gData != nullptr || gSize == 0);

            assert(sIndex < gSize);

            return *(gData + sIndex);
        }

        operator TArrayView <uObject, uSize> () const
        {
            return TArrayView <uObject, uSize>(gData, gSize);
        }

    protected:
        uObject *gData;
        uSize gSize;
    };

    template <class QObject, class QSize>
    const QSize TArrayRef <QObject, QSize>::INVALID = std::numeric_limits <uSize>::max();

    template <class QObject, class QSize>
    const TArrayRef <QObject, QSize> TArrayRef <QObject, QSize>::EMPTY(reinterpret_cast<uObject *>(1),
                                                                       0);

    template <class QObject, class QSize>
    const TArrayRef <QObject, QSize> TArrayRef <QObject, QSize>::NIL(nullptr, 0);

} // namespace Common
