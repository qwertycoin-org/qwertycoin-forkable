// Copyright (c) 2017-2018, Karbo developers
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
// along with Qwertycoin. If not, see <http://www.gnu.org/licenses/>.

#include <cstring>
#include <future>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <boost/program_options/variables_map.hpp>

#include <Common/DnsTools.h>

#ifdef _WIN32

#include <Windows.h>
#include <io.h>
#include <windns.h>
#include <Rpc.h>
#include <sstream>

#else
#include <arpa/nameser.h>
#include <resolv.h>
#endif

namespace Common {

#ifndef __ANDROID__

    bool fetchDnsTxt (const std::string &cDomain,
                      std::vector <std::string> &vRecords)
    {
#ifdef _WIN32
        using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")

        PDNS_RECORD pDnsRecord; // pointer to DNS_RECORD structure.

        {
            WORD type = DNS_TYPE_TEXT;

            if (0 != DnsQuery_A(cDomain.c_str(), type, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord,
                                NULL)) {
                cerr << "Error querying: '" << cDomain << "'" << endl;
                return false;
            }
        }

        PDNS_RECORD it;
        map <WORD, function <void (void)>> callbacks;

        callbacks[DNS_TYPE_TEXT] = [&it, &vRecords] (void) -> void
        {
            std::stringstream stream;
            for (DWORD i = 0; i < it->Data.TXT.dwStringCount; i++) {
                stream << RPC_CSTR(it->Data.TXT.pStringArray[i]) << endl;;
            }
            vRecords.push_back(stream.str());
        };

        for (it = pDnsRecord; it != NULL; it = it->pNext) {
            if (callbacks.count(it->wType)) {
                callbacks[it->wType]();
            }
        }

        DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);
#else
        using namespace std;

        res_init();

        ns_msg nsMsg;
        int response;
        unsigned char queryBuffer[4096];

        {
            ns_type sType = ns_t_txt;

            const char *domain = (cDomain).c_str();
            response = res_query(domain, 1, sType, queryBuffer, sizeof(queryBuffer));

            if (response < 0) {
                return false;
            }
        }

        ns_initparse(queryBuffer, response, &nsMsg);

        map<ns_type, function<void(const ns_rr &rr)>> callbacks;

        callbacks[ns_t_txt] = [&nsMsg, &vRecords](const ns_rr &rr) -> void {
            int txt_len = *(unsigned char *) ns_rr_rdata(rr);
            char txt[256];
            memset(txt, 0, 256);
            if (txt_len <= 255) {
                memcpy(txt, ns_rr_rdata(rr) + 1, txt_len);
                vRecords.emplace_back(txt);
            }
        };

        for (int x = 0; x < ns_msg_count(nsMsg, ns_s_an); x++) {
            ns_rr rr;
            ns_parserr(&nsMsg, ns_s_an, x, &rr);
            ns_type sType = ns_rr_type(rr);
            if (callbacks.count(sType)) {
                callbacks[sType](rr);
            }
        }
#endif
        return !vRecords.empty();
    }

#endif

} // namespace Common
