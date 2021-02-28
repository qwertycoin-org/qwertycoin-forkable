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

#include <Crypto/Hash.h>

namespace Tools {

    namespace Base58 {

        namespace {

            const char cAlphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
            const size_t uAlphabetSize = sizeof(cAlphabet) - 1;
            const size_t uEncodedBlockSizes[] = {0, 2, 3, 5, 6, 7, 9, 10, 11};
            const size_t uFullBlockSize =
                sizeof(uEncodedBlockSizes) / sizeof(uEncodedBlockSizes[0]) - 1;
            const size_t uFullEncodedBlockSize = uEncodedBlockSizes[uFullBlockSize];
            const size_t uAddressChecksumSize = 4;

            struct FReverseAlphabet
            {
                FReverseAlphabet ()
                {
                    mData.resize(cAlphabet[uAlphabetSize - 1] - cAlphabet[0] + 1, -1);

                    for (size_t i = 0; i < uAlphabetSize; ++i) {
                        size_t idx = static_cast<size_t>(cAlphabet[i] - cAlphabet[0]);
                        mData[idx] = static_cast<int8_t>(i);
                    }
                }

                int operator() (char cLetter) const
                {
                    size_t uIndex = static_cast<size_t>(cLetter - cAlphabet[0]);

                    return uIndex < mData.size() ? mData[uIndex] : -1;
                }

                static FReverseAlphabet pInstance;

            private:
                std::vector <int8_t> mData;
            };

            FReverseAlphabet FReverseAlphabet::pInstance;

            struct FDecodedBlockSizes
            {
                FDecodedBlockSizes ()
                {
                    mData.resize(uEncodedBlockSizes[uFullBlockSize] + 1, -1);

                    for (size_t i = 0; i <= uFullBlockSize; ++i) {
                        mData[uEncodedBlockSizes[i]] = static_cast<int>(i);
                    }
                }

                int operator() (size_t uEncodedBlockSize) const
                {
                    assert(uEncodedBlockSize <= uFullEncodedBlockSize);

                    return mData[uEncodedBlockSize];
                }

                static FDecodedBlockSizes pInstance;

            private:
                std::vector <int> mData;
            };

            FDecodedBlockSizes FDecodedBlockSizes::pInstance;

            uint64_t uint8BeTo64 (const uint8_t *uData, size_t uSize)
            {
                assert(1 <= uSize && uSize <= sizeof(uint64_t));

                uint64_t uResult = 0;
                switch (9 - uSize) {
                    case 1:
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 2:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 3:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 4:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 5:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 6:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 7:
                        uResult <<= 8;
                        uResult |= *uData++;
                        /* FALLTHROUGH */
                    case 8:
                        uResult <<= 8;
                        uResult |= *uData;
                        break;
                    default:
                        assert(false);
                }

                return uResult;
            }

            void uint64To8Be (uint64_t uNum, size_t uSize, uint8_t *uData)
            {
                assert(1 <= uSize && uSize <= sizeof(uint64_t));

                uint64_t uNumBe = SWAP64BE(uNum);

                memcpy(uData,
                       reinterpret_cast<uint8_t *>(&uNumBe) + sizeof(uint64_t) - uSize,
                       uSize);
            }

            void encodeBlock (const char *cBlock, size_t uSize, char *cResult)
            {
                assert(1 <= uSize && uSize <= uFullBlockSize);

                uint64_t uNumber = uint8BeTo64(reinterpret_cast<const uint8_t *>(cBlock), uSize);
                int i = static_cast<int>(uEncodedBlockSizes[uSize]) - 1;

                while (0 < uNumber) {
                    uint64_t uRemainder = uNumber % uAlphabetSize;
                    uNumber /= uAlphabetSize;
                    cResult[i] = cAlphabet[uRemainder];
                    --i;
                }
            }

            bool decodeBlock (const char *cBlock, size_t uSize, char *cResult)
            {
                assert(1 <= uSize && uSize <= uFullEncodedBlockSize);

                int iResultSize = FDecodedBlockSizes::pInstance(uSize);
                if (iResultSize <= 0) {
                    return false; // uInvalid cBlock uSize
                }

                uint64_t uResNum = 0;
                uint64_t uOrder = 1;
                for (size_t i = uSize - 1; i < uSize; --i) {
                    int iDigit = FReverseAlphabet::pInstance(cBlock[i]);
                    if (iDigit < 0) {
                        return false; // uInvalid symbol
                    }

                    uint64_t uProductHi;
                    uint64_t uTmp = uResNum + mul128(uOrder, iDigit, &uProductHi);
                    if (uTmp < uResNum || 0 != uProductHi) {
                        return false; // overflow
                    }

                    uResNum = uTmp;
                    uOrder *= uAlphabetSize; // never overflows, 58^10 < 2^64
                }

                if (static_cast<size_t>(iResultSize) < uFullBlockSize
                    && (UINT64_C(1) << (8 * iResultSize)) <= uResNum) {
                    return false; // overflow
                }

                uint64To8Be(uResNum, iResultSize, reinterpret_cast<uint8_t *>(cResult));

                return true;
            }

        } // namespace

        std::string encode (const std::string &cData)
        {
            if (cData.empty()) {
                return std::string();
            }

            size_t uFullBlockCount = cData.size() / uFullBlockSize;
            size_t uLastBlockSize = cData.size() % uFullBlockSize;
            size_t uResultSize = uFullBlockCount *
                                 uFullEncodedBlockSize + uEncodedBlockSizes[uLastBlockSize];

            std::string cResult(uResultSize, cAlphabet[0]);
            for (size_t i = 0; i < uFullBlockCount; ++i) {
                encodeBlock(cData.data() + i *
                                           uFullBlockSize,
                            uFullBlockSize,
                            &cResult[i * uFullEncodedBlockSize]);
            }

            if (0 < uLastBlockSize) {
                encodeBlock(cData.data() + uFullBlockCount *
                                           uFullBlockSize,
                            uLastBlockSize,
                            &cResult[uFullBlockCount * uFullEncodedBlockSize]);
            }

            return cResult;
        }

        bool decode (const std::string &cEncodedString, std::string &cData)
        {
            if (cEncodedString.empty()) {
                cData.clear();
                return true;
            }

            size_t uFullBlockCount = cEncodedString.size() / uFullEncodedBlockSize;
            size_t uLastBlockSize = cEncodedString.size() % uFullEncodedBlockSize;
            int iLastBlockDecodedSize = FDecodedBlockSizes::pInstance(uLastBlockSize);
            if (iLastBlockDecodedSize < 0) {
                return false; // uInvalid cEncodedString length
            }
            size_t uDataSize = uFullBlockCount * uFullBlockSize + iLastBlockDecodedSize;

            cData.resize(uDataSize, 0);
            for (size_t i = 0; i < uFullBlockCount; ++i) {
                if (!decodeBlock(cEncodedString.data() + i * uFullEncodedBlockSize,
                                 uFullEncodedBlockSize,
                                 &cData[i * uFullBlockSize])) {
                    return false;
                }
            }

            if (0 < uLastBlockSize) {
                if (!decodeBlock(cEncodedString.data() + uFullBlockCount * uFullEncodedBlockSize,
                                 uLastBlockSize,
                                 &cData[uFullBlockCount * uFullBlockSize])) {
                    return false;
                }
            }

            return true;
        }

        std::string encodeAddress (uint64_t uExtraTag, const std::string &cData)
        {
            std::string cBuffer = getVarintData(uExtraTag);
            cBuffer += cData;

            Crypto::FHash sHash = Crypto::cnFastHash(cBuffer.data(), cBuffer.size());

            const char *cHashData = reinterpret_cast<const char *>(&sHash);
            cBuffer.append(cHashData, uAddressChecksumSize);

            return encode(cBuffer);
        }

        bool decodeAddress (std::string cAddress, uint64_t &uExtraTag, std::string &sData)
        {
            std::string cAddrData;
            bool r = decode(cAddress, cAddrData);
            if (!r) {
                return false;
            }
            if (cAddrData.size() <= uAddressChecksumSize) {
                return false;
            }

            std::string cChecksum(uAddressChecksumSize, '\0');
            cChecksum = cAddrData.substr(cAddrData.size() - uAddressChecksumSize);

            cAddrData.resize(cAddrData.size() - uAddressChecksumSize);
            Crypto::FHash sHash = Crypto::cnFastHash(cAddrData.data(), cAddrData.size());
            std::string cExpectedChecksum(reinterpret_cast<const char *>(&sHash),
                                          uAddressChecksumSize);
            if (cExpectedChecksum != cChecksum) {
                return false;
            }

            int iRead = Tools::readVarint(cAddrData.begin(), cAddrData.end(), uExtraTag);
            if (iRead <= 0) {
                return false;
            }

            sData = cAddrData.substr(iRead);

            return true;
        }

    } // namespace Base58

} // namespace Tools
