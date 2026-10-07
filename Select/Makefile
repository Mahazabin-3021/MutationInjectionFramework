CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -I./tree-sitter-deps/tree-sitter/lib/include
CFLAGS   = -w -I./tree-sitter-deps/tree-sitter/lib/include -I./tree-sitter-deps/tree-sitter/lib/src

TS_SRCS = tree-sitter-deps/tree-sitter/lib/src/lib.c \
          tree-sitter-deps/tree-sitter-c/src/parser.c \
          tree-sitter-deps/tree-sitter-cpp/src/parser.c \
          tree-sitter-deps/tree-sitter-cpp/src/scanner.c \
          tree-sitter-deps/tree-sitter-java/src/parser.c \
          tree-sitter-deps/tree-sitter-c-sharp/src/parser.c \
          tree-sitter-deps/tree-sitter-c-sharp/src/scanner.c

TS_OBJS = $(TS_SRCS:.c=.o)

SRCS = select.cpp lang_registry.cpp file_walker.cpp \
       function_extractor.cpp selector.cpp validator.cpp \
       ../Mutate/tokenizer.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = select

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(TS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: tests/test_select.cpp lang_registry.o file_walker.o \
      function_extractor.o selector.o validator.o $(TS_OBJS)
	$(CXX) $(CXXFLAGS) -o test_select $^
	./test_select

clean:
	rm -f $(TARGET) test_select $(OBJS) $(TS_OBJS)
