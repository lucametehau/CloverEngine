CC  = g++
SRC = *.cpp tbprobe.c

EXE = Clover.3.0-dev36
EVALFILE = chess4.nn

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
run:
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) -o $(EXE)$(EXT)
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(POPCNTFLAGS) -o $(EXE)-popcnt$(EXT)
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(BMI2FLAGS) -o $(EXE)-bmi2$(EXT)
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(POPCNTFLAGS) $(AVX2FLAGS) -o $(EXE)-avx2$(EXT)
	$(CC) $(SRC) $(EVALFILEFLAGS) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native$(EXT)