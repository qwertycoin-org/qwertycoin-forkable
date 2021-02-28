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

#include <algorithm>
#include <mutex>
#include <vector>

namespace Tools {

    template <typename T>
    class QObserverManager
    {
    public:
        bool add (T *sObserver)
        {
            std::unique_lock <std::mutex> lock(mObserversMutex);
            auto it = std::find(mObservers.begin(), mObservers.end(), sObserver);
            if (mObservers.end() == it) {
                mObservers.push_back(sObserver);

                return true;
            } else {
                return false;
            }
        }

        bool remove (T *sObserver)
        {
            std::unique_lock <std::mutex> lock(mObserversMutex);

            auto it = std::find(mObservers.begin(), mObservers.end(), sObserver);
            if (mObservers.end() == it) {
                return false;
            } else {
                mObservers.erase(it);

                return true;
            }
        }

        void clear ()
        {
            std::unique_lock <std::mutex> lock(mObserversMutex);
            mObservers.clear();
        }

#if defined(_MSC_VER)
        template<typename F>
        void notify(F notification)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)();
            }
        }

        template<typename F, typename Arg0>
        void notify(F notification, const Arg0 &arg0) {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0);
            }
        }

        template<typename F, typename Arg0, typename Arg1>
        void notify(F notification, const Arg0 &arg0, const Arg1 &arg1)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0, arg1);
            }
        }

        template<typename F, typename Arg0, typename Arg1, typename Arg2>
        void notify(F notification, const Arg0 &arg0, const Arg1 &arg1, const Arg2 &arg2)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0, arg1, arg2);
            }
        }

        template<typename F, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        void notify(F notification,
                    const Arg0 &arg0,
                    const Arg1 &arg1,
                    const Arg2 &arg2,
                    const Arg3 &arg3)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0, arg1, arg2, arg3);
            }
        }

        template<typename F, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        void notify(F notification,
                    const Arg0 &arg0,
                    const Arg1 &arg1,
                    const Arg2 &arg2,
                    const Arg3 &arg3,
                    const Arg4 &arg4)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0, arg1, arg2, arg3, arg4);
            }
        }

        template<typename F,
                 typename Arg0,
                 typename Arg1,
                 typename Arg2,
                 typename Arg3,
                 typename Arg4,
                 typename Arg5>
        void notify(F notification,
                    const Arg0 &arg0,
                    const Arg1 &arg1,
                    const Arg2 &arg2,
                    const Arg3 &arg3,
                    const Arg4 &arg4,
                    const Arg5 &arg5)
        {
            std::vector<T *> vObserversCopy;

            {
                std::unique_lock<std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *sObserver : vObserversCopy) {
                (sObserver->*notification)(arg0, arg1, arg2, arg3, arg4, arg5);
            }
        }
#else

        template <typename F, typename... Args>
        void notify (F notification, Args... sArgs)
        {
            std::vector <T *> vObserversCopy;

            {
                std::unique_lock <std::mutex> lock(mObserversMutex);
                vObserversCopy = mObservers;
            }

            for (T *observer : vObserversCopy) {
                (observer->*notification)(sArgs...);
            }
        }

#endif

    private:
        std::vector <T *> mObservers;
        std::mutex mObserversMutex;
    };

} // namespace Tools
