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

#include <boost/utility/value_init.hpp>

#include <QwertyNoteCore/BlockIndex.h>
#include <QwertyNoteCore/CryptoNoteSerialization.h>

#include <Serialization/SerializationOverloads.h>

namespace QwertyNote {

Crypto::FHash BlockIndex::getBlockId(uint32_t height) const
{
    assert(height < m_container.size());
    return m_container[static_cast<size_t>(height)];
}

std::vector<Crypto::FHash> BlockIndex::getBlockIds(uint32_t startBlockIndex, uint32_t maxCount) const
{
    std::vector<Crypto::FHash> result;
    if (startBlockIndex >= m_container.size()) {
        return result;
    }

    size_t count = std::min(
        static_cast<size_t>(maxCount),
        m_container.size() - static_cast<size_t>(startBlockIndex)
    );
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(m_container[startBlockIndex + i]);
    }

    return result;
}

bool BlockIndex::findSupplement(const std::vector<Crypto::FHash> &ids, uint32_t &offset) const
{
    for (const auto &id : ids) {
        if (getBlockHeight(id, offset)) {
            return true;
        }
    }

    return false;
}

std::vector<Crypto::FHash> BlockIndex::buildSparseChain(const Crypto::FHash &startBlockId) const
{
    assert(m_index.count(startBlockId) > 0);

    uint32_t startBlockHeight;
    getBlockHeight(startBlockId, startBlockHeight);

    std::vector<Crypto::FHash> result;
    auto sparseChainEnd = static_cast<size_t>(startBlockHeight + 1);
    for (size_t i = 1; i <= sparseChainEnd; i *= 2) {
        result.emplace_back(m_container[sparseChainEnd - i]);
    }

    if (result.back() != m_container[0]) {
        result.emplace_back(m_container[0]);
    }

    return result;
}

Crypto::FHash BlockIndex::getTailId() const
{
    assert(!m_container.empty());
    return m_container.back();
}

void BlockIndex::serialize(ISerializer &s)
{
    if (s.type() == ISerializer::INPUT) {
        readSequence<Crypto::FHash>(std::back_inserter(m_container), "index", s);
    } else {
        writeSequence<Crypto::FHash>(m_container.begin(), m_container.end(), "index", s);
    }
}

} // namespace QwertyNote