OUT := boa
LIB_OUT := $(shell echo $(OUT) | tr a-z A-Z)# Make library name uppercase
OUT_DIR := out

CC := g++
AR := ar
CFLAGS := -std=c++14 -O2 -c
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

INCLUDE_DIRECTORIES := -Iinclude
LIBRARY_DIRECTORIES := -Llib
LIBRARIES := -lSOIL -lGL -lglfw -lGLEW

.PHONY : all clean
all: $(OBJS)
	$(AR) $(ARFLAGS) $(OUT_DIR)/lib$(LIB_OUT).a $(OBJS)
	cp $(SRC_DIR)/$(OUT).h $(OUT_DIR)/$(OUT).h

shared: CFLAGS += -fpic
shared: $(OBJS)
	$(CC) -shared $(OBJS) $(INCLUDE_DIRECTORIES) $(LIBRARY_DIRECTORIES) $(LIBRARIES) -o $(OUT_DIR)/lib$(LIB_OUT).so # Not sure if linking libraries works when compiling shared libraries
	cp $(SRC_DIR)/$(OUT).h $(OUT_DIR)/$(OUT).h

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) $< $(INCLUDE_DIRECTORIES) -o $@

debug: CFLAGS += -g -D DEBUG_MODE
debug: clean all

run: all
	./$(OUT_DIR)/$(OUT)

clean:
	rm -f $(OUT_DIR)/$(OUT) $(OBJS)
