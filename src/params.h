/*
  Clover is a UCI chess playing engine authored by Luca Metehau.
  <https://github.com/lucametehau/CloverEngine>

  Clover is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clover is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <cassert>
#include <iostream>
#include <type_traits>
#include <vector>

// tuning param/option
template <typename T> struct Parameter
{
    std::string name;
    T &value;
    T min, max;
};

inline std::vector<Parameter<int>> params_int;
inline std::vector<Parameter<double>> params_double;

// trick to be able to create options
template <typename T> struct CreateParam
{
    T value;
    CreateParam(std::string name, T _value, T min, T max) : value(_value)
    {
        assert(min <= _value && _value <= max);
        if constexpr (std::is_integral_v<T>)
            params_int.push_back({name, value, min, max});
        else
            params_double.push_back({name, value, min, max});
    }

    operator T() const
    {
        return value;
    }
};

#ifndef TUNE_FLAG
#define TUNE_PARAM(name, value, min, max) constexpr int name = value;
#define TUNE_PARAM_DOUBLE(name, value, min, max) constexpr double name = value;
#else
#define TUNE_PARAM(name, value, min, max) CreateParam<int> name(#name, value, min, max);
#define TUNE_PARAM_DOUBLE(name, value, min, max) CreateParam<double> name(#name, value, min, max);
#endif

// search constants
TUNE_PARAM(DeltaPruningMargin, 1031, 900, 1100);
TUNE_PARAM(QuiesceFutilityBias, 196, 150, 250);

TUNE_PARAM(NegativeImprovingMargin, -153, -300, -100);

TUNE_PARAM(EvalHistCoef, 10, 0, 20);
TUNE_PARAM(EvalHistMin, -1000, -2000, 0);
TUNE_PARAM(EvalHistMax, 1000, 0, 2000);
TUNE_PARAM(EvalHistMargin, 0, -500, 500);

TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2);
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2);

TUNE_PARAM(RazoringDepth, 2, 1, 5);
TUNE_PARAM(RazoringMargin, 158, 100, 200);

TUNE_PARAM(SNMPDepth, 13, 8, 15);
TUNE_PARAM(SNMPMargin, 88, 75, 100);
TUNE_PARAM(SNMPImproving, 18, 10, 25);
TUNE_PARAM(SNMPImprovingAfterMove, 30, 1, 60);
TUNE_PARAM(SNMPCutNode, 30, 1, 60);

TUNE_PARAM(NMPEvalMargin, 32, 20, 40);
TUNE_PARAM(NMPReduction, 4, 3, 5);
TUNE_PARAM(NMPDepthDiv, 3, 3, 5);
TUNE_PARAM(NMPEvalDiv, 136, 120, 150);

TUNE_PARAM(ProbcutMargin, 176, 100, 300);
TUNE_PARAM(ProbcutDepth, 4, 3, 7);
TUNE_PARAM(ProbcutReduction, 5, 3, 5);

TUNE_PARAM(PVSSeeDepthCoef, 17, 10, 20);

TUNE_PARAM(MoveloopHistDiv, 14272, 8192, 17000);

TUNE_PARAM(FPDepth, 10, 5, 10);
TUNE_PARAM(FPBias, 96, 90, 120);
TUNE_PARAM(FPMargin, 98, 90, 120);

TUNE_PARAM(LMPDepth, 7, 5, 10);
TUNE_PARAM(LMPBias, 1, -1, 5);

TUNE_PARAM(HistoryPruningDepth, 2, 2, 5);
TUNE_PARAM(HistoryPruningMargin, 4501, 2048, 6144);

TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningQuietMargin, 70, 60, 90);
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningNoisyMargin, 13, 5, 25);
TUNE_PARAM(SEENoisyHistDiv, 271, 128, 512);
TUNE_PARAM(FPNoisyDepth, 8, 5, 10);

TUNE_PARAM(SEDepth, 5, 4, 8);
TUNE_PARAM(SEMargin, 45, 32, 64);
TUNE_PARAM(SEWasPVMargin, 52, 32, 96);
TUNE_PARAM(SEDoubleExtensionsMargin, 10, 0, 16);
TUNE_PARAM(SETripleExtensionsMargin, 52, 50, 100);

constexpr int LMRGrain = 1024;

TUNE_PARAM(LMRWasNotPV, 1064, 512, 2048);
TUNE_PARAM(LMRImprovingM1, 2092, 1024, 3072);
TUNE_PARAM(LMRImproving0, 1163, 512, 2048);

TUNE_PARAM(LMRPVNode, 907, 512, 2048);
TUNE_PARAM(LMRGoodEval, 1075, 512, 2048);
TUNE_PARAM(LMREvalDifference, 1045, 512, 2048);
TUNE_PARAM(LMRRefutation, 2147, 1024, 3072);
TUNE_PARAM(LMRGivesCheck, 1012, 512, 2048);

TUNE_PARAM(LMRNoisyNotImproving, 941, 412, 2048);
TUNE_PARAM(LMRBadNoisy, 1078, 512, 2048);

TUNE_PARAM(LMRCutNode, 2253, 1024, 3072);
TUNE_PARAM(LMRWasPVHighDepth, 957, 512, 2048);
TUNE_PARAM(LMRTTMoveNoisy, 1184, 512, 2048);
TUNE_PARAM(LMRBadStaticEval, 1031, 512, 2048);
TUNE_PARAM(LMRFailLowPV, 982, 512, 2048);
TUNE_PARAM(LMRFailLowPVHighDepth, 1089, 512, 2048);

TUNE_PARAM(EvalDifferenceReductionMargin, 202, 100, 300);
TUNE_PARAM(HistReductionDiv, 8509, 7000, 10000);
TUNE_PARAM(CapHistReductionDiv, 4090, 2048, 8192);
TUNE_PARAM(LMRBadStaticEvalMargin, 244, 150, 300);
TUNE_PARAM(LMRCorrectionDivisor, 53, 25, 75);
TUNE_PARAM(DeeperMargin, 80, 40, 80);

TUNE_PARAM(RootSeeDepthCoef, 8, 5, 20);

TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10);
TUNE_PARAM(AspirationWindosValue, 7, 5, 20);
TUNE_PARAM(AspirationWindowExpandMargin, 24, 10, 100);
TUNE_PARAM(AspirationWindowExpandBias, 1, 0, 10);

TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedMaxPercentage, 1.858202760568546, 1.400, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedCoef, 1.0638501544601164, 0.500, 2.000);

TUNE_PARAM_DOUBLE(TimeManagerBestMoveMax, 1.2285085797971442, 1.100, 1.500);
TUNE_PARAM_DOUBLE(TimeManagerbestMoveStep, 0.07547058646143265, 0.010, 0.100);

TUNE_PARAM_DOUBLE(TimeManagerScoreMin, 0.46958865843889463, 0.400, 0.900);
TUNE_PARAM_DOUBLE(TimeManagerScoreMax, 1.6124152349918068, 1.100, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerScoreBias, 1.058150881442622, 0.700, 1.500);
TUNE_PARAM(TimeManagerScoreDiv, 117, 90, 150);

TUNE_PARAM_DOUBLE(TMCoef1, 0.05117403122729696, 0.035, 0.075);
TUNE_PARAM_DOUBLE(TMCoef2, 0.34772368050337443, 0.250, 0.550);
TUNE_PARAM_DOUBLE(TMCoef3, 5.5483805603934195, 4.500, 6.500);
TUNE_PARAM_DOUBLE(TMCoef4, 0.7453397854308839, 0.650, 0.850);

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.1454140230965018, 1.00, 1.25);
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.586525522389304, 1.50, 2.70);

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20);
TUNE_PARAM(QuietHistCoef, 1, 1, 4);
TUNE_PARAM(QuietContHist1, 2, 1, 2);
TUNE_PARAM(QuietContHist2, 2, 1, 2);
TUNE_PARAM(QuietContHist4, 1, 1, 2);
TUNE_PARAM(QuietPawnAttackedCoef, 34, 20, 50);
TUNE_PARAM(QuietPawnAttackedDodgeCoef, 36, 20, 50);
TUNE_PARAM(QuietPawnPushBonus, 9027, 8000, 9500);
TUNE_PARAM(QuietKingRingAttackBonus, 3509, 3000, 4000);
TUNE_PARAM(KPMoveBonus, 9114, 5000, 20000);

// eval constants
TUNE_PARAM(EvalScaleBias, 720, 600, 800);
TUNE_PARAM(EvalShuffleCoef, 5, 1, 10);

// history constants
TUNE_PARAM(HistoryBonusMargin, 269, 250, 500);
TUNE_PARAM(HistoryBonusBias, 330, 200, 600);
TUNE_PARAM(HistoryBonusMax, 2596, 1800, 3000);
TUNE_PARAM(HistoryUpdateMinDepth, 4, 2, 6);
TUNE_PARAM(HistoryMalusMargin, 327, 250, 500);
TUNE_PARAM(HistoryMalusBias, 337, 200, 600);
TUNE_PARAM(HistoryMalusMax, 2564, 1800, 3000);

// universal constants
TUNE_PARAM(SeeValPawn, 92, 80, 120);
TUNE_PARAM(SeeValKnight, 308, 280, 350);
TUNE_PARAM(SeeValBishop, 334, 320, 380);
TUNE_PARAM(SeeValRook, 507, 450, 600);
TUNE_PARAM(SeeValQueen, 990, 900, 1100);

// LMR weights
TUNE_PARAM(lsw0, -126, -4096, 4096)
TUNE_PARAM(lsw1, 895, -4096, 4096)
TUNE_PARAM(lsw2, -84, -4096, 4096)
TUNE_PARAM(lsw3, -6, -4096, 4096)
TUNE_PARAM(lsw4, -143, -4096, 4096)
TUNE_PARAM(lsw5, 1901, -4096, 4096)
TUNE_PARAM(lsw6, 1504, -4096, 4096)
TUNE_PARAM(lsw7, -130, -4096, 4096)
TUNE_PARAM(lsw8, -407, -4096, 4096)
TUNE_PARAM(lsw9, 88, -4096, 4096)
TUNE_PARAM(lsw10, 135, -4096, 4096)
TUNE_PARAM(lsw11, -16, -4096, 4096)
TUNE_PARAM(lsw12, -267, -4096, 4096)
TUNE_PARAM(lsw13, 35, -4096, 4096)
TUNE_PARAM(lsw14, 1298, -4096, 4096)
TUNE_PARAM(ldw0, -585, -4096, 4096)
TUNE_PARAM(ldw1, 96, -4096, 4096)
TUNE_PARAM(ldw2, -213, -4096, 4096)
TUNE_PARAM(ldw3, -643, -4096, 4096)
TUNE_PARAM(ldw4, -128, -4096, 4096)
TUNE_PARAM(ldw5, 440, -4096, 4096)
TUNE_PARAM(ldw6, -725, -4096, 4096)
TUNE_PARAM(ldw7, 1332, -4096, 4096)
TUNE_PARAM(ldw8, 8, -4096, 4096)
TUNE_PARAM(ldw9, 98, -4096, 4096)
TUNE_PARAM(ldw10, 36, -4096, 4096)
TUNE_PARAM(ldw11, -212, -4096, 4096)
TUNE_PARAM(ldw12, -13, -4096, 4096)
TUNE_PARAM(ldw13, -252, -4096, 4096)
TUNE_PARAM(ldw14, 1877, -4096, 4096)
TUNE_PARAM(ldw15, 945, -4096, 4096)
TUNE_PARAM(ldw16, -605, -4096, 4096)
TUNE_PARAM(ldw17, -334, -4096, 4096)
TUNE_PARAM(ldw18, -173, -4096, 4096)
TUNE_PARAM(ldw19, -4, -4096, 4096)
TUNE_PARAM(ldw20, -107, -4096, 4096)
TUNE_PARAM(ldw21, 779, -4096, 4096)
TUNE_PARAM(ldw22, 1380, -4096, 4096)
TUNE_PARAM(ldw23, -1741, -4096, 4096)
TUNE_PARAM(ldw24, -1250, -4096, 4096)
TUNE_PARAM(ldw25, 165, -4096, 4096)
TUNE_PARAM(ldw26, 107, -4096, 4096)
TUNE_PARAM(ldw27, -34, -4096, 4096)
TUNE_PARAM(ldw28, 94, -4096, 4096)
TUNE_PARAM(ldw29, -17, -4096, 4096)
TUNE_PARAM(ldw30, 104, -4096, 4096)
TUNE_PARAM(ldw31, 258, -4096, 4096)
TUNE_PARAM(ldw32, -98, -4096, 4096)
TUNE_PARAM(ldw33, -301, -4096, 4096)
TUNE_PARAM(ldw34, 327, -4096, 4096)
TUNE_PARAM(ldw35, -63, -4096, 4096)
TUNE_PARAM(ldw36, -412, -4096, 4096)
TUNE_PARAM(ldw37, 526, -4096, 4096)
TUNE_PARAM(ldw38, -100, -4096, 4096)
TUNE_PARAM(ldw39, 272, -4096, 4096)
TUNE_PARAM(ldw40, -44, -4096, 4096)
TUNE_PARAM(ldw41, 146, -4096, 4096)
TUNE_PARAM(ldw42, -247, -4096, 4096)
TUNE_PARAM(ldw43, 22, -4096, 4096)
TUNE_PARAM(ldw44, -243, -4096, 4096)
TUNE_PARAM(ldw45, 209, -4096, 4096)
TUNE_PARAM(ldw46, -349, -4096, 4096)
TUNE_PARAM(ldw47, -222, -4096, 4096)
TUNE_PARAM(ldw48, -183, -4096, 4096)
TUNE_PARAM(ldw49, 22, -4096, 4096)
TUNE_PARAM(ldw50, -450, -4096, 4096)
TUNE_PARAM(ldw51, -195, -4096, 4096)
TUNE_PARAM(ldw52, -68, -4096, 4096)
TUNE_PARAM(ldw53, 85, -4096, 4096)
TUNE_PARAM(ldw54, 143, -4096, 4096)
TUNE_PARAM(ldw55, 37, -4096, 4096)
TUNE_PARAM(ldw56, 190, -4096, 4096)
TUNE_PARAM(ldw57, 281, -4096, 4096)
TUNE_PARAM(ldw58, -115, -4096, 4096)
TUNE_PARAM(ldw59, 154, -4096, 4096)
TUNE_PARAM(ldw60, -277, -4096, 4096)
TUNE_PARAM(ldw61, -242, -4096, 4096)
TUNE_PARAM(ldw62, 22, -4096, 4096)
TUNE_PARAM(ldw63, -134, -4096, 4096)
TUNE_PARAM(ldw64, -276, -4096, 4096)
TUNE_PARAM(ldw65, 298, -4096, 4096)
TUNE_PARAM(ldw66, 187, -4096, 4096)
TUNE_PARAM(ldw67, 63, -4096, 4096)
TUNE_PARAM(ldw68, -79, -4096, 4096)
TUNE_PARAM(ldw69, -102, -4096, 4096)
TUNE_PARAM(ldw70, -173, -4096, 4096)
TUNE_PARAM(ldw71, -154, -4096, 4096)
TUNE_PARAM(ldw72, 191, -4096, 4096)
TUNE_PARAM(ldw73, -200, -4096, 4096)
TUNE_PARAM(ldw74, 61, -4096, 4096)
TUNE_PARAM(ldw75, 226, -4096, 4096)
TUNE_PARAM(ldw76, 278, -4096, 4096)
TUNE_PARAM(ldw77, 192, -4096, 4096)
TUNE_PARAM(ldw78, 419, -4096, 4096)
TUNE_PARAM(ldw79, 118, -4096, 4096)
TUNE_PARAM(ldw80, 19, -4096, 4096)
TUNE_PARAM(ldw81, -22, -4096, 4096)
TUNE_PARAM(ldw82, -459, -4096, 4096)
TUNE_PARAM(ldw83, 156, -4096, 4096)
TUNE_PARAM(ldw84, -387, -4096, 4096)
TUNE_PARAM(ldw85, -66, -4096, 4096)
TUNE_PARAM(ldw86, -218, -4096, 4096)
TUNE_PARAM(ldw87, -64, -4096, 4096)
TUNE_PARAM(ldw88, 216, -4096, 4096)
TUNE_PARAM(ldw89, -2, -4096, 4096)
TUNE_PARAM(ldw90, 121, -4096, 4096)
TUNE_PARAM(ldw91, -82, -4096, 4096)
TUNE_PARAM(ldw92, 77, -4096, 4096)
TUNE_PARAM(ldw93, -215, -4096, 4096)
TUNE_PARAM(ldw94, 131, -4096, 4096)
TUNE_PARAM(ldw95, -389, -4096, 4096)
TUNE_PARAM(ldw96, 4, -4096, 4096)
TUNE_PARAM(ldw97, 166, -4096, 4096)
TUNE_PARAM(ldw98, -629, -4096, 4096)
TUNE_PARAM(ldw99, -35, -4096, 4096)
TUNE_PARAM(ldw100, 109, -4096, 4096)
TUNE_PARAM(ldw101, 322, -4096, 4096)
TUNE_PARAM(ldw102, -129, -4096, 4096)
TUNE_PARAM(ldw103, 287, -4096, 4096)
TUNE_PARAM(ldw104, -210, -4096, 4096)

inline void print_params_for_ob()
{
    for (auto &param : params_int)
    {
        std::cout << param.name << ", int, " << param.value << ", " << param.min << ", " << param.max << ", "
                  << std::max(0.5, (param.max - param.min) / 20.0) << ", 0.002\n";
    }
    for (auto &param : params_double)
    {
        std::cout << param.name << ", float, " << param.value << ", " << param.min << ", " << param.max << ", "
                  << (param.max - param.min) / 20.0 << ", 0.002\n";
    }
}