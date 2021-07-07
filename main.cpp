#include <iostream>
#include "init.h"
#include "perft.h"
#include "play.h"
#include "uci.h"
//Info info[1];

//Search sr[1];

int main(int argc, char **argv) {

  init_defs();
  initAttacks();
  initPSQT();

  std::unique_ptr <Search> searcher(new Search);

  searcher->_setFen(START_POS_FEN);

  UCI uci(*searcher.get());
  if (argc > 1 && !strncmp(argv[1], "bench", 5)) {
    uci.Bench();
    return 0;
  }
  uci.Uci_Loop();
  return 0;
}
