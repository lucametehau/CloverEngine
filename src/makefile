SRCXX = $(wildcard *.cpp)
SRC = 3rdparty/Fathom/src/tbprobe.c
OBJ = $(SRCXX:.cpp=.o) $(SRC:.c=.o)
EXE = Clover.8.0.24
EVALFILE = clover-7buckets-mse-0.95-1280-cosine-600.bin

ifeq ($(CXX),)
    CXX = g++
endif

ifeq ($(OS), Windows_NT)
	EXT = .exe
else
	EXT = 
endif


WFLAGS = -Wall -g -flto
RFLAGS = $(WFLAGS) -std=c++20 -O3 -funroll-loops
LIBS =
RM_CMD = 

FLAGS = $(shell echo | $(CXX) -march=native -E -dM -)

ifneq ($(findstring __BMI2__, $(FLAGS)),)
	ifeq ($(findstring __znver1, $(FLAGS)),)
		ifeq ($(findstring __znver2, $(FLAGS)),)
			PEXT_FLAGS += -DPEXT_GOOD
		endif
	endif
endif

ifeq ($(EXT), .exe)
	LIBS += -pthread
	RM_CMD = powershell "rm -Force *.o, 3rdparty/Fathom/src/tbprobe.o"
else
	LIBS += -lpthread
	RM_CMD = rm -f $(OBJ)
endif

AVX2_FLAGS     = -march=core-avx2
NATIVE_FLAGS   = -mno-avx512f -march=native
OLD_FLAGS      = -march=core2
AVX512_FLAGS   = $(AVX2FLAGS) -mavx512f -mavx512bw -mavx512dq
EVALFILE_FLAGS = -DEVALFILE=\"$(EVALFILE)\" -I . -IFathom
BUILD_FLAGS    = $(EVALFILE_FLAGS) $(PEXT_FLAGS)

ifeq ($(build_flag),)
	build_flag = native
endif

ifneq ($(findstring old, $(build_flag)),)
	BUILD_FLAGS += $(OLD_FLAGS)
	EXE += -old
else ifneq ($(findstring avx2, $(build_flag)),)
	BUILD_FLAGS += $(AVX2_FLAGS)
	EXE += -avx2
else ifneq ($(findstring avx512, $(build_flag)),)
	BUILD_FLAGS += $(AVX512_FLAGS)
	EXE += -avx512
else ifneq ($(findstring native, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS)
else ifneq ($(findstring tune, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS) -DTUNE_FLAG
	EXE += -tune
else ifneq ($(findstring generate, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS) -DGENERATE
	EXE += -generate
endif

%.o: %.cpp
	$(CXX) $(BUILD_FLAGS) $(RFLAGS) -o $@ -c $<

# Fathom moment
%.o: %.c
	$(CXX) $(BUILD_FLAGS) $(RFLAGS) -o $@ -c $<

make: $(OBJ)
	$(CXX) $(OBJ) $(RFLAGS) $(LIBS) $(BUILD_FLAGS) -o $(EXE)$(EXT)

release:
	make build_flag=old
	make build_flag=avx2
	make build_flag=avx512

clean:
	$(RM_CMD)
