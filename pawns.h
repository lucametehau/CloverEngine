#pragma once
#include "defs.h"

const int PT_SIZE = 65535;

struct ptEntry {
  uint64_t pawnKey;
  uint64_t passers;
  int eval[2];
};

class pTable {
public:
  ptEntry *table;

  pTable();
  ~pTable();
  void initpTable();
  pTable(const pTable &) = delete;
  pTable& operator = (const pTable&) = delete;

  void save(uint64_t pawnKey, int eval[], uint64_t passers);

  bool probe(uint64_t pawnKey, ptEntry &entry);
};

pTable :: pTable() {
  initpTable();
}

pTable :: ~pTable() {
  delete[] table;
}

void pTable :: initpTable() {
  table = new ptEntry[PT_SIZE + 1]();
  memset(table, 0, PT_SIZE + 1);
}

void pTable :: save(uint64_t pawnKey, int eval[], uint64_t passers) {
  ptEntry entry;

  entry.pawnKey = pawnKey;
  for(int i = 0; i < 2; i++)
    entry.eval[i] = eval[i];
  entry.passers = passers;
  table[pawnKey & PT_SIZE] = entry;
}

bool pTable :: probe(uint64_t pawnKey, ptEntry &entry) {
  int ind = pawnKey & PT_SIZE;
  entry = table[ind];

  //return 0;
  if(TUNE)
    return 0;
  return (entry.pawnKey == pawnKey);
}
