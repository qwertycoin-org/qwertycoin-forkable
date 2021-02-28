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

#include <iostream>
#include <type_traits>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace CommandLine {

    template <typename T, bool required = false>
    struct FArgDescriptor
    {
    };

    template <typename T>
    struct FArgDescriptor <T, false>
    {
        typedef T vValueType;

        const char *cName;
        const char *cDescription;
        T defaultValue;
        bool bNotUseDefault;
    };

    template <typename T>
    struct FArgDescriptor <std::vector <T>, false>
    {
        typedef std::vector <T> vValueType;

        const char *cName;
        const char *cDescription;
    };

    template <typename T>
    struct FArgDescriptor <T, true>
    {
        static_assert(!std::is_same <T, bool>::value, "Boolean switch can't be required");

        typedef T vValueType;

        const char *cName;
        const char *cDescription;
    };

    template <typename T>
    boost::program_options::typed_value <T, char> *
    makeSemantic (const FArgDescriptor <T, true> &/*arg*/)
    {
        return boost::program_options::value <T>()->required();
    }

    template <typename T>
    boost::program_options::typed_value <T, char> *
    makeSemantic (const FArgDescriptor <T, false> &arg)
    {
        auto semantic = boost::program_options::value <T>();
        if (!arg.bNotUseDefault) {
            semantic->default_value(arg.defaultValue);
        }

        return semantic;
    }

    template <typename T>
    boost::program_options::typed_value <T, char> *
    makeSemantic (const FArgDescriptor <T, false> &arg,
                  const T &def)
    {
        auto semantic = boost::program_options::value <T>();
        if (!arg.bNotUseDefault) {
            semantic->default_value(def);
        }

        return semantic;
    }

    template <typename T>
    boost::program_options::typed_value <std::vector <T>, char> *makeSemantic (
        const FArgDescriptor <std::vector <T>, false> &arg /*arg*/)
    {
        auto semantic = boost::program_options::value <std::vector <T>>();
        semantic->default_value(std::vector <T>(), "");

        return semantic;
    }

    template <typename T, bool required>
    void addArg (boost::program_options::options_description &description,
                 const FArgDescriptor <T, required> &sArg,
                 bool bUnique = true)
    {
        if (bUnique && description.find_nothrow(sArg.cName, false) != nullptr) {
            std::cerr << "Argument already exists: " << sArg.cName << std::endl;
            return;
        }

        description.add_options()(sArg.cName, makeSemantic(sArg), sArg.cDescription);
    }

    template <typename T>
    void addArg (boost::program_options::options_description &description,
                 const FArgDescriptor <T, false> &sArg,
                 const T &def,
                 bool bUnique = true)
    {
        if (bUnique && description.find_nothrow(sArg.cName, false) != nullptr) {
            std::cerr << "Argument already exists: " << sArg.cName << std::endl;
            return;
        }

        description.add_options()(sArg.cName, makeSemantic(sArg, def), sArg.cDescription);
    }

    template <>
    inline void addArg (boost::program_options::options_description &description,
                        const FArgDescriptor <bool, false> &sArg,
                        bool bUnique)
    {
        if (bUnique && description.find_nothrow(sArg.cName, false) != nullptr) {
            std::cerr << "Argument already exists: " << sArg.cName << std::endl;
            return;
        }

        description.add_options()(sArg.cName,
                                  boost::program_options::bool_switch(),
                                  sArg.cDescription);
    }

    template <typename charT>
    boost::program_options::basic_parsed_options <charT> parseCommandLine (
        int iArgCount,
        const charT *const *sArgValue,
        const boost::program_options::options_description &desc,
        bool bAllowUnregistered = false)
    {
        auto parser = boost::program_options::command_line_parser(iArgCount, sArgValue);
        parser.options(desc);
        if (bAllowUnregistered) {
            parser.allow_unregistered();
        }

        return parser.run();
    }

    template <typename F>
    bool handleErrorHelper (const boost::program_options::options_description &desc,
                            F parser)
    {
        try {
            return parser();
        } catch (std::exception &e) {
            std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
            std::cerr << desc << std::endl;

            return false;
        } catch (...) {
            std::cerr << "Failed to parse arguments: unknown exception" << std::endl;
            std::cerr << desc << std::endl;

            return false;
        }
    }

    template <typename T, bool required>
    bool hasArg (const boost::program_options::variables_map &vm,
                 const FArgDescriptor <T, required> &sArg)
    {
        auto value = vm[sArg.cName];

        return !value.empty();
    }


    template <typename T, bool required>
    T getArg (const boost::program_options::variables_map &vm,
              const FArgDescriptor <T, required> &sArg)
    {
        return vm[sArg.cName].template as <T>();
    }

    template <>
    inline bool hasArg <bool, false> (const boost::program_options::variables_map &vm,
                                      const FArgDescriptor <bool, false> &sArg)
    {
        return getArg <bool, false>(vm, sArg);
    }

    extern const FArgDescriptor <bool> sArgHelp;
    extern const FArgDescriptor <bool> sArgVersion;
    extern const FArgDescriptor <std::string> sArgDataDir;

} // namespace CommandLine
