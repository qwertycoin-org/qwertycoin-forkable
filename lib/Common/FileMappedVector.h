// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2019, Karbo developers
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
#include <cstdint>
#include <string>

#include <boost/filesystem.hpp>

#include <Android.h>

#include <Common/ScopeExit.h>

#include <System/MemoryMappedFile.h>

namespace Common {

    template <class T>
    struct FEnableIfPod
    {
        typedef typename std::enable_if <std::is_pod <T>::value, FEnableIfPod>::type type;
    };

    enum class EFileMappedVectorOpenMode
    {
        OPEN,
        CREATE,
        OPEN_OR_CREATE
    };

    template <class T>
    class QFileMappedVector : public FEnableIfPod <T>::type
    {
    public:
        typedef T sValueType;

        const static uint64_t uMetadataSize = static_cast<uint64_t>(sizeof(uint64_t) * 2);
        const static uint64_t uValueSize = static_cast<uint64_t>(sizeof(T));

        class QConstIterator
        {
        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef T value_type;
            typedef ptrdiff_t difference_type;
            typedef const T *pointer;
            typedef const T &reference;

            QConstIterator ()
                : gFileMappedVector(nullptr),
                  gIndex(0)
            {
            }

            QConstIterator (const QFileMappedVector *sFileMappedVector,
                            size_t uIndex)
                : gFileMappedVector(sFileMappedVector),
                  gIndex(uIndex)
            {
            }

            const T &operator* () const
            {
                return (*gFileMappedVector)[gIndex];
            }

            const T *operator-> () const
            {
                return &(*gFileMappedVector)[gIndex];
            }

            QConstIterator &operator++ ()
            {
                ++gIndex;

                return *this;
            }

            QConstIterator operator++ (int)
            {
                QConstIterator tmp = *this;
                ++gIndex;

                return tmp;
            }

            QConstIterator &operator-- ()
            {
                --gIndex;

                return *this;
            }

            QConstIterator operator-- (int)
            {
                QConstIterator tmp = *this;
                --gIndex;

                return tmp;
            }

            QConstIterator &operator+= (difference_type n)
            {
                gIndex += n;

                return *this;
            }

            QConstIterator operator+ (difference_type n) const
            {
                return QConstIterator(gFileMappedVector, gIndex + n);
            }

            friend QConstIterator operator+ (difference_type n,
                                             const QConstIterator &i)
            {
                return QConstIterator(i.gFileMappedVector, n + i.gIndex);
            }

            QConstIterator &operator-= (difference_type n)
            {
                gIndex -= n;

                return *this;
            }

            QConstIterator operator- (difference_type n) const
            {
                return QConstIterator(gFileMappedVector, gIndex - n);
            }

            difference_type operator- (const QConstIterator &other) const
            {
                return gIndex - other.gIndex;
            }

            const T &operator[] (difference_type offset) const
            {
                return (*gFileMappedVector)[gIndex + offset];
            }

            bool operator== (const QConstIterator &other) const
            {
                return gIndex == other.gIndex;
            }

            bool operator!= (const QConstIterator &other) const
            {
                return gIndex != other.gIndex;
            }

            bool operator< (const QConstIterator &other) const
            {
                return gIndex < other.gIndex;
            }

            bool operator> (const QConstIterator &other) const
            {
                return gIndex > other.gIndex;
            }

            bool operator<= (const QConstIterator &other) const
            {
                return gIndex <= other.gIndex;
            }

            bool operator>= (const QConstIterator &other) const
            {
                return gIndex >= other.gIndex;
            }

            size_t index () const
            {
                return gIndex;
            }

        protected:
            const QFileMappedVector *gFileMappedVector;
            size_t gIndex;
        };

        class QIterator : public QConstIterator
        {
        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef T value_type;
            typedef ptrdiff_t difference_type;
            typedef T *pointer;
            typedef T &reference;

            QIterator ()
                : QConstIterator()
            {
            }

            QIterator (const QFileMappedVector *sFileMappedVector,
                       size_t uIndex)
                : QConstIterator(sFileMappedVector, uIndex)
            {
            }

            T &operator* () const
            {
                return const_cast<T &>((*QConstIterator::gFileMappedVector)[QConstIterator::gIndex]);
            }

            T *operator-> () const
            {
                return const_cast<T *>(&(*QConstIterator::gFileMappedVector)[QConstIterator::gIndex]);
            }

            QIterator &operator++ ()
            {
                ++QConstIterator::gIndex;

                return *this;
            }

            QIterator operator++ (int)
            {
                QIterator tmp = *this;
                ++QConstIterator::gIndex;

                return tmp;
            }

            QIterator &operator-- ()
            {
                --QConstIterator::gIndex;

                return *this;
            }

            QIterator operator-- (int)
            {
                QIterator tmp = *this;
                --QConstIterator::gIndex;

                return tmp;
            }

            QIterator &operator+= (difference_type n)
            {
                QConstIterator::gIndex += n;

                return *this;
            }

            QIterator operator+ (difference_type n) const
            {
                return QIterator(QConstIterator::gFileMappedVector,
                                 QConstIterator::gIndex + n);
            }

            friend QIterator operator+ (difference_type n, const QIterator &i)
            {
                return QIterator(i.gFileMappedVector,
                                 n + i.gIndex);
            }

            QIterator &operator-= (difference_type n)
            {
                QConstIterator::gIndex -= n;

                return *this;
            }

            QIterator operator- (difference_type n) const
            {
                return QIterator(QConstIterator::gFileMappedVector,
                                 QConstIterator::gIndex - n);
            }

            difference_type operator- (const QIterator &other) const
            {
                return QConstIterator::gIndex - other.gIndex;
            }

            T &operator[] (difference_type offset) const
            {
                return (*QConstIterator::gFileMappedVector)[QConstIterator::gIndex + offset];
            }
        };

        QFileMappedVector ();

        explicit QFileMappedVector (
            const std::string &cPath,
            EFileMappedVectorOpenMode sMode = EFileMappedVectorOpenMode::OPEN_OR_CREATE,
            uint64_t uPrefixSize = 0);

        QFileMappedVector (const QFileMappedVector &) = delete;

        QFileMappedVector &operator= (const QFileMappedVector &) = delete;

        void open (const std::string &cPath,
                   EFileMappedVectorOpenMode sMode = EFileMappedVectorOpenMode::OPEN_OR_CREATE,
                   uint64_t uPrefixSize = 0);

        void close ();

        void close (std::error_code &sEc);

        bool isOpened () const;

        bool empty () const;

        uint64_t capacity () const;

        uint64_t size () const;

        void reserve (uint64_t n);

        void shrinkToFit ();

        QConstIterator begin () const;

        QIterator begin ();

        QConstIterator cbegin () const;

        QConstIterator end () const;

        QIterator end ();

        QConstIterator cend () const;

        const T &operator[] (uint64_t uIndex) const;

        T &operator[] (uint64_t uIndex);

        const T &at (uint64_t uIndex) const;

        T &at (uint64_t uIndex);

        const T &front () const;

        T &front ();

        const T &back () const;

        T &back ();

        const T *data () const;

        T *data ();

        void clear ();

        QIterator erase (QConstIterator sPosition);

        QIterator erase (QConstIterator sFirst, QConstIterator sLast);

        QIterator insert (QConstIterator sPosition, const T &sValue);

        template <class InputIterator>
        QIterator insert (QConstIterator sPosition, InputIterator sFirst, InputIterator sLast);

        void pop_back ();

        void push_back (const T &sValue);

        void swap (QFileMappedVector &sOther);

        bool getAutoFlush () const;

        void setAutoFlush (bool bAutoFlush);

        void flush ();

        const uint8_t *prefix () const;

        uint8_t *prefix ();

        uint64_t prefixSize () const;

        void resizePrefix (uint64_t uNewPrefixSize);

        const uint8_t *suffix () const;

        uint8_t *suffix ();

        uint64_t suffixSize () const;

        void resizeSuffix (uint64_t uNewSuffixSize);

        void rename (const std::string &cNewPath, std::error_code &sEc);

        void rename (const std::string &cNewPath);

        template <class F>
        void atomicUpdate (F &&sFunc);

    private:
        template <class F>
        void atomicUpdate (uint64_t uNewSize,
                           uint64_t uNewCapacity,
                           uint64_t uNewPrefixSize,
                           uint64_t uNewSuffixSize,
                           F &&sFunc);

        template <class F>
        void atomicUpdate0 (uint64_t uNewCapacity,
                            uint64_t uNewPrefixSize,
                            uint64_t uNewSuffixSize,
                            F &&sFunc);

        void open (const std::string &cPath, uint64_t uPrefixSize);

        void create (const std::string &cPath,
                     uint64_t uInitialCapacity,
                     uint64_t uPrefixSize,
                     uint64_t uSuffixSize);

        uint8_t *prefixPtr ();

        const uint8_t *prefixPtr () const;

        uint64_t *capacityPtr ();

        const uint64_t *capacityPtr () const;

        const uint64_t *sizePtr () const;

        uint64_t *sizePtr ();

        T *vectorDataPtr ();

        const T *vectorDataPtr () const;

        uint8_t *suffixPtr ();

        const uint8_t *suffixPtr () const;

        uint64_t vectorDataSize ();

        uint64_t nextCapacity ();

        void flushElement (uint64_t uIndex);

        void flushSize ();

    private:
        std::string mPath;
        System::MemoryMappedFile mFile;
        uint64_t mPrefixSize;
        uint64_t mSuffixSize;
        bool mAutoFlush;
    };

    template <class T>
    QFileMappedVector <T>::QFileMappedVector ()
        : mAutoFlush(true),
          mPrefixSize(0),
          mSuffixSize(0)
    {
    }

    template <class T>
    QFileMappedVector <T>::QFileMappedVector (const std::string &cPath,
                                              EFileMappedVectorOpenMode sMode,
                                              uint64_t uPrefixSize)
        : mAutoFlush(true),
          mPrefixSize(uPrefixSize),
          mSuffixSize(0)
    {
        open(cPath, sMode, uPrefixSize);
    }

    template <class T>
    void QFileMappedVector <T>::open (const std::string &cPath,
                                      EFileMappedVectorOpenMode sMode,
                                      uint64_t uPrefixSize)
    {
        assert(!isOpened());

        const uint64_t uInitialCapacity = 10;

        boost::filesystem::path filePath = cPath;
        boost::filesystem::path bakPath = cPath + ".bak";
        bool bFileExists;
        if (boost::filesystem::exists(filePath)) {
            if (boost::filesystem::exists(bakPath)) {
                boost::filesystem::remove(bakPath);
            }
            bFileExists = true;
        } else if (boost::filesystem::exists(bakPath)) {
            boost::filesystem::rename(bakPath, filePath);
            bFileExists = true;
        } else {
            bFileExists = false;
        }

        if (sMode == EFileMappedVectorOpenMode::OPEN) {
            open(cPath, uPrefixSize);
        } else if (sMode == EFileMappedVectorOpenMode::CREATE) {
            create(cPath, uInitialCapacity, uPrefixSize, 0);
        } else if (sMode == EFileMappedVectorOpenMode::OPEN_OR_CREATE) {
            if (bFileExists) {
                open(cPath, uPrefixSize);
            } else {
                create(cPath, uInitialCapacity, uPrefixSize, 0);
            }
        } else {
            throw std::runtime_error("QFileMappedVector: Unsupported open sMode: "
                                     + std::to_string(static_cast<int>(sMode)));
        }
    }

    template <class T>
    void QFileMappedVector <T>::close (std::error_code &sEc)
    {
        mFile.close(sEc);
        if (!sEc) {
            mPrefixSize = 0;
            mSuffixSize = 0;
            mPath.clear();
        }
    }

    template <class T>
    void QFileMappedVector <T>::close ()
    {
        std::error_code sEc;
        close(sEc);
        if (sEc) {
            throw std::system_error(sEc, "QFileMappedVector::close");
        }
    }

    template <class T>
    bool QFileMappedVector <T>::isOpened () const
    {
        return mFile.isOpened();
    }

    template <class T>
    bool QFileMappedVector <T>::empty () const
    {
        assert(isOpened());

        return size() == 0;
    }

    template <class T>
    uint64_t QFileMappedVector <T>::capacity () const
    {
        assert(isOpened());

        return *capacityPtr();
    }

    template <class T>
    uint64_t QFileMappedVector <T>::size () const
    {
        assert(isOpened());

        return *sizePtr();
    }

    template <class T>
    void QFileMappedVector <T>::reserve (uint64_t n)
    {
        assert(isOpened());

        if (n > capacity()) {
            atomicUpdate(size(),
                         n,
                         prefixSize(),
                         suffixSize(),
                         [this] (sValueType *target)
                         {
                             std::copy(cbegin(), cend(), target);
                         });
        }
    }

    template <class T>
    void QFileMappedVector <T>::shrinkToFit ()
    {
        assert(isOpened());

        if (size() < capacity()) {
            atomicUpdate(size(),
                         size(),
                         prefixSize(),
                         suffixSize(),
                         [this] (sValueType *sTarget)
                         {
                             std::copy(cbegin(), cend(), sTarget);
                         });
        }
    }

    template <class T>
    typename QFileMappedVector <T>::QIterator QFileMappedVector <T>::begin ()
    {
        assert(isOpened());

        return QIterator(this, 0);
    }

    template <class T>
    typename QFileMappedVector <T>::QConstIterator QFileMappedVector <T>::begin () const
    {
        assert(isOpened());

        return QConstIterator(this, 0);
    }

    template <class T>
    typename QFileMappedVector <T>::QConstIterator QFileMappedVector <T>::cbegin () const
    {
        assert(isOpened());

        return QConstIterator(this, 0);
    }

    template <class T>
    typename QFileMappedVector <T>::QConstIterator QFileMappedVector <T>::end () const
    {
        assert(isOpened());

        return QConstIterator(this, size());
    }

    template <class T>
    typename QFileMappedVector <T>::QIterator QFileMappedVector <T>::end ()
    {
        assert(isOpened());

        return QIterator(this, size());
    }

    template <class T>
    typename QFileMappedVector <T>::QConstIterator QFileMappedVector <T>::cend () const
    {
        assert(isOpened());

        return QConstIterator(this, size());
    }

    template <class T>
    const T &QFileMappedVector <T>::operator[] (uint64_t uIndex) const
    {
        assert(isOpened());

        return vectorDataPtr()[uIndex];
    }

    template <class T>
    T &QFileMappedVector <T>::operator[] (uint64_t uIndex)
    {
        assert(isOpened());

        return vectorDataPtr()[uIndex];
    }

    template <class T>
    const T &QFileMappedVector <T>::at (uint64_t uIndex) const
    {
        assert(isOpened());

        if (uIndex >= size()) {
            throw std::out_of_range("QFileMappedVector::at " + std::to_string(uIndex));
        }

        return vectorDataPtr()[uIndex];
    }

    template <class T>
    T &QFileMappedVector <T>::at (uint64_t uIndex)
    {
        assert(isOpened());

        if (uIndex >= size()) {
            throw std::out_of_range("QFileMappedVector::at " + std::to_string(uIndex));
        }

        return vectorDataPtr()[uIndex];
    }

    template <class T>
    const T &QFileMappedVector <T>::front () const
    {
        assert(isOpened());

        return vectorDataPtr()[0];
    }

    template <class T>
    T &QFileMappedVector <T>::front ()
    {
        assert(isOpened());

        return vectorDataPtr()[0];
    }

    template <class T>
    const T &QFileMappedVector <T>::back () const
    {
        assert(isOpened());

        return vectorDataPtr()[size() - 1];
    }

    template <class T>
    T &QFileMappedVector <T>::back ()
    {
        assert(isOpened());

        return vectorDataPtr()[size() - 1];
    }

    template <class T>
    const T *QFileMappedVector <T>::data () const
    {
        assert(isOpened());

        return vectorDataPtr();
    }

    template <class T>
    T *QFileMappedVector <T>::data ()
    {
        assert(isOpened());

        return vectorDataPtr();
    }

    template <class T>
    void QFileMappedVector <T>::clear ()
    {
        assert(isOpened());

        *sizePtr() = 0;
        flushSize();
    }

    template <class T>
    typename QFileMappedVector <T>::QIterator
    QFileMappedVector <T>::erase (QConstIterator sPosition)
    {
        assert(isOpened());

        return erase(sPosition, std::next(sPosition));
    }

    template <class T>
    typename QFileMappedVector <T>::QIterator QFileMappedVector <T>::erase (QConstIterator sFirst,
                                                                            QConstIterator sLast)
    {
        assert(isOpened());

        uint64_t uNewSize = size() - std::distance(sFirst, sLast);

        atomicUpdate(uNewSize,
                     capacity(),
                     prefixSize(),
                     suffixSize(),
                     [this, sFirst, sLast] (sValueType *target)
                     {
                         std::copy(cbegin(), sFirst, target);
                         std::copy(sLast, cend(), target + std::distance(cbegin(), sFirst));
                     });

        return QIterator(this, sFirst.index());
    }

    template <class T>
    typename QFileMappedVector <T>::QIterator
    QFileMappedVector <T>::insert (QConstIterator sPosition,
                                   const T &sValue)
    {
        assert(isOpened());

        return insert(sPosition, &sValue, &sValue + 1);
    }

    template <class T>
    template <class InputIterator>
    typename QFileMappedVector <T>::QIterator
    QFileMappedVector <T>::insert (QConstIterator sPosition,
                                   InputIterator sFirst,
                                   InputIterator sLast)
    {
        assert(isOpened());

        uint64_t uNewSize = size() + static_cast<uint64_t>(std::distance(sFirst, sLast));
        uint64_t uNewCapacity;
        if (uNewSize > capacity()) {
            uNewCapacity = nextCapacity();
            if (uNewSize > uNewCapacity) {
                uNewCapacity = uNewSize;
            }
        } else {
            uNewCapacity = capacity();
        }

        atomicUpdate(uNewSize,
                     uNewCapacity,
                     prefixSize(),
                     suffixSize(),
                     [this, sPosition, sFirst, sLast] (sValueType *target)
                     {
                         std::copy(cbegin(), sPosition, target);
                         std::copy(sFirst, sLast, target + sPosition.index());
                         std::copy(sPosition, cend(),
                                   target + sPosition.index() + std::distance(sFirst, sLast));
                     });

        return QIterator(this, sPosition.index());
    }

    template <class T>
    void QFileMappedVector <T>::pop_back ()
    {
        assert(isOpened());

        --(*sizePtr());
        flushSize();
    }

    template <class T>
    void QFileMappedVector <T>::push_back (const T &sValue)
    {
        assert(isOpened());

        if (capacity() == size()) {
            reserve(nextCapacity());
        }

        vectorDataPtr()[size()] = sValue;
        flushElement(size());

        ++(*sizePtr());
        flushSize();
    }

    template <class T>
    void QFileMappedVector <T>::swap (QFileMappedVector &sOther)
    {
        mPath.swap(sOther.mPath);
        mFile.swap(sOther.mFile);
        std::swap(mPrefixSize, sOther.mPrefixSize);
        std::swap(mSuffixSize, sOther.mSuffixSize);
    }

    template <class T>
    bool QFileMappedVector <T>::getAutoFlush () const
    {
        return mAutoFlush;
    }

    template <class T>
    void QFileMappedVector <T>::setAutoFlush (bool bAutoFlush)
    {
        mAutoFlush = bAutoFlush;
    }

    template <class T>
    void QFileMappedVector <T>::flush ()
    {
        assert(isOpened());

        mFile.flush(mFile.data(), mFile.size());
    }

    template <class T>
    const uint8_t *QFileMappedVector <T>::prefix () const
    {
        assert(isOpened());

        return prefixPtr();
    }

    template <class T>
    uint8_t *QFileMappedVector <T>::prefix ()
    {
        assert(isOpened());

        return prefixPtr();
    }

    template <class T>
    uint64_t QFileMappedVector <T>::prefixSize () const
    {
        assert(isOpened());

        return mPrefixSize;
    }

    template <class T>
    void QFileMappedVector <T>::resizePrefix (uint64_t uNewPrefixSize)
    {
        assert(isOpened());

        if (prefixSize() != uNewPrefixSize) {
            atomicUpdate(size(),
                         capacity(),
                         uNewPrefixSize,
                         suffixSize(),
                         [this] (sValueType *target)
                         {
                             std::copy(cbegin(), cend(), target);
                         });
        }
    }

    template <class T>
    const uint8_t *QFileMappedVector <T>::suffix () const
    {
        assert(isOpened());

        return suffixPtr();
    }

    template <class T>
    uint8_t *QFileMappedVector <T>::suffix ()
    {
        assert(isOpened());

        return suffixPtr();
    }

    template <class T>
    uint64_t QFileMappedVector <T>::suffixSize () const
    {
        assert(isOpened());

        return mSuffixSize;
    }

    template <class T>
    void QFileMappedVector <T>::resizeSuffix (uint64_t uNewSuffixSize)
    {
        assert(isOpened());

        if (suffixSize() != uNewSuffixSize) {
            atomicUpdate(size(),
                         capacity(),
                         prefixSize(),
                         uNewSuffixSize,
                         [this] (sValueType *target)
                         {
                             std::copy(cbegin(), cend(), target);
                         });
        }
    }

    template <class T>
    void QFileMappedVector <T>::rename (const std::string &cNewPath,
                                        std::error_code &sEc)
    {
        mFile.rename(cNewPath, sEc);
        if (!sEc) {
            mPath = cNewPath;
        }
    }

    template <class T>
    void QFileMappedVector <T>::rename (const std::string &cNewPath)
    {
        mFile.rename(cNewPath);
        mPath = cNewPath;
    }

    template <class T>
    template <class F>
    void QFileMappedVector <T>::atomicUpdate (F &&sFunc)
    {
        atomicUpdate0(capacity(),
                      prefixSize(),
                      suffixSize(),
                      std::forward <F>(sFunc));
    }

    template <class T>
    template <class F>
    void QFileMappedVector <T>::atomicUpdate (uint64_t uNewSize,
                                              uint64_t uNewCapacity,
                                              uint64_t uNewPrefixSize,
                                              uint64_t uNewSuffixSize,
                                              F &&sFunc)
    {
        assert(uNewSize <= uNewCapacity);

        atomicUpdate0(
            uNewCapacity,
            uNewPrefixSize,
            uNewSuffixSize,
            [this, uNewSize, &sFunc] (QFileMappedVector <T> &newVector)
            {
                if (prefixSize() != 0 && newVector.prefixSize() != 0) {
                    std::copy(prefixPtr(),
                              prefixPtr() + std::min(prefixSize(),
                                                     newVector.prefixSize()),
                              newVector.prefix());
                }

                *newVector.sizePtr() = uNewSize;
                sFunc(newVector.data());

                if (suffixSize() != 0 && newVector.suffixSize() != 0) {
                    std::copy(suffixPtr(),
                              suffixPtr() + std::min(suffixSize(),
                                                     newVector.suffixSize()),
                              newVector.suffix());
                }
            }
        );
    }

    template <class T>
    template <class F>
    void QFileMappedVector <T>::atomicUpdate0 (uint64_t uNewCapacity,
                                               uint64_t uNewPrefixSize,
                                               uint64_t uNewSuffixSize,
                                               F &&sFunc)
    {
        if (mFile.path() != mPath) {
            throw std::runtime_error("Vector is mapped to a .bak file due to earlier errors");
        }

        boost::filesystem::path bakPath = mPath + ".bak";
        boost::filesystem::path tmpPath = boost::filesystem::unique_path(mPath + ".tmp.%%%%-%%%%");

        if (boost::filesystem::exists(bakPath)) {
            boost::filesystem::remove(bakPath);
        }

        Tools::QScopeExit tmpFileDeleter(
            [&tmpPath]
            {
                boost::system::error_code ignore;
                boost::filesystem::remove(tmpPath, ignore);
            }
        );

        // Copy file. It is slow but atomic operation
        QFileMappedVector <T> sTmpVector;
        sTmpVector.create(tmpPath.string(), uNewCapacity, uNewPrefixSize, uNewSuffixSize);
        sFunc(sTmpVector);
        sTmpVector.flush();

        // Swap files
        std::error_code sEc;
        std::error_code sIgnore;
        mFile.rename(bakPath.string());
        sTmpVector.rename(mPath, sEc);
        if (sEc) {
            // Try to restore and sIgnore errors
            mFile.rename(mPath, sIgnore);
            throw std::system_error(sEc, "Failed to swap temporary and vector files");
        }

        mPath = bakPath.string();
        swap(sTmpVector);
        tmpFileDeleter.cancel();

        // Remove .bak file and sIgnore errors
        sTmpVector.close(sIgnore);
        boost::system::error_code boostError;
        boost::filesystem::remove(bakPath, boostError);
    }

    template <class T>
    void QFileMappedVector <T>::open (const std::string &cPath, uint64_t uPrefixSize)
    {
        mPrefixSize = uPrefixSize;
        mFile.open(cPath);
        mPath = cPath;

        if (mFile.size() < uPrefixSize + uMetadataSize) {
            throw std::runtime_error("QFileMappedVector::open() file is too small");
        }

        if (size() > capacity()) {
            throw std::runtime_error(
                "QFileMappedVector::open() vector uSize is greater than capacity");
        }

        auto minRequiredFileSize = mPrefixSize + uMetadataSize + vectorDataSize();
        if (mFile.size() < minRequiredFileSize) {
            throw std::runtime_error("QFileMappedVector::open() uInvalid file uSize");
        }

        mSuffixSize = mFile.size() - minRequiredFileSize;
    }

    template <class T>
    void QFileMappedVector <T>::create (const std::string &cPath,
                                        uint64_t uInitialCapacity,
                                        uint64_t uPrefixSize,
                                        uint64_t uSuffixSize)
    {
        mFile.create(cPath,
                     uPrefixSize + uMetadataSize + uInitialCapacity * uValueSize + uSuffixSize,
                     false);
        mPath = cPath;
        mPrefixSize = uPrefixSize;
        mSuffixSize = uSuffixSize;
        *sizePtr() = 0;
        *capacityPtr() = uInitialCapacity;
        mFile.flush(reinterpret_cast<uint8_t *>(sizePtr()), uMetadataSize);
    }

    template <class T>
    uint8_t *QFileMappedVector <T>::prefixPtr ()
    {
        return mFile.data();
    }

    template <class T>
    const uint8_t *QFileMappedVector <T>::prefixPtr () const
    {
        return mFile.data();
    }

    template <class T>
    uint64_t *QFileMappedVector <T>::capacityPtr ()
    {
        return reinterpret_cast<uint64_t *>(prefixPtr() + mPrefixSize);
    }

    template <class T>
    const uint64_t *QFileMappedVector <T>::capacityPtr () const
    {
        return reinterpret_cast<const uint64_t *>(prefixPtr() + mPrefixSize);
    }

    template <class T>
    const uint64_t *QFileMappedVector <T>::sizePtr () const
    {
        return capacityPtr() + 1;
    }

    template <class T>
    uint64_t *QFileMappedVector <T>::sizePtr ()
    {
        return capacityPtr() + 1;
    }

    template <class T>
    T *QFileMappedVector <T>::vectorDataPtr ()
    {
        return reinterpret_cast<T *>(sizePtr() + 1);
    }

    template <class T>
    const T *QFileMappedVector <T>::vectorDataPtr () const
    {
        return reinterpret_cast<const T *>(sizePtr() + 1);
    }

    template <class T>
    uint8_t *QFileMappedVector <T>::suffixPtr ()
    {
        return reinterpret_cast<uint8_t *>(vectorDataPtr() + capacity());
    }

    template <class T>
    const uint8_t *QFileMappedVector <T>::suffixPtr () const
    {
        return reinterpret_cast<const uint8_t *>(vectorDataPtr() + capacity());
    }

    template <class T>
    uint64_t QFileMappedVector <T>::vectorDataSize ()
    {
        return capacity() * uValueSize;
    }

    template <class T>
    uint64_t QFileMappedVector <T>::nextCapacity ()
    {
        return capacity() + capacity() / 2 + 1;
    }

    template <class T>
    void QFileMappedVector <T>::flushElement (uint64_t uIndex)
    {
        if (mAutoFlush) {
            mFile.flush(reinterpret_cast<uint8_t *>(vectorDataPtr() + uIndex), uValueSize);
        }
    }

    template <class T>
    void QFileMappedVector <T>::flushSize ()
    {
        if (mAutoFlush) {
            mFile.flush(reinterpret_cast<uint8_t *>(sizePtr()), sizeof(uint64_t));
        }
    }

} // namespace Common
