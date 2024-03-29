#==== DEFINITIONS ==============================================================
#---- TARGET -------------------------------------------------------------------

NAME    := gc
VERSION := 0.1

#---- COLORS -------------------------------------------------------------------

BOLD   := \x1b[1m
NOBOLD := \x1b[0m

#---- TOOLS --------------------------------------------------------------------

CCACHE := ccache
CC     := $(CCACHE) clang
LD     := $(CCACHE) clang
RM     := rm --force
MKDIR  := mkdir --parents
Q      ?= @

#---- DIRECTORIES --------------------------------------------------------------

SRC_DIR   := lib tests
SRC_DIRS  := $(SRC_DIR) $(wildcard ./deps/*)
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
MAKEFLAGS := --jobs=$(shell nproc)

ERR := -Wall #-Wpedantic -Wextra -Werror
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
	$(Q)echo -e "${BOLD}====> LD $@${NOBOLD}"
	$(Q)$(CC) $(RELEASE) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.opt.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(RELEASE) $(CFLAGS) -c $< -o $@

release: $(BIN)_release

#---- DEBUGGING ----------------------------------------------------------------

$(BIN)_debugging: $(patsubst src/%.c, build/%.dbg.o, $(SRCS)) 
	$(Q)$(MKDIR) $(BIN_DIR)
	$(Q)echo -e "${BOLD}====> LD $@${NOBOLD}"
	$(Q)$(CC) $(DEBUGGING) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.dbg.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(DEBUGGING) $(CFLAGS) -c $< -o $@

debugging: $(BIN)_debugging

#---- MEMCHECK -----------------------------------------------------------------

$(BIN)_sanitized: $(patsubst src/%.c, build/%.san.o, $(SRCS)) 
	$(Q)$(MKDIR) $(BIN_DIR)
	$(Q)echo -e "${BOLD}====> LD $@${NOBOLD}"
	$(Q)$(CC) $(MEMCHECK) $+ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.san.o: src/%.c
	$(Q)echo "====> CC $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(MEMCHECK) $(CFLAGS) -c $< -o $@

memcheck: $(BIN)_sanitized

#---- CLEANING -----------------------------------------------------------------

clean:
	$(Q)$(RM) --recursive $(BUILD_DIR)

#---- STYLE --------------------------------------------------------------------

style:
	$(Q)echo "====> Formatting..."
	$(Q)find $(SRC_DIR) -iname *.h -o -iname *.c | xargs clang-format -i

#==== EPILOGUE =================================================================

all: style release debugging memcheck

# Include the .d makefiles
-include $(DEPS)
.PHONY: all clean style
