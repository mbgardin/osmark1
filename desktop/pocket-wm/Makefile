# pocket-wm Makefile
# Builds the Pocket OS Window Manager

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 \
          $(shell pkg-config --cflags x11 xrandr) \
          -D_GNU_SOURCE
LDFLAGS = $(shell pkg-config --libs x11 xrandr) -lXext

SRCS = main.c atoms.c draw.c window.c events.c
OBJS = $(SRCS:.c=.o)
TARGET = pocket-wm

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo "Built: $(TARGET)"

%.o: %.c pocket-wm.h
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/bin/$(TARGET)
	@echo "Installed: /usr/bin/$(TARGET)"

clean:
	rm -f $(OBJS) $(TARGET)
