# Desktop simulator: builds the firmware's engine for the PC.
#   make -C sim            build ./sim/sim
#   ./sim/sim              usage
CXX ?= g++
CXXFLAGS ?= -O2 -g -std=c++17 -Wall -Wextra -Wno-unused-parameter
ENGINE := $(wildcard ../src/engine/*.cpp ../src/engine/modes/*.cpp)
SRC := sim.cpp Smf.cpp $(ENGINE)
HDR := $(wildcard ../src/engine/*.h ../src/*.h *.h)
OUT ?= out

sim: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f sim

.PHONY: clean
