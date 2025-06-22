#pragma once

#include "params.h"
#include <array>

constexpr int LMR_CONDITIONS = 15;

std::array<int, LMR_CONDITIONS> lmr_single_weight = {
    lsw0, lsw1, lsw2, lsw3, lsw4, lsw5, lsw6, lsw7, lsw8, lsw9, lsw10, lsw11, lsw12, lsw13, lsw14,
};

std::array<int, LMR_CONDITIONS *(LMR_CONDITIONS - 1) / 2> lmr_double_weight = {
    ldw0,  ldw1,  ldw2,  ldw3,  ldw4,  ldw5,  ldw6,  ldw7,  ldw8,  ldw9,  ldw10,  ldw11,  ldw12,  ldw13,  ldw14,
    ldw15, ldw16, ldw17, ldw18, ldw19, ldw20, ldw21, ldw22, ldw23, ldw24, ldw25,  ldw26,  ldw27,  ldw28,  ldw29,
    ldw30, ldw31, ldw32, ldw33, ldw34, ldw35, ldw36, ldw37, ldw38, ldw39, ldw40,  ldw41,  ldw42,  ldw43,  ldw44,
    ldw45, ldw46, ldw47, ldw48, ldw49, ldw50, ldw51, ldw52, ldw53, ldw54, ldw55,  ldw56,  ldw57,  ldw58,  ldw59,
    ldw60, ldw61, ldw62, ldw63, ldw64, ldw65, ldw66, ldw67, ldw68, ldw69, ldw70,  ldw71,  ldw72,  ldw73,  ldw74,
    ldw75, ldw76, ldw77, ldw78, ldw79, ldw80, ldw81, ldw82, ldw83, ldw84, ldw85,  ldw86,  ldw87,  ldw88,  ldw89,
    ldw90, ldw91, ldw92, ldw93, ldw94, ldw95, ldw96, ldw97, ldw98, ldw99, ldw100, ldw101, ldw102, ldw103, ldw104,
};

int compute_lmr(std::array<bool, LMR_CONDITIONS> &lmr_conditions)
{
    int lmr = 0;
    for (int i = 0; i < LMR_CONDITIONS; i++)
    {
        if (lmr_conditions[i])
        {
            lmr += lmr_single_weight[i];
        }
    }

    for (int i = 0, cnt = 0; i < LMR_CONDITIONS; i++)
    {
        for (int j = i + 1; j < LMR_CONDITIONS; j++, cnt++)
        {
            if (lmr_conditions[i] && lmr_conditions[j])
            {
                lmr += lmr_double_weight[cnt];
            }
        }
    }

    return lmr;
}