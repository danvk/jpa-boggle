CC = gcc
CXX = g++
CFLAGS = -O3
CXXFLAGS = -O3 -std=c++20
TARGET = deepsearch
SRCS = DeepSearch.cc board-evaluate.c const.c insert.cc trie.cc
OBJS = $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SRCS)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
