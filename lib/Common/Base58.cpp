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

#include <cassert>
#include <string>
#include <vector>

#include <Common/Base58.h>
#include <Common/IntUtil.h>
#include <Common/Varint.h>

#include <Crypto/hash.h>

namespace Tools {

namespace Base58 {

namespace {

const char alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const size_t alphabetSize = sizeof(alphabet) - 1;
const size_t encodedBlockSizes[] = {0, 2, 3, 5, 6, 7, 9, 10, 11};
const size_t fullBlockSize = sizeof(encodedBlockSizes) / sizeof(encodedBlockSizes[0]) - 1;
const size_t fullEncodedBlockSize = encodedBlockSizes[fullBlockSize];
const size_t addrChecksumSize = 4;

struct ReverseAlphabet {
    ReverseAlphabet()
    {
        m_data.resize(alphabet[alphabetSize - 1] - alphabet[0] + 1, -1);

        for (size_t i = 0; i < alphabetSize; ++i) {
            size_t idx = static_cast<size_t>(alphabet[i] - alphabet[0]);
            m_data[idx] = static_cast<int8_t>(i);
        }
    }

    int operator()(char letter) const
    {
        size_t idx = static_cast<size_t>(letter - alphabet[0]);

        return idx < m_data.size() ? m_data[idx] : -1;
    }

    static ReverseAlphabet instance;

private:
    std::vector<int8_t> m_data;
};

ReverseAlphabet ReverseAlphabet::instance;

struct DecodedBlockSizes {
    DecodedBlockSizes()
    {
        m_data.resize(encodedBlockSizes[fullBlockSize] + 1, -1);
        for (size_t i = 0; i <= fullBlockSize; ++i) {
            m_data[encodedBlockSizes[i]] = static_cast<int>(i);
        }
    }

    int operator()(size_t encodedBlockSize) const
    {
        assert(encodedBlockSize <= fullEncodedBlockSize);

        return m_data[encodedBlockSize];
    }

    static DecodedBlockSizes instance;

private:
    std::vector<int> m_data;
};

DecodedBlockSizes DecodedBlockSizes::instance;

uint64_t uint8BeTo64(const uint8_t *data, size_t size)
{
    assert(1 <= size && size <= sizeof(uint64_t));

    uint64_t res = 0;
    switch (9 - size) {
    case 1:
        res |= *data++;
        /* FALLTHROUGH */
    case 2:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 3:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 4:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 5:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 6:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 7:
        res <<= 8;
        res |= *data++;
        /* FALLTHROUGH */
    case 8:
        res <<= 8;
        res |= *data;
        break;
    default:
        assert(false);
    }

    return res;
}

void uint64To8Be(uint64_t num, size_t size, uint8_t *data)
{
    assert(1 <= size && size <= sizeof(uint64_t));

    uint64_t numBe = SWAP64BE(num);

    memcpy(data, reinterpret_cast<uint8_t*>(&numBe) + sizeof(uint64_t) - size, size);
}

void encodeBlock(const char *block, size_t size, char *res)
{
    assert(1 <= size && size <= fullBlockSize);

    uint64_t num = uint8BeTo64(reinterpret_cast<const uint8_t *>(block), size);
    int i = static_cast<int>(encodedBlockSizes[size]) - 1;
    while (0 < num) {
        uint64_t remainder = num % alphabetSize;
        num /= alphabetSize;
        res[i] = alphabet[remainder];
        --i;
    }
}

bool decodeBlock(const char *block, size_t size, char *res)
{
    assert(1 <= size && size <= fullEncodedBlockSize);

    int res_size = DecodedBlockSizes::instance(size);
    if (res_size <= 0) {
        return false; // invalid block size
    }

    uint64_t resNum = 0;
    uint64_t order = 1;
    for (size_t i = size - 1; i < size; --i) {
        int digit = ReverseAlphabet::instance(block[i]);
        if (digit < 0) {
            return false; // invalid symbol
        }

        uint64_t product_hi;
        uint64_t tmp = resNum + mul128(order, digit, &product_hi);
        if (tmp < resNum || 0 != product_hi) {
            return false; // overflow
        }

        resNum = tmp;
        order *= alphabetSize; // never overflows, 58^10 < 2^64
    }

    if (static_cast<size_t>(res_size) < fullBlockSize
        && (UINT64_C(1) << (8 * res_size)) <= resNum) {
        return false; // overflow
    }

    uint64To8Be(resNum, res_size, reinterpret_cast<uint8_t *>(res));

    return true;
}

} // namespace

std::string encode(const std::string &data)
{
    if (data.empty()) {
        return std::string();
    }

    size_t fullBlockCount = data.size() / fullBlockSize;
    size_t lastBlockSize = data.size() % fullBlockSize;
    size_t resSize = fullBlockCount * fullEncodedBlockSize + encodedBlockSizes[lastBlockSize];

    std::string res(resSize, alphabet[0]);
    for (size_t i = 0; i < fullBlockCount; ++i) {
        encodeBlock(data.data() + i * fullBlockSize, fullBlockSize, &res[i * fullEncodedBlockSize]);
    }

    if (0 < lastBlockSize) {
        encodeBlock(data.data() + fullBlockCount * fullBlockSize, lastBlockSize,
                    &res[fullBlockCount * fullEncodedBlockSize]);
    }

    return res;
}

bool decode(const std::string &enc, std::string &data)
{
    if (enc.empty()) {
        data.clear();
        return true;
    }

    size_t fullBlockCount = enc.size() / fullEncodedBlockSize;
    size_t lastBlockSize = enc.size() % fullEncodedBlockSize;
    int lastBlockDecodedSize = DecodedBlockSizes::instance(lastBlockSize);
    if (lastBlockDecodedSize < 0) {
        return false; // invalid enc length
    }
    size_t dataSize = fullBlockCount * fullBlockSize + lastBlockDecodedSize;

    data.resize(dataSize, 0);
    for (size_t i = 0; i < fullBlockCount; ++i) {
        if (!decodeBlock(enc.data() + i * fullEncodedBlockSize, fullEncodedBlockSize,
                         &data[i * fullBlockSize])) {
            return false;
        }
    }

    if (0 < lastBlockSize) {
        if (!decodeBlock(enc.data() + fullBlockCount * fullEncodedBlockSize, lastBlockSize,
                         &data[fullBlockCount * fullBlockSize])) {
            return false;
        }
    }

    return true;
}

std::string encodeAddr(uint64_t tag, const std::string &data)
{
    std::string buf = getVarintData(tag);
    buf += data;

    Crypto::Hash hash = Crypto::cn_fast_hash(buf.data(), buf.size());

    const char *hashData = reinterpret_cast<const char*>(&hash);
    buf.append(hashData, addrChecksumSize);

    return encode(buf);
}

bool decodeAddr(std::string addr, uint64_t &tag, std::string &data)
{
    std::string addrData;
    bool r = decode(addr, addrData);
    if (!r) {
        return false;
    }
    if (addrData.size() <= addrChecksumSize) {
        return false;
    }

    std::string checksum(addrChecksumSize, '\0');
    checksum = addrData.substr(addrData.size() - addrChecksumSize);

    addrData.resize(addrData.size() - addrChecksumSize);
    Crypto::Hash hash = Crypto::cn_fast_hash(addrData.data(), addrData.size());
    std::string expectedChecksum(reinterpret_cast<const char *>(&hash), addrChecksumSize);
    if (expectedChecksum != checksum) {
        return false;
    }

    int read = Tools::readVarint(addrData.begin(), addrData.end(), tag);
    if (read <= 0) {
        return false;
    }

    data = addrData.substr(read);

    return true;
}

} // namespace Base58

} // namespace Tools
