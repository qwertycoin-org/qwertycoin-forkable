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

#include <Common/CommandLine.h>

#include <QwertyNoteCore/MinerConfig.h>

namespace QwertyNote {

namespace {

const CommandLine::FArgDescriptor<std::string> arg_extra_messages = {
    "extra-messages-file",
    "Specify file for extra messages to include into coinbase transactions",
    "",
    true
};

const CommandLine::FArgDescriptor<std::string> arg_start_mining = {
    "start-mining",
    "Specify wallet address to mining for",
    "",
    true
};

const CommandLine::FArgDescriptor<uint32_t> arg_mining_threads = {
    "mining-threads",
    "Specify mining threads mCount",
    0,
    true
};

} // namespace

MinerConfig::MinerConfig()
{
    miningThreads = 0;
}

void MinerConfig::initOptions(boost::program_options::options_description &desc)
{
    CommandLine::addArg(desc, arg_extra_messages);
    CommandLine::addArg(desc, arg_start_mining);
    CommandLine::addArg(desc, arg_mining_threads);
}

void MinerConfig::init(const boost::program_options::variables_map &options)
{
    if(CommandLine::hasArg(options, arg_extra_messages)) {
        extraMessages = CommandLine::getArg(options, arg_extra_messages);
    }

    if (CommandLine::hasArg(options, arg_start_mining)) {
        startMining = CommandLine::getArg(options, arg_start_mining);
    }

    if (CommandLine::hasArg(options, arg_mining_threads)) {
        miningThreads = CommandLine::getArg(options, arg_mining_threads);
    }
}

} //namespace QwertyNote
