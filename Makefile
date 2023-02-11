CC   := clang
LIB  := src/gc.c
MAIN := playground.c
OUT  := bin/playground

ERROR_FLAGS  := -Wall -Wpedantic -Wextra -Werror
DEBUG_FLAGS  := -Og -g -fsanitize=address -fsanitize=undefined
OPT_FLAGS    := -Ofast

debug:
	${CC} ${DEBUG_FLAGS} ${ERROR_FLAGS} ${MAIN} ${LIB} -o ${OUT}_debug

opt:
	${CC} ${OPT_FLAGS} ${ERROR_FLAGS} ${MAIN} ${LIB} -o ${OUT}_opt
