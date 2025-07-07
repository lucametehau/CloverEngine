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
TUNE_PARAM(DeltaPruningMargin, 1032, 900, 1100);
TUNE_PARAM(QuiesceFutilityBias, 195, 150, 250);

TUNE_PARAM(NegativeImprovingMargin, -159, -300, -100);

TUNE_PARAM(EvalHistCoef, 9, 0, 20);
TUNE_PARAM(EvalHistMin, -1039, -2000, 0);
TUNE_PARAM(EvalHistMax, 848, 0, 2000);
TUNE_PARAM(EvalHistMargin, 20, -500, 500);

TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2);
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2);

TUNE_PARAM(RazoringDepth, 2, 1, 5);
TUNE_PARAM(RazoringMargin, 152, 100, 200);

TUNE_PARAM(SNMPDepth, 13, 8, 15);
TUNE_PARAM(SNMPMargin, 89, 75, 100);
TUNE_PARAM(SNMPImproving, 18, 10, 25);
TUNE_PARAM(SNMPImprovingAfterMove, 24, 1, 60);
TUNE_PARAM(SNMPCutNode, 25, 1, 60);
TUNE_PARAM(SNMPBase, 1, 0, 50);

TUNE_PARAM(NMPEvalMargin, 32, 20, 40);
TUNE_PARAM(NMPReduction, 4, 3, 5);
TUNE_PARAM(NMPDepthDiv, 3, 3, 5);
TUNE_PARAM(NMPEvalDiv, 134, 120, 150);

TUNE_PARAM(ProbcutMargin, 182, 100, 300);
TUNE_PARAM(ProbcutDepth, 4, 3, 7);
TUNE_PARAM(ProbcutReduction, 5, 3, 5);

TUNE_PARAM(PVSSeeDepthCoef, 18, 10, 20);

TUNE_PARAM(MoveloopHistDiv, 13813, 1, 17000);

TUNE_PARAM(FPDepth, 10, 5, 15);
TUNE_PARAM(FPBias, 106, 0, 150);
TUNE_PARAM(FPMargin, 87, 0, 150);

TUNE_PARAM(LMPDepth, 7, 5, 10);
TUNE_PARAM(LMPBias, 1, -1, 5);

TUNE_PARAM(HistoryPruningDepth, 2, 2, 5);
TUNE_PARAM(HistoryPruningMargin, 4379, 2048, 6144);

TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningQuietMargin, 71, 60, 90);
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningNoisyMargin, 14, 5, 25);
TUNE_PARAM(SEENoisyHistDiv, 285, 128, 512);
TUNE_PARAM(FPNoisyDepth, 8, 5, 15);

TUNE_PARAM(FPNoisyBias, 101, 0, 150);
TUNE_PARAM(FPNoisyMargin, 86, 0, 150);

TUNE_PARAM(SEDepth, 5, 4, 8);
TUNE_PARAM(SEMargin, 43, 32, 64);
TUNE_PARAM(SEWasPVMargin, 44, 32, 96);
TUNE_PARAM(SEDoubleExtensionsMargin, 10, 0, 16);
TUNE_PARAM(SETripleExtensionsMargin, 52, 50, 100);

constexpr int LMRGrain = 1024;

TUNE_PARAM(LMRWasNotPV, 1090, 512, 2048);
TUNE_PARAM(LMRImprovingM1, 2084, 1024, 3072);
TUNE_PARAM(LMRImproving0, 1178, 512, 2048);

TUNE_PARAM(LMRPVNode, 827, 512, 2048);
TUNE_PARAM(LMRGoodEval, 1121, 512, 2048);
TUNE_PARAM(LMREvalDifference, 1028, 512, 2048);
TUNE_PARAM(LMRRefutation, 2013, 1024, 3072);
TUNE_PARAM(LMRGivesCheck, 1161, 512, 2048);

TUNE_PARAM(LMRBadNoisy, 1121, 512, 2048);

TUNE_PARAM(LMRCutNode, 2350, 1024, 3072);
TUNE_PARAM(LMRWasPVHighDepth, 977, 512, 2048);
TUNE_PARAM(LMRTTMoveNoisy, 1149, 512, 2048);
TUNE_PARAM(LMRBadStaticEval, 917, 512, 2048);
TUNE_PARAM(LMRFailLowPV, 950, 512, 2048);
TUNE_PARAM(LMRFailLowPVHighDepth, 1075, 512, 2048);

TUNE_PARAM(EvalDifferenceReductionMargin, 196, 100, 300);
TUNE_PARAM(HistReductionDiv, 8505, 7000, 10000);
TUNE_PARAM(CapHistReductionDiv, 3901, 2048, 8192);
TUNE_PARAM(LMRBadStaticEvalMargin, 234, 150, 300);
TUNE_PARAM(LMRCorrectionDivisor, 50, 25, 75);
TUNE_PARAM(DeeperMargin, 70, 0, 150);
TUNE_PARAM(PVDeeperMargin, 74, 0, 150);

TUNE_PARAM(RootSeeDepthCoef, 7, 5, 20);

TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10);
TUNE_PARAM(AspirationWindosValue, 7, 5, 20);
TUNE_PARAM(AspirationWindowsDivisor, 10613, 1, 20000);
TUNE_PARAM(AspirationWindowExpandMargin, 18, 10, 100);
TUNE_PARAM(AspirationWindowExpandBias, 0, 0, 10);

TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedMaxPercentage, 1.858202760568546, 1.400, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedCoef, 1.0638501544601164, 0.500, 2.000);

TUNE_PARAM_DOUBLE(TimeManagerBestMoveMax, 1.2285085797971442, 1.100, 1.500);
TUNE_PARAM_DOUBLE(TimeManagerbestMoveStep, 0.07547058646143265, 0.010, 0.100);

TUNE_PARAM_DOUBLE(TimeManagerScoreMin, 0.46958865843889463, 0.400, 0.900);
TUNE_PARAM_DOUBLE(TimeManagerScoreMax, 1.6124152349918068, 1.100, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerScoreBias, 1.058150881442622, 0.700, 1.500);
TUNE_PARAM(TimeManagerScoreDiv, 116, 90, 150);

TUNE_PARAM_DOUBLE(TMCoef1, 0.05117403122729696, 0.035, 0.075);
TUNE_PARAM_DOUBLE(TMCoef2, 0.34772368050337443, 0.250, 0.550);
TUNE_PARAM_DOUBLE(TMCoef3, 5.5483805603934195, 4.500, 6.500);
TUNE_PARAM_DOUBLE(TMCoef4, 0.7453397854308839, 0.650, 0.850);

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.1590765562858443, 1.00, 1.25);
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.6334838267129452, 1.50, 2.70);

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20);
TUNE_PARAM(QuietHistCoef, 1106, 1, 3072);
TUNE_PARAM(QuietContHist1, 2015, 1, 3072);
TUNE_PARAM(QuietContHist2, 2024, 1, 3072);
TUNE_PARAM(QuietContHist4, 1192, 1, 3072);
TUNE_PARAM(QuietPawnAttackedCoef, 32, 20, 50);
TUNE_PARAM(QuietPawnAttackedDodgeCoef, 35, 20, 50);
TUNE_PARAM(QuietPawnPushBonus, 8357, 1, 16000);
TUNE_PARAM(QuietKingRingAttackBonus, 3383, 1, 5000);
TUNE_PARAM(KPMoveBonus, 8470, 5000, 20000);
TUNE_PARAM(ThreatCoef1, 16699, 0, 32768);
TUNE_PARAM(ThreatCoef2, 14666, 0, 32768);
TUNE_PARAM(ThreatCoef3, 16861, 0, 32768);
TUNE_PARAM(ThreatCoef4, 16597, 0, 32768);

// eval constants
TUNE_PARAM(EvalScaleBias, 726, 600, 800);
TUNE_PARAM(EvalShuffleCoef, 4, 1, 10);

// history constants
TUNE_PARAM(HistoryBonusMargin, 265, 250, 500);
TUNE_PARAM(HistoryBonusBias, 338, 200, 600);
TUNE_PARAM(HistoryBonusMax, 2613, 1800, 3000);
TUNE_PARAM(HistoryUpdateMinDepth, 4, 2, 6);
TUNE_PARAM(HistoryMalusMargin, 335, 250, 500);
TUNE_PARAM(HistoryMalusBias, 305, 200, 600);
TUNE_PARAM(HistoryMalusMax, 2494, 1800, 3000);

TUNE_PARAM(FailLowContHistCoef, 50, 0, 256);
TUNE_PARAM(FailLowHistCoef, 79, 0, 256);

// corrhist constants
TUNE_PARAM(CorrHistPawn, 974, 0, 2048);
TUNE_PARAM(CorrHistMat, 1133, 0, 2048);
TUNE_PARAM(CorrHistCont2, 882, 0, 2048);
TUNE_PARAM(CorrHistCont3, 882, 0, 2048);
TUNE_PARAM(CorrHistCont4, 882, 0, 2048);
TUNE_PARAM(CorrHistCont5, 882, 0, 2048);
TUNE_PARAM(CorrHistCont6, 882, 0, 2048);

// universal constants
TUNE_PARAM(SeeValPawn, 93, 80, 120);
TUNE_PARAM(SeeValKnight, 310, 280, 350);
TUNE_PARAM(SeeValBishop, 335, 320, 380);
TUNE_PARAM(SeeValRook, 514, 450, 600);
TUNE_PARAM(SeeValQueen, 984, 900, 1100);

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