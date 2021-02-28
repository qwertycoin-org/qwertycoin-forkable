// Copyright (c) 2012-2017, The QwertyNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project, The Forknote developers
// Copyright (c) 2018, Ryo Currency Project
// Copyright (c) 2016-2018, The Karbowanec developers
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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>

#include <boost/uuid/uuid.hpp>

namespace QwertyNote {

namespace parameters {
// Currency parameters
const uint64_t  ADDRESS_BASE58_PREFIX = 0x430c; // Addresses starts with "QWF"
const size_t    DISPLAY_DECIMAL_POINT = 8;
const uint64_t  COIN                                           = std::pow(10, DISPLAY_DECIMAL_POINT);
const uint64_t  MONEY_SUPPLY                                   = (uint64_t)(-1);
const size_t    COIN_VERSION = 1;
const unsigned  EMISSION_SPEED_FACTOR                          = 19;
const uint64_t  TAIL_EMISSION_REWARD                           = 10000000000;

// Block parameters
const uint64_t MAX_BLOCK_NUMBER                               = 500000000;
const size_t   MAX_BLOCK_BLOB_SIZE = 500000000;
const size_t   MAX_TX_SIZE = 1000000000;
const uint64_t DIFFICULTY_TARGET                              = 120; // seconds
const size_t   MINED_MONEY_UNLOCK_WINDOW = 60;
const uint64_t BLOCK_FUTURE_TIME_LIMIT = DIFFICULTY_TARGET * 60;
const size_t   BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW              = DIFFICULTY_TARGET / 2;
const size_t   CRYPTONOTE_REWARD_BLOCKS_WINDOW                = 100;
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE      = 1000000;
const size_t   CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE         = 600;
const uint64_t EXPECTED_NUMBER_OF_BLOCKS_PER_DAY              = 24 * 60 * 60 / DIFFICULTY_TARGET;
const size_t   DIFFICULTY_WINDOW                              = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY; // blocks
const size_t   DIFFICULTY_WINDOW_V2                           = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY / 24;

const size_t   DIFFICULTY_CUT                                 = 60;  // timestamps to cut after sorting
const size_t   DIFFICULTY_LAG                                 = 15;  // !!!
static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Bad DIFFICULTY_WINDOW or DIFFICULTY_CUT");
const size_t DEFAULT_DIFFICULTY                               = 1;
const size_t FIXED_DIFFICULTY                                 = 0;

// Tx parameters
const size_t   TX_SPENDABLE_AGE = 10;
const size_t   SAFE_TX_SPENDABLE_AGE = 1;
const uint64_t DEFAULT_DUST_THRESHOLD                         = UINT64_C(100000);

/* This section defines our minimum fee count required for transactions */
const uint64_t MINIMUM_FEE                                    = UINT64_C(10000);
const uint64_t MAXIMUM_FEE                                    = UINT64_C(250000);
const uint64_t BASE_FEE                                       = UINT64_C(25000);

/* This section defines the fees are remote node will receive from the sender */
const double   REMOTE_NODE_FEE_FACTOR                         = 0.25; // percent
const uint64_t MAX_REMOTE_NODE_FEE                            = UINT64_C(10000000000);

/* This section defines our minimum and maximum mixin counts required for transactions */
const uint64_t MIN_TX_MIXIN_SIZE                              = 0;
const uint64_t MAX_TX_MIXIN_SIZE                              = 20;

/* Maximum transaction size in byte */
const uint64_t MAX_TRANSACTION_SIZE_LIMIT                     = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 2 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;

const uint64_t POISSON_CHECK_TRIGGER               = 10;   // Reorg size that triggers poisson timestamp check
const uint64_t POISSON_CHECK_DEPTH                 = 60;   // Main-chain depth of the poisson check. The attacker will have to tamper 50% of those blocks
const double   POISSON_LOG_P_REJECT                = -75.0;// Reject reorg if the probablity that the timestamps are genuine is below e^x, -75 = 10^-33

const size_t   MAX_BLOCK_SIZE_INITIAL                         = 1000000;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR          = 100 * 1024;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR        = 365 * 24 * 60 * 60 / DIFFICULTY_TARGET;

const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS      = 1;
const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS     = DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;

const uint64_t CRYPTONOTE_MEMPOOL_TX_LIVETIME                 = 60 * 60 * 14; // 14 hours
const uint64_t CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME  = 60 * 60 * 24; // 24 hours
const uint64_t CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL = 7;

const uint64_t CRYPTONOTE_CLIF_THRESHOLD                     = 60 * 60; // 1 hour

const size_t   FUSION_TX_MAX_SIZE                             = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 30 / 100;
const size_t   FUSION_TX_MIN_INPUT_COUNT                      = 12;
const size_t   FUSION_TX_MIN_IN_OUT_COUNT_RATIO               = 4;

const uint32_t UPGRADE_HEIGHT_V1                              = 1;
const uint32_t UPGRADE_HEIGHT_V2                              = 30;
const uint32_t UPGRADE_HEIGHT_V3                              = 40;
const uint32_t UPGRADE_HEIGHT_V4                              = 50; // Reserved for
const uint32_t UPGRADE_HEIGHT_V5                              = 60; // future
const uint32_t UPGRADE_HEIGHT_V6                              = 100; // use :)

const uint32_t TESTNET_UPGRADE_HEIGHT_V1                      = 1;
const uint32_t TESTNET_UPGRADE_HEIGHT_V2                      = 20;
const uint32_t TESTNET_UPGRADE_HEIGHT_V3                      = 300;
const uint32_t TESTNET_UPGRADE_HEIGHT_V4                      = 4000;
const uint32_t TESTNET_UPGRADE_HEIGHT_V5                      = 50000;
const uint32_t TESTNET_UPGRADE_HEIGHT_V6                      = 600000;

const unsigned UPGRADE_VOTING_THRESHOLD                      = 90; // percent
const uint32_t UPGRADE_VOTING_WINDOW                         = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;  // blocks
const uint32_t UPGRADE_WINDOW                                = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;  // blocks
static_assert(UPGRADE_VOTING_THRESHOLD <= 100, "Bad UPGRADE_VOTING_THRESHOLD");
static_assert(UPGRADE_VOTING_WINDOW > 1, "Bad UPGRADE_VOTING_WINDOW");

const char     CRYPTONOTE_BLOCKS_FILENAME[]                  = "blocks.bin";
const char     CRYPTONOTE_BLOCKINDEXES_FILENAME[]            = "blockindexes.bin";
const char     CRYPTONOTE_BLOCKSCACHE_FILENAME[]             = "blockscache.bin";
const char     CRYPTONOTE_POOLDATA_FILENAME[]                = "poolstate.dat";
const char     P2P_NET_DATA_FILENAME[]                       = "p2pstate.dat";
const char     CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME[]      = "blockchainindices.bin";
const char     MINER_CONFIG_FILE_NAME[]                      = "miner_conf.json";

/* Governance Fee and range // The QWC Foundation */
const uint16_t GOVERNANCE_PERCENT_FEE                        = 10; // 10 percent of base block reward
const uint32_t GOVERNANCE_HEIGHT_START                       = UPGRADE_HEIGHT_V2;
const uint32_t GOVERNANCE_HEIGHT_END                         = 10000000;
const uint16_t TESTNET_GOVERNANCE_PERCENT_FEE                = 10; // 10 percent of base block reward
const uint32_t TESTNET_GOVERNANCE_HEIGHT_START               = TESTNET_UPGRADE_HEIGHT_V2;
const uint32_t TESTNET_GOVERNANCE_HEIGHT_END                 = 10000000;

/* Governance Fee Wallets // The QWC Foundation */
const std::string GOVERNANCE_WALLET_ADDRESS                  = "QWFiAbhkBoWdF1g9tufMfubsz5e6yyx3h8WESitYYUmugihGS6a5YPfS4xKq1W6VWucreYfGXdqUReY1kQy2pUdg6uag8Yw1KG";
const std::string GOVERNANCE_VIEW_SECRET_KEY                 = "8e70e25148875bc7850a068c57bd91c8bff65eb2009c70eeeba2c87fd0f9ca0c";

const std::string TESTNET_GOVERNANCE_WALLET_ADDRESS          = "QWFi5QHxmajhg2SxRS1FhjcyoM2FTYxmSjmvaBr7uXzz1b8Ms4M2h8PiKTL2YswTz1dFrV8ry5UQmBBLrb27tecs7MuNbLGuy5";
const std::string TESTNET_GOVERNANCE_VIEW_SECRET_KEY         = "f747f4a4838027c9af80e6364a941b60c538e67e9ea198b6ec452b74c276de06";

// Fixed rewarding
const bool FIXED_REWARDING                                   = true;
const uint64_t START_BLOCK_REWARD                            = (UINT64_C(300000) * COIN);
const uint64_t MIN_BLOCK_REWARD                              = (UINT64_C(100) * COIN);
const uint64_t REWARD_HALVING_INTERVAL                       = UINT64_C(11000);

} // namespace parameters

const char     CRYPTONOTE_NAME[]                             = "QwertyFork";
const char     GENESIS_COINBASE_TX_HEX[]                     = "010a01ff000180c0dfda8ee906029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807121017f9b72c9323f424c3996c0a5ad6542f7c87e404d4b204ee265eecf97ae05ce41";
const char     NETWORK_ID_BASE[]                             = "deadbeef";
const char     DNS_CHECKPOINTS_HOST[]                        = "checkpoints-qwertyfork.qwertycoin.org";

const uint8_t  TRANSACTION_VERSION_1                         =  1;
const uint8_t  TRANSACTION_VERSION_2                         =  2;
const uint8_t  CURRENT_TRANSACTION_VERSION                   =  TRANSACTION_VERSION_1;

const uint8_t  BLOCK_MAJOR_VERSION_1                         =  1;
const uint8_t  BLOCK_MAJOR_VERSION_2                         =  2;
const uint8_t  BLOCK_MAJOR_VERSION_3                         =  3;
const uint8_t  BLOCK_MAJOR_VERSION_4                         =  4;
const uint8_t  BLOCK_MAJOR_VERSION_5                         =  5;
const uint8_t  BLOCK_MAJOR_VERSION_6                         =  6;

const uint8_t  BLOCK_MINOR_VERSION_0                         =  0;
const uint8_t  BLOCK_MINOR_VERSION_1                         =  1;

const size_t   BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT        =  10000; // by default, blocks ids count in synchronizing
const size_t   BLOCKS_SYNCHRONIZING_DEFAULT_COUNT            =  128; // by default, blocks count in blocks downloading
const size_t   COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT         =  1000;

const int      P2P_DEFAULT_PORT                              =  5196;
const int      RPC_DEFAULT_PORT                              =  5197;
const int      SERVICE_DEFAULT_PORT                          =  5198;

const size_t   P2P_LOCAL_WHITE_PEERLIST_LIMIT                =  1000;
const size_t   P2P_LOCAL_GRAY_PEERLIST_LIMIT                 =  5000;

// P2P Network Configuration Section - This defines our current P2P network version
// and the minimum version for communication between nodes
const uint8_t  P2P_CURRENT_VERSION                           = 1;
const uint8_t  P2P_MINIMUM_VERSION                           = 1;

// This defines the number of versions ahead we must see peers before we start displaying
// warning messages that we need to upgrade our software.
const uint8_t  P2P_UPGRADE_WINDOW                            = 1;

const size_t   P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE          = 64 * 1024 * 1024; // 64 MB
const uint32_t P2P_DEFAULT_CONNECTIONS_COUNT                 = 8;
const size_t   P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT     = 70;
const uint32_t P2P_DEFAULT_HANDSHAKE_INTERVAL                = 60;            // seconds
const uint32_t P2P_DEFAULT_PACKET_MAX_SIZE                   = 50000000;      // 50000000 bytes maximum packet size
const uint32_t P2P_DEFAULT_PEERS_IN_HANDSHAKE                = 250;
const uint32_t P2P_DEFAULT_CONNECTION_TIMEOUT                = 5000;          // 5 seconds
const uint32_t P2P_DEFAULT_PING_CONNECTION_TIMEOUT           = 2000;          // 2 seconds
const uint64_t P2P_DEFAULT_INVOKE_TIMEOUT                    = 60 * 2 * 1000; // 2 minutes
const size_t   P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT          = 5000;          // 5 seconds
const uint32_t P2P_FAILED_ADDR_FORGET_SECONDS                = (60 * 60);     //1 hour
const uint32_t P2P_IP_BLOCKTIME                              = (60 * 60 * 24);//24 hour
const uint32_t P2P_IP_FAILS_BEFORE_BLOCK                     = 10;
const uint32_t P2P_IDLE_CONNECTION_KILL_INTERVAL             = (5 * 60);      //5 minutes
const char     P2P_STAT_TRUSTED_PUB_KEY[]                    = "07190a04cf6b457c09e39c9c90b1f8da9d263d1101318884a194b47ca6590e43";

const char        LATEST_VERSION_URL[]                       = "https://releases.qwertycoin.org";
const std::string LICENSE_URL                                = "https://github.com/qwertycoin-org/qwertycoin-forkable/blob/master/LICENSE.txt";

/* Modules */

// P2P encrypted blockchain messenger settings:
const bool     P2P_MESSAGES                                  =  true;
const uint16_t P2P_MESSAGES_CHAR_COUNT                       =  160;

const char *const SEED_NODES[] = {
	"n00-fork.qwertycoin.org:5196",
	"n01-fork.qwertycoin.org:5196"
	/*
		Add your lovely nodes!
		Recommended: At best 8, at least: 2
	*/
};

} // namespace QwertyNote

#define ALLOW_DEBUG_COMMANDS