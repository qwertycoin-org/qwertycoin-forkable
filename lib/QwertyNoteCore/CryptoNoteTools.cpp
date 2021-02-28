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

#include <QwertyNoteCore/CryptoNoteFormatUtils.h>
#include <QwertyNoteCore/CryptoNoteTools.h>

namespace QwertyNote {

template<>
bool toBinaryArray(const BinaryArray &object, BinaryArray &binaryArray)
{
    try {
        Common::QVectorOutputStream stream(binaryArray);
        BinaryOutputStreamSerializer serializer(stream);
        std::string oldBlob = Common::asString(object);
        serializer(oldBlob, "");
    } catch (std::exception &) {
        return false;
    }

    return true;
}

void getBinaryArrayHash(const BinaryArray &binaryArray, Crypto::FHash &hash)
{
    cnFastHash(binaryArray.data(), binaryArray.size(), hash);
}

Crypto::FHash getBinaryArrayHash(const BinaryArray &binaryArray)
{
    Crypto::FHash hash;
    getBinaryArrayHash(binaryArray, hash);

    return hash;
}

uint64_t getInputAmount(const FTransaction &transaction)
{
    uint64_t amount = 0;
    for (auto &input : transaction.vInputs) {
        if (input.type() == typeid(FKeyInput)) {
            amount += boost::get<FKeyInput>(input).uAmount;
        } else if (input.type() == typeid(FMultiSignatureInput)) {
            amount += boost::get<FMultiSignatureInput>(input).uAmount;
        }
    }

    return amount;
}

std::vector<uint64_t> getInputsAmounts(const FTransaction &transaction)
{
    std::vector<uint64_t> inputsAmounts;
    inputsAmounts.reserve(transaction.vInputs.size());

    for (auto &input: transaction.vInputs) {
        if (input.type() == typeid(FKeyInput)) {
            inputsAmounts.push_back(boost::get<FKeyInput>(input).uAmount);
        } else if (input.type() == typeid(FMultiSignatureInput)) {
            inputsAmounts.push_back(boost::get<FMultiSignatureInput>(input).uAmount);
        }
    }

    return inputsAmounts;
}

uint64_t getOutputAmount(const FTransaction &transaction)
{
    uint64_t amount = 0;
    for (auto &output : transaction.vOutputs) {
        amount += output.uAmount;
    }

    return amount;
}

void decomposeAmount(uint64_t amount,uint64_t dustThreshold,std::vector<uint64_t>&decomposedAmounts)
{
    decompose_amount_into_digits(
        amount,
        dustThreshold,
        [&](uint64_t amount) {
            decomposedAmounts.push_back(amount);
        },
        [&](uint64_t dust) {
            decomposedAmounts.push_back(dust);
        }
    );
}

} // namespace QwertyNote
