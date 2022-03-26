CC  = g++
SRC = *.cpp tbprobe.c
EXE = Clover.3.1
EVALFILE = Clover_3_1_590mil_e32_512_eval.nn

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif

WFLAGS = -Wall
RFLAGS = $(WFLAGS) -std=c++17 -O3

ifeq ($(EXT), .exe)
	RFLAGS += -static -static-libgcc -static-libstdc++
endif

LIBS   = -pthread


POPCNTFLAGS   = -mpopcnt -msse3
BMI2FLAGS     = $(POPCNTFLAGS) -mbmi2
AVX2FLAGS     = $(POPCNTFLAGS) -msse -mavx2 -msse4.1 -mssse3 -msse2
NATIVEFLAGS   = -march=native
EVALFILEFLAGS = -DEVALFILE=\"$(EVALFILE)\"

ob:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o Clover$(EXT)
native:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native$(EXT)
debug:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -g -o $(EXE)-debug$(EXT)
run:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(POPCNTFLAGS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
