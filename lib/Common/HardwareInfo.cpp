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

#ifndef _WIN32

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/utsname.h>
#include <unistd.h>

#endif

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <thread>
#include <windows.h>

#endif

#if __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include <vector>

#include <Common/HardwareInfo.h>

using namespace Tools::CPU;

#ifdef _WIN32

static std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> cpuinfoBuffer()
{
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> mBuffer;

    DWORD byte_count = 0;
    GetLogicalProcessorInformation(nullptr, &byte_count);
    mBuffer.resize(byte_count / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    GetLogicalProcessorInformation(mBuffer.data(), &byte_count);

    return mBuffer;
}

#endif

std::string Tools::CPU::architecture() noexcept
{
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    case PROCESSOR_ARCHITECTURE_IA64:
        return "Itanium";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    default:
        return "unknown";
    }
#endif

#ifndef _WIN32
    utsname buf {};
    if (uname(&buf) == -1) {
        return "unknown";
    }

    if (!strcmp(buf.machine, "x86_64")) {
        return "x64";
    }
    else if (strstr(buf.machine, "arm") == buf.machine) {
        return "ARM";
    }
    else if (!strcmp(buf.machine, "ia64") || !strcmp(buf.machine, "IA64")) {
        return "Itanium";
    }
    else if (!strcmp(buf.machine, "i686")) {
        return "x86";
    }
    else {
        return "unknown";
    }
#endif
}

Tools::CPU::FQuantities Tools::CPU::quantities()
{
    FQuantities sReturn {};

#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    sReturn.uLogical = sysinfo.dwNumberOfProcessors;
    sReturn.uPhysical = sReturn.uLogical / 2;
#elif __APPLE__
    int nm[2];
    size_t len = 4;
    uint32_t mCount;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &mCount, &len, NULL, 0);

    if (mCount < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &mCount, &len, NULL, 0);
        if (mCount < 1) {
            mCount = 1;
        }
    }
    sReturn.uLogical = mCount;
    sReturn.uPhysical = sReturn.uLogical / 2;
#else
    sReturn.uLogical = sysconf(_SC_NPROCESSORS_ONLN);

    std::ifstream cpuinfo("/proc/cpuinfo");

    if (!cpuinfo.is_open() || !cpuinfo) {
        return sReturn;
    }

    std::vector<unsigned int> packageIds;
    for (std::string line; std::getline(cpuinfo, line);) {
        if (line.find("uPhysical id") == 0) {
            const auto physicalId =
                    std::strtoul(line.c_str() + line.find_first_of("1234567890"), nullptr, 10);
            if (std::find(packageIds.begin(), packageIds.end(), physicalId) == packageIds.end()) {
                packageIds.emplace_back(physicalId);
            }
        }
    }

    sReturn.uPackages = packageIds.size();
    sReturn.uPhysical = sReturn.uLogical / sReturn.uPackages;
#endif
    return sReturn;
}
