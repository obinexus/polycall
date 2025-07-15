# QA Library Makefile
# OBINexus Polycall v2 Testing Framework

# Configuration
BUILD_DIR ?= ../../build/debug
COVERAGE ?= 1

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = $(BUILD_DIR)/obj/qa_lib
LIB_DIR = $(BUILD_DIR)/lib

# Compiler settings
CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Werror -std=c11 -fPIC -g
CFLAGS += -I$(INC_DIR)
ARFLAGS = rcs

# Coverage
ifeq ($(COVERAGE),1)
    CFLAGS += --coverage -fprofile-arcs -ftest-coverage
endif

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Libraries to build
STATIC_LIB = $(LIB_DIR)/libqa_framework.a
SHARED_LIB = $(LIB_DIR)/libqa_framework.so

# Individual component libraries
ASSERT_LIB = $(LIB_DIR)/libqa_assert.a
IOC_LIB = $(LIB_DIR)/libqa_ioc.a

.PHONY: all clean static shared components

all: static shared components

static: $(STATIC_LIB)

shared: $(SHARED_LIB)

components: $(ASSERT_LIB) $(IOC_LIB)

# Build rules
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(STATIC_LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(SHARED_LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Component libraries
$(ASSERT_LIB): $(OBJ_DIR)/qa_assert.o
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(IOC_LIB): $(OBJ_DIR)/qa_ioc.o
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(STATIC_LIB) $(SHARED_LIB) $(ASSERT_LIB) $(IOC_LIB)

# Install headers (for development)
install-headers: @mkdir -p 
	$(BUILD_DIR)/include/qa cp -r 
	$(INC_DIR)/* 
	$(BUILD_DIR)/include/qa/
