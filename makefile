CC  = g++
SRC = *.cpp tbprobe.c
EXE = Clover.3.3.18-4buckets-4
EVALFILE = 4buckets_768_3.nn

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall -mno-avx512f
RFLAGS = $(WFLAGS) -std=c++17 -O3 -fopenmp

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
endif

LIBS   = -pthread

AVX2FLAGS     = -march=core-avx2
NATIVEFLAGS   = -march=native
EVALFILEFLAGS = -DEVALFILE=\"$(EVALFILE)\"

ob:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o Clover$(EXT)
native:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native$(EXT)
debug:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -g -o $(EXE)-debug$(EXT)
run:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
