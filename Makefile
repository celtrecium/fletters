CXX=g++
CXXFLAGS=-Iinclude -pedantic -Wall -Wextra -Wsign-conversion -Wconversion -Wshadow
LIBS=-ltdl -lsbvector -lu8string -ltdlcpp -lqwerty
OBJS=main.o
SRC=src

all: options main.o fletters

options:
	@echo Build options:
	@echo "CXX      = $(CXX)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo

clean:
	rm -rf invaders

main.o: main.cpp
	${CXX} ${CXXFLAGS} -c $^

fletters: ${OBJS}
	${CXX} ${LIBS} $^ -o $@
	rm -rf ${OBJS}
