CC = gcc
CXX = g++
CFLAGS = -O3
CXXFLAGS = -O3 -std=c++20
TARGET = deepsearch
TARGET2 = gunsofnavarone
SRCS = DeepSearch.cc insert.cc trie.cc
OBJS = $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SRCS)))

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TARGET2): GunsOfNavarone.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET2)

.PHONY: all clean
