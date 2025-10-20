CC = gcc
CFLAGS = -O3
TARGET = deepsearch
SRCS = DeepSearch.c board-evaluate.c const.c insert.c min-board-trie.c board-data.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
