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
TUNE_PARAM(DeltaPruningMargin, 1038, 900, 1100)
TUNE_PARAM(QuiesceFutilityBias, 192, 150, 250)
TUNE_PARAM(QuiesceSEEMargin, -12, -100, 100)

TUNE_PARAM(NegativeImprovingMargin, -150, -300, -100)

TUNE_PARAM(EvalHistCoef, 8, 0, 20)
TUNE_PARAM(EvalHistMin, -1062, -2000, 0)
TUNE_PARAM(EvalHistMax, 9255, 0, 2000)
TUNE_PARAM(EvalHistMargin, 52, -500, 500)

TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5)
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2)
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5)
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2)

TUNE_PARAM(RazoringDepth, 2, 1, 5)
TUNE_PARAM(RazoringMargin, 145, 100, 200)

TUNE_PARAM(SNMPDepth, 13, 8, 15)
TUNE_PARAM(SNMPMargin, 89, 75, 100)
TUNE_PARAM(SNMPImproving, 18, 10, 25)
TUNE_PARAM(SNMPImprovingAfterMove, 20, 1, 60)
TUNE_PARAM(SNMPCutNode, 22, 1, 60)
TUNE_PARAM(SNMPBase, 0, 0, 50)
TUNE_PARAM(SNMPComplexityCoef, 608, 512, 2048)

TUNE_PARAM(NMPEvalMargin, 33, 20, 40)
TUNE_PARAM(NMPReduction, 4, 3, 5)
TUNE_PARAM(NMPDepthDiv, 3, 3, 5)
TUNE_PARAM(NMPEvalDiv, 133, 120, 150)
TUNE_PARAM(NMPStaticEvalCoef, 14, 10, 30)
TUNE_PARAM(NMPStaticEvalMargin, 109, 50, 200)
TUNE_PARAM(NMPHindsightMargin, 114, 1, 200)

TUNE_PARAM(ProbcutMargin, 174, 100, 300)
TUNE_PARAM(ProbcutImprovingMargin, 41, 20, 80)
TUNE_PARAM(ProbcutDepth, 4, 3, 7)
TUNE_PARAM(ProbcutReduction, 5, 3, 5)

TUNE_PARAM(PVSSeeDepthCoef, 18, 10, 20)

TUNE_PARAM(MoveloopHistDiv, 13075, 1, 20000)

TUNE_PARAM(FPDepth, 10, 5, 15)
TUNE_PARAM(FPBias, 110, 0, 150)
TUNE_PARAM(FPMargin, 80, 0, 150)

TUNE_PARAM(LMPDepth, 7, 5, 10)
TUNE_PARAM(LMPBias, 1, -1, 5)

TUNE_PARAM(HistoryPruningDepth, 2, 2, 5)
TUNE_PARAM(HistoryPruningMargin, 4544, 2048, 6144)

TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10)
TUNE_PARAM(SEEPruningQuietMargin, 70, 60, 90)
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10)
TUNE_PARAM(SEEPruningNoisyMargin, 14, 5, 25)
TUNE_PARAM(SEENoisyHistDiv, 265, 128, 512)
TUNE_PARAM(SEEQuietHistDiv, 123, 64, 256)
TUNE_PARAM(FPNoisyDepth, 8, 5, 15)

TUNE_PARAM(FPNoisyBias, 98, 0, 150)
TUNE_PARAM(FPNoisyMargin, 88, 0, 150)
TUNE_PARAM(FPNoisyHistoryDiv, 31, 16, 64)

TUNE_PARAM(SEDepth, 5, 4, 8)
TUNE_PARAM(SEMargin, 41, 1, 64)
TUNE_PARAM(SEWasPVMargin, 37, 1, 96)
TUNE_PARAM(SEDoubleExtensionsMargin, 9, 0, 16)
TUNE_PARAM(SETripleExtensionsMargin, 52, 1, 100)

constexpr int LMRGrain = 1024;

TUNE_PARAM(LMRWasNotPV, 1134, 512, 2048)
TUNE_PARAM(LMRImprovingM1, 2075, 1024, 3072)
TUNE_PARAM(LMRImproving0, 1151, 512, 2048)

TUNE_PARAM(LMRPVNode, 843, 512, 2048)
TUNE_PARAM(LMRGoodEval, 1064, 512, 2048)
TUNE_PARAM(LMREvalDifference, 1065, 512, 2048)
TUNE_PARAM(LMRRefutation, 2200, 1024, 3072)
TUNE_PARAM(LMRGivesCheck, 1166, 512, 2048)

TUNE_PARAM(LMRBadNoisy, 1138, 512, 2048)

TUNE_PARAM(LMRCutNode, 2321, 1024, 3072)
TUNE_PARAM(LMRWasPVHighDepth, 971, 512, 2048)
TUNE_PARAM(LMRTTMoveNoisy, 1165, 512, 2048)
TUNE_PARAM(LMRBadStaticEval, 903, 512, 2048)
TUNE_PARAM(LMRFailLowPV, 861, 512, 2048)
TUNE_PARAM(LMRFailLowPVHighDepth, 1096, 512, 2048)
TUNE_PARAM(LMRCutoffCount, 1000, 512, 2048)

TUNE_PARAM(EvalDifferenceReductionMargin, 208, 100, 300)
TUNE_PARAM(HistReductionDiv, 8488, 7000, 10000)
TUNE_PARAM(CapHistReductionDiv, 4065, 2048, 8192)
TUNE_PARAM(LMRGoodEvalMargin, 303, 200, 400)
TUNE_PARAM(LMRBadStaticEvalMargin, 231, 150, 300)
TUNE_PARAM(LMRCorrectionDivisor, 53, 25, 75)
TUNE_PARAM(DeeperMargin, 77, 0, 150)
TUNE_PARAM(PVDeeperMargin, 75, 0, 150)

TUNE_PARAM(RootSeeDepthCoef, 7, 5, 20)

TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10)
TUNE_PARAM(AspirationWindowsValue, 7, 5, 20)
TUNE_PARAM(AspirationWindowsDivisor, 10160, 1, 20000)
TUNE_PARAM(AspirationWindowExpandMargin, 15, 10, 100)

TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11)
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedMaxPercentage, 1.858202760568546, 1.400, 2.000)
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedCoef, 1.0638501544601164, 0.500, 2.000)

TUNE_PARAM_DOUBLE(TimeManagerBestMoveMax, 1.2285085797971442, 1.100, 1.500)
TUNE_PARAM_DOUBLE(TimeManagerbestMoveStep, 0.07547058646143265, 0.010, 0.200)

TUNE_PARAM_DOUBLE(TimeManagerScoreMin, 0.46958865843889463, 0.400, 0.900)
TUNE_PARAM_DOUBLE(TimeManagerScoreMax, 1.6124152349918068, 1.100, 2.000)
TUNE_PARAM_DOUBLE(TimeManagerScoreBias, 1.058150881442622, 0.700, 1.500)
TUNE_PARAM(TimeManagerScoreDiv, 116, 50, 150)

TUNE_PARAM_DOUBLE(TMCoef1, 0.05117403122729696, 0.035, 0.075)
TUNE_PARAM_DOUBLE(TMCoef2, 0.34772368050337443, 0.250, 0.550)
TUNE_PARAM_DOUBLE(TMCoef3, 5.5483805603934195, 4.500, 6.500)
TUNE_PARAM_DOUBLE(TMCoef4, 0.7453397854308839, 0.650, 0.850)

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.4309152808298575, 0.00, 2.25)
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.5301421809723648, 1.50, 3.70)

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20)
TUNE_PARAM(QuietPawnAttackedCoef, 32, 20, 50)
TUNE_PARAM(QuietPawnAttackedDodgeCoef, 36, 20, 50)
TUNE_PARAM(QuietPawnPushBonus, 9164, 1, 16000)
TUNE_PARAM(QuietKingRingAttackBonus, 3670, 1, 5000)
TUNE_PARAM(KPMoveBonus, 8954, 5000, 20000)
TUNE_PARAM(ThreatCoef1, 15985, 0, 32768)
TUNE_PARAM(ThreatCoef2, 11594, 0, 32768)
TUNE_PARAM(ThreatCoef3, 17673, 0, 32768)
TUNE_PARAM(ThreatCoef4, 16815, 0, 32768)

// eval constants
TUNE_PARAM(EvalScaleBias, 741, 600, 800)
TUNE_PARAM(EvalShuffleCoef, 4, 1, 10)

// main history constants
TUNE_PARAM(MainHistoryBonusMargin, 271, 100, 500)
TUNE_PARAM(MainHistoryBonusBias, 329, 200, 600)
TUNE_PARAM(MainHistoryBonusMax, 2612, 1000, 4000)
TUNE_PARAM(MainHistoryMalusMargin, 349, 200, 500)
TUNE_PARAM(MainHistoryMalusBias, 292, 200, 600)
TUNE_PARAM(MainHistoryMalusMax, 2491, 1000, 4000)
TUNE_PARAM(MainHistoryDivisor, 15372, 1, 32768)

// continuation history constants
TUNE_PARAM(ContinuationHistoryBonusMargin, 273, 100, 500)
TUNE_PARAM(ContinuationHistoryBonusBias, 343, 200, 600)
TUNE_PARAM(ContinuationHistoryBonusMax, 2468, 1000, 4000)
TUNE_PARAM(ContinuationHistoryMalusMargin, 349, 200, 500)
TUNE_PARAM(ContinuationHistoryMalusBias, 276, 200, 600)
TUNE_PARAM(ContinuationHistoryMalusMax, 2483, 1000, 4000)
TUNE_PARAM(ContinuationHistoryDivisor, 15040, 1, 32768)

// capture history constants
TUNE_PARAM(CaptureHistoryBonusMargin, 250, 100, 500)
TUNE_PARAM(CaptureHistoryBonusBias, 340, 200, 600)
TUNE_PARAM(CaptureHistoryBonusMax, 2677, 1000, 4000)
TUNE_PARAM(CaptureHistoryMalusMargin, 335, 200, 500)
TUNE_PARAM(CaptureHistoryMalusBias, 306, 200, 600)
TUNE_PARAM(CaptureHistoryMalusMax, 2473, 1000, 4000)
TUNE_PARAM(CaptureHistoryDivisor, 15708, 1, 32768)

// pawn history constants
TUNE_PARAM(PawnHistoryBonusMargin, 277, 100, 500)
TUNE_PARAM(PawnHistoryBonusBias, 331, 200, 600)
TUNE_PARAM(PawnHistoryBonusMax, 2732, 1000, 4000)
TUNE_PARAM(PawnHistoryMalusMargin, 342, 200, 500)
TUNE_PARAM(PawnHistoryMalusBias, 305, 200, 600)
TUNE_PARAM(PawnHistoryMalusMax, 2588, 1000, 4000)
TUNE_PARAM(PawnHistoryDivisor, 14674, 1, 32768)

// misc history constants
TUNE_PARAM(HistoryUpdateMinDepth, 4, 2, 6)
TUNE_PARAM(FailLowContHistCoef, 50, 0, 256)
TUNE_PARAM(FailLowHistCoef, 70, 0, 256)

TUNE_PARAM(QuietSearchHistCoef, 681, 1, 3072)
TUNE_PARAM(QuietSearchContHist1, 1197, 1, 3072)
TUNE_PARAM(QuietSearchContHist2, 994, 1, 3072)
TUNE_PARAM(QuietSearchContHist4, 983, 1, 3072)
TUNE_PARAM(QuietMPHistCoef, 1069, 1, 3072)
TUNE_PARAM(QuietMPContHist1, 1921, 1, 3072)
TUNE_PARAM(QuietMPContHist2, 1924, 1, 3072)
TUNE_PARAM(QuietMPContHist4, 1090, 1, 3072)
TUNE_PARAM(QuietMPPawnHist, 1070, 1, 3072)

// corrhist constants
TUNE_PARAM(CorrHistPawn, 945, 0, 2048)
TUNE_PARAM(CorrHistMat, 1100, 0, 2048)
TUNE_PARAM(CorrHistCont2, 935, 0, 2048)
TUNE_PARAM(CorrHistCont3, 892, 0, 2048)
TUNE_PARAM(CorrHistCont4, 895, 0, 2048)
TUNE_PARAM(CorrectionHistoryDivisor, 1011, 1, 2048)

// universal constants
TUNE_PARAM(SeeValPawn, 91, 80, 120)
TUNE_PARAM(SeeValKnight, 309, 280, 350)
TUNE_PARAM(SeeValBishop, 338, 320, 380)
TUNE_PARAM(SeeValRook, 514, 450, 600)
TUNE_PARAM(SeeValQueen, 989, 900, 1100)

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