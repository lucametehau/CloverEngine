CC  = g++
SRC = *.cpp tbprobe.c
EXE = Clover.4.0.25-2
EVALFILE = Clover_nn_1bil.nn

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall -g
RFLAGS = $(WFLAGS) -std=c++17 -O3 -fopenmp

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
endif

LIBS   = -pthread

AVX2FLAGS     = -march=core-avx2
NATIVEFLAGS   = -mno-avx512f -march=native
OLDFLAGS      = -march=core2
AVX512FLAGS   = $(AVX2FLAGS) -mavx512f -mavx512bw -mavx512dq
EVALFILEFLAGS = -DEVALFILE=\"$(EVALFILE)\"

ob:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o Clover$(EXT)
native:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native$(EXT)
old:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(OLDFLAGS) -o $(EXE)-old$(EXT)
avx2:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
avx512:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX512FLAGS) -o $(EXE)-avx512$(EXT)

release:
	make old
	make avx2
	make avx512
