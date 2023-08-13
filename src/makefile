CC  = g++
SRC = *.cpp Fathom/src/tbprobe.c
EXE = Clover.6.0.3
EVALFILE = trinity.nnue

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall -g
RFLAGS = $(WFLAGS) -std=c++17 -O3
LIBS =

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
	LIBS += -pthread
else
	LIBS += -lpthread
endif

ifneq ($(findstring g++, $(CC)),)
	PGO_GENERATE = -fprofile-generate
	PGO_USE      = -fprofile-use
else ifneq ($(findstring clang++, $(CC)),)
	PGO_MERGE    = llvm-profdata merge -output=clover.profdata *.profraw
	PGO_GENERATE = -fprofile-instr-generate
	PGO_USE      = -fprofile-instr-use=clover.profdata
endif

AVX2FLAGS     = -march=core-avx2
NATIVEFLAGS   = -mno-avx512f -march=native
OLDFLAGS      = -march=core2
AVX512FLAGS   = $(AVX2FLAGS) -mavx512f -mavx512bw -mavx512dq
EVALFILEFLAGS = -DEVALFILE=\"$(EVALFILE)\" -I . -IFathom

pgo:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT) $(PGO_GENERATE)
	./$(EXE)$(EXT) bench 15
	$(PGO_MERGE)
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT) $(PGO_USE)
generate:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -DGENERATE -o Clover-generator.exe
debug:
	git submodule update --init Fathom
	clang++ $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT)
native:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT)
old:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(OLDFLAGS) -o $(EXE)-old$(EXT)
avx2:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
avx512:
	git submodule update --init Fathom
	$(CC)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX512FLAGS) -o $(EXE)-avx512$(EXT)

release:
	git submodule update --init Fathom
	make old
	make avx2
	make avx512