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
#include <type_traits>
#include <cassert>
#include <iostream>

// tuning param/option
template <typename T>
struct Parameter {
    std::string name;
    T &value;
    T min, max;
};


inline std::vector<Parameter<int>> params_int;
inline std::vector<Parameter<double>> params_double;

// trick to be able to create options
template <typename T>
struct CreateParam {
    T value;
    CreateParam(std::string name, T _value, T min, T max) : value(_value) { 
        assert(min <= _value && _value <= max);
        if constexpr (std::is_integral_v<T>) params_int.push_back({ name, value, min, max });
        else params_double.push_back({ name, value, min, max }); 
    }

    operator T() const { return value; }
};


#ifndef TUNE_FLAG
#define TUNE_PARAM(name, value, min, max) constexpr int name = value;
#define TUNE_PARAM_DOUBLE(name, value, min, max) constexpr double name = value;
#else
#define TUNE_PARAM(name, value, min, max) CreateParam<int> name(#name, value, min, max);
#define TUNE_PARAM_DOUBLE(name, value, min, max) CreateParam<double> name(#name, value, min, max);
#endif


// search constants
TUNE_PARAM(DeltaPruningMargin, 1023, 900, 1100);
TUNE_PARAM(QuiesceFutilityBias, 201, 150, 250);

TUNE_PARAM(NegativeImprovingMargin, -159, -300, -100);

TUNE_PARAM(RazoringDepth, 2, 1, 5);
TUNE_PARAM(RazoringMargin, 157, 100, 200);

TUNE_PARAM(SNMPDepth, 13, 8, 15);
TUNE_PARAM(SNMPMargin, 88, 75, 100);
TUNE_PARAM(SNMPImproving, 19, 10, 25);

TUNE_PARAM(NMPEvalMargin, 31, 20, 40);
TUNE_PARAM(NMPReduction, 4, 3, 5);
TUNE_PARAM(NMPDepthDiv, 3, 3, 5);
TUNE_PARAM(NMPEvalDiv, 136, 120, 150);

TUNE_PARAM(ProbcutMargin, 176, 100, 300);
TUNE_PARAM(ProbcutDepth, 4, 3, 7);
TUNE_PARAM(ProbcutReduction, 5, 3, 5);

TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2);
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2);

TUNE_PARAM(PVSSeeDepthCoef, 16, 10, 20);

TUNE_PARAM(MoveloopHistDiv, 14312, 8192, 17000);

TUNE_PARAM(FPDepth, 10, 5, 10);
TUNE_PARAM(FPBias, 96, 90, 120);
TUNE_PARAM(FPMargin, 97, 90, 120);

TUNE_PARAM(LMPDepth, 7, 5, 10);
TUNE_PARAM(LMPBias, 1, -1, 5);

TUNE_PARAM(HistoryPruningDepth, 2, 2, 5);
TUNE_PARAM(HistoryPruningMargin, 4470, 2048, 6144);

TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningQuietMargin, 70, 60, 90);
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningNoisyMargin, 13, 5, 25);
TUNE_PARAM(SEENoisyHistDiv, 273, 128, 512);
TUNE_PARAM(FPNoisyDepth, 8, 5, 10);

TUNE_PARAM(SEDepth, 5, 4, 8);
TUNE_PARAM(SEMargin, 46, 32, 64);
TUNE_PARAM(SEWasPVMargin, 55, 32, 96);
TUNE_PARAM(SEDoubleExtensionsMargin, 11, 0, 16);
TUNE_PARAM(SETripleExtensionsMargin, 53, 50, 100);

TUNE_PARAM(LMRWasNotPV, 1065, 512, 2048);
TUNE_PARAM(LMRImprovingM1, 2079, 1024, 3072);
TUNE_PARAM(LMRImproving0, 1148, 512, 2048);

TUNE_PARAM(LMRPVNode, 911, 512, 2048);
TUNE_PARAM(LMRGoodEval, 1057, 512, 2048);
TUNE_PARAM(LMREvalDifference, 999, 512, 2048);
TUNE_PARAM(LMRRefutation, 2127, 1024, 3072);
TUNE_PARAM(LMRGivesCheck, 1039, 512, 2048);

TUNE_PARAM(LMRNoisyNotImproving, 995, 412, 2048);
TUNE_PARAM(LMRBadNoisy, 1053, 512, 2048);

TUNE_PARAM(LMRCutNode, 2197, 1024, 3072);
TUNE_PARAM(LMRWasPVHighDepth, 983, 512, 2048);
TUNE_PARAM(LMRTTMoveNoisy, 1143, 512, 2048);
TUNE_PARAM(LMRBadStaticEval, 1061, 512, 2048);
TUNE_PARAM(LMRFailLowPV, 959, 512, 2048);
TUNE_PARAM(LMRFailLowPVHighDepth, 1129, 512, 2048);

TUNE_PARAM(EvalDifferenceReductionMargin, 201, 100, 300);
TUNE_PARAM(HistReductionDiv, 8458, 7000, 10000);
TUNE_PARAM(CapHistReductionDiv, 3978, 2048, 8192);
TUNE_PARAM(LMRBadStaticEvalMargin, 245, 150, 300);
TUNE_PARAM(LMRCorrectionDivisor, 52, 25, 75);
TUNE_PARAM(DeeperMargin, 80, 40, 80);

TUNE_PARAM(RootSeeDepthCoef, 8, 5, 20);

TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10);
TUNE_PARAM(AspirationWindosValue, 7, 5, 20);
TUNE_PARAM(AspirationWindowExpandMargin, 24, 10, 100);
TUNE_PARAM(AspirationWindowExpandBias, 0, 0, 10);

TUNE_PARAM(TimeManagerMinDepth, 8, 5, 11);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedMaxPercentage, 1.8362629088744435, 1.400, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedCoef, 1.0863310971639488, 0.500, 2.000);

TUNE_PARAM_DOUBLE(TimeManagerBestMoveMax, 1.2322638607912109, 1.100, 1.500);
TUNE_PARAM_DOUBLE(TimeManagerbestMoveStep, 0.07127471646638137, 0.010, 0.100);

TUNE_PARAM_DOUBLE(TimeManagerScoreMin, 0.47914961933033773, 0.400, 0.900);
TUNE_PARAM_DOUBLE(TimeManagerScoreMax, 1.5984209246576317, 1.100, 2.000);
TUNE_PARAM_DOUBLE(TimeManagerScoreBias, 1.0591846911721798, 0.700, 1.500);
TUNE_PARAM(TimeManagerScoreDiv, 114, 90, 150);

TUNE_PARAM_DOUBLE(TMCoef1, 0.051019169371045434, 0.035, 0.075);
TUNE_PARAM_DOUBLE(TMCoef2, 0.3522696173376939, 0.250, 0.550);
TUNE_PARAM_DOUBLE(TMCoef3, 5.538003047210683, 4.500, 6.500);
TUNE_PARAM_DOUBLE(TMCoef4, 0.7470050629755027, 0.650, 0.850);

TUNE_PARAM_DOUBLE(LMRQuietBias,  1.1591537442175806, 1.00, 1.25);
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.575429520757855, 1.50, 2.70);

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20);
TUNE_PARAM(QuietHistCoef, 1, 1, 4);
TUNE_PARAM(QuietContHist1, 2, 1, 2);
TUNE_PARAM(QuietContHist2, 2, 1, 2);
TUNE_PARAM(QuietContHist4, 1, 1, 2);
TUNE_PARAM(QuietPawnAttackedCoef, 34, 20, 50);
TUNE_PARAM(QuietPawnAttackedDodgeCoef, 35, 20, 50);
TUNE_PARAM(QuietPawnPushBonus, 9013, 8000, 9500);
TUNE_PARAM(QuietKingRingAttackBonus, 3488, 3000, 4000);
TUNE_PARAM(KPMoveBonus, 9311, 5000, 20000);

// eval constants
TUNE_PARAM(EvalScaleBias, 718, 600, 800);
TUNE_PARAM(EvalShuffleCoef, 5, 1, 10);

// history constants
TUNE_PARAM(HistoryBonusMargin, 263, 250, 500);
TUNE_PARAM(HistoryBonusBias, 347, 200, 600);
TUNE_PARAM(HistoryBonusMax, 2568, 1800, 3000);
TUNE_PARAM(HistoryUpdateMinDepth, 4, 2, 6);
TUNE_PARAM(HistoryMalusMargin, 318, 250, 500);
TUNE_PARAM(HistoryMalusBias, 336, 200, 600);
TUNE_PARAM(HistoryMalusMax, 2535, 1800, 3000);

// universal constants
TUNE_PARAM(SeeValPawn, 93, 80, 120);
TUNE_PARAM(SeeValKnight, 309, 280, 350);
TUNE_PARAM(SeeValBishop, 331, 320, 380);
TUNE_PARAM(SeeValRook, 509, 450, 600);
TUNE_PARAM(SeeValQueen, 993, 900, 1100);

inline void print_params_for_ob() {
    for (auto& param : params_int) {
        std::cout << param.name << ", int, " << param.value << ", " << param.min << ", " << param.max << ", " << std::max(0.5, (param.max - param.min) / 20.0) << ", 0.002\n";
    }
    for (auto& param : params_double) {
        std::cout << param.name << ", float, " << param.value << ", " << param.min << ", " << param.max << ", " << (param.max - param.min) / 20.0 << ", 0.002\n";
    }
}