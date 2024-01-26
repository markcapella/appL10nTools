
# *****************************************************
# Variables to control Compile / Link.

CC = g++
CFLAGS = -Wall -O3 -s

# ****************************************************
# Compile any single FOO.cpp file and link it as FOO.

all: $(patsubst %.cpp, %.target, $(wildcard *.cpp))

%.target: %.cpp Makefile
	$(CC) $(CFLAGS) $< -o $(@:.target=)
	@echo "Build "$(@:.target=)" Complete!"
	@echo

# ****************************************************
# make clean

clean: $(patsubst %.cpp, %.clean, $(wildcard *.cpp))
	@echo "Done!"
	@echo

%.clean:
	rm -f $(@:.clean=)
