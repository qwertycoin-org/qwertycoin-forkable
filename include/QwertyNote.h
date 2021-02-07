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

#include <vector>

#include <boost/variant.hpp>

#include <Android.h>
#include <CryptoTypes.h>

namespace QwertyNote {

	struct FBaseInput
	{
		uint32_t uBlockIndex;
	};

	struct FKeyInput
	{
		uint64_t uAmount;
		std::vector <uint32_t> vOutputIndexes;
		Crypto::FKeyImage sKeyImage;
	};

	struct FMultiSignatureInput
	{
		uint8_t uSignatureCount;
		uint32_t uOutputIndex;
		uint64_t uAmount;
	};

	struct FKeyOutput
	{
		Crypto::FPublicKey sPublicKey;
	};

	struct FMultiSignatureOutput
	{
		uint8_t uRequiredSignatureCount;
		std::vector <Crypto::FPublicKey> vPublicKeys;
	};

	typedef boost::variant <FBaseInput, FKeyInput, FMultiSignatureInput> FTransactionInput;

	typedef boost::variant <FKeyOutput, FMultiSignatureOutput> FTransactionOutputTarget;

	struct FTransactionOutput
	{
		uint64_t uAmount;
		FTransactionOutputTarget sTarget;
	};

	struct FTransactionPrefix
	{
		uint8_t uVersion;
		uint64_t uUnlockTime;
		std::vector <uint8_t> vExtra;
		std::vector <FTransactionInput> vInputs;
		std::vector <FTransactionOutput> vOutputs;
	};

	struct FTransaction : public FTransactionPrefix
	{
		std::vector <std::vector <Crypto::FSignature>> vSignatures;
	};

	struct FParentBlock
	{
		uint8_t uMajorVersion;
		uint8_t uMinorVersion;
		uint16_t uTransactionCount;
		std::vector <Crypto::FHash> vBaseTransactionBranch;
		std::vector <Crypto::FHash> vBlockchainBranch;
		Crypto::FHash sPreviousBlockHash;
		FTransaction sBaseTransaction;
	};

	struct FBlockHeader
	{
		uint8_t uMajorVersion;
		uint8_t uMinorVersion;
		uint32_t uNonce;
		uint64_t uTimestamp;
		Crypto::FHash sPreviousBlockHash;
	};

	struct FBlock : public FBlockHeader
	{
		std::vector <Crypto::FHash> vTransactionHashes;
		FParentBlock sParentBlock;
		FTransaction sBaseTransaction;
	};

	struct FAccountPublicAddress
	{
		Crypto::FPublicKey sSpendPublicKey;
		Crypto::FPublicKey sViewPublicKey;
	};

	struct FAccountKeys
	{
		FAccountPublicAddress sAddress;
		Crypto::FSecretKey sSpendSecretKey;
		Crypto::FSecretKey sViewSecretKey;
	};

	struct FKeyPair
	{
		Crypto::FPublicKey sPublicKey;
		Crypto::FSecretKey sSecretKey;
	};

	using BinaryArray = std::vector <uint8_t>;

} // namespace QwertyNote
