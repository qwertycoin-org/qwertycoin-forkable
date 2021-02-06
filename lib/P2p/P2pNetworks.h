// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2019, The Karbowanec developers
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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <Global/QwertyNoteConfig.h>

#pragma once

namespace QwertyNote {

namespace {

boost::uuids::uuid name;
std::string network_1 = GENESIS_COINBASE_TX_HEX;
std::string network_2 = NETWORK_ID_BASE;

boost::uuids::name_generator gen(name);
boost::uuids::uuid u = gen(network_1 + network_2);

} // namespace

const static boost::uuids::uuid QWERTYCOIN_NETWORK = u;

} // namespace QwertyNote
