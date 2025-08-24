#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::vector<std::string> files;
    for (auto directory_path_str : {
        "G:\\CloverData\\testrun11_full_adj_5k", 
        "G:\\CloverData\\testrun12_full_adj_5k", 
        "G:\\CloverData\\testrun13_full_adj_20k", 
        "G:\\CloverData\\testrun14_full_adj_20k", 
        "G:\\CloverData\\testrun15_full_adj_20k",
        "G:\\CloverData\\testrun16_stm_tt_5k",
        "G:\\CloverData\\testrun17_stm_tt_20k",
        "G:\\CloverData\\testrun18_stm_tt_5k",
        "G:\\CloverData\\testrun19_stm_tt_5k",
        "G:\\CloverData\\testrun20_20k",
        "G:\\CloverData\\testrun21_20k",
        "G:\\CloverData\\testrun22_20k_dfrc",
        "G:\\CloverData\\testrun23_20k",
        "G:\\CloverData\\testrun24_20k_dfrc_std\\Clover_data",
        "G:\\CloverData\\testrun25_20k_dfrc",
        "G:\\CloverData\\testrun26_20k_dfrc",
        "G:\\CloverData\\testrun27_20k",
        "G:\\CloverData\\testrun28_20k_dfrc",
    }) {
        // std::string prefix = "data";
        // std::filesystem::path directory_path(directory_path_str);

        // if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) {
        //     std::cerr << "Error: The path '" << directory_path_str << "' does not exist or is not a directory." << std::endl;
        //     return 1;
        // }

        // std::cout << "Files in '" << directory_path_str << "' starting with prefix '" << prefix << "':" << std::endl;

        // for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        //     if (std::filesystem::is_regular_file(entry.status())) {
        //         std::string filename = entry.path().filename().string();
        //         if (!filename.ends_with(".bin")) continue;
        //         if (filename.rfind(prefix, 0) == 0) {
        //             std::string full_path = directory_path_str;
        //             full_path += '\\';
        //             full_path += filename;
        //             files.push_back(full_path);
        //         }
        //     }
        // }
        files.push_back(directory_path_str);
    }

    std::cout << "./target/release/bullet-utils.exe viribinpack interleave ";
    for (auto &filename : files)
        std::cout << filename << " ";
    std::cout << "--output G:\\CloverData\\interleaved11-28.bin\n";
    return 0;
}
