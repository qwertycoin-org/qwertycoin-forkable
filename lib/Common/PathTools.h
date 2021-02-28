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

#include <string>

namespace Common {
    bool hasParentPath (const std::string &cPath);

    std::string nativePathToGeneric (const std::string &cNativePath);

    std::string getPathDirectory (const std::string &cPath);
    std::string getPathFilename (const std::string &cPath);
    void splitPath (const std::string &cPath, std::string &cDirectory, std::string &cFilename);

    std::string combinePath (const std::string &cPath1, const std::string &cPath2);
    std::string getExtension (const std::string &cPath);
    std::string removeExtension (const std::string &cPath);
    std::string replaceExtension (const std::string &cPath, const std::string &cExtension);

} // namespace Common
