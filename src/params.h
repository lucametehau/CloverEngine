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
    int value;
    int min, max;
};

std::vector <Parameter> params;

// trick to be able to create options
struct CreateParam {
    CreateParam(std::string name, int value, int min, int max) { params.push_back({ name, value, min, max }); }
};

#ifndef TUNE_FLAG

#define TUNE_PARAM(name, value, min, max) constexpr int name = value;

#else

#define TUNE_PARAM(name, value, min, max) CreateParam name(#name, value, min, max);

#endif


// search constants
TUNE_PARAM(DeltaPruningMargin, 994, 900, 1100);
TUNE_PARAM(QuiesceFutilityBias, 203, 150, 250);
TUNE_PARAM(NegativeImprovingMargin, -200, -300, -100);
TUNE_PARAM(RazoringDepth, 2, 1, 5);
TUNE_PARAM(RazoringMargin, 150, 100, 200);
TUNE_PARAM(SNMPDepth, 12, 8, 15);
TUNE_PARAM(SNMPMargin, 84, 75, 100);
TUNE_PARAM(SNMPImproving, 19, 10, 25);
TUNE_PARAM(NMPEvalMargin, 30, 20, 40);
TUNE_PARAM(NMPReduction, 3, 3, 5);
TUNE_PARAM(NMPDepthDiv, 3, 3, 5);
TUNE_PARAM(NMPEvalDiv, 135, 120, 150);
TUNE_PARAM(ProbcutMargin, 200, 100, 300);
TUNE_PARAM(ProbcutDepth, 5, 3, 7);
TUNE_PARAM(ProbcutReduction, 4, 3, 5);
TUNE_PARAM(IIRPvNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRPvNodeReduction, 1, 1, 2);
TUNE_PARAM(IIRCutNodeDepth, 3, 0, 5);
TUNE_PARAM(IIRCutNodeReduction, 1, 1, 2);
TUNE_PARAM(PVSSeeDepthCoef, 15, 10, 20);
TUNE_PARAM(FPDepth, 8, 5, 10);
TUNE_PARAM(FPBias, 100, 90, 120);
TUNE_PARAM(FPMargin, 99, 90, 120);
TUNE_PARAM(LMPDepth, 8, 5, 10);
TUNE_PARAM(LMPBias, 3, 1, 5);
TUNE_PARAM(SEEPruningQuietDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningQuietMargin, 71, 60, 90);
TUNE_PARAM(SEEPruningNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEEPruningNoisyMargin, 10, 5, 25);
TUNE_PARAM(FPNoisyDepth, 8, 5, 10);
TUNE_PARAM(SEDepth, 6, 4, 8);
TUNE_PARAM(SEMargin, 64, 32, 96);
TUNE_PARAM(SEDoubleExtensionsMargin, 25, 15, 35);
TUNE_PARAM(EvalDifferenceReductionMargin, 200, 100, 300);
TUNE_PARAM(HistReductionDiv, 8350, 7000, 10000);
TUNE_PARAM(RootSeeDepthCoef, 10, 5, 20);
TUNE_PARAM(AspirationWindowsDepth, 6, 4, 10);
TUNE_PARAM(AspirationWindosValue, 8, 5, 20);
TUNE_PARAM(AspirationWindowExpandMargin, 25, 10, 100);
TUNE_PARAM(AspirationWindowExpandBias, 0, 0, 10);
TUNE_PARAM(TimeManagerMinDepth, 9, 5, 11);
TUNE_PARAM(TimeManagerNodesSearchedMaxPercentage, 1570, 1200, 1800);
TUNE_PARAM(TimeManagerBestMoveMax, 1250, 1100, 1500);
TUNE_PARAM(TimeManagerBestMoveStep, 50, 10, 100);
TUNE_PARAM(TimeManagerScoreMin, 50, 40, 90);
TUNE_PARAM(TimeManagerScoreMax, 150, 110, 200);
TUNE_PARAM(TimeManagerScoreBias, 100, 10, 150);
TUNE_PARAM(TimeManagerScoreDiv, 111, 90, 150);
TUNE_PARAM(TimeManagerNodesSeachedMaxCoef, 100, 90, 200);
TUNE_PARAM(TimeManagerNodesSearchedCoef, 100, 50, 200);
TUNE_PARAM(LMRQuietBias, 120, 100, 200);
TUNE_PARAM(LMRQuietDiv, 260, 220, 300);
TUNE_PARAM(LMRNoisyBias, 63, 40, 100);
TUNE_PARAM(LMRNoisyDiv, 494, 300, 600);

// movepicker constants
TUNE_PARAM(GoodNoisyValueCoef, 10, 1, 20);
TUNE_PARAM(GoodNoisyPromotionBonus, 10000, 5000, 12000);
TUNE_PARAM(QuietHistCoef, 1, 1, 4);
TUNE_PARAM(QuietContHist1, 1, 1, 2);
TUNE_PARAM(QuietContHist2, 1, 1, 2);
TUNE_PARAM(QuietContHist4, 1, 1, 2);
TUNE_PARAM(QuietPawnAttackedCoef, 36, 20, 50);
TUNE_PARAM(QuietPawnPushBonus, 9520, 8000, 12000);
TUNE_PARAM(QuietKingRingAttackBonus, 3579, 3000, 5000);

// eval constants
TUNE_PARAM(EvalScaleBias, 680, 600, 800);
TUNE_PARAM(EvalShuffleCoef, 5, 1, 10);

// history constants
TUNE_PARAM(HistoryBonusMargin, 300, 250, 500);
TUNE_PARAM(HistoryBonusBias, 300, 200, 600);
TUNE_PARAM(HistoryBonusMax, 2400, 1800, 3000);

// universal constants
TUNE_PARAM(SeeValPawn, 93, 80, 120);
TUNE_PARAM(SeeValKnight, 308, 280, 350);
TUNE_PARAM(SeeValBishop, 346, 320, 380);
TUNE_PARAM(SeeValRook, 521, 450, 600);
TUNE_PARAM(SeeValQueen, 994, 900, 1100);