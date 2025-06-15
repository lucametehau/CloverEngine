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
TUNE_PARAM_DOUBLE(l1_weight0, 0.120393, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight1, 0.381622, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight2, 0.162907, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight3, -2.12141, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight4, -0.335856, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight5, 0.155312, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight6, 0.192467, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight7, -0.123787, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight8, 0.257347, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight9, -0.478859, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight10, -0.258468, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight11, 0.0239859, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight12, 0.118649, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight13, 0.0533357, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight14, 0.0612502, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight15, -0.723946, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight16, 0.00043532, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight17, 0.0411566, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight18, 0.137404, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight19, -0.11341, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight20, -0.0926227, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight21, 0.43016, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight22, 0.250809, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight23, 0.330776, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight24, 1.93141, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight25, -0.549884, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight26, -0.721727, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight27, 0.0744686, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight28, -0.223601, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight29, 0.118341, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight30, 0.754722, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight31, -0.175539, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight32, -2.56492, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight33, -0.735986, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight34, 0.343534, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight35, -0.0801447, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight36, 0.276151, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight37, -0.624023, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight38, 0.0328745, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight39, 0.00448123, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight40, -0.0440145, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight41, 0.0866075, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight42, 0.0437982, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight43, 0.28033, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight44, 0.0830869, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight45, 0.000992049, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight46, 0.255105, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight47, 0.0325328, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight48, -0.446653, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight49, -0.294428, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight50, 0.295516, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight51, -0.137065, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight52, -0.0374599, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight53, -0.13748, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight54, -0.086819, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight55, 0.156997, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight56, 0.0909363, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight57, -0.236662, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight58, 0.159035, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight59, -0.909032, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight60, -0.0889751, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight61, -0.0120312, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight62, -0.393356, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight63, -0.733182, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight64, -1.19045, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight65, 0.828568, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight66, -1.29354, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight67, 0.290795, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight68, 0.0981637, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight69, -0.117343, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight70, -0.273126, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight71, -0.0700205, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight72, 0.750981, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight73, 0.23866, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight74, 0.105744, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight75, -0.330514, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight76, -0.538513, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight77, -0.757899, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight78, 0.219728, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight79, -0.117354, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight80, -0.379015, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight81, -0.747959, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight82, 0.153884, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight83, -0.0912985, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight84, -0.183506, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight85, -0.381184, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight86, 0.439374, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight87, 0.25548, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight88, -0.268864, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight89, 0.514422, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight90, -0.382594, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight91, 0.112788, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight92, 0.204651, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight93, 0.293036, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight94, -0.543524, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight95, 0.0204743, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight96, -0.115247, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight97, 0.0904434, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight98, -2.40736, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight99, -0.682618, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight100, -0.0964205, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight101, -0.00389332, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight102, 0.0768916, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight103, 0.113496, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight104, 0.189683, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight105, 0.184729, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight106, 0.153674, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight107, 0.269865, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight108, 0.264892, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight109, -0.0997342, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight110, 0.0716267, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight111, 0.0260663, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight112, 0.707173, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight113, -2.06237, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight114, -0.0391812, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight115, 0.0397226, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight116, -0.577665, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight117, 0.225696, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight118, 0.300911, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight119, -0.0609792, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight120, -1.56761, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight121, -0.601571, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight122, 0.0164302, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight123, -0.133031, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight124, 0.125849, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight125, -0.955579, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight126, 0.209574, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight127, -0.447882, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight128, 0.0992192, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight129, -0.0758041, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight130, 0.508529, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight131, 0.0165967, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight132, 0.301103, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight133, 0.086959, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight134, 1.74857, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight135, -2.31423, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight136, -0.280954, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight137, 0.0651728, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight138, -0.695511, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight139, 0.0694988, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight140, 0.0824294, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight141, -0.214342, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight142, 0.217049, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight143, -0.86441, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight144, -0.0960265, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight145, -0.0224857, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight146, 0.24729, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight147, -0.736314, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight148, 0.505978, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight149, -0.295423, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight150, 0.0655543, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight151, -0.620658, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight152, -0.49886, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight153, -0.0124459, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight154, -0.235313, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight155, 0.548536, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight156, -0.127958, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight157, 0.096467, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight158, -0.0722505, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight159, -0.110367, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight160, 0.395342, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight161, 0.191588, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight162, 0.18875, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight163, 0.142748, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight164, -1.4446, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight165, -2.86196, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight166, 0.0185702, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight167, 0.0266193, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight168, 0.197023, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight169, -0.0866381, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight170, 0.266231, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight171, 0.175638, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight172, -0.0429284, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight173, 0.286721, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight174, 0.109784, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_weight175, 0.0499009, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias0, 0.243601, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias1, 0.268836, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias2, 1.21978, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias3, -1.02086, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias4, 0.40336, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias5, 0.748347, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias6, 0.201438, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l1_bias7, -0.883146, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight0, 1.0529, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight1, 1.60868, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight2, 0.965601, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight3, -2.17384, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight4, 0.566966, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight5, 1.03696, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight6, -1.82914, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_weight7, -1.0536, -10.0, 10.0);
TUNE_PARAM_DOUBLE(l2_bias, -0.0535495, -10.0, 10.0);

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