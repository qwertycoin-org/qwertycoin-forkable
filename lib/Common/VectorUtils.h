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
#include <iterator>
#include <vector>

namespace Common {

    namespace VectorUtils {

        template <typename T, class QUnaryPredicate>
        std::vector <T> filter (const std::vector <T> &vOriginal, QUnaryPredicate sPredicate)
        {
            std::vector <T> vFiltered;

            std::copy_if(begin(vOriginal), end(vOriginal), std::back_inserter(vFiltered),
                         sPredicate);

            return vFiltered;
        }

        template <typename T2, typename T1, class QUnaryOperation>
        std::vector <T2> map (const std::vector <T1> &vOriginal, QUnaryOperation sMappingFunction)
        {
            std::vector <T2> vMapped;

            std::transform(begin(vOriginal),
                           end(vOriginal),
                           std::back_inserter(vMapped),
                           sMappingFunction);

            return vMapped;
        }

        template <typename T>
        void append (std::vector <T> &vAppendedTo, const std::vector <T> &vAppended)
        {
            vAppendedTo.insert(end(vAppendedTo),
                               begin(vAppended),
                               end(vAppended));
        }

    } // namespace VectorUtils

} // namespace Common
