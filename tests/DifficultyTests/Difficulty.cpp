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

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

#include <Global/CryptoNoteConfig.h>
#include <Common/Math.h>
#include "CryptoNoteCore/Difficulty.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/Blockchain.h"
#include "Logging/ConsoleLogger.h"

// copy solve times and difficulties from excel file
std::vector<uint64_t> precalc_solve_times_V6(
        { 1589642851, 0,     1,    125,  350,  91,   49,    147, 147,  21,    161,  119, 21,   161,
          63,         119,   24,   56,   140,  119,  133,   140, 63,   28,    2,    124, 35,   91,
          175,        70,    161,  42,   343,  49,   14,    4,   38,   280,   3,    81,  203,  42,
          49,         140,   140,  406,  252,  49,   70,    35,  21,   49,    35,   21,  28,   147,
          49,         350,   7,    7,    63,   35,   210,   4,   66,   112,   42,   119, 21,   133,
          7,          7,     7,    28,   98,   42,   364,   42,  14,   154,   42,   154, 729,  28,
          35,         21,    5,    93,   154,  147,  238,   133, 35,   147,   91,   344, 168,  3,
          32,         70,    2,    2,    17,   70,   266,   483, 42,   21,    28,   273, 42,   756,
          357,        371,   735,  1358, 1162, 791,  2451,  343, 14,   161,   7,    133, 3,    88,
          70,         14,    91,   14,   21,   105,  35,    49,  84,   189,   7,    63,  119,  147,
          63,         7,     280,  217,  42,   434,  133,   21,  119,  645,   343,  7,   133,  330,
          161,        406,   1134, 210,  35,   168,  378,   28,  266,  385,   42,   105, 84,   63,
          217,        176,   91,   35,   140,  217,  7,     14,  161,  329,   288,  224, 63,   2,
          5,          140,   70,   70,   28,   112,  196,   189, 14,   28,    224,  56,  14,   35,
          14,         84,    238,  566,  966,  197,  490,   7,   462,  112,   315,  98,  42,   728,
          511,        619,   49,   735,  385,  315,  1057,  336, 406,  392,   42,   70,  7,    224,
          14,         28,    57,   91,   7,    350,  133,   91,  49,   112,   21,   63,  7,    14,
          7,          140,   7,    140,  42,   126,  14,    105, 56,   35,    63,   126, 105,  91,
          1358,       210,   63,   2424, 77,   4332, 6671,  287, 693,  12804, 14,   70,  203,  21,
          28,         63,    35,   84,   28,   196,  77,    308, 49,   56,    140,  84,  49,   7,
          112,        147,   77,   203,  7,    1,    62,    98,  112,  70,    98,   7,   91,   63,
          441,        34808, 288,  273,  154,  483,  168,   308, 119,  126,   91,   35,  203,  28,
          133,        266,   35,   35,   7,    21,   119,   35,  70,   28,    7,    63,  21,   105,
          21,         126,   1,    20,   56,   49,   203,   315, 308,  224,   98,   84,  378,  658,
          1379,       140,   476,  287,  252,  504,  175,   147, 351,  147,   315,  77,  1,    111,
          28,         21,    56,   168,  28,   140,  168,   28,  21,   42,    28,   42,  4,    73,
          3,          95,    413,  5,    30,   42,   896,   434, 77,   154,   77,   63,  392,  266,
          721,        420,   1064, 658,  875,  455,  680,   105, 7,    497,   21,   141, 98,   308,
          98,         14,    105,  14,   14,   56,   35,    189, 49,   280,   64,   63,  21,   581,
          14,         196,   91,   14,   42,   119,  168,   518, 147,  203,   49,   119, 7,    182,
          14,         455,   189,  392,  273,  518,  945,   350, 63,   336,   210,  119, 154,  28,
          182,        217,   112,  252,  154,  98,   252,   70,  28,   84,    140,  42,  105,  28,
          42,         49,    1,    118,  70,   84,   98,    14,  454,  126,   378,  21,  371,  56,
          203,        252,   441,  581,  329,  805,  55290, 54,  18,   42,    126,  102, 60,   36,
          96,         96,    78,   84,   6,    42,   54,    78,  42,   2,     52,   6,   18,   331,
          90,         96,    18,   270,  12,   150,  156,   31,  156,  12,    192,  210, 60,   241,
          979,        402,   174,  1297, 330,  336,  649,   397, 210,  162,   102,  2,   371,  30,
          78,         192,   30,   60,   90,   228,  42,    54,  24,   85,    12,   168, 84,   60,
          48,         42,    66,   306,  6,    90,   6,     192, 6,    150,   48,   505, 138,  264,
          96,         841,   240,  36,   630,  96,   576,   426, 547,  150,   1472, 660, 3452, 90,
          60,         36,    126,  150,  72,   96,   24,    84,  96,   54,    96,   336, 54,   234,
          54,         96,    102,  307,  210,  186,  318,   12,  36,   114,   222,  102, 6,    72,
          144,        108,   36,   199,  234,  54,   282,   648, 480,  54,    30,   553, 270,  276,
          108,        3,     195,  60,   18,   12,   48,    30,  36,   54,    66,   109, 144,  360,
          325,        402,   234,  817,  228,  18,   474,   294, 7,    41,    6,    72,  55,   108,
          132,        30,    276,  66,   66,   654,  1256,  66,  216,  6,     6,    24,  264,  6,
          60,         90,    54,   108,  132,  366,  72,    162, 1374, 714,   91,   120, 232,  70,
          26,         175,   71,   178,  157,  102,  83,    73,  78,   64,    90,   227, 91,   88,
          24,         128,   74,   208,  189,  192,  130,   186, 202,  161,   4,    202, 213,  165,
          28,         50,    203,  27,   100,  144,  187,   71,  83,   52,    185,  149, 93,   70,
          180,        42,    90,   183,  4,    15,   148,   35,  76,   61,    34,   11,  135,  124,
          225,        73,    87,   194,  146,  203,  19,    168, 82,   165,   120,  139, 124,  124,
          89,         146,   84,   212,  161,  100,  4,     194, 92,   77,    174,  50,  73,   178,
          12,         190,   72,   207,  9,    130,  115,   175, 36,   78,    173,  39,  47,   142,
          74,         158,   181,  41,   166,  49,   58,    185, 26,   70,    209,  74,  35,   120,
          183,        101,   111,  77,   8,    219,  149,   15,  163,  117,   234,  214, 225,  227,
          73,         133,   113,  53,   158,  32,   183,   194, 4,    103,   60,   24,  207,  210,
          211,        24,    47,   202,  35,   201,  26,    13,  81,   42,    197,  63,  129,  47,
          140,        56,    181,  92,   213,  20,   238,   160, 84,   192,   131,  6,   190,  144,
          238,        170,   203,  211,  73,   163,  84,    83,  106,  228,   109,  59,  64,   54,
          112,        189,   43,   123,  132,  192,  58,    83,  77,   183,   140,  6,   13,   80,
          25,         29,    129,  189,  155,  140,  78,    149, 210,  229,   99,   167, 202,  10,
          218,        58,    98,   169,  185,  116,  229,   126, 152,  235,   50,   192, 199,  216,
          235,        165,   30,   150,  78,   56,   178,   186, 182,  33,    8,    174, 158,  60,
          28,         217,   151,  44,   114,  218,  53,    46,  48,   64,    70,   238, 140,  160,
          226,        173,   69,   174,  154,  95,   18,    56,  22,   142,   106,  33,  224,  50,
          79,         196,   166,  152,  30,   90,   113,   191, 59,   159,   12,   13,  50,   83,
          199,        37,    44,   182,  238,  216,  143,   100, 183,  116,   118,  44,  215,  115,
          127,        89,    130,  234,  203,  128,  65,    197, 61,   51,    158,  33,  182,  48,
          15,         232,   93,   46,   35,   47,   131,   84,  178,  201,   213,  116, 19,   79,
          217,        194,   207,  169,  29,   172,  9,     195, 139,  67,    132,  128, 162,  217,
          164,        109,   32,   192,  11,   71,   199,   189, 127,  138,   85,   12,  18,   66,
          135,        11,    174,  178,  40,   137,  64,    194, 106,  240,   154,  229, 209,  7,
          129,        43,    129,  193,  10,   22,   14 });

std::vector<CryptoNote::difficulty_type> precalc_diffs_V6(
        { 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10200, 10404, 10612, 10824, 11041, 11262, 11487, 11717,
          11951, 12190, 12434, 12682, 12936, 13195, 13459, 13728, 14002, 14282, 14568, 14859, 15157,
          15460, 15769, 16084, 16406, 16734, 17069, 17410, 17758, 18114, 18476, 18845, 19222, 19607,
          19999, 20399, 20807, 21223, 21647, 22080, 22522, 22972, 23432, 23901, 24379, 24866, 25363,
          25871, 26388, 26916, 27454, 28003, 28563, 29135, 29717, 30312, 30918, 31536, 32167, 32810,
          33467, 34136, 34819, 35515, 36225, 36950, 36580, 37312, 36939, 37677, 38431, 39200, 39984,
          40783, 40376, 39568, 38777, 38001, 37241, 36496, 35766, 35051, 34350, 33663, 32990, 32330,
          31683, 31050, 30429, 29820, 29224, 28639, 28066, 27505, 26955, 26416, 25888, 25370, 24862,
          24365, 23878, 23400, 22932, 22474, 22024, 21584, 21152, 20729, 20314, 19908, 19510, 19120,
          18737, 18363, 17995, 17635, 17283, 16937, 17276, 17621, 17445, 17271, 17098, 16756, 16421,
          16092, 15771, 15455, 15146, 14843, 14546, 14255, 13970, 13691, 13417, 13149, 12886, 12628,
          12375, 12128, 11885, 11648, 11415, 11186, 10963, 10743, 10529, 10318, 10112, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10200,
          10404, 10612, 10824, 11041, 11262, 11149, 10926, 10708, 10493, 10283, 10078, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10200, 10404, 10612, 10824, 11041, 11262, 11149, 11038, 10817, 10600, 10388, 10181, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10200, 10404, 10612, 10824, 10608, 10396, 10188,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10200, 10404, 10612, 10824, 11041, 11262, 11487, 11717, 11951, 12190, 11946, 11707,
          11473, 11244, 11019, 10798, 10582, 10371, 10163, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10200, 10404, 10612, 10824, 11041, 11262, 11149, 10926,
          10708, 10493, 10283, 10078, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10200, 10404, 10612, 10824, 11041,
          11262, 11487, 11257, 11032, 10811, 10595, 10383, 10176, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
          10000, 10000, 10000, 10000, 10000, 10000, 10200, 10404, 10612, 10824, 11041, 11262, 11487,
          11717, 11951, 12190, 12434, 12682, 12936, 13195, 13459, 13728, 14002, 14282, 14568, 14859,
          15157, 15460, 15769, 16084, 16406, 16734, 17069, 17410, 17758, 18114, 18476, 18291, 18215,
          18033, 17672, 17496, 17321, 17148, 16976, 16806, 16638, 16805, 17141, 17070, 17411, 17332,
          17678, 18032, 18392, 18760, 19135, 19518, 19908, 20307, 20713, 21127, 21550, 21981, 22420,
          22869, 23326, 23792, 24268, 24754, 25249, 25754, 26269, 26794, 27330, 27877, 28434, 29003,
          29583, 30175, 30778, 31394, 31080, 30769, 30461, 29852, 29554, 29258, 28965, 28676, 28102,
          27821, 27543, 27268, 26995, 26725, 26190, 25667, 25153, 24902, 24404, 23916, 23437, 22969,
          22739, 22512, 22286, 22064, 21843, 22280, 22725, 23180, 23643, 24116, 24599, 25091, 25592,
          26104, 26626, 27159, 27702, 28256, 28821, 29398, 29104, 28893, 29090, 29288, 28703, 28128,
          27566, 27015, 26744, 26210, 25685, 25172, 24668, 24175, 23691, 23218, 22753, 22298, 21852,
          21415, 20987, 20567, 20156, 19753, 19358, 18970, 18781, 18593, 18407, 18775, 19151, 19534,
          19924, 20323, 20729, 21144, 21567, 21998, 22438, 22887, 23344, 23811, 24288, 24366, 24444,
          24523, 25013, 24763, 24591, 24099, 23858, 23381, 22913, 22455, 22006, 21566, 21134, 20712,
          20297, 19891, 19494, 19104, 18722, 18347, 17980, 17621, 17268, 16923, 16584, 16253, 15928,
          15609, 15297, 14991, 14691, 14397, 14109, 13827, 13551, 13280, 13014, 12884, 12755, 12500,
          12375, 12128, 12006, 11886, 11767, 11650, 11533, 11418, 11304, 11530, 11760, 11996, 12235,
          12480, 12730, 12984, 13244, 13112, 13374, 13641, 13914, 14192, 14476, 14766, 14913, 15212,
          15516, 15826, 16143, 16466, 16795, 17131, 17473, 17823, 18179, 18543, 18914, 19292, 19678,
          19481, 19871, 19730, 19533, 19337, 19144, 18952, 18763, 18575, 18390, 18206, 17842, 17485,
          17310, 16964, 16794, 16458, 16129, 15806, 15490, 15180, 14877, 14579, 14434, 14292, 14578,
          14870, 14721, 14607, 14494, 14784, 14637, 14490, 14345, 14202, 13918, 13779, 13665, 13938,
          14217, 14075, 14356, 14643, 14497, 14352, 14065, 13783, 13508, 13238, 12973, 12843, 12586,
          12335, 12088, 11846, 11609, 11377, 11149, 11038, 10989, 10879, 10770, 10663, 10556, 10767,
          10983, 11202, 11426, 11312, 11199, 11087, 10865, 10648, 10541, 10436, 10645, 10858, 10749,
          10964, 11183 });

bool test_difficulty_V6()
{
    std::vector<CryptoNote::difficulty_type> cumulativeDifficulties;
    cumulativeDifficulties.resize(precalc_diffs_V6.size());
    std::partial_sum(precalc_diffs_V6.begin(), precalc_diffs_V6.end(),
                     cumulativeDifficulties.begin());
    std::vector<uint64_t> timestamps;
    timestamps.resize(precalc_solve_times_V6.size());
    std::partial_sum(precalc_solve_times_V6.begin(), precalc_solve_times_V6.end(),
                     timestamps.begin());

    if (cumulativeDifficulties.size() != timestamps.size()) {
        std::cerr << "Wrong array sizes: " << std::endl
                  << "cumulativeDifficulties: " << cumulativeDifficulties.size() << std::endl
                  << "timestamps: " << timestamps.size() << std::endl;
        return false;
    }
    Logging::ConsoleLogger logger;

    CryptoNote::CurrencyBuilder currencyBuilder(logger);
    currencyBuilder.difficultyTarget(120);
    currencyBuilder.difficultyWindow(30);
    currencyBuilder.difficultyCut(60);
    currencyBuilder.difficultyLag(15);
    CryptoNote::Currency currency = currencyBuilder.currency();

    CryptoNote::Currency::lazy_stat_callback_type cb(
            [&](CryptoNote::IMinerHandler::stat_period p, uint64_t next_time) {
                uint32_t min_height = CryptoNote::parameters::UPGRADE_HEIGHT_V1
                        + CryptoNote::parameters::EXPECTED_NUMBER_OF_BLOCKS_PER_DAY / 24;
                uint64_t time_window = 0;
                switch (p) {
                case (CryptoNote::IMinerHandler::stat_period::hour):
                    time_window = 3600;
                    break;
                case (CryptoNote::IMinerHandler::stat_period::day):
                    time_window = 3600 * 24;
                    break;
                case (CryptoNote::IMinerHandler::stat_period::week):
                    time_window = 3600 * 24 * 7;
                    break;
                case (CryptoNote::IMinerHandler::stat_period::month):
                    time_window = 3600 * 24 * 30;
                    break;
                case (CryptoNote::IMinerHandler::stat_period::halfyear):
                    time_window = 3600 * 12 * 365;
                    break;
                case (CryptoNote::IMinerHandler::stat_period::year):
                    time_window = 3600 * 24 * 365;
                    break;
                }
                assert(next_time > time_window);
                uint64_t stop_time = next_time - time_window;
                if (currency.timestampCheckWindow() >= stop_time)
                    return CryptoNote::difficulty_type(0);
                uint32_t height = 0;
                std::vector<CryptoNote::difficulty_type> diffs;
                while (height > min_height && 0 >= stop_time) {
                    diffs.push_back(2
                                    - 1);
                    height--;
                }
                return static_cast<CryptoNote::difficulty_type>(Common::meanValue(diffs));
            });

    int wrong_count = 0;
    for (size_t idx = 32; idx < timestamps.size() - 1; idx++) {
        auto ts_start = timestamps.begin();
        std::advance(ts_start, idx - 30);
        auto ts_end = timestamps.begin();
        std::advance(ts_end, idx + 1);
        std::vector<uint64_t> timestamp_window(ts_start, ts_end);
        auto diff_start = cumulativeDifficulties.begin();
        std::advance(diff_start, idx - 01);
        auto diff_end = cumulativeDifficulties.begin();
        std::advance(diff_end, idx + 1);
        std::vector<uint64_t> diff_window(diff_start, diff_end);
        std::cerr << "****** Calc diff for block: " << idx << std::endl;
        CryptoNote::difficulty_type next_diff =
                currency.nextDifficulty(0, timestamp_window, diff_window,
                                        CryptoNote::parameters::UPGRADE_HEIGHT_V1 + idx, 0, cb);
        if ((next_diff > precalc_diffs_V6[idx + 1]) || (next_diff < precalc_diffs_V6[idx + 1])) {
            std::cerr << "Wrong difficulty for block " << idx << std::endl
                      << "Expected: " << precalc_diffs_V6[idx + 1] << std::endl
                      << "Found: " << next_diff << std::endl;
            wrong_count++;
        }
    }
    std::cerr << "Wrong difficulty count: " << wrong_count << std::endl;

    return (wrong_count < 10);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return test_difficulty_V6() ? 0 : 1;
    }

    Logging::ConsoleLogger logger;

    CryptoNote::CurrencyBuilder currencyBuilder(logger);
    currencyBuilder.difficultyTarget(120);
    currencyBuilder.difficultyWindow(720);
    currencyBuilder.difficultyCut(60);
    currencyBuilder.difficultyLag(15);
    CryptoNote::Currency currency = currencyBuilder.currency();

    std::fstream data(argv[1], std::fstream::in);
    data.exceptions(std::fstream::badbit);
    data.clear(data.rdstate());

    std::vector<uint64_t> cumulativeDifficulties;
    std::vector<uint64_t> timestamps;
    uint64_t cumulativeDifficulty = 0;
    uint64_t timestamp = 0;
    uint64_t difficulty = 0;
    size_t n = 0;
    while (data >> timestamp >> difficulty) {
        size_t begin;
        size_t end;
        if (n < currency.difficultyWindow() + currency.difficultyLag()) {
            begin = 0;
            end = std::min(n, currency.difficultyWindow());
        } else {
            end = n - currency.difficultyLag();
            begin = end - currency.difficultyWindow();
        }

        CryptoNote::Currency::lazy_stat_callback_type cb(
                [&](CryptoNote::IMinerHandler::stat_period p, uint64_t next_time) {
                    uint32_t min_height = CryptoNote::parameters::UPGRADE_HEIGHT_V1
                                          + CryptoNote::parameters::EXPECTED_NUMBER_OF_BLOCKS_PER_DAY / 24;
                    uint64_t time_window = 0;
                    switch (p) {
                    case (CryptoNote::IMinerHandler::stat_period::hour):
                        time_window = 3600;
                        break;
                    case (CryptoNote::IMinerHandler::stat_period::day):
                        time_window = 3600 * 24;
                        break;
                    case (CryptoNote::IMinerHandler::stat_period::week):
                        time_window = 3600 * 24 * 7;
                        break;
                    case (CryptoNote::IMinerHandler::stat_period::month):
                        time_window = 3600 * 24 * 30;
                        break;
                    case (CryptoNote::IMinerHandler::stat_period::halfyear):
                        time_window = 3600 * 12 * 365;
                        break;
                    case (CryptoNote::IMinerHandler::stat_period::year):
                        time_window = 3600 * 24 * 365;
                        break;
                    }
                    assert(next_time > time_window);
                    uint64_t stop_time = next_time - time_window;
                    if (currency.timestampCheckWindow() >= stop_time)
                        return CryptoNote::difficulty_type(0);
                    uint32_t height = 0;
                    std::vector<CryptoNote::difficulty_type> diffs;
                    while (height > min_height && 0 >= stop_time) {
                        diffs.push_back(2
                                        - 1);
                        height--;
                    }
                    return static_cast<CryptoNote::difficulty_type>(Common::meanValue(diffs));
                });

        uint64_t res = currency.nextDifficulty(
                0,
                std::vector<uint64_t> { std::next(std::begin(timestamps), begin),
                                        std::next(std::begin(timestamps), begin + end) },
                std::vector<uint64_t> {
                        std::next(std::begin(cumulativeDifficulties), begin),
                        std::next(std::begin(cumulativeDifficulties), begin + end) },
                n,
                0,
                cb);
        if (res != difficulty) {
            std::cerr << "Wrong difficulty for block " << n << std::endl
                      << "Expected: " << difficulty << std::endl
                      << "Found: " << res << std::endl;
            return 1;
        }

        cumulativeDifficulty += difficulty;
        cumulativeDifficulties.push_back(cumulativeDifficulty);
        timestamps.push_back(timestamp);
        ++n;
    }

    if (!data.eof()) {
        data.clear(std::fstream::badbit);
    }

    return 0;
}
