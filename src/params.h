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

TUNE_PARAM(LMRNoisyNotImproving, 1010, 412, 2048);
TUNE_PARAM(LMRBadNoisy, 1081, 512, 2048);

TUNE_PARAM(LMRCutNode, 2253, 1024, 3072);
TUNE_PARAM(LMRWasPVHighDepth, 957, 512, 2048);
TUNE_PARAM(LMRTTMoveNoisy, 1184, 512, 2048);
TUNE_PARAM(LMRBadStaticEval, 1031, 512, 2048);
TUNE_PARAM(LMRFailLowPV, 982, 512, 2048);
TUNE_PARAM(LMRFailLowPVHighDepth, 1118, 512, 2048);

TUNE_PARAM(EvalDifferenceReductionMargin, 202, 100, 300);
TUNE_PARAM(HistReductionDiv, 8463, 7000, 10000);
TUNE_PARAM(CapHistReductionDiv, 4158, 2048, 8192);
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

TUNE_PARAM_DOUBLE(LMRQuietBias, 1.1540091336006513, 1.00, 1.25);
TUNE_PARAM_DOUBLE(LMRQuietDiv, 2.575107162151004, 1.50, 2.70);

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

TUNE_PARAM_DOUBLE(l1_weight0, 0.1212, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight1, 0.3991, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight2, 0.1935, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight3, -2.2528, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight4, -0.2837, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight5, -0.0176, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight6, 0.1675, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight7, 0.0144, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight8, 0.0536, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight9, -0.2324, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight10, -0.3012, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight11, 0.082, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight12, 0.1217, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight13, 0.0755, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight14, -0.1318, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight15, -0.6849, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight16, 0.0548, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight17, -0.0592, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight18, -0.0682, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight19, -0.1475, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight20, -0.0573, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight21, 0.2281, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight22, 0.1585, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight23, 0.2855, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight24, 2.0734, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight25, -0.502, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight26, -0.4052, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight27, 0.0149, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight28, -0.227, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight29, -0.0317, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight30, 0.5592, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight31, -0.2986, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight32, -2.6317, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight33, -0.7808, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight34, 0.1837, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight35, -0.1569, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight36, 0.3154, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight37, -0.6331, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight38, 0.2022, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight39, -0.0183, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight40, -0.1361, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight41, -0.0915, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight42, 0.0576, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight43, 0.292, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight44, 0.1074, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight45, 0.1497, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight46, 0.2527, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight47, 0.1542, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight48, -0.3276, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight49, -0.0244, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight50, 0.4473, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight51, 0.0377, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight52, 0.0797, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight53, -0.2623, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight54, -0.1981, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight55, 0.1184, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight56, 0.2693, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight57, -0.0454, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight58, 0.1398, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight59, -0.6626, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight60, 0.0153, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight61, -0.0085, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight62, -0.0578, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight63, -0.2939, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight64, -1.0436, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight65, 0.645, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight66, -1.3387, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight67, 0.1848, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight68, 0.1251, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight69, -0.1272, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight70, -0.3417, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight71, -0.0026, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight72, 0.9119, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight73, 0.04, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight74, 0.0797, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight75, -0.289, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight76, -0.559, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight77, -0.4582, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight78, 0.2128, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight79, -0.012, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight80, 0.0079, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight81, -0.6782, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight82, 0.0655, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight83, 0.0057, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight84, -0.0628, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight85, -0.1397, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight86, 0.0323, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight87, 0.4489, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight88, -0.1587, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight89, 0.7515, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight90, -0.3832, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight91, 0.1464, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight92, 0.1119, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight93, 0.0101, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight94, -0.7445, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight95, -0.0386, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight96, 0.0603, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight97, 0.0781, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight98, -2.4528, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight99, -0.7456, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight100, -0.0723, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight101, -0.0111, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight102, 0.0358, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight103, 0.2836, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight104, 0.1419, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight105, 0.034, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight106, 0.0079, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight107, 0.2067, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight108, 0.0343, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight109, 0.0451, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight110, 0.1154, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight111, 0.0224, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight112, 0.6273, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight113, -2.1998, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight114, -0.0361, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight115, -0.0009, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight116, -0.7611, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight117, 0.1521, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight118, 0.1362, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight119, -0.1511, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight120, -1.5835, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight121, -0.5806, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight122, 0.111, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight123, -0.032, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight124, 0.0398, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight125, -0.918, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight126, 0.2184, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight127, -0.4764, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight128, 0.0235, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight129, -0.4162, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight130, 0.4803, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight131, 0.0547, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight132, 0.1253, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight133, 0.2016, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight134, 1.6644, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight135, -2.2652, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight136, -0.1745, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight137, -0.0018, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight138, -0.7405, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight139, 0.0627, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight140, 0.2071, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight141, -0.2116, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight142, 0.1758, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight143, -0.6781, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight144, 0.1289, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight145, -0.1048, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight146, 0.2285, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight147, -0.8075, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight148, 0.0501, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight149, -0.1012, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight150, -0.0276, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight151, -0.4865, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight152, -0.4435, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight153, 0.1942, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight154, -0.25, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight155, 0.6621, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight156, -0.0747, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight157, 0.0008, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight158, -0.0475, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight159, 0.0007, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight160, 0.5146, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight161, -0.0642, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight162, 0.1675, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight163, 0.0327, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight164, -1.5936, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight165, -2.8569, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight166, -0.0078, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight167, 0.0106, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight168, -0.0042, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight169, -0.0054, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight170, 0.2417, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight171, -0.0178, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight172, -0.0324, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight173, -0.0023, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight174, -0.0461, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_weight175, -0.0341, -3.0, 3.0);

TUNE_PARAM_DOUBLE(l1_bias0, 0.1245, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias1, 0.2338, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias2, 1.2253, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias3, -0.9936, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias4, 0.6881, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias5, 0.6972, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias6, 0.0825, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l1_bias7, -0.708, -3.0, 3.0);

TUNE_PARAM_DOUBLE(l2_weight0, 1.0379, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight1, 1.338, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight2, 0.9375, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight3, -2.0775, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight4, 0.5964, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight5, 1.0669, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight6, -1.9759, -3.0, 3.0);
TUNE_PARAM_DOUBLE(l2_weight7, -1.0226, -3.0, 3.0);

TUNE_PARAM_DOUBLE(l2_bias, -0.1706, -3.0, 3.0);

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
                  << (param.max - param.min) / 60.0 << ", 0.002\n";
    }
}