CC = g++
CFLAGS = -Wall -std=c++17 -pthread
SRC_DIR = src
TARGETS = cco_latency tlb_miss

all: $(TARGETS)

cco_latency: $(SRC_DIR)/cco_latency.cpp
	$(CC) $(CFLAGS) -o $@ $<

tlb_miss: $(SRC_DIR)/tlb_miss.cpp
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm $(TARGETS)