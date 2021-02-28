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

#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <Crypto/Hash.h>

namespace QwertyNote {

class ISerializer;

class BlockIndex
{
    typedef boost::multi_index_container <
        Crypto::FHash,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<>,
            boost::multi_index::hashed_unique<boost::multi_index::identity<Crypto::FHash>>
        >
    > ContainerT;

public:
    BlockIndex()
        : m_index(m_container.get<1>())
    {
    }

    void pop()
    {
        m_container.pop_back();
    }

    // returns true if new element was inserted, false if already exists
    bool push(const Crypto::FHash &h)
    {
        auto result = m_container.push_back(h);
        return result.second;
    }

    bool hasBlock(const Crypto::FHash &h) const
    {
        return m_index.find(h) != m_index.end();
    }

    bool getBlockHeight(const Crypto::FHash &h, uint32_t &height) const
    {
        auto hi = m_index.find(h);
        if (hi == m_index.end()) {
            return false;
        }

        height = static_cast<uint32_t>(std::distance(m_container.begin(), m_container.project<0>(hi)));

        return true;
    }

    uint32_t size() const
    {
        return static_cast<uint32_t>(m_container.size());
    }

    void clear()
    {
        m_container.clear();
    }

    Crypto::FHash getBlockId(uint32_t height) const;
    std::vector<Crypto::FHash> getBlockIds(uint32_t startBlockIndex, uint32_t maxCount) const;
    bool findSupplement(const std::vector<Crypto::FHash> &ids, uint32_t &offset) const;
    std::vector<Crypto::FHash> buildSparseChain(const Crypto::FHash &startBlockId) const;
    Crypto::FHash getTailId() const;

    void serialize(ISerializer &s);

private:
    ContainerT m_container;
    ContainerT::nth_index<1>::type &m_index;
};

} // namespace QwertyNote