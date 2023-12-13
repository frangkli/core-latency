CC = g++
CFLAGS = -Wall -std=c++17 -O2 -pthread
SRC_DIR = src
TARGETS = cco_latency

all: $(TARGETS)

cco_latency: $(SRC_DIR)/cco_latency.cpp
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm $(TARGETS)