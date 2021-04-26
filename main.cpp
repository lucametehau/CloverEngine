#include <iostream>
#include "init.h"
#include "perft.h"
#include "play.h"
#include "uci.h"
//Info info[1];

int main() {
  init_defs();
  initAttacks();

  std::unique_ptr <Search> searcher(new Search);

  //std::cout << searcher->getThreadCount() << "\n";

  searcher->_setFen(START_POS_FEN);

  UCI uci(*searcher.get());
  uci.Uci_Loop();
  return 0;
}
