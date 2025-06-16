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
TUNE_PARAM_DOUBLE(l1_weight0, 0.167112, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight1, 0.413087, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight2, 0.153562, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight3, -2.1154, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight4, -0.324559, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight5, 0.105292, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight6, 0.218835, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight7, -0.188133, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight8, 0.270457, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight9, -0.399612, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight10, -0.239073, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight11, -0.0689916, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight12, 0.126805, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight13, 0.00745681, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight14, 0.0792529, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight15, -0.808828, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight16, 0.00785719, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight17, 0.048498, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight18, 0.118331, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight19, -0.0993503, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight20, -0.0735041, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight21, 0.394543, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight22, 0.219746, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight23, 0.291114, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight24, 1.94104, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight25, -0.61154, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight26, -0.736767, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight27, 0.00187327, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight28, -0.26057, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight29, 0.0482366, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight30, 0.740513, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight31, -0.294528, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight32, -2.65027, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight33, -0.772518, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight34, 0.276137, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight35, -0.0719376, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight36, 0.271024, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight37, -0.621173, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight38, 0.0299956, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight39, 0.0717349, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight40, -0.0467965, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight41, 0.149178, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight42, 0.0246557, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight43, 0.267655, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight44, 0.0654112, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight45, -0.0113015, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight46, 0.180897, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight47, -0.0533539, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight48, -0.482892, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight49, -0.271495, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight50, 0.313642, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight51, -0.246652, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight52, 0.0275458, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight53, -0.111031, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight54, -0.0409022, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight55, 0.111263, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight56, 0.189212, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight57, -0.291623, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight58, 0.0502712, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight59, -0.821408, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight60, -0.137979, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight61, 0.0732832, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight62, -0.193233, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight63, -0.726803, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight64, -1.1673, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight65, 0.8696, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight66, -1.38386, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight67, 0.338572, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight68, 0.0762032, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight69, -0.142559, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight70, -0.341669, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight71, -0.183448, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight72, 0.828531, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight73, 0.28194, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight74, 0.0714339, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight75, -0.440917, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight76, -0.497143, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight77, -0.824372, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight78, 0.2593, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight79, -0.128876, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight80, -0.418872, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight81, -0.748386, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight82, 0.174896, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight83, -0.0707175, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight84, -0.119623, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight85, -0.335138, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight86, 0.437473, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight87, 0.300177, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight88, -0.22975, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight89, 0.574953, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight90, -0.368523, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight91, -0.0105726, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight92, 0.238237, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight93, 0.320238, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight94, -0.671663, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight95, 0.0771523, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight96, -0.103735, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight97, 0.0843637, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight98, -2.41183, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight99, -0.591747, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight100, -0.0539212, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight101, 0.0296177, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight102, 0.127081, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight103, 0.114673, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight104, 0.162234, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight105, 0.208024, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight106, 0.11643, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight107, 0.350706, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight108, 0.24639, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight109, -0.0450212, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight110, 0.083465, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight111, -0.0203188, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight112, 0.633411, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight113, -2.05436, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight114, 0.0153056, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight115, 0.0329857, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight116, -0.601417, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight117, 0.242626, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight118, 0.260619, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight119, -0.0347241, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight120, -1.56098, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight121, -0.552343, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight122, 0.0462501, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight123, -0.239504, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight124, 0.18808, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight125, -0.879708, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight126, 0.127161, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight127, -0.414023, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight128, 0.211299, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight129, -0.143841, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight130, 0.561093, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight131, 0.0826688, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight132, 0.37191, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight133, 0.0731462, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight134, 1.79636, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight135, -2.30724, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight136, -0.246475, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight137, 0.127079, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight138, -0.685683, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight139, 0.0219649, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight140, 0.14384, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight141, -0.270081, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight142, 0.226744, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight143, -0.814084, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight144, -0.0996122, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight145, 0.000565383, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight146, 0.305379, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight147, -0.735878, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight148, 0.584791, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight149, -0.251787, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight150, 0.081689, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight151, -0.625793, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight152, -0.500285, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight153, -0.0600044, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight154, -0.345949, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight155, 0.59644, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight156, -0.132262, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight157, 0.133813, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight158, -0.0526609, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight159, -0.117493, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight160, 0.318785, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight161, 0.244437, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight162, 0.131574, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight163, 0.163187, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight164, -1.37948, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight165, -2.85443, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight166, 0.00425128, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight167, -0.0185706, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight168, 0.253093, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight169, 0.0121945, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight170, 0.280338, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight171, 0.170862, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight172, 0.0140408, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight173, 0.324629, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight174, 0.158962, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight175, 0.152578, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias0, 0.223928, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias1, 0.180762, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias2, 1.28385, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias3, -1.07305, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias4, 0.445705, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias5, 0.796861, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias6, 0.210662, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias7, -0.929121, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight0, 1.04817, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight1, 1.58135, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight2, 0.903177, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight3, -2.16242, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight4, 0.620654, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight5, 0.990677, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight6, -1.77422, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight7, -0.891109, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_bias, -0.00834145, -10.0, 10.0);

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