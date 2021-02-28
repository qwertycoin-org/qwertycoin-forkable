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
#include <string.h>
#include <Common/StringView.h>

namespace Common {

    /**
     * @class TStringBuffer
     * @namespace Common
     * @brief This is a string of fixed maximum uSize.
     *
     * @tparam uMaximumSizeValue
     */
    template <size_t uMaximumSizeValue>
    class TStringBuffer
    {
    public:
        typedef char cObject;
        typedef size_t uSize;

        const static uSize uMaximumSize = uMaximumSizeValue;
        const static uSize uInvalid;

        static_assert(uMaximumSize != 0, "TStringBuffer's uSize must not be zero");

        /**
         * Default constructor.
         * After construction, 'TStringBuffer' is empty, that is 'uSize' == 0
         */
        TStringBuffer ()
            : gSize(0)
        {
        }

        /**
         * Direct constructor.
         * Copies string from 'cStringData' to 'TStringBuffer'.
         *
         * The behavior is undefined unless:
         * ('cStringData' != 'nullptr' || 'uStringSize' == 0) && 'uStringSize' <= 'uMaximumSize'.
         *
         * @param cStringData
         * @param uStringSize
         */
        TStringBuffer (const cObject *cStringData, uSize uStringSize)
            : gSize(uStringSize)
        {
            assert(cStringData != nullptr || gSize == 0);

            assert(gSize <= uMaximumSize);

            memcpy(gData, cStringData, gSize);
        }

        /**
         * Constructor from C array.
         * Copies string from 'cStringData' to 'TStringBuffer'.
         *
         * The behavior is undefined unless:
         * ('cStringData' != 'nullptr' || 'TStringSize' == 0) && 'TStringSize' <= 'uMaximumSize'
         *
         * Input state can be malformed using pointer conversions.
         *
         * @tparam TStringSize
         * @param cStringData
         */
        template <uSize TStringSize>
        explicit TStringBuffer (const cObject(&cStringData)[TStringSize])
            : gSize(TStringSize - 1)
        {
            assert(cStringData != nullptr || gSize == 0);

            assert(gSize <= uMaximumSize);

            memcpy(gData, cStringData, gSize);
        }

        /**
         * Constructor from QStringView
         * Copies string from 'sStringView' to 'TStringBuffer'.
         *
         * The behavior is undefined unless 'sStringView.uSize()' <= 'uMaximumSize'.
         *
         * @param sStringView
         */
        explicit TStringBuffer (const QStringView &sStringView)
            : gSize(sStringView.getSize())
        {
            assert(gSize <= uMaximumSize);

            memcpy(gData, sStringView.getData(), gSize);
        }

        /**
         * Copy constructor.
         * Copies string from 'sOther' to 'TStringBuffer'.
         *
         * @param sOther
         */
        TStringBuffer (const TStringBuffer &sOther)
            : gSize(sOther.gSize)
        {
            memcpy(gData, sOther.gData, gSize);
        }

        /**
         * Destructor.
         * Does nothing.
         */
        ~TStringBuffer () = default;

        // Copy assignment operator.
        TStringBuffer &operator= (const TStringBuffer &sOther)
        {
            gSize = sOther.gSize;

            memcpy(gData, sOther.gData, gSize);

            return *this;
        }

        /**
         * QStringView assignment operator.
         * Copies string from 'sStringView' to 'TStringBuffer'.
         * The behavior is undefined unless 'sStringView.uSize()' <= 'uMaximumSize'.
         *
         * @param sStringView
         * @return
         */
        TStringBuffer &operator= (const QStringView &sStringView)
        {
            assert(sStringView.getSize() <= uMaximumSize);

            memcpy(gData, sStringView.getData(), sStringView.getSize());

            gSize = sStringView.getSize();

            return *this;
        }

        operator QStringView () const
        {
            return QStringView(gData, gSize);
        }

        operator std::string () const
        {
            return std::string(gData, gSize);
        }

        cObject *getData ()
        {
            return gData;
        }

        const cObject *getData () const
        {
            return gData;
        }

        uSize getSize () const
        {
            return gSize;
        }

        // Return false if 'QStringView' is not EMPTY.
        bool isEmpty () const
        {
            return gSize == 0;
        }

        /**
         * Get 'TStringBuffer' element by uIndex.
         * The behavior is undefined unless 'uIndex' < 'uSize'.
         *
         * @param uIndex
         * @return gData + uIndex
         */
        cObject &operator[] (uSize uIndex)
        {
            assert(uIndex < gSize);

            return *(gData + uIndex);
        }

        /**
         * Get 'TStringBuffer' element by uIndex.
         * The behavior is undefined unless 'uIndex' < 'uSize'.
         *
         * @param uIndex
         * @return gData + uIndex
         */
        const cObject &operator[] (uSize uIndex) const
        {
            assert(uIndex < gSize);

            return *(gData + uIndex);
        }

        /**
         * Get first element.
         * The behavior is undefined unless 'uSize' > 0
         *
         * @return gData
         */
        cObject &first ()
        {
            assert(gSize > 0);

            return *gData;
        }

        /**
         * Get first element.
         * The behavior is undefined unless 'uSize' > 0
         *
         * @return gData
         */
        const cObject &first () const
        {
            assert(gSize > 0);

            return *gData;
        }

        /**
         * Get last element.
         * The behavior is undefined unless 'uSize' > 0
         *
         * @return gData + (gSize - 1)
         */
        cObject &last ()
        {
            assert(gSize > 0);

            return *(gData + (gSize - 1));
        }

        /**
         * Get last element.
         * The behavior is undefined unless 'uSize' > 0
         *
         * @return gData + (gSize - 1)
         */
        const cObject &last () const
        {
            assert(gSize > 0);

            return *(gData + (gSize - 1));
        }

        // Return a pointer to the first element.
        cObject *begin ()
        {
            return gData;
        }

        // Return a pointer to the first element.
        const cObject *begin () const
        {
            return gData;
        }

        // Return a pointer after the last element.
        cObject *end ()
        {
            return gData + gSize;
        }

        // Return a pointer after the last element.
        const cObject *end () const
        {
            return gData + gSize;
        }

        // Compare elements of two strings, return false if there is a difference.
        bool operator== (const QStringView &sOther) const
        {
            if (gSize == sOther.getSize()) {
                for (uSize i = 0;; ++i) {
                    if (i == gSize) {
                        return true;
                    }

                    if (*(gData + i) != *(sOther.getData() + i)) {
                        break;
                    }
                }
            }

            return false;
        }

        // Compare elements two strings, return false if there is no difference.
        bool operator!= (const QStringView &sOther) const
        {
            return !(*this == sOther);
        }

        // Compare two strings character-wise.
        bool operator< (const QStringView &sOther) const
        {
            uSize uCount = sOther.getSize() < gSize ? sOther.getSize() : gSize;
            for (uSize i = 0; i < uCount; ++i) {
                cObject char1 = *(gData + i);
                cObject char2 = *(sOther.getData() + i);
                if (char1 < char2) {
                    return true;
                }

                if (char2 < char1) {
                    return false;
                }
            }

            return gSize < sOther.getSize();
        }

        // Compare two strings character-wise.
        bool operator<= (QStringView sOther) const
        {
            return !(sOther < *this);
        }

        // Compare two strings character-wise.
        bool operator> (QStringView sOther) const
        {
            return sOther < *this;
        }

        // Compare two strings character-wise.
        bool operator>= (QStringView sOther) const
        {
            return !(*this < sOther);
        }

        // Return false if 'QStringView' does not contain 'sObject' at the beginning.
        bool beginsWith (const cObject &sObject) const
        {
            if (gSize == 0) {
                return false;
            }

            return *gData == sObject;
        }

        // Return false if 'QStringView' does not contain 'sOther' at the beginning.
        bool beginsWith (const QStringView &sOther) const
        {
            if (gSize >= sOther.getSize()) {
                for (uSize i = 0;; ++i) {
                    if (i == sOther.getSize()) {
                        return true;
                    }

                    if (*(gData + i) != *(sOther.getData() + i)) {
                        break;
                    }
                }
            }

            return false;
        }

        // Return false if 'QStringView' does not contain 'sObject'.
        bool contains (const cObject &sObject) const
        {
            for (uSize i = 0; i < gSize; ++i) {
                if (*(gData + i) == sObject) {
                    return true;
                }
            }

            return false;
        }

        // Return false if 'QStringView' does not contain 'sOther'.
        bool contains (const QStringView &sOther) const
        {
            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; i >= j; ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return true;
                        }

                        if (*(gData + j + k) != *(sOther.getData() + k)) {
                            break;
                        }
                    }
                }
            }

            return false;
        }

        // Return false if 'QStringView' does not contain 'sObject' at the end.
        bool endsWith (const cObject &sObject) const
        {
            if (gSize == 0) {
                return false;
            }

            return *(gData + (gSize - 1)) == sObject;
        }

        // Return false if 'QStringView' does not contain 'sOther' at the end.
        bool endsWith (const QStringView &sOther) const
        {
            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0;; ++j) {
                    if (j == sOther.getSize()) {
                        return true;
                    }

                    if (*(gData + i + j) != *(sOther.getData() + j)) {
                        break;
                    }
                }
            }

            return false;
        }

        /**
         * Looks for the first occurrence of 'sObject' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         *
         * @param sObject
         * @return
         */
        uSize find (const cObject &sObject) const
        {
            for (uSize i = 0; i < gSize; ++i) {
                if (*(gData + i) == sObject) {
                    return i;
                }
            }

            return uInvalid;
        }

        /**
         * Looks for the first occurrence of 'sOther' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         *
         * @param sOther
         * @return uInvalid
         */
        uSize find (const QStringView &sOther) const
        {
            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; i >= j; ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return j;
                        }

                        if (*(gData + j + k) != *(sOther.getData() + k)) {
                            break;
                        }
                    }
                }
            }

            return uInvalid;
        }

        /**
         * Looks for the last occurrence of 'sObject' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         *
         * @param sObject
         * @return uInvalid
         */
        uSize findLast (const cObject &sObject) const
        {
            for (uSize i = 0; i < gSize; ++i) {
                if (*(gData + (gSize - 1 - i)) == sObject) {
                    return gSize - 1 - i;
                }
            }

            return uInvalid;
        }

        /**
         * Looks for the first occurrence of 'sOther' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         *
         * @param sOther
         * @return uInvalid
         */
        uSize findLast (QStringView sOther) const
        {
            if (gSize >= sOther.getSize()) {
                uSize i = gSize - sOther.getSize();
                for (uSize j = 0; i >= j; ++j) {
                    for (uSize k = 0;; ++k) {
                        if (k == sOther.getSize()) {
                            return i - j;
                        }

                        if (*(gData + (i - j + k)) != *(sOther.getData() + k)) {
                            break;
                        }
                    }
                }
            }

            return uInvalid;
        }

        /**
         * Returns substring of 'uHeadSize' first elements.
         * The behavior is undefined unless 'uHeadSize' <= 'uSize'.
         *
         * @param uHeadSize
         * @return QStringView(gData, uHeadSize)
         */
        QStringView head (uSize uHeadSize) const
        {
            assert(uHeadSize <= gSize);

            return QStringView(gData, uHeadSize);
        }

        /**
         * Returns substring of 'uTailSize' last elements.
         * The behavior is undefined unless 'uTailSize' <= 'uSize'.
         *
         * @param uTailSize
         * @return QStringView(gData + (gSize - uTailSize), uTailSize)
         */
        QStringView tail (uSize uTailSize) const
        {
            assert(uTailSize <= gSize);

            return QStringView(gData + (gSize - uTailSize), uTailSize);
        }

        /**
         * Returns 'QStringView' without 'uHeadSize' first elements.
         * The behavior is undefined unless 'uHeadSize' <= 'uSize'.
         *
         * @param uHeadSize
         * @return QStringView(gData + uHeadSize, gSize - uHeadSize)
         */
        QStringView unHead (uSize uHeadSize) const
        {
            assert(uHeadSize <= gSize);

            return QStringView(gData + uHeadSize, gSize - uHeadSize);
        }

        /**
         * Returns 'QStringView' without 'uTailSize' last elements.
         * The behavior is undefined unless 'uTailSize' <= 'uSize'.
         *
         * @param uTailSize
         * @return QStringView(gData, gSize - uTailSize)
         */
        QStringView unTail (uSize uTailSize) const
        {
            assert(uTailSize <= gSize);

            return QStringView(gData, gSize - uTailSize);
        }

        /**
         * Returns substring starting at 'uStartIndex' and containing 'uEndIndex' - 'uStartIndex' elements.
         * The behavior is undefined unless 'uStartIndex' <= 'uEndIndex' and 'uEndIndex' <= 'uSize'.
         *
         * @param uStartIndex
         * @param uEndIndex
         * @return QStringView(gData + uStartIndex, uEndIndex - uStartIndex)
         */
        QStringView range (uSize uStartIndex, uSize uEndIndex) const
        {
            assert(uStartIndex <= uEndIndex && uEndIndex <= gSize);

            return QStringView(gData + uStartIndex, uEndIndex - uStartIndex);
        }

        /**
         * Returns substring starting at 'uStartIndex' and containing 'uSliceSize' elements.
         * The behavior is undefined unless:
         * 'uStartIndex' <= 'uSize' and 'uSliceSize' <= 'uSize' - 'uStartIndex'.
         *
         * @param uStartIndex
         * @param uSliceSize
         * @return QStringView(gData + uStartIndex, uSliceSize)
         */
        QStringView slice (uSize uStartIndex, uSize uSliceSize) const
        {
            assert(uStartIndex <= gSize && uSliceSize <= gSize - uStartIndex);

            return QStringView(gData + uStartIndex, uSliceSize);
        }

        /**
         * Appends 'sObject' to 'TStringBuffer'.
         * The behavior is undefined unless 1 <= 'uMaximumSize' - 'uSize'.
         *
         * @param sObject
         * @return
         */
        TStringBuffer &append (cObject sObject)
        {
            assert(1 <= uMaximumSize - gSize);

            gData[gSize] = sObject;
            ++gSize;

            return *this;
        }

        /**
         * Appends 'sStringView' to 'TStringBuffer'.
         * The behavior is undefined unless 'sStringView.uSize()' <= 'uMaximumSize' - 'uSize'.
         *
         * @param sStringView
         * @return
         */
        TStringBuffer &append (const QStringView &sStringView)
        {
            assert(sStringView.getSize() <= uMaximumSize - gSize);

            if (sStringView.getSize() != 0) {
                memcpy(gData + gSize, sStringView.getData(), sStringView.getSize());
                gSize += sStringView.getSize();
            }

            return *this;
        }

        // Sets 'TStringBuffer' to empty state, that is 'uSize' == 0
        TStringBuffer &clear ()
        {
            gSize = 0;

            return *this;
        }

        /**
         * Removes substring starting at 'uStartIndex' and containing 'uCutSize' elements.
         * The behavior is undefined unless:
         * 'uStartIndex' <= 'uSize' and 'uCutSize' <= 'uSize' - 'uStartIndex'.
         *
         * @param uStartIndex
         * @param uCutSize
         * @return
         */
        TStringBuffer &cut (uSize uStartIndex, uSize uCutSize)
        {
            assert(uStartIndex <= gSize && uCutSize <= gSize - uStartIndex);

            if (uCutSize != 0) {
                memcpy(gData + uStartIndex, gData + uStartIndex + uCutSize,
                       gSize - uStartIndex - uCutSize);
                gSize -= uCutSize;
            }

            return *this;
        }

        // Copy 'sObject' to each element of 'TStringBuffer'.
        TStringBuffer &fill (cObject sObject)
        {
            if (gSize > 0) {
                memset(gData, sObject, gSize);
            }

            return *this;
        }

        /**
         * Inserts 'sObject' into 'TStringBuffer' at 'uIndex'.
         * The behavior is undefined unless 'uIndex' <= 'uSize' and 1 <= 'uMaximumSize' - 'uSize'.
         *
         * @param uIndex
         * @param sObject
         * @return
         */
        TStringBuffer &insert (uSize uIndex, cObject sObject)
        {
            assert(uIndex <= gSize);

            assert(1 <= uMaximumSize - gSize);

            memmove(gData + uIndex + 1, gData + uIndex, gSize - uIndex);
            gData[uIndex] = sObject;
            ++gSize;

            return *this;
        }

        /**
         * Inserts 'sStringView' into 'TStringBuffer' at 'uIndex'.
         * The behavior is undefined unless:
         * 'uIndex' <= 'uSize' and 'sStringView.uSize()' <= 'uMaximumSize' - 'uSize'.
         *
         * @param uIndex
         * @param sStringView
         * @return
         */
        TStringBuffer &insert (uSize uIndex, const QStringView &sStringView)
        {
            assert(uIndex <= gSize);

            assert(sStringView.getSize() <= uMaximumSize - gSize);

            if (sStringView.getSize() != 0) {
                memmove(gData + uIndex + sStringView.getSize(), gData + uIndex, gSize - uIndex);
                memcpy(gData + uIndex, sStringView.getData(), sStringView.getSize());
                gSize += sStringView.getSize();
            }

            return *this;
        }

        /**
         * Overwrites 'TStringBuffer' starting at 'uIndex' with 'sStringView',
         * possibly expanding 'TStringBuffer'. The behavior is undefined unless:
         * 'uIndex' <= 'uSize' and 'sStringView.uSize()' <= 'uMaximumSize' - 'uIndex'.
         *
         * @param uIndex
         * @param sStringView
         * @return
         */
        TStringBuffer &overwrite (uSize uIndex, const QStringView &sStringView)
        {
            assert(uIndex <= gSize);

            assert(sStringView.getSize() <= uMaximumSize - uIndex);

            memcpy(gData + uIndex, sStringView.getData(), sStringView.getSize());
            if (gSize < uIndex + sStringView.getSize()) {
                gSize = uIndex + sStringView.getSize();
            }

            return *this;
        }

        /**
         * Sets 'uSize' to 'mBufferSize', assigning value '\0' to newly inserted elements.
         * The behavior is undefined unless 'mBufferSize' <= 'uMaximumSize'.
         *
         * @param uBufferSize
         * @return
         */
        TStringBuffer &resize (uSize uBufferSize)
        {
            assert(uBufferSize <= uMaximumSize);

            if (uBufferSize > gSize) {
                memset(gData + gSize, 0, uBufferSize - gSize);
            }

            gSize = uBufferSize;

            return *this;
        }

        // Reverse 'TStringBuffer' elements.
        TStringBuffer &reverse ()
        {
            for (uSize i = 0; i < gSize / 2; ++i) {
                cObject object = *(gData + i);
                *(gData + i) = *(gData + (gSize - 1 - i));
                *(gData + (gSize - 1 - i)) = object;
            }

            return *this;
        }

        /**
         * Sets 'uSize' to 'mBufferSize'.
         * The behavior is undefined unless 'mBufferSize' <= 'uSize'.
         *
         * @param uBufferSize
         * @return
         */
        TStringBuffer &shrink (uSize uBufferSize)
        {
            assert(uBufferSize <= gSize);

            gSize = uBufferSize;

            return *this;
        }

    protected:
        cObject gData[uMaximumSize];
        uSize gSize;
    };

    template <size_t uMaximumSize>
    const typename TStringBuffer
        <uMaximumSize>::uSize TStringBuffer
        <uMaximumSize>::uInvalid = std::numeric_limits
        <typename TStringBuffer
            <uMaximumSize>::Size>::max();

} // namespace Common
