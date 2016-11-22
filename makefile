SHELL=/bin/bash

CPPFLAGS += -std=c++11 -W -Wall  -g
CPPFLAGS += -O3
CPPFLAGS += -I include

ifeq ($(OS),Windows_NT)
LDLIBS += -lws2_32
else
LDLIBS += -lrt
endif

all : bin/execute_puzzle bin/create_puzzle_input bin/run_puzzle bin/compare_puzzle_output

lib/libpuzzler.a : $(wildcard provider/*.cpp provider/*.hpp include/puzzler/*.hpp include/puzzler/*/*.hpp)
	cd provider && $(MAKE) all

bin/% : src/%.cpp lib/libpuzzler.a
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) -Llib -lpuzzler


serenity_now_% : all
	mkdir -p w
	bin/run_puzzle $* 100 2
	bin/create_puzzle_input $* 100 2 > w/$*.in
	cat w/$*.in | bin/execute_puzzle 1 2 > w/$*.ref.out
	cat w/$*.in | bin/execute_puzzle 0 2 > w/$*.got.out
	bin/compare_puzzle_output w/$*.ref.out w/$*.got.out 2

serenity_now : $(foreach x,julia ising_spin logic_sim random_walk,serenity_now_$(x))
