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
TUNE_PARAM(DeltaPruningMargin, 1033, 900, 1100)
TUNE_PARAM(QuiesceFutilityBias, 193, 150, 250)
TUNE_PARAM(QuiesceSEEMargin, -11, -100, 100)

TUNE_PARAM(NegativeImprovingMargin, -146, -300, -100)

TUNE_PARAM(EvalHistCoef, 9, 0, 20)
TUNE_PARAM(EvalHistMin, -1079, -2000, 0)
TUNE_PARAM(EvalHistMax, 886, 0, 2000)
TUNE_PARAM(EvalHistMargin, 15, -500, 500)

TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5)
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2)
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5)
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2)

TUNE_PARAM(RazoringDepth, 2, 1, 5)
TUNE_PARAM(RazoringMargin, 148, 100, 200)

TUNE_PARAM(SNMPDepth, 13, 8, 15)
TUNE_PARAM(SNMPMargin, 89, 75, 100)
TUNE_PARAM(SNMPImproving, 18, 10, 25)
TUNE_PARAM(SNMPImprovingAfterMove, 21, 1, 60)
TUNE_PARAM(SNMPCutNode, 23, 1, 60)
TUNE_PARAM(SNMPBase, 0, 0, 50)
TUNE_PARAM(SNMPComplexityCoef, 572, 512, 2048)

TUNE_PARAM(NMPEvalMargin, 33, 20, 40)
TUNE_PARAM(NMPReduction, 4, 3, 5)
TUNE_PARAM(NMPDepthDiv, 3, 3, 5)
TUNE_PARAM(NMPEvalDiv, 134, 120, 150)
TUNE_PARAM(NMPStaticEvalCoef, 15, 10, 30)
TUNE_PARAM(NMPStaticEvalMargin, 104, 50, 200)
TUNE_PARAM(NMPHindsightMargin, 108, 1, 200)

TUNE_PARAM(ProbcutMargin, 179, 100, 300)
TUNE_PARAM(ProbcutDepth, 4, 3, 7)
TUNE_PARAM(ProbcutReduction, 5, 3, 5)

TUNE_PARAM(PVSSeeDepthCoef, 18, 10, 20)

TUNE_PARAM(MoveloopHistDiv, 13547, 1, 17000)

TUNE_PARAM(FPDepth, 10, 5, 15)
TUNE_PARAM(FPBias, 109, 0, 150)
TUNE_PARAM(FPMargin, 86, 0, 150)

TUNE_PARAM(LMPDepth, 7, 5, 10)
TUNE_PARAM(LMPBias, 1, -1, 5)

TUNE_PARAM(HistoryPruningDepth, 2, 2, 5)
TUNE_PARAM(HistoryPruningMargin, 4524, 2048, 6144)

TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10)
TUNE_PARAM(SEEPruningQuietMargin, 70, 60, 90)
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10)
TUNE_PARAM(SEEPruningNoisyMargin, 14, 5, 25)
TUNE_PARAM(SEENoisyHistDiv, 275, 128, 512)
TUNE_PARAM(FPNoisyDepth, 8, 5, 15)

TUNE_PARAM(FPNoisyBias, 99, 0, 150)
TUNE_PARAM(FPNoisyMargin, 88, 0, 150)
TUNE_PARAM(FPNoisyHistoryDiv, 34, 16, 64)

TUNE_PARAM(SEDepth, 5, 4, 8)
TUNE_PARAM(SEMargin, 42, 1, 64)
TUNE_PARAM(SEWasPVMargin, 41, 1, 96)
TUNE_PARAM(SEDoubleExtensionsMargin, 10, 0, 16)
TUNE_PARAM(SETripleExtensionsMargin, 52, 1, 100)

constexpr int LMRGrain = 1024;

TUNE_PARAM(LMRWasNotPV, 1125, 512, 2048)
TUNE_PARAM(LMRImprovingM1, 2032, 1024, 3072)
TUNE_PARAM(LMRImproving0, 1156, 512, 2048)

TUNE_PARAM(LMRPVNode, 848, 512, 2048)
TUNE_PARAM(LMRGoodEval, 1141, 512, 2048)
TUNE_PARAM(LMREvalDifference, 1017, 512, 2048)
TUNE_PARAM(LMRRefutation, 2154, 1024, 3072)
TUNE_PARAM(LMRGivesCheck, 1178, 512, 2048)

TUNE_PARAM(LMRBadNoisy, 1139, 512, 2048)

TUNE_PARAM(LMRCutNode, 2334, 1024, 3072)
TUNE_PARAM(LMRWasPVHighDepth, 941, 512, 2048)
TUNE_PARAM(LMRTTMoveNoisy, 1132, 512, 2048)
TUNE_PARAM(LMRBadStaticEval, 913, 512, 2048)
TUNE_PARAM(LMRFailLowPV, 896, 512, 2048)
TUNE_PARAM(LMRFailLowPVHighDepth, 1118, 512, 2048)
TUNE_PARAM(LMRCutoffCount, 963, 512, 2048)

TUNE_PARAM(EvalDifferenceReductionMargin, 204, 100, 300)
TUNE_PARAM(HistReductionDiv, 8501, 7000, 10000)
TUNE_PARAM(CapHistReductionDiv, 3895, 2048, 8192)
TUNE_PARAM(LMRGoodEvalMargin, 310, 200, 400)
TUNE_PARAM(LMRBadStaticEvalMargin, 233, 150, 300)
TUNE_PARAM(LMRCorrectionDivisor, 51, 25, 75)
TUNE_PARAM(DeeperMargin, 76, 0, 150)
TUNE_PARAM(PVDeeperMargin, 74, 0, 150)

TUNE_PARAM(RootSeeDepthCoef, 7, 5, 20)

TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10)
TUNE_PARAM(AspirationWindowsValue, 7, 5, 20)
TUNE_PARAM(AspirationWindowsDivisor, 10052, 1, 20000)
TUNE_PARAM(AspirationWindowExpandMargin, 18, 10, 100)

TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11)
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedMaxPercentage, 1.858202760568546, 1.400, 2.000)
TUNE_PARAM_DOUBLE(TimeManagerNodesSearchedCoef, 1.0638501544601164, 0.500, 2.000)

TUNE_PARAM_DOUBLE(TimeManagerBestMoveMax, 1.2285085797971442, 1.100, 1.500)
TUNE_PARAM_DOUBLE(TimeManagerbestMoveStep, 0.07547058646143265, 0.010, 0.200)

TUNE_PARAM_DOUBLE(TimeManagerScoreMin, 0.46958865843889463, 0.400, 0.900)
TUNE_PARAM_DOUBLE(TimeManagerScoreMax, 1.6124152349918068, 1.100, 2.000)
TUNE_PARAM_DOUBLE(TimeManagerScoreBias, 1.058150881442622, 0.700, 1.500)
TUNE_PARAM(TimeManagerScoreDiv, 116, 90, 150)

TUNE_PARAM_DOUBLE(TMCoef1, 0.05117403122729696, 0.035, 0.075)
TUNE_PARAM_DOUBLE(TMCoef2, 0.34772368050337443, 0.250, 0.550)
TUNE_PARAM_DOUBLE(TMCoef3, 5.5483805603934195, 4.500, 6.500)
TUNE_PARAM_DOUBLE(TMCoef4, 0.7453397854308839, 0.650, 0.850)

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.2705205633876617, 0.00, 2.25)
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.5306565119514737, 1.50, 3.70)

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20)
TUNE_PARAM(QuietPawnAttackedCoef, 32, 20, 50)
TUNE_PARAM(QuietPawnAttackedDodgeCoef, 36, 20, 50)
TUNE_PARAM(QuietPawnPushBonus, 8828, 1, 16000)
TUNE_PARAM(QuietKingRingAttackBonus, 3442, 1, 5000)
TUNE_PARAM(KPMoveBonus, 8681, 5000, 20000)
TUNE_PARAM(ThreatCoef1, 15734, 0, 32768)
TUNE_PARAM(ThreatCoef2, 12404, 0, 32768)
TUNE_PARAM(ThreatCoef3, 17344, 0, 32768)
TUNE_PARAM(ThreatCoef4, 15592, 0, 32768)

// eval constants
TUNE_PARAM(EvalScaleBias, 739, 600, 800)
TUNE_PARAM(EvalShuffleCoef, 4, 1, 10)

// main history constants
TUNE_PARAM(MainHistoryBonusMargin, 274, 100, 500)
TUNE_PARAM(MainHistoryBonusBias, 333, 200, 600)
TUNE_PARAM(MainHistoryBonusMax, 2668, 1000, 4000)
TUNE_PARAM(MainHistoryMalusMargin, 346, 200, 500)
TUNE_PARAM(MainHistoryMalusBias, 297, 200, 600)
TUNE_PARAM(MainHistoryMalusMax, 2534, 1000, 4000)
TUNE_PARAM(MainHistoryDivisor, 15261, 1, 32768)

// continuation history constants
TUNE_PARAM(ContinuationHistoryBonusMargin, 269, 100, 500)
TUNE_PARAM(ContinuationHistoryBonusBias, 342, 200, 600)
TUNE_PARAM(ContinuationHistoryBonusMax, 2563, 1000, 4000)
TUNE_PARAM(ContinuationHistoryMalusMargin, 348, 200, 500)
TUNE_PARAM(ContinuationHistoryMalusBias, 292, 200, 600)
TUNE_PARAM(ContinuationHistoryMalusMax, 2495, 1000, 4000)
TUNE_PARAM(ContinuationHistoryDivisor, 15394, 1, 32768)

// capture history constants
TUNE_PARAM(CaptureHistoryBonusMargin, 236, 100, 500)
TUNE_PARAM(CaptureHistoryBonusBias, 339, 200, 600)
TUNE_PARAM(CaptureHistoryBonusMax, 2653, 1000, 4000)
TUNE_PARAM(CaptureHistoryMalusMargin, 335, 200, 500)
TUNE_PARAM(CaptureHistoryMalusBias, 312, 200, 600)
TUNE_PARAM(CaptureHistoryMalusMax, 2441, 1000, 4000)
TUNE_PARAM(CaptureHistoryDivisor, 15684, 1, 32768)

// pawn history constants
TUNE_PARAM(PawnHistoryBonusMargin, 274, 100, 500)
TUNE_PARAM(PawnHistoryBonusBias, 335, 200, 600)
TUNE_PARAM(PawnHistoryBonusMax, 2623, 1000, 4000)
TUNE_PARAM(PawnHistoryMalusMargin, 334, 200, 500)
TUNE_PARAM(PawnHistoryMalusBias, 299, 200, 600)
TUNE_PARAM(PawnHistoryMalusMax, 2636, 1000, 4000)
TUNE_PARAM(PawnHistoryDivisor, 15433, 1, 32768)

// misc history constants
TUNE_PARAM(HistoryUpdateMinDepth, 4, 2, 6)
TUNE_PARAM(FailLowContHistCoef, 53, 0, 256)
TUNE_PARAM(FailLowHistCoef, 71, 0, 256)

TUNE_PARAM(QuietSearchHistCoef, 723, 1, 3072)
TUNE_PARAM(QuietSearchContHist1, 1117, 1, 3072)
TUNE_PARAM(QuietSearchContHist2, 1049, 1, 3072)
TUNE_PARAM(QuietSearchContHist4, 1027, 1, 3072)
TUNE_PARAM(QuietMPHistCoef, 1155, 1, 3072)
TUNE_PARAM(QuietMPContHist1, 1869, 1, 3072)
TUNE_PARAM(QuietMPContHist2, 2008, 1, 3072)
TUNE_PARAM(QuietMPContHist4, 1159, 1, 3072)
TUNE_PARAM(QuietMPPawnHist, 1046, 1, 3072)

// corrhist constants
TUNE_PARAM(CorrHistPawn, 918, 0, 2048)
TUNE_PARAM(CorrHistMat, 1091, 0, 2048)
TUNE_PARAM(CorrHistMinor, 1024, 0, 2048)
TUNE_PARAM(CorrHistMajor, 1024, 0, 2048)
TUNE_PARAM(CorrHistCont2, 913, 0, 2048)
TUNE_PARAM(CorrHistCont3, 872, 0, 2048)
TUNE_PARAM(CorrHistCont4, 897, 0, 2048)
TUNE_PARAM(CorrectionHistoryDivisor, 947, 1, 2048)

// universal constants
TUNE_PARAM(SeeValPawn, 92, 80, 120)
TUNE_PARAM(SeeValKnight, 310, 280, 350)
TUNE_PARAM(SeeValBishop, 338, 320, 380)
TUNE_PARAM(SeeValRook, 514, 450, 600)
TUNE_PARAM(SeeValQueen, 987, 900, 1100)

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