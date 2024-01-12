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

// tuning param/option
struct Parameter {
    std::string name;
    int &value;
    int min, max;
};

std::vector <Parameter> params;

// trick to be able to create options
struct CreateParam {
    int _value;
    CreateParam(std::string name, int value, int min, int max) : _value(value) { params.push_back({ name, _value, min, max }); }

    operator int() const {
        return _value;
    }
};

#ifndef TUNE_FLAG

#define TUNE_PARAM(name, value, min, max) constexpr int name = value;

#else

#define TUNE_PARAM(name, value, min, max) CreateParam name(#name, value, min, max);

#endif


// search constants
TUNE_PARAM(DeltaPruningMargin, 1009, 900, 1100);
TUNE_PARAM(QuiesceFutilityBias, 210, 150, 250);
TUNE_PARAM(NegativeImprovingMargin, -186, -300, -100);
TUNE_PARAM(RazoringDepth, 2, 1, 5);
TUNE_PARAM(RazoringMargin, 148, 100, 200);
TUNE_PARAM(SNMPDepth, 13, 8, 15);
TUNE_PARAM(SNMPMargin, 86, 75, 100);
TUNE_PARAM(SNMPImproving, 19, 10, 25);
TUNE_PARAM(NMPEvalMargin, 31, 20, 40);
TUNE_PARAM(NMPReduction, 3, 3, 5);
TUNE_PARAM(NMPDepthDiv, 4, 3, 5);
TUNE_PARAM(NMPEvalDiv, 134, 120, 150);
TUNE_PARAM(ProbcutMargin, 192, 100, 300);
TUNE_PARAM(ProbcutDepth, 6, 3, 7);
TUNE_PARAM(ProbcutReduction, 5, 3, 5);
TUNE_PARAM(IIRPvNodeDepth, 4, 0, 5);
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2);
TUNE_PARAM(IIRCutNodeDepth, 4, 0, 5);
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2);
TUNE_PARAM(PVSSeeDepthCoef, 15, 10, 20);
TUNE_PARAM(FPDepth, 9, 5, 10);
TUNE_PARAM(FPBias, 99, 90, 120);
TUNE_PARAM(FPMargin, 97, 90, 120);
TUNE_PARAM(LMPDepth, 7, 5, 10);
TUNE_PARAM(LMPBias, 2, 1, 5);
TUNE_PARAM(SEEPruningQuietDepth, 7, 5, 10);
TUNE_PARAM(SEEPruningQuietMargin, 70, 60, 90);
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningNoisyMargin, 10, 5, 25);
TUNE_PARAM(FPNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEDepth, 6, 4, 8);
TUNE_PARAM(SEMargin, 58, 32, 96);
TUNE_PARAM(SEDoubleExtensionsMargin, 24, 15, 35);
TUNE_PARAM(EvalDifferenceReductionMargin, 205, 100, 300);
TUNE_PARAM(HistReductionDiv, 8284, 7000, 10000);
TUNE_PARAM(RootSeeDepthCoef, 9, 5, 20);
TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10);
TUNE_PARAM(AspirationWindosValue, 7, 5, 20);
TUNE_PARAM(AspirationWindowExpandMargin, 32, 10, 100);
TUNE_PARAM(AspirationWindowExpandBias, 1, 0, 10);
TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11);
TUNE_PARAM(TimeManagerNodesSearchedMaxPercentage, 1556, 1200, 1800);
TUNE_PARAM(TimeManagerBestMoveMax, 1249, 1100, 1500);
TUNE_PARAM(TimeManagerBestMoveStep, 52, 10, 100);
TUNE_PARAM(TimeManagerScoreMin, 48, 40, 90);
TUNE_PARAM(TimeManagerScoreMax, 157, 110, 200);
TUNE_PARAM(TimeManagerScoreBias, 106, 10, 150);
TUNE_PARAM(TimeManagerScoreDiv, 112, 90, 150);
TUNE_PARAM(TimeManagerNodesSeachedMaxCoef, 108, 90, 200);
TUNE_PARAM(TimeManagerNodesSearchedCoef, 117, 50, 200);
TUNE_PARAM(LMRQuietBias, 120, 100, 200);
TUNE_PARAM(LMRQuietDiv, 260, 220, 300);
TUNE_PARAM(LMRNoisyBias, 65, 40, 100);
TUNE_PARAM(LMRNoisyDiv, 464, 300, 600);

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 9, 1, 20);
TUNE_PARAM(GoodNoisyPromotionBonus, 10022, 5000, 12000);
TUNE_PARAM(QuietHistCoef, 1, 1, 4);
TUNE_PARAM(QuietContHist1, 1, 1, 2);
TUNE_PARAM(QuietContHist2, 2, 1, 2);
TUNE_PARAM(QuietContHist4, 2, 1, 2);
TUNE_PARAM(QuietPawnAttackedCoef, 35, 20, 50);
TUNE_PARAM(QuietPawnPushBonus, 9306, 8000, 12000);
TUNE_PARAM(QuietKingRingAttackBonus, 3508, 3000, 5000);

// eval constants
TUNE_PARAM(EvalScaleBias, 681, 600, 800);
TUNE_PARAM(EvalShuffleCoef, 5, 1, 10);

// history constants
TUNE_PARAM(HistoryBonusMargin, 282, 250, 500);
TUNE_PARAM(HistoryBonusBias, 336, 200, 600);
TUNE_PARAM(HistoryBonusMax, 2501, 1800, 3000);

// universal constants
TUNE_PARAM(SeeValPawn, 94, 80, 120);
TUNE_PARAM(SeeValKnight, 311, 280, 350);
TUNE_PARAM(SeeValBishop, 342, 320, 380);
TUNE_PARAM(SeeValRook, 515, 450, 600);
TUNE_PARAM(SeeValQueen, 990, 900, 1100);

void print_params_for_ob() {
    for (auto& param : params) {
        std::cout << param.name << ", int, " << param.value << ", " << param.min << ", " << param.max << ", " << std::max(0.5, (param.max - param.min) / 20.0) << ", 0.002\n";
    }
}