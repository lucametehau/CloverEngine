#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>
#include "binpack.h"

int main(int argc, char* argv[]) {
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
                    int game_count = 0;
                    for (size_t i = 0; i < file_size; ) {
                        if (i + 32 >= file_size)
                            break;
                        if (game_count == 221412 || game_count == 221411) {
                            PackedBoard pb;
                            std::vector<uint8_t> packed_data(buffer.begin() + i, buffer.begin() + i + 48);
                            std::memcpy(&pb, buffer.data() + i, sizeof(PackedBoard));
                            Board board;
                            pb.unpack(board);
                            board.print();
                            for (int i = 0; i < 48; i++) {
                                std::cout << int(packed_data[i]) << " ";
                            }
                            for (int i = 0; i < 16; i += 4) {
                                uint16_t move;
                                int16_t score;
                                std::memcpy(&move, packed_data.data() + i, sizeof(uint16_t));
                                std::memcpy(&score, packed_data.data() + i + 2, sizeof(int16_t));
                                std::cout << (move & 63) << " " << ((move >> 6) & 63) << " " << (move >> 12) << " " << score << std::endl;
                            }
                            std::cout << std::endl;
                        }
                        i += 32;
                        while (i + 3 < file_size && !(buffer[i] == 0 && buffer[i + 1] == 0 && buffer[i + 2] == 0 && buffer[i + 3] == 0)) {
                            if (game_count == 221411 || game_count == 221412) {
                                uint16_t move;
                                int16_t score;
                                std::memcpy(&move, buffer.data() + i, sizeof(uint16_t));
                                std::memcpy(&score, buffer.data() + i + 2, sizeof(int16_t));
                                auto square_in__san_format = [&](uint16_t square) {
                                    std::string sq = "";
                                    sq += 'a' + (square % 8);
                                    sq += '1' + (square / 8);
                                    return sq;
                                };
                                std::cout << square_in__san_format(move & 63) << " " << square_in__san_format((move >> 6) & 63) << " " << (move >> 12) << " " << score << std::endl;
                            }
                            i += 4;
                        }
                        if (i + 3 < file_size)
                            last_zero_pos = i + 3;
                        i += 4;
                        if (game_count == 221412 || game_count == 221411) {
                            std::cout << "Game count: " << game_count << std::endl;
                        }
                        game_count++;
                    }

                    // if (last_zero_pos != std::string::npos) {
                    //     std::cout << "Last zero position: " << last_zero_pos << " File size: " << file_size << std::endl;

                    //     std::string temp_path = entry.path().string();
                    //     std::ofstream outFile(temp_path, std::ios::binary | std::ios::trunc);
                    //     if (outFile.is_open()) {
                    //         outFile.write(buffer.data(), last_zero_pos + 1);
                    //         outFile.close();
                    //     }
                    // } else {
                    //      std::cout << "No null terminator found in " << filename << std::endl;
                    // }
                } else {
                    std::cerr << "Error opening file: " << filename << std::endl;
                }
            }
        }
    }

    return 0;
}
