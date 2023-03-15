#==== DEFINITIONS ==============================================================
#---- TARGET -------------------------------------------------------------------

NAME    := gc
VERSION := 0.1

#---- COLORS -------------------------------------------------------------------

BOLD   := \x1b[1m
NOBOLD := \x1b[0m

#---- TOOLS --------------------------------------------------------------------

CC    := clang
LD    := clang
RM    := rm --force
MKDIR := mkdir --parents
Q     ?= @

#---- DIRECTORIES --------------------------------------------------------------

SRC_DIRS  := src $(wildcard ./deps/*)
BUILD_DIR := build
BIN_DIR   := bin
INC_DIRS  := $(shell find $(SRC_DIRS) -type d)

#---- FILES --------------------------------------------------------------------

BIN  := $(BIN_DIR)/$(NAME)
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

#---- FLAGS --------------------------------------------------------------------

LDFLAGS   += -ledit
CFLAGS    := $(INC_FLAGS) -MMD -MP
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

ERR := -Wall -Wpedantic -Wextra -Werror
OPT := -Ofast -DNDEBUG
DBG := -Og -g 
SAN := -fsanitize=address \
	   -fsanitize=pointer-compare \
	   -fsanitize=pointer-subtract \
	   -fsanitize=shadow-call-stack \
	   -fsanitize=leak \
	   -fsanitize=undefined \
	   -fsanitize-address-use-after-scope

RELEASE   := ${ERR} ${OPT}
DEBUGGING := ${ERR} ${DBG}
MEMCHECK  := ${ERR} ${DBG} ${SAN}

#==== RULES ====================================================================
#---- RELEASE ------------------------------------------------------------------

$(BIN)_release: $(patsubst src/%.c, build/%.opt.o, $(SRCS)) 
	$(Q)$(MKDIR) $(BIN_DIR)
	$(Q)echo -e "${BOLD}====> LD $@\n${NOBOLD}"
	$(CC) $(RELEASE) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.opt.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(CC) $(RELEASE) $(CFLAGS) -c $< -o $@

#---- DEBUGGING ----------------------------------------------------------------

$(BIN)_debugging: $(patsubst src/%.c, build/%.dbg.o, $(SRCS)) 
	$(Q)$(MKDIR) $(BIN_DIR)
	$(Q)echo -e "${BOLD}====> LD $@\n${NOBOLD}"
	$(CC) $(DEBUGGING) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.dbg.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(CC) $(DEBUGGING) $(CFLAGS) -c $< -o $@

#---- MEMCHECK -----------------------------------------------------------------

$(BIN)_sanitized: $(patsubst src/%.c, build/%.san.o, $(SRCS)) 
	$(Q)$(MKDIR) $(BIN_DIR)
	$(Q)echo -e "${BOLD}====> LD $@\n${NOBOLD}"
	$(CC) $(MEMCHECK) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.san.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(CC) $(MEMCHECK) $(CFLAGS) -c $< -o $@

#---- EPILOGUE -----------------------------------------------------------------

.PHONY: all clean

clean:
	$(Q)$(RM) --recursive $(BUILD_DIR)

all: $(BIN)_release $(BIN)_debugging $(BIN)_sanitized

# Include the .d makefiles
-include $(DEPS)
