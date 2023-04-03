CC  = g++
SRC = *.cpp tbprobe.c
EXE = Clover.4.0-lim
EVALFILE = Clover_nn.nn

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall
RFLAGS = $(WFLAGS) -std=c++17 -O3 -fopenmp

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
endif

LIBS   = -pthread

AVX2FLAGS     = -march=core-avx2
NATIVEFLAGS   = -march=native
OLDFLAGS      = -march=core2
AVX512FLAGS   = $(AVX2FLAGS) -mavx512f -mavx512bw -mavx512dq
EVALFILEFLAGS = -DEVALFILE=\"$(EVALFILE)\"

ob:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o Clover$(EXT)
native:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native$(EXT)
debug:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -g -o $(EXE)-debug$(EXT)
old:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(OLDFLAGS) -g -o $(EXE)-old$(EXT)
avx2:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
avx512:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(AVX512FLAGS) -o $(EXE)-avx512$(EXT)

release:
	make old
	make avx2
	make avx512
