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
#include <iostream>
#include <string>
#include "init.h"
#include "perft.h"
#include "uci.h"

int main(int argc, char** argv) {
    init_defs();
    initAttacks();

    std::unique_ptr <Search> searcher(new Search);

    searcher->_setFen(START_POS_FEN);

    UCI uci(*searcher.get());
    if (argc > 1) {
        if (!strncmp(argv[1], "bench", 5)) {
            int depth = -1;
            if (argc > 2) {
                std::string s = argv[2];
                depth = stoi(s);
            }
            uci.Bench(depth);
            return 0;
        }
    }

    uci.Uci_Loop();
    return 0;
}