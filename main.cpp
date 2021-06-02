#include <iostream>
#include "init.h"
#include "perft.h"
#include "play.h"
#include "uci.h"
//Info info[1];

//Search sr[1];

int main(int argc, char **argv) {
  //sr->clearStack();
  init_defs();
  initAttacks();
  initPSQT();

  std::unique_ptr <Search> searcher(new Search);

  /*Board board;

  board.setFen(START_POS_FEN);

  uint64_t ans = 0;
  long double t1 = getTime();

  for(int i = 0; i < 10000000; i++)
    ans += evaluate(board, sr);

  long double t2 = getTime();

  std::cout << "ans = " << ans << ", time taken: " << (t2 - t1) / 1000.0 << "\n";*/

  //std::cout << searcher->getThreadCount() << "\n";

  //std::cout << sizeof(Board) << "\n";

  searcher->_setFen(START_POS_FEN);

  UCI uci(*searcher.get());
  if (argc > 1 && !strncmp(argv[1], "bench", 5)) {
    uci.Bench("benchpos.txt");
    return 0;
  }
  uci.Uci_Loop();
  return 0;
}
