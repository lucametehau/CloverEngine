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
#include "cuckoo.h"
#include "perft.h"
#include "uci.h"
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    init_defs();
    attacks::init();
    cuckoo::init();
    load_nnue_weights();

    UCI uci;

    if (argc > 1)
    {
        if (!strncmp(argv[1], "bench", 5))
        {
            int depth = -1;
            if (argc > 2)
            {
                std::string s = argv[2];
                depth = stoi(s);
            }
            uci.bench(depth);
            return 0;
        }
#ifdef GENERATE
        std::map<std::string, std::string> args;
        for (int i = 1; i < argc; i++)
        {
            std::string s = argv[i];
            if (s[0] == '-')
            {
                if (i == argc - 1)
                {
                    std::cout << "Error! Expected value after argument!" << std::endl;
                    return 0;
                }
                args[s.substr(1)] = argv[i + 1];
                i++;
            }
        }
        uint64_t num_fens;
        std::size_t num_threads;
        std::string path;
        uint64_t seed = 0;

        if (args.find("N") == args.end() && args.find("fens") == args.end())
        {
            std::cout << "Error! Expected number of fens to generate!\n";
            return 0;
        }
        else
            num_fens = std::stoull(args.find("N") != args.end() ? args["N"] : args["fens"]);

        if (args.find("P") == args.end() && args.find("path") == args.end())
        {
            std::cout << "Error! Expected path to write data to!\n";
            return 0;
        }
        else
            path = args.find("P") != args.end() ? args["P"] : args["path"];

        if (args.find("T") == args.end() && args.find("threads") == args.end())
        {
            std::cout << "Error! Expected number of threads!\n";
            return 0;
        }
        else
            num_threads = std::stoull(args.find("T") != args.end() ? args["T"] : args["threads"]);

        if (args.find("S") != args.end())
            seed = std::stoull(args["S"]);
        else if (args.find("seed") != args.end())
            seed = std::stoull(args["seed"]);

        generateData(num_fens, num_threads, path, seed);
#endif
        return 0;
    }

#ifndef GENERATE
    uci.uci_loop();
#endif
    return 0;
}