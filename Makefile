CC = clang++

EFlags	  := -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wdouble-promotion -Wformat=2 -Wnull-dereference -Wno-unused-parameter
CFLAGS    := -g -Iinclude -O0 -std=c++23 $(EFlags)
HEAD      := $(wildcard *.h)
SRC       := $(wildcard *.cpp)
OBJS      := $(addsuffix .o,$(basename $(SRC)))

TARGET    := driver

.PHONY: all clean

all : $(TARGET)

%.o: %.cpp
	@echo "  Compiling \"/$<\"..."
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET) : $(OBJS) $(HEAD)
	@echo "compiling target "$(TARGET)
	@$(CC) -o $(TARGET) $(CFLAGS) $(OBJS)

clean:
	@echo "  Cleaning..."
	@rm -f $(OBJS) $(TARGET)
