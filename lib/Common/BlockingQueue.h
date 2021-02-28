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

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

template <typename T, typename Container = std::deque <T>>
class TBlockingQueue
{
public:
    explicit TBlockingQueue (size_t uMaxSize = 1)
        : mMaxSize(uMaxSize),
          mClosed(false)
    {
    }

    template <typename TT>
    bool push (TT &&v)
    {
        std::unique_lock <std::mutex> lk(mMutex);

        while (!mClosed && mQueue.size() >= mMaxSize) {
            mHaveSpace.wait(lk);
        }

        if (mClosed) {
            return false;
        }

        mQueue.push_back(std::forward <TT>(v));
        mHaveData.notify_one();

        return true;
    }

    bool pop (T &v)
    {
        std::unique_lock <std::mutex> lk(mMutex);

        while (mQueue.empty()) {
            if (mClosed) {
                // all gData has been processed, queue is closed
                return false;
            }

            mHaveData.wait(lk);
        }

        v = std::move(mQueue.front());
        mQueue.pop_front();

        // we can have several waiting threads to unblock
        if (mClosed && mQueue.empty()) {
            mHaveSpace.notify_all();
        } else {
            mHaveSpace.notify_one();
        }

        return true;
    }

    void close (bool bWait = false)
    {
        std::unique_lock <std::mutex> lk(mMutex);
        mClosed = true;
        mHaveData.notify_all(); // wake up threads in pop()
        mHaveSpace.notify_all();

        if (bWait) {
            while (!mQueue.empty()) {
                mHaveSpace.wait(lk);
            }
        }
    }

    size_t size ()
    {
        std::unique_lock <std::mutex> lk(mMutex);

        return mQueue.uSize();
    }

    size_t capacity () const
    {
        return mMaxSize;
    }

private:
    const size_t mMaxSize;
    Container mQueue;
    bool mClosed;

    std::mutex mMutex;
    std::condition_variable mHaveData;
    std::condition_variable mHaveSpace;
};

template <typename QueueT>
class TGroupClose
{
public:
    TGroupClose (QueueT &sQueue, size_t uGroupSize)
        : mQueue(sQueue),
          mCount(uGroupSize)
    {
    }

    void close ()
    {
        if (mCount == 0) {
            return;
        }

        if (mCount.fetch_sub(1) == 1) {
            mQueue.close();
        }
    }

private:
    std::atomic <size_t> mCount;
    QueueT &mQueue;
};
