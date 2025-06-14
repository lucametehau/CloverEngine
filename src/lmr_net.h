#pragma once

#include "params.h"
#include <array>

constexpr int INPUT_SIZE = 22;
constexpr int L2_SIZE = 8;
constexpr int L3_SIZE = 1;

std::array<double, INPUT_SIZE * L2_SIZE> input_weights;

std::array<double, L2_SIZE> input_biases;

std::array<double, L2_SIZE> l2_weights;

double l2_biases;

constexpr std::array<double, INPUT_SIZE> means = {
    6.052971661385465,    7.403703288471869,    0.8494777524258209, 0.23117956002582285, 0.2605247239073334,
    0.31164683546799,     0.040395030578402435, 0.8120013453511465, 5.902212252551486,   27.343499132667393,
    1180.0386622133738,   1183.2996298436337,   0.172057413968983,  0.17767998383607414, 0.4826277837187871,
    -1863.4423119095609,  -29.14443551924148,   39.332582014675666, 0.04788994239137398, 0.1303256562898862,
    0.013452279458508484, 0.28066033086767755};

constexpr std::array<double, INPUT_SIZE> stddevs = {
    3.290717758894745,   6.568045236456167,   0.3575825786575691, 0.4215869673650857,  0.699052522070873,
    0.46316636904116626, 0.19688390508869016, 0.3907110959507992, 1.0093552591133754,  32.82536789572978,
    6335.004738825656,   6339.912684720735,   0.3774303382973989, 0.38224312574601854, 0.49969811496690536,
    10467.99995984764,   343.803490774988,    1476.807450108158,  0.21353335994435366, 0.3366613723053131,
    0.11520119633006727, 0.4493218329270493};

void init_lmr_net()
{
    input_weights = {
        l1_weight0,   l1_weight1,   l1_weight2,   l1_weight3,   l1_weight4,   l1_weight5,   l1_weight6,   l1_weight7,
        l1_weight8,   l1_weight9,   l1_weight10,  l1_weight11,  l1_weight12,  l1_weight13,  l1_weight14,  l1_weight15,
        l1_weight16,  l1_weight17,  l1_weight18,  l1_weight19,  l1_weight20,  l1_weight21,  l1_weight22,  l1_weight23,
        l1_weight24,  l1_weight25,  l1_weight26,  l1_weight27,  l1_weight28,  l1_weight29,  l1_weight30,  l1_weight31,
        l1_weight32,  l1_weight33,  l1_weight34,  l1_weight35,  l1_weight36,  l1_weight37,  l1_weight38,  l1_weight39,
        l1_weight40,  l1_weight41,  l1_weight42,  l1_weight43,  l1_weight44,  l1_weight45,  l1_weight46,  l1_weight47,
        l1_weight48,  l1_weight49,  l1_weight50,  l1_weight51,  l1_weight52,  l1_weight53,  l1_weight54,  l1_weight55,
        l1_weight56,  l1_weight57,  l1_weight58,  l1_weight59,  l1_weight60,  l1_weight61,  l1_weight62,  l1_weight63,
        l1_weight64,  l1_weight65,  l1_weight66,  l1_weight67,  l1_weight68,  l1_weight69,  l1_weight70,  l1_weight71,
        l1_weight72,  l1_weight73,  l1_weight74,  l1_weight75,  l1_weight76,  l1_weight77,  l1_weight78,  l1_weight79,
        l1_weight80,  l1_weight81,  l1_weight82,  l1_weight83,  l1_weight84,  l1_weight85,  l1_weight86,  l1_weight87,
        l1_weight88,  l1_weight89,  l1_weight90,  l1_weight91,  l1_weight92,  l1_weight93,  l1_weight94,  l1_weight95,
        l1_weight96,  l1_weight97,  l1_weight98,  l1_weight99,  l1_weight100, l1_weight101, l1_weight102, l1_weight103,
        l1_weight104, l1_weight105, l1_weight106, l1_weight107, l1_weight108, l1_weight109, l1_weight110, l1_weight111,
        l1_weight112, l1_weight113, l1_weight114, l1_weight115, l1_weight116, l1_weight117, l1_weight118, l1_weight119,
        l1_weight120, l1_weight121, l1_weight122, l1_weight123, l1_weight124, l1_weight125, l1_weight126, l1_weight127,
        l1_weight128, l1_weight129, l1_weight130, l1_weight131, l1_weight132, l1_weight133, l1_weight134, l1_weight135,
        l1_weight136, l1_weight137, l1_weight138, l1_weight139, l1_weight140, l1_weight141, l1_weight142, l1_weight143,
        l1_weight144, l1_weight145, l1_weight146, l1_weight147, l1_weight148, l1_weight149, l1_weight150, l1_weight151,
        l1_weight152, l1_weight153, l1_weight154, l1_weight155, l1_weight156, l1_weight157, l1_weight158, l1_weight159,
        l1_weight160, l1_weight161, l1_weight162, l1_weight163, l1_weight164, l1_weight165, l1_weight166, l1_weight167,
        l1_weight168, l1_weight169, l1_weight170, l1_weight171, l1_weight172, l1_weight173, l1_weight174, l1_weight175};
    input_biases = {l1_bias0, l1_bias1, l1_bias2, l1_bias3, l1_bias4, l1_bias5, l1_bias6, l1_bias7};
    l2_weights = {l2_weight0, l2_weight1, l2_weight2, l2_weight3, l2_weight4, l2_weight5, l2_weight6, l2_weight7};
    l2_biases = l2_bias;
}

class LMRNet
{
  public:
    LMRNet() = default;

    int predict(const std::array<int, INPUT_SIZE> &input)
    {
        double result = l2_biases;

        std::array<double, INPUT_SIZE> scaled;
        for (int i = 0; i < INPUT_SIZE; ++i)
        {
            scaled[i] = 1.0 * (input[i] - means[i]) / stddevs[i];
        }

        for (int i = 0; i < L2_SIZE; ++i)
        {
            output[i] = 0.0;
            for (int j = 0; j < INPUT_SIZE; ++j)
            {
                output[i] += scaled[j] * input_weights[i * INPUT_SIZE + j];
            }
            output[i] += input_biases[i];
            output[i] = std::max(0.0, output[i]);
        }

        for (int i = 0; i < L2_SIZE; ++i)
        {
            result += output[i] * l2_weights[i];
        }
        return round(result);
    }

    std::array<double, L2_SIZE> output;
};