#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>
#include "binpack.h"

int pad(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_directory> <prefix>" << std::endl;
        return 1;
    }

    std::string directory_path_str = argv[1];
    std::string prefix = argv[2];
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
                std::cout << filename << std::endl;
                std::ifstream file(entry.path(), std::ios::binary);
                if (file.is_open()) {
                    file.seekg(0, std::ios::end);
                    std::streampos file_size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    
                    std::vector<char> buffer(file_size);
                    file.read(buffer.data(), file_size);
                    file.close();

                    size_t last_zero_pos = std::string::npos;
                    int count = 0, fens = 0;
                    bool flag = false;
                    for (size_t i = 0; i < file_size; ) {
                        if (i + 32 >= file_size)
                            break;
                        char wdl = buffer[i + 30];
                        int j = i;
                        // flag = (i == 9176124);
                        if (flag) {
                            PackedBoard packed_board;
                            std::memcpy(&packed_board, &buffer[j], sizeof(PackedBoard));
                            Board board;
                            packed_board.unpack(board);
                            std::cout << "Board: " << board.fen() << "\n";
                        }
                        i += 32;
                        while (i + 3 < file_size && !(buffer[i] == 0 && buffer[i + 1] == 0 && buffer[i + 2] == 0 && buffer[i + 3] == 0)) {
                            int16_t score;
                            uint16_t move;
                            std::memcpy(&score, &buffer[i + 2], sizeof(int16_t));
                            std::memcpy(&move, &buffer[i], sizeof(move));
                            auto sq_to_string = [&](int sq) {
                                char file = 'a' + sq % 8, rank = '1' + sq / 8;
                                std::string s;
                                s += file, s += rank;
                                return s;
                            };
                            if (wdl == 0 && score > 400) count++;
                            else if (wdl == 1 && abs(score) > 400) count++;
                            else if (wdl == 2 && score < -400) count++;
                            i += 4;
                            fens++;
                        }
                        if (i + 3 < file_size)
                            last_zero_pos = i + 3;
                        i += 4;

                        if (flag)
                            return 1;
                    }

                    if (last_zero_pos != std::string::npos) {
                        std::cout << "Last zero position: " << last_zero_pos << " File size: " << file_size << std::endl;
                        std::cout << count << " out of " << fens << "\n";
                        std::string temp_path = entry.path().string();
                        std::ofstream outFile(temp_path, std::ios::binary | std::ios::trunc);
                        if (outFile.is_open()) {
                            outFile.write(buffer.data(), last_zero_pos + 1);
                            outFile.close();
                        }
                    } else {
                         std::cout << "No null terminator found in " << filename << std::endl;
                    }
                } else {
                    std::cerr << "Error opening file: " << filename << std::endl;
                }
            }
        }
    }

    return 0;
}
