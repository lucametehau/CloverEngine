#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::vector<std::string> files;

    std::cout << "./target/release/bullet-utils.exe viribinpack interleave ";
    for (auto &filename : {
        "G:\\CloverData\\testrun20_20k",
        "G:\\CloverData\\testrun21_20k",
        "G:\\CloverData\\testrun22_20k_dfrc",
        "G:\\CloverData\\testrun23_20k",
        "G:\\CloverData\\testrun24_20k_dfrc_std\\Clover_data",
        "G:\\CloverData\\testrun25_20k_dfrc",
        "G:\\CloverData\\testrun26_20k_dfrc",
        "G:\\CloverData\\testrun27_20k",
        "G:\\CloverData\\testrun28_20k_dfrc",
        "G:\\CloverData\\testrun29_20k_dfrc",
        "G:\\CloverData\\testrun30_20k",
    })
        std::cout << filename << " ";
    std::cout << "--output G:\\CloverData\\interleaved20-30.bin\n";
    return 0;
}
