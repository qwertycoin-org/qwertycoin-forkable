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

template<typename T, bool required = false>
struct ArgDescriptor {
};

template<typename T>
struct ArgDescriptor<T, false>
{
    typedef T valueType;

    const char *name;
    const char *description;
    T defaultValue;
    bool notUseDefault;
};

template<typename T>
struct ArgDescriptor<std::vector<T>, false>
{
    typedef std::vector<T> valueType;

    const char *name;
    const char *description;
};

template<typename T>
struct ArgDescriptor<T, true>
{
    static_assert(!std::is_same<T, bool>::value, "Boolean switch can't be required");

    typedef T valueType;

    const char *name;
    const char *description;
};

template<typename T>
boost::program_options::typed_value<T, char> *makeSemantic(const ArgDescriptor<T, true> &/*arg*/)
{
    return boost::program_options::value<T>()->required();
}

template<typename T>
boost::program_options::typed_value<T, char> *makeSemantic(const ArgDescriptor<T, false> &arg)
{
    auto semantic = boost::program_options::value<T>();
    if (!arg.notUseDefault) {
        semantic->default_value(arg.defaultValue);
    }

    return semantic;
}

template<typename T>
boost::program_options::typed_value<T, char> *makeSemantic(const ArgDescriptor<T, false> &arg,
                                                            const T &def)
{
    auto semantic = boost::program_options::value<T>();
    if (!arg.notUseDefault) {
        semantic->default_value(def);
    }

    return semantic;
}

template<typename T>
boost::program_options::typed_value<std::vector<T>, char> *makeSemantic(
    const ArgDescriptor<std::vector<T>, false> &arg /*arg*/)
{
    auto semantic = boost::program_options::value<std::vector<T>>();
    semantic->default_value(std::vector<T>(), "");

    return semantic;
}

template<typename T, bool required>
void addArg(boost::program_options::options_description &description,
            const ArgDescriptor<T, required> &arg,
            bool unique = true)
{
    if (unique && description.find_nothrow(arg.name, false) != nullptr) {
        std::cerr << "Argument already exists: " << arg.name << std::endl;
        return;
    }

    description.add_options()(arg.name, makeSemantic(arg), arg.description);
}

template<typename T>
void addArg(boost::program_options::options_description &description,
            const ArgDescriptor<T, false> &arg,
            const T &def,
            bool unique = true)
{
    if (unique && description.find_nothrow(arg.name, false) != nullptr) {
        std::cerr << "Argument already exists: " << arg.name << std::endl;
        return;
    }

    description.add_options()(arg.name, makeSemantic(arg, def), arg.description);
}

template<>
inline void addArg(boost::program_options::options_description &description,
                   const ArgDescriptor<bool, false> &arg,
                   bool unique)
{
    if (unique && description.find_nothrow(arg.name, false) != nullptr) {
        std::cerr << "Argument already exists: " << arg.name << std::endl;
        return;
    }

    description.add_options()(arg.name, boost::program_options::bool_switch(), arg.description);
}

template<typename charT>
boost::program_options::basic_parsed_options<charT> parseCommandLine(
        int argc,
        const charT *const *argv,
        const boost::program_options::options_description &desc,
        bool allowUnregistered = false)
{
    auto parser = boost::program_options::command_line_parser(argc, argv);
    parser.options(desc);
    if (allowUnregistered) {
        parser.allow_unregistered();
    }

    return parser.run();
}

template<typename F>
bool handleErrorHelper(const boost::program_options::options_description &desc,
                       F parser)
{
    try {
        return parser();
    } catch (std::exception& e) {
        std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
        std::cerr << desc << std::endl;

        return false;
    } catch (...) {
        std::cerr << "Failed to parse arguments: unknown exception" << std::endl;
        std::cerr << desc << std::endl;

        return false;
    }
}

template<typename T, bool required>
bool hasArg(const boost::program_options::variables_map &vm,
             const ArgDescriptor<T, required> &arg)
{
    auto value = vm[arg.name];

    return !value.empty();
}


template<typename T, bool required>
T getArg(const boost::program_options::variables_map &vm,
         const ArgDescriptor<T, required> &arg)
{
    return vm[arg.name].template as<T>();
}

template<>
inline bool hasArg<bool, false>(const boost::program_options::variables_map &vm,
                                const ArgDescriptor<bool, false> &arg)
{
    return getArg<bool, false>(vm, arg);
}

extern const ArgDescriptor<bool> argHelp;
extern const ArgDescriptor<bool> argVersion;
extern const ArgDescriptor<std::string> argDataDir;

} // namespace CommandLine
