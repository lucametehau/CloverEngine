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
        if (!strncmp(argv[1], "generate", 8))
        {
            if (argc < 5)
            {
                std::cout << "Wrong number of arguments!\n";
                return 0;
            }
            uint64_t nrFens = stoull(std::string(argv[2])), nrThreads = stoi(std::string(argv[3])), extraSeed = 0;
            std::string rootPath = std::string(argv[4]);
            if (argc == 6)
                extraSeed = stoull(std::string(argv[5]));
            generateData(nrFens, nrThreads, rootPath, extraSeed);
            return 0;
        }
    }

    uci.uci_loop();
    return 0;
}