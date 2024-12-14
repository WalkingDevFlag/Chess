

#ifndef ALTAIR_EVALUATION_CONSTANTS_H
#define ALTAIR_EVALUATION_CONSTANTS_H

#include "evaluation.h"
#include "types.h"

constexpr int GAME_PHASE_SCORES[6] = {0, 1, 1, 2, 4, 0};

constexpr int MVV_LVA_VALUES[6] = {  120, 430, 450, 620, 1400,   0};

constexpr SCORE_TYPE CANONICAL_PIECE_VALUES[6] = {100, 310, 340, 500, 900, 0};

constexpr SCORE_TYPE MAX_MINOR_PIECE_VALUE = CANONICAL_PIECE_VALUES[BISHOP];
constexpr SCORE_TYPE MIN_MINOR_PIECE_VALUE = CANONICAL_PIECE_VALUES[KNIGHT];




constexpr SCORE_TYPE PIECE_VALUES[6] = {S(   114,    84), S(   395,   218), S(   403,   199), S(   662,   312), S(  1566,   413), S(     0,     0)};

constexpr SCORE_TYPE PIECE_SQUARE_TABLES[6][64] = {
        {
                S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0),
                S(   155,   -18), S(    86,    25), S(   115,    30), S(   119,    10), S(    32,    28), S(    -9,    80), S(   -36,    64), S(    70,    28),
                S(   -31,    14), S(   -21,    15), S(    -8,     7), S(     8,    -7), S(    17,    -8), S(   -18,    10), S(    15,    11), S(     0,    -4),
                S(   -23,    10), S(   -20,    -4), S(   -22,    -4), S(    -5,   -16), S(    -2,   -15), S(     4,   -10), S(   -12,   -11), S(    -1,    -9),
                S(   -28,    -2), S(   -36,     0), S(   -16,   -19), S(    -9,   -16), S(   -10,   -16), S(   -13,    -6), S(   -11,    -6), S(   -25,    -5),
                S(   -25,    -5), S(   -31,   -10), S(   -22,   -13), S(   -15,   -12), S(    -2,   -12), S(    -4,   -12), S(    12,   -12), S(   -13,    -9),
                S(   -32,    -1), S(   -41,    -6), S(   -37,    -6), S(   -23,   -12), S(   -23,   -11), S(     2,   -12), S(     6,   -14), S(   -18,    -7),
                S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)
        },
        {
                S(  -193,    62), S(  -134,    52), S(    -3,   -11), S(   -19,    25), S(    82,     1), S(   111,     2), S(    -2,    32), S(   -27,     7),
                S(   -33,    19), S(    -3,    24), S(     2,     2), S(    33,    10), S(    52,     1), S(    40,    -8), S(   -61,    26), S(    -4,    43),
                S(   -24,    16), S(    -7,    10), S(    23,    15), S(    35,    12), S(    66,     2), S(    97,     5), S(    23,    12), S(    -6,    15),
                S(     4,    11), S(    15,     0), S(    35,    13), S(    35,    14), S(    20,    18), S(    24,    18), S(    22,    -3), S(    22,     7),
                S(    -7,     7), S(     3,     5), S(    22,     9), S(    19,    11), S(    16,    11), S(    11,     3), S(    16,    -7), S(    -8,    18),
                S(   -17,   -21), S(     1,   -14), S(     0,   -11), S(     9,    -2), S(    17,   -15), S(     1,   -32), S(   -10,   -21), S(   -18,   -13),
                S(   -35,    -5), S(   -32,     2), S(   -12,   -39), S(     3,   -21), S(    -6,   -18), S(     6,   -38), S(   -21,     0), S(   -22,   -29),
                S(   -53,   -39), S(    -7,   -22), S(   -29,   -25), S(    14,   -27), S(    -3,   -30), S(    -9,   -35), S(   -13,   -25), S(   -64,   -27)
        },
        {
                S(   -25,     7), S(   -47,    13), S(   -68,    15), S(   -10,     9), S(   -31,    10), S(   -12,     4), S(    64,    -6), S(   -11,     0),
                S(     2,     0), S(    27,     0), S(    12,     5), S(   -10,    11), S(   -18,    11), S(   -13,    11), S(    -6,     0), S(     8,    -5),
                S(    16,     5), S(    22,     4), S(    44,    -3), S(    14,     3), S(    41,     5), S(    33,     9), S(    23,     7), S(    16,     5),
                S(     9,     4), S(    19,     8), S(    24,    -1), S(    53,    -7), S(     9,     6), S(    16,     8), S(    -3,    13), S(   -10,     8),
                S(    -5,     1), S(    12,     0), S(    11,     3), S(    18,     5), S(    12,     0), S(   -11,    10), S(    -2,     3), S(     0,    -3),
                S(   -10,    -2), S(     5,     4), S(     2,    -7), S(    -1,     0), S(    -4,     8), S(     1,    -4), S(    -6,    -7), S(   -10,    -1),
                S(    -1,   -14), S(    -7,   -17), S(   -16,    -6), S(   -14,    -1), S(    -8,     0), S(    10,   -13), S(    11,   -12), S(   -12,   -35),
                S(   -24,   -30), S(   -15,   -10), S(   -23,     1), S(   -35,    -2), S(   -12,   -17), S(   -24,     6), S(   -17,    -3), S(   -13,   -16)
        },
        {
                S(    48,    -8), S(    13,    19), S(    40,    -2), S(    45,     4), S(    -1,    12), S(    35,     2), S(    75,    -9), S(    50,    -6),
                S(    42,    -2), S(    16,    13), S(    39,     0), S(    54,    -4), S(    34,     7), S(    48,     9), S(    86,    -6), S(    76,    -6),
                S(    28,    -5), S(    22,     4), S(     9,     1), S(    34,    -8), S(    -6,     6), S(    55,    -4), S(    52,    -4), S(    24,     0),
                S(     5,    -1), S(    15,     6), S(     1,     2), S(     9,    -3), S(     9,    -1), S(   -10,     7), S(    15,    -2), S(   -15,     7),
                S(    -5,    -3), S(   -30,    18), S(   -24,     7), S(   -22,     7), S(   -27,     4), S(   -50,    16), S(   -24,    15), S(   -48,    16),
                S(   -12,    -7), S(   -25,    14), S(   -34,     1), S(   -37,    10), S(   -39,     2), S(   -54,    12), S(   -27,     5), S(   -58,    24),
                S(    -8,   -26), S(   -27,   -11), S(   -23,    -4), S(   -29,    -3), S(   -35,    -7), S(   -40,    -5), S(   -18,   -22), S(   -82,     6),
                S(     9,   -28), S(    -4,   -19), S(   -12,   -12), S(   -19,   -12), S(   -26,   -16), S(   -34,    -2), S(   -42,    -2), S(   -41,    -6)
        },
        {
                S(   -50,    35), S(   -10,    16), S(    -9,    27), S(    -2,    13), S(   -21,    38), S(   182,   -74), S(   119,   -39), S(   123,   -45),
                S(   -37,    31), S(   -62,    44), S(   -51,    58), S(   -48,    45), S(   -48,    59), S(    13,    40), S(    37,    -7), S(    55,    -9),
                S(   -19,     6), S(   -36,    42), S(   -23,    15), S(   -34,    37), S(   -34,    44), S(     5,    26), S(    33,    16), S(   -18,    43),
                S(     7,   -12), S(   -10,    27), S(   -25,    40), S(   -34,    42), S(   -40,    42), S(   -19,    44), S(   -27,    49), S(   -16,    26),
                S(    11,   -18), S(    -8,    34), S(   -15,    30), S(   -12,    43), S(   -31,    27), S(   -18,    18), S(   -14,     3), S(   -10,     8),
                S(    18,   -10), S(    14,    -4), S(     2,    11), S(    -6,     7), S(    -3,   -15), S(    -3,    -5), S(     6,   -31), S(     0,   -13),
                S(    24,   -32), S(    19,   -13), S(    19,   -31), S(    15,   -24), S(    13,   -42), S(    28,   -89), S(     6,   -80), S(   -13,   -38),
                S(    35,   -44), S(    24,   -55), S(    15,   -35), S(    14,   -30), S(     7,   -65), S(   -19,   -47), S(   -12,   -81), S(    -6,   -99)
        },
        {
                S(  -971,  -210), S(   367,   -68), S(   216,     9), S(   136,     2), S(   138,   -10), S(   122,     7), S(  -169,    53), S(  -354,    23),
                S(    77,    -8), S(   166,    26), S(   193,    32), S(    67,    12), S(    90,    -1), S(   218,   -40), S(   133,   -10), S(   -89,     0),
                S(     3,    35), S(    76,    47), S(   154,    30), S(    42,    20), S(   -44,    24), S(   113,    -2), S(   118,    -7), S(    19,   -10),
                S(    28,    26), S(    70,    29), S(    79,    36), S(    12,    16), S(    -1,    13), S(    66,     4), S(    86,    -8), S(   -14,    -1),
                S(    10,    25), S(    51,    28), S(   127,    18), S(    56,     0), S(    95,    -9), S(   105,    -6), S(    72,    -7), S(   -29,    -3),
                S(   -15,    33), S(    60,    22), S(    89,    17), S(    70,   -15), S(    45,   -10), S(    73,    -5), S(    35,    -2), S(   -16,    -3),
                S(   -38,    34), S(   -19,    42), S(     9,    24), S(   -32,    -7), S(   -36,    -4), S(    -5,     5), S(     5,     6), S(   -18,    -6),
                S(  -223,    86), S(   -16,    29), S(   -27,    32), S(  -103,    -3), S(   -59,   -15), S(   -79,    11), S(   -25,     1), S(   -58,    -8)
        }
};


constexpr SCORE_TYPE MOBILITY_VALUES[4][28] = {
        {S(   -30,  -102), S(   -14,   -46), S(    -8,    -7), S(    -4,    13), S(     1,    23), S(     5,    34), S(     9,    34), S(    15,    32), S(    26,    20), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)},
        {S(   -30,   -51), S(   -28,   -37), S(   -20,   -25), S(   -16,    -8), S(   -12,     2), S(    -9,     7), S(    -6,    10), S(    -3,    13), S(     1,    14), S(     7,    14), S(    14,    13), S(    26,    13), S(     4,    31), S(    71,     4), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)},
        {S(   -40,   -68), S(   -33,   -25), S(   -32,   -11), S(   -28,     0), S(   -26,     9), S(   -23,    15), S(   -17,    14), S(   -10,    12), S(    -5,    19), S(     1,    19), S(    11,    16), S(    13,    13), S(    19,    14), S(    38,     4), S(   132,   -33), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)},
        {S(   -86,  -153), S(   -86,  -155), S(   -92,   -38), S(   -92,   -36), S(   -94,    17), S(   -92,    22), S(   -91,    45), S(   -88,    55), S(   -86,    66), S(   -83,    71), S(   -84,    81), S(   -82,    84), S(   -80,    86), S(   -74,    86), S(   -76,    90), S(   -71,    91), S(   -70,    91), S(   -52,    78), S(   -32,    62), S(    -9,    50), S(     6,    34), S(   109,   -34), S(   103,   -32), S(   284,  -135), S(   263,  -119), S(   388,  -192), S(   125,   -70), S(   243,  -145)}
};


constexpr SCORE_TYPE PASSED_PAWN_BONUSES[3][8] = {
        {S(     0,     0), S(   -65,    34), S(   -59,    29), S(   -44,    31), S(   -10,    29), S(   112,    13), S(   134,    75), S(     0,     0)},
        {S(     0,     0), S(     0,     0), S(   -54,    40), S(   -43,    38), S(     4,    50), S(   227,    42), S(   311,    83), S(     0,     0)},
        {S(     0,     0), S(     0,     0), S(   -79,    54), S(     0,     2), S(    47,    23), S(   379,   -17), S(    86,   -65), S(     0,     0)}
};


constexpr SCORE_TYPE PASSED_PAWN_BLOCKERS[6][8] = {
        {S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)},
        {S(     0,     0), S(     0,     0), S(  -107,    36), S(    -2,     5), S(   -36,    -6), S(   -31,   -12), S(   -35,     2), S(  -222,     7)},
        {S(     0,     0), S(     0,     0), S(    18,   -64), S(     8,   -35), S(    17,   -44), S(    -9,   -61), S(   -20,   -80), S(    36,  -126)},
        {S(     0,     0), S(     0,     0), S(    65,   -23), S(    -9,     6), S(     4,    16), S(    -6,    33), S(   -42,    23), S(  -155,    44)},
        {S(     0,     0), S(     0,     0), S(    34,    -1), S(    -8,    22), S(    -6,     8), S(    -9,    41), S(   -21,    85), S(    -8,   111)},
        {S(     0,     0), S(     0,     0), S(   379,   -45), S(   129,   -34), S(    83,   -32), S(    73,   -38), S(    -9,   -40), S(   -47,  -104)}
};


constexpr SCORE_TYPE PASSED_PAWN_BLOCKERS_2[6][8] = {
        {S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0), S(     0,     0)},
        {S(     0,     0), S(     0,     0), S(     0,     0), S(   -10,    12), S(    -7,     6), S(   -20,     8), S(    -9,    16), S(     6,    68)},
        {S(     0,     0), S(     0,     0), S(     0,     0), S(   -51,    23), S(   -10,   -18), S(     0,   -14), S(   -19,    -3), S(    -8,   -10)},
        {S(     0,     0), S(     0,     0), S(     0,     0), S(   -18,   -22), S(    17,    -4), S(    11,    11), S(    20,    20), S(   -46,    25)},
        {S(     0,     0), S(     0,     0), S(     0,     0), S(     6,   -23), S(   -12,    15), S(     1,    -6), S(     3,     8), S(   -52,   120)},
        {S(     0,     0), S(     0,     0), S(     0,     0), S(    10,   -30), S(    28,   -16), S(     1,   -34), S(    35,   -37), S(    11,   -57)}
};


constexpr SCORE_TYPE PHALANX_PAWN_BONUSES[8] = {S(     0,     0), S(     2,    -5), S(    -3,    -5), S(    14,    14), S(    45,    55), S(    47,   153), S(   232,   153), S(     0,     0)};

constexpr SCORE_TYPE ISOLATED_PAWN_PENALTY = S(   -19,    -9);

constexpr SCORE_TYPE BISHOP_PAIR_BONUS = S(    33,    38);

constexpr SCORE_TYPE TEMPO_BONUS = S(    11,     4);

constexpr SCORE_TYPE SEMI_OPEN_FILE_VALUES[6] = {S(     0,     0), S(     0,     0), S(     0,     0), S(    18,     4), S(     3,     8), S(    -9,     6)};

constexpr SCORE_TYPE OPEN_FILE_VALUES[6] = {S(     0,     0), S(     0,     0), S(     0,     0), S(    25,    -2), S(    -4,    10), S(   -37,     3)};

constexpr SCORE_TYPE PIECE_THREATS[6][6] = {
        {S(    17,   -44), S(    45,    40), S(    43,    51), S(    54,    36), S(    35,    10), S(     0,     0)},
        {S(    -7,    11), S(     0,     0), S(    29,    14), S(    56,     3), S(    19,    32), S(     0,     0)},
        {S(     1,    17), S(    17,    34), S(    12,    61), S(    55,    12), S(    39,    60), S(     0,     0)},
        {S(    -9,    21), S(     0,    22), S(     6,    13), S(    10,   -25), S(    43,    20), S(     0,     0)},
        {S(     2,     3), S(     1,     1), S(    -2,    22), S(    -4,     7), S(   -16,    -5), S(   168,  -140)},
        {S(    70,    15), S(     5,     6), S(    60,     0), S(    25,    -2), S(     0,     0), S(     0,     0)}
};


constexpr SCORE_TYPE KING_RING_ATTACKS[2][6] = {
        {S(    18,    -7), S(    16,   -10), S(    25,    -2), S(    12,    -2), S(    18,     0), S(     0,     0)},
        {S(    13,    -5), S(    15,    -4), S(    14,    -2), S(     8,    -1), S(    11,     0), S(     0,     0)}
};


constexpr SCORE_TYPE TOTAL_KING_RING_ATTACKS[40] = {S(    31,     6), S(    16,   -15), S(     2,   -11), S(    -6,   -10), S(   -17,    -7), S(   -23,    -2), S(   -34,     1), S(   -38,     1), S(   -34,     1), S(   -35,    -1), S(   -33,    -2), S(   -29,    -2), S(   -24,    -2), S(   -17,    -2), S(   -14,     0), S(   -20,     7), S(   -12,     6), S(   -15,    12), S(    -2,     8), S(    -2,    13), S(   -28,    25), S(   -12,    22), S(    -5,    13), S(     6,    17), S(    35,    14), S(     8,    40), S(    15,    41), S(    75,     5), S(    77,    16), S(    61,    -9), S(    41,   -11), S(   125,    26), S(    26,   -14), S(   187,   -47), S(   104,   -36), S(    24,    71), S(  -363,  -267), S(   156,    77), S(   177,   188), S(  -105,  -133)};

constexpr SCORE_TYPE KING_PAWN_SHIELD[5][8] = {
        {S(     7,     7), S(    27,    -7), S(    18,   -20), S(     0,     0), S(     0,     0), S(     2,     1), S(    13,    -1), S(    -2,    -6)},
        {S(     3,    -5), S(    28,    -9), S(     9,    -7), S(     0,     0), S(     0,     0), S(    -3,    -9), S(    -5,    -6), S(     1,    -5)},
        {S(     1,     1), S(   -13,    -5), S(     1,     3), S(     0,     0), S(     0,     0), S(    -3,    -8), S(   -10,    -9), S(     8,    -8)},
        {S(    -5,    -1), S(     7,     1), S(    -6,     6), S(     0,     0), S(     0,     0), S(     7,    -3), S(    -6,     0), S(    20,    -6)},
        {S(    -9,    -1), S(   -28,     1), S(   -20,    -2), S(     0,     0), S(     0,     0), S(   -14,     7), S(   -14,     7), S(   -28,    11)}
};


constexpr SCORE_TYPE KING_PAWN_STORM[6][8] = {
        {S(    77,   -23), S(    72,    11), S(    89,     9), S(     0,     0), S(     0,     0), S(    51,    45), S(    50,    22), S(    44,    28)},
        {S(   -52,    13), S(   -48,    14), S(   -41,     8), S(     0,     0), S(     0,     0), S(   -64,    11), S(   -40,    19), S(     7,     5)},
        {S(     5,     8), S(    31,   -12), S(   -61,    13), S(     0,     0), S(     0,     0), S(     3,     9), S(    -1,    -3), S(    29,    -2)},
        {S(    -7,    -6), S(    -2,    -6), S(   -13,    -7), S(     0,     0), S(     0,     0), S(    -5,     1), S(    -3,     0), S(   -10,     6)},
        {S(   -25,    -3), S(   -15,    -9), S(   -10,    -5), S(     0,     0), S(     0,     0), S(   -10,    -5), S(    -4,    -3), S(    -9,     1)},
        {S(   -10,   -10), S(   -28,    -5), S(     1,    -8), S(     0,     0), S(     0,     0), S(     9,    -5), S(    -6,    -2), S(   -12,    -5)}
};


constexpr SCORE_TYPE OPP_KING_TROPISM[6] = {S(     0,     0), S(    -1,    -2), S(     2,    -1), S(    -7,     4), S(    -5,    -3), S(     0,     0)};

constexpr SCORE_TYPE OUR_KING_TROPISM[6] = {S(     0,     0), S(     1,    -4), S(    -4,     1), S(    -6,     3), S(    -1,     2), S(     0,     0)};

constexpr SCORE_TYPE DOUBLED_PAWN_PENALTY = S(   -28,   -24);

constexpr SCORE_TYPE SQUARE_OF_THE_PAWN = S(     7,    -1);

constexpr SCORE_TYPE BACKWARDS_PAWN_PENALTY[2] = {S(   -10,    -2), S(   -15,   -21)};

constexpr SCORE_TYPE PASSED_OUR_DISTANCE[8] = {S(     0,     0), S(     1,     0), S(     2,    -2), S(     5,    -6), S(    -1,    -6), S(   -15,    -4), S(    -2,    -9), S(     0,     0)};

constexpr SCORE_TYPE PASSED_OPP_DISTANCE[8] = {S(     0,     0), S(     9,    -1), S(     7,     0), S(     4,     6), S(     8,    13), S(     9,    21), S(     3,    11), S(     0,     0)};



#endif //ALTAIR_EVALUATION_CONSTANTS_H
