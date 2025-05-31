SRCXX = $(wildcard *.cpp)
SRCHEADERS = $(wildcard *.h)
SRC = 3rdparty/Fathom/src/tbprobe.c
OBJ = $(SRCXX:.cpp=.o) $(SRC:.c=.o)
VERSION = 8.1.16
EXE = Clover.$(VERSION)
EVALFILE = clover-7buckets-mse-0.95-1280-cosine-600.nnue

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
	RFLAGS += -static -static-libgcc -static-libstdc++
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
BUILD_FLAGS    = $(EVALFILE_FLAGS) $(PEXT_FLAGS) -DVERSION_NAME=\"$(VERSION)\"

ifeq ($(build_flag),)
	build_flag = native
endif

ifneq ($(findstring old, $(build_flag)),)
	BUILD_FLAGS += $(OLD_FLAGS)
	PEXT_FLAGS := 
	EXE := $(EXE)-old
else ifneq ($(findstring avx2, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS) $(AVX2_FLAGS)
	EXE := $(EXE)-avx2
else ifneq ($(findstring avx512, $(build_flag)),)
	BUILD_FLAGS += -march=native $(AVX512_FLAGS)
	EXE := $(EXE)-avx512
else ifneq ($(findstring native, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS)
else ifneq ($(findstring tune, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS) -DTUNE_FLAG
	EXE := $(EXE)-tune
else ifneq ($(findstring generate, $(build_flag)),)
	BUILD_FLAGS += $(NATIVE_FLAGS) -DGENERATE
	EXE := $(EXE)-generate
endif

%.o: %.cpp
	$(CXX) $(BUILD_FLAGS) $(RFLAGS) -o $@ -c $<

# Fathom moment
%.o: %.c
	$(CXX) $(BUILD_FLAGS) $(RFLAGS) -o $@ -c $<

make: $(OBJ)
	$(CXX) $(OBJ) $(RFLAGS) $(LIBS) $(BUILD_FLAGS) -o $(EXE)$(EXT)
	make format

ifeq ($(OS),Windows_NT)
    CLANG_FORMAT_PATH := $(shell where $(FORMATTER) 2>nul)
    ifeq ($(CLANG_FORMAT_PATH),)
        CLANG_FORMAT_PATH := $(shell command -v $(FORMATTER) 2>nul)
    endif
else
    CLANG_FORMAT_PATH := $(shell command -v $(FORMATTER) 2>/dev/null)
endif

ifeq ($(CLANG_FORMAT_PATH),)
format:
	echo "clang-format undetected"
else
format: $(SRCHEADERS) $(SRCXX)
	clang-format -i $(SRCHEADERS) $(SRCXX)
endif

release:
	make build_flag=old
	make clean
	make build_flag=avx2
	make clean
	make build_flag=avx512

clean:
	$(RM_CMD)
