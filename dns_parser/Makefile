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



# Source code directory structure
BINDIR := bin
SRCDIR := src
LOGDIR := log
ALLLIBDIR := lib/all
PRODLIBDIR := lib/prod
TESTDIR := test


# Source code file extension
SRCEXT := c


# Defines the C Compiler
CC := gcc


# Defines the language standards for GCC
STD := -std=gnu99 # See man gcc for more options

# Protection for stack-smashing attack
STACK := #-Wstack-protector # -fstack-protector-all

# Specifies to GCC the required warnings
WARNS := -Wall -Wextra -pedantic # -pedantic warns on language standards

NOWARNS := -Wno-unused-parameter -Wno-unused-variable -Wno-unused-variable -Wno-unused-but-set-variable

INCLUDES := -I/usr/include/postgresql/

# Flags for compiling
# CFLAGS := -O3 $(STD) $(STACK) $(WARNS) -I/usr/include/postgresql/
CFLAGS := $(STD) $(STACK) $(WARNS) $(NOWARNS) -I/usr/include/postgresql/
PROD_CFLAGS := -O3 $(STD) $(STACK) $(WARNS) $(NOWARNS) -I/usr/include/postgresql/
DEBUG := -g3 -DDEBUG=1

# Dependency libraries
POSTGRES_LIBS := -lpq  -lm -lssl -lcrypto -ldl -pthread -lncurses # -lpgcommon -lpgport -lgssapi_krb5 -lldap_r 
LIBS := -lpcap -lidn -lregdom  $(POSTGRES_LIBS) #-I /home/princio/Repo/b64.c/build/libb64c.so # -lm  -I some/path/to/library

# Test libraries
TEST_LIBS := -l cmocka -L /usr/lib



# Tests binary file
TEST_BINARY := $(BINARY)_test_runner
PROD_BINARY := $(BINARY)_prod



# %.o file names
NAMES := $(notdir $(basename $(wildcard $(SRCDIR)/*.$(SRCEXT))))

ALLOBJECTS :=$(patsubst %,$(ALLLIBDIR)/%.o,$(NAMES))

PRODOBJECTS :=$(patsubst %,$(PRODLIBDIR)/%.o,$(NAMES))


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


# Rule for link and generate the binary file
all: $(ALLOBJECTS)
	@echo -en "$(BROWN)LD $(END_COLOR)";
	$(CC) -o $(BINDIR)/$(BINARY) $+ $(DEBUG) $(CFLAGS) $(LIBS)
	@echo -en "\n--\nBinary file placed at" \
			  "$(BROWN)$(BINDIR)/$(BINARY)$(END_COLOR)\n";

prod: $(PRODOBJECTS)
	@echo -en "$(BROWN)LD $(END_COLOR)";
	$(CC) -o $(BINDIR)/$(PROD_BINARY) $+ $(PROD_CFLAGS) $(LIBS)
	@echo -en "\n--\nBinary file placed at" \
			  "$(BROWN)$(BINDIR)/$(PROD_BINARY)$(END_COLOR)\n";

# Rule for object binaries compilation
$(ALLLIBDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS)

# Rule for object binaries compilation
$(PRODLIBDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(PROD_CFLAGS) $(LIBS)


# Rule for run valgrind tool
valgrind:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--log-file=$(LOGDIR)/$@.log \
		--main-stacksize=838860800\
		$(BINDIR)/$(BINARY)
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

valgrind_prod:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--log-file=$(LOGDIR)/$@.log \
		--main-stacksize=838860800\
		$(BINDIR)/$(BINARY)_prod
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

valgrind2:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--show-leak-kinds=all \
		--log-file=$(LOGDIR)/$@.log \
		--main-stacksize=838860800\
		$(BINDIR)/$(BINARY)
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

# Compile tests and run the test binary
tests:
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) $(TESTDIR)/main.c -o $(BINDIR)/$(TEST_BINARY) $(DEBUG) $(CFLAGS) $(LIBS) $(TEST_LIBS)
	@which ldconfig && ldconfig -C /tmp/ld.so.cache || true # caching the library linking
	@echo -en "$(BROWN) Running tests: $(END_COLOR)";
	./$(BINDIR)/$(TEST_BINARY)


# Rule for cleaning the project
clean:
	@rm -rvf $(BINDIR)/* $(ALLLIBDIR)/* $(LOGDIR)/* $(PRODLIBDIR)/* ;

ca: clean all

ab: all
	clear; bin/binary

av: all valgrind

cab: clean all
	clear; bin/binary

cp: clean prod

cpb: clean prod
	clear; bin/binary_prod

cav: clean all valgrind

cav2: clean all valgrind2
