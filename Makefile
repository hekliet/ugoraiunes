CC = gcc
CFLAGS =
LDLIBS = -lSDL2

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=build/%.o)
TARGET = ugoraiunes

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDLIBS)

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf build $(TARGET)
