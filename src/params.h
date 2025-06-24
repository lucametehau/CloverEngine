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

TUNE_PARAM(LMRNoisyNotImproving, 1051, 412, 2048);
TUNE_PARAM(LMRBadNoisy, 1089, 512, 2048);

TUNE_PARAM(LMRCutNode, 2253, 1024, 3072);
TUNE_PARAM(LMRWasPVHighDepth, 957, 512, 2048);
TUNE_PARAM(LMRTTMoveNoisy, 1184, 512, 2048);
TUNE_PARAM(LMRBadStaticEval, 1031, 512, 2048);
TUNE_PARAM(LMRFailLowPV, 982, 512, 2048);
TUNE_PARAM(LMRFailLowPVHighDepth, 1170, 512, 2048);

TUNE_PARAM(EvalDifferenceReductionMargin, 202, 100, 300);
TUNE_PARAM(HistReductionDiv, 8566, 7000, 10000);
TUNE_PARAM(CapHistReductionDiv, 4090, 2048, 8192);
TUNE_PARAM(LMRBadStaticEvalMargin, 244, 150, 300);
TUNE_PARAM(LMRCorrectionDivisor, 52, 25, 75);
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

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.150659787479384, 1.00, 1.25);
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.5463352376290795, 1.50, 2.70);

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
TUNE_PARAM(lsw0, 174, -4096, 4096)
TUNE_PARAM(lsw1, 1027, -4096, 4096)
TUNE_PARAM(lsw2, 138, -4096, 4096)
TUNE_PARAM(lsw3, 113, -4096, 4096)
TUNE_PARAM(lsw4, 273, -4096, 4096)
TUNE_PARAM(lsw5, 2238, -4096, 4096)
TUNE_PARAM(lsw6, 1151, -4096, 4096)
TUNE_PARAM(lsw7, 98, -4096, 4096)
TUNE_PARAM(lsw8, -169, -4096, 4096)
TUNE_PARAM(lsw9, 209, -4096, 4096)
TUNE_PARAM(lsw10, 15, -4096, 4096)
TUNE_PARAM(lsw11, -17, -4096, 4096)
TUNE_PARAM(lsw12, -358, -4096, 4096)
TUNE_PARAM(lsw13, 246, -4096, 4096)
TUNE_PARAM(lsw14, 1041, -4096, 4096)
TUNE_PARAM(ldw0, -718, -4096, 4096)
TUNE_PARAM(ldw1, -18, -4096, 4096)
TUNE_PARAM(ldw2, 26, -4096, 4096)
TUNE_PARAM(ldw3, -112, -4096, 4096)
TUNE_PARAM(ldw4, -167, -4096, 4096)
TUNE_PARAM(ldw5, 188, -4096, 4096)
TUNE_PARAM(ldw6, -1099, -4096, 4096)
TUNE_PARAM(ldw7, 909, -4096, 4096)
TUNE_PARAM(ldw8, -229, -4096, 4096)
TUNE_PARAM(ldw9, -27, -4096, 4096)
TUNE_PARAM(ldw10, 45, -4096, 4096)
TUNE_PARAM(ldw11, 180, -4096, 4096)
TUNE_PARAM(ldw12, -155, -4096, 4096)
TUNE_PARAM(ldw13, -25, -4096, 4096)
TUNE_PARAM(ldw14, 1874, -4096, 4096)
TUNE_PARAM(ldw15, 1199, -4096, 4096)
TUNE_PARAM(ldw16, -1078, -4096, 4096)
TUNE_PARAM(ldw17, -11, -4096, 4096)
TUNE_PARAM(ldw18, -113, -4096, 4096)
TUNE_PARAM(ldw19, 470, -4096, 4096)
TUNE_PARAM(ldw20, 198, -4096, 4096)
TUNE_PARAM(ldw21, 1024, -4096, 4096)
TUNE_PARAM(ldw22, 1206, -4096, 4096)
TUNE_PARAM(ldw23, -1854, -4096, 4096)
TUNE_PARAM(ldw24, -981, -4096, 4096)
TUNE_PARAM(ldw25, 86, -4096, 4096)
TUNE_PARAM(ldw26, 304, -4096, 4096)
TUNE_PARAM(ldw27, 28, -4096, 4096)
TUNE_PARAM(ldw28, -140, -4096, 4096)
TUNE_PARAM(ldw29, -269, -4096, 4096)
TUNE_PARAM(ldw30, -116, -4096, 4096)
TUNE_PARAM(ldw31, 125, -4096, 4096)
TUNE_PARAM(ldw32, 147, -4096, 4096)
TUNE_PARAM(ldw33, 181, -4096, 4096)
TUNE_PARAM(ldw34, 243, -4096, 4096)
TUNE_PARAM(ldw35, -457, -4096, 4096)
TUNE_PARAM(ldw36, -16, -4096, 4096)
TUNE_PARAM(ldw37, 195, -4096, 4096)
TUNE_PARAM(ldw38, 57, -4096, 4096)
TUNE_PARAM(ldw39, 199, -4096, 4096)
TUNE_PARAM(ldw40, -108, -4096, 4096)
TUNE_PARAM(ldw41, -135, -4096, 4096)
TUNE_PARAM(ldw42, 210, -4096, 4096)
TUNE_PARAM(ldw43, 278, -4096, 4096)
TUNE_PARAM(ldw44, -89, -4096, 4096)
TUNE_PARAM(ldw45, -37, -4096, 4096)
TUNE_PARAM(ldw46, -246, -4096, 4096)
TUNE_PARAM(ldw47, -10, -4096, 4096)
TUNE_PARAM(ldw48, -228, -4096, 4096)
TUNE_PARAM(ldw49, 291, -4096, 4096)
TUNE_PARAM(ldw50, 42, -4096, 4096)
TUNE_PARAM(ldw51, -13, -4096, 4096)
TUNE_PARAM(ldw52, 204, -4096, 4096)
TUNE_PARAM(ldw53, 423, -4096, 4096)
TUNE_PARAM(ldw54, 21, -4096, 4096)
TUNE_PARAM(ldw55, 140, -4096, 4096)
TUNE_PARAM(ldw56, -79, -4096, 4096)
TUNE_PARAM(ldw57, 128, -4096, 4096)
TUNE_PARAM(ldw58, -29, -4096, 4096)
TUNE_PARAM(ldw59, -162, -4096, 4096)
TUNE_PARAM(ldw60, 44, -4096, 4096)
TUNE_PARAM(ldw61, -126, -4096, 4096)
TUNE_PARAM(ldw62, 144, -4096, 4096)
TUNE_PARAM(ldw63, 293, -4096, 4096)
TUNE_PARAM(ldw64, 54, -4096, 4096)
TUNE_PARAM(ldw65, 38, -4096, 4096)
TUNE_PARAM(ldw66, -253, -4096, 4096)
TUNE_PARAM(ldw67, -14, -4096, 4096)
TUNE_PARAM(ldw68, -185, -4096, 4096)
TUNE_PARAM(ldw69, -126, -4096, 4096)
TUNE_PARAM(ldw70, -193, -4096, 4096)
TUNE_PARAM(ldw71, -49, -4096, 4096)
TUNE_PARAM(ldw72, 87, -4096, 4096)
TUNE_PARAM(ldw73, 280, -4096, 4096)
TUNE_PARAM(ldw74, -195, -4096, 4096)
TUNE_PARAM(ldw75, -73, -4096, 4096)
TUNE_PARAM(ldw76, -23, -4096, 4096)
TUNE_PARAM(ldw77, 33, -4096, 4096)
TUNE_PARAM(ldw78, -136, -4096, 4096)
TUNE_PARAM(ldw79, -220, -4096, 4096)
TUNE_PARAM(ldw80, -35, -4096, 4096)
TUNE_PARAM(ldw81, -166, -4096, 4096)
TUNE_PARAM(ldw82, 12, -4096, 4096)
TUNE_PARAM(ldw83, -166, -4096, 4096)
TUNE_PARAM(ldw84, -166, -4096, 4096)
TUNE_PARAM(ldw85, 166, -4096, 4096)
TUNE_PARAM(ldw86, 6, -4096, 4096)
TUNE_PARAM(ldw87, 22, -4096, 4096)
TUNE_PARAM(ldw88, -21, -4096, 4096)
TUNE_PARAM(ldw89, 41, -4096, 4096)
TUNE_PARAM(ldw90, -31, -4096, 4096)
TUNE_PARAM(ldw91, 188, -4096, 4096)
TUNE_PARAM(ldw92, -7, -4096, 4096)
TUNE_PARAM(ldw93, -101, -4096, 4096)
TUNE_PARAM(ldw94, 267, -4096, 4096)
TUNE_PARAM(ldw95, 26, -4096, 4096)
TUNE_PARAM(ldw96, -34, -4096, 4096)
TUNE_PARAM(ldw97, 58, -4096, 4096)
TUNE_PARAM(ldw98, -88, -4096, 4096)
TUNE_PARAM(ldw99, -19, -4096, 4096)
TUNE_PARAM(ldw100, 141, -4096, 4096)
TUNE_PARAM(ldw101, -221, -4096, 4096)
TUNE_PARAM(ldw102, -97, -4096, 4096)
TUNE_PARAM(ldw103, -34, -4096, 4096)
TUNE_PARAM(ldw104, 122, -4096, 4096)

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