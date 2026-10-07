CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.

# Single-TU build: main.cpp #includes all mutation .cpp files directly.
# The mutations/ files are NOT compiled separately here.
SRCS   = tokenizer.cpp main.cpp
TARGET = mutate

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

test: $(TARGET)
	cd tests && $(CXX) $(CXXFLAGS) -I.. -o test_runner test_runner.cpp ../tokenizer.cpp && ./test_runner

.PHONY: all clean test
