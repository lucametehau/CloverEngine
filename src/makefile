SRC = *.cpp Fathom/src/tbprobe.c
EXE = Clover.6.2.3
EVALFILE = weights-5buckets-epoch390.nnue

ifeq ($(CXX),)
    CXX = g++
endif

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall -g
RFLAGS = $(WFLAGS) -std=c++17 -O3 -funroll-loops
LIBS =

FLAGS = $(shell echo | $(CXX) -march=native -E -dM -)

ifneq ($(findstring __BMI2__, $(FLAGS)),)
	ifeq ($(findstring __znver1, $(FLAGS)),)
		ifeq ($(findstring __znver2, $(FLAGS)),)
			PEXT_FLAGS += -DPEXT_GOOD
		endif
	endif
endif

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
	LIBS += -pthread
else
	LIBS += -lpthread
endif

ifneq ($(findstring g++, $(CXX)),)
	PGO_GENERATE = -fprofile-generate
	PGO_USE      = -fprofile-use
endif
ifneq ($(findstring clang++, $(CXX)),)
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
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT) $(PGO_GENERATE)
	./$(EXE)$(EXT) bench 15
	$(PGO_MERGE)
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT) $(PGO_USE)
generate:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -DGENERATE -o Clover-generator$(EXT)
debug:
	clang++ $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT)
native:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)$(EXT)
old:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(OLDFLAGS) -o $(EXE)-old$(EXT) $(PGO_GENERATE)
	./$(EXE)-old$(EXT) bench 15
	$(PGO_MERGE)
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(OLDFLAGS) -o $(EXE)-old$(EXT) $(PGO_USE)
avx2:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT) $(PGO_GENERATE)
	./$(EXE)-avx2$(EXT) bench 15
	$(PGO_MERGE)
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT) $(PGO_USE)
avx512:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(AVX512FLAGS) -o $(EXE)-avx512$(EXT) $(PGO_GENERATE)
	./$(EXE)-avx512$(EXT) bench 15
	$(PGO_MERGE)
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(AVX512FLAGS) -o $(EXE)-avx512$(EXT) $(PGO_USE)
tune:
	$(CXX)   $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(PEXT_FLAGS) $(LIBS) $(NATIVEFLAGS) -DTUNE_FLAG -o $(EXE)$(EXT)

release:
	make old
	make avx2
	make avx512
