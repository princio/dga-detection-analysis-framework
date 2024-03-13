#
#
#
# Makefile template for C code
#
# Author: Gustavo Pantuza Coelho Pinto
# Since: 24.03.2016
#
#
#


# Includes the project configurations
include project.conf

#
# Validating project variables defined in project.conf
#
ifndef PROJECT_NAME
$(error Missing PROJECT_NAME. Put variables at project.conf file)
endif
ifndef BINARY
$(error Missing BINARY. Put variables at project.conf file)
endif
ifndef PROJECT_PATH
$(error Missing PROJECT_PATH. Put variables at project.conf file)
endif


# Gets the Operating system name
OS := $(shell uname -s)

# Default shell
SHELL := bash

# Color prefix for Linux distributions
COLOR_PREFIX := e

ifeq ($(OS),Darwin)
	COLOR_PREFIX := 033
endif

# Color definition for print purpose
BROWN=\$(COLOR_PREFIX)[0;33m
BLUE=\$(COLOR_PREFIX)[1;34m
END_COLOR=\$(COLOR_PREFIX)[0m

PSLTSUFFIXFILE := /home/princio/Repo/princio/malware_detection_preditc_file/outdirs/pslregex2/etld.csv

# Source code directory structure
BINDIR := bin
SRCDIR := src
LOGDIR := log
LIBDIR := lib
TESTDIR := test
EXPORTDIR := export

DIRS := 


# Source code file extension
SRCEXT := c


# Defines the C Compiler
CC := gcc


# Defines the language standards for GCC
STD := -std=gnu99 # See man gcc for more options

# Protection for stack-smashing attack
# STACK := -fstack-protector-all -Wstack-protector

# Specifies to GCC the required warnings
WARNS := -Wall -Wextra -pedantic # -pedantic warns on language standards

# Flags for compiling
CFLAGS_OPT := -O3
CFLAGS := $(STD) $(STACK) $(WARNS) -fpic

# Debug options
DEBUG := -g3 -DDEBUG=1

# Dependency libraries
LIBS := # -lm  -I some/path/to/library

# Test libraries
TEST_LIBS := -l cmocka -L /usr/lib



# Tests binary file
TEST_BINARY := $(BINARY)_test_runner

# %.o file names
NAMES := $(notdir $(basename $(wildcard $(SRCDIR)/*.$(SRCEXT))))
SHAREDLIB := lib$(PROJECT_NAME).so
STATICLIB := lib$(PROJECT_NAME).a
OBJECTS :=$(patsubst %,$(LIBDIR)/%.o,$(NAMES))

#
# COMPILATION RULES
#

default: all

# Help message
help:
	@echo "C Project Template"
	@echo
	@echo "Target rules:"
	@echo "    all      - Compiles and generates binary file"
	@echo "    tests    - Compiles with cmocka and run tests binary file"
	@echo "    start    - Starts a new project using C project template"
	@echo "    valgrind - Runs binary file using valgrind tool"
	@echo "    clean    - Clean the project by removing binaries"
	@echo "    help     - Prints a help message with target rules"

# Starts a new project using C project template
start:
	@echo "Creating project: $(PROJECT_NAME)"
	@mkdir -pv $(PROJECT_PATH)
	@echo "Copying files from template to new directory:"
	@cp -rvf ./* $(PROJECT_PATH)/
	@echo
	@echo "Go to $(PROJECT_PATH) and compile your project: make"
	@echo "Then execute it: bin/$(BINARY) --help"
	@echo "Happy hacking o/"

EXPORTDIRS := $(EXPORTDIR)/lib $(EXPORTDIR)/include/$(PROJECT_NAME)

# Rule for link and generate the binary file
all: $(EXPORTDIRS) $(SHAREDLIB) $(STATICLIB)
	@echo -en "$(BROWN)LD $(END_COLOR)";
	$(CC) -o $(BINDIR)/$(BINARY) $(OBJECTS) $(DEBUG) $(CFLAGS) $(LIBS)
	cp $(SRCDIR)/*.h $(EXPORTDIR)/include/$(PROJECT_NAME)
	@echo -en "\n--\nBinary file placed at" \
			  "$(BROWN)$(BINDIR)/$(BINARY)$(END_COLOR)\n";

$(EXPORTDIRS):
	mkdir -p $@;

$(SHAREDLIB): $(OBJECTS)
	@echo -en "$(BROWN)CC $(END_COLOR)";
	gcc -shared -o $(LIBDIR)/$@ $(OBJECTS)
	cp $(LIBDIR)/$@ $(EXPORTDIR)/lib

$(STATICLIB): $(OBJECTS)
	@echo -en "$(BROWN)AR $(END_COLOR)";
	ar rcs $(LIBDIR)/$@ $^
	cp $(LIBDIR)/$@ $(EXPORTDIR)/lib

$(LIBDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS)

ARGS_TEST := test google.co.uk
ARGS_CSV := csv \
		0 \
		/home/princio/Repo/princio/malware_detection_preditc_file/top_10m.csv \
  		/tmp/top_10m.psltrie.csv

ARGSS := $(ARGS_CSV)

# Rule for run valgrind tool
valgrind:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--log-file=$(LOGDIR)/$@.log \
		$(BINDIR)/$(BINARY) $(PSLTSUFFIXFILE) $(ARGSS)
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

valgrind2:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--show-leak-kinds=all \
		--log-file=$(LOGDIR)/$@.log \
		--main-stacksize=838860800\
		$(BINDIR)/$(BINARY) $(PSLTSUFFIXFILE) google.co.uk
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

install:
	cp $(EXPORTDIR)/lib/$(SHAREDLIB) /usr/local/lib/$(SHAREDLIB)
	chmod 0755 /usr/local/lib/$(SHAREDLIB)
	cp -r $(EXPORTDIR)/include/$(PROJECT_NAME) /usr/local/include/$(PROJECT_NAME)

# Compile tests and run the test binary
tests:
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) $(TESTDIR)/main.c -o $(BINDIR)/$(TEST_BINARY) $(DEBUG) $(CFLAGS) $(LIBS) $(TEST_LIBS)
	@which ldconfig && ldconfig -C /tmp/ld.so.cache || true # caching the library linking
	@echo -en "$(BROWN) Running tests: $(END_COLOR)";
	./$(BINDIR)/$(TEST_BINARY)


# Rule for cleaning the project
clean:
	@rm -rvf $(BINDIR)/* $(LIBDIR)/* $(LOGDIR)/* $(EXPORTDIR)/*;

ca: clean all

ab: all
	clear; bin/binary $(PSLTSUFFIXFILE) google.co.uk

av: all valgrind

cab: clean all
	clear; bin/binary $(PSLTSUFFIXFILE) google.co.uk

cp: clean prod

cpb: clean prod
	clear; bin/binary_prod

cav: clean all valgrind

cav2: clean all valgrind2