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

#include <algorithm>

#include <Common/PathTools.h>

namespace {

    const char GENERIC_PATH_SEPARATOR = '/';

#ifdef _WIN32
    const char NATIVE_PATH_SEPARATOR = '\\';
#else
    const char NATIVE_PATH_SEPARATOR = '/';
#endif

    std::string::size_type findExtensionPosition (const std::string &cFilename)
    {
        auto pos = cFilename.rfind('.');

        if (pos != std::string::npos) {
            auto slashPos = cFilename.rfind(GENERIC_PATH_SEPARATOR);
            if (slashPos != std::string::npos && slashPos > pos) {
                return std::string::npos;
            }
        }

        return pos;
    }

} // namespace

namespace Common {
    bool hasParentPath (const std::string &cPath)
    {
        return cPath.find(GENERIC_PATH_SEPARATOR) != std::string::npos;
    }

    std::string nativePathToGeneric (const std::string &cNativePath)
    {
        if (GENERIC_PATH_SEPARATOR == NATIVE_PATH_SEPARATOR) {
            return cNativePath;
        }

        std::string genericPath(cNativePath);
        std::replace(genericPath.begin(),
                     genericPath.end(),
                     NATIVE_PATH_SEPARATOR,
                     GENERIC_PATH_SEPARATOR);

        return genericPath;
    }

    std::string getPathDirectory (const std::string &cPath)
    {
        auto slashPos = cPath.rfind(GENERIC_PATH_SEPARATOR);
        if (slashPos == std::string::npos) {
            return std::string();
        }

        return cPath.substr(0, slashPos);
    }

    std::string getPathFilename (const std::string &cPath)
    {
        auto slashPos = cPath.rfind(GENERIC_PATH_SEPARATOR);
        if (slashPos == std::string::npos) {
            return cPath;
        }

        return cPath.substr(slashPos + 1);
    }

    void splitPath (const std::string &cPath, std::string &cDirectory, std::string &cFilename)
    {
        cDirectory = getPathDirectory(cPath);
        cFilename = getPathFilename(cPath);
    }

    std::string combinePath (const std::string &cPath1, const std::string &cPath2)
    {
        return cPath1.empty() ? cPath2 : cPath1 + GENERIC_PATH_SEPARATOR + cPath2;
    }

    std::string replaceExtension (const std::string &cPath, const std::string &cExtension)
    {
        return removeExtension(cPath) + cExtension;
    }

    std::string getExtension (const std::string &cPath)
    {
        auto pos = findExtensionPosition(cPath);
        if (pos != std::string::npos) {
            return cPath.substr(pos);
        }

        return std::string();
    }

    std::string removeExtension (const std::string &cPath)
    {
        auto pos = findExtensionPosition(cPath);
        if (pos == std::string::npos) {
            return cPath;
        }

        return cPath.substr(0, pos);
    }

} // namespace Common
