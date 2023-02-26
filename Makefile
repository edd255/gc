CC   := clang
LIB  := src/gc.c
MAIN := src/playground.c
OUT  := bin/playground

ERROR_FLAGS    := -Wall -Wpedantic -Wextra -Werror
DEBUG_FLAGS    := -Og -g -fsanitize=address -fsanitize=undefined
OPT_FLAGS      := -Ofast
VALGRIND_FLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
MAKEFLAGS      := --jobs=$(shell nproc)

all: debug opt

debug:
	${CC} ${DEBUG_FLAGS} ${ERROR_FLAGS} ${MAIN} ${LIB} -o ${OUT}_debug

opt:
	${CC} ${OPT_FLAGS} ${ERROR_FLAGS} ${MAIN} ${LIB} -o ${OUT}_opt

memcheck: 
	valgrind ${VALGRIND_FLAGS} ./${OUT}_debug
