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
#include <string>

namespace Common {

/**
 * @class QStringView
 * @namespace Common
 * @brief This is a pair of pointer to constant char and uSize.
 *
 * Supports 'EMPTY' and 'NIL' representations as follows:
 * - 'gData' == 'nullptr' && 'uSize' == 0 - EMPTY NIL
 * - 'gData' != 'nullptr' && 'uSize' == 0 - EMPTY NOTNIL
 * - 'gData' == 'nullptr' && 'uSize' > 0 - Undefined
 * - 'gData' != 'nullptr' && 'uSize' > 0 - NOTEMPTY NOTNIL
 *
 * @note It is recommended to pass `QStringView` to procedures by value.
 */
    class QStringView
    {
    public:
        typedef char Object;
        typedef size_t Size;

        const static Size INVALID;
        const static QStringView EMPTY;
        const static QStringView NIL;

        /**
         * Default constructor.
         * Leaves object uninitialized. Any usage before initializing it is undefined.
         */
        QStringView ();

        /**
         * Direct constructor.
         * The behavior is undefined unless 'stringData' != 'nullptr' || 'stringSize' == 0
         *
         * @param stringData
         * @param stringSize
         */
        QStringView (const Object *stringData, Size stringSize);

        /**
         * Constructor from C array.
         * The behavior is undefined unless 'stringData' != 'nullptr' || 'stringSize' == 0.
         * Input state can be malformed using poiner conversions.
         *
         * @tparam stringSize
         * @param stringData
         */
        template <Size stringSize>
        QStringView (const Object(&stringData)[stringSize])
            : gData(stringData),
              gSize(stringSize - 1)
        {
            assert(gData != nullptr || gSize == 0);
        }

        /**
         * Constructor from std::string
         *
         * @param string
         */
        QStringView (const std::string &string);

        /**
         * Copy constructor.
         * Performs default action - bitwise copying of source object.
         * The behavior is undefined unless 'other' 'QStringView' is in defined state,
         * that is 'gData' != 'nullptr' || 'uSize' == 0
         *
         * @param other
         */
        QStringView (const QStringView &other);

        /**
         * Destructor.
         * Does nothing.
         */
        ~QStringView () = default;

        /**
         * Copy assignment operator.
         * The behavior is undefined unless 'other' 'QStringView' is in defined state,
         * that is 'gData' != 'nullptr' || 'uSize' == 0
         *
         * @param other
         * @return
         */
        QStringView &operator= (const QStringView &other);

        explicit operator std::string () const;

        const Object *getData () const;

        Size getSize () const;

        /**
         * Return false if 'QStringView' is not EMPTY.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @return
         */
        bool isEmpty () const;

        /**
         * Return false if 'QStringView' is not NIL.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @return
         */
        bool isNil () const;

        /**
         * Get 'QStringView' element by index.
         * The behavior is undefined unless 'QStringView' was initialized and 'index' < 'uSize'.
         *
         * @param index
         * @return
         */
        const Object &operator[] (Size index) const;

        /**
         * Get first element.
         * The behavior is undefined unless 'QStringView' was initialized and 'uSize' > 0
         *
         * @return
         */
        const Object &first () const;

        /**
         * Get last element.
         * The behavior is undefined unless 'QStringView' was initialized and 'uSize' > 0
         *
         * @return
         */
        const Object &last () const;

        /**
         * Return a pointer to the first element.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @return
         */
        const Object *begin () const;

        /**
         * Return a pointer after the last element.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @return
         */
        const Object *end () const;

        /**
         * Compare elements of two strings, return false if there is a difference.
         * EMPTY and NIL strings are considered equal.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator== (QStringView other) const;

        /**
         * Compare elements two strings, return false if there is no difference.
         * EMPTY and NIL strings are considered equal.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator!= (QStringView other) const;

        /**
         * Compare two strings character-wise.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator< (QStringView other) const;

        /**
         * Compare two strings character-wise.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator<= (QStringView other) const;

        /**
         * Compare two strings character-wise.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator> (QStringView other) const;

        /**
         * Compare two strings character-wise.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool operator>= (QStringView other) const;

        /**
         * Return false if 'QStringView' does not contain 'object' at the beginning.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @param object
         * @return
         */
        bool beginsWith (const Object &object) const;

        /**
         * Return false if 'QStringView' does not contain 'other' at the beginning.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool beginsWith (QStringView other) const;

        /**
         * Return false if 'QStringView' does not contain 'object'.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @param object
         * @return
         */
        bool contains (const Object &object) const;

        /**
         * Return false if 'QStringView' does not contain 'other'.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool contains (QStringView other) const;

        /**
         * Return false if 'QStringView' does not contain 'object' at the end.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @param object
         * @return
         */
        bool endsWith (const Object &object) const;

        /**
         * Return false if 'QStringView' does not contain 'other' at the end.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        bool endsWith (QStringView other) const;

        /**
         * Looks for the first occurrence of 'object' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @param object
         * @return
         */
        Size find (const Object &object) const;

        /**
         * Looks for the first occurrence of 'other' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        Size find (QStringView other) const;

        /**
         * Looks for the last occurrence of 'object' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless 'QStringView' was initialized.
         *
         * @param object
         * @return
         */
        Size findLast (const Object &object) const;

        /**
         * Looks for the first occurrence of 'other' in 'QStringView',
         * returns index or uInvalid if there are no occurrences.
         * The behavior is undefined unless both strings were initialized.
         *
         * @param other
         * @return
         */
        Size findLast (QStringView other) const;

        /**
         * Returns substring of 'headSize' first elements.
         * The behavior is undefined unless 'QStringView' was initialized and 'headSize' <= 'uSize'.
         *
         * @param headSize
         * @return
         */
        QStringView head (Size headSize) const;

        /**
         * Returns substring of 'tailSize' last elements.
         * The behavior is undefined unless 'QStringView' was initialized and 'tailSize' <= 'uSize'.
         *
         * @param tailSize
         * @return
         */
        QStringView tail (Size tailSize) const;

        /**
         * Returns 'QStringView' without 'headSize' first elements.
         * The behavior is undefined unless 'QStringView' was initialized and 'headSize' <= 'uSize'.
         *
         * @param headSize
         * @return
         */
        QStringView unhead (Size headSize) const;

        /**
         * Returns 'QStringView' without 'tailSize' last elements.
         * The behavior is undefined unless 'QStringView' was initialized and 'tailSize' <= 'uSize'.
         *
         * @param tailSize
         * @return
         */
        QStringView untail (Size tailSize) const;

        /**
         * Returns substring starting at 'startIndex' and containing 'endIndex' - 'startIndex'
         * elements. The behavior is undefined unless 'QStringView' was initialized and
         * 'startIndex' <= 'endIndex' and 'endIndex' <= 'uSize'.
         *
         * @param startIndex
         * @param endIndex
         * @return
         */
        QStringView range (Size startIndex, Size endIndex) const;

        /**
         * Returns substring starting at 'startIndex' and contaning 'sliceSize' elements.
         * The behavior is undefined unless 'QStringView' was initialized and
         * 'startIndex' <= 'uSize' and 'startIndex' + 'sliceSize' <= 'uSize'.
         *
         * @param startIndex
         * @param sliceSize
         * @return
         */
        QStringView slice (Size startIndex, Size sliceSize) const;

    protected:
        const Object *gData;
        Size gSize;
    };

} // namespace Common
