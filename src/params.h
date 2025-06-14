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

// LMR Network params
TUNE_PARAM_DOUBLE(l1_weight0, -1.18046, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight1, -0.749275, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight2, 2.20222, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight3, -1.9356, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight4, -3.04782, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight5, -0.512532, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight6, 1.76307, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight7, -1.84792, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight8, -1.51827, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight9, 0.950849, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight10, -0.766134, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight11, 0.876646, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight12, -1.23588, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight13, -1.4483, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight14, 1.76677, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight15, 2.78284, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight16, 1.83763, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight17, -1.67534, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight18, 0.0717797, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight19, -1.07016, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight20, -0.228306, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight21, 2.06369, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight22, -3.1536, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight23, -0.399933, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight24, 5.07155, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight25, -1.97531, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight26, -2.7359, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight27, 3.38808, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight28, 1.81751, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight29, 1.95836, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight30, -2.06693, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight31, -0.524014, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight32, -2.0637, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight33, 0.662003, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight34, -1.9647, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight35, 0.722656, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight36, 3.55749, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight37, 0.328675, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight38, 2.68188, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight39, -0.315158, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight40, -0.462664, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight41, 2.13812, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight42, -1.09927, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight43, 1.10643, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight44, -0.843917, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight45, -2.17636, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight46, -0.447801, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight47, -0.0550075, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight48, 0.648399, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight49, 0.312573, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight50, 1.00029, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight51, -1.59745, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight52, 1.97374, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight53, -2.07272, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight54, 0.451915, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight55, 0.0947866, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight56, 2.74145, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight57, -0.632305, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight58, -0.274485, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight59, 1.61468, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight60, 0.705142, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight61, 1.06222, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight62, -1.22701, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight63, -1.91558, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight64, -2.41141, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight65, -0.850817, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight66, 1.94513, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight67, -0.581192, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight68, -0.691638, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight69, 0.0500272, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight70, 3.77818, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight71, 0.570706, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight72, 0.80002, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight73, 1.99647, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight74, -0.621596, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight75, -0.987237, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight76, -1.69891, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight77, 0.00213861, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight78, -0.0858536, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight79, 2.68784, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight80, -0.347815, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight81, 3.59376, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight82, 0.803454, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight83, 0.131958, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight84, 0.18882, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight85, 1.20623, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight86, -3.96436, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight87, 0.649666, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight88, 1.47203, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight89, 2.49317, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight90, -1.67244, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight91, -4.06847, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight92, 0.73125, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight93, 0.919739, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight94, -4.09653, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight95, -2.30588, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight96, 1.77744, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight97, -2.68069, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight98, -0.153914, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight99, -0.781881, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight100, 0.623866, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight101, 1.82652, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight102, -0.373078, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight103, 2.50691, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight104, -0.291655, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight105, 4.81889, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight106, 2.36113, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight107, 2.18806, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight108, 1.18977, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight109, 1.58778, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight110, -0.51796, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight111, 3.36406, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight112, 1.70484, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight113, -4.62784, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight114, 3.14866, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight115, -0.49873, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight116, -0.847103, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight117, 0.347069, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight118, -0.0851356, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight119, -3.69931, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight120, -2.72061, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight121, -2.8699, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight122, 0.658768, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight123, -3.58717, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight124, -3.72968, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight125, -3.13785, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight126, -2.34611, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight127, 0.816709, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight128, -0.874846, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight129, -0.572592, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight130, 1.95413, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight131, 0.937969, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight132, -0.751485, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight133, -1.76915, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight134, 1.82852, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight135, 0.0714761, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight136, 0.896366, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight137, 2.22453, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight138, -0.0763104, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight139, 0.163285, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight140, 0.886665, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight141, -2.83998, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight142, 3.35558, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight143, 1.22229, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight144, 1.41099, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight145, 0.994449, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight146, -2.25099, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight147, 3.49878, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight148, -3.20522, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight149, 0.692806, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight150, 0.651807, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight151, 0.166865, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight152, -0.0517879, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight153, -2.71766, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight154, -0.494099, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight155, 1.3056, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight156, -1.51259, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight157, -1.45648, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight158, -0.619973, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight159, -2.22027, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight160, -0.256563, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight161, 0.693385, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight162, 2.22235, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight163, 1.341, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight164, -0.287685, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight165, -7.81141, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight166, -1.22953, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight167, 1.22687, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight168, -2.86776, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight169, 3.20931, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight170, -3.89621, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight171, -0.195373, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight172, 0.555521, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight173, 1.2825, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight174, -2.84321, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight175, 1.28996, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias0, -4.81877, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias1, -1.98398, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias2, -3.102, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias3, -2.29346, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias4, -1.17793, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias5, 0.0400501, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias6, 0.715099, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias7, 0.689496, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight0, -1.23567, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight1, 0.918872, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight2, -0.289199, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight3, -2.61777, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight4, -0.654302, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight5, 4.21058, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight6, -2.90534, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight7, -2.17896, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_bias, 2.87166, -10.0, 10.0);

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