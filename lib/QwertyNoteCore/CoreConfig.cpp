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
#include <Common/Util.h>

#include <QwertyNoteCore/CoreConfig.h>

namespace QwertyNote {

CoreConfig::CoreConfig()
{
    configFolder = Tools::getDefaultDataDirectory();
}

void CoreConfig::init(const boost::program_options::variables_map &options)
{
    if (options.count(CommandLine::argDataDir.name) != 0
        && (!options[CommandLine::argDataDir.name].defaulted()
            || configFolder == Tools::getDefaultDataDirectory())
        ) {
        configFolder = CommandLine::getArg(options, CommandLine::argDataDir);
        configFolderDefaulted = options[CommandLine::argDataDir.name].defaulted();
    }
}

void CoreConfig::initOptions(boost::program_options::options_description &desc)
{
}

} // namespace QwertyNote
