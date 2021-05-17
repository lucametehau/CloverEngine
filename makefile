CC  = g++
SRC = *.cpp *.c
EXE = Clover.2.3

WFLAGS = -Wall
RFLAGS = $(WFLAGS) -std=c++17 -O3 -static -static-libgcc -static-libstdc++
LIBS   = -pthread


POPCNTFLAGS = -mpopcnt -msse3
BMI2FLAGS   = $(POPCNTFLAGS) -mbmi2
AVX2FLAGS   = -msse -msse3 -mpopcnt -mavx2 -msse4.1 -mssse3 -msse2
NATIVEFLAGS = -march=native

run:
	$(CC) $(SRC) $(RFLAGS) $(LIBS) -o $(EXE).exe
	$(CC) $(SRC) $(RFLAGS) $(LIBS) $(POPCNTFLAGS) -o $(EXE)-popcnt.exe
	$(CC) $(SRC) $(RFLAGS) $(LIBS) $(BMI2FLAGS) -o $(EXE)-bmi2.exe
	$(CC) $(SRC) $(RFLAGS) $(LIBS) $(POPCNTFLAGS) $(AVX2FLAGS) -o $(EXE)-avx2.exe
	$(CC) $(SRC) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native.exe
native:
	$(CC) $(SRC) $(RFLAGS) $(LIBS) $(NATIVEFLAGS) -o $(EXE)-native.exe