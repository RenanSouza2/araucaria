PRJ_NAME = araucaria
PRJ_DIR ?= $(shell git rev-parse --show-toplevel)
LIB_DIR = $(PRJ_DIR)/lib
DBG_FULL_FILE = $(LIB_DIR)/debug_full.o
DBG_FILE = $(LIB_DIR)/debug.o
LIB_FILE = $(LIB_DIR)/lib.o
CLU_FILE = $(PRJ_DIR)/mods/clu/bin/clu.o
DIR = $(shell basename $(CURDIR))
