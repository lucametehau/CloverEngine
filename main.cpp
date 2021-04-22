#include <iostream>
#pragma GCC optimize("Ofast")
#pragma GCC target("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,tune=native")
#include "init.h"
#include "perft.h"
#include "play.h"
#include "uci.h"

using namespace std;
//Info info[1];

int main() {
  unique_ptr <Search> searcher(new Search);

  searcher->_setFen(START_POS_FEN);

  UCI uci(*searcher.get());
  uci.Uci_Loop();
  return 0;
}
