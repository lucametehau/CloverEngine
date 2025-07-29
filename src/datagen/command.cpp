#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::vector<std::string> files;
    for (auto directory_path_str : {"G:\\CloverData\\testrun8_full_adj", "G:\\CloverData\\testrun9_normal", "G:\\CloverData\\testrun10_normal_20k"}) {
        std::string prefix = "data";
        std::filesystem::path directory_path(directory_path_str);

        if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) {
            std::cerr << "Error: The path '" << directory_path_str << "' does not exist or is not a directory." << std::endl;
            return 1;
        }

        std::cout << "Files in '" << directory_path_str << "' starting with prefix '" << prefix << "':" << std::endl;

        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                std::string filename = entry.path().filename().string();
                if (!filename.ends_with(".bin")) continue;
                if (filename.rfind(prefix, 0) == 0) {
                    std::string full_path = directory_path_str;
                    full_path += '\\';
                    full_path += filename;
                    files.push_back(full_path);
                }
            }
        }
    }

    std::cout << "./target/release/bullet-utils.exe viribinpack interleave ";
    for (auto &filename : files)
        std::cout << filename << " ";
    std::cout << "--output G:\\CloverData\\interleaved.bin\n";
    return 0;
}
